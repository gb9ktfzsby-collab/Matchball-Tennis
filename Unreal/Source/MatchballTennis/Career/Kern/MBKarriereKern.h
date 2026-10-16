// Matchball Tennis - Kern: Karriere-Logik ohne Grafik (README 9.2, 14.4, 14.5, 14.6)
//
// - Spielerwerte und Gesamtwert (gewichteter Durchschnitt, 0-99)
// - Weltrangliste: rollierend ueber 52 Wochen, die besten N Ergebnisse zaehlen (ATP 2026: 18, WTA: 18),
//   die Finals zaehlen zusaetzlich
// - Auslosung: Setzliste nach Rangliste, Gesetzte verteilt wie auf der Tour, Freilose fuer die besten Gesetzten
// - Match-Simulation fuer alle nicht selbst gespielten Matches (Ergebnis aus Werten, Form, Belag, Zufall)

#pragma once

#include "Core/Kern/MBZufall.h"
#include "Match/Kern/MBZaehlweise.h"

#include <string>
#include <vector>

namespace MB
{
	// --- Spielerwerte (README 9.2) ------------------------------------------------------------

	struct FSpielerwerte
	{
		double Aufschlag = 50.0;
		double Vorhand = 50.0;
		double Rueckhand = 50.0;
		double Volley = 50.0;
		double Schnelligkeit = 50.0;
		double Ausdauer = 50.0;
		double Nerven = 50.0;
		double Spin = 50.0;
		double Return = 50.0;
	};

	struct FWertGewichte
	{
		double Aufschlag = 0.15;
		double Vorhand = 0.15;
		double Rueckhand = 0.15;
		double Volley = 0.08;
		double Schnelligkeit = 0.12;
		double Ausdauer = 0.07;
		double Nerven = 0.08;
		double Spin = 0.08;
		double Return = 0.12;

		double Summe() const { return Aufschlag + Vorhand + Rueckhand + Volley + Schnelligkeit + Ausdauer + Nerven + Spin + Return; }
	};

	// Gesamtwert 0-99 (gerundet), gewichteter Durchschnitt
	int Gesamtwert(const FSpielerwerte& Werte, const FWertGewichte& Gewichte = FWertGewichte());

	// --- Weltrangliste (README 14.6) ----------------------------------------------------------

	struct FTurnierErgebnis
	{
		int SpielerId = 0;
		int Woche = 0;            // fortlaufende Kalenderwoche seit Karrierebeginn (Woche des Turnierendes)
		int Punkte = 0;
		bool bFinals = false;     // ATP/WTA Finals: zaehlen zusaetzlich zu den besten N
		std::string Turnier;
	};

	struct FRanglistenRegeln
	{
		int BesteErgebnisse = 18;  // ATP ab 2026: 18 (vorher 19), WTA: 18
		int Wochen = 52;
		bool bFinalsZusaetzlich = true;
	};

	struct FRanglistenEintrag
	{
		int SpielerId = 0;
		int Punkte = 0;
		int GewerteteTurniere = 0;
		int BestesErgebnis = 0;
		int Rang = 0;
	};

	// Punkte eines Spielers in Woche AktuelleWoche (Ergebnisse aus den Wochen AktuelleWoche-51 .. AktuelleWoche)
	FRanglistenEintrag RanglistenPunkte(int SpielerId, const std::vector<FTurnierErgebnis>& Ergebnisse, int AktuelleWoche, const FRanglistenRegeln& Regeln);

	// Rangliste aller angegebenen Spieler; Gleichstand: hoeheres Einzelergebnis, dann kleinere Id
	std::vector<FRanglistenEintrag> BerechneRangliste(const std::vector<int>& Spieler, const std::vector<FTurnierErgebnis>& Ergebnisse, int AktuelleWoche, const FRanglistenRegeln& Regeln);

	// --- Auslosung (README 14.4) ----------------------------------------------------------------

	// Kanonische Setzposition: welche Setzlistennummer (1-basiert) auf welcher Position (0-basiert) steht.
	// Nummer 1 oben, Nummer 2 ganz unten, 3/4 in den mittleren Vierteln usw.
	std::vector<int> KanonischeSetzordnung(int Feldgroesse);

	// Auslosung. Teilnehmer nach Rangliste sortiert (bester zuerst). Feldgroesse = 2er-Potenz >= Teilnehmerzahl.
	// Die besten AnzahlGesetzte werden gesetzt (Gruppen 1, 2, 3-4, 5-8, 9-16, 17-32 werden innerhalb ihrer
	// Positionen ausgelost); Freilose (Feldgroesse - Teilnehmer) erhalten die besten Gesetzten.
	// Ergebnis: Spieler-Id je Position, -1 = Freilos.
	std::vector<int> Auslosung(const std::vector<int>& TeilnehmerNachRang, int Feldgroesse, int AnzahlGesetzte, FZufall& Zufall);

	// --- Match-Simulation (README 14.5) ---------------------------------------------------------

	struct FSimSpieler
	{
		FSpielerwerte Werte;
		double Form = 0.0;            // -1 .. +1 (Frische, Formkurve)
		double BelagBonus = 0.0;      // Lieblingsbelag: kleiner Bonus (z. B. 0,02)
	};

	struct FSimParameter
	{
		double GrundAufschlagPunkt = 0.635;   // Tour-Durchschnitt gewonnene Aufschlagpunkte
		double AufschlagGegenReturn = 0.0040; // je Wertpunkt Differenz Aufschlag(A) - Return(B)
		double GesamtDifferenz = 0.0035;      // je Punkt Gesamtwert-Differenz
		double FormEinfluss = 0.015;
		double NervenDruck = 0.0006;          // bei Break-/Satz-/Matchbaellen: je Wertpunkt Nerven-Differenz
		double Min = 0.40;
		double Max = 0.85;
	};

	struct FSimErgebnis
	{
		int Sieger = -1;
		std::vector<FSatzErgebnis> Saetze;
		int Punkte[2] = {0, 0};
		int Asse[2] = {0, 0};
		std::string Stand;
	};

	// Wahrscheinlichkeit, dass Aufschlaeger A einen Aufschlagpunkt gegen B gewinnt
	double AufschlagPunktChance(const FSimSpieler& A, const FSimSpieler& B, const FSimParameter& Parameter);

	FSimErgebnis SimuliereMatch(const FSimSpieler& Spieler0, const FSimSpieler& Spieler1, const FZaehlRegeln& Regeln, const FSimParameter& Parameter, FZufall& Zufall);
}
