// Matchball Tennis - Eingabe

#include "Player/MBEingabe.h"

#include "EnhancedActionKeyMapping.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

UInputAction* UMBEingabe::NeueAktion(const TCHAR* Name, bool bAchse2D)
{
	UInputAction* Aktion = NewObject<UInputAction>(this, FName(Name));
	Aktion->ValueType = bAchse2D ? EInputActionValueType::Axis2D : EInputActionValueType::Boolean;
	return Aktion;
}

void UMBEingabe::Taste(UInputAction* Aktion, const FKey& Knopf)
{
	MatchKontext->MapKey(Aktion, Knopf);
}

void UMBEingabe::Richtungstaste(const FKey& Knopf, bool bAufY, bool bNegativ)
{
	FEnhancedActionKeyMapping& Zuordnung = MatchKontext->MapKey(Bewegen, Knopf);
	if (bAufY)
	{
		// Tastenwert liegt auf X - auf Y umlegen
		UInputModifierSwizzleAxis* Tausch = NewObject<UInputModifierSwizzleAxis>(this);
		Tausch->Order = EInputAxisSwizzle::YXZ;
		Zuordnung.Modifiers.Add(Tausch);
	}
	if (bNegativ)
	{
		Zuordnung.Modifiers.Add(NewObject<UInputModifierNegate>(this));
	}
}

void UMBEingabe::Erzeuge()
{
	MatchKontext = NewObject<UInputMappingContext>(this, TEXT("IMC_Match"));

	Bewegen = NeueAktion(TEXT("IA_Bewegen"), true);
	Flach = NeueAktion(TEXT("IA_Flach"), false);
	Topspin = NeueAktion(TEXT("IA_Topspin"), false);
	Slice = NeueAktion(TEXT("IA_Slice"), false);
	Lob = NeueAktion(TEXT("IA_Lob"), false);
	Stopp = NeueAktion(TEXT("IA_Stopp"), false);
	Power = NeueAktion(TEXT("IA_Power"), false);
	Praezision = NeueAktion(TEXT("IA_Praezision"), false);
	Challenge = NeueAktion(TEXT("IA_Challenge"), false);
	Kamera = NeueAktion(TEXT("IA_Kamera"), false);
	Ton = NeueAktion(TEXT("IA_Ton"), false);
	Pause = NeueAktion(TEXT("IA_Pause"), false);
	Vollbild = NeueAktion(TEXT("IA_Vollbild"), false);
	Ueberspringen = NeueAktion(TEXT("IA_Ueberspringen"), false);
	NaechstesProgramm = NeueAktion(TEXT("IA_NaechstesProgramm"), false);
	NaechsterBelag = NeueAktion(TEXT("IA_NaechsterBelag"), false);

	// Laufen / Zielen: Tastatur
	Richtungstaste(EKeys::W, true, false);
	Richtungstaste(EKeys::S, true, true);
	Richtungstaste(EKeys::D, false, false);
	Richtungstaste(EKeys::A, false, true);
	Richtungstaste(EKeys::Up, true, false);
	Richtungstaste(EKeys::Down, true, true);
	Richtungstaste(EKeys::Right, false, false);
	Richtungstaste(EKeys::Left, false, true);
	// Controller: linker Stick (2D) und Steuerkreuz
	MatchKontext->MapKey(Bewegen, EKeys::Gamepad_Left2D);
	Richtungstaste(EKeys::Gamepad_DPad_Up, true, false);
	Richtungstaste(EKeys::Gamepad_DPad_Down, true, true);
	Richtungstaste(EKeys::Gamepad_DPad_Right, false, false);
	Richtungstaste(EKeys::Gamepad_DPad_Left, false, true);

	// Schlaege (README 5.1 / 5.2)
	Taste(Flach, EKeys::J);
	Taste(Flach, EKeys::Gamepad_FaceButton_Bottom);
	Taste(Topspin, EKeys::K);
	Taste(Topspin, EKeys::Gamepad_FaceButton_Right);
	Taste(Slice, EKeys::L);
	Taste(Slice, EKeys::Gamepad_FaceButton_Left);
	Taste(Lob, EKeys::I);
	Taste(Lob, EKeys::Gamepad_FaceButton_Top);
	Taste(Stopp, EKeys::U);
	Taste(Stopp, EKeys::Gamepad_LeftShoulder);
	Taste(Stopp, EKeys::Gamepad_RightShoulder);

	// Power / Praezision (gehalten)
	Taste(Power, EKeys::LeftShift);
	Taste(Power, EKeys::Q);
	Taste(Power, EKeys::Gamepad_RightTrigger);
	Taste(Praezision, EKeys::SpaceBar);
	Taste(Praezision, EKeys::E);
	Taste(Praezision, EKeys::Gamepad_LeftTrigger);

	// Nach dem Punkt: Challenge (H bzw. Y - Y ist im Ballwechsel Lob, das Spiel unterscheidet nach Zustand)
	Taste(Challenge, EKeys::H);
	Taste(Challenge, EKeys::Gamepad_FaceButton_Top);

	Taste(Kamera, EKeys::C);
	Taste(Kamera, EKeys::Gamepad_RightThumbstick);
	Taste(Ton, EKeys::M);
	Taste(Pause, EKeys::Escape);
	Taste(Pause, EKeys::P);
	Taste(Pause, EKeys::Gamepad_Special_Right);
	Taste(Vollbild, EKeys::F11);
	Taste(Ueberspringen, EKeys::Enter);
	Taste(Ueberspringen, EKeys::Gamepad_Special_Left);

	// Entwicklung (M1)
	Taste(NaechstesProgramm, EKeys::F5);
	Taste(NaechsterBelag, EKeys::F6);
}
