// Matchball Tennis - sichtbarer Ball

#include "Physics/MBBall.h"

#include "Components/StaticMeshComponent.h"
#include "Core/MBKernBruecke.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AMBBall::AMBBall()
{
	PrimaryActorTick.bCanEverTick = false;

	Kugel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Kugel"));
	RootComponent = Kugel;
	Kugel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Kugel->SetGenerateOverlapEvents(false);
	Kugel->SetMobility(EComponentMobility::Movable);
	Kugel->CastShadow = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> KugelMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (KugelMesh.Succeeded())
	{
		Kugel->SetStaticMesh(KugelMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Grundmaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (Grundmaterial.Succeeded())
	{
		Kugel->SetMaterial(0, Grundmaterial.Object);
	}
	SetzeDurchmesser(0.067);
}

void AMBBall::BeginPlay()
{
	Super::BeginPlay();
	Material = Kugel->CreateAndSetMaterialInstanceDynamic(0);
	if (Material != nullptr)
	{
		// Optic Yellow (Platzhalter bis zum Filz-Material aus Blender)
		Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.80f, 0.95f, 0.05f));
	}
}

void AMBBall::SetzeDurchmesser(double Meter)
{
	// Engine-Kugel hat 100 cm Durchmesser
	const double Skalierung = Meter * 100.0 / 100.0;
	Kugel->SetRelativeScale3D(FVector(Skalierung));
}

void AMBBall::SetzeKernZustand(const MB::FVek3& Position, const MB::FVek3& Drall, double Dt)
{
	SetActorLocation(MBBruecke::NachUnreal(Position));
	FVector Achse;
	double Winkel = 0.0;
	MBBruecke::DrehungFuerDarstellung(Drall, Dt, Achse, Winkel);
	if (Winkel != 0.0)
	{
		const FQuat Drehung(Achse, Winkel);
		SetActorRotation(Drehung * GetActorQuat());
	}
}

void AMBBall::SetzeSichtbar(bool bSichtbar)
{
	SetActorHiddenInGame(!bSichtbar);
}
