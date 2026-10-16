// Matchball Tennis - Unreal-spezifische Tests (laufen nur im Editor, nicht in Tools/KernTests)
//
// 1. CSV-Spalten in Data/ stimmen per Reflection mit den Zeilenstrukturen ueberein
// 2. Die importierten Data Tables (/Game/Data/DT_*) existieren und haben die richtige Struktur
//    (nach Tools/assets.ps1; vorher nur eine Warnung)
// 3. Koordinaten-Bruecke Kern <-> Unreal

#include "Core/MBDatenTabellen.h"
#include "Core/MBKernBruecke.h"
#include "Engine/DataTable.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UnrealType.h"

namespace
{
	void MBDatenTest_PruefeTabelle(FAutomationTestBase& Test, const TCHAR* Datei, UScriptStruct* Struktur, const TCHAR* Tabelle)
	{
		const FString Pfad = FPaths::Combine(FPaths::ProjectDir(), TEXT("Data"), FString(Datei) + TEXT(".csv"));
		TArray<FString> Zeilen;
		if (!FFileHelper::LoadFileToStringArray(Zeilen, *Pfad) || Zeilen.Num() == 0)
		{
			Test.AddError(FString::Printf(TEXT("%s nicht lesbar"), *Pfad));
			return;
		}
		TArray<FString> Kopf;
		Zeilen[0].ParseIntoArray(Kopf, TEXT(","), false);
		for (int32 Index = 1; Index < Kopf.Num(); ++Index)
		{
			const FString Spalte = Kopf[Index].TrimStartAndEnd();
			if (Struktur->FindPropertyByName(FName(*Spalte)) == nullptr)
			{
				Test.AddError(FString::Printf(TEXT("%s.csv: Spalte %s fehlt in %s"), Datei, *Spalte, *Struktur->GetName()));
			}
		}
		for (TFieldIterator<FProperty> Iterator(Struktur); Iterator; ++Iterator)
		{
			if (!Kopf.Contains(Iterator->GetName()))
			{
				Test.AddError(FString::Printf(TEXT("%s: Feld %s fehlt in %s.csv"), *Struktur->GetName(), *Iterator->GetName(), Datei));
			}
		}

		const FString Asset = FString::Printf(TEXT("/Game/Data/%s.%s"), Tabelle, Tabelle);
		const UDataTable* Daten = LoadObject<UDataTable>(nullptr, *Asset, nullptr, LOAD_NoWarn);
		if (Daten == nullptr)
		{
			Test.AddWarning(FString::Printf(TEXT("%s noch nicht importiert (Tools/assets.ps1 ausfuehren)"), *Asset));
			return;
		}
		Test.TestTrue(FString::Printf(TEXT("%s hat die Struktur %s"), Tabelle, *Struktur->GetName()), Daten->GetRowStruct() == Struktur);
		Test.TestEqual(FString::Printf(TEXT("%s Zeilenzahl wie CSV"), Tabelle), Daten->GetRowNames().Num(), Zeilen.Num() - 1);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMBDatenTabellenTest, "MatchballTennis.Unreal.DatenTabellen", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMBDatenTabellenTest::RunTest(const FString& Parameters)
{
	MBDatenTest_PruefeTabelle(*this, TEXT("belaege"), FMBBelagZeile::StaticStruct(), TEXT("DT_Belaege"));
	MBDatenTest_PruefeTabelle(*this, TEXT("ball"), FMBBallZeile::StaticStruct(), TEXT("DT_Ball"));
	MBDatenTest_PruefeTabelle(*this, TEXT("netz"), FMBNetzZeile::StaticStruct(), TEXT("DT_Netz"));
	MBDatenTest_PruefeTabelle(*this, TEXT("schlagarten"), FMBSchlagartZeile::StaticStruct(), TEXT("DT_Schlagarten"));
	MBDatenTest_PruefeTabelle(*this, TEXT("timing"), FMBTimingZeile::StaticStruct(), TEXT("DT_Timing"));
	MBDatenTest_PruefeTabelle(*this, TEXT("ballmaschine"), FMBBallmaschineZeile::StaticStruct(), TEXT("DT_Ballmaschine"));
	MBDatenTest_PruefeTabelle(*this, TEXT("ansagen"), FMBAnsageZeile::StaticStruct(), TEXT("DT_Ansagen"));
	MBDatenTest_PruefeTabelle(*this, TEXT("spieler"), FMBSpielerZeile::StaticStruct(), TEXT("DT_Spieler"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMBKernBrueckeTest, "MatchballTennis.Unreal.KernBruecke", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMBKernBrueckeTest::RunTest(const FString& Parameters)
{
	// Kern (m): X quer, Y laengs, Z hoch -> Unreal (cm): X laengs, Y quer, Z hoch
	const FVector Unreal = MBBruecke::NachUnreal(MB::FVek3(1.0, 2.0, 3.0));
	TestEqual(TEXT("Kern X -> Unreal Y"), Unreal.Y, 100.0);
	TestEqual(TEXT("Kern Y -> Unreal X"), Unreal.X, 200.0);
	TestEqual(TEXT("Kern Z -> Unreal Z"), Unreal.Z, 300.0);
	const MB::FVek3 Zurueck = MBBruecke::NachKern(Unreal);
	TestEqual(TEXT("Hin und zurueck X"), Zurueck.X, 1.0);
	TestEqual(TEXT("Hin und zurueck Y"), Zurueck.Y, 2.0);
	TestEqual(TEXT("Hin und zurueck Z"), Zurueck.Z, 3.0);

	// Topspin beim Flug zur fernen Seite (Kern +Y = Unreal +X): die Oberseite des Balls muss sich nach +X bewegen
	FVector Achse;
	double Winkel = 0.0;
	MBBruecke::DrehungFuerDarstellung(MB::FVek3(-100.0, 0.0, 0.0), 0.01, Achse, Winkel);
	const FVector Oben = FQuat(Achse, Winkel).RotateVector(FVector(0.0, 0.0, 1.0));
	TestTrue(TEXT("Topspin dreht die Oberseite nach vorne (+X)"), Oben.X > 0.0);
	return true;
}
