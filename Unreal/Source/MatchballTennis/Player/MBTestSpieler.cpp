// Matchball Tennis - Platzhalter-Spieler fuer Meilenstein M1

#include "Player/MBTestSpieler.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/MBKernBruecke.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AMBTestSpieler::AMBTestSpieler()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessPlayer = EAutoReceiveInput::Disabled;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Zylindermesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Quadermesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	Wurzel = CreateDefaultSubobject<USceneComponent>(TEXT("Wurzel"));
	RootComponent = Wurzel;

	// Koerper: Zylinder 50 cm Durchmesser, 185 cm hoch (Fusspunkt = Actor-Position)
	Koerper = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Koerper"));
	Koerper->SetupAttachment(Wurzel);
	Koerper->SetStaticMesh(Zylindermesh.Object);
	Koerper->SetMaterial(0, Material.Object);
	Koerper->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Koerper->SetRelativeLocation(FVector(0.0, 0.0, 92.5));
	Koerper->SetRelativeScale3D(FVector(0.5, 0.5, 1.85));

	// Schlaeger-Andeutung rechts in Hueftnaehe (Platzhalter bis zum Schlaegermodell, README 3.4)
	Schlaeger = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Schlaeger"));
	Schlaeger->SetupAttachment(Wurzel);
	Schlaeger->SetStaticMesh(Quadermesh.Object);
	Schlaeger->SetMaterial(0, Material.Object);
	Schlaeger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Schlaeger->SetRelativeLocation(FVector(20.0, 45.0, 100.0));
	Schlaeger->SetRelativeScale3D(FVector(0.03, 0.27, 0.68));
}

void AMBTestSpieler::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInstanceDynamic* Farbe = Koerper->CreateAndSetMaterialInstanceDynamic(0))
	{
		Farbe->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.85f, 0.85f, 0.88f));
	}
	if (UMaterialInstanceDynamic* Farbe = Schlaeger->CreateAndSetMaterialInstanceDynamic(0))
	{
		Farbe->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.10f, 0.10f, 0.12f));
	}
	// Startposition: Mitte hinter der nahen Grundlinie, Blick zum Netz (+X in Unreal)
	SetzeKernPosition(MB::FVek3(0.4, -(MB::Platzmass::HalbeLaenge + 0.8), 0.0));
	SetActorRotation(FRotator(0.0, Seite == MB::EPlatzseite::Nah ? 0.0 : 180.0, 0.0));
}

MB::FVek3 AMBTestSpieler::GetKernPosition() const
{
	return MBBruecke::NachKern(GetActorLocation());
}

void AMBTestSpieler::SetzeKernPosition(const MB::FVek3& Position)
{
	SetActorLocation(MBBruecke::NachUnreal(MB::FVek3(Position.X, Position.Y, 0.0)));
}

void AMBTestSpieler::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Eingabe.IsNearlyZero())
	{
		return;
	}
	// Eingabe aus Sicht des Spielers: auf der nahen Seite ist "vor" = +Y und "rechts" = +X im Kern
	const double Richtung = Seite == MB::EPlatzseite::Nah ? 1.0 : -1.0;
	FVector2D Begrenzt = Eingabe;
	if (Begrenzt.SizeSquared() > 1.0)
	{
		Begrenzt.Normalize();
	}
	MB::FVek3 Position = GetKernPosition();
	Position.X += Richtung * Begrenzt.X * Laufgeschwindigkeit * DeltaSeconds;
	Position.Y += Richtung * Begrenzt.Y * Laufgeschwindigkeit * DeltaSeconds;
	// Eigene Platzhaelfte inkl. Auslauf, nicht ins Netz
	Position.X = MB::Begrenze(Position.X, -8.0, 8.0);
	if (Seite == MB::EPlatzseite::Nah)
	{
		Position.Y = MB::Begrenze(Position.Y, -18.0, -0.4);
	}
	else
	{
		Position.Y = MB::Begrenze(Position.Y, 0.4, 18.0);
	}
	SetzeKernPosition(Position);
}
