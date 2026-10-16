// Matchball Tennis - Kern: Linienrichter (README 7.1)
//
// Bei Baellen nahe der Linie (Abstand d bis 15 cm zur Entscheidungsgrenze, siehe FLinienUrteil::Abstand)
// liegt der Linienrichter im Schnitt in 90 % der Faelle richtig - je knapper, desto eher falsch:
//   p(d) = p_max * (1 - d / d_max)   fuer d <= d_max, sonst 0   (Vorschlag README: p_max = 0,2; d_max = 15 cm)
// Bei gleichverteiltem d ist der Mittelwert p_max / 2 = 10 %. Die Pruefung per Simulation steht in den Tests.
// Option "Elektronische Linienrichter": keine Fehler, keine Challenges.

#pragma once

#include "Core/Kern/MBZufall.h"
#include "Physics/Kern/MBKontaktflaeche.h"

namespace MB
{
	struct FLinienrichterParameter
	{
		double MaxFehler = 0.20;         // Fehlerwahrscheinlichkeit direkt an der Grenze
		double MaxAbstand = 0.15;        // m, darueber immer richtig
		bool bElektronisch = false;      // elektronische Linienrichter (moderne Tour)
	};

	struct FLinienEntscheidung
	{
		bool bGutGegeben = true;         // Ansage des Linienrichters
		bool bFehlentscheidung = false;  // weicht vom tatsaechlichen Ergebnis ab
		double Fehlerwahrscheinlichkeit = 0.0;
		FLinienUrteil Wahrheit;          // tatsaechliches Ergebnis (Hawk-Eye)
	};

	// Fehlerwahrscheinlichkeit fuer einen Abstand d (m)
	double LinienrichterFehlerwahrscheinlichkeit(double Abstand, const FLinienrichterParameter& Parameter);

	// Entscheidung des Linienrichters fuer ein tatsaechliches Urteil. Verbraucht genau eine Zufallszahl,
	// damit beide PCs online im Gleichschritt bleiben.
	FLinienEntscheidung EntscheideLinie(const FLinienUrteil& Wahrheit, const FLinienrichterParameter& Parameter, FZufall& Zufall);
}
