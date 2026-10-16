// Matchball Tennis - Tests der Schlagberechnung und Streuung (README 5.3, 5.4, 6.2)

#include "Kern/MBTestDaten.h"
#include "Kern/MBTestRahmen.h"

#include "Physics/Kern/MBSchlagberechnung.h"
#include "Physics/Kern/MBVorhersage.h"

using namespace MB;

namespace
{
	double MBSchlagTest_Abweichung(const FSchlagErgebnis& Ergebnis, double X, double Y)
	{
		return std::sqrt(Quadrat(Ergebnis.Aufsprung.X - X) + Quadrat(Ergebnis.Aufsprung.Y - Y));
	}
}

MB_TEST(SchlagTopspinZiel, "Schlag.TopspinTrifftZiel")
{
	const FBallSimulation Simulation = MBTest::Simulation("Hart");
	FSchlagAnfrage Anfrage;
	Anfrage.Treffpunkt = FVek3(1.5, -11.8, 0.9);
	Anfrage.Tempo = KmhZuMs(120.0);
	Anfrage.MinTempo = KmhZuMs(70.0);
	Anfrage.Topspin = UpmZuRadS(2500.0);
	const double Ziele[4][2] = {{-2.5, 9.5}, {2.8, 10.0}, {0.0, 7.0}, {-3.5, 5.5}};
	for (const auto& Ziel : Ziele)
	{
		Anfrage.ZielX = Ziel[0];
		Anfrage.ZielY = Ziel[1];
		const FSchlagErgebnis Ergebnis = BerechneSchlag(Simulation, Anfrage);
		MB_INFO("Ziel (%.1f, %.1f): Aufsprung (%.2f, %.2f), Netz +%.2f m, Tempo %.0f km/h, Flugzeit %.2f s", Ziel[0], Ziel[1], Ergebnis.Aufsprung.X, Ergebnis.Aufsprung.Y, Ergebnis.NetzAbstand, MsZuKmh(Ergebnis.TatsaechlichesTempo), Ergebnis.Flugzeit);
		MB_PRUEFE(Ergebnis.bZielErreicht);
		MB_PRUEFE(Ergebnis.bNetzFrei);
		MB_PRUEFE(Ergebnis.NetzAbstand >= Anfrage.MinNetzAbstand - 1e-9);
		MB_PRUEFE(MBSchlagTest_Abweichung(Ergebnis, Ziel[0], Ziel[1]) < 0.15);
		// Nachrechnen: der Abflugzustand trifft tatsaechlich (Determinismus der Suche)
		FVorhersageOptionen Optionen;
		Optionen.MaxAufspruenge = 1;
		const FFlugbahn Bahn = FVorhersage::Berechne(Simulation, Ergebnis.Abflug, Optionen);
		const FBallEreignis* Aufsprung = Bahn.ErsterAufsprung();
		MB_PRUEFE(Aufsprung != nullptr && Aufsprung->Position == Ergebnis.Aufsprung);
	}
}

MB_TEST(SchlagTempoWirdGesenkt, "Schlag.TempoWirdFuersNetzGesenkt")
{
	// Flacher, harter Schlag von tief unten kurz hinters Netz: Tempo muss sinken, damit der Ball
	// ueber das Netz kommt und trotzdem im Feld landet
	const FBallSimulation Simulation = MBTest::Simulation("Hart");
	FSchlagAnfrage Anfrage;
	Anfrage.Treffpunkt = FVek3(0.0, -11.0, 0.35);
	Anfrage.ZielX = 1.0;
	Anfrage.ZielY = 5.0;
	Anfrage.Tempo = KmhZuMs(150.0);
	Anfrage.MinTempo = KmhZuMs(50.0);
	Anfrage.Topspin = UpmZuRadS(600.0);
	Anfrage.MinNetzAbstand = 0.15;
	const FSchlagErgebnis Ergebnis = BerechneSchlag(Simulation, Anfrage);
	MB_INFO("Tempo %.0f km/h, Netz +%.2f m, Aufsprung (%.2f, %.2f)", MsZuKmh(Ergebnis.TatsaechlichesTempo), Ergebnis.NetzAbstand, Ergebnis.Aufsprung.X, Ergebnis.Aufsprung.Y);
	MB_PRUEFE(Ergebnis.TatsaechlichesTempo < Anfrage.Tempo);
	MB_PRUEFE(Ergebnis.bNetzFrei);
	MB_PRUEFE(Ergebnis.bZielErreicht);
	MB_PRUEFE(Ergebnis.NetzAbstand >= 0.15 - 1e-9);
}

MB_TEST(SchlagLob, "Schlag.LobHoherBogen")
{
	const FBallSimulation Simulation = MBTest::Simulation("Sand");
	FSchlagAnfrage Anfrage;
	Anfrage.Treffpunkt = FVek3(0.0, -12.0, 0.8);
	Anfrage.ZielX = 0.5;
	Anfrage.ZielY = 10.0;
	Anfrage.Tempo = KmhZuMs(68.0);
	Anfrage.MinTempo = KmhZuMs(40.0);
	Anfrage.Topspin = UpmZuRadS(-800.0);
	Anfrage.bHoherBogen = true;
	const FSchlagErgebnis Ergebnis = BerechneSchlag(Simulation, Anfrage);
	MB_PRUEFE(Ergebnis.bZielErreicht);
	FVorhersageOptionen Optionen;
	Optionen.MaxAufspruenge = 1;
	const FFlugbahn Bahn = FVorhersage::Berechne(Simulation, Ergebnis.Abflug, Optionen);
	double Scheitel = 0.0;
	for (const FBahnPunkt& Punkt : Bahn.Punkte)
	{
		Scheitel = MaxWert(Scheitel, Punkt.Position.Z);
	}
	MB_INFO("Lob: Scheitel %.2f m, Flugzeit %.2f s", Scheitel, Ergebnis.Flugzeit);
	// Defensiver Lob von Grundlinie zu Grundlinie: Scheitel etwa 6-13 m, lange Flugzeit
	MB_PRUEFE(Scheitel > 6.0 && Scheitel < 13.0);
	MB_PRUEFE(Ergebnis.Flugzeit > 2.0);
}

MB_TEST(SchlagAufschlag, "Schlag.AufschlagInsFeld")
{
	const FBallSimulation Simulation = MBTest::Simulation("Rasen");
	const FPlatz Platz;
	const FFeld Feld = Platz.Aufschlagfeld(EPlatzseite::Nah, EAufschlagHaelfte::Einstand);
	FSchlagAnfrage Anfrage;
	Anfrage.Treffpunkt = FVek3(0.6, -12.0, 2.75);
	// "T": nahe der Mittellinie, kurz vor der Aufschlaglinie
	Anfrage.ZielX = -0.3;
	Anfrage.ZielY = 5.8;
	Anfrage.Tempo = KmhZuMs(205.0);
	Anfrage.MinTempo = KmhZuMs(150.0);
	Anfrage.Topspin = UpmZuRadS(900.0);
	Anfrage.MinNetzAbstand = 0.05;
	const FSchlagErgebnis Ergebnis = BerechneSchlag(Simulation, Anfrage);
	MB_INFO("Aufschlag: Aufsprung (%.2f, %.2f), Netz +%.2f m, %.0f km/h", Ergebnis.Aufsprung.X, Ergebnis.Aufsprung.Y, Ergebnis.NetzAbstand, MsZuKmh(Ergebnis.TatsaechlichesTempo));
	MB_PRUEFE(Ergebnis.bZielErreicht);
	MB_PRUEFE(Ergebnis.bNetzFrei);
	MB_PRUEFE(Feld.Enthaelt(Ergebnis.Aufsprung.X, Ergebnis.Aufsprung.Y));
	MB_PRUEFE(Ergebnis.TatsaechlichesTempo > KmhZuMs(180.0));
}

MB_TEST(SchlagUnerreichbar, "Schlag.ZielZuWeit")
{
	// Viel zu langsam fuer ein Ziel an der Grundlinie: Ball landet zu kurz, kein Absturz
	const FBallSimulation Simulation = MBTest::Simulation("Hart");
	FSchlagAnfrage Anfrage;
	Anfrage.Treffpunkt = FVek3(0.0, -12.0, 0.9);
	Anfrage.ZielX = 0.0;
	Anfrage.ZielY = 11.0;
	Anfrage.Tempo = KmhZuMs(25.0);
	Anfrage.MinTempo = KmhZuMs(25.0);
	const FSchlagErgebnis Ergebnis = BerechneSchlag(Simulation, Anfrage);
	MB_PRUEFE(!Ergebnis.bZielErreicht);
	MB_PRUEFE(Ergebnis.Aufsprung.Y < 11.0);
}

MB_TEST(SchlagStreuungSymmetrisch, "Schlag.StreuungLinksRechtsGleich")
{
	FStreuParameter Parameter;
	FZufall Zufall(12345u);
	const FVek3 Treffpunkt(0.0, -12.0, 1.0);
	int Links = 0;
	int Rechts = 0;
	double SummeSeitlich = 0.0;
	double BetragPerfekt = 0.0;
	double BetragSpaet = 0.0;
	constexpr int Anzahl = 20000;
	for (int Index = 0; Index < Anzahl; ++Index)
	{
		// Gleicher Timing-Fehler (zu spaet) - die Seite muss trotzdem zufaellig sein
		const FStreuErgebnis Spaet = StreueZiel(Treffpunkt, 0.0, 9.0, 0.080, Parameter, Zufall);
		(Spaet.Seitlich > 0.0 ? Links : Rechts) += 1;
		SummeSeitlich += Spaet.Seitlich;
		BetragSpaet += Betrag(Spaet.Seitlich);
		const FStreuErgebnis Perfekt = StreueZiel(Treffpunkt, 0.0, 9.0, 0.0, Parameter, Zufall);
		BetragPerfekt += Betrag(Perfekt.Seitlich);
		MB_PRUEFE(Perfekt.Bewertung == ETimingBewertung::Gruen);
	}
	const double AnteilLinks = static_cast<double>(Links) / Anzahl;
	MB_INFO("Links %.3f, mittlere Abweichung perfekt %.2f m, 80 ms spaet %.2f m", AnteilLinks, BetragPerfekt / Anzahl, BetragSpaet / Anzahl);
	MB_PRUEFE_NAH(AnteilLinks, 0.5, 0.02);
	MB_PRUEFE_NAH(SummeSeitlich / Anzahl, 0.0, 0.05);
	MB_PRUEFE(BetragSpaet > 3.0 * BetragPerfekt);
	MB_PRUEFE(Rechts > 0);
}

MB_TEST(SchlagTimingBewertung, "Schlag.TimingFensterUndModifikatoren")
{
	const FStreuParameter Grund;
	MB_PRUEFE(BewerteTiming(0.0, Grund) == ETimingBewertung::Gruen);
	MB_PRUEFE(BewerteTiming(-Grund.GruenHalbfenster, Grund) == ETimingBewertung::Gruen);
	MB_PRUEFE(BewerteTiming(Grund.GruenHalbfenster + 0.001, Grund) == ETimingBewertung::Gelb);
	MB_PRUEFE(BewerteTiming(-(Grund.GelbHalbfenster + 0.001), Grund) == ETimingBewertung::Rot);

	const FStreuModifikatoren Mod;
	const FStreuParameter Normal = WendeModifikatorenAn(Grund, Mod, false, false, 75.0);
	const FStreuParameter Power = WendeModifikatorenAn(Grund, Mod, true, false, 75.0);
	const FStreuParameter Praezision = WendeModifikatorenAn(Grund, Mod, false, true, 75.0);
	// Power: kleineres Fenster, groessere Streuung; Praezision umgekehrt
	MB_PRUEFE(Power.GruenHalbfenster < Normal.GruenHalbfenster);
	MB_PRUEFE(Power.SeitlichProSekunde > Normal.SeitlichProSekunde);
	MB_PRUEFE(Praezision.GruenHalbfenster > Normal.GruenHalbfenster);
	MB_PRUEFE(Praezision.Grundstreuung < Normal.Grundstreuung);
	// Besserer Spielerwert: groesseres Fenster, weniger Streuung
	const FStreuParameter Schwach = WendeModifikatorenAn(Grund, Mod, false, false, 50.0);
	const FStreuParameter Stark = WendeModifikatorenAn(Grund, Mod, false, false, 99.0);
	MB_PRUEFE(Stark.GruenHalbfenster > Schwach.GruenHalbfenster);
	MB_PRUEFE(Stark.Grundstreuung < Schwach.Grundstreuung);
	// Match-Einstellung "Timing-Fenster" wirkt auf beide gleich
	FStreuModifikatoren Gross = Mod;
	Gross.MatchFenster = 1.5;
	MB_PRUEFE_NAH(WendeModifikatorenAn(Grund, Gross, false, false, 75.0).GruenHalbfenster, Normal.GruenHalbfenster * 1.5, 1e-12);
}

MB_TEST(SchlagDrallRichtung, "Schlag.DrallRichtung")
{
	// Topspin beim Flug nach +Y: Achse -X (Oberseite dreht nach vorne)
	const FVek3 Drall = DrallFuerRichtung(FVek3(0.0, 1.0, 0.1), 100.0, 0.0);
	MB_PRUEFE_NAH(Drall.X, -100.0, 1e-9);
	MB_PRUEFE_NAH(Drall.Y, 0.0, 1e-9);
	// Geschwindigkeit des obersten Punkts = w x (0,0,r) zeigt in Flugrichtung
	const FVek3 Oben = Kreuzprodukt(Drall, FVek3(0.0, 0.0, 0.0335));
	MB_PRUEFE(Oben.Y > 0.0);
}
