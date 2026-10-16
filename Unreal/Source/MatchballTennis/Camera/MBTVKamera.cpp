// Matchball Tennis - Fernsehkamera

#include "Camera/MBTVKamera.h"

#include "Camera/CameraComponent.h"
#include "Core/MBKernBruecke.h"

AMBTVKamera::AMBTVKamera()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	if (UCameraComponent* Kamera = GetCameraComponent())
	{
		Kamera->SetConstraintAspectRatio(false);
		Kamera->SetFieldOfView(55.0f);
	}
}

void AMBTVKamera::Verfolge(AActor* InSpieler, AActor* InBall)
{
	Spieler = InSpieler;
	Ball = InBall;
}

void AMBTVKamera::NaechsterModus()
{
	switch (Modus)
	{
	case EMBKameraModus::TV: Modus = EMBKameraModus::TVHoch; break;
	case EMBKameraModus::TVHoch: Modus = EMBKameraModus::Spieler; break;
	case EMBKameraModus::Spieler: Modus = EMBKameraModus::Seite; break;
	case EMBKameraModus::Seite: Modus = EMBKameraModus::TV; break;
	}
	bInitialisiert = false;   // harter Schnitt wie im Fernsehen
}

void AMBTVKamera::BerechneZiel(FVector& OutPosition, FVector& OutBlickpunkt, float& OutSichtfeld) const
{
	using namespace MB::Platzmass;
	// Richtung "hinter dem eigenen Spieler": nahe Seite = -Y im Kern
	const double S = MB::SeitenVorzeichen(Seite);
	MB::FVek3 SpielerPos(0.0, S * (HalbeLaenge + 0.5), 0.0);
	if (Spieler.IsValid())
	{
		SpielerPos = MBBruecke::NachKern(Spieler->GetActorLocation());
	}
	MB::FVek3 BallPos = SpielerPos;
	if (Ball.IsValid() && !Ball->IsHidden())
	{
		BallPos = MBBruecke::NachKern(Ball->GetActorLocation());
	}
	const double TiefeSpieler = MB::Betrag(SpielerPos.Y);

	MB::FVek3 Position;
	MB::FVek3 Blick;
	switch (Modus)
	{
	case EMBKameraModus::TV:
	{
		const double Abstand = MB::MaxWert(HalbeLaenge + 7.5, TiefeSpieler + 7.0);
		Position = MB::FVek3(SpielerPos.X * 0.35, S * Abstand, 6.0);
		Blick = MB::FVek3(SpielerPos.X * 0.15, -S * 3.5, 0.3);
		OutSichtfeld = 55.0f;
		break;
	}
	case EMBKameraModus::TVHoch:
	{
		const double Abstand = MB::MaxWert(HalbeLaenge + 12.0, TiefeSpieler + 11.0);
		Position = MB::FVek3(SpielerPos.X * 0.2, S * Abstand, 11.0);
		Blick = MB::FVek3(0.0, -S * 2.0, 0.0);
		OutSichtfeld = 48.0f;
		break;
	}
	case EMBKameraModus::Spieler:
	{
		Position = MB::FVek3(SpielerPos.X, SpielerPos.Y + S * 4.5, 2.4);
		Blick = MB::Interpoliere(MB::FVek3(SpielerPos.X * 0.5, -S * 6.0, 0.8), BallPos, 0.35);
		OutSichtfeld = 65.0f;
		break;
	}
	case EMBKameraModus::Seite:
	{
		Position = MB::FVek3(16.0, 0.0, 4.5);
		Blick = MB::FVek3(0.0, BallPos.Y * 0.3, 0.5);
		OutSichtfeld = 70.0f;
		break;
	}
	}
	OutPosition = MBBruecke::NachUnreal(Position);
	OutBlickpunkt = MBBruecke::NachUnreal(Blick);
}

void AMBTVKamera::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	FVector Position;
	FVector Blick;
	float Sichtfeld = 55.0f;
	BerechneZiel(Position, Blick, Sichtfeld);
	if (!bInitialisiert)
	{
		GlattePosition = Position;
		GlatterBlick = Blick;
		bInitialisiert = true;
	}
	else
	{
		GlattePosition = FMath::VInterpTo(GlattePosition, Position, DeltaSeconds, Nachfuehrung);
		GlatterBlick = FMath::VInterpTo(GlatterBlick, Blick, DeltaSeconds, Nachfuehrung);
	}
	SetActorLocationAndRotation(GlattePosition, (GlatterBlick - GlattePosition).Rotation());
	if (UCameraComponent* Kamera = GetCameraComponent())
	{
		Kamera->SetFieldOfView(Sichtfeld);
	}
}
