// Matchball Tennis - GameInstance

#include "Core/MBGameInstance.h"

#include "Core/MBDatenTabellen.h"
#include "Core/MBKernBruecke.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "MatchballTennis.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

void UMBGameInstance::Init()
{
	Super::Init();
	Belaege = LadeTabelle(TEXT("DT_Belaege"));
	Ball = LadeTabelle(TEXT("DT_Ball"));
	Netz = LadeTabelle(TEXT("DT_Netz"));
	Schlagarten = LadeTabelle(TEXT("DT_Schlagarten"));
	Timing = LadeTabelle(TEXT("DT_Timing"));
	Ballmaschine = LadeTabelle(TEXT("DT_Ballmaschine"));
	Ansagen = LadeTabelle(TEXT("DT_Ansagen"));
	Spieler = LadeTabelle(TEXT("DT_Spieler"));
	UE_LOG(LogMatchball, Log, TEXT("Matchball Tennis %s gestartet"), *VersionText());
}

UMBGameInstance* UMBGameInstance::Hole(const UObject* WeltKontext)
{
	return Cast<UMBGameInstance>(UGameplayStatics::GetGameInstance(WeltKontext));
}

FString UMBGameInstance::VersionText()
{
	// Einmal ermitteln und merken (wird z. B. jedes Bild im HUD angezeigt)
	static const FString Text = []()
	{
		// build.json schreibt Tools/package.ps1 in den Spielordner (neben "Matchball Tennis.exe"): {"build":"6e5689499cda"}
		FString Build = TEXT("Entwicklung");
		const FString Datei = FPaths::Combine(FPaths::RootDir(), TEXT("build.json"));
		FString Inhalt;
		if (FFileHelper::LoadFileToString(Inhalt, *Datei))
		{
			int32 Start = Inhalt.Find(TEXT("\"build\""));
			if (Start != INDEX_NONE)
			{
				Start = Inhalt.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, Start + 7);
				const int32 Ende = Start != INDEX_NONE ? Inhalt.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, Start + 1) : INDEX_NONE;
				if (Start != INDEX_NONE && Ende != INDEX_NONE)
				{
					Build = Inhalt.Mid(Start + 1, Ende - Start - 1);
				}
			}
		}
		return FString::Printf(TEXT("Version %s - Build %s"), MBVersion::Version, *Build);
	}();
	return Text;
}

UDataTable* UMBGameInstance::LadeTabelle(const TCHAR* Name) const
{
	const FString Pfad = FString::Printf(TEXT("/Game/Data/%s.%s"), Name, Name);
	UDataTable* Tabelle = LoadObject<UDataTable>(nullptr, *Pfad, nullptr, LOAD_NoWarn);
	if (Tabelle == nullptr)
	{
		UE_LOG(LogMatchball, Warning, TEXT("Data Table %s fehlt - Standardwerte werden benutzt. Tools/assets.ps1 ausfuehren."), *Pfad);
	}
	return Tabelle;
}

MB::FBallParameter UMBGameInstance::GetBallParameter() const
{
	if (Ball != nullptr)
	{
		if (const FMBBallZeile* Zeile = Ball->FindRow<FMBBallZeile>(TEXT("Standard"), TEXT("Ball")))
		{
			return MBBruecke::Ball(*Zeile);
		}
	}
	return MB::FBallParameter();
}

MB::FNetzParameter UMBGameInstance::GetNetzParameter() const
{
	if (Netz != nullptr)
	{
		if (const FMBNetzZeile* Zeile = Netz->FindRow<FMBNetzZeile>(TEXT("Standard"), TEXT("Netz")))
		{
			return MBBruecke::Netz(*Zeile);
		}
	}
	return MB::FNetzParameter();
}

MB::FBelagParameter UMBGameInstance::GetBelag(FName Name) const
{
	if (Belaege != nullptr)
	{
		if (const FMBBelagZeile* Zeile = Belaege->FindRow<FMBBelagZeile>(Name, TEXT("Belag")))
		{
			return MBBruecke::Belag(*Zeile, Name);
		}
	}
	UE_LOG(LogMatchball, Warning, TEXT("Belag %s nicht gefunden - Standard-Hartplatz"), *Name.ToString());
	return MB::FBelagParameter();
}

const FMBSchlagartZeile* UMBGameInstance::GetSchlagart(FName Name) const
{
	return Schlagarten != nullptr ? Schlagarten->FindRow<FMBSchlagartZeile>(Name, TEXT("Schlagart"), false) : nullptr;
}

const FMBBallmaschineZeile* UMBGameInstance::GetBallmaschinenProgramm(FName Name) const
{
	return Ballmaschine != nullptr ? Ballmaschine->FindRow<FMBBallmaschineZeile>(Name, TEXT("Ballmaschine"), false) : nullptr;
}

const FMBTimingZeile* UMBGameInstance::GetTiming() const
{
	return Timing != nullptr ? Timing->FindRow<FMBTimingZeile>(TEXT("Standard"), TEXT("Timing"), false) : nullptr;
}

void UMBGameInstance::HoleStreuParameter(MB::FStreuParameter& OutGrund, MB::FStreuModifikatoren& OutModifikatoren) const
{
	OutGrund = MB::FStreuParameter();
	OutModifikatoren = MB::FStreuModifikatoren();
	const FMBTimingZeile* Zeile = GetTiming();
	if (Zeile == nullptr)
	{
		return;
	}
	OutGrund.GruenHalbfenster = Zeile->GruenHalbfenster_ms / 1000.0;
	OutGrund.GelbHalbfenster = Zeile->GelbHalbfenster_ms / 1000.0;
	OutGrund.Grundstreuung = Zeile->Grundstreuung;
	OutGrund.SeitlichProSekunde = Zeile->SeitlichProSekunde;
	OutGrund.LaengenStreuung = Zeile->LaengenStreuung;
	OutGrund.LaengeProSekunde = Zeile->LaengeProSekunde;
	OutModifikatoren.PowerFenster = Zeile->PowerFenster;
	OutModifikatoren.PowerStreuung = Zeile->PowerStreuung;
	OutModifikatoren.PraezisionFenster = Zeile->PraezisionFenster;
	OutModifikatoren.PraezisionStreuung = Zeile->PraezisionStreuung;
	OutModifikatoren.FensterBeiWert50 = Zeile->FensterBeiWert50;
	OutModifikatoren.FensterBeiWert99 = Zeile->FensterBeiWert99;
	OutModifikatoren.StreuungBeiWert50 = Zeile->StreuungBeiWert50;
	OutModifikatoren.StreuungBeiWert99 = Zeile->StreuungBeiWert99;
}
