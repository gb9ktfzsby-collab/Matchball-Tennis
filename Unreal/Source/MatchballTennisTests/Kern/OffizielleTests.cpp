// Matchball Tennis - Tests fuer Platz, Hawk-Eye-Kontaktflaeche, Linienrichter und Challenge (README 7, 12.1, 24.1)

#include "Kern/MBTestDaten.h"
#include "Kern/MBTestRahmen.h"

#include "Core/Kern/MBZufall.h"
#include "Officials/Kern/MBChallenge.h"
#include "Officials/Kern/MBLinienrichter.h"
#include "Physics/Kern/MBKontaktflaeche.h"
#include "Physics/Kern/MBPlatz.h"

using namespace MB;

namespace
{
	// Kuenstliches Aufsprung-Ereignis (Ballmittelpunkt bei X/Y, Aufprall mit Geschwindigkeit V)
	FBallEreignis MBOffizielleTest_Aufsprung(double X, double Y, const FVek3& V)
	{
		FBallEreignis Ereignis;
		Ereignis.Art = EBallEreignisArt::Aufsprung;
		Ereignis.Position = FVek3(X, Y, 0.0335);
		Ereignis.GeschwindigkeitVorher = V;
		Ereignis.GeschwindigkeitNachher = FVek3(V.X * 0.7, V.Y * 0.7, -V.Z * 0.8);
		return Ereignis;
	}
}

MB_TEST(PlatzMasse, "Platz.MasseUndFelder")
{
	MB_PRUEFE_NAH(Platzmass::HalbeLaenge, 11.885, 1e-12);
	MB_PRUEFE_NAH(Platzmass::HalbeBreiteEinzel, 4.115, 1e-12);
	MB_PRUEFE_NAH(Platzmass::PfostenX, 6.399, 1e-12);
	MB_PRUEFE_NAH(FPlatz::NetzOberkante(0.0), 0.914, 1e-12);
	MB_PRUEFE_NAH(FPlatz::NetzOberkante(Platzmass::EinzelstuetzeX), 1.07, 1e-12);
	MB_PRUEFE_NAH(FPlatz::NetzOberkante(-6.0), 1.07, 1e-12);
	MB_PRUEFE(FPlatz::NetzOberkante(2.0) > 0.914 && FPlatz::NetzOberkante(2.0) < 1.07);

	const FPlatz Platz;
	// Aufschlaeger nah, Einstand: steht rechts (X > 0), Ziel diagonal links im fernen Feld
	const FFeld Einstand = Platz.Aufschlagfeld(EPlatzseite::Nah, EAufschlagHaelfte::Einstand);
	MB_PRUEFE(Einstand.Enthaelt(-2.0, 3.0));
	MB_PRUEFE(!Einstand.Enthaelt(2.0, 3.0));
	MB_PRUEFE(Einstand.Enthaelt(0.02, 3.0));     // Mittellinie gehoert dazu (5 cm breit, mittig)
	MB_PRUEFE(!Einstand.Enthaelt(0.03, 3.0));
	MB_PRUEFE(Einstand.Enthaelt(-2.0, 6.40));    // Aufschlaglinie gehoert dazu
	MB_PRUEFE(!Einstand.Enthaelt(-2.0, 6.41));
	const FVek3 Position = Platz.AufschlagPosition(EPlatzseite::Nah, EAufschlagHaelfte::Einstand);
	MB_PRUEFE(Position.X > 0.0 && Position.Y < -Platzmass::HalbeLaenge);
	// Aufschlaeger fern, Einstand: steht bei X < 0, Ziel im nahen Feld bei X > 0
	const FFeld Fern = Platz.Aufschlagfeld(EPlatzseite::Fern, EAufschlagHaelfte::Einstand);
	MB_PRUEFE(Fern.Enthaelt(2.0, -3.0));
	MB_PRUEFE(!Fern.Enthaelt(-2.0, -3.0));
	MB_PRUEFE(Platz.AufschlagPosition(EPlatzseite::Fern, EAufschlagHaelfte::Einstand).X < 0.0);
	const FFeld Vorteil = Platz.Aufschlagfeld(EPlatzseite::Nah, EAufschlagHaelfte::Vorteil);
	MB_PRUEFE(Vorteil.Enthaelt(2.0, 3.0));

	// Einzelfeld fern: Grundlinie 11,885 m, Netzseite ist keine Linie
	const FFeld Einzel = Platz.Einzelfeld(EPlatzseite::Fern);
	ELinie Linie = ELinie::Keine;
	MB_PRUEFE_NAH(Einzel.VorzeichenAbstand(0.0, 11.0, &Linie), -0.885, 1e-9);
	MB_PRUEFE(Linie == ELinie::Grundlinie);
	MB_PRUEFE_NAH(Einzel.VorzeichenAbstand(0.0, 0.1, &Linie), -4.115, 1e-9);   // naechste Linie: Seitenlinie, nicht das Netz
	MB_PRUEFE_NAH(Einzel.VorzeichenAbstand(4.215, 5.0, &Linie), 0.1, 1e-9);
	MB_PRUEFE(Linie == ELinie::SeitenlinieRechts);
}

MB_TEST(HawkEyeKontaktflaeche, "HawkEye.Kontaktflaeche")
{
	const FBallParameter Ball = MBTest::LadeBall();
	// Steiler, langsamer Ball: rundlicher Abdruck; flacher, schneller Ball: laenglich
	const FKontaktflaeche Langsam = BerechneKontaktflaeche(MBOffizielleTest_Aufsprung(0.0, 5.0, FVek3(0.0, 6.0, -8.0)), Ball);
	const FKontaktflaeche Schnell = BerechneKontaktflaeche(MBOffizielleTest_Aufsprung(0.0, 5.0, FVek3(0.0, 35.0, -8.0)), Ball);
	const FKontaktflaeche Weich = BerechneKontaktflaeche(MBOffizielleTest_Aufsprung(0.0, 5.0, FVek3(0.0, 35.0, -3.0)), Ball);
	MB_INFO("Abdruck langsam %.1f x %.1f cm, schnell %.1f x %.1f cm, flach %.1f x %.1f cm, Kontaktzeit %.2f ms", Langsam.Laenge * 100.0, Langsam.Breite * 100.0, Schnell.Laenge * 100.0, Schnell.Breite * 100.0, Weich.Laenge * 100.0, Weich.Breite * 100.0, Schnell.Kontaktzeit * 1000.0);
	MB_PRUEFE(Schnell.Laenge > Langsam.Laenge);
	MB_PRUEFE_NAH(Schnell.Breite, Langsam.Breite, 1e-12);   // Breite haengt nur vom senkrechten Tempo ab
	MB_PRUEFE(Weich.Breite < Schnell.Breite);
	MB_PRUEFE(Langsam.Laenge >= Langsam.Breite);
	// Kontaktzeit 4-5 ms (Messwerte fuer Tennisbaelle)
	MB_PRUEFE(Schnell.Kontaktzeit > 0.004 && Schnell.Kontaktzeit < 0.005);
	// Plausible Groesse: Breite 3-6 cm, Laenge 4-20 cm
	MB_PRUEFE(Schnell.Breite > 0.03 && Schnell.Breite < 0.06);
	MB_PRUEFE(Schnell.Laenge > 0.04 && Schnell.Laenge < 0.20);
}

MB_TEST(HawkEyeLinienentscheidung, "HawkEye.InSobaldLinieBeruehrt")
{
	const FBallParameter Ball = MBTest::LadeBall();
	const FPlatz Platz;
	const FFeld Feld = Platz.Einzelfeld(EPlatzseite::Fern);
	const FVek3 Seitlich(10.0, 0.0, -8.0);   // Ball fliegt quer auf die rechte Seitenlinie zu

	// Halbe Breite des Abdrucks bestimmen
	const FKontaktflaeche Probe = BerechneKontaktflaeche(MBOffizielleTest_Aufsprung(0.0, 5.0, Seitlich), Ball);
	double MinX = 1e9;
	for (const FKontaktKreis& Kreis : Probe.Kreise)
	{
		MinX = MinWert(MinX, Kreis.X - Kreis.Radius);
	}
	// Abdruck so legen, dass sein hinterster Rand 1 mm ausserhalb bzw. 1 mm innerhalb der Linie liegt
	const double Versatz = -MinX;   // Abstand vom Aufsprungpunkt zum hintersten Rand
	{
		const FKontaktflaeche Aus = BerechneKontaktflaeche(MBOffizielleTest_Aufsprung(Platzmass::HalbeBreiteEinzel + Versatz + 0.001, 5.0, Seitlich), Ball);
		const FLinienUrteil Urteil = BeurteileKontaktflaeche(Aus, Feld);
		MB_PRUEFE(!Urteil.bIn);
		MB_PRUEFE_NAH(Urteil.Abstand, 0.001, 1e-6);
		MB_PRUEFE(Urteil.Linie == ELinie::SeitenlinieRechts);
	}
	{
		const FKontaktflaeche In = BerechneKontaktflaeche(MBOffizielleTest_Aufsprung(Platzmass::HalbeBreiteEinzel + Versatz - 0.001, 5.0, Seitlich), Ball);
		const FLinienUrteil Urteil = BeurteileKontaktflaeche(In, Feld);
		MB_PRUEFE(Urteil.bIn);
		MB_PRUEFE_NAH(Urteil.Abstand, 0.001, 1e-6);
	}
	// Ballmitte ausserhalb, aber Abdruck beruehrt die Linie: In
	{
		const FKontaktflaeche Knapp = BerechneKontaktflaeche(MBOffizielleTest_Aufsprung(Platzmass::HalbeBreiteEinzel + 0.005, 5.0, FVek3(0.0, 20.0, -8.0)), Ball);
		MB_PRUEFE(BeurteileKontaktflaeche(Knapp, Feld).bIn);
	}
	// Klar im Feld: grosser Abstand
	{
		const FKontaktflaeche Mitte = BerechneKontaktflaeche(MBOffizielleTest_Aufsprung(0.0, 6.0, FVek3(0.0, 20.0, -8.0)), Ball);
		const FLinienUrteil Urteil = BeurteileKontaktflaeche(Mitte, Feld);
		MB_PRUEFE(Urteil.bIn);
		MB_PRUEFE(Urteil.Abstand > 1.0);
	}
}

MB_TEST(LinienrichterKurve, "Linienrichter.FehlerKurveUndMittelwert")
{
	const FLinienrichterParameter Parameter;
	MB_PRUEFE_NAH(LinienrichterFehlerwahrscheinlichkeit(0.0, Parameter), 0.20, 1e-12);
	MB_PRUEFE_NAH(LinienrichterFehlerwahrscheinlichkeit(0.075, Parameter), 0.10, 1e-12);
	MB_PRUEFE_NAH(LinienrichterFehlerwahrscheinlichkeit(0.15, Parameter), 0.0, 1e-12);
	MB_PRUEFE_NAH(LinienrichterFehlerwahrscheinlichkeit(0.30, Parameter), 0.0, 1e-12);

	// Simulation (README 7.1, PRUEFEN): knappe Baelle (d <= 15 cm, gleichverteilt) -> im Mittel ca. 10 % Fehler
	FZufall Zufall(2026u);
	FZufall Lage(77u);
	constexpr int Anzahl = 200000;
	int Fehler = 0;
	int FehlerSehrKnapp = 0;
	int AnzahlSehrKnapp = 0;
	for (int Index = 0; Index < Anzahl; ++Index)
	{
		FLinienUrteil Wahrheit;
		Wahrheit.bIn = Lage.Chance(0.5);
		Wahrheit.Abstand = Lage.Bereich(0.0, 0.15);
		const FLinienEntscheidung Entscheidung = EntscheideLinie(Wahrheit, Parameter, Zufall);
		MB_PRUEFE(Entscheidung.bFehlentscheidung == (Entscheidung.bGutGegeben != Wahrheit.bIn));
		Fehler += Entscheidung.bFehlentscheidung ? 1 : 0;
		if (Wahrheit.Abstand < 0.03)
		{
			++AnzahlSehrKnapp;
			FehlerSehrKnapp += Entscheidung.bFehlentscheidung ? 1 : 0;
		}
	}
	const double Quote = static_cast<double>(Fehler) / Anzahl;
	const double QuoteKnapp = static_cast<double>(FehlerSehrKnapp) / AnzahlSehrKnapp;
	MB_INFO("Fehlerquote knappe Baelle %.4f, bei d < 3 cm %.4f", Quote, QuoteKnapp);
	MB_PRUEFE_NAH(Quote, 0.10, 0.004);
	MB_PRUEFE(QuoteKnapp > 0.17);

	// Elektronische Linienrichter: nie ein Fehler
	FLinienrichterParameter Elektronisch;
	Elektronisch.bElektronisch = true;
	for (int Index = 0; Index < 1000; ++Index)
	{
		FLinienUrteil Wahrheit;
		Wahrheit.bIn = (Index % 2) == 0;
		Wahrheit.Abstand = 0.0;
		MB_PRUEFE(!EntscheideLinie(Wahrheit, Elektronisch, Zufall).bFehlentscheidung);
	}
}

MB_TEST(ChallengeRegeln, "Challenge.Regeln")
{
	// Aufschlag faelschlich als Fehler gegeben -> erster Aufschlag bzw. Ass
	{
		FStrittigeEntscheidung E;
		E.Art = EEntscheidungsArt::AufschlagFehlerGerufen;
		E.Schlagender = 0;
		E.bWarIn = true;
		E.bZweiterAufschlag = true;
		MB_PRUEFE_GLEICH(Benachteiligter(E), 0);
		FChallengeAuswertung A = WerteChallengeAus(E);
		MB_PRUEFE(A.bErfolgreich);
		MB_PRUEFE(A.Folge == EChallengeFolge::ErsterAufschlag);
		MB_PRUEFE_GLEICH(A.Punktgewinner, -1);
		E.bRueckschlaegerChancenlos = true;
		A = WerteChallengeAus(E);
		MB_PRUEFE(A.Folge == EChallengeFolge::AssFuerAufschlaeger);
		MB_PRUEFE_GLEICH(A.Punktgewinner, 0);
		E.bWarIn = false;
		MB_PRUEFE(WerteChallengeAus(E).Folge == EChallengeFolge::EntscheidungBleibt);
	}
	// Im Ballwechsel faelschlich Aus -> Wiederholung bzw. Punkt bei klarem Winner
	{
		FStrittigeEntscheidung E;
		E.Art = EEntscheidungsArt::BallAusGerufen;
		E.Schlagender = 1;
		E.bWarIn = true;
		MB_PRUEFE_GLEICH(Benachteiligter(E), 1);
		MB_PRUEFE(WerteChallengeAus(E).Folge == EChallengeFolge::PunktWiederholen);
		E.bKlarerWinner = true;
		const FChallengeAuswertung A = WerteChallengeAus(E);
		MB_PRUEFE(A.Folge == EChallengeFolge::PunktFuerSchlagenden);
		MB_PRUEFE_GLEICH(A.Punktgewinner, 1);
	}
	// Ball faelschlich gut gegeben -> Punkt fuer den Challenger
	{
		FStrittigeEntscheidung E;
		E.Art = EEntscheidungsArt::BallGutGegeben;
		E.Schlagender = 1;
		E.bWarIn = false;
		MB_PRUEFE_GLEICH(Benachteiligter(E), 0);
		const FChallengeAuswertung A = WerteChallengeAus(E);
		MB_PRUEFE(A.Folge == EChallengeFolge::PunktFuerChallenger);
		MB_PRUEFE_GLEICH(A.Punktgewinner, 0);
	}
	// Aufschlag faelschlich gut gegeben -> Fehler bzw. Doppelfehler
	{
		FStrittigeEntscheidung E;
		E.Art = EEntscheidungsArt::AufschlagGutGegeben;
		E.Schlagender = 0;
		E.bWarIn = false;
		MB_PRUEFE_GLEICH(Benachteiligter(E), 1);
		MB_PRUEFE(WerteChallengeAus(E).Folge == EChallengeFolge::Aufschlagfehler);
		E.bZweiterAufschlag = true;
		const FChallengeAuswertung A = WerteChallengeAus(E);
		MB_PRUEFE(A.Folge == EChallengeFolge::Doppelfehler);
		MB_PRUEFE_GLEICH(A.Punktgewinner, 1);
	}
}

MB_TEST(ChallengeKonto, "Challenge.KontoDreiProSatzPlusTiebreak")
{
	FChallengeKonto Konto;
	FStrittigeEntscheidung Falsch;     // Ball war gut, wurde aus gegeben -> erfolgreich
	Falsch.Art = EEntscheidungsArt::BallAusGerufen;
	Falsch.Schlagender = 0;
	Falsch.bWarIn = true;
	FStrittigeEntscheidung Richtig = Falsch;   // Ball war aus -> erfolglos
	Richtig.bWarIn = false;

	FChallengeAuswertung Auswertung;
	// Nur der Benachteiligte darf challengen
	MB_PRUEFE(!Konto.KannChallengen(1, Falsch));
	MB_PRUEFE(!Konto.Challenge(1, Falsch, Auswertung));
	MB_PRUEFE_GLEICH(Konto.Verbleibend(1), 3);

	// Erfolgreiche Challenges bleiben erhalten
	MB_PRUEFE(Konto.Challenge(0, Falsch, Auswertung));
	MB_PRUEFE(Auswertung.bErfolgreich);
	MB_PRUEFE_GLEICH(Konto.Verbleibend(0), 3);
	// Drei erfolglose -> keine mehr
	for (int Index = 0; Index < 3; ++Index)
	{
		MB_PRUEFE(Konto.Challenge(0, Richtig, Auswertung));
		MB_PRUEFE(!Auswertung.bErfolgreich);
	}
	MB_PRUEFE_GLEICH(Konto.Verbleibend(0), 0);
	MB_PRUEFE(!Konto.KannChallengen(0, Falsch));
	// Tiebreak: +1
	Konto.TiebreakBeginnt();
	MB_PRUEFE_GLEICH(Konto.Verbleibend(0), 1);
	MB_PRUEFE_GLEICH(Konto.Verbleibend(1), 4);
	// Neuer Satz: wieder 3 (nicht genutzte verfallen)
	Konto.NeuerSatz();
	MB_PRUEFE_GLEICH(Konto.Verbleibend(0), 3);
	MB_PRUEFE_GLEICH(Konto.Verbleibend(1), 3);
	// Elektronische Linienrichter: keine Challenges
	Konto.SetzeAktiv(false);
	MB_PRUEFE(!Konto.KannChallengen(0, Falsch));
}

MB_TEST(ChallengeKI, "Challenge.KIVerhalten")
{
	const FKIChallengeParameter Parameter;
	FStrittigeEntscheidung E;
	E.Art = EEntscheidungsArt::BallAusGerufen;
	E.bWarIn = true;
	E.Abstand = 0.01;
	const double Knapp = KIChallengeWahrscheinlichkeit(E, 3, 3, false, Parameter);
	E.Abstand = 0.06;
	const double Weiter = KIChallengeWahrscheinlichkeit(E, 3, 3, false, Parameter);
	E.Abstand = 0.5;
	const double Klar = KIChallengeWahrscheinlichkeit(E, 3, 3, false, Parameter);
	const double KlarWichtig = KIChallengeWahrscheinlichkeit(E, 3, 3, true, Parameter);
	MB_PRUEFE(Knapp > Weiter);
	MB_PRUEFE(Weiter > 0.0);
	MB_PRUEFE_NAH(Klar, 0.0, 1e-12);            // nicht bei klaren Baellen
	MB_PRUEFE_NAH(KlarWichtig, 0.0, 1e-12);
	E.Abstand = 0.12;
	MB_PRUEFE(KIChallengeWahrscheinlichkeit(E, 3, 3, true, Parameter) > 0.0);   // selten strategisch
	E.Abstand = 0.01;
	MB_PRUEFE(KIChallengeWahrscheinlichkeit(E, 1, 3, false, Parameter) < Knapp);
	MB_PRUEFE_NAH(KIChallengeWahrscheinlichkeit(E, 0, 3, true, Parameter), 0.0, 1e-12);
	E.bWarIn = false;   // Entscheidung war richtig -> KI challengt seltener
	MB_PRUEFE(KIChallengeWahrscheinlichkeit(E, 3, 3, false, Parameter) < Knapp);
}

MB_TEST(ZufallDeterminismus, "Zufall.Determinismus")
{
	// Referenzwerte des PCG32-Beispielprogramms (Startwert 42, Folge 54)
	FZufall Referenz(42u, 54u);
	const std::uint32_t Erwartet[6] = {0xa15c02b7u, 0x7b47f409u, 0xba1d3330u, 0x83d2f293u, 0xbfa4784bu, 0xcbed606eu};
	for (std::uint32_t Wert : Erwartet)
	{
		MB_PRUEFE_GLEICH(Referenz.Naechste(), Wert);
	}
	FZufall A(99u);
	FZufall B(99u);
	double Summe = 0.0;
	double SummeQuadrat = 0.0;
	bool bGanzzahlOk = true;
	for (int Index = 0; Index < 100000; ++Index)
	{
		const double Wert = A.Normal();
		MB_PRUEFE_GLEICH(Wert, B.Normal());
		Summe += Wert;
		SummeQuadrat += Wert * Wert;
		const int Zahl = A.Ganzzahl(1, 4);
		B.Ganzzahl(1, 4);
		bGanzzahlOk = bGanzzahlOk && Zahl >= 1 && Zahl <= 4;
		const double Gleich = A.Gleich01();
		B.Gleich01();
		bGanzzahlOk = bGanzzahlOk && Gleich >= 0.0 && Gleich < 1.0;
	}
	MB_PRUEFE(bGanzzahlOk);
	MB_PRUEFE_NAH(Summe / 100000.0, 0.0, 0.02);
	MB_PRUEFE_NAH(SummeQuadrat / 100000.0, 1.0, 0.03);
	MB_PRUEFE(MischeStartwert(1u, 2u) != MischeStartwert(2u, 1u));
}
