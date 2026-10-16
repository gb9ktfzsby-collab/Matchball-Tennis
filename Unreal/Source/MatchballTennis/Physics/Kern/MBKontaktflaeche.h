// Matchball Tennis - Kern: Kontaktflaeche des Balls beim Aufsprung (README 6.2 und 7.3)
//
// Wie bei Hawk-Eye zaehlt die Kontaktflaeche des zusammengedrueckten Balls: ein laenglicher Abdruck
// in Flugrichtung; schnelle Baelle rutschen laenger. Herleitung (Dokumente/Werte.md, Abschnitt Kontaktflaeche):
//
//  - Der Ball verhaelt sich beim Bodenkontakt wie eine Feder (Steifigkeit k): Kontaktzeit
//    t_c = pi * sqrt(m / k), maximale Stauchung delta_max = v_n * sqrt(m / k).
//  - Die Stauchung folgt waehrend des Kontakts einer Halbwelle; statt sin wird die Parabel
//    delta(t) = delta_max * 4 u (1 - u), u = t / t_c, benutzt (deterministisch, Abweichung < 6 %).
//  - Der Radius der Beruehrflaeche ist der Kugelabschnitt: a = sqrt(2 r delta - delta^2).
//  - Der Ballmittelpunkt wandert waehrend des Kontakts mit der mittleren Horizontalgeschwindigkeit
//    weiter. Die Kontaktflaeche ist die Vereinigung aller Beruehrkreise (33 Stuetzstellen).
//  => Breite = 2 a_max (haengt von v_n ab), Laenge = Breite + Wanderweg (haengt vom Tempo ab).

#pragma once

#include "Core/Kern/MBMathe.h"
#include "Physics/Kern/MBBallPhysik.h"
#include "Physics/Kern/MBPlatz.h"

namespace MB
{
	struct FKontaktKreis
	{
		double X = 0.0;
		double Y = 0.0;
		double Radius = 0.0;
	};

	struct FKontaktflaeche
	{
		static constexpr int AnzahlKreise = 33;

		FKontaktKreis Kreise[AnzahlKreise];
		FVek3 Beginn;                 // Bodenpunkt unter dem Ball beim ersten Kontakt
		FVek3 Richtung;               // horizontale Flugrichtung (Einheitsvektor)
		double Laenge = 0.0;          // m in Flugrichtung
		double Breite = 0.0;          // m quer
		double Kontaktzeit = 0.0;     // s
		double MaxStauchung = 0.0;    // m

		// Mittelpunkt der Flaeche (fuer Anzeige/Kamera)
		FVek3 Mitte() const;
	};

	// Berechnet die Kontaktflaeche aus einem Aufsprung-Ereignis der Simulation
	FKontaktflaeche BerechneKontaktflaeche(const FBallEreignis& Aufsprung, const FBallParameter& Ball);

	// Ergebnis der elektronischen Entscheidung (Hawk-Eye)
	struct FLinienUrteil
	{
		bool bIn = true;
		// Abstand zur Entscheidungsgrenze (Aussenkante der Linie), immer >= 0:
		//  - Aus: Luecke zwischen Kontaktflaeche und Linie
		//  - In:  wie weit die Kontaktflaeche ueber die Aussenkante ins Feld hineinreicht
		// Grundlage fuer die Hawk-Eye-Anzeige (mm) und die Linienrichter-Fehlerquote.
		double Abstand = 0.0;
		ELinie Linie = ELinie::Keine; // massgebliche Linie
	};

	// "In", sobald die Kontaktflaeche das Feld (inklusive Linien) beruehrt.
	FLinienUrteil BeurteileKontaktflaeche(const FKontaktflaeche& Flaeche, const FFeld& Feld);
}
