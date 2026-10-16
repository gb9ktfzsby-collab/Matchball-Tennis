// Matchball Tennis - Kern: deterministische Ballsimulation

#include "Physics/Kern/MBBallPhysik.h"

#if defined(_MSC_VER) && !defined(__clang__)
// Strenge Gleitkomma-Semantik fuer die Simulation (kein Umsortieren von Rechenschritten).
// Determinismus zwischen zwei PCs setzt ausserdem dieselbe .exe voraus (Build-Pruefung, README 15.3).
#pragma float_control(precise, on, push)
#endif

namespace MB
{
	// ---------------------------------------------------------------------------------------------
	// Belag

	double FBelagParameter::StosszahlBei(double NormalGeschwindigkeit) const
	{
		const double E = Stosszahl - StosszahlAbnahme * (NormalGeschwindigkeit - StosszahlReferenz);
		return Begrenze(E, 0.30, 0.95);
	}

	double FBelagParameter::CourtPaceRating() const
	{
		return 100.0 * (1.0 - Reibung) + 150.0 * (0.81 - Stosszahl);
	}

	void FBelagParameter::SetzeCourtPace(double ZielCpr)
	{
		// CPR = 100 (1 - mu) + 150 (0,81 - e)  =>  mu = 1 - (CPR - 150 (0,81 - e)) / 100
		const double NeueReibung = 1.0 - (ZielCpr - 150.0 * (0.81 - Stosszahl)) / 100.0;
		Reibung = Begrenze(NeueReibung, 0.30, 0.95);
	}

	// ---------------------------------------------------------------------------------------------
	// Simulation

	FBallSimulation::FBallSimulation()
		: FBallSimulation(FBallParameter(), FBelagParameter(), FNetzParameter(), StandardFrequenz)
	{
	}

	FBallSimulation::FBallSimulation(const FBallParameter& InBall, const FBelagParameter& InBelag, const FNetzParameter& InNetz, int InFrequenz)
		: Ball(InBall)
		, Belag(InBelag)
		, Netz(InNetz)
	{
		const int Frequenz = InFrequenz > 0 ? InFrequenz : StandardFrequenz;
		Zeitschritt = 1.0 / static_cast<double>(Frequenz);
		const double HalbRhoA = 0.5 * Ball.Luftdichte * Ball.Querschnitt();
		WiderstandsKonstante = HalbRhoA * Ball.Widerstandsbeiwert / Ball.Masse;
		AuftriebsKonstante = HalbRhoA / Ball.Masse;
		Ereignisse.reserve(16);
	}

	void FBallSimulation::Starte(const FBallZustand& Start)
	{
		Zustand = Start;
		Schritte = 0;
		SchrittBeginnZeit = 0.0;
		SchrittVerbraucht = 0.0;
		Ereignisse.clear();
		if (!Zustand.bRuht && Zustand.Position.Z <= Ball.Radius && Zustand.Geschwindigkeit.Z <= 0.0)
		{
			Zustand.Position.Z = Ball.Radius;
			Zustand.Geschwindigkeit.Z = 0.0;
			Zustand.bRollt = true;
		}
	}

	void FBallSimulation::Ableitung(const FVek3& Geschwindigkeit, const FVek3& Drall, FVek3& OutBeschleunigung, FVek3& OutDrallAenderung) const
	{
		const FVek3 Relativ = Geschwindigkeit - Wind;
		const double Tempo = Relativ.Laenge();
		OutBeschleunigung = FVek3(0.0, 0.0, -Ball.Schwerkraft);
		OutDrallAenderung = FVek3();
		if (Tempo < 1e-9)
		{
			return;
		}

		// Luftwiderstand: a = -k |v| v
		OutBeschleunigung += Relativ * (-WiderstandsKonstante * Tempo);

		// Magnus-Kraft: a = 0,5 rho A c_A |v|^2 / m in Richtung (w x v),
		// mit c_A = S / (1 + 2 S) und Drallzahl S = r |w| / |v| (Stepanek 1988).
		// Umgeformt: a = K r (w x v) |v| / (|v| + 2 r |w|) - ohne Division durch |w|.
		const double DrallBetrag = Drall.Laenge();
		if (DrallBetrag > 1e-9)
		{
			const double Nenner = Tempo + 2.0 * Ball.Radius * DrallBetrag;
			const double Faktor = AuftriebsKonstante * Ball.Radius * Tempo / Nenner;
			OutBeschleunigung += Kreuzprodukt(Drall, Relativ) * Faktor;
		}

		// Drall-Abnahme proportional zum geflogenen Weg
		OutDrallAenderung = Drall * (-Ball.DrallAbnahmeProMeter * Tempo);
	}

	FBallSimulation::FTeilschritt FBallSimulation::Integriere(const FTeilschritt& Start, double Dauer) const
	{
		const double Halb = 0.5 * Dauer;

		FVek3 A1, W1;
		Ableitung(Start.Geschwindigkeit, Start.Drall, A1, W1);
		const FVek3 V2 = Start.Geschwindigkeit + A1 * Halb;
		const FVek3 D2 = Start.Drall + W1 * Halb;

		FVek3 A2, W2;
		Ableitung(V2, D2, A2, W2);
		const FVek3 V3 = Start.Geschwindigkeit + A2 * Halb;
		const FVek3 D3 = Start.Drall + W2 * Halb;

		FVek3 A3, W3;
		Ableitung(V3, D3, A3, W3);
		const FVek3 V4 = Start.Geschwindigkeit + A3 * Dauer;
		const FVek3 D4 = Start.Drall + W3 * Dauer;

		FVek3 A4, W4;
		Ableitung(V4, D4, A4, W4);

		const double Sechstel = Dauer / 6.0;
		FTeilschritt Ende;
		Ende.Position = Start.Position + (Start.Geschwindigkeit + V2 * 2.0 + V3 * 2.0 + V4) * Sechstel;
		Ende.Geschwindigkeit = Start.Geschwindigkeit + (A1 + A2 * 2.0 + A3 * 2.0 + A4) * Sechstel;
		Ende.Drall = Start.Drall + (W1 + W2 * 2.0 + W3 * 2.0 + W4) * Sechstel;
		return Ende;
	}

	double FBallSimulation::Kontaktzeit() const
	{
		return KreiszahlPi * std::sqrt(Ball.Masse / Ball.BallSteifigkeit);
	}

	void FBallSimulation::BerechneAufsprung(const FVek3& V, const FVek3& W, FVek3& OutV, FVek3& OutW, double& OutGleitweg) const
	{
		const double R = Ball.Radius;
		const double Alpha = Ball.TraegheitsFaktor;
		const double Normal = V.Z < 0.0 ? -V.Z : 0.0;
		const double E = Belag.StosszahlBei(Normal);

		// Normalkraftstoss pro Masse
		const double NormalStoss = (1.0 + E) * Normal;

		// Gleitgeschwindigkeit des Kontaktpunkts: v_t + w x (-r z)
		const double GleitX = V.X - R * W.Y;
		const double GleitY = V.Y + R * W.X;
		const double Gleiten = std::sqrt(GleitX * GleitX + GleitY * GleitY);

		// Reibungsstoss: hoechstens mu * Normalstoss; reicht er, das Gleiten zu stoppen, rollt der Ball ab
		const double StossZumRollen = Gleiten * Alpha / (1.0 + Alpha);
		const double StossMax = Belag.Reibung * NormalStoss;
		const bool bGreift = StossZumRollen <= StossMax;
		const double TangentialStoss = bGreift ? StossZumRollen : StossMax;

		double RichtungX = 0.0;
		double RichtungY = 0.0;
		if (Gleiten > 1e-9)
		{
			RichtungX = GleitX / Gleiten;
			RichtungY = GleitY / Gleiten;
		}

		OutV = FVek3(V.X - TangentialStoss * RichtungX, V.Y - TangentialStoss * RichtungY, E * Normal);

		// Drehstoss r x F mit r = (0, 0, -r), F = -Stoss * m * Richtung; geteilt durch I = Alpha m r^2
		const double DrallFaktor = TangentialStoss / (Alpha * R);
		OutW = FVek3(W.X - DrallFaktor * RichtungY, W.Y + DrallFaktor * RichtungX, W.Z);

		// Gleitweg waehrend des Kontakts (fuer Abdruck/Spuren): mittlere Gleitgeschwindigkeit * Gleitdauer
		const double Kontakt = Kontaktzeit();
		if (Gleiten <= 1e-9 || StossMax <= 1e-12)
		{
			OutGleitweg = Gleiten * Kontakt;
		}
		else if (bGreift)
		{
			const double Gleitdauer = Kontakt * (StossZumRollen / StossMax);
			OutGleitweg = 0.5 * Gleiten * Gleitdauer;
		}
		else
		{
			const double GleitenEnde = Gleiten - StossMax * (1.0 + 1.0 / Alpha);
			OutGleitweg = 0.5 * (Gleiten + GleitenEnde) * Kontakt;
		}
	}

	void FBallSimulation::FuegeEreignisHinzu(EBallEreignisArt Art, double Zeitanteil, const FVek3& Pos, const FVek3& VVorher, const FVek3& VNachher, const FVek3& WVorher, const FVek3& WNachher, double Gleitweg)
	{
		FBallEreignis Ereignis;
		Ereignis.Art = Art;
		Ereignis.Schritt = Schritte;
		Ereignis.Zeit = SchrittBeginnZeit + SchrittVerbraucht + Zeitanteil;
		Ereignis.Position = Pos;
		Ereignis.GeschwindigkeitVorher = VVorher;
		Ereignis.GeschwindigkeitNachher = VNachher;
		Ereignis.DrallVorher = WVorher;
		Ereignis.DrallNachher = WNachher;
		Ereignis.Gleitweg = Gleitweg;
		Ereignisse.push_back(Ereignis);
	}

	void FBallSimulation::Schritt()
	{
		SchrittBeginnZeit = static_cast<double>(Schritte) * Zeitschritt;
		SchrittVerbraucht = 0.0;
		if (!Zustand.bRuht)
		{
			if (Zustand.bRollt)
			{
				Rollen(Zeitschritt);
			}
			else
			{
				SchrittMitDauer(Zeitschritt, 0);
			}
			PruefeRuhe();
		}
		++Schritte;
	}

	void FBallSimulation::SimuliereBis(double Zielzeit)
	{
		// Anzahl der noetigen Schritte ganzzahlig bestimmen (keine Drift durch Aufsummieren)
		const double Ziel = Zielzeit / Zeitschritt;
		while (static_cast<double>(Schritte) < Ziel - 1e-9 && !Zustand.bRuht)
		{
			Schritt();
		}
	}

	void FBallSimulation::SchrittMitDauer(double Dauer, int Tiefe)
	{
		if (Dauer <= 0.0 || Zustand.bRuht)
		{
			return;
		}

		FTeilschritt Start;
		Start.Position = Zustand.Position;
		Start.Geschwindigkeit = Zustand.Geschwindigkeit;
		Start.Drall = Zustand.Drall;
		const FTeilschritt Ende = Integriere(Start, Dauer);

		const double R = Ball.Radius;
		const FVek3 Weg = Ende.Position - Start.Position;

		// Fruehestes Kollisionsereignis im Schritt suchen (Anteil 0..1 entlang des Schritts)
		enum class EKollision : unsigned char { Keine, Boden, Band, Geflecht };
		EKollision Kollision = EKollision::Keine;
		double KollisionsAnteil = 2.0;

		// Boden (liegt der Ball bereits auf und faellt, sofortiger Kontakt)
		if (Start.Position.Z <= R && Start.Geschwindigkeit.Z < 0.0)
		{
			KollisionsAnteil = 0.0;
			Kollision = EKollision::Boden;
		}
		else if (Start.Position.Z > R && Ende.Position.Z < R)
		{
			const double Anteil = Begrenze((Start.Position.Z - R) / (Start.Position.Z - Ende.Position.Z), 0.0, 1.0);
			if (Anteil < KollisionsAnteil)
			{
				KollisionsAnteil = Anteil;
				Kollision = EKollision::Boden;
			}
		}

		// Netz nur pruefen, wenn der Schritt in Netznaehe liegt
		const double Reichweite = R + Netz.BandRadius + 0.05;
		const bool bNetzNah = (Start.Position.Y * Ende.Position.Y <= 0.0) || Betrag(Start.Position.Y) < Reichweite || Betrag(Ende.Position.Y) < Reichweite;
		if (bNetzNah)
		{
			// Netzband: Zylinder entlang X, Achse auf Hoehe (Oberkante - Bandradius)
			const double Abstand = R + Netz.BandRadius;
			const double AchseStart = FPlatz::NetzOberkante(Start.Position.X) - Netz.BandRadius;
			const double AchseEnde = FPlatz::NetzOberkante(Ende.Position.X) - Netz.BandRadius;
			const double Y0 = Start.Position.Y;
			const double DY = Weg.Y;
			const double Z0 = Start.Position.Z - AchseStart;
			const double DZ = (Ende.Position.Z - AchseEnde) - Z0;
			const double QA = DY * DY + DZ * DZ;
			const double QB = 2.0 * (Y0 * DY + Z0 * DZ);
			const double QC = Y0 * Y0 + Z0 * Z0 - Abstand * Abstand;
			if (QC > 0.0 && QA > 1e-18)
			{
				const double Diskriminante = QB * QB - 4.0 * QA * QC;
				if (Diskriminante >= 0.0)
				{
					const double Anteil = (-QB - std::sqrt(Diskriminante)) / (2.0 * QA);
					if (Anteil >= 0.0 && Anteil <= 1.0 && Anteil < KollisionsAnteil)
					{
						const double XTreffer = Start.Position.X + Weg.X * Anteil;
						if (Betrag(XTreffer) <= FPlatz::NetzHalbeBreite())
						{
							KollisionsAnteil = Anteil;
							Kollision = EKollision::Band;
						}
					}
				}
			}

			// Netzgeflecht: Ebene Y = +-R auf der Seite, von der der Ball kommt
			double Ebene = 0.0;
			bool bPruefen = false;
			if (Start.Position.Y >= R && Ende.Position.Y < R)
			{
				Ebene = R;
				bPruefen = true;
			}
			else if (Start.Position.Y <= -R && Ende.Position.Y > -R)
			{
				Ebene = -R;
				bPruefen = true;
			}
			if (bPruefen && Betrag(Weg.Y) > 1e-15)
			{
				const double Anteil = (Start.Position.Y - Ebene) / (Start.Position.Y - Ende.Position.Y);
				if (Anteil >= 0.0 && Anteil <= 1.0 && Anteil < KollisionsAnteil)
				{
					const double XTreffer = Start.Position.X + Weg.X * Anteil;
					const double ZTreffer = Start.Position.Z + Weg.Z * Anteil;
					const double Achse = FPlatz::NetzOberkante(XTreffer) - Netz.BandRadius;
					if (Betrag(XTreffer) <= FPlatz::NetzHalbeBreite() && ZTreffer < Achse && ZTreffer > -R)
					{
						KollisionsAnteil = Anteil;
						Kollision = EKollision::Geflecht;
					}
				}
			}
		}

		// Ueberqueren der Netzebene (Y = 0) vor der Kollision bzw. im ganzen Schritt
		const double Grenze = Kollision == EKollision::Keine ? 1.0 : KollisionsAnteil;
		if (Start.Position.Y != 0.0 && Start.Position.Y * Ende.Position.Y <= 0.0 && Weg.Y != 0.0)
		{
			const double Anteil = Start.Position.Y / (Start.Position.Y - Ende.Position.Y);
			if (Anteil >= 0.0 && Anteil <= Grenze)
			{
				const FVek3 Pos = Interpoliere(Start.Position, Ende.Position, Anteil);
				const FVek3 V = Interpoliere(Start.Geschwindigkeit, Ende.Geschwindigkeit, Anteil);
				const FVek3 W = Interpoliere(Start.Drall, Ende.Drall, Anteil);
				FuegeEreignisHinzu(EBallEreignisArt::NetzUeberquert, Anteil * Dauer, Pos, V, V, W, W, 0.0);
			}
		}

		if (Kollision == EKollision::Keine || Tiefe >= 6)
		{
			Zustand.Position = Ende.Position;
			Zustand.Geschwindigkeit = Ende.Geschwindigkeit;
			Zustand.Drall = Ende.Drall;
			// Sicherung (nur bei Erreichen der Rekursionsgrenze denkbar): nie unter den Boden
			if (Zustand.Position.Z < R)
			{
				Zustand.Position.Z = R;
				if (Zustand.Geschwindigkeit.Z < 0.0)
				{
					Zustand.Geschwindigkeit.Z = 0.0;
					Zustand.bRollt = true;
				}
			}
			SchrittVerbraucht += Dauer;
			return;
		}

		// Zustand zum Kollisionszeitpunkt (linear im Schritt; Fehler < 0,1 mm bei 240 Hz)
		const FVek3 Pos = Interpoliere(Start.Position, Ende.Position, KollisionsAnteil);
		const FVek3 VVorher = Interpoliere(Start.Geschwindigkeit, Ende.Geschwindigkeit, KollisionsAnteil);
		const FVek3 WVorher = Interpoliere(Start.Drall, Ende.Drall, KollisionsAnteil);
		FVek3 VNachher = VVorher;
		FVek3 WNachher = WVorher;
		FVek3 PosNachher = Pos;
		EBallEreignisArt Art = EBallEreignisArt::Aufsprung;
		double Gleitweg = 0.0;

		if (Kollision == EKollision::Boden)
		{
			Art = EBallEreignisArt::Aufsprung;
			PosNachher.Z = R;
			BerechneAufsprung(VVorher, WVorher, VNachher, WNachher, Gleitweg);
			// Sehr flacher Absprung: Ball rollt
			if (VNachher.Z < 0.25)
			{
				VNachher.Z = 0.0;
				Zustand.bRollt = true;
			}
		}
		else if (Kollision == EKollision::Band)
		{
			Art = EBallEreignisArt::Netzband;
			const double Achse = FPlatz::NetzOberkante(Pos.X) - Netz.BandRadius;
			FVek3 Normale(0.0, Pos.Y, Pos.Z - Achse);
			Normale = Normale.Normalisiert();
			const double VNormal = Skalarprodukt(VVorher, Normale);
			if (VNormal < 0.0)
			{
				const FVek3 Tangential = VVorher - Normale * VNormal;
				VNachher = Tangential * Netz.BandTangentialFaktor + Normale * (-Netz.BandStosszahl * VNormal);
				WNachher = WVorher * 0.7;
			}
		}
		else
		{
			Art = EBallEreignisArt::Netzgeflecht;
			VNachher = FVek3(VVorher.X * Netz.GeflechtDaempfung, -VVorher.Y * Netz.GeflechtStosszahl, VVorher.Z * Netz.GeflechtDaempfung);
			WNachher = WVorher * 0.2;
		}

		FuegeEreignisHinzu(Art, KollisionsAnteil * Dauer, PosNachher, VVorher, VNachher, WVorher, WNachher, Gleitweg);

		Zustand.Position = PosNachher;
		Zustand.Geschwindigkeit = VNachher;
		Zustand.Drall = WNachher;
		SchrittVerbraucht += KollisionsAnteil * Dauer;

		const double Rest = (1.0 - KollisionsAnteil) * Dauer;
		if (Zustand.bRollt)
		{
			Rollen(Rest);
		}
		else
		{
			SchrittMitDauer(Rest, Tiefe + 1);
		}
	}

	void FBallSimulation::Rollen(double Dauer)
	{
		if (Dauer <= 0.0)
		{
			return;
		}
		const double R = Ball.Radius;
		FVek3 V(Zustand.Geschwindigkeit.X, Zustand.Geschwindigkeit.Y, 0.0);
		const double Tempo = V.LaengeXY();
		const FVek3 PosStart = Zustand.Position;
		if (Tempo < 0.05)
		{
			Zustand.Geschwindigkeit = FVek3();
			Zustand.Drall = FVek3();
			Zustand.bRuht = true;
			FuegeEreignisHinzu(EBallEreignisArt::Ruhe, 0.0, Zustand.Position, V, FVek3(), Zustand.Drall, FVek3(), 0.0);
			SchrittVerbraucht += Dauer;
			return;
		}
		const double Abnahme = Belag.RollReibung * Dauer;
		const double NeuesTempo = Abnahme >= Tempo ? 0.0 : Tempo - Abnahme;
		const FVek3 VNeu = V * (NeuesTempo / Tempo);
		FVek3 PosNeu = PosStart + (V + VNeu) * (0.5 * Dauer);
		PosNeu.Z = R;

		// Ball rollt ins Netz: bleibt dort liegen
		if (PosStart.Y * PosNeu.Y <= 0.0 && PosStart.Y != PosNeu.Y)
		{
			const double Anteil = PosStart.Y / (PosStart.Y - PosNeu.Y);
			const double XTreffer = PosStart.X + (PosNeu.X - PosStart.X) * Anteil;
			if (Betrag(XTreffer) <= FPlatz::NetzHalbeBreite())
			{
				const double Seite = PosStart.Y >= 0.0 ? 1.0 : -1.0;
				Zustand.Position = FVek3(XTreffer, Seite * R, R);
				Zustand.Geschwindigkeit = FVek3();
				Zustand.Drall = FVek3();
				Zustand.bRuht = true;
				FuegeEreignisHinzu(EBallEreignisArt::Netzgeflecht, Anteil * Dauer, Zustand.Position, V, FVek3(), FVek3(), FVek3(), 0.0);
				SchrittVerbraucht += Dauer;
				return;
			}
		}

		Zustand.Position = PosNeu;
		Zustand.Geschwindigkeit = VNeu;
		// Rollen ohne Gleiten: w = (z x v) / r  (Kontaktpunkt steht still)
		Zustand.Drall = FVek3(-VNeu.Y / R, VNeu.X / R, 0.0);
		if (NeuesTempo <= 0.0)
		{
			Zustand.bRuht = true;
			Zustand.Drall = FVek3();
			FuegeEreignisHinzu(EBallEreignisArt::Ruhe, Dauer, Zustand.Position, V, FVek3(), FVek3(), FVek3(), 0.0);
		}
		SchrittVerbraucht += Dauer;
	}

	void FBallSimulation::PruefeRuhe()
	{
		if (Zustand.bRuht)
		{
			return;
		}
		const FVek3& P = Zustand.Position;
		if (Betrag(P.X) > BereichHalbeBreite || Betrag(P.Y) > BereichHalbeLaenge || P.Z < -1.0 || P.Z > 60.0)
		{
			Zustand.bRuht = true;
			SchrittVerbraucht = Zeitschritt;
			FuegeEreignisHinzu(EBallEreignisArt::Ruhe, 0.0, P, Zustand.Geschwindigkeit, FVek3(), Zustand.Drall, FVek3(), 0.0);
		}
	}
}

#if defined(_MSC_VER) && !defined(__clang__)
#pragma float_control(pop)
#endif
