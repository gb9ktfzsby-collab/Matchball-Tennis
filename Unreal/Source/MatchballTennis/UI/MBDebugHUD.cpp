// Matchball Tennis - Entwicklungs-HUD fuer Meilenstein M1

#include "UI/MBDebugHUD.h"

#include "Camera/MBTVKamera.h"
#include "Core/MBGameInstance.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "Match/MBTrainingGameMode.h"

void AMBDebugHUD::Zeile(const FString& Text, float X, float& Y, const FLinearColor& Farbe, float Skalierung)
{
	UFont* Schrift = GEngine != nullptr ? GEngine->GetMediumFont() : nullptr;
	DrawText(Text, Farbe, X, Y, Schrift, Skalierung);
	Y += 22.0f * Skalierung;
}

void AMBDebugHUD::DrawHUD()
{
	Super::DrawHUD();
	if (Canvas == nullptr)
	{
		return;
	}
	const AMBTrainingGameMode* Modus = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<AMBTrainingGameMode>() : nullptr;
	const float Skalierung = FMath::Max(0.75f, Canvas->ClipY / 1080.0f);
	const FLinearColor Weiss(0.95f, 0.95f, 0.95f);
	const FLinearColor Gelb(1.0f, 0.85f, 0.2f);
	const FLinearColor Grau(0.7f, 0.7f, 0.7f);
	float Y = 24.0f * Skalierung;
	const float X = 24.0f * Skalierung;

	Zeile(TEXT("MATCHBALL TENNIS - Training (M1, Entwicklungsstand)"), X, Y, Gelb, Skalierung);
	if (Modus == nullptr)
	{
		Zeile(TEXT("Kein Trainingsmodus aktiv"), X, Y, Weiss, Skalierung);
		return;
	}
	const FMBTrainingAnzeige& A = Modus->GetAnzeige();
	Zeile(FString::Printf(TEXT("Belag: %s  (Court Pace Rating %.1f)"), *A.Belag, A.CourtPace), X, Y, Weiss, Skalierung);
	Zeile(FString::Printf(TEXT("Ballmaschine: %s"), *A.Programm), X, Y, Weiss, Skalierung);
	Zeile(FString::Printf(TEXT("Ball: %.0f km/h, %.0f U/min, Hoehe %.2f m, letzter Absprung %.0f km/h"), A.BallTempoKmh, A.BallDrallUpm, A.BallHoehe, A.LetzterAufsprungKmh), X, Y, Weiss, Skalierung);
	Zeile(FString::Printf(TEXT("Schlaege: %d, davon im Feld: %d"), A.Schlaege, A.Treffer), X, Y, Weiss, Skalierung);
	if (!A.LetzterSchlag.IsEmpty())
	{
		Zeile(FString::Printf(TEXT("Letzter Schlag: %s"), *A.LetzterSchlag), X, Y, Weiss, Skalierung);
	}
	if (!A.LetztesUrteil.IsEmpty())
	{
		Zeile(FString::Printf(TEXT("Hawk-Eye: %s"), *A.LetztesUrteil), X, Y, Gelb, Skalierung);
	}
	if (A.bBallInReichweite)
	{
		Zeile(TEXT(">> Ball in Reichweite <<"), X, Y, FLinearColor(0.3f, 1.0f, 0.3f), Skalierung);
	}

	// Tastenbelegung unten links
	float Unten = Canvas->ClipY - 5.0f * 22.0f * Skalierung - 16.0f * Skalierung;
	Zeile(TEXT("Laufen/Zielen: WASD / Pfeile / linker Stick   Schlag: J Flach, K Topspin, L Slice, I Lob, U Stopp"), X, Unten, Grau, Skalierung);
	Zeile(TEXT("Power: Shift/Q (RT)   Praezision: Leertaste/E (LT)   Kamera: C   Vollbild: F11   Pause: Esc/P"), X, Unten, Grau, Skalierung);
	Zeile(TEXT("Entwicklung: F5 naechstes Ballmaschinen-Programm, F6 naechster Belag"), X, Unten, Grau, Skalierung);
	const AMBTVKamera* Kamera = Modus->GetKamera();
	const TCHAR* KameraName = TEXT("TV");
	if (Kamera != nullptr)
	{
		switch (Kamera->GetModus())
		{
		case EMBKameraModus::TV: KameraName = TEXT("TV"); break;
		case EMBKameraModus::TVHoch: KameraName = TEXT("TV hoch"); break;
		case EMBKameraModus::Spieler: KameraName = TEXT("Spieler"); break;
		case EMBKameraModus::Seite: KameraName = TEXT("Seite"); break;
		}
	}
	Zeile(FString::Printf(TEXT("Kamera: %s   |   %s"), KameraName, *UMBGameInstance::VersionText()), X, Unten, Grau, Skalierung);
}
