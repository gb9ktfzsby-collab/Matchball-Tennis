// Matchball Tennis - Platzhalter-Spieler fuer Meilenstein M1
//
// Ein Zylinder mit Schlaeger-Andeutung, der seitlich/vorwaerts laeuft und Baelle in Reichweite
// zurueckschlagen kann. Die echte Figur (Skelett, Motion Matching, Schlag-Montagen mit Warping,
// Timing-Meter) folgt mit M2. Die Bewegungs- und Reichweitenwerte sind Startwerte (Dokumente/Werte.md).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

#include "Core/Kern/MBMathe.h"
#include "Physics/Kern/MBPlatz.h"

#include "MBTestSpieler.generated.h"

class UStaticMeshComponent;
class USceneComponent;

UCLASS()
class MATCHBALLTENNIS_API AMBTestSpieler : public APawn
{
	GENERATED_BODY()

public:
	AMBTestSpieler();

	virtual void Tick(float DeltaSeconds) override;

	// Stick/WASD: X = rechts, Y = vor (aus Sicht des Spielers)
	void SetzeEingabe(const FVector2D& InEingabe) { Eingabe = InEingabe; }
	FVector2D GetEingabe() const { return Eingabe; }

	MB::FVek3 GetKernPosition() const;
	void SetzeKernPosition(const MB::FVek3& Position);

	MB::EPlatzseite GetSeite() const { return Seite; }

	// Laufgeschwindigkeit (m/s) - spaeter aus dem Spielerwert "Schnelligkeit"
	UPROPERTY(EditAnywhere, Category = "Spieler")
	double Laufgeschwindigkeit = 6.0;

	// Waagerechte Reichweite bis zur Ballmitte (m)
	UPROPERTY(EditAnywhere, Category = "Spieler")
	double Reichweite = 1.6;

	// Spielerwert (0-99) fuer Schlagtempo und Streuung
	UPROPERTY(EditAnywhere, Category = "Spieler")
	double Spielerwert = 80.0;

	UPROPERTY(EditAnywhere, Category = "Spieler")
	bool bLinkshaender = false;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Spieler")
	TObjectPtr<USceneComponent> Wurzel;

	UPROPERTY(VisibleAnywhere, Category = "Spieler")
	TObjectPtr<UStaticMeshComponent> Koerper;

	UPROPERTY(VisibleAnywhere, Category = "Spieler")
	TObjectPtr<UStaticMeshComponent> Schlaeger;

private:
	FVector2D Eingabe = FVector2D::ZeroVector;
	MB::EPlatzseite Seite = MB::EPlatzseite::Nah;
};
