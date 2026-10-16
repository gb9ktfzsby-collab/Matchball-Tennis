// Matchball Tennis - Spieler-Controller

#include "Player/MBSpielerController.h"

#include "Camera/MBTVKamera.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/GameUserSettings.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "Match/MBTrainingGameMode.h"
#include "Misc/App.h"
#include "Player/MBEingabe.h"
#include "Player/MBTestSpieler.h"

AMBSpielerController::AMBSpielerController()
{
	// Die Kamera steuert der Spielmodus (TV-Kamera), nicht die Spielfigur
	bAutoManageActiveCameraTarget = false;
	bShowMouseCursor = false;
}

UMBEingabe* AMBSpielerController::SichereEingabe()
{
	if (Eingabe == nullptr)
	{
		Eingabe = NewObject<UMBEingabe>(this, TEXT("Eingabe"));
		Eingabe->Erzeuge();
		// Pause muss auch im pausierten Spiel funktionieren
		Eingabe->Pause->bTriggerWhenPaused = true;
		Eingabe->Vollbild->bTriggerWhenPaused = true;
	}
	return Eingabe;
}

void AMBSpielerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(SichereEingabe()->MatchKontext, 0);
		}
		SetInputMode(FInputModeGameOnly());
		if (const AMBTrainingGameMode* Modus = GetWorld()->GetAuthGameMode<AMBTrainingGameMode>())
		{
			if (Modus->GetKamera() != nullptr)
			{
				SetViewTarget(Modus->GetKamera());
			}
		}
	}
}

void AMBSpielerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	UEnhancedInputComponent* Komponente = Cast<UEnhancedInputComponent>(InputComponent);
	if (Komponente == nullptr)
	{
		return;
	}
	UMBEingabe* E = SichereEingabe();
	Komponente->BindAction(E->Bewegen, ETriggerEvent::Triggered, this, &AMBSpielerController::Bewegen);
	Komponente->BindAction(E->Bewegen, ETriggerEvent::Completed, this, &AMBSpielerController::BewegenEnde);
	Komponente->BindAction(E->Flach, ETriggerEvent::Started, this, &AMBSpielerController::SchlagFlach);
	Komponente->BindAction(E->Topspin, ETriggerEvent::Started, this, &AMBSpielerController::SchlagTopspin);
	Komponente->BindAction(E->Slice, ETriggerEvent::Started, this, &AMBSpielerController::SchlagSlice);
	Komponente->BindAction(E->Lob, ETriggerEvent::Started, this, &AMBSpielerController::SchlagLob);
	Komponente->BindAction(E->Stopp, ETriggerEvent::Started, this, &AMBSpielerController::SchlagStopp);
	Komponente->BindAction(E->Power, ETriggerEvent::Started, this, &AMBSpielerController::PowerAn);
	Komponente->BindAction(E->Power, ETriggerEvent::Completed, this, &AMBSpielerController::PowerAus);
	Komponente->BindAction(E->Praezision, ETriggerEvent::Started, this, &AMBSpielerController::PraezisionAn);
	Komponente->BindAction(E->Praezision, ETriggerEvent::Completed, this, &AMBSpielerController::PraezisionAus);
	Komponente->BindAction(E->Kamera, ETriggerEvent::Started, this, &AMBSpielerController::KameraWechseln);
	Komponente->BindAction(E->Ton, ETriggerEvent::Started, this, &AMBSpielerController::TonUmschalten);
	Komponente->BindAction(E->Pause, ETriggerEvent::Started, this, &AMBSpielerController::PauseUmschalten);
	Komponente->BindAction(E->Vollbild, ETriggerEvent::Started, this, &AMBSpielerController::VollbildUmschalten);
	Komponente->BindAction(E->Challenge, ETriggerEvent::Started, this, &AMBSpielerController::Challenge);
	Komponente->BindAction(E->Ueberspringen, ETriggerEvent::Started, this, &AMBSpielerController::Ueberspringen);
	Komponente->BindAction(E->NaechstesProgramm, ETriggerEvent::Started, this, &AMBSpielerController::NaechstesProgramm);
	Komponente->BindAction(E->NaechsterBelag, ETriggerEvent::Started, this, &AMBSpielerController::NaechsterBelag);
}

void AMBSpielerController::Bewegen(const FInputActionValue& Wert)
{
	if (AMBTestSpieler* Spieler = Cast<AMBTestSpieler>(GetPawn()))
	{
		Spieler->SetzeEingabe(Wert.Get<FVector2D>());
	}
}

void AMBSpielerController::BewegenEnde(const FInputActionValue& Wert)
{
	if (AMBTestSpieler* Spieler = Cast<AMBTestSpieler>(GetPawn()))
	{
		Spieler->SetzeEingabe(FVector2D::ZeroVector);
	}
}

void AMBSpielerController::Schlag(int32 Art)
{
	AMBTrainingGameMode* Modus = GetWorld()->GetAuthGameMode<AMBTrainingGameMode>();
	AMBTestSpieler* Spieler = Cast<AMBTestSpieler>(GetPawn());
	if (Modus == nullptr || Spieler == nullptr)
	{
		return;
	}
	Modus->Schlage(Spieler, static_cast<EMBSchlagWunsch>(Art), bPower, bPraezision);
}

void AMBSpielerController::SchlagFlach()
{
	Schlag(static_cast<int32>(EMBSchlagWunsch::Flach));
}

void AMBSpielerController::SchlagTopspin()
{
	Schlag(static_cast<int32>(EMBSchlagWunsch::Topspin));
}

void AMBSpielerController::SchlagSlice()
{
	Schlag(static_cast<int32>(EMBSchlagWunsch::Slice));
}

void AMBSpielerController::SchlagLob()
{
	Schlag(static_cast<int32>(EMBSchlagWunsch::Lob));
}

void AMBSpielerController::SchlagStopp()
{
	Schlag(static_cast<int32>(EMBSchlagWunsch::Stopp));
}

void AMBSpielerController::KameraWechseln()
{
	if (AMBTrainingGameMode* Modus = GetWorld()->GetAuthGameMode<AMBTrainingGameMode>())
	{
		Modus->KameraWechseln();
	}
}

void AMBSpielerController::TonUmschalten()
{
	bTonAus = !bTonAus;
	FApp::SetVolumeMultiplier(bTonAus ? 0.0f : 1.0f);
}

void AMBSpielerController::PauseUmschalten()
{
	// Online laeuft das Match bei Esc weiter (README 15.2) - das gilt ab M9; offline wird pausiert
	UGameplayStatics::SetGamePaused(this, !UGameplayStatics::IsGamePaused(this));
}

void AMBSpielerController::VollbildUmschalten()
{
	if (GEngine == nullptr)
	{
		return;
	}
	if (UGameUserSettings* Einstellungen = GEngine->GetGameUserSettings())
	{
		const EWindowMode::Type Neu = Einstellungen->GetFullscreenMode() == EWindowMode::Windowed ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed;
		Einstellungen->SetFullscreenMode(Neu);
		Einstellungen->ApplySettings(false);
	}
}

void AMBSpielerController::Challenge()
{
	// Im Training gibt es keine Linienrichter; die Challenge folgt mit dem Match (M3)
}

void AMBSpielerController::Ueberspringen()
{
	// Zwischensequenzen gibt es im Training noch nicht (M3/M5)
}

void AMBSpielerController::NaechstesProgramm()
{
	if (AMBTrainingGameMode* Modus = GetWorld()->GetAuthGameMode<AMBTrainingGameMode>())
	{
		Modus->NaechstesProgramm();
	}
}

void AMBSpielerController::NaechsterBelag()
{
	if (AMBTrainingGameMode* Modus = GetWorld()->GetAuthGameMode<AMBTrainingGameMode>())
	{
		Modus->NaechsterBelag();
	}
}
