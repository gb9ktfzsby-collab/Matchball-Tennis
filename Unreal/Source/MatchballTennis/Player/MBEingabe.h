// Matchball Tennis - Eingabe (README 5.1, 5.2, 2.2 "Enhanced Input")
//
// Alle Input Actions und Mapping Contexts werden zur Laufzeit in C++ angelegt - es gibt keine
// handgebauten .uasset-Dateien (README 0.2). Kontexte: Match (Laufen, Schlaege, Kamera ...) und
// Menue folgt mit M6 (CommonUI).
//
// Tastatur: WASD/Pfeile laufen und zielen, J Flach, K Topspin, L Slice, I Lob, U Stopp,
// Shift/Q Power, Leertaste/E Praezision, H Challenge, C Kamera, M Ton, Esc/P Pause, F11 Vollbild.
// Controller: linker Stick/Steuerkreuz, A Flach, B Topspin, X Slice, Y Lob/Challenge, LB/RB Stopp,
// RT Power, LT Praezision, rechter Stick klicken Kamera, Start Pause.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "MBEingabe.generated.h"

class UInputAction;
class UInputMappingContext;
struct FKey;

UCLASS()
class MATCHBALLTENNIS_API UMBEingabe : public UObject
{
	GENERATED_BODY()

public:
	// Legt alle Aktionen und den Match-Kontext an
	void Erzeuge();

	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> MatchKontext;

	UPROPERTY(Transient) TObjectPtr<UInputAction> Bewegen;        // Axis2D: X = rechts, Y = vor
	UPROPERTY(Transient) TObjectPtr<UInputAction> Flach;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Topspin;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Slice;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Lob;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Stopp;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Power;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Praezision;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Challenge;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Kamera;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Ton;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Pause;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Vollbild;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Ueberspringen;
	// Nur Entwicklung (M1): Ballmaschinen-Programm und Belag wechseln
	UPROPERTY(Transient) TObjectPtr<UInputAction> NaechstesProgramm;
	UPROPERTY(Transient) TObjectPtr<UInputAction> NaechsterBelag;

private:
	UInputAction* NeueAktion(const TCHAR* Name, bool bAchse2D);
	void Taste(UInputAction* Aktion, const FKey& Knopf);
	// WASD/Pfeile/Steuerkreuz als 2D-Achse: Richtung (X, Y) je Taste
	void Richtungstaste(const FKey& Knopf, bool bAufY, bool bNegativ);
};
