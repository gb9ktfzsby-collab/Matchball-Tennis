// Matchball Tennis - Ballmaschine auf der Trainingsanlage (README 13.2)
//
// Einstellungen je Programm aus Data/ballmaschine.csv: Tempo, Drall, Hoehe, Position
// (Vorhand/Rueckhand/abwechselnd/zufaellig), Zieltiefe und Takt. Der Abschuss wird mit der
// Kern-Schlagberechnung so bestimmt, dass der Ball an der gewuenschten Stelle aufspringt.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Core/Kern/MBZufall.h"
#include "Physics/Kern/MBBallPhysik.h"

#include "MBBallmaschine.generated.h"

class UStaticMeshComponent;
struct FMBBallmaschineZeile;

UCLASS()
class MATCHBALLTENNIS_API AMBBallmaschine : public AActor
{
	GENERATED_BODY()

public:
	AMBBallmaschine();

	// Programm setzen (Zeile aus DT_Ballmaschine); ohne Tabelle gelten Standardwerte
	void SetzeProgramm(FName Name, const FMBBallmaschineZeile* Zeile);
	FName GetProgramm() const { return Programm; }
	FString GetProgrammName() const { return Anzeigename; }
	double GetTakt() const { return Takt; }

	// Naechsten Ball berechnen. Rechtshaender auf der nahen Seite: Vorhand = rechts (+X im Kern).
	MB::FBallZustand NaechsterBall(const MB::FBallSimulation& Vorlage, bool bSpielerLinkshaender);

	// Position der Maschine im Kern (m): hinter der fernen Grundlinie, Mitte
	MB::FVek3 GetKernPosition() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Ballmaschine")
	TObjectPtr<UStaticMeshComponent> Gehaeuse;

	UPROPERTY(VisibleAnywhere, Category = "Ballmaschine")
	TObjectPtr<UStaticMeshComponent> Rohr;

private:
	FName Programm = TEXT("Grundschlag");
	FString Anzeigename = TEXT("Grundschlag (abwechselnd)");
	double Tempo = 80.0;
	double Drall = 1500.0;
	double Hoehe = 1.0;
	FString Position = TEXT("Abwechselnd");
	double ZielTiefe = 8.5;
	double Takt = 3.0;
	bool bHoherBogen = false;

	bool bNaechsterVorhand = true;
	MB::FZufall Zufall;
};
