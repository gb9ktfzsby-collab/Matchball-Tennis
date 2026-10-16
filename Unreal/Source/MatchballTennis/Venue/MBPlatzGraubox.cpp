// Matchball Tennis - Graubox-Platz mit exakten Massen

#include "Venue/MBPlatzGraubox.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/MBKernBruecke.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Physics/Kern/MBPlatz.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	struct FMBPlatzFarben
	{
		FLinearColor Feld;
		FLinearColor Auslauf;
		FLinearColor Umgebung;
	};

	FMBPlatzFarben MBPlatz_Farben(FName Belag)
	{
		if (Belag == TEXT("Sand"))
		{
			return {FLinearColor(0.55f, 0.22f, 0.10f), FLinearColor(0.55f, 0.22f, 0.10f), FLinearColor(0.25f, 0.30f, 0.20f)};
		}
		if (Belag == TEXT("Rasen"))
		{
			return {FLinearColor(0.16f, 0.38f, 0.10f), FLinearColor(0.18f, 0.42f, 0.12f), FLinearColor(0.14f, 0.30f, 0.10f)};
		}
		if (Belag == TEXT("Halle"))
		{
			return {FLinearColor(0.04f, 0.09f, 0.25f), FLinearColor(0.03f, 0.04f, 0.08f), FLinearColor(0.02f, 0.02f, 0.03f)};
		}
		// Hartplatz: blaues Feld, gruener Auslauf
		return {FLinearColor(0.08f, 0.20f, 0.48f), FLinearColor(0.15f, 0.35f, 0.22f), FLinearColor(0.20f, 0.22f, 0.22f)};
	}

	const FLinearColor MBPlatz_Weiss(0.92f, 0.92f, 0.92f);
	const FLinearColor MBPlatz_Netz(0.05f, 0.05f, 0.05f);
	const FLinearColor MBPlatz_Pfosten(0.10f, 0.18f, 0.12f);
}

AMBPlatzGraubox::AMBPlatzGraubox()
{
	PrimaryActorTick.bCanEverTick = false;
	Wurzel = CreateDefaultSubobject<USceneComponent>(TEXT("Wurzel"));
	RootComponent = Wurzel;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Quadermesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Zylindermesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	QuaderMesh = Quadermesh.Object;
	ZylinderMesh = Zylindermesh.Object;
	Grundmaterial = Material.Object;
}

void AMBPlatzGraubox::BeginPlay()
{
	Super::BeginPlay();
}

UStaticMeshComponent* AMBPlatzGraubox::Quader(const FString& Name, double MitteX, double MitteY, double MitteZ, double BreiteX, double LaengeY, double HoeheZ, const FLinearColor& Farbe)
{
	UStaticMeshComponent* Teil = NewObject<UStaticMeshComponent>(this, FName(*Name));
	Teil->SetupAttachment(Wurzel);
	Teil->SetStaticMesh(QuaderMesh);
	Teil->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Teil->SetMobility(EComponentMobility::Static);
	Teil->SetRelativeLocation(MBBruecke::NachUnreal(MB::FVek3(MitteX, MitteY, MitteZ)));
	// Engine-Quader: 100 cm Kantenlaenge -> Skalierung = Meter; UE.X = Laenge (Kern Y), UE.Y = Breite (Kern X)
	Teil->SetRelativeScale3D(FVector(LaengeY, BreiteX, HoeheZ));
	if (UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Grundmaterial, this))
	{
		Material->SetVectorParameterValue(TEXT("Color"), Farbe);
		Teil->SetMaterial(0, Material);
	}
	Teil->RegisterComponent();
	AddInstanceComponent(Teil);
	Teile.Add(Teil);
	return Teil;
}

UStaticMeshComponent* AMBPlatzGraubox::Zylinder(const FString& Name, double MitteX, double MitteY, double Hoehe, double Durchmesser, const FLinearColor& Farbe)
{
	UStaticMeshComponent* Teil = NewObject<UStaticMeshComponent>(this, FName(*Name));
	Teil->SetupAttachment(Wurzel);
	Teil->SetStaticMesh(ZylinderMesh);
	Teil->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Teil->SetMobility(EComponentMobility::Static);
	Teil->SetRelativeLocation(MBBruecke::NachUnreal(MB::FVek3(MitteX, MitteY, Hoehe * 0.5)));
	Teil->SetRelativeScale3D(FVector(Durchmesser, Durchmesser, Hoehe));
	if (UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Grundmaterial, this))
	{
		Material->SetVectorParameterValue(TEXT("Color"), Farbe);
		Teil->SetMaterial(0, Material);
	}
	Teil->RegisterComponent();
	AddInstanceComponent(Teil);
	Teile.Add(Teil);
	return Teil;
}

void AMBPlatzGraubox::Baue(FName Belag)
{
	if (bGebaut || QuaderMesh == nullptr || ZylinderMesh == nullptr)
	{
		return;
	}
	bGebaut = true;
	using namespace MB::Platzmass;
	const FMBPlatzFarben Farben = MBPlatz_Farben(Belag);
	const MB::FLinienbreiten Linien;

	// Flaechen: Umgebung, Auslauf, Spielfeld (Doppelfeld) - Oberkanten knapp gestaffelt gegen Flimmern
	Quader(TEXT("Umgebung"), 0.0, 0.0, -0.06, 50.0, 80.0, 0.10, Farben.Umgebung);
	const double AuslaufLaenge = Laenge + 2.0 * AuslaufHintenMin;
	const double AuslaufBreite = BreiteDoppel + 2.0 * AuslaufSeitlichMin;
	Quader(TEXT("Auslauf"), 0.0, 0.0, -0.028, AuslaufBreite, AuslaufLaenge, 0.05, Farben.Auslauf);
	Quader(TEXT("Spielfeld"), 0.0, 0.0, -0.025, BreiteDoppel, Laenge, 0.05, Farben.Feld);

	// Linien liegen innerhalb der Masse (gemessen bis zur Aussenkante)
	constexpr double LinienHoehe = 0.004;
	for (int Seite = -1; Seite <= 1; Seite += 2)
	{
		const double S = static_cast<double>(Seite);
		const FString Kennung = Seite < 0 ? TEXT("Nah") : TEXT("Fern");
		Quader(TEXT("Grundlinie") + Kennung, 0.0, S * (HalbeLaenge - Linien.Grundlinie * 0.5), 0.0, BreiteDoppel, Linien.Grundlinie, LinienHoehe, MBPlatz_Weiss);
		Quader(TEXT("Aufschlaglinie") + Kennung, 0.0, S * (AufschlaglinieVomNetz - Linien.Aufschlaglinie * 0.5), 0.0, BreiteEinzel, Linien.Aufschlaglinie, LinienHoehe, MBPlatz_Weiss);
		Quader(TEXT("Mittelmarke") + Kennung, 0.0, S * (HalbeLaenge - Linien.Grundlinie - MittelmarkeLaenge * 0.5), 0.0, MittelmarkeBreite, MittelmarkeLaenge, LinienHoehe, MBPlatz_Weiss);
		const FString Seitlich = Seite < 0 ? TEXT("Links") : TEXT("Rechts");
		Quader(TEXT("EinzelSeitenlinie") + Seitlich, S * (HalbeBreiteEinzel - Linien.Seitenlinie * 0.5), 0.0, 0.0, Linien.Seitenlinie, Laenge, LinienHoehe, MBPlatz_Weiss);
		Quader(TEXT("DoppelSeitenlinie") + Seitlich, S * (HalbeBreiteDoppel - Linien.Seitenlinie * 0.5), 0.0, 0.0, Linien.Seitenlinie, Laenge, LinienHoehe, MBPlatz_Weiss);
	}
	Quader(TEXT("Mittellinie"), 0.0, 0.0, 0.0, Linien.Mittellinie, 2.0 * AufschlaglinieVomNetz, LinienHoehe, MBPlatz_Weiss);

	// Netz in senkrechten Streifen, damit der Durchhang (0,914 m Mitte -> 1,07 m an den Einzelstuetzen) stimmt
	constexpr int StreifenInnen = 10;
	constexpr int StreifenAussen = 2;
	constexpr double BandHoehe = 0.06;
	for (int Seite = -1; Seite <= 1; Seite += 2)
	{
		const double S = static_cast<double>(Seite);
		for (int Index = 0; Index < StreifenInnen + StreifenAussen; ++Index)
		{
			const bool bInnen = Index < StreifenInnen;
			const double Von = bInnen ? EinzelstuetzeX * Index / StreifenInnen : EinzelstuetzeX + (PfostenX - EinzelstuetzeX) * (Index - StreifenInnen) / StreifenAussen;
			const double Bis = bInnen ? EinzelstuetzeX * (Index + 1) / StreifenInnen : EinzelstuetzeX + (PfostenX - EinzelstuetzeX) * (Index + 1 - StreifenInnen) / StreifenAussen;
			const double Mitte = 0.5 * (Von + Bis);
			const double Oberkante = MB::FPlatz::NetzOberkante(Mitte);
			const FString Nummer = FString::Printf(TEXT("%s%02d"), Seite < 0 ? TEXT("L") : TEXT("R"), Index);
			Quader(TEXT("Netz") + Nummer, S * Mitte, 0.0, (Oberkante - BandHoehe) * 0.5, Bis - Von, 0.01, Oberkante - BandHoehe, MBPlatz_Netz);
			Quader(TEXT("Netzband") + Nummer, S * Mitte, 0.0, Oberkante - BandHoehe * 0.5, Bis - Von, 0.025, BandHoehe, MBPlatz_Weiss);
		}
		Zylinder(Seite < 0 ? TEXT("PfostenLinks") : TEXT("PfostenRechts"), S * PfostenX, 0.0, NetzhoehePfosten, 0.10, MBPlatz_Pfosten);
		Zylinder(Seite < 0 ? TEXT("EinzelstuetzeLinks") : TEXT("EinzelstuetzeRechts"), S * EinzelstuetzeX, 0.0, NetzhoehePfosten, 0.075, MBPlatz_Weiss);
	}
	// Mittelband (haelt das Netz in der Mitte auf 0,914 m)
	Quader(TEXT("Mittelband"), 0.0, 0.0, NetzhoeheMitte * 0.5, 0.05, 0.02, NetzhoeheMitte, MBPlatz_Weiss);
}
