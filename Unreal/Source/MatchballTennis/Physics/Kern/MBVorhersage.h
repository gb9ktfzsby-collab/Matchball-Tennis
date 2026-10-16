// Matchball Tennis - Kern: Flugbahn-Vorhersage (README 6.2)
//
// Fuer KI, Animation und Timing-Meter wird die Flugbahn mit derselben Simulation im Schnelldurchlauf
// vorausberechnet: Aufsprungpunkte, Hoehe an moeglichen Treffpunkten, Zeit bis zum Treffpunkt.

#pragma once

#include "Physics/Kern/MBBallPhysik.h"

#include <vector>

namespace MB
{
	struct FBahnPunkt
	{
		double Zeit = 0.0;
		FVek3 Position;
		FVek3 Geschwindigkeit;
		int Aufspruenge = 0;    // Anzahl Bodenkontakte bis zu diesem Punkt
	};

	struct FFlugbahn
	{
		std::vector<FBahnPunkt> Punkte;      // jeder Simulationsschritt (240 Hz)
		std::vector<FBallEreignis> Ereignisse;

		// Erster Bodenkontakt (nullptr, wenn keiner)
		const FBallEreignis* ErsterAufsprung() const;
		const FBallEreignis* ZweiterAufsprung() const;

		// Erstes Netz-Ereignis (Band oder Geflecht), nullptr wenn keins
		const FBallEreignis* ErsteNetzberuehrung() const;

		// Hoehe des Ballmittelpunkts beim (ersten) Ueberqueren der Netzebene; < 0, wenn nie ueberquert
		double HoeheUeberNetz() const;

		// Endet der Ball (bis zum ersten Aufsprung) auf der Gegenseite?
		bool UeberquertNetz(EPlatzseite Startseite) const;

		// Ballzustand zur Zeit T (linear zwischen den Schritten); false, wenn T ausserhalb liegt
		bool ZustandBei(double T, FBahnPunkt& OutPunkt) const;
	};

	struct FVorhersageOptionen
	{
		double MaxZeit = 4.0;          // s
		int MaxAufspruenge = 2;        // Simulation endet nach dem n-ten Aufsprung (+ kurzer Nachlauf)
		double NachlaufNachEnde = 0.0; // s, die nach dem letzten Aufsprung noch aufgezeichnet werden
		bool bPunkteSpeichern = true;
	};

	class FVorhersage
	{
	public:
		// Simuliert ab einem Zustand. Die Simulation selbst wird kopiert, der Aufrufer bleibt unveraendert.
		static FFlugbahn Berechne(const FBallSimulation& Vorlage, const FBallZustand& Start, const FVorhersageOptionen& Optionen);
	};

	// Moeglicher Treffpunkt fuer einen Spieler
	struct FTreffpunkt
	{
		double Zeit = 0.0;         // s ab Beginn der Flugbahn
		FVek3 Position;            // Ballmittelpunkt
		FVek3 Geschwindigkeit;
		bool bVolley = false;      // vor dem Aufsprung
		bool bGueltig = false;
	};

	struct FTreffSuche
	{
		EPlatzseite Seite = EPlatzseite::Nah;   // Seite des schlagenden Spielers
		double MinHoehe = 0.25;                 // Ballmittelpunkt ueber dem Boden (m)
		double MaxHoehe = 1.60;                 // Grundschlag-Treffzone (Volley/Schmetterball eigene Werte)
		bool bVolleyErlaubt = false;
		double MinZeit = 0.0;                   // fruehester Zeitpunkt
	};

	// Sucht entlang der Flugbahn alle Zeitpunkte, an denen der Ball die Ebene Y = TreffY quert,
	// und liefert den ersten passenden Treffpunkt (Hoehe im Fenster, Ball auf der eigenen Seite,
	// vor dem zweiten Aufsprung; Volley nur wenn erlaubt).
	FTreffpunkt FindeTreffpunktInEbene(const FFlugbahn& Bahn, double TreffY, const FTreffSuche& Suche);

	// Sucht den Punkt der Flugbahn (auf der eigenen Seite, vor dem 2. Aufsprung, Hoehe im Fenster),
	// den ein Spieler von Spielerposition aus am fruehesten erreichen kann.
	// Laufzeit-Modell: Reaktionszeit + Weg / Laufgeschwindigkeit, abzueglich Reichweite.
	FTreffpunkt FindeErreichbarenTreffpunkt(const FFlugbahn& Bahn, const FVek3& Spielerposition, double Laufgeschwindigkeit, double Reaktionszeit, double Reichweite, const FTreffSuche& Suche);
}
