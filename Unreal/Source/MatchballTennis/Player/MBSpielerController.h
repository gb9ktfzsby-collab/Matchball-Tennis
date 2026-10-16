// Matchball Tennis - Spieler-Controller: Eingabe, Kamera, Pause, Vollbild (README 5.1, 5.2, 19.1)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "MBSpielerController.generated.h"

class UMBEingabe;
struct FInputActionValue;

UCLASS()
class MATCHBALLTENNIS_API AMBSpielerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMBSpielerController();

	bool IstPowerGehalten() const { return bPower; }
	bool IstPraezisionGehalten() const { return bPraezision; }
	bool IstTonAus() const { return bTonAus; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	UMBEingabe* SichereEingabe();

	void Bewegen(const FInputActionValue& Wert);
	void BewegenEnde(const FInputActionValue& Wert);
	void SchlagFlach();
	void SchlagTopspin();
	void SchlagSlice();
	void SchlagLob();
	void SchlagStopp();
	void Schlag(int32 Art);
	void PowerAn() { bPower = true; }
	void PowerAus() { bPower = false; }
	void PraezisionAn() { bPraezision = true; }
	void PraezisionAus() { bPraezision = false; }
	void KameraWechseln();
	void TonUmschalten();
	void PauseUmschalten();
	void VollbildUmschalten();
	void Challenge();
	void Ueberspringen();
	void NaechstesProgramm();
	void NaechsterBelag();

	UPROPERTY(Transient)
	TObjectPtr<UMBEingabe> Eingabe;

	bool bPower = false;
	bool bPraezision = false;
	bool bTonAus = false;
};
