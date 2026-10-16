// Matchball Tennis - Kern: Match-Zustandsmaschine

#include "Match/Kern/MBMatchAblauf.h"

namespace MB
{
	const char* MatchZustandName(EMatchZustand Zustand)
	{
		switch (Zustand)
		{
		case EMatchZustand::Vorbereitung: return "Vorbereitung";
		case EMatchZustand::Aufschlagvorbereitung: return "Aufschlagvorbereitung";
		case EMatchZustand::Aufschlag: return "Aufschlag";
		case EMatchZustand::Ballwechsel: return "Ballwechsel";
		case EMatchZustand::Punktende: return "Punktende";
		case EMatchZustand::ChallengeFenster: return "ChallengeFenster";
		case EMatchZustand::Videobeweis: return "Videobeweis";
		case EMatchZustand::Ansage: return "Ansage";
		case EMatchZustand::ZwischenDenPunkten: return "ZwischenDenPunkten";
		case EMatchZustand::Seitenwechsel: return "Seitenwechsel";
		case EMatchZustand::Satzpause: return "Satzpause";
		case EMatchZustand::Matchende: return "Matchende";
		}
		return "Unbekannt";
	}

	FMatchAblauf::FMatchAblauf(const FMatchEinstellungen& InEinstellungen)
		: Einstellungen(InEinstellungen)
		, Stand(InEinstellungen.Regeln, InEinstellungen.ErsterAufschlaeger, InEinstellungen.SeiteSpieler0)
		, Konto(InEinstellungen.ChallengesProSatz, InEinstellungen.ChallengesZusatzTiebreak, InEinstellungen.bChallenges && !InEinstellungen.Linienrichter.bElektronisch)
		, Richter(InEinstellungen.Platz, InEinstellungen.Ball, InEinstellungen.Linienrichter)
		, Zufall(InEinstellungen.Startwert)
	{
		if (Stand.IstTiebreak())
		{
			// Match besteht nur aus einem Match-Tiebreak
			Konto.TiebreakBeginnt();
		}
		ZustandDauer = Einstellungen.Zeiten.Vorbereitung;
		FAblaufMeldung Meldung;
		Meldung.Art = EMeldungArt::Zustand;
		Meldung.Zustand = Zustand;
		Melde(Meldung);
	}

	void FMatchAblauf::Wechsle(EMatchZustand Neu)
	{
		Zustand = Neu;
		ZeitImZustand = 0.0;
		const FAblaufZeiten& Zeiten = Einstellungen.Zeiten;
		switch (Neu)
		{
		case EMatchZustand::Vorbereitung: ZustandDauer = Zeiten.Vorbereitung; break;
		case EMatchZustand::Punktende: ZustandDauer = Zeiten.Punktende; break;
		case EMatchZustand::ChallengeFenster: ZustandDauer = Zeiten.ChallengeFenster; break;
		case EMatchZustand::Videobeweis: ZustandDauer = Zeiten.Videobeweis; break;
		case EMatchZustand::Ansage: ZustandDauer = Zeiten.Ansage; break;
		case EMatchZustand::ZwischenDenPunkten: ZustandDauer = Zeiten.ZwischenDenPunkten; break;
		case EMatchZustand::Seitenwechsel: ZustandDauer = Zeiten.Seitenwechsel; break;
		case EMatchZustand::Satzpause: ZustandDauer = Zeiten.Satzpause; break;
		default: ZustandDauer = 0.0; break;
		}

		if (Neu == EMatchZustand::Aufschlagvorbereitung)
		{
			// Neuer Aufschlag: Zufall neu starten (online auf beiden PCs gleich)
			++AufschlagNummer;
			Zufall.Starte(MischeStartwert(Einstellungen.Startwert, AufschlagNummer));
			LetzterSchlagender = -1;
			SchlaegeImBallwechsel = 0;
			Ausgang = FAusgang();
			LetzteEntscheidung = FRichterEntscheidung();
			bChallengeAngefordert = false;
			ChallengeErgebnis = FChallengeAuswertung();
			bKlarerWinner = false;
			bRueckschlaegerChancenlos = false;
		}

		FAblaufMeldung Meldung;
		Meldung.Art = EMeldungArt::Zustand;
		Meldung.Zustand = Neu;
		Melde(Meldung);
	}

	std::vector<FAblaufMeldung> FMatchAblauf::HoleMeldungen()
	{
		std::vector<FAblaufMeldung> Ergebnis;
		Ergebnis.swap(Meldungen);
		return Ergebnis;
	}

	bool FMatchAblauf::IstBallImSpiel() const
	{
		if (Zustand != EMatchZustand::Ballwechsel || Richter.IstAbgeschlossen())
		{
			return false;
		}
		return Richter.ErsterAufsprungBeurteilt() || Richter.BallAufGegenseite();
	}

	void FMatchAblauf::Fortschreiten(double Dt)
	{
		if (Zustand == EMatchZustand::Matchende)
		{
			return;
		}
		ZeitImZustand += Dt;
		const bool bZeitgesteuert = Zustand == EMatchZustand::Vorbereitung || Zustand == EMatchZustand::Punktende || Zustand == EMatchZustand::ChallengeFenster
			|| Zustand == EMatchZustand::Videobeweis || Zustand == EMatchZustand::Ansage || Zustand == EMatchZustand::ZwischenDenPunkten
			|| Zustand == EMatchZustand::Seitenwechsel || Zustand == EMatchZustand::Satzpause;
		if (!bZeitgesteuert || ZeitImZustand < ZustandDauer)
		{
			return;
		}
		switch (Zustand)
		{
		case EMatchZustand::Vorbereitung:
		case EMatchZustand::ZwischenDenPunkten:
		case EMatchZustand::Seitenwechsel:
		case EMatchZustand::Satzpause:
			Wechsle(EMatchZustand::Aufschlagvorbereitung);
			break;
		case EMatchZustand::Punktende:
			NachPunktende();
			break;
		case EMatchZustand::ChallengeFenster:
			BeginneAnsage();
			break;
		case EMatchZustand::Videobeweis:
		{
			FAblaufMeldung Meldung;
			Meldung.Art = EMeldungArt::ChallengeErgebnis;
			Meldung.Entscheidung = LetzteEntscheidung;
			Meldung.Challenge = ChallengeErgebnis;
			Meldung.Ansage = AnsageChallengeErgebnis(LetzteEntscheidung.Strittig, ChallengeErgebnis);
			Melde(Meldung);

			const int Aufschlaeger = Stand.GetAufschlaeger();
			switch (ChallengeErgebnis.Folge)
			{
			case EChallengeFolge::EntscheidungBleibt:
				break;
			case EChallengeFolge::ErsterAufschlag:
			case EChallengeFolge::PunktWiederholen:
				Ausgang = FAusgang{EAusgangArt::Wiederholung, -1, EPunktGrund::Challenge};
				break;
			case EChallengeFolge::AssFuerAufschlaeger:
				Ausgang = FAusgang{EAusgangArt::Punkt, Aufschlaeger, EPunktGrund::Ass};
				break;
			case EChallengeFolge::Aufschlagfehler:
				Ausgang = FAusgang{EAusgangArt::Fehler, -1, EPunktGrund::Challenge};
				break;
			case EChallengeFolge::Doppelfehler:
				Ausgang = FAusgang{EAusgangArt::Punkt, 1 - Aufschlaeger, EPunktGrund::Doppelfehler};
				break;
			case EChallengeFolge::PunktFuerSchlagenden:
				Ausgang = FAusgang{EAusgangArt::Punkt, LetzteEntscheidung.Strittig.Schlagender, EPunktGrund::Winner};
				break;
			case EChallengeFolge::PunktFuerChallenger:
				Ausgang = FAusgang{EAusgangArt::Punkt, ChallengeErgebnis.Punktgewinner, EPunktGrund::Challenge};
				break;
			}
			BeginneAnsage();
			break;
		}
		case EMatchZustand::Ansage:
			NachAnsage();
			break;
		default:
			break;
		}
	}

	bool FMatchAblauf::AufschlagGeschlagen()
	{
		if (Zustand != EMatchZustand::Aufschlagvorbereitung)
		{
			return false;
		}
		const int Aufschlaeger = Stand.GetAufschlaeger();
		FSchlagKontext Kontext;
		Kontext.Schlagender = Aufschlaeger;
		Kontext.Seite = Stand.GetSeite(Aufschlaeger);
		Kontext.bAufschlag = true;
		Kontext.Haelfte = Stand.GetAufschlagHaelfte();
		Kontext.bZweiterAufschlag = bZweiterAufschlag;
		Richter.NeuerSchlag(Kontext);
		LetzterSchlagender = Aufschlaeger;
		SchlaegeImBallwechsel = 1;
		Wechsle(EMatchZustand::Aufschlag);
		return true;
	}

	bool FMatchAblauf::BallGeschlagen(int Spieler)
	{
		if ((Spieler != 0 && Spieler != 1) || Spieler == LetzterSchlagender || !IstBallImSpiel())
		{
			return false;
		}
		FSchlagKontext Kontext;
		Kontext.Schlagender = Spieler;
		Kontext.Seite = Stand.GetSeite(Spieler);
		Richter.NeuerSchlag(Kontext);
		LetzterSchlagender = Spieler;
		++SchlaegeImBallwechsel;
		return true;
	}

	void FMatchAblauf::BallEreignis(const FBallEreignis& Ereignis)
	{
		if (Zustand != EMatchZustand::Aufschlag && Zustand != EMatchZustand::Ballwechsel)
		{
			return;
		}
		const FRichterEntscheidung Entscheidung = Richter.Ereignis(Ereignis, Zufall);
		switch (Entscheidung.Signal)
		{
		case ERichterSignal::Keins:
			break;
		case ERichterSignal::BallGut:
			LetzteEntscheidung = Entscheidung;
			if (Zustand == EMatchZustand::Aufschlag)
			{
				Wechsle(EMatchZustand::Ballwechsel);
			}
			break;
		case ERichterSignal::Fehler:
		case ERichterSignal::Let:
		case ERichterSignal::PunktEnde:
			PunktentscheidungEingetroffen(Entscheidung);
			break;
		}
	}

	void FMatchAblauf::PunktentscheidungEingetroffen(const FRichterEntscheidung& Entscheidung)
	{
		LetzteEntscheidung = Entscheidung;
		const bool bAufschlag = Richter.GetKontext().bAufschlag;
		switch (Entscheidung.Signal)
		{
		case ERichterSignal::Fehler:
			Ausgang = FAusgang{EAusgangArt::Fehler, -1, Entscheidung.Grund};
			break;
		case ERichterSignal::Let:
			Ausgang = FAusgang{EAusgangArt::Let, -1, EPunktGrund::Keiner};
			break;
		default:
			Ausgang = FAusgang{EAusgangArt::Punkt, Entscheidung.Punktgewinner, Entscheidung.Grund};
			break;
		}

		// Rufe von Linienrichter/Schiedsrichter
		const char* Ruf = nullptr;
		if (Entscheidung.Signal == ERichterSignal::Let)
		{
			Ruf = "let";
		}
		else if (bAufschlag && (Entscheidung.Signal == ERichterSignal::Fehler || Entscheidung.Grund == EPunktGrund::Doppelfehler))
		{
			Ruf = "fault";
		}
		else if (Entscheidung.bAusgerufen)
		{
			Ruf = "out";
		}
		if (Ruf != nullptr)
		{
			FAblaufMeldung Meldung;
			Meldung.Art = EMeldungArt::Ruf;
			Meldung.Ansage = AnsageRuf(Ruf);
			Meldung.Entscheidung = Entscheidung;
			Melde(Meldung);
		}
		Wechsle(EMatchZustand::Punktende);
	}

	int FMatchAblauf::ChallengeBerechtigter() const
	{
		if (!LetzteEntscheidung.bStrittig || !Konto.IstAktiv())
		{
			return -1;
		}
		const int Spieler = Benachteiligter(LetzteEntscheidung.Strittig);
		return Konto.KannChallengen(Spieler, LetzteEntscheidung.Strittig) ? Spieler : -1;
	}

	void FMatchAblauf::NachPunktende()
	{
		const int Berechtigt = ChallengeBerechtigter();
		const bool bKnapp = LetzteEntscheidung.Strittig.Abstand <= Einstellungen.ChallengeFensterMaxAbstand;
		if (Berechtigt >= 0 && bKnapp && Ausgang.Art != EAusgangArt::Let)
		{
			FAblaufMeldung Meldung;
			Meldung.Art = EMeldungArt::ChallengeMoeglich;
			Meldung.Spieler = Berechtigt;
			Meldung.Entscheidung = LetzteEntscheidung;
			Melde(Meldung);
			Wechsle(EMatchZustand::ChallengeFenster);
			return;
		}
		BeginneAnsage();
	}

	void FMatchAblauf::SetzeChallengeUmstaende(bool bInKlarerWinner, bool bInRueckschlaegerChancenlos)
	{
		bKlarerWinner = bInKlarerWinner;
		bRueckschlaegerChancenlos = bInRueckschlaegerChancenlos;
	}

	bool FMatchAblauf::ChallengeAnfordern(int Spieler)
	{
		if (Zustand != EMatchZustand::Punktende && Zustand != EMatchZustand::ChallengeFenster)
		{
			return false;
		}
		if (bChallengeAngefordert || !LetzteEntscheidung.bStrittig || Ausgang.Art == EAusgangArt::Let)
		{
			return false;
		}
		FStrittigeEntscheidung Strittig = LetzteEntscheidung.Strittig;
		Strittig.bKlarerWinner = bKlarerWinner;
		Strittig.bRueckschlaegerChancenlos = bRueckschlaegerChancenlos;
		FChallengeAuswertung Auswertung;
		if (!Konto.Challenge(Spieler, Strittig, Auswertung))
		{
			return false;
		}
		LetzteEntscheidung.Strittig = Strittig;
		bChallengeAngefordert = true;
		ChallengeErgebnis = Auswertung;

		FAblaufMeldung Meldung;
		Meldung.Art = EMeldungArt::ChallengeAngefordert;
		Meldung.Spieler = Spieler;
		Meldung.Entscheidung = LetzteEntscheidung;
		Meldung.Ansage = AnsageChallenge(Spieler, LetzteEntscheidung.Linie.Wahrheit.Linie, LetzteEntscheidung.Kontaktflaeche.Beginn.Y);
		Melde(Meldung);
		Wechsle(EMatchZustand::Videobeweis);
		return true;
	}

	void FMatchAblauf::BeginneAnsage()
	{
		LetzteFolge = FPunktFolge();
		FAnsage Ansage;
		switch (Ausgang.Art)
		{
		case EAusgangArt::Punkt:
		{
			LetzteFolge = Stand.PunktFuer(Ausgang.Punktgewinner);
			bZweiterAufschlag = false;
			if (LetzteFolge.bSatzGewonnen && !LetzteFolge.bMatchGewonnen)
			{
				Konto.NeuerSatz();
			}
			if (LetzteFolge.bTiebreakBeginnt || LetzteFolge.bMatchTiebreakBeginnt)
			{
				Konto.TiebreakBeginnt();
			}
			FAblaufMeldung Meldung;
			Meldung.Art = EMeldungArt::PunktGezaehlt;
			Meldung.Spieler = Ausgang.Punktgewinner;
			Meldung.Grund = Ausgang.Grund;
			Meldung.Folge = LetzteFolge;
			Melde(Meldung);
			Ansage = AnsageNachPunkt(Stand, LetzteFolge);
			if (LetzteFolge.bNeueBaelle)
			{
				FAblaufMeldung Baelle;
				Baelle.Art = EMeldungArt::NeueBaelle;
				Baelle.Ansage = AnsageRuf("new_balls_please");
				Melde(Baelle);
			}
			break;
		}
		case EAusgangArt::Fehler:
			bZweiterAufschlag = true;
			break;
		case EAusgangArt::Wiederholung:
			bZweiterAufschlag = false;
			break;
		case EAusgangArt::Let:
		case EAusgangArt::Keiner:
			break;
		}
		if (!Ansage.IstLeer())
		{
			FAblaufMeldung Meldung;
			Meldung.Art = EMeldungArt::Ansage;
			Meldung.Ansage = Ansage;
			Meldung.Folge = LetzteFolge;
			Melde(Meldung);
		}
		Wechsle(EMatchZustand::Ansage);
		if (Ausgang.Art != EAusgangArt::Punkt)
		{
			ZustandDauer = Einstellungen.Zeiten.NachFehler;
		}
	}

	void FMatchAblauf::NachAnsage()
	{
		if (Stand.IstBeendet())
		{
			Sieger = Stand.GetSieger();
			FAblaufMeldung Meldung;
			Meldung.Art = EMeldungArt::Matchende;
			Meldung.Spieler = Sieger;
			Melde(Meldung);
			Wechsle(EMatchZustand::Matchende);
			return;
		}
		if (Ausgang.Art != EAusgangArt::Punkt)
		{
			Wechsle(EMatchZustand::Aufschlagvorbereitung);
			return;
		}
		if (LetzteFolge.Pause == EPause::Satzpause)
		{
			Wechsle(EMatchZustand::Satzpause);
		}
		else if (LetzteFolge.bSeitenwechsel)
		{
			Wechsle(EMatchZustand::Seitenwechsel);
			ZustandDauer = LetzteFolge.Pause == EPause::Seitenwechsel ? Einstellungen.Zeiten.Seitenwechsel : Einstellungen.Zeiten.SeitenwechselOhnePause;
		}
		else
		{
			Wechsle(EMatchZustand::ZwischenDenPunkten);
		}
	}

	void FMatchAblauf::Ueberspringen()
	{
		switch (Zustand)
		{
		case EMatchZustand::Vorbereitung:
		case EMatchZustand::ChallengeFenster:
		case EMatchZustand::Videobeweis:
		case EMatchZustand::Ansage:
		case EMatchZustand::ZwischenDenPunkten:
		case EMatchZustand::Seitenwechsel:
		case EMatchZustand::Satzpause:
			ZeitImZustand = ZustandDauer;
			Fortschreiten(0.0);
			break;
		default:
			break;
		}
	}

	void FMatchAblauf::Aufgabe(int Spieler)
	{
		if (Zustand == EMatchZustand::Matchende || (Spieler != 0 && Spieler != 1))
		{
			return;
		}
		Sieger = 1 - Spieler;
		FAblaufMeldung Meldung;
		Meldung.Art = EMeldungArt::Matchende;
		Meldung.Spieler = Sieger;
		Meldung.Grund = EPunktGrund::Aufgabe;
		Melde(Meldung);
		Wechsle(EMatchZustand::Matchende);
	}
}
