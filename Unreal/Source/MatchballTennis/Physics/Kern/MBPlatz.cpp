// Matchball Tennis - Kern: Platzgeometrie

#include "Physics/Kern/MBPlatz.h"

namespace MB
{
	double FFeld::VorzeichenAbstand(double X, double Y, ELinie* NaechsteLinie) const
	{
		if (!Enthaelt(X, Y))
		{
			// Ausserhalb: euklidischer Abstand zum Rechteck
			const double DX = X < MinX ? MinX - X : (X > MaxX ? X - MaxX : 0.0);
			const double DY = Y < MinY ? MinY - Y : (Y > MaxY ? Y - MaxY : 0.0);
			if (NaechsteLinie != nullptr)
			{
				// Massgeblich ist die Linie, ueber die der Punkt am weitesten hinaus liegt
				if (DX >= DY)
				{
					*NaechsteLinie = X < MinX ? LinieMinX : LinieMaxX;
				}
				else
				{
					*NaechsteLinie = Y < MinY ? LinieMinY : LinieMaxY;
				}
			}
			return std::sqrt(DX * DX + DY * DY);
		}

		// Innerhalb: Abstand zur naechsten echten Linie (Netzseite ausgenommen)
		double Bester = 1.0e9;
		ELinie BesteLinie = ELinie::Keine;
		const double Abstaende[4] = {X - MinX, MaxX - X, Y - MinY, MaxY - Y};
		const ELinie KantenLinien[4] = {LinieMinX, LinieMaxX, LinieMinY, LinieMaxY};
		for (int Index = 0; Index < 4; ++Index)
		{
			if (KantenLinien[Index] != ELinie::Keine && Abstaende[Index] < Bester)
			{
				Bester = Abstaende[Index];
				BesteLinie = KantenLinien[Index];
			}
		}
		if (NaechsteLinie != nullptr)
		{
			*NaechsteLinie = BesteLinie;
		}
		return -Bester;
	}

	FFeld FPlatz::Einzelfeld(EPlatzseite Seite) const
	{
		FFeld Feld;
		Feld.MinX = -Platzmass::HalbeBreiteEinzel;
		Feld.MaxX = Platzmass::HalbeBreiteEinzel;
		Feld.LinieMinX = ELinie::SeitenlinieLinks;
		Feld.LinieMaxX = ELinie::SeitenlinieRechts;
		if (Seite == EPlatzseite::Fern)
		{
			Feld.MinY = 0.0;
			Feld.MaxY = Platzmass::HalbeLaenge;
			Feld.LinieMinY = ELinie::Keine;
			Feld.LinieMaxY = ELinie::Grundlinie;
		}
		else
		{
			Feld.MinY = -Platzmass::HalbeLaenge;
			Feld.MaxY = 0.0;
			Feld.LinieMinY = ELinie::Grundlinie;
			Feld.LinieMaxY = ELinie::Keine;
		}
		return Feld;
	}

	FFeld FPlatz::Aufschlagfeld(EPlatzseite AufschlagSeite, EAufschlagHaelfte Haelfte) const
	{
		// Die Mittellinie ist mittig auf X = 0 und gehoert zu beiden Aufschlagfeldern.
		const double HalbeMittellinie = Linien.Mittellinie * 0.5;
		const EPlatzseite Zielseite = Gegenseite(AufschlagSeite);

		// Aufschlaeger nah, Einstand: steht bei X > 0, Ziel diagonal bei X < 0.
		// Aufschlaeger fern, Einstand: steht bei X < 0 (sein "rechts"), Ziel bei X > 0.
		const bool bZielLinks = (AufschlagSeite == EPlatzseite::Nah) == (Haelfte == EAufschlagHaelfte::Einstand);

		FFeld Feld;
		if (bZielLinks)
		{
			Feld.MinX = -Platzmass::HalbeBreiteEinzel;
			Feld.MaxX = HalbeMittellinie;
			Feld.LinieMinX = ELinie::SeitenlinieLinks;
			Feld.LinieMaxX = ELinie::Mittellinie;
		}
		else
		{
			Feld.MinX = -HalbeMittellinie;
			Feld.MaxX = Platzmass::HalbeBreiteEinzel;
			Feld.LinieMinX = ELinie::Mittellinie;
			Feld.LinieMaxX = ELinie::SeitenlinieRechts;
		}

		if (Zielseite == EPlatzseite::Fern)
		{
			Feld.MinY = 0.0;
			Feld.MaxY = Platzmass::AufschlaglinieVomNetz;
			Feld.LinieMinY = ELinie::Keine;
			Feld.LinieMaxY = ELinie::Aufschlaglinie;
		}
		else
		{
			Feld.MinY = -Platzmass::AufschlaglinieVomNetz;
			Feld.MaxY = 0.0;
			Feld.LinieMinY = ELinie::Aufschlaglinie;
			Feld.LinieMaxY = ELinie::Keine;
		}
		return Feld;
	}

	double FPlatz::NetzOberkante(double X)
	{
		const double Abstand = Betrag(X);
		if (Abstand >= Platzmass::EinzelstuetzeX)
		{
			return Platzmass::NetzhoehePfosten;
		}
		const double Anteil = Abstand / Platzmass::EinzelstuetzeX;
		return Platzmass::NetzhoeheMitte + (Platzmass::NetzhoehePfosten - Platzmass::NetzhoeheMitte) * Anteil;
	}

	FVek3 FPlatz::AufschlagPosition(EPlatzseite Seite, EAufschlagHaelfte Haelfte, double AbstandZurMitte) const
	{
		// Nahe Seite blickt nach +Y: rechts = +X. Ferne Seite blickt nach -Y: rechts = -X.
		const double RechtsX = Seite == EPlatzseite::Nah ? 1.0 : -1.0;
		const double SeiteX = Haelfte == EAufschlagHaelfte::Einstand ? RechtsX : -RechtsX;
		const double Y = SeitenVorzeichen(Seite) * (Platzmass::HalbeLaenge + 0.15);
		return FVek3(SeiteX * AbstandZurMitte, Y, 0.0);
	}
}
