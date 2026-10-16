// Matchball Tennis - Einstiegspunkt der eigenstaendigen Kern-Tests (ohne Unreal)
//
// Aufruf: MatchballKernTests [Filter]
//   Filter = Teilstring des Testnamens, z. B. "Zaehlweise" oder "Ballphysik.Aufsprung"

#include "Kern/MBTestRahmen.h"

#include <cstdio>
#include <cstring>

int main(int Anzahl, char** Argumente)
{
	const char* Filter = Anzahl > 1 ? Argumente[1] : nullptr;
	int Tests = 0;
	int Fehlgeschlagen = 0;
	int Pruefungen = 0;
	for (const MBTest::FEintrag& Eintrag : MBTest::Verzeichnis())
	{
		if (Filter != nullptr && std::strstr(Eintrag.Name, Filter) == nullptr)
		{
			continue;
		}
		++Tests;
		std::printf("[ TEST ] %s\n", Eintrag.Name);
		MBTest::FKontext Ctx(Eintrag.Name);
		Eintrag.Funktion(Ctx);
		Pruefungen += Ctx.GetPruefungen();
		if (Ctx.Erfolgreich())
		{
			std::printf("[  OK  ] %s (%d Pruefungen)\n", Eintrag.Name, Ctx.GetPruefungen());
		}
		else
		{
			++Fehlgeschlagen;
			std::printf("[FEHLER] %s (%d von %d Pruefungen fehlgeschlagen)\n", Eintrag.Name, Ctx.GetFehler(), Ctx.GetPruefungen());
		}
	}
	std::printf("\n%d Tests, %d Pruefungen, %d fehlgeschlagen\n", Tests, Pruefungen, Fehlgeschlagen);
	return Fehlgeschlagen == 0 && Tests > 0 ? 0 : 1;
}
