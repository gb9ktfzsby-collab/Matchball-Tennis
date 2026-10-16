// Matchball Tennis - Entwicklungs-HUD fuer Meilenstein M1
//
// Zeigt Belag (mit Court Pace Rating), Ballmaschinen-Programm, Balltempo/-drall, das Hawk-Eye-Urteil
// des letzten Schlags und die Tastenbelegung. Das echte TV-HUD (Spielstand, Timing-Meter,
// Challenges) entsteht mit CommonUI/UMG in M2/M6.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"

#include "MBDebugHUD.generated.h"

UCLASS()
class MATCHBALLTENNIS_API AMBDebugHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void Zeile(const FString& Text, float X, float& Y, const FLinearColor& Farbe, float Skalierung);
};
