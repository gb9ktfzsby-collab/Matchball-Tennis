// Matchball Tennis - gemeinsamer Testrahmen fuer die Kern-Tests
//
// Dieselben Testdateien laufen auf zwei Wegen:
//  1. im Unreal Automation Framework (Modul MatchballTennisTests, Session Frontend / -ExecCmds="Automation RunTests MatchballTennis")
//  2. eigenstaendig ohne Unreal (Tools/KernTests, CMake) - mit MB_KERN_EIGENSTAENDIG
//
// Schreibweise:
//   MB_TEST(ZaehlweiseEinstand, "Zaehlweise.Einstand")
//   {
//       MB_PRUEFE(Bedingung);
//       MB_PRUEFE_NAH(Ist, Soll, Toleranz);
//       MB_PRUEFE_GLEICH(Ist, Soll);
//   }

#pragma once

#include <cmath>
#include <cstdio>
#include <string>

#if defined(MB_KERN_EIGENSTAENDIG)

#include <vector>

namespace MBTest
{
	class FKontext
	{
	public:
		explicit FKontext(const char* InName) : Name(InName) {}

		void Pruefe(bool bOk, const char* Ausdruck, const char* Datei, int Zeile)
		{
			++Pruefungen;
			if (!bOk)
			{
				++Fehler;
				std::printf("  FEHLER %s:%d: %s\n", Datei, Zeile, Ausdruck);
			}
		}

		void PruefeNah(double Ist, double Soll, double Toleranz, const char* AusdruckIst, const char* AusdruckSoll, const char* Datei, int Zeile)
		{
			++Pruefungen;
			if (!(std::fabs(Ist - Soll) <= Toleranz))
			{
				++Fehler;
				std::printf("  FEHLER %s:%d: %s = %.9g, erwartet %s = %.9g (+- %.3g)\n", Datei, Zeile, AusdruckIst, Ist, AusdruckSoll, Soll, Toleranz);
			}
		}

		void Info(const std::string& Text) { std::printf("    %s\n", Text.c_str()); }

		bool Erfolgreich() const { return Fehler == 0; }
		int GetFehler() const { return Fehler; }
		int GetPruefungen() const { return Pruefungen; }
		const char* GetName() const { return Name; }

	private:
		const char* Name;
		int Fehler = 0;
		int Pruefungen = 0;
	};

	using FTestFunktion = void (*)(FKontext&);

	struct FEintrag
	{
		const char* Name;
		FTestFunktion Funktion;
	};

	inline std::vector<FEintrag>& Verzeichnis()
	{
		static std::vector<FEintrag> Liste;
		return Liste;
	}

	struct FRegistrierung
	{
		FRegistrierung(const char* Name, FTestFunktion Funktion) { Verzeichnis().push_back(FEintrag{Name, Funktion}); }
	};
}

#define MB_TEST(Bezeichner, Pfad) \
	static void Bezeichner##_Lauf(MBTest::FKontext& Ctx); \
	static MBTest::FRegistrierung Bezeichner##_Registrierung("MatchballTennis.Kern." Pfad, &Bezeichner##_Lauf); \
	static void Bezeichner##_Lauf(MBTest::FKontext& Ctx)

#else // Unreal Automation Framework

#include "Misc/AutomationTest.h"

namespace MBTest
{
	class FKontext
	{
	public:
		explicit FKontext(FAutomationTestBase* InTest) : Test(InTest) {}

		void Pruefe(bool bOk, const char* Ausdruck, const char* Datei, int Zeile)
		{
			if (!bOk)
			{
				++Fehler;
				Test->AddError(FString::Printf(TEXT("%s (%s:%d)"), UTF8_TO_TCHAR(Ausdruck), UTF8_TO_TCHAR(Datei), Zeile));
			}
		}

		void PruefeNah(double Ist, double Soll, double Toleranz, const char* AusdruckIst, const char* AusdruckSoll, const char* Datei, int Zeile)
		{
			if (!(std::fabs(Ist - Soll) <= Toleranz))
			{
				++Fehler;
				Test->AddError(FString::Printf(TEXT("%s = %.9g, erwartet %s = %.9g (+- %.3g) (%s:%d)"), UTF8_TO_TCHAR(AusdruckIst), Ist, UTF8_TO_TCHAR(AusdruckSoll), Soll, Toleranz, UTF8_TO_TCHAR(Datei), Zeile));
			}
		}

		void Info(const std::string& Text) { Test->AddInfo(FString(UTF8_TO_TCHAR(Text.c_str()))); }

		bool Erfolgreich() const { return Fehler == 0; }

	private:
		FAutomationTestBase* Test;
		int Fehler = 0;
	};
}

#define MB_TEST(Bezeichner, Pfad) \
	static void Bezeichner##_Lauf(MBTest::FKontext& Ctx); \
	IMPLEMENT_SIMPLE_AUTOMATION_TEST(F##Bezeichner##Test, "MatchballTennis.Kern." Pfad, EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter) \
	bool F##Bezeichner##Test::RunTest(const FString& Parameters) \
	{ \
		MBTest::FKontext Ctx(this); \
		Bezeichner##_Lauf(Ctx); \
		return Ctx.Erfolgreich(); \
	} \
	static void Bezeichner##_Lauf(MBTest::FKontext& Ctx)

#endif

#define MB_PRUEFE(Bedingung) Ctx.Pruefe(static_cast<bool>(Bedingung), #Bedingung, __FILE__, __LINE__)
#define MB_PRUEFE_NAH(Ist, Soll, Toleranz) Ctx.PruefeNah(static_cast<double>(Ist), static_cast<double>(Soll), static_cast<double>(Toleranz), #Ist, #Soll, __FILE__, __LINE__)
#define MB_PRUEFE_GLEICH(Ist, Soll) Ctx.Pruefe((Ist) == (Soll), #Ist " == " #Soll, __FILE__, __LINE__)

// Formatierte Info-Zeile (nur fuer Testausgaben)
#define MB_INFO(...) \
	do \
	{ \
		char MBInfoPuffer[512]; \
		std::snprintf(MBInfoPuffer, sizeof(MBInfoPuffer), __VA_ARGS__); \
		Ctx.Info(MBInfoPuffer); \
	} while (false)
