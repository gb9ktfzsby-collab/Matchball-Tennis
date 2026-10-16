// Matchball Tennis - Kern: deterministischer Zufallsgenerator (PCG32, M. O'Neill)
//
// Gleicher Startwert -> auf jedem PC exakt dieselbe Zahlenfolge (nur Ganzzahl-Arithmetik).
// Wird u. a. fuer Linienrichter-Fehler, Schlagstreuung und KI genutzt; online erzeugen beide PCs
// pro Punkt denselben Startwert (README 15.2).

#pragma once

#include <cstdint>

namespace MB
{
	class FZufall
	{
	public:
		explicit FZufall(std::uint64_t Startwert = 0x853c49e6748fea9bULL, std::uint64_t Folge = 0xda3e39cb94b95bdbULL);

		// Setzt den Generator auf einen neuen Startwert zurueck
		void Starte(std::uint64_t Startwert, std::uint64_t Folge = 0xda3e39cb94b95bdbULL);

		// Naechste 32-Bit-Zufallszahl
		std::uint32_t Naechste();

		// Gleichverteilt in [0, 1)
		double Gleich01();

		// Gleichverteilt in [Unten, Oben)
		double Bereich(double Unten, double Oben);

		// Ganzzahl gleichverteilt in [Unten, Oben] (beide inklusive); ohne Modulo-Verzerrung
		int Ganzzahl(int Unten, int Oben);

		// true mit Wahrscheinlichkeit P (P <= 0: nie, P >= 1: immer)
		bool Chance(double P);

		// Naeherungsweise standardnormalverteilt (Irwin-Hall mit 12 Summanden, Wertebereich [-6, 6]).
		// Bewusst ohne log/cos (Box-Muller), damit das Ergebnis auf jedem PC identisch ist.
		double Normal();

		// Zufaelliges Vorzeichen (-1 oder +1), je 50 %
		double Vorzeichen();

	private:
		std::uint64_t Zustand = 0;
		std::uint64_t Inkrement = 1;
	};

	// Mischt mehrere Werte zu einem Startwert (z. B. Match-Startwert + Punktnummer)
	std::uint64_t MischeStartwert(std::uint64_t A, std::uint64_t B);
}
