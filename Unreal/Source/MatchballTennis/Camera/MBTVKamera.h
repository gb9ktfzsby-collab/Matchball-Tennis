// Matchball Tennis - Fernsehkamera (README 5.9)
//
// Standard: TV-Kamera hinter dem eigenen Spieler, leicht erhoeht, folgt seitlich. Laeuft der Spieler weit
// hinter die Grundlinie, faehrt die Kamera in gleicher Hoehe mit zurueck. C wechselt zwischen
// TV, hoher TV-Kamera, Spielerkamera und [NEU] Seitenkamera. Online/Seitenwechsel: die Kamera steht
// immer auf der Seite des eigenen Spielers ("jeder sieht sich selbst vorne").
// Ausblenden von Bande/Tribuene hinter dem Spieler (Dither-Fade) folgt mit den Anlagen (M4).

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"

#include "Physics/Kern/MBPlatz.h"

#include "MBTVKamera.generated.h"

UENUM(BlueprintType)
enum class EMBKameraModus : uint8
{
	TV,
	TVHoch,
	Spieler,
	Seite
};

UCLASS()
class MATCHBALLTENNIS_API AMBTVKamera : public ACameraActor
{
	GENERATED_BODY()

public:
	AMBTVKamera();

	virtual void Tick(float DeltaSeconds) override;

	void Verfolge(AActor* InSpieler, AActor* InBall);
	void SetzeSeite(MB::EPlatzseite InSeite) { Seite = InSeite; }
	void NaechsterModus();
	EMBKameraModus GetModus() const { return Modus; }

	// Geschwindigkeit der Nachfuehrung (hoeher = direkter)
	UPROPERTY(EditAnywhere, Category = "Kamera")
	float Nachfuehrung = 4.0f;

private:
	void BerechneZiel(FVector& OutPosition, FVector& OutBlickpunkt, float& OutSichtfeld) const;

	TWeakObjectPtr<AActor> Spieler;
	TWeakObjectPtr<AActor> Ball;
	EMBKameraModus Modus = EMBKameraModus::TV;
	MB::EPlatzseite Seite = MB::EPlatzseite::Nah;
	FVector GlattePosition = FVector::ZeroVector;
	FVector GlatterBlick = FVector::ZeroVector;
	bool bInitialisiert = false;
};
