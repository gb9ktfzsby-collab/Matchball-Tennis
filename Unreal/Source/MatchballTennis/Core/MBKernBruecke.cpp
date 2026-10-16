// Matchball Tennis - Bruecke zwischen Kern und Unreal

#include "Core/MBKernBruecke.h"

#include "Core/MBDatenTabellen.h"

namespace MBBruecke
{
	void DrehungFuerDarstellung(const MB::FVek3& Drall, double Dt, FVector& OutAchse, double& OutWinkel)
	{
		const double Betrag = Drall.Laenge();
		if (Betrag < 1e-6)
		{
			OutAchse = FVector::UpVector;
			OutWinkel = 0.0;
			return;
		}
		// Achse wie eine Position abbilden, Drehsinn wegen der Spiegelung umkehren
		OutAchse = FVector(Drall.Y, Drall.X, Drall.Z) / Betrag;
		OutWinkel = -Betrag * Dt;
	}

	MB::FBelagParameter Belag(const FMBBelagZeile& Zeile, const FName& Name)
	{
		MB::FBelagParameter Ergebnis;
		Ergebnis.Name = TCHAR_TO_UTF8(*Name.ToString());
		Ergebnis.Reibung = Zeile.Reibung;
		Ergebnis.Stosszahl = Zeile.Stosszahl;
		Ergebnis.StosszahlReferenz = Zeile.StosszahlReferenz;
		Ergebnis.StosszahlAbnahme = Zeile.StosszahlAbnahme;
		Ergebnis.RollReibung = Zeile.RollReibung;
		return Ergebnis;
	}

	MB::FBallParameter Ball(const FMBBallZeile& Zeile)
	{
		MB::FBallParameter Ergebnis;
		Ergebnis.Masse = Zeile.Masse;
		Ergebnis.Radius = Zeile.Radius;
		Ergebnis.Luftdichte = Zeile.Luftdichte;
		Ergebnis.Widerstandsbeiwert = Zeile.Widerstandsbeiwert;
		Ergebnis.TraegheitsFaktor = Zeile.TraegheitsFaktor;
		Ergebnis.DrallAbnahmeProMeter = Zeile.DrallAbnahmeProMeter;
		Ergebnis.Schwerkraft = Zeile.Schwerkraft;
		Ergebnis.BallSteifigkeit = Zeile.BallSteifigkeit;
		return Ergebnis;
	}

	MB::FNetzParameter Netz(const FMBNetzZeile& Zeile)
	{
		MB::FNetzParameter Ergebnis;
		Ergebnis.BandRadius = Zeile.BandRadius;
		Ergebnis.BandStosszahl = Zeile.BandStosszahl;
		Ergebnis.BandTangentialFaktor = Zeile.BandTangentialFaktor;
		Ergebnis.GeflechtStosszahl = Zeile.GeflechtStosszahl;
		Ergebnis.GeflechtDaempfung = Zeile.GeflechtDaempfung;
		return Ergebnis;
	}
}
