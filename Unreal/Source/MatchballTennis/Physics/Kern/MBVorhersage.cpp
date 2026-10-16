// Matchball Tennis - Kern: Flugbahn-Vorhersage

#include "Physics/Kern/MBVorhersage.h"

namespace MB
{
	const FBallEreignis* FFlugbahn::ErsterAufsprung() const
	{
		for (const FBallEreignis& Ereignis : Ereignisse)
		{
			if (Ereignis.Art == EBallEreignisArt::Aufsprung)
			{
				return &Ereignis;
			}
		}
		return nullptr;
	}

	const FBallEreignis* FFlugbahn::ZweiterAufsprung() const
	{
		int Gefunden = 0;
		for (const FBallEreignis& Ereignis : Ereignisse)
		{
			if (Ereignis.Art == EBallEreignisArt::Aufsprung && ++Gefunden == 2)
			{
				return &Ereignis;
			}
		}
		return nullptr;
	}

	const FBallEreignis* FFlugbahn::ErsteNetzberuehrung() const
	{
		for (const FBallEreignis& Ereignis : Ereignisse)
		{
			if (Ereignis.Art == EBallEreignisArt::Netzband || Ereignis.Art == EBallEreignisArt::Netzgeflecht)
			{
				return &Ereignis;
			}
		}
		return nullptr;
	}

	double FFlugbahn::HoeheUeberNetz() const
	{
		for (const FBallEreignis& Ereignis : Ereignisse)
		{
			if (Ereignis.Art == EBallEreignisArt::NetzUeberquert)
			{
				return Ereignis.Position.Z;
			}
		}
		return -1.0;
	}

	bool FFlugbahn::UeberquertNetz(EPlatzseite Startseite) const
	{
		const FBallEreignis* Aufsprung = ErsterAufsprung();
		if (Aufsprung == nullptr)
		{
			return false;
		}
		return SeitenVorzeichen(Gegenseite(Startseite)) * Aufsprung->Position.Y > 0.0;
	}

	bool FFlugbahn::ZustandBei(double T, FBahnPunkt& OutPunkt) const
	{
		if (Punkte.empty() || T < Punkte.front().Zeit || T > Punkte.back().Zeit)
		{
			return false;
		}
		// Binaere Suche nach dem Intervall [i, i+1] mit Zeit[i] <= T <= Zeit[i+1]
		std::size_t Unten = 0;
		std::size_t Oben = Punkte.size() - 1;
		while (Oben - Unten > 1)
		{
			const std::size_t Mitte = (Unten + Oben) / 2;
			if (Punkte[Mitte].Zeit <= T)
			{
				Unten = Mitte;
			}
			else
			{
				Oben = Mitte;
			}
		}
		const FBahnPunkt& A = Punkte[Unten];
		const FBahnPunkt& B = Punkte[Oben];
		const double Spanne = B.Zeit - A.Zeit;
		const double Anteil = Spanne > 0.0 ? (T - A.Zeit) / Spanne : 0.0;
		OutPunkt.Zeit = T;
		OutPunkt.Position = Interpoliere(A.Position, B.Position, Anteil);
		OutPunkt.Geschwindigkeit = Interpoliere(A.Geschwindigkeit, B.Geschwindigkeit, Anteil);
		OutPunkt.Aufspruenge = Anteil < 1.0 ? A.Aufspruenge : B.Aufspruenge;
		return true;
	}

	FFlugbahn FVorhersage::Berechne(const FBallSimulation& Vorlage, const FBallZustand& Start, const FVorhersageOptionen& Optionen)
	{
		FBallSimulation Simulation = Vorlage;
		Simulation.Starte(Start);

		FFlugbahn Bahn;
		const int MaxSchritte = static_cast<int>(Optionen.MaxZeit / Simulation.GetZeitschritt()) + 1;
		if (Optionen.bPunkteSpeichern)
		{
			Bahn.Punkte.reserve(static_cast<std::size_t>(MaxSchritte) + 1u);
			Bahn.Punkte.push_back(FBahnPunkt{0.0, Start.Position, Start.Geschwindigkeit, 0});
		}

		int Aufspruenge = 0;
		double EndeZeit = -1.0;
		for (int Index = 0; Index < MaxSchritte; ++Index)
		{
			const std::size_t VorherAnzahl = Simulation.GetEreignisse().size();
			Simulation.Schritt();
			const std::vector<FBallEreignis>& Alle = Simulation.GetEreignisse();
			for (std::size_t Neu = VorherAnzahl; Neu < Alle.size(); ++Neu)
			{
				if (Alle[Neu].Art == EBallEreignisArt::Aufsprung)
				{
					++Aufspruenge;
					if (Aufspruenge >= Optionen.MaxAufspruenge && EndeZeit < 0.0)
					{
						EndeZeit = Alle[Neu].Zeit + Optionen.NachlaufNachEnde;
					}
				}
			}
			if (Optionen.bPunkteSpeichern)
			{
				const FBallZustand& Zustand = Simulation.GetZustand();
				Bahn.Punkte.push_back(FBahnPunkt{Simulation.GetZeit(), Zustand.Position, Zustand.Geschwindigkeit, Aufspruenge});
			}
			if (Simulation.GetZustand().bRuht || (EndeZeit >= 0.0 && Simulation.GetZeit() >= EndeZeit))
			{
				break;
			}
		}
		Bahn.Ereignisse = Simulation.GetEreignisse();
		return Bahn;
	}

	static bool MBVorhersage_PunktPasst(const FBahnPunkt& Punkt, const FTreffSuche& Suche)
	{
		if (Punkt.Zeit < Suche.MinZeit)
		{
			return false;
		}
		if (SeitenVorzeichen(Suche.Seite) * Punkt.Position.Y <= 0.0)
		{
			return false;
		}
		if (Punkt.Position.Z < Suche.MinHoehe || Punkt.Position.Z > Suche.MaxHoehe)
		{
			return false;
		}
		if (Punkt.Aufspruenge >= 2)
		{
			return false;
		}
		if (Punkt.Aufspruenge == 0 && !Suche.bVolleyErlaubt)
		{
			return false;
		}
		return true;
	}

	FTreffpunkt FindeTreffpunktInEbene(const FFlugbahn& Bahn, double TreffY, const FTreffSuche& Suche)
	{
		FTreffpunkt Ergebnis;
		for (std::size_t Index = 1; Index < Bahn.Punkte.size(); ++Index)
		{
			const FBahnPunkt& A = Bahn.Punkte[Index - 1];
			const FBahnPunkt& B = Bahn.Punkte[Index];
			const double DA = A.Position.Y - TreffY;
			const double DB = B.Position.Y - TreffY;
			if (DA == DB || DA * DB > 0.0)
			{
				continue;
			}
			const double Anteil = DA / (DA - DB);
			FBahnPunkt Punkt;
			Punkt.Zeit = Interpoliere(A.Zeit, B.Zeit, Anteil);
			Punkt.Position = Interpoliere(A.Position, B.Position, Anteil);
			Punkt.Geschwindigkeit = Interpoliere(A.Geschwindigkeit, B.Geschwindigkeit, Anteil);
			Punkt.Aufspruenge = B.Aufspruenge;
			if (MBVorhersage_PunktPasst(Punkt, Suche))
			{
				Ergebnis.Zeit = Punkt.Zeit;
				Ergebnis.Position = Punkt.Position;
				Ergebnis.Geschwindigkeit = Punkt.Geschwindigkeit;
				Ergebnis.bVolley = Punkt.Aufspruenge == 0;
				Ergebnis.bGueltig = true;
				return Ergebnis;
			}
		}
		return Ergebnis;
	}

	FTreffpunkt FindeErreichbarenTreffpunkt(const FFlugbahn& Bahn, const FVek3& Spielerposition, double Laufgeschwindigkeit, double Reaktionszeit, double Reichweite, const FTreffSuche& Suche)
	{
		FTreffpunkt Ergebnis;
		const double Tempo = Laufgeschwindigkeit > 0.1 ? Laufgeschwindigkeit : 0.1;
		for (const FBahnPunkt& Punkt : Bahn.Punkte)
		{
			if (!MBVorhersage_PunktPasst(Punkt, Suche))
			{
				continue;
			}
			const double DX = Punkt.Position.X - Spielerposition.X;
			const double DY = Punkt.Position.Y - Spielerposition.Y;
			const double Weg = std::sqrt(DX * DX + DY * DY) - Reichweite;
			const double Benoetigt = Reaktionszeit + (Weg > 0.0 ? Weg / Tempo : 0.0);
			if (Benoetigt <= Punkt.Zeit)
			{
				Ergebnis.Zeit = Punkt.Zeit;
				Ergebnis.Position = Punkt.Position;
				Ergebnis.Geschwindigkeit = Punkt.Geschwindigkeit;
				Ergebnis.bVolley = Punkt.Aufspruenge == 0;
				Ergebnis.bGueltig = true;
				return Ergebnis;
			}
		}
		return Ergebnis;
	}
}
