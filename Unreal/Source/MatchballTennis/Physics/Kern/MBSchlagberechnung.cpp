// Matchball Tennis - Kern: Schlagberechnung

#include "Physics/Kern/MBSchlagberechnung.h"

namespace MB
{
	const char* SchlagArtName(ESchlagArt Art)
	{
		switch (Art)
		{
		case ESchlagArt::Flach: return "Flach";
		case ESchlagArt::Topspin: return "Topspin";
		case ESchlagArt::Slice: return "Slice";
		case ESchlagArt::Lob: return "Lob";
		case ESchlagArt::Stopp: return "Stopp";
		case ESchlagArt::Volley: return "Volley";
		case ESchlagArt::Halbvolley: return "Halbvolley";
		case ESchlagArt::Schmetterball: return "Schmetterball";
		case ESchlagArt::AufschlagFlach: return "AufschlagFlach";
		case ESchlagArt::AufschlagKick: return "AufschlagKick";
		case ESchlagArt::AufschlagSlice: return "AufschlagSlice";
		}
		return "Unbekannt";
	}

	FVek3 DrallFuerRichtung(const FVek3& Richtung, double Topspin, double Seitendrall)
	{
		FVek3 Horizontal(Richtung.X, Richtung.Y, 0.0);
		Horizontal = Horizontal.Normalisiert();
		// Topspin-Achse: z x h (Oberseite des Balls dreht in Flugrichtung)
		const FVek3 Achse = Kreuzprodukt(FVek3(0.0, 0.0, 1.0), Horizontal);
		return Achse * Topspin + FVek3(0.0, 0.0, Seitendrall);
	}

	namespace
	{
		struct FMBSchlagProbe
		{
			double Weite = 0.0;      // entlang der Zielrichtung
			double Quer = 0.0;       // seitlich zur Zielrichtung (+ = links)
			FVek3 Aufsprung;
			double Flugzeit = 0.0;
			double NetzAbstand = -1.0;
			bool bNetzBeruehrt = false;
			bool bAufsprung = false;
		};

		// Simuliert einen Probeschlag bis zum ersten Aufsprung (ohne Punkte zu speichern)
		FMBSchlagProbe MBSchlag_Probe(const FBallSimulation& Vorlage, const FSchlagAnfrage& Anfrage, const FVek3& Richtung, const FVek3& Quer, const FVek3& Abflugrichtung, double Tempo, FBallZustand* OutAbflug)
		{
			FBallSimulation Simulation = Vorlage;
			FBallZustand Start;
			Start.Position = Anfrage.Treffpunkt;
			Start.Geschwindigkeit = Abflugrichtung * Tempo;
			Start.Drall = DrallFuerRichtung(Abflugrichtung, Anfrage.Topspin, Anfrage.Seitendrall);
			if (OutAbflug != nullptr)
			{
				*OutAbflug = Start;
			}
			Simulation.Starte(Start);

			FMBSchlagProbe Probe;
			const double Radius = Simulation.GetBall().Radius;
			const int MaxSchritte = static_cast<int>(6.0 / Simulation.GetZeitschritt());
			for (int Index = 0; Index < MaxSchritte; ++Index)
			{
				const std::size_t Vorher = Simulation.GetEreignisse().size();
				Simulation.Schritt();
				const std::vector<FBallEreignis>& Alle = Simulation.GetEreignisse();
				bool bFertig = false;
				for (std::size_t Neu = Vorher; Neu < Alle.size(); ++Neu)
				{
					const FBallEreignis& Ereignis = Alle[Neu];
					if (Ereignis.Art == EBallEreignisArt::NetzUeberquert && Probe.NetzAbstand < 0.0 && !Probe.bNetzBeruehrt)
					{
						Probe.NetzAbstand = Ereignis.Position.Z - Radius - FPlatz::NetzOberkante(Ereignis.Position.X);
					}
					else if (Ereignis.Art == EBallEreignisArt::Netzband || Ereignis.Art == EBallEreignisArt::Netzgeflecht)
					{
						Probe.bNetzBeruehrt = true;
						Probe.NetzAbstand = -1.0;
					}
					else if (Ereignis.Art == EBallEreignisArt::Aufsprung)
					{
						Probe.bAufsprung = true;
						Probe.Aufsprung = Ereignis.Position;
						Probe.Flugzeit = Ereignis.Zeit;
						bFertig = true;
						break;
					}
				}
				if (bFertig)
				{
					break;
				}
				if (Simulation.GetZustand().bRuht)
				{
					Probe.Aufsprung = Simulation.GetZustand().Position;
					Probe.Flugzeit = Simulation.GetZeit();
					break;
				}
			}
			if (!Probe.bAufsprung && !Simulation.GetZustand().bRuht)
			{
				Probe.Aufsprung = Simulation.GetZustand().Position;
				Probe.Flugzeit = Simulation.GetZeit();
			}
			const FVek3 Versatz(Probe.Aufsprung.X - Anfrage.Treffpunkt.X, Probe.Aufsprung.Y - Anfrage.Treffpunkt.Y, 0.0);
			Probe.Weite = Skalarprodukt(Versatz, Richtung);
			Probe.Quer = Skalarprodukt(FVek3(Probe.Aufsprung.X - Anfrage.ZielX, Probe.Aufsprung.Y - Anfrage.ZielY, 0.0), Quer);
			return Probe;
		}

		FVek3 MBSchlag_Abflugrichtung(const FVek3& Richtung, const FVek3& Quer, double Korrektur, double Steigung)
		{
			const FVek3 Horizontal = (Richtung + Quer * Korrektur).Normalisiert();
			return FVek3(Horizontal.X, Horizontal.Y, Steigung).Normalisiert();
		}

		// Sucht die Steigung, mit der die Weite Zielweite erreicht wird (untere oder obere Loesung)
		bool MBSchlag_SucheSteigung(const FBallSimulation& Vorlage, const FSchlagAnfrage& Anfrage, const FVek3& Richtung, const FVek3& Quer, double Korrektur, double Tempo, double Zielweite, double& OutSteigung)
		{
			constexpr int Raster = 28;
			constexpr double SteigungMin = -0.60;
			const double SteigungMax = Anfrage.bHoherBogen ? 5.0 : 2.0;
			double Steigungen[Raster + 1];
			double Weiten[Raster + 1];
			for (int Index = 0; Index <= Raster; ++Index)
			{
				const double Steigung = SteigungMin + (SteigungMax - SteigungMin) * (static_cast<double>(Index) / static_cast<double>(Raster));
				Steigungen[Index] = Steigung;
				Weiten[Index] = MBSchlag_Probe(Vorlage, Anfrage, Richtung, Quer, MBSchlag_Abflugrichtung(Richtung, Quer, Korrektur, Steigung), Tempo, nullptr).Weite;
			}

			int Unten = -1;
			if (!Anfrage.bHoherBogen)
			{
				// Untere Loesung: erster Wechsel von "zu kurz" zu "lang genug"
				for (int Index = 1; Index <= Raster; ++Index)
				{
					if (Weiten[Index - 1] < Zielweite && Weiten[Index] >= Zielweite)
					{
						Unten = Index - 1;
						break;
					}
				}
			}
			else
			{
				// Obere Loesung: letzter Wechsel von "lang genug" zu "zu kurz"
				for (int Index = Raster; Index >= 1; --Index)
				{
					if (Weiten[Index - 1] >= Zielweite && Weiten[Index] < Zielweite)
					{
						Unten = Index - 1;
						break;
					}
				}
			}
			if (Unten < 0)
			{
				// Nicht erreichbar: Steigung mit der groessten Weite (bzw. kleinster Abweichung)
				int Beste = 0;
				for (int Index = 1; Index <= Raster; ++Index)
				{
					if (Betrag(Weiten[Index] - Zielweite) < Betrag(Weiten[Beste] - Zielweite))
					{
						Beste = Index;
					}
				}
				OutSteigung = Steigungen[Beste];
				return false;
			}

			// Bisektion zwischen Steigungen[Unten] und Steigungen[Unten + 1]
			double A = Steigungen[Unten];
			double B = Steigungen[Unten + 1];
			const bool bAZuKurz = Weiten[Unten] < Zielweite;
			for (int Iteration = 0; Iteration < 18; ++Iteration)
			{
				const double Mitte = 0.5 * (A + B);
				const double Weite = MBSchlag_Probe(Vorlage, Anfrage, Richtung, Quer, MBSchlag_Abflugrichtung(Richtung, Quer, Korrektur, Mitte), Tempo, nullptr).Weite;
				const bool bMitteZuKurz = Weite < Zielweite;
				if (bMitteZuKurz == bAZuKurz)
				{
					A = Mitte;
				}
				else
				{
					B = Mitte;
				}
			}
			OutSteigung = 0.5 * (A + B);
			return true;
		}
	}

	FSchlagErgebnis BerechneSchlag(const FBallSimulation& Vorlage, const FSchlagAnfrage& Anfrage)
	{
		FSchlagErgebnis Ergebnis;

		FVek3 Richtung(Anfrage.ZielX - Anfrage.Treffpunkt.X, Anfrage.ZielY - Anfrage.Treffpunkt.Y, 0.0);
		const double Zielweite = Richtung.LaengeXY();
		Richtung = Richtung.Normalisiert();
		if (Zielweite < 0.5)
		{
			// Ziel direkt unter dem Ball: senkrecht fallen lassen (Sonderfall, sollte nicht vorkommen)
			Ergebnis.Abflug.Position = Anfrage.Treffpunkt;
			return Ergebnis;
		}
		// Quer = links aus Sicht des Schlagenden
		const FVek3 Quer(-Richtung.Y, Richtung.X, 0.0);

		double Tempo = Anfrage.Tempo;
		const double MinTempo = Anfrage.MinTempo < Anfrage.Tempo ? Anfrage.MinTempo : Anfrage.Tempo;
		constexpr int MaxTempoStufen = 16;
		FSchlagErgebnis LetztesGefundenes;
		bool bHatGefunden = false;

		for (int Stufe = 0; Stufe < MaxTempoStufen; ++Stufe)
		{
			double Korrektur = 0.0;
			double Steigung = 0.0;
			bool bGefunden = false;
			for (int Seitlich = 0; Seitlich < 3; ++Seitlich)
			{
				bGefunden = MBSchlag_SucheSteigung(Vorlage, Anfrage, Richtung, Quer, Korrektur, Tempo, Zielweite, Steigung);
				if (!bGefunden)
				{
					// Ohne gueltige Weite ist die seitliche Abweichung bedeutungslos
					break;
				}
				const FMBSchlagProbe Probe = MBSchlag_Probe(Vorlage, Anfrage, Richtung, Quer, MBSchlag_Abflugrichtung(Richtung, Quer, Korrektur, Steigung), Tempo, nullptr);
				if (Probe.bNetzBeruehrt || !Probe.bAufsprung || Probe.Weite <= 0.5)
				{
					break;
				}
				// Seitliche Abweichung in eine Richtungskorrektur umrechnen (kleine Winkel)
				Korrektur -= Probe.Quer / Probe.Weite;
			}
			if (bGefunden)
			{
				// Mit der endgueltigen Korrektur noch einmal die Steigung bestimmen
				bGefunden = MBSchlag_SucheSteigung(Vorlage, Anfrage, Richtung, Quer, Korrektur, Tempo, Zielweite, Steigung);
			}

			FBallZustand Abflug;
			const FMBSchlagProbe Probe = MBSchlag_Probe(Vorlage, Anfrage, Richtung, Quer, MBSchlag_Abflugrichtung(Richtung, Quer, Korrektur, Steigung), Tempo, &Abflug);
			Ergebnis.Abflug = Abflug;
			Ergebnis.Aufsprung = Probe.Aufsprung;
			Ergebnis.Flugzeit = Probe.Flugzeit;
			Ergebnis.NetzAbstand = Probe.bNetzBeruehrt ? -1.0 : Probe.NetzAbstand;
			Ergebnis.TatsaechlichesTempo = Tempo;
			Ergebnis.bNetzFrei = !Probe.bNetzBeruehrt && Probe.NetzAbstand >= 0.0;
			const double Abweichung = std::sqrt(Quadrat(Probe.Aufsprung.X - Anfrage.ZielX) + Quadrat(Probe.Aufsprung.Y - Anfrage.ZielY));
			Ergebnis.bZielErreicht = bGefunden && Abweichung < 0.15;

			// Ziel bei diesem Tempo nicht erreichbar (zu weit): ein langsamerer Ball kaeme noch kuerzer -
			// das beste Ergebnis bleibt stehen (Ball landet zu kurz).
			if (!bGefunden)
			{
				if (bHatGefunden)
				{
					Ergebnis = LetztesGefundenes;
				}
				break;
			}
			LetztesGefundenes = Ergebnis;
			bHatGefunden = true;
			const bool bNetzReicht = Ergebnis.bNetzFrei && Probe.NetzAbstand >= Anfrage.MinNetzAbstand;
			if (bNetzReicht || Tempo <= MinTempo + 1e-9)
			{
				break;
			}
			// Zu flach fuers Netz: Tempo senken (hoeherer Bogen) und erneut suchen
			Tempo = MaxWert(MinTempo, Tempo * 0.93);
		}
		return Ergebnis;
	}

	// ---------------------------------------------------------------------------------------------
	// Timing und Streuung

	static double MBSchlag_WertFaktor(double Spielerwert, double Bei50, double Bei99)
	{
		const double Anteil = Begrenze((Spielerwert - 50.0) / 49.0, -1.0, 1.0);
		return Bei50 + (Bei99 - Bei50) * Anteil;
	}

	FStreuParameter WendeModifikatorenAn(const FStreuParameter& Grund, const FStreuModifikatoren& Mod, bool bPower, bool bPraezision, double Spielerwert)
	{
		FStreuParameter Ergebnis = Grund;
		double Fenster = Mod.MatchFenster * MBSchlag_WertFaktor(Spielerwert, Mod.FensterBeiWert50, Mod.FensterBeiWert99);
		double Streuung = MBSchlag_WertFaktor(Spielerwert, Mod.StreuungBeiWert50, Mod.StreuungBeiWert99);
		if (bPower && !bPraezision)
		{
			Fenster *= Mod.PowerFenster;
			Streuung *= Mod.PowerStreuung;
		}
		else if (bPraezision && !bPower)
		{
			Fenster *= Mod.PraezisionFenster;
			Streuung *= Mod.PraezisionStreuung;
		}
		Ergebnis.GruenHalbfenster *= Fenster;
		Ergebnis.GelbHalbfenster *= Fenster;
		Ergebnis.Grundstreuung *= Streuung;
		Ergebnis.SeitlichProSekunde *= Streuung;
		Ergebnis.LaengenStreuung *= Streuung;
		Ergebnis.LaengeProSekunde *= Streuung;
		return Ergebnis;
	}

	ETimingBewertung BewerteTiming(double TimingFehler, const FStreuParameter& Parameter)
	{
		const double Fehler = Betrag(TimingFehler);
		if (Fehler <= Parameter.GruenHalbfenster)
		{
			return ETimingBewertung::Gruen;
		}
		if (Fehler <= Parameter.GelbHalbfenster)
		{
			return ETimingBewertung::Gelb;
		}
		return ETimingBewertung::Rot;
	}

	FStreuErgebnis StreueZiel(const FVek3& Treffpunkt, double ZielX, double ZielY, double TimingFehler, const FStreuParameter& Parameter, FZufall& Zufall)
	{
		FStreuErgebnis Ergebnis;
		Ergebnis.Bewertung = BewerteTiming(TimingFehler, Parameter);

		const double Fehler = Betrag(TimingFehler);
		const double Ueberschuss = Fehler > Parameter.GruenHalbfenster ? Fehler - Parameter.GruenHalbfenster : 0.0;

		// Seitlich: Betrag waechst mit dem Fehler, Richtung zufaellig (beide Seiten gleich riskant)
		const double Grund = Betrag(Zufall.Normal()) * Parameter.Grundstreuung;
		const double Seitlich = Zufall.Vorzeichen() * (Grund + Ueberschuss * Parameter.SeitlichProSekunde);

		// Laenge: leichte Streuung in beide Richtungen
		const double Laenge = Zufall.Normal() * (Parameter.LaengenStreuung + Fehler * Parameter.LaengeProSekunde) * 0.5;

		FVek3 Richtung(ZielX - Treffpunkt.X, ZielY - Treffpunkt.Y, 0.0);
		Richtung = Richtung.Normalisiert();
		const FVek3 Quer(-Richtung.Y, Richtung.X, 0.0);

		Ergebnis.Seitlich = Seitlich;
		Ergebnis.Laenge = Laenge;
		Ergebnis.ZielX = ZielX + Quer.X * Seitlich + Richtung.X * Laenge;
		Ergebnis.ZielY = ZielY + Quer.Y * Seitlich + Richtung.Y * Laenge;
		return Ergebnis;
	}
}
