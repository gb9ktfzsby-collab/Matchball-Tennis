// Matchball Tennis - Trainingsmodus mit Ballmaschine (Meilenstein M1)

#include "Match/MBTrainingGameMode.h"

#include "Camera/MBTVKamera.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Core/MBDatenTabellen.h"
#include "Core/MBGameInstance.h"
#include "Core/MBKernBruecke.h"
#include "DrawDebugHelpers.h"
#include "Engine/DataTable.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "MatchballTennis.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Physics/Kern/MBPlatz.h"
#include "Physics/Kern/MBSchlagberechnung.h"
#include "Physics/MBBall.h"
#include "Player/MBSpielerController.h"
#include "Player/MBTestSpieler.h"
#include "UI/MBDebugHUD.h"
#include "Venue/MBBallmaschine.h"
#include "Venue/MBPlatzGraubox.h"

namespace
{
	const TCHAR* MBTraining_LinienName(MB::ELinie Linie)
	{
		switch (Linie)
		{
		case MB::ELinie::Grundlinie: return TEXT("Grundlinie");
		case MB::ELinie::SeitenlinieLinks: return TEXT("Seitenlinie links");
		case MB::ELinie::SeitenlinieRechts: return TEXT("Seitenlinie rechts");
		case MB::ELinie::Aufschlaglinie: return TEXT("Aufschlaglinie");
		case MB::ELinie::Mittellinie: return TEXT("Mittellinie");
		case MB::ELinie::Keine: break;
		}
		return TEXT("");
	}
}

AMBTrainingGameMode::AMBTrainingGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	DefaultPawnClass = AMBTestSpieler::StaticClass();
	PlayerControllerClass = AMBSpielerController::StaticClass();
	HUDClass = AMBDebugHUD::StaticClass();
}

void AMBTrainingGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	// Optionen aus der Map-URL, ersatzweise von der Kommandozeile (-Belag=Sand -Programm=Topspin)
	FString Wert = UGameplayStatics::ParseOption(Options, TEXT("Belag"));
	if (Wert.IsEmpty())
	{
		FParse::Value(FCommandLine::Get(), TEXT("Belag="), Wert);
	}
	if (!Wert.IsEmpty())
	{
		Belag = FName(*Wert);
	}
	Wert = UGameplayStatics::ParseOption(Options, TEXT("Programm"));
	if (Wert.IsEmpty())
	{
		FParse::Value(FCommandLine::Get(), TEXT("Programm="), Wert);
	}
	if (!Wert.IsEmpty())
	{
		Programm = FName(*Wert);
	}
	Zufall.Starte(FPlatformTime::Cycles64());
}

void AMBTrainingGameMode::BeginPlay()
{
	Super::BeginPlay();
	ErzeugeWelt();
	// Kamera fuer alle (lokalen) Spieler setzen
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		if (APlayerController* Controller = Iterator->Get())
		{
			Controller->SetViewTarget(Kamera);
		}
	}
}

void AMBTrainingGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	if (Kamera != nullptr && NewPlayer != nullptr)
	{
		NewPlayer->SetViewTarget(Kamera);
	}
}

void AMBTrainingGameMode::ErzeugeWelt()
{
	UWorld* Welt = GetWorld();
	UMBGameInstance* Instanz = UMBGameInstance::Hole(this);

	// Programme und Belaege aus den Tabellen (oder Standard)
	Programme.Reset();
	Belaege.Reset();
	if (Instanz != nullptr && Instanz->GetBallmaschinenTabelle() != nullptr)
	{
		Programme = Instanz->GetBallmaschinenTabelle()->GetRowNames();
	}
	if (Instanz != nullptr && Instanz->GetBelagTabelle() != nullptr)
	{
		Belaege = Instanz->GetBelagTabelle()->GetRowNames();
	}
	if (Programme.Num() == 0)
	{
		Programme.Add(TEXT("Grundschlag"));
	}
	if (Belaege.Num() == 0)
	{
		Belaege = {TEXT("Hart"), TEXT("Sand"), TEXT("Rasen"), TEXT("Halle")};
	}

	// Simulation mit den Werten aus Data/
	const MB::FBallParameter BallWerte = Instanz != nullptr ? Instanz->GetBallParameter() : MB::FBallParameter();
	const MB::FNetzParameter NetzWerte = Instanz != nullptr ? Instanz->GetNetzParameter() : MB::FNetzParameter();
	const MB::FBelagParameter BelagWerte = Instanz != nullptr ? Instanz->GetBelag(Belag) : MB::FBelagParameter();
	Simulation = MB::FBallSimulation(BallWerte, BelagWerte, NetzWerte);

	// Licht, falls die Map keins hat (Rueckfall, z. B. wenn M_Graubox noch nicht erzeugt wurde)
	TActorIterator<ADirectionalLight> LichtSuche(Welt);
	const bool bHatLicht = static_cast<bool>(LichtSuche);
	if (!bHatLicht)
	{
		ADirectionalLight* Sonne = Welt->SpawnActor<ADirectionalLight>(FVector(0.0, 0.0, 2000.0), FRotator(-50.0, 35.0, 0.0));
		if (Sonne != nullptr)
		{
			Sonne->GetLightComponent()->SetMobility(EComponentMobility::Movable);
			if (UDirectionalLightComponent* Licht = Cast<UDirectionalLightComponent>(Sonne->GetLightComponent()))
			{
				Licht->SetAtmosphereSunLight(true);
			}
		}
		Welt->SpawnActor<ASkyAtmosphere>(FVector::ZeroVector, FRotator::ZeroRotator);
		ASkyLight* Himmel = Welt->SpawnActor<ASkyLight>(FVector(0.0, 0.0, 500.0), FRotator::ZeroRotator);
		if (Himmel != nullptr && Himmel->GetLightComponent() != nullptr)
		{
			USkyLightComponent* Himmelslicht = Himmel->GetLightComponent();
			Himmelslicht->SetMobility(EComponentMobility::Movable);
			// bRealTimeCapture wird beim Anlegen des Render-Proxys gelesen - deshalb neu anlegen lassen
			Himmelslicht->bRealTimeCapture = true;
			Himmelslicht->MarkRenderStateDirty();
			Himmelslicht->RecaptureSky();
		}
	}

	Platz = Welt->SpawnActor<AMBPlatzGraubox>(AMBPlatzGraubox::StaticClass(), FTransform::Identity);
	if (Platz != nullptr)
	{
		Platz->Baue(Belag);
	}
	Ball = Welt->SpawnActor<AMBBall>(AMBBall::StaticClass(), FTransform::Identity);
	if (Ball != nullptr)
	{
		Ball->SetzeDurchmesser(BallWerte.Radius * 2.0);
		Ball->SetzeSichtbar(false);
	}
	Maschine = Welt->SpawnActor<AMBBallmaschine>(AMBBallmaschine::StaticClass(), FTransform::Identity);
	if (Maschine != nullptr)
	{
		Maschine->SetzeProgramm(Programm, Instanz != nullptr ? Instanz->GetBallmaschinenProgramm(Programm) : nullptr);
	}
	Kamera = Welt->SpawnActor<AMBTVKamera>(AMBTVKamera::StaticClass(), FTransform::Identity);
	if (Kamera != nullptr)
	{
		APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0);
		Kamera->Verfolge(Controller != nullptr ? Controller->GetPawn() : nullptr, Ball);
	}

	Anzeige.Belag = Belag.ToString();
	Anzeige.CourtPace = BelagWerte.CourtPaceRating();
	Anzeige.Programm = Maschine != nullptr ? Maschine->GetProgrammName() : Programm.ToString();
	ZeitSeitSchuss = 0.0;
	bBallAktiv = false;
	UE_LOG(LogMatchball, Log, TEXT("Training: Belag %s (CPR %.1f), Programm %s"), *Belag.ToString(), Anzeige.CourtPace, *Anzeige.Programm);
}

void AMBTrainingGameMode::StarteBall(const MB::FBallZustand& Start, int32 Schlagender)
{
	Simulation.Starte(Start);
	VorherZustand = Simulation.GetZustand();
	Akkumulator = 0.0;
	LetzterSchlagender = Schlagender;
	AufspruengeSeitSchlag = 0;
	ZeitSeitSchuss = 0.0;
	ZeitSeitAufsprungFern = -1.0;
	bBallAktiv = true;
	if (Ball != nullptr)
	{
		Ball->SetzeSichtbar(true);
		Ball->SetzeKernZustand(Start.Position, Start.Drall, 0.0);
	}
	MB::FVorhersageOptionen Optionen;
	Optionen.MaxAufspruenge = 2;
	Optionen.MaxZeit = 5.0;
	Vorhersage = MB::FVorhersage::Berechne(Simulation, Start, Optionen);
}

void AMBTrainingGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const double Zeitschritt = Simulation.GetZeitschritt();
	// Fester Zeitschritt: Bildrate beeinflusst die Physik nicht; nach Rucklern hoechstens 0,25 s nachholen
	Akkumulator = FMath::Min(Akkumulator + static_cast<double>(DeltaSeconds), 0.25);
	while (Akkumulator >= Zeitschritt)
	{
		VorherZustand = Simulation.GetZustand();
		SimulationsSchritt();
		Akkumulator -= Zeitschritt;
	}
	AktualisiereDarstellung(Akkumulator / Zeitschritt, DeltaSeconds);
	PruefeBallmaschine(DeltaSeconds);
	ZeichneVorhersage();
}

void AMBTrainingGameMode::SimulationsSchritt()
{
	Simulation.Schritt();
	for (const MB::FBallEreignis& Ereignis : Simulation.GetEreignisse())
	{
		if (Ereignis.Art == MB::EBallEreignisArt::Aufsprung)
		{
			++AufspruengeSeitSchlag;
			Anzeige.LetzterAufsprungKmh = MB::MsZuKmh(Ereignis.GeschwindigkeitNachher.Laenge());
			WerteAufsprungAus(Ereignis);
		}
	}
	Simulation.LeereEreignisse();
}

void AMBTrainingGameMode::WerteAufsprungAus(const MB::FBallEreignis& Ereignis)
{
	if (LetzterSchlagender != 0 || AufspruengeSeitSchlag != 1)
	{
		return;
	}
	// Erster Aufsprung nach einem Schlag des Spielers: Hawk-Eye-Urteil ueber das gegnerische Einzelfeld
	if (Ereignis.Position.Y <= 0.0)
	{
		Anzeige.LetztesUrteil = TEXT("NICHT UEBERS NETZ");
		return;
	}
	ZeitSeitAufsprungFern = 0.0;
	const MB::FPlatz Platzgeometrie;
	const MB::FKontaktflaeche Flaeche = MB::BerechneKontaktflaeche(Ereignis, Simulation.GetBall());
	const MB::FLinienUrteil Urteil = MB::BeurteileKontaktflaeche(Flaeche, Platzgeometrie.Einzelfeld(MB::EPlatzseite::Fern));
	if (Urteil.bIn)
	{
		++Anzeige.Treffer;
	}
	Anzeige.LetztesUrteil = FString::Printf(TEXT("%s (%.0f mm) %s"), Urteil.bIn ? TEXT("IN") : TEXT("AUS"), Urteil.Abstand * 1000.0, MBTraining_LinienName(Urteil.Linie));
}

void AMBTrainingGameMode::AktualisiereDarstellung(double Alpha, double Dt)
{
	const MB::FBallZustand& Jetzt = Simulation.GetZustand();
	const MB::FVek3 Position = MB::Interpoliere(VorherZustand.Position, Jetzt.Position, MB::Begrenze(Alpha, 0.0, 1.0));
	if (Ball != nullptr && bBallAktiv)
	{
		Ball->SetzeKernZustand(Position, Jetzt.Drall, Dt);
	}
	Anzeige.BallTempoKmh = MB::MsZuKmh(Jetzt.Geschwindigkeit.Laenge());
	Anzeige.BallDrallUpm = MB::RadSZuUpm(Jetzt.Drall.Laenge());
	Anzeige.BallHoehe = Jetzt.Position.Z;

	Anzeige.bBallInReichweite = false;
	APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0);
	if (AMBTestSpieler* Spieler = Controller != nullptr ? Cast<AMBTestSpieler>(Controller->GetPawn()) : nullptr)
	{
		const MB::FVek3 P = Spieler->GetKernPosition();
		const double Abstand = FMath::Sqrt(MB::Quadrat(Jetzt.Position.X - P.X) + MB::Quadrat(Jetzt.Position.Y - P.Y));
		Anzeige.bBallInReichweite = bBallAktiv && LetzterSchlagender != 0 && Jetzt.Position.Y < 0.0 && Abstand <= Spieler->Reichweite;
		if (Kamera != nullptr)
		{
			Kamera->Verfolge(Spieler, Ball);
		}
	}
}

void AMBTrainingGameMode::PruefeBallmaschine(double Dt)
{
	if (Maschine == nullptr)
	{
		return;
	}
	ZeitSeitSchuss += Dt;
	if (ZeitSeitAufsprungFern >= 0.0)
	{
		ZeitSeitAufsprungFern += Dt;
	}
	const MB::FBallZustand& Zustand = Simulation.GetZustand();
	bool bBereit = false;
	if (!bBallAktiv || Zustand.bRuht)
	{
		bBereit = ZeitSeitSchuss >= 1.5;
	}
	else if (LetzterSchlagender < 0)
	{
		// Ball der Maschine: naechster, wenn er vorbei ist (zweiter Aufsprung oder weit hinter dem Spieler)
		bBereit = ZeitSeitSchuss >= Maschine->GetTakt() && (AufspruengeSeitSchlag >= 2 || Zustand.Position.Y < -(MB::Platzmass::HalbeLaenge + 3.0));
	}
	else
	{
		// Rueckschlag des Spielers: kurz nach dem Aufsprung im Feld der Maschine
		bBereit = ZeitSeitAufsprungFern >= 1.2 || ZeitSeitSchuss >= Maschine->GetTakt() + 2.0;
	}
	if (!bBereit)
	{
		return;
	}
	bool bLinkshaender = false;
	APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0);
	if (const AMBTestSpieler* Spieler = Controller != nullptr ? Cast<AMBTestSpieler>(Controller->GetPawn()) : nullptr)
	{
		bLinkshaender = Spieler->bLinkshaender;
	}
	StarteBall(Maschine->NaechsterBall(Simulation, bLinkshaender), -1);
}

FName AMBTrainingGameMode::SchlagartZeile(EMBSchlagWunsch Wunsch, bool bPower, const MB::FVek3& BallPosition, const MB::FVek3& Spieler, bool bVorAufsprung) const
{
	if (Wunsch == EMBSchlagWunsch::Lob)
	{
		return bPower ? FName(TEXT("LobOffensiv")) : FName(TEXT("Lob"));
	}
	if (Wunsch == EMBSchlagWunsch::Stopp)
	{
		return FName(TEXT("Stopp"));
	}
	// Hohe Baelle werden ueber dem Kopf geschmettert - immer flach und mit voller Wucht (README 5.7)
	if (BallPosition.Z > 2.1)
	{
		return FName(TEXT("Schmetterball"));
	}
	if (bVorAufsprung)
	{
		return FName(TEXT("Volley"));
	}
	// Halbvolley: Ball springt direkt vor den Fuessen auf und wird ganz tief genommen
	if (BallPosition.Z < 0.30 && AufspruengeSeitSchlag == 1 && MB::Betrag(BallPosition.Y - Spieler.Y) < 1.2)
	{
		return FName(TEXT("Halbvolley"));
	}
	switch (Wunsch)
	{
	case EMBSchlagWunsch::Topspin: return FName(TEXT("Topspin"));
	case EMBSchlagWunsch::Slice: return FName(TEXT("Slice"));
	default: return FName(TEXT("Flach"));
	}
}

bool AMBTrainingGameMode::Schlage(AMBTestSpieler* Spieler, EMBSchlagWunsch Wunsch, bool bPower, bool bPraezision)
{
	if (Spieler == nullptr || !bBallAktiv || LetzterSchlagender == 0)
	{
		return false;
	}
	const MB::FBallZustand& Zustand = Simulation.GetZustand();
	if (Zustand.bRuht || AufspruengeSeitSchlag >= 2)
	{
		return false;
	}
	const MB::FVek3 BallPosition = Zustand.Position;
	const MB::FVek3 SpielerPosition = Spieler->GetKernPosition();
	if (BallPosition.Y >= 0.0)
	{
		return false;
	}
	const bool bVorAufsprung = AufspruengeSeitSchlag == 0;
	// Am Netz reicht der Arm weiter (Ausfallschritt, README 5.6)
	const double Reichweite = Spieler->Reichweite * ((bVorAufsprung && BallPosition.Y > -MB::Platzmass::AufschlaglinieVomNetz) ? 1.2 : 1.0);
	const double Abstand = FMath::Sqrt(MB::Quadrat(BallPosition.X - SpielerPosition.X) + MB::Quadrat(BallPosition.Y - SpielerPosition.Y));
	if (Abstand > Reichweite || BallPosition.Z < 0.12 || BallPosition.Z > 2.9)
	{
		return false;
	}

	const FName Zeile = SchlagartZeile(Wunsch, bPower, BallPosition, SpielerPosition, bVorAufsprung);
	UMBGameInstance* Instanz = UMBGameInstance::Hole(this);
	FMBSchlagartZeile Schlagart;
	if (const FMBSchlagartZeile* Gefunden = Instanz != nullptr ? Instanz->GetSchlagart(Zeile) : nullptr)
	{
		Schlagart = *Gefunden;
	}

	// Tempo aus dem Spielerwert (50 -> 99 linear), Power/Praezision
	const double Wert = Spieler->Spielerwert;
	double TempoKmh = Schlagart.TempoBeiWert50 + (Schlagart.TempoBeiWert99 - Schlagart.TempoBeiWert50) * MB::Begrenze((Wert - 50.0) / 49.0, 0.0, 1.0);
	if (bPower && !bPraezision)
	{
		TempoKmh *= Schlagart.PowerTempoFaktor;
	}
	else if (bPraezision && !bPower)
	{
		TempoKmh *= Schlagart.PraezisionTempoFaktor;
	}

	// Zielen: Stick bzw. WASD beim Schlag (vor = lang, zurueck = kurz, links/rechts = Richtung)
	const FVector2D Zielen = Spieler->GetEingabe();
	const double Rand = bPraezision ? 0.25 : (bPower ? 0.8 : 0.55);
	double ZielX = Zielen.X * 3.0;
	double ZielY = Schlagart.ZielTiefe + Zielen.Y * (Zeile == FName(TEXT("Stopp")) ? 0.8 : 2.0);
	ZielX = MB::Begrenze(ZielX, -(MB::Platzmass::HalbeBreiteEinzel - Rand), MB::Platzmass::HalbeBreiteEinzel - Rand);
	ZielY = MB::Begrenze(ZielY, 1.0, MB::Platzmass::HalbeLaenge - Rand);

	// Streuung: Timing-Meter folgt mit M2 - bis dahin perfektes Timing (nur Grundstreuung)
	MB::FStreuParameter Grund;
	MB::FStreuModifikatoren Modifikatoren;
	if (Instanz != nullptr)
	{
		Instanz->HoleStreuParameter(Grund, Modifikatoren);
	}
	const MB::FStreuParameter Streuung = MB::WendeModifikatorenAn(Grund, Modifikatoren, bPower, bPraezision, Wert);
	const MB::FStreuErgebnis Gestreut = MB::StreueZiel(BallPosition, ZielX, ZielY, 0.0, Streuung, Zufall);

	MB::FSchlagAnfrage Anfrage;
	Anfrage.Treffpunkt = BallPosition;
	Anfrage.ZielX = Gestreut.ZielX;
	Anfrage.ZielY = Gestreut.ZielY;
	Anfrage.Tempo = MB::KmhZuMs(TempoKmh);
	Anfrage.MinTempo = Anfrage.Tempo * 0.5;
	Anfrage.Topspin = MB::UpmZuRadS(Schlagart.DrallUpm);
	Anfrage.Seitendrall = MB::UpmZuRadS(Schlagart.SeitendrallUpm);
	Anfrage.bHoherBogen = Schlagart.bHoherBogen;
	Anfrage.MinNetzAbstand = Schlagart.MinNetzAbstand;
	const MB::FSchlagErgebnis Ergebnis = MB::BerechneSchlag(Simulation, Anfrage);

	StarteBall(Ergebnis.Abflug, 0);
	++Anzeige.Schlaege;
	Anzeige.LetzterSchlag = FString::Printf(TEXT("%s %.0f km/h, %.0f U/min, Netz %+.2f m"), *Zeile.ToString(), MB::MsZuKmh(Ergebnis.TatsaechlichesTempo), Schlagart.DrallUpm, Ergebnis.NetzAbstand);
	Anzeige.LetztesUrteil.Reset();
	return true;
}

void AMBTrainingGameMode::NaechstesProgramm()
{
	if (Programme.Num() == 0 || Maschine == nullptr)
	{
		return;
	}
	const int32 Index = (Programme.IndexOfByKey(Programm) + 1) % Programme.Num();
	Programm = Programme[Index];
	UMBGameInstance* Instanz = UMBGameInstance::Hole(this);
	Maschine->SetzeProgramm(Programm, Instanz != nullptr ? Instanz->GetBallmaschinenProgramm(Programm) : nullptr);
	Anzeige.Programm = Maschine->GetProgrammName();
}

void AMBTrainingGameMode::NaechsterBelag()
{
	if (Belaege.Num() == 0)
	{
		return;
	}
	const int32 Index = (Belaege.IndexOfByKey(Belag) + 1) % Belaege.Num();
	Belag = Belaege[Index];
	UMBGameInstance* Instanz = UMBGameInstance::Hole(this);
	const MB::FBelagParameter BelagWerte = Instanz != nullptr ? Instanz->GetBelag(Belag) : MB::FBelagParameter();
	Simulation.SetzeBelag(BelagWerte);
	if (Platz != nullptr)
	{
		Platz->Destroy();
	}
	Platz = GetWorld()->SpawnActor<AMBPlatzGraubox>(AMBPlatzGraubox::StaticClass(), FTransform::Identity);
	if (Platz != nullptr)
	{
		Platz->Baue(Belag);
	}
	Anzeige.Belag = Belag.ToString();
	Anzeige.CourtPace = BelagWerte.CourtPaceRating();
}

void AMBTrainingGameMode::KameraWechseln()
{
	if (Kamera != nullptr)
	{
		Kamera->NaechsterModus();
	}
}

void AMBTrainingGameMode::ZeichneVorhersage() const
{
#if ENABLE_DRAW_DEBUG
	if (!bVorhersageZeigen || !bBallAktiv)
	{
		return;
	}
	int32 Nummer = 0;
	for (const MB::FBallEreignis& Ereignis : Vorhersage.Ereignisse)
	{
		if (Ereignis.Art != MB::EBallEreignisArt::Aufsprung)
		{
			continue;
		}
		const FVector Mitte = MBBruecke::NachUnreal(MB::FVek3(Ereignis.Position.X, Ereignis.Position.Y, 0.01));
		const FColor Farbe = Nummer == 0 ? FColor::Yellow : FColor::Orange;
		DrawDebugCircle(GetWorld(), Mitte, 18.0f, 24, Farbe, false, -1.0f, 0, 1.5f, FVector(1.0, 0.0, 0.0), FVector(0.0, 1.0, 0.0), false);
		++Nummer;
	}
#endif
}
