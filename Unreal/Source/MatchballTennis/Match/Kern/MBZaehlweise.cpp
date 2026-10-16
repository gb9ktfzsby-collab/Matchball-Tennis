// Matchball Tennis - Kern: Zaehlweise

#include "Match/Kern/MBZaehlweise.h"

namespace MB
{
	const char* TennisPunktName(int Punkte)
	{
		switch (Punkte)
		{
		case 0: return "0";
		case 1: return "15";
		case 2: return "30";
		default: return "40";
		}
	}

	void FZaehlRegeln::Normalisiere()
	{
		Gewinnsaetze = Begrenze(Gewinnsaetze, 1, 3);
		SpieleProSatz = Begrenze(SpieleProSatz, 1, 6);
		TiebreakBis = Begrenze(TiebreakBis, 1, 20);
		LangerTiebreakBis = Begrenze(LangerTiebreakBis, 1, 20);
		NeueBaelleErstesMal = MaxWert(NeueBaelleErstesMal, 0);
		NeueBaelleDanach = MaxWert(NeueBaelleDanach, 1);
	}

	FZaehlweise::FZaehlweise()
		: FZaehlweise(FZaehlRegeln(), 0, EPlatzseite::Nah)
	{
	}

	FZaehlweise::FZaehlweise(const FZaehlRegeln& InRegeln, int InErsterAufschlaeger, EPlatzseite InSeiteSpieler0)
		: Regeln(InRegeln)
		, SpielAufschlaeger(InErsterAufschlaeger == 1 ? 1 : 0)
		, SeiteSpieler0(InSeiteSpieler0)
	{
		Regeln.Normalisiere();
		TiebreakZiel = Regeln.TiebreakBis;
		// Sonderfall: Match besteht nur aus dem Entscheidungssatz (1 Gewinnsatz) und dieser ist ein Match-Tiebreak
		if (IstEntscheidungssatz() && Regeln.Entscheidungssatz == EEntscheidungssatz::MatchTiebreak)
		{
			bTiebreak = true;
			bMatchTiebreakLaeuft = true;
			TiebreakZiel = Regeln.LangerTiebreakBis;
		}
	}

	bool FZaehlweise::IstEntscheidungssatz() const
	{
		const int Grenze = Regeln.Gewinnsaetze - 1;
		return SaetzeGewonnen[0] == Grenze && SaetzeGewonnen[1] == Grenze;
	}

	int FZaehlweise::GetAufschlaeger() const
	{
		if (!bTiebreak)
		{
			return SpielAufschlaeger;
		}
		// Tiebreak: erster Punkt der "Spielaufschlaeger", danach jeweils zwei Punkte im Wechsel
		const int Gespielt = Punkte[0] + Punkte[1];
		return ((Gespielt + 1) / 2) % 2 == 0 ? SpielAufschlaeger : 1 - SpielAufschlaeger;
	}

	EAufschlagHaelfte FZaehlweise::GetAufschlagHaelfte() const
	{
		// Gerade Anzahl gespielter Punkte: von rechts (Einstand), ungerade: von links (Vorteil).
		// Gilt auch im Tiebreak und beim No-Ad-Entscheidungspunkt (3:3 = 6 Punkte -> Einstand).
		return ((Punkte[0] + Punkte[1]) % 2 == 0) ? EAufschlagHaelfte::Einstand : EAufschlagHaelfte::Vorteil;
	}

	EPlatzseite FZaehlweise::GetSeite(int Spieler) const
	{
		return Spieler == 0 ? SeiteSpieler0 : Gegenseite(SeiteSpieler0);
	}

	int FZaehlweise::GetPunkte(int Spieler) const
	{
		return (Spieler == 0 || Spieler == 1) ? Punkte[Spieler] : 0;
	}

	int FZaehlweise::GetSpiele(int Spieler) const
	{
		return (Spieler == 0 || Spieler == 1) ? Spiele[Spieler] : 0;
	}

	int FZaehlweise::GetGewonneneSaetze(int Spieler) const
	{
		return (Spieler == 0 || Spieler == 1) ? SaetzeGewonnen[Spieler] : 0;
	}

	bool FZaehlweise::IstEinstand() const
	{
		return !bTiebreak && !Regeln.bNoAd && Punkte[0] >= 3 && Punkte[1] >= 3 && Punkte[0] == Punkte[1];
	}

	int FZaehlweise::GetVorteil() const
	{
		if (bTiebreak || Regeln.bNoAd || Punkte[0] < 3 || Punkte[1] < 3)
		{
			return -1;
		}
		if (Punkte[0] == Punkte[1] + 1)
		{
			return 0;
		}
		if (Punkte[1] == Punkte[0] + 1)
		{
			return 1;
		}
		return -1;
	}

	bool FZaehlweise::TiebreakFaellig() const
	{
		const int N = Regeln.SpieleProSatz;
		if (N <= 1)
		{
			return false;
		}
		if (IstEntscheidungssatz() && Regeln.Entscheidungssatz == EEntscheidungssatz::Ausspielen)
		{
			return false;
		}
		return Spiele[0] == N && Spiele[1] == N;
	}

	bool FZaehlweise::SatzEntschieden(int& OutGewinner) const
	{
		const int N = Regeln.SpieleProSatz;
		for (int Spieler = 0; Spieler < 2; ++Spieler)
		{
			const int Gegner = 1 - Spieler;
			if (N <= 1)
			{
				if (Spiele[Spieler] >= 1)
				{
					OutGewinner = Spieler;
					return true;
				}
				continue;
			}
			if (Spiele[Spieler] >= N && Spiele[Spieler] - Spiele[Gegner] >= 2)
			{
				OutGewinner = Spieler;
				return true;
			}
		}
		return false;
	}

	FPunktFolge FZaehlweise::PunktFuer(int Spieler)
	{
		FPunktFolge Folge;
		if (Sieger >= 0 || (Spieler != 0 && Spieler != 1))
		{
			return Folge;
		}
		Folge.Punktgewinner = Spieler;
		const int Gegner = 1 - Spieler;
		const int AufschlaegerVorher = GetAufschlaeger();

		++Punkte[Spieler];

		if (bTiebreak)
		{
			if (Punkte[Spieler] >= TiebreakZiel && Punkte[Spieler] - Punkte[Gegner] >= 2)
			{
				// Tiebreak gewonnen: zaehlt als ein Spiel und entscheidet den Satz
				Folge.bSpielGewonnen = true;
				Folge.Spielgewinner = Spieler;
				++Spiele[Spieler];
				++SpieleGesamt;
				++SpieleSeitBallwechsel;
				SatzGewonnen(Spieler, Folge);
			}
			else if ((Punkte[0] + Punkte[1]) % 6 == 0)
			{
				// Seitenwechsel alle 6 Punkte, ohne Pause
				Folge.bSeitenwechsel = true;
				SeiteSpieler0 = Gegenseite(SeiteSpieler0);
			}
		}
		else
		{
			bool bSpielVorbei = false;
			if (Regeln.bNoAd)
			{
				bSpielVorbei = Punkte[Spieler] >= 4;
			}
			else
			{
				bSpielVorbei = Punkte[Spieler] >= 4 && Punkte[Spieler] - Punkte[Gegner] >= 2;
				// Einstand normalisieren (Zahlen bleiben klein)
				if (!bSpielVorbei && Punkte[0] >= 3 && Punkte[1] >= 3 && Punkte[0] == Punkte[1])
				{
					Punkte[0] = 3;
					Punkte[1] = 3;
				}
			}
			if (bSpielVorbei)
			{
				SpielGewonnen(Spieler, Folge);
			}
		}

		if (Sieger < 0)
		{
			Folge.bAufschlagwechsel = GetAufschlaeger() != AufschlaegerVorher;
		}
		return Folge;
	}

	void FZaehlweise::SpielGewonnen(int Gewinner, FPunktFolge& Folge)
	{
		Folge.bSpielGewonnen = true;
		Folge.Spielgewinner = Gewinner;
		Folge.bBreak = Gewinner != SpielAufschlaeger;
		++Spiele[Gewinner];
		++SpieleGesamt;
		++SpieleSeitBallwechsel;
		Punkte[0] = 0;
		Punkte[1] = 0;

		int SatzGewinner = -1;
		if (SatzEntschieden(SatzGewinner))
		{
			SatzGewonnen(SatzGewinner, Folge);
			return;
		}

		BeginneNeuesSpiel(Folge);
	}

	void FZaehlweise::BeginneNeuesSpiel(FPunktFolge& Folge)
	{
		const int SpieleImSatz = Spiele[0] + Spiele[1];
		if (SpieleImSatz % 2 == 1)
		{
			Folge.bSeitenwechsel = true;
			SeiteSpieler0 = Gegenseite(SeiteSpieler0);
			// Nach dem ersten Spiel eines Satzes ohne Pause
			Folge.Pause = SpieleImSatz == 1 ? EPause::Keine : EPause::Seitenwechsel;
		}
		SpielAufschlaeger = 1 - SpielAufschlaeger;

		if (TiebreakFaellig())
		{
			bTiebreak = true;
			Folge.bTiebreakBeginnt = true;
			const bool bLang = IstEntscheidungssatz() && Regeln.Entscheidungssatz == EEntscheidungssatz::Tiebreak10;
			TiebreakZiel = bLang ? Regeln.LangerTiebreakBis : Regeln.TiebreakBis;
		}
		PruefeNeueBaelle(Folge, bTiebreak);
	}

	void FZaehlweise::SatzGewonnen(int Gewinner, FPunktFolge& Folge)
	{
		FSatzErgebnis Ergebnis;
		Ergebnis.Spiele[0] = Spiele[0];
		Ergebnis.Spiele[1] = Spiele[1];
		Ergebnis.bMitTiebreak = bTiebreak && !bMatchTiebreakLaeuft;
		Ergebnis.bMatchTiebreak = bMatchTiebreakLaeuft;
		if (bTiebreak)
		{
			Ergebnis.TiebreakPunkte[0] = Punkte[0];
			Ergebnis.TiebreakPunkte[1] = Punkte[1];
		}
		Ergebnis.Gewinner = Gewinner;
		Saetze.push_back(Ergebnis);
		++SaetzeGewonnen[Gewinner];

		Folge.bSatzGewonnen = true;
		Folge.Satzgewinner = Gewinner;

		const int SpieleImSatz = Spiele[0] + Spiele[1];
		Punkte[0] = 0;
		Punkte[1] = 0;
		Spiele[0] = 0;
		Spiele[1] = 0;
		bTiebreak = false;
		bMatchTiebreakLaeuft = false;
		TiebreakZiel = Regeln.TiebreakBis;

		if (SaetzeGewonnen[Gewinner] >= Regeln.Gewinnsaetze)
		{
			Sieger = Gewinner;
			Folge.bMatchGewonnen = true;
			Folge.Matchgewinner = Gewinner;
			return;
		}

		// Seitenwechsel am Satzende, wenn die Spielzahl ungerade war (Tiebreak zaehlt als ein Spiel)
		if (SpieleImSatz % 2 == 1)
		{
			Folge.bSeitenwechsel = true;
			SeiteSpieler0 = Gegenseite(SeiteSpieler0);
		}
		Folge.Pause = EPause::Satzpause;

		// Aufschlag wechselt weiter; nach einem Tiebreak nimmt der erste Tiebreak-Aufschlaeger auf
		SpielAufschlaeger = 1 - SpielAufschlaeger;

		if (IstEntscheidungssatz() && Regeln.Entscheidungssatz == EEntscheidungssatz::MatchTiebreak)
		{
			bTiebreak = true;
			bMatchTiebreakLaeuft = true;
			TiebreakZiel = Regeln.LangerTiebreakBis;
			Folge.bMatchTiebreakBeginnt = true;
		}
		PruefeNeueBaelle(Folge, bTiebreak);
	}

	void FZaehlweise::PruefeNeueBaelle(FPunktFolge& Folge, bool bNaechstesIstTiebreak)
	{
		if (Regeln.NeueBaelleErstesMal <= 0 || Sieger >= 0)
		{
			return;
		}
		if (bBallwechselAufgeschoben)
		{
			// Aufgeschoben (Faelligkeit vor einem Tiebreak): im zweiten Spiel des folgenden Satzes
			if (!bNaechstesIstTiebreak && Spiele[0] + Spiele[1] == 1)
			{
				Folge.bNeueBaelle = true;
				bBallwechselAufgeschoben = false;
				bErsterBallwechselErfolgt = true;
				SpieleSeitBallwechsel = 0;
			}
			return;
		}
		const int Faellig = bErsterBallwechselErfolgt ? Regeln.NeueBaelleDanach : Regeln.NeueBaelleErstesMal;
		if (SpieleSeitBallwechsel >= Faellig)
		{
			if (bNaechstesIstTiebreak)
			{
				bBallwechselAufgeschoben = true;
				return;
			}
			Folge.bNeueBaelle = true;
			bErsterBallwechselErfolgt = true;
			SpieleSeitBallwechsel = 0;
		}
	}

	std::string FZaehlweise::PunktText(int Spieler) const
	{
		if (Spieler != 0 && Spieler != 1)
		{
			return std::string();
		}
		const int Gegner = 1 - Spieler;
		if (bTiebreak)
		{
			return std::to_string(Punkte[Spieler]);
		}
		if (!Regeln.bNoAd && Punkte[0] >= 3 && Punkte[1] >= 3)
		{
			return Punkte[Spieler] > Punkte[Gegner] ? std::string("AD") : std::string("40");
		}
		return std::string(TennisPunktName(Punkte[Spieler] > 3 ? 3 : Punkte[Spieler]));
	}

	std::string FZaehlweise::StandText() const
	{
		std::string Text;
		for (const FSatzErgebnis& Satz : Saetze)
		{
			if (Satz.bMatchTiebreak)
			{
				Text += "[" + std::to_string(Satz.TiebreakPunkte[0]) + "-" + std::to_string(Satz.TiebreakPunkte[1]) + "] ";
				continue;
			}
			Text += std::to_string(Satz.Spiele[0]) + "-" + std::to_string(Satz.Spiele[1]);
			if (Satz.bMitTiebreak)
			{
				const int Verlierer = 1 - Satz.Gewinner;
				Text += "(" + std::to_string(Satz.TiebreakPunkte[Verlierer]) + ")";
			}
			Text += " ";
		}
		if (Sieger < 0)
		{
			if (IstMatchTiebreak())
			{
				Text += "[" + std::to_string(Punkte[0]) + "-" + std::to_string(Punkte[1]) + "]";
			}
			else
			{
				Text += std::to_string(Spiele[0]) + "-" + std::to_string(Spiele[1]) + " " + PunktText(0) + "-" + PunktText(1);
			}
		}
		else if (!Text.empty())
		{
			Text.pop_back();
		}
		return Text;
	}

	FPunktBedeutung FZaehlweise::Bedeutung(int Spieler) const
	{
		FPunktBedeutung Ergebnis;
		if (Sieger >= 0 || (Spieler != 0 && Spieler != 1))
		{
			return Ergebnis;
		}
		FZaehlweise Probe = *this;
		const FPunktFolge Folge = Probe.PunktFuer(Spieler);
		Ergebnis.bMatchball = Folge.bMatchGewonnen && Folge.Matchgewinner == Spieler;
		Ergebnis.bSatzball = Folge.bSatzGewonnen && !Folge.bMatchGewonnen && Folge.Satzgewinner == Spieler;
		Ergebnis.bBreakball = !bTiebreak && Folge.bSpielGewonnen && Folge.bBreak && Folge.Spielgewinner == Spieler;
		Ergebnis.bEntscheidungspunkt = !bTiebreak && Regeln.bNoAd && Punkte[0] == 3 && Punkte[1] == 3;
		return Ergebnis;
	}
}
