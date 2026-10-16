// Matchball Tennis - Tests der Match-Zustandsmaschine, des Punktrichters und der Ansagen (README 2.3, 5, 7, 16.2)

#include "Kern/MBTestDaten.h"
#include "Kern/MBTestRahmen.h"

#include "Match/Kern/MBMatchAblauf.h"
#include "Physics/Kern/MBSchlagberechnung.h"

#include <algorithm>

using namespace MB;

namespace
{
	FBallEreignis MBAblaufTest_Aufsprung(double X, double Y, double VY)
	{
		FBallEreignis Ereignis;
		Ereignis.Art = EBallEreignisArt::Aufsprung;
		Ereignis.Position = FVek3(X, Y, 0.0335);
		Ereignis.GeschwindigkeitVorher = FVek3(0.0, VY, -8.0);
		Ereignis.GeschwindigkeitNachher = FVek3(0.0, VY * 0.7, 6.0);
		return Ereignis;
	}

	FBallEreignis MBAblaufTest_Netz(EBallEreignisArt Art)
	{
		FBallEreignis Ereignis;
		Ereignis.Art = Art;
		Ereignis.Position = FVek3(0.0, 0.0, 0.95);
		return Ereignis;
	}

	// Richtung "nach vorne" fuer den aktuellen Aufschlaeger (+1: zum fernen Ende)
	double MBAblaufTest_Vorwaerts(const FMatchAblauf& Ablauf)
	{
		return SeitenVorzeichen(Gegenseite(Ablauf.GetStand().GetSeite(Ablauf.GetAufschlaeger())));
	}

	// Mitte des richtigen Aufschlagfelds des aktuellen Aufschlags
	FVek3 MBAblaufTest_Feldmitte(const FMatchAblauf& Ablauf)
	{
		const FPlatz Platz;
		const FFeld Feld = Platz.Aufschlagfeld(Ablauf.GetStand().GetSeite(Ablauf.GetAufschlaeger()), Ablauf.GetStand().GetAufschlagHaelfte());
		return FVek3(0.5 * (Feld.MinX + Feld.MaxX), 0.5 * (Feld.MinY + Feld.MaxY), 0.0);
	}

	// Bis zum naechsten Zustand mit Eingabe vorspulen (in kleinen Zeitschritten)
	void MBAblaufTest_Warte(FMatchAblauf& Ablauf, EMatchZustand Ziel, double MaxZeit = 200.0)
	{
		double Zeit = 0.0;
		while (Ablauf.GetZustand() != Ziel && Zeit < MaxZeit && Ablauf.GetZustand() != EMatchZustand::Matchende)
		{
			Ablauf.Fortschreiten(0.05);
			Zeit += 0.05;
		}
	}

	// Zwei Aufschlaege klar hinter die Aufschlaglinie -> Doppelfehler
	void MBAblaufTest_Doppelfehler(FMatchAblauf& Ablauf)
	{
		for (int Versuch = 0; Versuch < 2; ++Versuch)
		{
			MBAblaufTest_Warte(Ablauf, EMatchZustand::Aufschlagvorbereitung);
			const double Vor = MBAblaufTest_Vorwaerts(Ablauf);
			Ablauf.AufschlagGeschlagen();
			Ablauf.BallEreignis(MBAblaufTest_Aufsprung(0.0, Vor * 10.0, Vor * 30.0));
		}
	}

	// Ein Ass fuer den aktuellen Aufschlaeger
	void MBAblaufTest_Ass(FMatchAblauf& Ablauf)
	{
		MBAblaufTest_Warte(Ablauf, EMatchZustand::Aufschlagvorbereitung);
		const double Vor = MBAblaufTest_Vorwaerts(Ablauf);
		const FVek3 Mitte = MBAblaufTest_Feldmitte(Ablauf);
		Ablauf.AufschlagGeschlagen();
		Ablauf.BallEreignis(MBAblaufTest_Aufsprung(Mitte.X, Mitte.Y, Vor * 30.0));
		Ablauf.BallEreignis(MBAblaufTest_Aufsprung(Mitte.X, Vor * 14.0, Vor * 20.0));
	}

	bool MBAblaufTest_HatMeldung(const std::vector<FAblaufMeldung>& Meldungen, EMeldungArt Art)
	{
		return std::any_of(Meldungen.begin(), Meldungen.end(), [Art](const FAblaufMeldung& M) { return M.Art == Art; });
	}

	FMatchEinstellungen MBAblaufTest_Einstellungen()
	{
		FMatchEinstellungen Einstellungen;
		Einstellungen.Ball = MBTest::LadeBall();
		Einstellungen.Startwert = 4711u;
		return Einstellungen;
	}
}

MB_TEST(AblaufAss, "Ablauf.AssUndAnsage")
{
	FMatchAblauf Ablauf(MBAblaufTest_Einstellungen());
	MB_PRUEFE(Ablauf.GetZustand() == EMatchZustand::Vorbereitung);
	MB_PRUEFE(!Ablauf.AufschlagGeschlagen());   // erst nach der Vorbereitung
	MBAblaufTest_Warte(Ablauf, EMatchZustand::Aufschlagvorbereitung);
	MB_PRUEFE(Ablauf.GetZustand() == EMatchZustand::Aufschlagvorbereitung);
	Ablauf.HoleMeldungen();

	MBAblaufTest_Ass(Ablauf);
	MB_PRUEFE(Ablauf.GetZustand() == EMatchZustand::Punktende);
	MB_PRUEFE(Ablauf.GetAusgang().Art == EAusgangArt::Punkt);
	MB_PRUEFE_GLEICH(Ablauf.GetAusgang().Punktgewinner, 0);
	MB_PRUEFE(Ablauf.GetAusgang().Grund == EPunktGrund::Ass);
	// Punkt wird erst in der Ansage gezaehlt
	MB_PRUEFE_GLEICH(Ablauf.GetStand().GetPunkte(0), 0);
	MBAblaufTest_Warte(Ablauf, EMatchZustand::Ansage);
	MB_PRUEFE_GLEICH(Ablauf.GetStand().GetPunkte(0), 1);
	const std::vector<FAblaufMeldung> Meldungen = Ablauf.HoleMeldungen();
	MB_PRUEFE(MBAblaufTest_HatMeldung(Meldungen, EMeldungArt::PunktGezaehlt));
	bool bAnsageOk = false;
	for (const FAblaufMeldung& M : Meldungen)
	{
		if (M.Art == EMeldungArt::Ansage)
		{
			bAnsageOk = M.Ansage.Bausteine == std::vector<std::string>{"fifteen", "love"};
		}
	}
	MB_PRUEFE(bAnsageOk);
	MBAblaufTest_Warte(Ablauf, EMatchZustand::ZwischenDenPunkten);
	MB_PRUEFE(Ablauf.GetZustand() == EMatchZustand::ZwischenDenPunkten);
	Ablauf.Ueberspringen();
	MB_PRUEFE(Ablauf.GetZustand() == EMatchZustand::Aufschlagvorbereitung);
}

MB_TEST(AblaufDoppelfehler, "Ablauf.FehlerUndDoppelfehler")
{
	FMatchAblauf Ablauf(MBAblaufTest_Einstellungen());
	MBAblaufTest_Warte(Ablauf, EMatchZustand::Aufschlagvorbereitung);
	// Erster Aufschlag weit hinter die Aufschlaglinie
	const double Vor = MBAblaufTest_Vorwaerts(Ablauf);
	Ablauf.AufschlagGeschlagen();
	Ablauf.HoleMeldungen();
	Ablauf.BallEreignis(MBAblaufTest_Aufsprung(-2.0, Vor * 9.5, Vor * 30.0));
	std::vector<FAblaufMeldung> Meldungen = Ablauf.HoleMeldungen();
	bool bFault = false;
	for (const FAblaufMeldung& M : Meldungen)
	{
		bFault = bFault || (M.Art == EMeldungArt::Ruf && M.Ansage.Bausteine.front() == "fault");
	}
	MB_PRUEFE(bFault);
	MB_PRUEFE(Ablauf.GetAusgang().Art == EAusgangArt::Fehler);
	MBAblaufTest_Warte(Ablauf, EMatchZustand::Aufschlagvorbereitung);
	MB_PRUEFE(Ablauf.IstZweiterAufschlag());
	MB_PRUEFE_GLEICH(Ablauf.GetStand().GetPunkte(1), 0);
	// Zweiter Aufschlag ins Netz -> Doppelfehler
	Ablauf.AufschlagGeschlagen();
	Ablauf.BallEreignis(MBAblaufTest_Netz(EBallEreignisArt::Netzgeflecht));
	Ablauf.BallEreignis(MBAblaufTest_Aufsprung(0.3, -Vor * 0.4, -Vor * 1.0));
	MB_PRUEFE(Ablauf.GetAusgang().Art == EAusgangArt::Punkt);
	MB_PRUEFE(Ablauf.GetAusgang().Grund == EPunktGrund::Doppelfehler);
	MB_PRUEFE_GLEICH(Ablauf.GetAusgang().Punktgewinner, 1);
	MBAblaufTest_Warte(Ablauf, EMatchZustand::ZwischenDenPunkten);
	MB_PRUEFE_GLEICH(Ablauf.GetStand().GetPunkte(1), 1);
	MB_PRUEFE(!Ablauf.IstZweiterAufschlag());
}

MB_TEST(AblaufLet, "Ablauf.Let")
{
	FMatchAblauf Ablauf(MBAblaufTest_Einstellungen());
	MBAblaufTest_Warte(Ablauf, EMatchZustand::Aufschlagvorbereitung);
	const double Vor = MBAblaufTest_Vorwaerts(Ablauf);
	const FVek3 Mitte = MBAblaufTest_Feldmitte(Ablauf);
	Ablauf.AufschlagGeschlagen();
	Ablauf.BallEreignis(MBAblaufTest_Netz(EBallEreignisArt::Netzband));
	Ablauf.BallEreignis(MBAblaufTest_Aufsprung(Mitte.X, Mitte.Y, Vor * 15.0));
	MB_PRUEFE(Ablauf.GetAusgang().Art == EAusgangArt::Let);
	MBAblaufTest_Warte(Ablauf, EMatchZustand::Aufschlagvorbereitung);
	MB_PRUEFE(!Ablauf.IstZweiterAufschlag());
	MB_PRUEFE_GLEICH(Ablauf.GetStand().GetPunkte(0) + Ablauf.GetStand().GetPunkte(1), 0);
}

MB_TEST(AblaufBallwechsel, "Ablauf.BallwechselUndAus")
{
	FMatchAblauf Ablauf(MBAblaufTest_Einstellungen());
	MBAblaufTest_Warte(Ablauf, EMatchZustand::Aufschlagvorbereitung);
	const double Vor = MBAblaufTest_Vorwaerts(Ablauf);
	const FVek3 Mitte = MBAblaufTest_Feldmitte(Ablauf);
	Ablauf.AufschlagGeschlagen();
	MB_PRUEFE(!Ablauf.BallGeschlagen(1));    // Aufschlag darf nicht volley genommen werden
	Ablauf.BallEreignis(MBAblaufTest_Aufsprung(Mitte.X, Mitte.Y, Vor * 30.0));
	MB_PRUEFE(Ablauf.GetZustand() == EMatchZustand::Ballwechsel);
	MB_PRUEFE(Ablauf.IstBallImSpiel());
	MB_PRUEFE(!Ablauf.BallGeschlagen(0));    // Aufschlaeger ist nicht dran
	MB_PRUEFE(Ablauf.BallGeschlagen(1));
	MB_PRUEFE_GLEICH(Ablauf.GetSchlaegeImBallwechsel(), 2);
	// Rueckschlag des Rueckschlaegers landet weit hinter der Grundlinie des Aufschlaegers
	Ablauf.BallEreignis(MBAblaufTest_Netz(EBallEreignisArt::NetzUeberquert));
	Ablauf.BallEreignis(MBAblaufTest_Aufsprung(0.0, -Vor * 14.0, -Vor * 25.0));
	MB_PRUEFE(Ablauf.GetAusgang().Art == EAusgangArt::Punkt);
	MB_PRUEFE(Ablauf.GetAusgang().Grund == EPunktGrund::Aus);
	MB_PRUEFE_GLEICH(Ablauf.GetAusgang().Punktgewinner, 0);
	// Klarer Ball (2 m aus): kein Challenge-Fenster
	MBAblaufTest_Warte(Ablauf, EMatchZustand::Ansage);
	MB_PRUEFE_GLEICH(Ablauf.GetStand().GetPunkte(0), 1);
}

MB_TEST(AblaufChallengeErfolgreich, "Ablauf.ChallengeErfolgreich")
{
	// Linienrichter irrt bei knappen Baellen immer (Testeinstellung)
	FMatchEinstellungen Einstellungen = MBAblaufTest_Einstellungen();
	Einstellungen.Linienrichter.MaxFehler = 2.0;
	FMatchAblauf Ablauf(Einstellungen);
	MBAblaufTest_Warte(Ablauf, EMatchZustand::Aufschlagvorbereitung);
	const double Vor = MBAblaufTest_Vorwaerts(Ablauf);
	const FVek3 Mitte = MBAblaufTest_Feldmitte(Ablauf);
	Ablauf.AufschlagGeschlagen();
	// Aufsprung 5 mm vor der Aufschlaglinie: eigentlich gut, wird aber als Fehler gegeben
	Ablauf.BallEreignis(MBAblaufTest_Aufsprung(Mitte.X, Vor * (Platzmass::AufschlaglinieVomNetz - 0.005), Vor * 30.0));
	MB_PRUEFE(Ablauf.GetAusgang().Art == EAusgangArt::Fehler);
	MB_PRUEFE(Ablauf.GetLetzteEntscheidung().Linie.bFehlentscheidung);
	MB_PRUEFE(Ablauf.GetLetzteEntscheidung().Strittig.bWarIn);
	MB_PRUEFE(!Ablauf.ChallengeAnfordern(1));   // nur der Aufschlaeger darf
	MBAblaufTest_Warte(Ablauf, EMatchZustand::ChallengeFenster);
	MB_PRUEFE(Ablauf.GetZustand() == EMatchZustand::ChallengeFenster);
	Ablauf.HoleMeldungen();
	MB_PRUEFE(Ablauf.ChallengeAnfordern(0));
	MB_PRUEFE(Ablauf.GetZustand() == EMatchZustand::Videobeweis);
	MB_PRUEFE(!Ablauf.ChallengeAnfordern(0));   // nicht doppelt
	std::vector<FAblaufMeldung> Meldungen = Ablauf.HoleMeldungen();
	bool bAnsage = false;
	for (const FAblaufMeldung& M : Meldungen)
	{
		if (M.Art == EMeldungArt::ChallengeAngefordert)
		{
			bAnsage = M.Ansage.Bausteine.size() == 3 && M.Ansage.Bausteine[0] == "@0" && M.Ansage.Bausteine[1] == "is_challenging_the_call_on_the";
		}
	}
	MB_PRUEFE(bAnsage);
	Ablauf.Ueberspringen();   // Videobeweis ueberspringen -> Ergebnis sofort
	Meldungen = Ablauf.HoleMeldungen();
	bool bErgebnis = false;
	for (const FAblaufMeldung& M : Meldungen)
	{
		if (M.Art == EMeldungArt::ChallengeErgebnis)
		{
			bErgebnis = M.Challenge.bErfolgreich && M.Challenge.Folge == EChallengeFolge::ErsterAufschlag;
			MB_PRUEFE(M.Ansage.Bausteine == (std::vector<std::string>{"correction", ",", "the_ball_was_good", ".", "replay_the_point", ".", "first_serve"}));
		}
	}
	MB_PRUEFE(bErgebnis);
	MB_PRUEFE(Ablauf.GetAusgang().Art == EAusgangArt::Wiederholung);
	MBAblaufTest_Warte(Ablauf, EMatchZustand::Aufschlagvorbereitung);
	MB_PRUEFE(!Ablauf.IstZweiterAufschlag());
	MB_PRUEFE_GLEICH(Ablauf.GetChallenges().Verbleibend(0), 3);   // erfolgreich -> bleibt erhalten
}

MB_TEST(AblaufChallengeErfolglos, "Ablauf.ChallengeErfolglos")
{
	FMatchEinstellungen Einstellungen = MBAblaufTest_Einstellungen();
	Einstellungen.Linienrichter.MaxFehler = 0.0;   // Linienrichter immer richtig
	FMatchAblauf Ablauf(Einstellungen);
	MBAblaufTest_Warte(Ablauf, EMatchZustand::Aufschlagvorbereitung);
	const double Vor = MBAblaufTest_Vorwaerts(Ablauf);
	const FVek3 Mitte = MBAblaufTest_Feldmitte(Ablauf);
	Ablauf.AufschlagGeschlagen();
	// 2 cm hinter der Aufschlaglinie: knapp aus, richtig gegeben
	Ablauf.BallEreignis(MBAblaufTest_Aufsprung(Mitte.X, Vor * (Platzmass::AufschlaglinieVomNetz + 0.02), Vor * 30.0));
	MB_PRUEFE(Ablauf.GetAusgang().Art == EAusgangArt::Fehler);
	MB_PRUEFE(!Ablauf.GetLetzteEntscheidung().Strittig.bWarIn);
	MB_PRUEFE(Ablauf.ChallengeAnfordern(0));
	Ablauf.Ueberspringen();
	MB_PRUEFE(Ablauf.GetAusgang().Art == EAusgangArt::Fehler);
	MB_PRUEFE_GLEICH(Ablauf.GetChallenges().Verbleibend(0), 2);
	MBAblaufTest_Warte(Ablauf, EMatchZustand::Aufschlagvorbereitung);
	MB_PRUEFE(Ablauf.IstZweiterAufschlag());
}

MB_TEST(AblaufGanzesMatch, "Ablauf.GanzesMatchMitSeitenwechsel")
{
	FMatchEinstellungen Einstellungen = MBAblaufTest_Einstellungen();
	FMatchAblauf Ablauf(Einstellungen);
	int Seitenwechsel = 0;
	int Satzpausen = 0;
	int Asse = 0;
	// Spieler 0 gewinnt jeden Punkt: Asse beim eigenen Aufschlag, Doppelfehler des Gegners
	while (Ablauf.GetZustand() != EMatchZustand::Matchende && Asse < 200)
	{
		MBAblaufTest_Warte(Ablauf, EMatchZustand::Aufschlagvorbereitung);
		if (Ablauf.GetAufschlaeger() == 0)
		{
			MBAblaufTest_Ass(Ablauf);
		}
		else
		{
			MBAblaufTest_Doppelfehler(Ablauf);
		}
		++Asse;
		for (const FAblaufMeldung& M : Ablauf.HoleMeldungen())
		{
			Seitenwechsel += (M.Art == EMeldungArt::Zustand && M.Zustand == EMatchZustand::Seitenwechsel) ? 1 : 0;
			Satzpausen += (M.Art == EMeldungArt::Zustand && M.Zustand == EMatchZustand::Satzpause) ? 1 : 0;
		}
		MBAblaufTest_Warte(Ablauf, EMatchZustand::Aufschlagvorbereitung);
		for (const FAblaufMeldung& M : Ablauf.HoleMeldungen())
		{
			Seitenwechsel += (M.Art == EMeldungArt::Zustand && M.Zustand == EMatchZustand::Seitenwechsel) ? 1 : 0;
			Satzpausen += (M.Art == EMeldungArt::Zustand && M.Zustand == EMatchZustand::Satzpause) ? 1 : 0;
		}
	}
	MB_PRUEFE(Ablauf.GetZustand() == EMatchZustand::Matchende);
	MB_PRUEFE_GLEICH(Ablauf.GetSieger(), 0);
	MB_INFO("Punkte: %d, Seitenwechsel: %d, Satzpausen: %d, Ergebnis %s", Asse, Seitenwechsel, Satzpausen, Ablauf.GetStand().StandText().c_str());
	MB_PRUEFE_GLEICH(Asse, 48);
	MB_PRUEFE_GLEICH(Ablauf.GetStand().StandText(), std::string("6-0 6-0"));
	// Seitenwechsel nach Spiel 1, 3, 5 jedes Satzes; 6:0 ist gerade -> kein Wechsel am Satzende
	MB_PRUEFE_GLEICH(Seitenwechsel, 6);
	MB_PRUEFE_GLEICH(Satzpausen, 1);
}

MB_TEST(AblaufAufgabe, "Ablauf.Aufgabe")
{
	FMatchAblauf Ablauf(MBAblaufTest_Einstellungen());
	MBAblaufTest_Warte(Ablauf, EMatchZustand::Aufschlagvorbereitung);
	Ablauf.HoleMeldungen();
	Ablauf.Aufgabe(0);
	MB_PRUEFE(Ablauf.GetZustand() == EMatchZustand::Matchende);
	MB_PRUEFE_GLEICH(Ablauf.GetSieger(), 1);
	MB_PRUEFE(MBAblaufTest_HatMeldung(Ablauf.HoleMeldungen(), EMeldungArt::Matchende));
}

MB_TEST(AnsagenTexte, "Ansagen.Bausteine")
{
	const std::vector<MBTest::FZeile> Zeilen = MBTest::LiesCsv("ansagen.csv");
	MB_PRUEFE(!Zeilen.empty());
	std::map<std::string, std::string> Texte;
	for (const MBTest::FZeile& Zeile : Zeilen)
	{
		Texte[MBTest::Text(Zeile, "Name")] = MBTest::Text(Zeile, "Text");
	}
	const std::string Namen[2] = {"Adler", "Falk"};

	FZaehlweise Z(FZaehlRegeln(), 0);
	FPunktFolge Folge = Z.PunktFuer(0);
	MB_PRUEFE_GLEICH(AnsageNachPunkt(Z, Folge).Text(Texte, Namen), std::string("Fifteen love"));
	Folge = Z.PunktFuer(1);
	MB_PRUEFE_GLEICH(AnsageNachPunkt(Z, Folge).Text(Texte, Namen), std::string("Fifteen all"));
	Z.PunktFuer(1);
	Folge = Z.PunktFuer(1);
	MB_PRUEFE_GLEICH(AnsageNachPunkt(Z, Folge).Text(Texte, Namen), std::string("Fifteen forty"));
	Z.PunktFuer(0);
	Folge = Z.PunktFuer(0);
	MB_PRUEFE_GLEICH(AnsageNachPunkt(Z, Folge).Text(Texte, Namen), std::string("Deuce"));
	Folge = Z.PunktFuer(1);
	MB_PRUEFE_GLEICH(AnsageNachPunkt(Z, Folge).Text(Texte, Namen), std::string("Advantage Falk"));
	MB_PRUEFE_GLEICH(AnsageNachPunkt(Z, Folge).OhneNamen(0).Text(Texte, Namen), std::string("Advantage receiver"));
	Folge = Z.PunktFuer(1);
	MB_PRUEFE_GLEICH(AnsageNachPunkt(Z, Folge).Text(Texte, Namen), std::string("Game Falk. Falk leads one game to love"));
	MB_PRUEFE_GLEICH(AnsageNachPunkt(Z, Folge).OhneNamen(0).Text(Texte, Namen), std::string("Game. One game to love"));

	// Alle verwendeten Bausteine muessen in Data/ansagen.csv stehen
	const char* Pflicht[] = {"love", "fifteen", "thirty", "forty", "all", "deuce", "advantage", "game", "leads", "game_to", "games_to", "game_all", "games_all",
		"tiebreak", "match_tiebreak", "game_and", "first_set", "second_set", "third_set", "fourth_set", "fifth_set", "game_set_and_match", "sets_to",
		"deciding_point", "fault", "out", "let", "new_balls_please", "is_challenging_the_call_on_the", "left_baseline", "right_baseline", "left_service_line",
		"right_service_line", "near_sideline", "far_sideline", "centre_service_line", "correction", "the_ball_was_good", "the_ball_was_out", "replay_the_point",
		"first_serve", "second_serve", "server", "receiver", "time", "quiet_please"};
	for (const char* Id : Pflicht)
	{
		const bool bVorhanden = Texte.count(Id) > 0;
		if (!bVorhanden)
		{
			MB_INFO("Baustein fehlt in ansagen.csv: %s", Id);
		}
		MB_PRUEFE(bVorhanden);
	}
	for (int Zahl = 0; Zahl <= 30; ++Zahl)
	{
		MB_PRUEFE(Texte.count(ZahlBaustein(Zahl)) > 0);
	}
}

MB_TEST(AnsagenTiebreakUndSatz, "Ansagen.TiebreakSatzMatch")
{
	FZaehlRegeln Regeln;
	Regeln.Gewinnsaetze = 1;
	FZaehlweise Z(Regeln, 0);
	for (int Spiel = 0; Spiel < 12; ++Spiel)
	{
		for (int Punkt = 0; Punkt < 4; ++Punkt)
		{
			Z.PunktFuer(Spiel % 2);
		}
	}
	MB_PRUEFE(Z.IstTiebreak());
	FPunktFolge Folge = Z.PunktFuer(1);
	MB_PRUEFE(AnsageNachPunkt(Z, Folge).Bausteine == (std::vector<std::string>{"n1", "n0", ",", "@1"}));
	Folge = Z.PunktFuer(0);
	MB_PRUEFE(AnsageNachPunkt(Z, Folge).Bausteine == (std::vector<std::string>{"n1", "all"}));
	for (int Index = 0; Index < 6; ++Index)
	{
		Folge = Z.PunktFuer(0);
	}
	MB_PRUEFE(Folge.bMatchGewonnen);
	MB_PRUEFE(AnsageNachPunkt(Z, Folge).Bausteine == (std::vector<std::string>{"game_set_and_match", "@0", ",", "n7", "games_to", "n6"}));
}

namespace
{
	// Spielt einen Aufschlag mit echter Physik durch: Schlagberechnung -> Simulation -> Ablauf
	void MBAblaufTest_AufschlagMitPhysik(FMatchAblauf& Ablauf, const FBallSimulation& Vorlage, double ZielTiefeVomNetz, int MaxSchritte = 2400)
	{
		MBAblaufTest_Warte(Ablauf, EMatchZustand::Aufschlagvorbereitung);
		const FZaehlweise& Stand = Ablauf.GetStand();
		const int Aufschlaeger = Stand.GetAufschlaeger();
		const EPlatzseite Seite = Stand.GetSeite(Aufschlaeger);
		const FPlatz Platz;
		const FFeld Feld = Platz.Aufschlagfeld(Seite, Stand.GetAufschlagHaelfte());
		const double Vor = SeitenVorzeichen(Gegenseite(Seite));
		FVek3 Treffpunkt = Platz.AufschlagPosition(Seite, Stand.GetAufschlagHaelfte());
		Treffpunkt.Z = 2.75;
		FSchlagAnfrage Anfrage;
		Anfrage.Treffpunkt = Treffpunkt;
		Anfrage.ZielX = 0.5 * (Feld.MinX + Feld.MaxX);
		Anfrage.ZielY = Vor * ZielTiefeVomNetz;
		Anfrage.Tempo = KmhZuMs(190.0);
		Anfrage.MinTempo = KmhZuMs(150.0);
		Anfrage.Topspin = UpmZuRadS(900.0);
		Anfrage.MinNetzAbstand = 0.05;
		const FSchlagErgebnis Schlag = BerechneSchlag(Vorlage, Anfrage);
		FBallSimulation Simulation = Vorlage;
		Simulation.Starte(Schlag.Abflug);
		Ablauf.AufschlagGeschlagen();
		for (int Index = 0; Index < MaxSchritte && (Ablauf.GetZustand() == EMatchZustand::Aufschlag || Ablauf.GetZustand() == EMatchZustand::Ballwechsel); ++Index)
		{
			Simulation.Schritt();
			for (const FBallEreignis& Ereignis : Simulation.GetEreignisse())
			{
				Ablauf.BallEreignis(Ereignis);
			}
			Simulation.LeereEreignisse();
			Ablauf.Fortschreiten(Simulation.GetZeitschritt());
		}
	}
}

MB_TEST(AblaufMitPhysik, "Ablauf.AufschlagMitEchterPhysik")
{
	FMatchEinstellungen Einstellungen = MBAblaufTest_Einstellungen();
	Einstellungen.Linienrichter.bElektronisch = true;   // ohne Zufall: Ergebnis = Hawk-Eye-Wahrheit
	FMatchAblauf Ablauf(Einstellungen);
	const FBallSimulation Vorlage = MBTest::Simulation("Hart");

	// 1. Aufschlag sauber ins Feld, niemand schlaegt zurueck -> Ass (Ball springt zweimal auf)
	MBAblaufTest_AufschlagMitPhysik(Ablauf, Vorlage, 5.0);
	MB_PRUEFE(Ablauf.GetAusgang().Art == EAusgangArt::Punkt);
	MB_PRUEFE(Ablauf.GetAusgang().Grund == EPunktGrund::Ass);
	MB_PRUEFE_GLEICH(Ablauf.GetAusgang().Punktgewinner, Ablauf.GetAufschlaeger());
	MBAblaufTest_Warte(Ablauf, EMatchZustand::ZwischenDenPunkten);
	MB_PRUEFE_GLEICH(Ablauf.GetStand().GetPunkte(0), 1);

	// 2. Aufschlag 1,5 m hinter die Aufschlaglinie -> Fehler, zweiter Aufschlag
	MBAblaufTest_AufschlagMitPhysik(Ablauf, Vorlage, Platzmass::AufschlaglinieVomNetz + 1.5);
	MB_PRUEFE(Ablauf.GetAusgang().Art == EAusgangArt::Fehler);
	MBAblaufTest_Warte(Ablauf, EMatchZustand::Aufschlagvorbereitung);
	MB_PRUEFE(Ablauf.IstZweiterAufschlag());

	// 3. Zweiter Aufschlag ebenfalls lang -> Doppelfehler, Punkt fuer den Rueckschlaeger
	MBAblaufTest_AufschlagMitPhysik(Ablauf, Vorlage, Platzmass::AufschlaglinieVomNetz + 1.5);
	MB_PRUEFE(Ablauf.GetAusgang().Grund == EPunktGrund::Doppelfehler);
	MB_PRUEFE_GLEICH(Ablauf.GetAusgang().Punktgewinner, 1);
	MBAblaufTest_Warte(Ablauf, EMatchZustand::ZwischenDenPunkten);
	MB_PRUEFE_GLEICH(Ablauf.GetStand().GetPunkte(1), 1);
	MB_PRUEFE(Ablauf.GetAufschlagNummer() >= 3u);
}
