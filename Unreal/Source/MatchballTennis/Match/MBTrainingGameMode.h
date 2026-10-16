// Matchball Tennis - Trainingsmodus mit Ballmaschine (Meilenstein M1, README 13.2)
//
// Besitzt die deterministische Ballsimulation (fester Zeitschritt 240 Hz, unabhaengig von der Bildrate)
// und verbindet sie mit Darstellung (Ball, Graubox-Platz), Ballmaschine, Testspieler und TV-Kamera.
// Zum Ausprobieren der Physik: Ballmaschine spielt zu, Spieler schlaegt mit J/K/L/I/U zurueck.
//
// Startoptionen (Kommandozeile oder "open M_Graubox?Belag=Sand?Programm=Topspin"):
//   Belag = Hart | Sand | Rasen | Halle      Programm = Zeile aus Data/ballmaschine.csv
// Entwicklungstasten: F5 naechstes Programm, F6 naechster Belag.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "Core/Kern/MBZufall.h"
#include "Physics/Kern/MBBallPhysik.h"
#include "Physics/Kern/MBKontaktflaeche.h"
#include "Physics/Kern/MBVorhersage.h"

#include "MBTrainingGameMode.generated.h"

class AMBBall;
class AMBBallmaschine;
class AMBPlatzGraubox;
class AMBTestSpieler;
class AMBTVKamera;

UENUM(BlueprintType)
enum class EMBSchlagWunsch : uint8
{
	Flach,
	Topspin,
	Slice,
	Lob,
	Stopp
};

// Anzeige-Werte fuer das HUD
struct FMBTrainingAnzeige
{
	FString Belag;
	double CourtPace = 0.0;
	FString Programm;
	int32 Schlaege = 0;
	int32 Treffer = 0;              // Rueckschlaege, die im Feld gelandet sind
	FString LetzterSchlag;          // z. B. "Topspin 118 km/h, 2600 U/min, Netz +0,55 m"
	FString LetztesUrteil;          // z. B. "IN (12 mm) - Grundlinie"
	double BallTempoKmh = 0.0;
	double BallDrallUpm = 0.0;
	double BallHoehe = 0.0;
	double LetzterAufsprungKmh = 0.0;
	bool bBallInReichweite = false;
};

UCLASS()
class MATCHBALLTENNIS_API AMBTrainingGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMBTrainingGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

	// Schlagversuch des Spielers (Taste gedrueckt). Liefert false, wenn der Ball nicht erreichbar ist.
	bool Schlage(AMBTestSpieler* Spieler, EMBSchlagWunsch Wunsch, bool bPower, bool bPraezision);

	void NaechstesProgramm();
	void NaechsterBelag();
	void KameraWechseln();

	const FMBTrainingAnzeige& GetAnzeige() const { return Anzeige; }
	AMBTVKamera* GetKamera() const { return Kamera; }

	UPROPERTY(EditAnywhere, Category = "Training")
	FName Belag = TEXT("Hart");

	UPROPERTY(EditAnywhere, Category = "Training")
	FName Programm = TEXT("Grundschlag");

	// Vorhersage-Markierungen (Aufsprungpunkte) anzeigen - nur Entwicklungs-Builds
	UPROPERTY(EditAnywhere, Category = "Training")
	bool bVorhersageZeigen = true;

private:
	void ErzeugeWelt();
	void StarteBall(const MB::FBallZustand& Start, int32 Schlagender);
	void SimulationsSchritt();
	void PruefeBallmaschine(double Dt);
	void AktualisiereDarstellung(double Alpha, double Dt);
	void ZeichneVorhersage() const;
	void WerteAufsprungAus(const MB::FBallEreignis& Ereignis);
	FName SchlagartZeile(EMBSchlagWunsch Wunsch, bool bPower, const MB::FVek3& BallPosition, const MB::FVek3& Spieler, bool bVorAufsprung) const;

	MB::FBallSimulation Simulation;
	MB::FBallZustand VorherZustand;
	double Akkumulator = 0.0;
	int32 LetzterSchlagender = -1;            // -1 Maschine, 0 Spieler
	int32 AufspruengeSeitSchlag = 0;
	double ZeitSeitSchuss = 0.0;
	double ZeitSeitAufsprungFern = -1.0;
	bool bBallAktiv = false;
	MB::FFlugbahn Vorhersage;
	MB::FZufall Zufall;
	TArray<FName> Programme;
	TArray<FName> Belaege;

	UPROPERTY(Transient)
	TObjectPtr<AMBBall> Ball;

	UPROPERTY(Transient)
	TObjectPtr<AMBPlatzGraubox> Platz;

	UPROPERTY(Transient)
	TObjectPtr<AMBBallmaschine> Maschine;

	UPROPERTY(Transient)
	TObjectPtr<AMBTVKamera> Kamera;

	FMBTrainingAnzeige Anzeige;
};
