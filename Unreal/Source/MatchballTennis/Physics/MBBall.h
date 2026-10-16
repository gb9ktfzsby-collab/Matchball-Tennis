// Matchball Tennis - sichtbarer Ball
//
// Reine Darstellung: Position und Drall kommen aus der deterministischen Kern-Simulation
// (Physics/Kern/MBBallPhysik). Keine Chaos-Physik, keine Kollision (README 2.2).
// Bis das Blender-Modell da ist (M4), dient die Engine-Kugel als Platzhalter.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Core/Kern/MBMathe.h"

#include "MBBall.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS()
class MATCHBALLTENNIS_API AMBBall : public AActor
{
	GENERATED_BODY()

public:
	AMBBall();

	// Stellt den Ball an die Kern-Position (m) und dreht ihn um den Drall fuer die Zeit Dt
	void SetzeKernZustand(const MB::FVek3& Position, const MB::FVek3& Drall, double Dt);

	// Durchmesser in Metern (aus Data/ball.csv)
	void SetzeDurchmesser(double Meter);

	void SetzeSichtbar(bool bSichtbar);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Ball")
	TObjectPtr<UStaticMeshComponent> Kugel;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;
};
