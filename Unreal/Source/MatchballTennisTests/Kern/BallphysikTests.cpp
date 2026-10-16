// Matchball Tennis - Tests der Ballphysik (README 6, Abnahme M1: Flugzeit, Absprunghoehe je Belag, Aufschlagtempo)
//
// Richtwerte (Herleitung in Dokumente/Werte.md):
//  - ITF-Ballpruefung: aus 254 cm fallen gelassen springt der Ball 135-147 cm hoch (Unterkante)
//  - Aufschlag 200 km/h: ca. 30 % Tempoverlust bis zum Aufsprung, ca. 0,55-0,7 s bis zur Grundlinie
//  - Absprunghoehe: Sand > Hart > Rasen; Court Pace Rating: Sand langsam (<= 29), Rasen/Halle schnell (>= 45)

#include "Kern/MBTestDaten.h"
#include "Kern/MBTestRahmen.h"

#include "Physics/Kern/MBBallPhysik.h"
#include "Physics/Kern/MBVorhersage.h"

#include <cstring>

using namespace MB;

namespace
{
	struct FMBPhysikTestFlug
	{
		bool bAufsprung = false;
		FBallEreignis Aufsprung;
		double HoeheNachAufsprung = 0.0;     // hoechster Punkt (Ballunterkante) nach dem 1. Aufsprung
		double NetzHoehe = -1.0;             // Ballmittelpunkt ueber der Netzebene
		bool bNetzBeruehrt = false;
		bool bGeflecht = false;
		double ZeitBisGrundlinie = -1.0;     // Zeit, bis der Ball die gegnerische Grundlinie (|Y| = 11,885) quert
		FBallZustand Ende;
	};

	FMBPhysikTestFlug MBPhysikTest_Fliege(FBallSimulation Simulation, const FBallZustand& Start, double MaxZeit = 4.0)
	{
		FMBPhysikTestFlug Flug;
		Simulation.Starte(Start);
		const double Richtung = Start.Geschwindigkeit.Y >= 0.0 ? 1.0 : -1.0;
		bool bAbwaerts = false;
		while (Simulation.GetZeit() < MaxZeit && !Simulation.GetZustand().bRuht)
		{
			const std::size_t Vorher = Simulation.GetEreignisse().size();
			const double YVorher = Simulation.GetZustand().Position.Y;
			Simulation.Schritt();
			const FBallZustand& Z = Simulation.GetZustand();
			for (std::size_t Index = Vorher; Index < Simulation.GetEreignisse().size(); ++Index)
			{
				const FBallEreignis& E = Simulation.GetEreignisse()[Index];
				if (E.Art == EBallEreignisArt::Aufsprung && !Flug.bAufsprung)
				{
					Flug.bAufsprung = true;
					Flug.Aufsprung = E;
				}
				else if (E.Art == EBallEreignisArt::NetzUeberquert && Flug.NetzHoehe < 0.0)
				{
					Flug.NetzHoehe = E.Position.Z;
				}
				else if (E.Art == EBallEreignisArt::Netzband)
				{
					Flug.bNetzBeruehrt = true;
				}
				else if (E.Art == EBallEreignisArt::Netzgeflecht)
				{
					Flug.bNetzBeruehrt = true;
					Flug.bGeflecht = true;
				}
			}
			if (Flug.ZeitBisGrundlinie < 0.0 && Richtung * YVorher < Platzmass::HalbeLaenge && Richtung * Z.Position.Y >= Platzmass::HalbeLaenge)
			{
				Flug.ZeitBisGrundlinie = Simulation.GetZeit();
			}
			if (Flug.bAufsprung)
			{
				const double Unterkante = Z.Position.Z - Simulation.GetBall().Radius;
				if (Unterkante > Flug.HoeheNachAufsprung)
				{
					Flug.HoeheNachAufsprung = Unterkante;
				}
				if (Z.Geschwindigkeit.Z < 0.0 && Unterkante > 0.05)
				{
					bAbwaerts = true;
				}
				if (bAbwaerts && Flug.ZeitBisGrundlinie >= 0.0)
				{
					break;
				}
			}
		}
		Flug.Ende = Simulation.GetZustand();
		return Flug;
	}

	// Topspin-Grundschlag von der nahen Grundlinie Richtung ferne Grundlinie
	FBallZustand MBPhysikTest_Grundschlag(double Kmh, double Steigung, double Upm)
	{
		FBallZustand Start;
		Start.Position = FVek3(0.0, -12.0, 1.0);
		const FVek3 Richtung = FVek3(0.0, 1.0, Steigung).Normalisiert();
		Start.Geschwindigkeit = Richtung * KmhZuMs(Kmh);
		Start.Drall = Kreuzprodukt(FVek3(0.0, 0.0, 1.0), FVek3(0.0, 1.0, 0.0)) * UpmZuRadS(Upm);
		return Start;
	}

	// Flacher Aufschlag aus 2,8 m Treffhoehe ins Einstand-Aufschlagfeld
	FBallZustand MBPhysikTest_Aufschlag(double Kmh, double Steigung)
	{
		FBallZustand Start;
		Start.Position = FVek3(0.5, -12.0, 2.8);
		FVek3 Horizontal(-2.5, 17.5, 0.0);
		Horizontal = Horizontal.Normalisiert();
		Start.Geschwindigkeit = FVek3(Horizontal.X, Horizontal.Y, Steigung).Normalisiert() * KmhZuMs(Kmh);
		Start.Drall = Kreuzprodukt(FVek3(0.0, 0.0, 1.0), Horizontal) * UpmZuRadS(1000.0);
		return Start;
	}
}

MB_TEST(BallphysikDatenVorhanden, "Ballphysik.DatenVorhanden")
{
	const char* Namen[4] = {"Hart", "Sand", "Rasen", "Halle"};
	for (const char* Name : Namen)
	{
		bool bOk = false;
		MBTest::LadeBelag(Name, &bOk);
		MB_PRUEFE(bOk);
	}
	const FBallParameter Ball = MBTest::LadeBall();
	// ITF-Spezifikation: Masse 56,0-59,4 g, Durchmesser 6,54-6,86 cm
	MB_PRUEFE(Ball.Masse >= 0.0560 && Ball.Masse <= 0.0594);
	MB_PRUEFE(Ball.Radius * 2.0 >= 0.0654 && Ball.Radius * 2.0 <= 0.0686);
}

MB_TEST(BallphysikItfFallversuch, "Ballphysik.ItfFallversuch")
{
	// Ballpruefung der ITF: Fall aus 254 cm (Unterkante) auf Beton -> Ruecksprung 135-147 cm.
	// Pruefbelag: Beton als starre Unterlage, Stosszahl bei senkrechtem Aufprall (Referenz 6,9 m/s).
	FBelagParameter Beton;
	Beton.Name = "Pruefbeton";
	Beton.Stosszahl = 0.77;
	Beton.StosszahlReferenz = 6.9;
	FBallSimulation Simulation(MBTest::LadeBall(), Beton, MBTest::LadeNetz());
	FBallZustand Start;
	Start.Position = FVek3(0.0, 5.0, 2.54 + Simulation.GetBall().Radius);
	const FMBPhysikTestFlug Flug = MBPhysikTest_Fliege(Simulation, Start, 3.0);
	MB_PRUEFE(Flug.bAufsprung);
	MB_INFO("Ruecksprung %.3f m, Aufprall %.2f m/s", Flug.HoeheNachAufsprung, -Flug.Aufsprung.GeschwindigkeitVorher.Z);
	MB_PRUEFE(Flug.HoeheNachAufsprung >= 1.35 && Flug.HoeheNachAufsprung <= 1.47);
	// Luftwiderstand bremst leicht: Aufprall knapp unter sqrt(2 g h) = 7,06 m/s
	MB_PRUEFE(-Flug.Aufsprung.GeschwindigkeitVorher.Z > 6.7 && -Flug.Aufsprung.GeschwindigkeitVorher.Z < 7.06);
}

MB_TEST(BallphysikCourtPace, "Ballphysik.CourtPaceRating")
{
	const double Sand = MBTest::LadeBelag("Sand").CourtPaceRating();
	const double Hart = MBTest::LadeBelag("Hart").CourtPaceRating();
	const double Rasen = MBTest::LadeBelag("Rasen").CourtPaceRating();
	const double Halle = MBTest::LadeBelag("Halle").CourtPaceRating();
	MB_INFO("CPR Sand %.1f, Hart %.1f, Rasen %.1f, Halle %.1f", Sand, Hart, Rasen, Halle);
	MB_PRUEFE(Sand <= 29.0);                 // ITF "langsam"
	MB_PRUEFE(Hart >= 35.0 && Hart <= 44.0); // "mittel" bis "mittel-schnell"
	MB_PRUEFE(Rasen >= 45.0);                // "schnell"
	MB_PRUEFE(Halle >= 45.0);                // "schnell"

	// Feineinstellung je Turnier: CPR-Zielwert wird ueber die Reibung erreicht
	FBelagParameter Langsam = MBTest::LadeBelag("Hart");
	Langsam.SetzeCourtPace(33.0);
	MB_PRUEFE_NAH(Langsam.CourtPaceRating(), 33.0, 1e-9);
}

MB_TEST(BallphysikAufschlagTempo, "Ballphysik.AufschlagTempoUndFlugzeit")
{
	const FMBPhysikTestFlug Flug = MBPhysikTest_Fliege(MBTest::Simulation("Hart"), MBPhysikTest_Aufschlag(200.0, -0.095));
	MB_PRUEFE(Flug.bAufsprung);
	MB_PRUEFE(!Flug.bNetzBeruehrt);
	const double VorAufsprung = MsZuKmh(Flug.Aufsprung.GeschwindigkeitVorher.Laenge());
	const double NachAufsprung = MsZuKmh(Flug.Aufsprung.GeschwindigkeitNachher.Laenge());
	MB_INFO("Aufschlag 200 km/h: Aufsprung bei Y=%.2f nach %.3f s, %.0f -> %.0f km/h, Grundlinie nach %.3f s", Flug.Aufsprung.Position.Y, Flug.Aufsprung.Zeit, VorAufsprung, NachAufsprung, Flug.ZeitBisGrundlinie);
	// Im Einstand-Aufschlagfeld (zwischen Netz und Aufschlaglinie, links der Mittellinie)
	MB_PRUEFE(Flug.Aufsprung.Position.Y > 0.0 && Flug.Aufsprung.Position.Y < Platzmass::AufschlaglinieVomNetz);
	MB_PRUEFE(Flug.Aufsprung.Position.X < 0.0 && Flug.Aufsprung.Position.X > -Platzmass::HalbeBreiteEinzel);
	// Ca. 25-35 % Tempoverlust im Flug, weitere 15-35 % beim Aufsprung
	MB_PRUEFE(VorAufsprung > 130.0 && VorAufsprung < 150.0);
	MB_PRUEFE(NachAufsprung > 90.0 && NachAufsprung < 125.0);
	// Reaktionszeit des Rueckschlaegers an der Grundlinie
	MB_PRUEFE(Flug.ZeitBisGrundlinie > 0.5 && Flug.ZeitBisGrundlinie < 0.75);
}

MB_TEST(BallphysikAbsprungJeBelag, "Ballphysik.AbsprungJeBelag")
{
	// Gleicher Topspin-Grundschlag (120 km/h, 2500 U/min) auf allen Belaegen
	const FBallZustand Start = MBPhysikTest_Grundschlag(120.0, 0.16, 2500.0);
	const FMBPhysikTestFlug Sand = MBPhysikTest_Fliege(MBTest::Simulation("Sand"), Start);
	const FMBPhysikTestFlug Hart = MBPhysikTest_Fliege(MBTest::Simulation("Hart"), Start);
	const FMBPhysikTestFlug Rasen = MBPhysikTest_Fliege(MBTest::Simulation("Rasen"), Start);
	MB_INFO("Absprunghoehe Sand %.2f m, Hart %.2f m, Rasen %.2f m; Flugzeit %.3f s, Netz %.2f m, Aufsprung Y=%.2f", Sand.HoeheNachAufsprung, Hart.HoeheNachAufsprung, Rasen.HoeheNachAufsprung, Hart.Aufsprung.Zeit, Hart.NetzHoehe, Hart.Aufsprung.Position.Y);
	MB_PRUEFE(Sand.bAufsprung && Hart.bAufsprung && Rasen.bAufsprung);
	MB_PRUEFE(Sand.HoeheNachAufsprung > Hart.HoeheNachAufsprung);
	MB_PRUEFE(Hart.HoeheNachAufsprung > Rasen.HoeheNachAufsprung);
	MB_PRUEFE(Rasen.HoeheNachAufsprung > 0.6 && Sand.HoeheNachAufsprung < 1.8);
	// Grundschlag: Flugzeit bis zum Aufsprung 0,6-1,1 s, landet im Feld
	MB_PRUEFE(Hart.Aufsprung.Zeit > 0.6 && Hart.Aufsprung.Zeit < 1.1);
	MB_PRUEFE(Hart.Aufsprung.Position.Y > 0.0 && Hart.Aufsprung.Position.Y < Platzmass::HalbeLaenge);
	MB_PRUEFE(!Hart.bNetzBeruehrt);

	// Flacher, schneller Ball ohne viel Drall: auf Sand langsamer nach dem Aufsprung als auf Rasen
	const FBallZustand Flach = MBPhysikTest_Grundschlag(140.0, 0.07, 600.0);
	const FMBPhysikTestFlug FlachSand = MBPhysikTest_Fliege(MBTest::Simulation("Sand"), Flach);
	const FMBPhysikTestFlug FlachRasen = MBPhysikTest_Fliege(MBTest::Simulation("Rasen"), Flach);
	const double TempoSand = FlachSand.Aufsprung.GeschwindigkeitNachher.LaengeXY();
	const double TempoRasen = FlachRasen.Aufsprung.GeschwindigkeitNachher.LaengeXY();
	MB_INFO("Flacher Ball nach Aufsprung: Sand %.1f km/h, Rasen %.1f km/h", MsZuKmh(TempoSand), MsZuKmh(TempoRasen));
	MB_PRUEFE(TempoSand < TempoRasen);
}

MB_TEST(BallphysikDrall, "Ballphysik.DrallMagnus")
{
	// Gleicher Abschuss: Topspin taucht ab (landet kuerzer), Unterschnitt traegt (landet laenger)
	const FMBPhysikTestFlug Topspin = MBPhysikTest_Fliege(MBTest::Simulation("Hart"), MBPhysikTest_Grundschlag(90.0, 0.22, 2500.0));
	const FMBPhysikTestFlug Ohne = MBPhysikTest_Fliege(MBTest::Simulation("Hart"), MBPhysikTest_Grundschlag(90.0, 0.22, 0.0));
	const FMBPhysikTestFlug Slice = MBPhysikTest_Fliege(MBTest::Simulation("Hart"), MBPhysikTest_Grundschlag(90.0, 0.22, -2000.0));
	MB_PRUEFE(!Topspin.bNetzBeruehrt && !Ohne.bNetzBeruehrt && !Slice.bNetzBeruehrt);
	MB_PRUEFE(Topspin.Aufsprung.Position.Y > 0.0);
	MB_INFO("Aufsprung Y: Topspin %.2f, ohne Drall %.2f, Slice %.2f", Topspin.Aufsprung.Position.Y, Ohne.Aufsprung.Position.Y, Slice.Aufsprung.Position.Y);
	MB_PRUEFE(Topspin.Aufsprung.Position.Y < Ohne.Aufsprung.Position.Y);
	MB_PRUEFE(Slice.Aufsprung.Position.Y > Ohne.Aufsprung.Position.Y);

	// Topspin springt flacher ab (behaelt Vorwaertstempo), Slice wird beim Aufsprung staerker gebremst
	const double VerlustTop = Topspin.Aufsprung.GeschwindigkeitNachher.LaengeXY() / Topspin.Aufsprung.GeschwindigkeitVorher.LaengeXY();
	const double VerlustSlice = Slice.Aufsprung.GeschwindigkeitNachher.LaengeXY() / Slice.Aufsprung.GeschwindigkeitVorher.LaengeXY();
	MB_PRUEFE(VerlustTop > VerlustSlice);

	// Seitendrall um die Hochachse (> 0) zieht nach links (X < 0 beim Flug nach +Y)
	FBallZustand Seite = MBPhysikTest_Grundschlag(90.0, 0.22, 0.0);
	Seite.Drall = FVek3(0.0, 0.0, UpmZuRadS(2000.0));
	const FMBPhysikTestFlug Kurve = MBPhysikTest_Fliege(MBTest::Simulation("Hart"), Seite);
	MB_PRUEFE(Kurve.Aufsprung.Position.X < -0.3);
}

MB_TEST(BallphysikStoppSpringtZurueck, "Ballphysik.StoppSpringtZurueck")
{
	// Langsamer Stopp mit starkem Rueckwaertsdrall: der Ball verliert beim Aufsprung fast das ganze
	// Vorwaertstempo; mit noch mehr Drall springt er zurueck
	FBallZustand Start;
	Start.Position = FVek3(0.0, -6.0, 0.8);
	Start.Geschwindigkeit = FVek3(0.0, 1.0, 0.60).Normalisiert() * KmhZuMs(32.0);
	Start.Drall = Kreuzprodukt(FVek3(0.0, 0.0, 1.0), FVek3(0.0, 1.0, 0.0)) * UpmZuRadS(-3000.0);
	const FMBPhysikTestFlug Flug = MBPhysikTest_Fliege(MBTest::Simulation("Sand"), Start);
	MB_PRUEFE(Flug.bAufsprung);
	MB_PRUEFE(!Flug.bNetzBeruehrt);
	const double Vorher = Flug.Aufsprung.GeschwindigkeitVorher.Y;
	const double Nachher = Flug.Aufsprung.GeschwindigkeitNachher.Y;
	MB_INFO("Stopp: Vorwaertstempo %.2f -> %.2f m/s, Aufsprung Y=%.2f", Vorher, Nachher, Flug.Aufsprung.Position.Y);
	MB_PRUEFE(Flug.Aufsprung.Position.Y > 0.0 && Flug.Aufsprung.Position.Y < 3.0);
	MB_PRUEFE(Nachher < 0.2 * Vorher);

	FBallZustand Stark = Start;
	Stark.Drall = Kreuzprodukt(FVek3(0.0, 0.0, 1.0), FVek3(0.0, 1.0, 0.0)) * UpmZuRadS(-4500.0);
	const FMBPhysikTestFlug Zurueck = MBPhysikTest_Fliege(MBTest::Simulation("Sand"), Stark);
	MB_INFO("Stopp mit 4500 U/min: Vorwaertstempo %.2f -> %.2f m/s, Aufsprung Y=%.2f", Zurueck.Aufsprung.GeschwindigkeitVorher.Y, Zurueck.Aufsprung.GeschwindigkeitNachher.Y, Zurueck.Aufsprung.Position.Y);
	MB_PRUEFE(Zurueck.bAufsprung && Zurueck.Aufsprung.Position.Y > 0.0);
	MB_PRUEFE(Zurueck.Aufsprung.GeschwindigkeitNachher.Y < 0.0);
}

MB_TEST(BallphysikNetz, "Ballphysik.Netz")
{
	// Zu flach: Ball geht ins Netzgeflecht und faellt auf der eigenen Seite zurueck
	{
		FBallZustand Start;
		Start.Position = FVek3(0.0, -8.0, 0.6);
		Start.Geschwindigkeit = FVek3(0.0, KmhZuMs(150.0), 0.0);
		const FMBPhysikTestFlug Flug = MBPhysikTest_Fliege(MBTest::Simulation("Hart"), Start, 3.0);
		MB_PRUEFE(Flug.bGeflecht);
		MB_PRUEFE(Flug.Ende.Position.Y < 0.0);
		MB_PRUEFE(Flug.NetzHoehe < 0.0);
	}
	// Sehr schnell (250 km/h) ins Netz: kein Durchtunneln
	{
		FBallZustand Start;
		Start.Position = FVek3(1.0, -3.0, 0.5);
		Start.Geschwindigkeit = FVek3(0.0, KmhZuMs(250.0), 0.0);
		const FMBPhysikTestFlug Flug = MBPhysikTest_Fliege(MBTest::Simulation("Hart"), Start, 3.0);
		MB_PRUEFE(Flug.bGeflecht);
		MB_PRUEFE(Flug.Ende.Position.Y < 0.0);
	}
	// Netzband gestreift: Ball beruehrt das Band (Netzroller)
	{
		FBallZustand Start;
		Start.Position = FVek3(0.0, -5.0, 0.92 + 0.03);
		Start.Geschwindigkeit = FVek3(0.0, 15.0, 0.6);
		FBallSimulation Simulation = MBTest::Simulation("Hart");
		// Flugbahn so waehlen, dass der Ball genau auf Hoehe der Netzoberkante ankommt
		const FMBPhysikTestFlug Probe = MBPhysikTest_Fliege(Simulation, Start, 3.0);
		MB_PRUEFE(Probe.bNetzBeruehrt);
		MB_PRUEFE(!Probe.bGeflecht || Probe.Ende.Position.Y < 0.0);
	}
	// Ball ueber das Netz neben den Pfosten (|X| > 6,4 m) ist erlaubt
	{
		FBallZustand Start;
		Start.Position = FVek3(7.0, -3.0, 0.3);
		Start.Geschwindigkeit = FVek3(0.0, 15.0, 0.0);
		const FMBPhysikTestFlug Flug = MBPhysikTest_Fliege(MBTest::Simulation("Hart"), Start, 3.0);
		MB_PRUEFE(!Flug.bNetzBeruehrt);
		MB_PRUEFE(Flug.NetzHoehe > 0.0);
	}
}

MB_TEST(BallphysikDeterminismus, "Ballphysik.Determinismus")
{
	// Gleiche Eingaben -> bitgenau gleiches Ergebnis (Voraussetzung fuer Online und Videobeweis)
	const FBallZustand Start = MBPhysikTest_Grundschlag(130.0, 0.15, 2800.0);
	FBallSimulation A = MBTest::Simulation("Sand");
	FBallSimulation B = MBTest::Simulation("Sand");
	A.Starte(Start);
	B.Starte(Start);
	for (int Index = 0; Index < 1200; ++Index)
	{
		A.Schritt();
		B.Schritt();
	}
	const FBallZustand& ZA = A.GetZustand();
	const FBallZustand& ZB = B.GetZustand();
	MB_PRUEFE(std::memcmp(&ZA.Position, &ZB.Position, sizeof(FVek3)) == 0);
	MB_PRUEFE(std::memcmp(&ZA.Geschwindigkeit, &ZB.Geschwindigkeit, sizeof(FVek3)) == 0);
	MB_PRUEFE_GLEICH(A.GetEreignisse().size(), B.GetEreignisse().size());

	// Vorhersage = tatsaechlicher Flug
	FVorhersageOptionen Optionen;
	Optionen.MaxAufspruenge = 1;
	const FFlugbahn Bahn = FVorhersage::Berechne(MBTest::Simulation("Sand"), Start, Optionen);
	const FBallEreignis* Vorhergesagt = Bahn.ErsterAufsprung();
	MB_PRUEFE(Vorhergesagt != nullptr);
	const FBallEreignis* Echt = nullptr;
	for (const FBallEreignis& E : A.GetEreignisse())
	{
		if (E.Art == EBallEreignisArt::Aufsprung)
		{
			Echt = &E;
			break;
		}
	}
	MB_PRUEFE(Echt != nullptr);
	if (Vorhergesagt != nullptr && Echt != nullptr)
	{
		MB_PRUEFE(Vorhergesagt->Position == Echt->Position);
		MB_PRUEFE(Vorhergesagt->Zeit == Echt->Zeit);
	}

	// Fingerabdruck der Flugbahn (fuer den Vergleich verschiedener Compiler/PCs in den Logs)
	std::uint64_t Fingerabdruck = 1469598103934665603ULL;
	for (const FBahnPunkt& Punkt : Bahn.Punkte)
	{
		const double Werte[3] = {Punkt.Position.X, Punkt.Position.Y, Punkt.Position.Z};
		unsigned char Bytes[sizeof(Werte)];
		std::memcpy(Bytes, Werte, sizeof(Werte));
		for (unsigned char Byte : Bytes)
		{
			Fingerabdruck = (Fingerabdruck ^ Byte) * 1099511628211ULL;
		}
	}
	MB_INFO("Fingerabdruck Flugbahn: %016llx (%zu Punkte)", static_cast<unsigned long long>(Fingerabdruck), Bahn.Punkte.size());
}

MB_TEST(BallphysikRollenUndRuhe, "Ballphysik.RollenUndRuhe")
{
	FBallZustand Start;
	Start.Position = FVek3(0.0, 2.0, 0.0335);
	Start.Geschwindigkeit = FVek3(0.0, 3.0, 0.0);
	FBallSimulation Simulation = MBTest::Simulation("Hart");
	Simulation.Starte(Start);
	MB_PRUEFE(Simulation.GetZustand().bRollt);
	Simulation.SimuliereBis(20.0);
	MB_PRUEFE(Simulation.GetZustand().bRuht);
	// Weg bei konstanter Verzoegerung a: v^2 / (2 a)
	const double Erwartet = 2.0 + 9.0 / (2.0 * MBTest::LadeBelag("Hart").RollReibung);
	MB_PRUEFE_NAH(Simulation.GetZustand().Position.Y, Erwartet, 0.05);
	MB_PRUEFE_NAH(Simulation.GetZustand().Position.Z, Simulation.GetBall().Radius, 1e-12);
}

MB_TEST(BallphysikVorhersageTreffpunkt, "Ballphysik.VorhersageTreffpunkt")
{
	const FBallZustand Start = MBPhysikTest_Grundschlag(110.0, 0.15, 2000.0);
	FVorhersageOptionen Optionen;
	const FFlugbahn Bahn = FVorhersage::Berechne(MBTest::Simulation("Hart"), Start, Optionen);
	MB_PRUEFE(Bahn.UeberquertNetz(EPlatzseite::Nah));
	MB_PRUEFE(Bahn.HoeheUeberNetz() > Platzmass::NetzhoeheMitte);

	// Treffpunkt des Gegners an seiner Grundlinie (nach dem Aufsprung)
	FTreffSuche Suche;
	Suche.Seite = EPlatzseite::Fern;
	const FTreffpunkt Treffer = FindeTreffpunktInEbene(Bahn, 11.5, Suche);
	MB_PRUEFE(Treffer.bGueltig);
	MB_PRUEFE(!Treffer.bVolley);
	MB_PRUEFE(Treffer.Position.Z >= Suche.MinHoehe && Treffer.Position.Z <= Suche.MaxHoehe);
	MB_INFO("Treffpunkt an Y=11,5: Hoehe %.2f m nach %.3f s", Treffer.Position.Z, Treffer.Zeit);

	// Erreichbarkeit: Spieler steht mittig hinter der Grundlinie
	const FTreffpunkt Erreichbar = FindeErreichbarenTreffpunkt(Bahn, FVek3(0.0, 12.5, 0.0), 6.0, 0.25, 1.0, Suche);
	MB_PRUEFE(Erreichbar.bGueltig);
	// Spieler weit weg (anderer Platz) erreicht den Ball nicht
	const FTreffpunkt Unerreichbar = FindeErreichbarenTreffpunkt(Bahn, FVek3(15.0, 12.5, 0.0), 6.0, 0.25, 1.0, Suche);
	MB_PRUEFE(!Unerreichbar.bGueltig);

	FBahnPunkt Mitte;
	MB_PRUEFE(Bahn.ZustandBei(0.3, Mitte));
	MB_PRUEFE(Mitte.Position.Y > Start.Position.Y);
}
