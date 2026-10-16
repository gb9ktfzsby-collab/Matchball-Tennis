// Matchball Tennis - Graubox-Platz mit exakten Massen (Meilenstein M1, README 12.1 und 24.1)
//
// Baut Belag, alle Linien (Grundlinie 10 cm, uebrige 5 cm, nach innen ab Aussenkante), Netz mit
// Durchhang (0,914 m Mitte / 1,07 m an den Einzelstuetzen), Pfosten und Einzelstuetzen aus Engine-
// Grundformen. Alle Masse kommen aus Physics/Kern/MBPlatz.h - dieselben Zahlen wie in der Physik.
// Wird mit M4 durch die Blender-Anlagen (Tools/Python/build_venue.py) ersetzt.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "MBPlatzGraubox.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;

UCLASS()
class MATCHBALLTENNIS_API AMBPlatzGraubox : public AActor
{
	GENERATED_BODY()

public:
	AMBPlatzGraubox();

	// Belag bestimmt die Farben (Hart, Sand, Rasen, Halle)
	void Baue(FName Belag);

protected:
	virtual void BeginPlay() override;

private:
	// Quader in Kern-Koordinaten (m): Mitte, Groesse
	UStaticMeshComponent* Quader(const FString& Name, double MitteX, double MitteY, double MitteZ, double BreiteX, double LaengeY, double HoeheZ, const FLinearColor& Farbe);
	UStaticMeshComponent* Zylinder(const FString& Name, double MitteX, double MitteY, double Hoehe, double Durchmesser, const FLinearColor& Farbe);

	UPROPERTY(VisibleAnywhere, Category = "Platz")
	TObjectPtr<USceneComponent> Wurzel;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Teile;

	UPROPERTY()
	TObjectPtr<UStaticMesh> QuaderMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> ZylinderMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> Grundmaterial;

	bool bGebaut = false;
};
