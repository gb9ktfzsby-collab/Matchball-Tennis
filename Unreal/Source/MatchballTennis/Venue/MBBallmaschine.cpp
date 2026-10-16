// Matchball Tennis - Ballmaschine

#include "Venue/MBBallmaschine.h"

#include "Components/StaticMeshComponent.h"
#include "Core/MBDatenTabellen.h"
#include "Core/MBKernBruecke.h"
#include "Engine/StaticMesh.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Physics/Kern/MBPlatz.h"
#include "Physics/Kern/MBSchlagberechnung.h"
#include "UObject/ConstructorHelpers.h"

AMBBallmaschine::AMBBallmaschine()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Quadermesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Zylindermesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	Gehaeuse = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Gehaeuse"));
	RootComponent = Gehaeuse;
	Gehaeuse->SetStaticMesh(Quadermesh.Object);
	Gehaeuse->SetMaterial(0, Material.Object);
	Gehaeuse->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// 60 x 45 x 70 cm Gehaeuse
	Gehaeuse->SetRelativeScale3D(FVector(0.6, 0.45, 0.7));

	Rohr = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rohr"));
	Rohr->SetupAttachment(Gehaeuse);
	Rohr->SetStaticMesh(Zylindermesh.Object);
	Rohr->SetMaterial(0, Material.Object);
	Rohr->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Eigene (absolute) Skalierung, damit das Rohr die Gehaeuse-Skalierung nicht erbt
	Rohr->SetUsingAbsoluteScale(true);
	Rohr->SetRelativeLocation(FVector(-60.0, 0.0, 40.0));
	Rohr->SetRelativeRotation(FRotator(80.0, 0.0, 0.0));
	Rohr->SetRelativeScale3D(FVector(0.15, 0.15, 0.6));

	Zufall.Starte(FPlatformTime::Cycles64());
}

void AMBBallmaschine::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInstanceDynamic* Farbe = Gehaeuse->CreateAndSetMaterialInstanceDynamic(0))
	{
		Farbe->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.15f, 0.15f, 0.18f));
	}
	if (UMaterialInstanceDynamic* Farbe = Rohr->CreateAndSetMaterialInstanceDynamic(0))
	{
		Farbe->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.6f, 0.6f, 0.6f));
	}
	// Maschine hinter der fernen Grundlinie, Blick zur nahen Seite (-X in Unreal)
	SetActorLocation(MBBruecke::NachUnreal(MB::FVek3(0.0, MB::Platzmass::HalbeLaenge + 1.0, 0.35)));
	SetActorRotation(FRotator(0.0, 180.0, 0.0));
}

void AMBBallmaschine::SetzeProgramm(FName Name, const FMBBallmaschineZeile* Zeile)
{
	Programm = Name;
	if (Zeile == nullptr)
	{
		return;
	}
	Anzeigename = Zeile->Anzeigename;
	Tempo = Zeile->Tempo_kmh;
	Drall = Zeile->Drall_upm;
	Hoehe = Zeile->Hoehe;
	Position = Zeile->Position;
	ZielTiefe = Zeile->ZielTiefe;
	Takt = Zeile->Takt_s;
	bHoherBogen = Zeile->bHoherBogen;
}

MB::FVek3 AMBBallmaschine::GetKernPosition() const
{
	return MB::FVek3(0.0, MB::Platzmass::HalbeLaenge + 1.0, Hoehe);
}

MB::FBallZustand AMBBallmaschine::NaechsterBall(const MB::FBallSimulation& Vorlage, bool bSpielerLinkshaender)
{
	// Zielseite aus Sicht des Spielers auf der nahen Seite (Blick nach +Y: rechts = +X)
	double Seite = 0.0;
	if (Position == TEXT("Vorhand"))
	{
		Seite = 1.0;
	}
	else if (Position == TEXT("Rueckhand"))
	{
		Seite = -1.0;
	}
	else if (Position == TEXT("Zufall"))
	{
		Seite = Zufall.Bereich(-1.0, 1.0);
	}
	else
	{
		Seite = bNaechsterVorhand ? 1.0 : -1.0;
		bNaechsterVorhand = !bNaechsterVorhand;
	}
	if (bSpielerLinkshaender)
	{
		Seite = -Seite;
	}

	MB::FSchlagAnfrage Anfrage;
	Anfrage.Treffpunkt = GetKernPosition();
	Anfrage.ZielX = Seite * 2.2 + Zufall.Bereich(-0.3, 0.3);
	Anfrage.ZielY = -(ZielTiefe + Zufall.Bereich(-0.4, 0.4));
	Anfrage.Tempo = MB::KmhZuMs(Tempo);
	Anfrage.MinTempo = Anfrage.Tempo * 0.6;
	Anfrage.Topspin = MB::UpmZuRadS(Drall);
	Anfrage.bHoherBogen = bHoherBogen;
	Anfrage.MinNetzAbstand = bHoherBogen ? 1.0 : 0.3;
	const MB::FSchlagErgebnis Ergebnis = MB::BerechneSchlag(Vorlage, Anfrage);
	return Ergebnis.Abflug;
}
