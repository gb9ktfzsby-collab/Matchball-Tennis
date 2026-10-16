// Matchball Tennis - Kern: deterministischer Zufallsgenerator (PCG32)

#include "Core/Kern/MBZufall.h"

namespace MB
{
	FZufall::FZufall(std::uint64_t Startwert, std::uint64_t Folge)
	{
		Starte(Startwert, Folge);
	}

	void FZufall::Starte(std::uint64_t Startwert, std::uint64_t Folge)
	{
		Zustand = 0u;
		Inkrement = (Folge << 1u) | 1u;
		Naechste();
		Zustand += Startwert;
		Naechste();
	}

	std::uint32_t FZufall::Naechste()
	{
		const std::uint64_t Alt = Zustand;
		Zustand = Alt * 6364136223846793005ULL + Inkrement;
		const std::uint32_t Verschoben = static_cast<std::uint32_t>(((Alt >> 18u) ^ Alt) >> 27u);
		const std::uint32_t Drehung = static_cast<std::uint32_t>(Alt >> 59u);
		return (Verschoben >> Drehung) | (Verschoben << ((32u - Drehung) & 31u));
	}

	double FZufall::Gleich01()
	{
		return static_cast<double>(Naechste()) * (1.0 / 4294967296.0);
	}

	double FZufall::Bereich(double Unten, double Oben)
	{
		return Unten + (Oben - Unten) * Gleich01();
	}

	int FZufall::Ganzzahl(int Unten, int Oben)
	{
		if (Oben <= Unten)
		{
			return Unten;
		}
		const std::uint32_t Spanne = static_cast<std::uint32_t>(static_cast<std::int64_t>(Oben) - static_cast<std::int64_t>(Unten)) + 1u;
		if (Spanne == 0u)
		{
			// Gesamter 32-Bit-Bereich
			return static_cast<int>(static_cast<std::int64_t>(Unten) + static_cast<std::int64_t>(Naechste()));
		}
		// Ablehnungsverfahren gegen Modulo-Verzerrung
		const std::uint32_t Grenze = (0u - Spanne) % Spanne;
		for (;;)
		{
			const std::uint32_t Wert = Naechste();
			if (Wert >= Grenze)
			{
				return static_cast<int>(static_cast<std::int64_t>(Unten) + static_cast<std::int64_t>(Wert % Spanne));
			}
		}
	}

	bool FZufall::Chance(double P)
	{
		if (P <= 0.0)
		{
			// Trotzdem eine Zahl verbrauchen, damit die Folge unabhaengig vom Wert von P bleibt
			Naechste();
			return false;
		}
		if (P >= 1.0)
		{
			Naechste();
			return true;
		}
		return Gleich01() < P;
	}

	double FZufall::Normal()
	{
		double Summe = 0.0;
		for (int Index = 0; Index < 12; ++Index)
		{
			Summe += Gleich01();
		}
		return Summe - 6.0;
	}

	double FZufall::Vorzeichen()
	{
		return (Naechste() & 1u) != 0u ? 1.0 : -1.0;
	}

	std::uint64_t MischeStartwert(std::uint64_t A, std::uint64_t B)
	{
		// SplitMix64-Schritt ueber A ^ (B * goldener Schnitt)
		std::uint64_t Z = A ^ (B * 0x9E3779B97F4A7C15ULL);
		Z += 0x9E3779B97F4A7C15ULL;
		Z = (Z ^ (Z >> 30u)) * 0xBF58476D1CE4E5B9ULL;
		Z = (Z ^ (Z >> 27u)) * 0x94D049BB133111EBULL;
		return Z ^ (Z >> 31u);
	}
}
