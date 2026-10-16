// Matchball Tennis - GameInstance: laedt die Data Tables und stellt die Kern-Parameter bereit
//
// Die Tabellen entstehen aus Data/*.csv (Tools/Python/import_daten.py) unter /Game/Data/DT_<Name>.
// Fehlt eine Tabelle (z. B. vor dem ersten Lauf von Tools/assets.ps1), werden die Standardwerte des
// Kerns benutzt und eine Warnung ins Log geschrieben.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"

#include "Physics/Kern/MBBallPhysik.h"
#include "Physics/Kern/MBSchlagberechnung.h"

#include "MBGameInstance.generated.h"

class UDataTable;
struct FMBSchlagartZeile;
struct FMBBallmaschineZeile;
struct FMBTimingZeile;

namespace MBVersion
{
	// Versionszaehlung der Unreal-Fassung (README: Start 1.0); release.ps1 erhoeht sie
	constexpr const TCHAR* Version = TEXT("1.0.0");
}

UCLASS()
class MATCHBALLTENNIS_API UMBGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	static UMBGameInstance* Hole(const UObject* WeltKontext);

	// "Version 1.0.0 - Build <id>" (Build-Id aus build.json neben der .exe, sonst "Entwicklung")
	static FString VersionText();

	MB::FBallParameter GetBallParameter() const;
	MB::FNetzParameter GetNetzParameter() const;
	MB::FBelagParameter GetBelag(FName Name) const;

	// Liefert nullptr, wenn Tabelle oder Zeile fehlen
	const FMBSchlagartZeile* GetSchlagart(FName Name) const;
	const FMBBallmaschineZeile* GetBallmaschinenProgramm(FName Name) const;
	const FMBTimingZeile* GetTiming() const;

	// Streu-Parameter und Modifikatoren aus Data/timing.csv
	void HoleStreuParameter(MB::FStreuParameter& OutGrund, MB::FStreuModifikatoren& OutModifikatoren) const;

	UDataTable* GetBallmaschinenTabelle() const { return Ballmaschine; }
	UDataTable* GetBelagTabelle() const { return Belaege; }

private:
	UDataTable* LadeTabelle(const TCHAR* Name) const;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> Belaege;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> Ball;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> Netz;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> Schlagarten;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> Timing;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> Ballmaschine;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> Ansagen;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> Spieler;
};
