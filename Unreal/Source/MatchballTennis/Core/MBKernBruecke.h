// Matchball Tennis - Bruecke zwischen dem reinen C++-Kern und Unreal
//
// Kern: Meter, rechtshaendig (X quer, Y laengs zur fernen Grundlinie, Z hoch), Ursprung Netzmitte am Boden.
// Unreal: Zentimeter, linkshaendig (X vorwaerts, Y rechts, Z hoch).
// Abbildung: UE.X = Kern.Y * 100, UE.Y = Kern.X * 100, UE.Z = Kern.Z * 100.
// Die nahe Grundlinie liegt damit bei UE.X = -1188,5 cm, der Blick der nahen Seite geht nach +X.

#pragma once

#include "CoreMinimal.h"

#include "Core/Kern/MBMathe.h"
#include "Physics/Kern/MBBallPhysik.h"

struct FMBBelagZeile;
struct FMBBallZeile;
struct FMBNetzZeile;

namespace MBBruecke
{
	// Position/Geschwindigkeit: Kern (m) -> Unreal (cm)
	inline FVector NachUnreal(const MB::FVek3& Wert)
	{
		return FVector(Wert.Y * 100.0, Wert.X * 100.0, Wert.Z * 100.0);
	}

	// Position/Geschwindigkeit: Unreal (cm) -> Kern (m)
	inline MB::FVek3 NachKern(const FVector& Wert)
	{
		return MB::FVek3(Wert.Y / 100.0, Wert.X / 100.0, Wert.Z / 100.0);
	}

	// Drehachse fuer die Darstellung: Achsvektoren wechseln bei der Spiegelung das Vorzeichen.
	// Liefert die Unreal-Achse und den Drehwinkel (rad) fuer die Zeit Dt.
	void DrehungFuerDarstellung(const MB::FVek3& Drall, double Dt, FVector& OutAchse, double& OutWinkel);

	MB::FBelagParameter Belag(const FMBBelagZeile& Zeile, const FName& Name);
	MB::FBallParameter Ball(const FMBBallZeile& Zeile);
	MB::FNetzParameter Netz(const FMBNetzZeile& Zeile);
}
