// Matchball Tennis - Kern: Kontaktflaeche des Balls beim Aufsprung

#include "Physics/Kern/MBKontaktflaeche.h"

namespace MB
{
	FVek3 FKontaktflaeche::Mitte() const
	{
		double MinS = 1.0e9;
		double MaxS = -1.0e9;
		for (const FKontaktKreis& Kreis : Kreise)
		{
			const double S = (Kreis.X - Beginn.X) * Richtung.X + (Kreis.Y - Beginn.Y) * Richtung.Y;
			MinS = MinWert(MinS, S - Kreis.Radius);
			MaxS = MaxWert(MaxS, S + Kreis.Radius);
		}
		const double S = 0.5 * (MinS + MaxS);
		return FVek3(Beginn.X + Richtung.X * S, Beginn.Y + Richtung.Y * S, 0.0);
	}

	FKontaktflaeche BerechneKontaktflaeche(const FBallEreignis& Aufsprung, const FBallParameter& Ball)
	{
		FKontaktflaeche Flaeche;
		const double R = Ball.Radius;
		const double Wurzel = std::sqrt(Ball.Masse / Ball.BallSteifigkeit);
		Flaeche.Kontaktzeit = KreiszahlPi * Wurzel;

		const double Normal = Aufsprung.GeschwindigkeitVorher.Z < 0.0 ? -Aufsprung.GeschwindigkeitVorher.Z : 0.0;
		// Stauchung begrenzt auf 45 % des Radius (staerker verformt sich ein Tennisball praktisch nicht)
		Flaeche.MaxStauchung = MinWert(Normal * Wurzel, 0.45 * R);

		const FVek3 HorizontalVorher(Aufsprung.GeschwindigkeitVorher.X, Aufsprung.GeschwindigkeitVorher.Y, 0.0);
		const FVek3 HorizontalNachher(Aufsprung.GeschwindigkeitNachher.X, Aufsprung.GeschwindigkeitNachher.Y, 0.0);
		const FVek3 Mittel = (HorizontalVorher + HorizontalNachher) * 0.5;
		const double MittelTempo = Mittel.LaengeXY();
		Flaeche.Richtung = MittelTempo > 1e-9 ? Mittel.Normalisiert() : FVek3(0.0, 1.0, 0.0);
		Flaeche.Beginn = FVek3(Aufsprung.Position.X, Aufsprung.Position.Y, 0.0);

		const double Wanderweg = MittelTempo * Flaeche.Kontaktzeit;
		double MinS = 1.0e9;
		double MaxS = -1.0e9;
		double MaxRadius = 0.0;
		for (int Index = 0; Index < FKontaktflaeche::AnzahlKreise; ++Index)
		{
			const double U = static_cast<double>(Index) / static_cast<double>(FKontaktflaeche::AnzahlKreise - 1);
			const double Stauchung = Flaeche.MaxStauchung * 4.0 * U * (1.0 - U);
			const double RadiusQuadrat = 2.0 * R * Stauchung - Stauchung * Stauchung;
			const double Radius = RadiusQuadrat > 0.0 ? std::sqrt(RadiusQuadrat) : 0.0;
			const double S = Wanderweg * U;
			FKontaktKreis& Kreis = Flaeche.Kreise[Index];
			Kreis.X = Flaeche.Beginn.X + Flaeche.Richtung.X * S;
			Kreis.Y = Flaeche.Beginn.Y + Flaeche.Richtung.Y * S;
			Kreis.Radius = Radius;
			MinS = MinWert(MinS, S - Radius);
			MaxS = MaxWert(MaxS, S + Radius);
			MaxRadius = MaxWert(MaxRadius, Radius);
		}
		Flaeche.Laenge = MaxS - MinS;
		Flaeche.Breite = 2.0 * MaxRadius;
		return Flaeche;
	}

	FLinienUrteil BeurteileKontaktflaeche(const FKontaktflaeche& Flaeche, const FFeld& Feld)
	{
		// Luecke = min ueber alle Kreise (Vorzeichenabstand des Mittelpunkts - Radius); <= 0 bedeutet Beruehrung
		double Luecke = 1.0e9;
		ELinie Linie = ELinie::Keine;
		for (const FKontaktKreis& Kreis : Flaeche.Kreise)
		{
			ELinie KreisLinie = ELinie::Keine;
			const double Wert = Feld.VorzeichenAbstand(Kreis.X, Kreis.Y, &KreisLinie) - Kreis.Radius;
			if (Wert < Luecke)
			{
				Luecke = Wert;
				Linie = KreisLinie;
			}
		}

		// Abstand = |Luecke|: bei "Aus" die Luecke zur Linie, bei "In" die Strecke, um die die Flaeche
		// nach aussen wandern muesste, bis sie die Linie nicht mehr beruehrt.
		FLinienUrteil Urteil;
		Urteil.bIn = Luecke <= 0.0;
		Urteil.Abstand = Betrag(Luecke);
		Urteil.Linie = Linie;
		return Urteil;
	}
}
