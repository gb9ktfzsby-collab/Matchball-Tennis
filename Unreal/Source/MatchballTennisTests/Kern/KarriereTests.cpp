// Matchball Tennis - Tests der Karriere-Logik: Spielerwerte, Rangliste, Auslosung, Simulation (README 9.2, 14)

#include "Kern/MBTestDaten.h"
#include "Kern/MBTestRahmen.h"

#include "Career/Kern/MBKarriereKern.h"

#include <algorithm>
#include <set>

using namespace MB;

namespace
{
	FSpielerwerte MBKarriereTest_Werte(double Wert)
	{
		FSpielerwerte Werte;
		Werte.Aufschlag = Wert;
		Werte.Vorhand = Wert;
		Werte.Rueckhand = Wert;
		Werte.Volley = Wert;
		Werte.Schnelligkeit = Wert;
		Werte.Ausdauer = Wert;
		Werte.Nerven = Wert;
		Werte.Spin = Wert;
		Werte.Return = Wert;
		return Werte;
	}

	// Section (0-basiert) einer Position bei Teilung des Felds in Teile gleicher Groesse
	int MBKarriereTest_Abschnitt(int Position, int Feld, int Teile)
	{
		return Position / (Feld / Teile);
	}
}

MB_TEST(KarriereGesamtwert, "Karriere.Gesamtwert")
{
	MB_PRUEFE_NAH(FWertGewichte().Summe(), 1.0, 1e-12);
	MB_PRUEFE_GLEICH(Gesamtwert(MBKarriereTest_Werte(99.0)), 99);
	MB_PRUEFE_GLEICH(Gesamtwert(MBKarriereTest_Werte(50.0)), 50);
	MB_PRUEFE_GLEICH(Gesamtwert(MBKarriereTest_Werte(120.0)), 99);
	FSpielerwerte Werte = MBKarriereTest_Werte(80.0);
	Werte.Aufschlag = 100.0;   // +20 * 0,15 = +3
	MB_PRUEFE_GLEICH(Gesamtwert(Werte), 83);

	// Kader: Adler und Falk sind die 99er-Spieler (README 9.1)
	const std::vector<MBTest::FZeile> Kader = MBTest::LiesCsv("spieler.csv");
	MB_PRUEFE(Kader.size() >= 20u);
	int Herren = 0;
	int Damen = 0;
	for (const MBTest::FZeile& Zeile : Kader)
	{
		FSpielerwerte W;
		W.Aufschlag = MBTest::Zahl(Zeile, "Aufschlag");
		W.Vorhand = MBTest::Zahl(Zeile, "Vorhand");
		W.Rueckhand = MBTest::Zahl(Zeile, "Rueckhand");
		W.Volley = MBTest::Zahl(Zeile, "Volley");
		W.Schnelligkeit = MBTest::Zahl(Zeile, "Schnelligkeit");
		W.Ausdauer = MBTest::Zahl(Zeile, "Ausdauer");
		W.Nerven = MBTest::Zahl(Zeile, "Nerven");
		W.Spin = MBTest::Zahl(Zeile, "Spin");
		W.Return = MBTest::Zahl(Zeile, "Return");
		const int Gesamt = Gesamtwert(W);
		const std::string Nachname = MBTest::Text(Zeile, "Nachname");
		const int Angegeben = static_cast<int>(MBTest::Zahl(Zeile, "Gesamtwert"));
		if (Gesamt != Angegeben)
		{
			MB_INFO("%s: berechnet %d, in spieler.csv %d", Nachname.c_str(), Gesamt, Angegeben);
		}
		MB_PRUEFE_GLEICH(Gesamt, Angegeben);
		if (Nachname == "Adler" || Nachname == "Falk")
		{
			MB_PRUEFE_GLEICH(Gesamt, 99);
		}
		else
		{
			MB_PRUEFE(Gesamt < 99);
		}
		if (Nachname == "Holloway" || Nachname == "Moreau")
		{
			MB_PRUEFE_GLEICH(MBTest::Text(Zeile, "Rueckhand_Art"), std::string("Einhaendig"));
		}
		const int Tipper = static_cast<int>(MBTest::Zahl(Zeile, "Balltipper"));
		MB_PRUEFE(Tipper >= 1 && Tipper <= 4);
		(MBTest::Text(Zeile, "Tour") == "ATP" ? Herren : Damen) += 1;
	}
	MB_PRUEFE(Herren >= 10);
	MB_PRUEFE(Damen >= 10);
}

MB_TEST(KarriereRangliste, "Karriere.Rangliste52WochenBeste18")
{
	FRanglistenRegeln Regeln;
	std::vector<FTurnierErgebnis> Ergebnisse;
	// Spieler 1: 25 Turniere mit 10..250 Punkten in den Wochen 1..25 -> die besten 18 zaehlen
	for (int Index = 0; Index < 25; ++Index)
	{
		Ergebnisse.push_back(FTurnierErgebnis{1, Index + 1, 10 * (Index + 1), false, "T"});
	}
	int Erwartet = 0;
	for (int Punkte = 250; Punkte > 250 - 18 * 10; Punkte -= 10)
	{
		Erwartet += Punkte;
	}
	FRanglistenEintrag Eintrag = RanglistenPunkte(1, Ergebnisse, 30, Regeln);
	MB_PRUEFE_GLEICH(Eintrag.Punkte, Erwartet);
	MB_PRUEFE_GLEICH(Eintrag.GewerteteTurniere, 18);
	// 52 Wochen rollierend: in Woche 60 fallen die Wochen 1..8 heraus (Fenster 9..60)
	Eintrag = RanglistenPunkte(1, Ergebnisse, 60, Regeln);
	MB_PRUEFE_GLEICH(Eintrag.GewerteteTurniere, 17);
	int Rest = 0;
	for (int Woche = 9; Woche <= 25; ++Woche)
	{
		Rest += 10 * Woche;
	}
	MB_PRUEFE_GLEICH(Eintrag.Punkte, Rest);
	// Finals zaehlen zusaetzlich
	Ergebnisse.push_back(FTurnierErgebnis{1, 26, 1500, true, "Finals"});
	MB_PRUEFE_GLEICH(RanglistenPunkte(1, Ergebnisse, 30, Regeln).Punkte, Erwartet + 1500);
	// Rangliste mit Gleichstand: hoeheres Einzelergebnis vorne
	Ergebnisse.push_back(FTurnierErgebnis{2, 10, 500, false, "A"});
	Ergebnisse.push_back(FTurnierErgebnis{3, 10, 250, false, "B"});
	Ergebnisse.push_back(FTurnierErgebnis{3, 11, 250, false, "C"});
	const std::vector<FRanglistenEintrag> Liste = BerechneRangliste({3, 2, 1, 4}, Ergebnisse, 30, Regeln);
	MB_PRUEFE_GLEICH(Liste[0].SpielerId, 1);
	MB_PRUEFE_GLEICH(Liste[1].SpielerId, 2);
	MB_PRUEFE_GLEICH(Liste[2].SpielerId, 3);
	MB_PRUEFE_GLEICH(Liste[3].SpielerId, 4);
	MB_PRUEFE_GLEICH(Liste[3].Punkte, 0);
	MB_PRUEFE_GLEICH(Liste[2].Rang, 3);
}

MB_TEST(KarriereSetzordnung, "Karriere.SetzordnungUndAuslosung")
{
	const std::vector<int> Kanon = KanonischeSetzordnung(32);
	MB_PRUEFE_GLEICH(Kanon.size(), static_cast<std::size_t>(32));
	MB_PRUEFE_GLEICH(Kanon.front(), 1);
	MB_PRUEFE_GLEICH(Kanon.back(), 2);
	// Jede Nummer genau einmal
	std::set<int> Nummern(Kanon.begin(), Kanon.end());
	MB_PRUEFE_GLEICH(Nummern.size(), static_cast<std::size_t>(32));
	// Setzlisten 1..2^k liegen in verschiedenen Abschnitten (Haelften, Viertel, Achtel)
	for (int Teile = 2; Teile <= 16; Teile *= 2)
	{
		std::set<int> Abschnitte;
		for (int Position = 0; Position < 32; ++Position)
		{
			if (Kanon[static_cast<std::size_t>(Position)] <= Teile)
			{
				Abschnitte.insert(MBKarriereTest_Abschnitt(Position, 32, Teile));
			}
		}
		MB_PRUEFE_GLEICH(Abschnitte.size(), static_cast<std::size_t>(Teile));
	}
	// In Runde 1 trifft Nummer k auf Nummer 33-k
	for (int Position = 0; Position < 32; Position += 2)
	{
		MB_PRUEFE_GLEICH(Kanon[static_cast<std::size_t>(Position)] + Kanon[static_cast<std::size_t>(Position) + 1u], 33);
	}

	// Auslosung: 28 Spieler im 32er-Feld (ATP 250), 8 Gesetzte, 4 Freilose fuer die Nummern 1-4
	std::vector<int> Teilnehmer;
	for (int Id = 100; Id < 128; ++Id)
	{
		Teilnehmer.push_back(Id);
	}
	FZufall Zufall(31u);
	const std::vector<int> Feld = Auslosung(Teilnehmer, 32, 8, Zufall);
	MB_PRUEFE_GLEICH(Feld.size(), static_cast<std::size_t>(32));
	MB_PRUEFE_GLEICH(Feld.front(), 100);   // Nummer 1 oben
	MB_PRUEFE_GLEICH(Feld.back(), 101);    // Nummer 2 unten
	MB_PRUEFE_GLEICH(std::count(Feld.begin(), Feld.end(), -1), 4);
	std::set<int> Gesehen;
	for (int Position = 0; Position < 32; ++Position)
	{
		const int Id = Feld[static_cast<std::size_t>(Position)];
		if (Id < 0)
		{
			// Freilos nur neben einem der vier besten Gesetzten
			const int Partner = Feld[static_cast<std::size_t>(Position ^ 1)];
			MB_PRUEFE(Partner >= 100 && Partner <= 103);
			continue;
		}
		MB_PRUEFE(Gesehen.insert(Id).second);
	}
	MB_PRUEFE_GLEICH(Gesehen.size(), static_cast<std::size_t>(28));
	// Gesetzte 1-4 in verschiedenen Vierteln, 1-8 in verschiedenen Achteln
	std::set<int> Viertel;
	std::set<int> Achtel;
	for (int Position = 0; Position < 32; ++Position)
	{
		const int Id = Feld[static_cast<std::size_t>(Position)];
		if (Id >= 100 && Id <= 103)
		{
			Viertel.insert(MBKarriereTest_Abschnitt(Position, 32, 4));
		}
		if (Id >= 100 && Id <= 107)
		{
			Achtel.insert(MBKarriereTest_Abschnitt(Position, 32, 8));
		}
	}
	MB_PRUEFE_GLEICH(Viertel.size(), static_cast<std::size_t>(4));
	MB_PRUEFE_GLEICH(Achtel.size(), static_cast<std::size_t>(8));
	// Gleicher Startwert -> gleiche Auslosung
	FZufall Zweiter(31u);
	MB_PRUEFE(Auslosung(Teilnehmer, 32, 8, Zweiter) == Feld);

	// 96er-Feld (Masters 1000): 128er-Raster, 32 Gesetzte mit Freilos
	std::vector<int> Gross;
	for (int Id = 0; Id < 96; ++Id)
	{
		Gross.push_back(Id);
	}
	const std::vector<int> Feld128 = Auslosung(Gross, 128, 32, Zufall);
	MB_PRUEFE_GLEICH(std::count(Feld128.begin(), Feld128.end(), -1), 32);
	for (int Position = 0; Position < 128; ++Position)
	{
		if (Feld128[static_cast<std::size_t>(Position)] == -1)
		{
			MB_PRUEFE(Feld128[static_cast<std::size_t>(Position ^ 1)] < 32);
		}
	}
}

MB_TEST(KarriereSimulation, "Karriere.MatchSimulation")
{
	const FSimParameter Parameter;
	FZaehlRegeln Regeln;
	FZufall Zufall(2024u);
	FSimSpieler Gleich;
	Gleich.Werte = MBKarriereTest_Werte(75.0);
	// Gleich starke Spieler: ca. 50 % Siege, Aufschlagpunkte ca. 63 %
	int Siege = 0;
	int PunkteGesamt = 0;
	constexpr int Anzahl = 3000;
	for (int Index = 0; Index < Anzahl; ++Index)
	{
		const FSimErgebnis Ergebnis = SimuliereMatch(Gleich, Gleich, Regeln, Parameter, Zufall);
		MB_PRUEFE(Ergebnis.Sieger == 0 || Ergebnis.Sieger == 1);
		Siege += Ergebnis.Sieger == 0 ? 1 : 0;
		PunkteGesamt += Ergebnis.Punkte[0] + Ergebnis.Punkte[1];
	}
	const double Quote = static_cast<double>(Siege) / Anzahl;
	MB_INFO("Gleich stark: Spieler 0 gewinnt %.3f, mittlere Punktzahl %.1f", Quote, static_cast<double>(PunkteGesamt) / Anzahl);
	MB_PRUEFE_NAH(Quote, 0.5, 0.03);
	// Best of 3: im Mittel ca. 140-170 Punkte
	MB_PRUEFE(PunkteGesamt / Anzahl > 120 && PunkteGesamt / Anzahl < 190);

	// Deutlich staerkerer Spieler gewinnt fast immer; 5 Punkte Unterschied: klarer Favorit
	FSimSpieler Stark;
	Stark.Werte = MBKarriereTest_Werte(95.0);
	FSimSpieler Schwach;
	Schwach.Werte = MBKarriereTest_Werte(70.0);
	FSimSpieler Etwas;
	Etwas.Werte = MBKarriereTest_Werte(80.0);
	int SiegeStark = 0;
	int SiegeEtwas = 0;
	for (int Index = 0; Index < 1000; ++Index)
	{
		SiegeStark += SimuliereMatch(Stark, Schwach, Regeln, Parameter, Zufall).Sieger == 0 ? 1 : 0;
		SiegeEtwas += SimuliereMatch(Etwas, Gleich, Regeln, Parameter, Zufall).Sieger == 0 ? 1 : 0;
	}
	MB_INFO("95 gegen 70: %.3f, 80 gegen 75: %.3f", SiegeStark / 1000.0, SiegeEtwas / 1000.0);
	MB_PRUEFE(SiegeStark > 980);
	MB_PRUEFE(SiegeEtwas > 600 && SiegeEtwas < 950);
	// Chance auf Aufschlagpunkt in plausiblen Grenzen
	MB_PRUEFE_NAH(AufschlagPunktChance(Gleich, Gleich, Parameter), Parameter.GrundAufschlagPunkt, 1e-12);
}
