// Matchball Tennis - Kern: Vektor- und Hilfsmathematik
//
// Regeln fuer alle Dateien in */Kern/ (siehe Dokumente/Architektur.md):
//  - keine Unreal-Header, nur Standard-C++ (damit die Logik ausserhalb von Unreal testbar ist)
//  - deterministisch: in Simulationspfaden nur +, -, *, / und sqrt (IEEE-754 exakt gerundet),
//    keine sin/cos/exp/pow/log (die CRT kann je nach CPU andere Rechenwege nehmen)
//  - Einheiten: Meter, Sekunden, Kilogramm, Radiant pro Sekunde
//
// Koordinatensystem des Kerns (rechtshaendig):
//  - Ursprung: Platzmitte unter dem Netz, auf dem Boden
//  - X: quer zum Platz, positiv nach rechts aus Sicht der nahen Grundlinie
//  - Y: laengs zum Platz, positiv zur fernen Grundlinie (nahe Seite: Y < 0)
//  - Z: nach oben
// Umrechnung nach Unreal (linkshaendig, cm): UE.X = Y * 100, UE.Y = X * 100, UE.Z = Z * 100

#pragma once

#include <cmath>

namespace MB
{
	struct FVek3
	{
		double X = 0.0;
		double Y = 0.0;
		double Z = 0.0;

		constexpr FVek3() = default;
		constexpr FVek3(double InX, double InY, double InZ) : X(InX), Y(InY), Z(InZ) {}

		constexpr FVek3 operator+(const FVek3& B) const { return FVek3(X + B.X, Y + B.Y, Z + B.Z); }
		constexpr FVek3 operator-(const FVek3& B) const { return FVek3(X - B.X, Y - B.Y, Z - B.Z); }
		constexpr FVek3 operator*(double S) const { return FVek3(X * S, Y * S, Z * S); }
		constexpr FVek3 operator-() const { return FVek3(-X, -Y, -Z); }

		FVek3& operator+=(const FVek3& B)
		{
			X += B.X;
			Y += B.Y;
			Z += B.Z;
			return *this;
		}

		FVek3& operator-=(const FVek3& B)
		{
			X -= B.X;
			Y -= B.Y;
			Z -= B.Z;
			return *this;
		}

		FVek3& operator*=(double S)
		{
			X *= S;
			Y *= S;
			Z *= S;
			return *this;
		}

		constexpr bool operator==(const FVek3& B) const { return X == B.X && Y == B.Y && Z == B.Z; }
		constexpr bool operator!=(const FVek3& B) const { return !(*this == B); }

		constexpr double LaengeQuadrat() const { return X * X + Y * Y + Z * Z; }
		double Laenge() const { return std::sqrt(LaengeQuadrat()); }

		// Laenge in der Bodenebene (X/Y)
		double LaengeXY() const { return std::sqrt(X * X + Y * Y); }

		// Normalisiert; bei (fast) Nullvektor wird der Nullvektor zurueckgegeben
		FVek3 Normalisiert() const
		{
			const double L = Laenge();
			if (L < 1e-12)
			{
				return FVek3();
			}
			const double Kehrwert = 1.0 / L;
			return FVek3(X * Kehrwert, Y * Kehrwert, Z * Kehrwert);
		}
	};

	constexpr FVek3 operator*(double S, const FVek3& V) { return V * S; }

	constexpr double Skalarprodukt(const FVek3& A, const FVek3& B)
	{
		return A.X * B.X + A.Y * B.Y + A.Z * B.Z;
	}

	constexpr FVek3 Kreuzprodukt(const FVek3& A, const FVek3& B)
	{
		return FVek3(A.Y * B.Z - A.Z * B.Y, A.Z * B.X - A.X * B.Z, A.X * B.Y - A.Y * B.X);
	}

	// Lineare Interpolation (T = 0 -> A, T = 1 -> B)
	constexpr FVek3 Interpoliere(const FVek3& A, const FVek3& B, double T)
	{
		return FVek3(A.X + (B.X - A.X) * T, A.Y + (B.Y - A.Y) * T, A.Z + (B.Z - A.Z) * T);
	}

	constexpr double Interpoliere(double A, double B, double T) { return A + (B - A) * T; }

	// Eigene Min/Max/Begrenze, damit keine Kollision mit Windows-Makros (min/max) entsteht
	constexpr double MinWert(double A, double B) { return A < B ? A : B; }
	constexpr double MaxWert(double A, double B) { return A > B ? A : B; }
	constexpr int MinWert(int A, int B) { return A < B ? A : B; }
	constexpr int MaxWert(int A, int B) { return A > B ? A : B; }
	constexpr double Begrenze(double Wert, double Unten, double Oben)
	{
		return Wert < Unten ? Unten : (Wert > Oben ? Oben : Wert);
	}
	constexpr int Begrenze(int Wert, int Unten, int Oben)
	{
		return Wert < Unten ? Unten : (Wert > Oben ? Oben : Wert);
	}
	constexpr double Betrag(double Wert) { return Wert < 0.0 ? -Wert : Wert; }
	constexpr double Quadrat(double Wert) { return Wert * Wert; }

	// Umrechnungen (konstante Faktoren, deterministisch)
	constexpr double KmhZuMs(double Kmh) { return Kmh / 3.6; }
	constexpr double MsZuKmh(double Ms) { return Ms * 3.6; }
	constexpr double UpmZuRadS(double Upm) { return Upm * (2.0 * 3.14159265358979323846 / 60.0); }
	constexpr double RadSZuUpm(double RadS) { return RadS * (60.0 / (2.0 * 3.14159265358979323846)); }

	constexpr double KreiszahlPi = 3.14159265358979323846;
}
