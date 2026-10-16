// Matchball Tennis - Tests der Zaehlweise (README 8)

#include "Kern/MBTestRahmen.h"

#include "Match/Kern/MBZaehlweise.h"

using namespace MB;

namespace
{
	// Spielt ein komplettes Spiel (4 Punkte in Folge) fuer Spieler und liefert die Folge des letzten Punkts
	FPunktFolge MBZaehlTest_Spiel(FZaehlweise& Z, int Spieler)
	{
		FPunktFolge Folge;
		for (int Index = 0; Index < 4; ++Index)
		{
			Folge = Z.PunktFuer(Spieler);
		}
		return Folge;
	}

	// Spielt Spiele abwechselnd so, dass am Ende der Stand A:B im Satz erreicht ist (A, B <= 5 bzw. 6:6 ueber 5:5)
	void MBZaehlTest_SpieleBis(FZaehlweise& Z, int A, int B)
	{
		int GespieltA = 0;
		int GespieltB = 0;
		while (GespieltA < A || GespieltB < B)
		{
			if (GespieltA < A && (GespieltA <= GespieltB || GespieltB >= B))
			{
				MBZaehlTest_Spiel(Z, 0);
				++GespieltA;
			}
			else
			{
				MBZaehlTest_Spiel(Z, 1);
				++GespieltB;
			}
		}
	}
}

MB_TEST(ZaehlweisePunkteUndSpiel, "Zaehlweise.PunkteUndSpiel")
{
	FZaehlweise Z(FZaehlRegeln(), 0);
	MB_PRUEFE_GLEICH(Z.GetAufschlaeger(), 0);
	MB_PRUEFE_GLEICH(Z.PunktText(0), std::string("0"));
	Z.PunktFuer(0);
	MB_PRUEFE_GLEICH(Z.PunktText(0), std::string("15"));
	MB_PRUEFE(Z.GetAufschlagHaelfte() == EAufschlagHaelfte::Vorteil);
	Z.PunktFuer(0);
	MB_PRUEFE_GLEICH(Z.PunktText(0), std::string("30"));
	MB_PRUEFE(Z.GetAufschlagHaelfte() == EAufschlagHaelfte::Einstand);
	Z.PunktFuer(0);
	MB_PRUEFE_GLEICH(Z.PunktText(0), std::string("40"));
	const FPunktFolge Folge = Z.PunktFuer(0);
	MB_PRUEFE(Folge.bSpielGewonnen);
	MB_PRUEFE_GLEICH(Folge.Spielgewinner, 0);
	MB_PRUEFE(!Folge.bBreak);
	MB_PRUEFE_GLEICH(Z.GetSpiele(0), 1);
	MB_PRUEFE_GLEICH(Z.GetPunkte(0), 0);
	// Nach dem 1. Spiel: Seitenwechsel ohne Pause, Aufschlagwechsel
	MB_PRUEFE(Folge.bSeitenwechsel);
	MB_PRUEFE(Folge.Pause == EPause::Keine);
	MB_PRUEFE(Folge.bAufschlagwechsel);
	MB_PRUEFE_GLEICH(Z.GetAufschlaeger(), 1);
	MB_PRUEFE(Z.GetSeite(0) == EPlatzseite::Fern);
	MB_PRUEFE(Z.GetSeite(1) == EPlatzseite::Nah);
}

MB_TEST(ZaehlweiseEinstandVorteil, "Zaehlweise.EinstandVorteil")
{
	FZaehlweise Z(FZaehlRegeln(), 0);
	for (int Index = 0; Index < 3; ++Index)
	{
		Z.PunktFuer(0);
		Z.PunktFuer(1);
	}
	MB_PRUEFE(Z.IstEinstand());
	MB_PRUEFE(Z.GetAufschlagHaelfte() == EAufschlagHaelfte::Einstand);
	Z.PunktFuer(1);
	MB_PRUEFE_GLEICH(Z.GetVorteil(), 1);
	MB_PRUEFE_GLEICH(Z.PunktText(1), std::string("AD"));
	MB_PRUEFE_GLEICH(Z.PunktText(0), std::string("40"));
	MB_PRUEFE(Z.GetAufschlagHaelfte() == EAufschlagHaelfte::Vorteil);
	MB_PRUEFE(Z.Bedeutung(1).bBreakball);
	MB_PRUEFE(!Z.Bedeutung(0).bBreakball);
	Z.PunktFuer(0);
	MB_PRUEFE(Z.IstEinstand());
	// Viele Einstaende hintereinander
	for (int Index = 0; Index < 20; ++Index)
	{
		Z.PunktFuer(Index % 2);
	}
	MB_PRUEFE(Z.IstEinstand());
	MB_PRUEFE_GLEICH(Z.GetSpiele(0) + Z.GetSpiele(1), 0);
	Z.PunktFuer(1);
	const FPunktFolge Folge = Z.PunktFuer(1);
	MB_PRUEFE(Folge.bSpielGewonnen);
	MB_PRUEFE(Folge.bBreak);
	MB_PRUEFE_GLEICH(Z.GetSpiele(1), 1);
}

MB_TEST(ZaehlweiseNoAd, "Zaehlweise.NoAd")
{
	FZaehlRegeln Regeln;
	Regeln.bNoAd = true;
	FZaehlweise Z(Regeln, 0);
	for (int Index = 0; Index < 3; ++Index)
	{
		Z.PunktFuer(0);
		Z.PunktFuer(1);
	}
	MB_PRUEFE(!Z.IstEinstand());
	MB_PRUEFE(Z.Bedeutung(0).bEntscheidungspunkt);
	MB_PRUEFE(Z.Bedeutung(1).bBreakball);
	MB_PRUEFE(Z.GetAufschlagHaelfte() == EAufschlagHaelfte::Einstand);
	MB_PRUEFE_GLEICH(Z.PunktText(0), std::string("40"));
	const FPunktFolge Folge = Z.PunktFuer(1);
	MB_PRUEFE(Folge.bSpielGewonnen);
	MB_PRUEFE_GLEICH(Folge.Spielgewinner, 1);
}

MB_TEST(ZaehlweiseSatzUndSeitenwechsel, "Zaehlweise.SatzUndSeitenwechsel")
{
	FZaehlweise Z(FZaehlRegeln(), 0);
	// Seitenwechsel nach Spiel 1, 3, 5, ...; Pause ab Spiel 3
	int Spieler = 0;
	for (int Spiel = 1; Spiel <= 9; ++Spiel)
	{
		const FPunktFolge Folge = MBZaehlTest_Spiel(Z, Spieler);
		const bool bUngerade = (Spiel % 2) == 1;
		MB_PRUEFE_GLEICH(Folge.bSeitenwechsel, bUngerade);
		if (bUngerade)
		{
			MB_PRUEFE(Folge.Pause == (Spiel == 1 ? EPause::Keine : EPause::Seitenwechsel));
		}
		else
		{
			MB_PRUEFE(Folge.Pause == EPause::Keine);
		}
		Spieler = 1 - Spieler;
	}
	// Stand 5:4 fuer Spieler 0; Spieler 1 schlaegt auf -> 0:40 ist Break- und Satzball
	MB_PRUEFE_GLEICH(Z.GetSpiele(0), 5);
	MB_PRUEFE_GLEICH(Z.GetSpiele(1), 4);
	MB_PRUEFE_GLEICH(Z.GetAufschlaeger(), 1);
	MB_PRUEFE(!Z.Bedeutung(0).bSatzball);
	Z.PunktFuer(0);
	Z.PunktFuer(0);
	Z.PunktFuer(0);
	MB_PRUEFE(Z.Bedeutung(0).bSatzball);
	MB_PRUEFE(Z.Bedeutung(0).bBreakball);
	MB_PRUEFE(!Z.Bedeutung(0).bMatchball);
	const FPunktFolge Folge = Z.PunktFuer(0);
	MB_PRUEFE(Folge.bSatzGewonnen);
	MB_PRUEFE_GLEICH(Folge.Satzgewinner, 0);
	MB_PRUEFE(Folge.Pause == EPause::Satzpause);
	// 10 Spiele (gerade): kein Seitenwechsel am Satzende, dafuer nach dem 1. Spiel des neuen Satzes
	MB_PRUEFE(!Folge.bSeitenwechsel);
	MB_PRUEFE_GLEICH(Z.GetGewonneneSaetze(0), 1);
	MB_PRUEFE_GLEICH(Z.GetSpiele(0), 0);
	MB_PRUEFE_GLEICH(Z.GetSatzNummer(), 2);
	MB_PRUEFE_GLEICH(Z.GetAbgeschlosseneSaetze().size(), static_cast<std::size_t>(1));
	MB_PRUEFE_GLEICH(Z.GetAbgeschlosseneSaetze()[0].Spiele[0], 6);
	MB_PRUEFE_GLEICH(Z.GetAbgeschlosseneSaetze()[0].Spiele[1], 4);
	// 10 Spiele -> Aufschlaeger des 11. Spiels ist wieder Spieler 0
	MB_PRUEFE_GLEICH(Z.GetAufschlaeger(), 0);
	const FPunktFolge Erstes = MBZaehlTest_Spiel(Z, 0);
	MB_PRUEFE(Erstes.bSeitenwechsel);
	MB_PRUEFE(Erstes.Pause == EPause::Keine);
}

MB_TEST(ZaehlweiseSatzUngeradeEnde, "Zaehlweise.SatzEndeUngeradeSeitenwechsel")
{
	FZaehlweise Z(FZaehlRegeln(), 0);
	MBZaehlTest_SpieleBis(Z, 5, 3);
	const FPunktFolge Folge = MBZaehlTest_Spiel(Z, 0);
	MB_PRUEFE(Folge.bSatzGewonnen);
	MB_PRUEFE(Folge.bSeitenwechsel);         // 9 Spiele: Wechsel am Satzende
	MB_PRUEFE(Folge.Pause == EPause::Satzpause);
}

MB_TEST(ZaehlweiseSiebenFuenf, "Zaehlweise.SiebenZuFuenf")
{
	FZaehlweise Z(FZaehlRegeln(), 0);
	MBZaehlTest_SpieleBis(Z, 5, 5);
	FPunktFolge Folge = MBZaehlTest_Spiel(Z, 0);
	MB_PRUEFE(!Folge.bSatzGewonnen);
	MB_PRUEFE(!Z.IstTiebreak());
	Folge = MBZaehlTest_Spiel(Z, 0);
	MB_PRUEFE(Folge.bSatzGewonnen);
	MB_PRUEFE_GLEICH(Z.GetAbgeschlosseneSaetze()[0].Spiele[0], 7);
	MB_PRUEFE_GLEICH(Z.GetAbgeschlosseneSaetze()[0].Spiele[1], 5);
}

MB_TEST(ZaehlweiseTiebreak, "Zaehlweise.Tiebreak")
{
	FZaehlweise Z(FZaehlRegeln(), 0);
	MBZaehlTest_SpieleBis(Z, 6, 5);
	const FPunktFolge Folge = MBZaehlTest_Spiel(Z, 1);
	MB_PRUEFE(Folge.bTiebreakBeginnt);
	MB_PRUEFE(Z.IstTiebreak());
	MB_PRUEFE_GLEICH(Z.GetTiebreakZiel(), 7);
	// 12 Spiele gespielt -> Spieler 0 schlaegt im Tiebreak zuerst auf
	const int Erster = Z.GetAufschlaeger();
	MB_PRUEFE_GLEICH(Erster, 0);
	const EPlatzseite SeiteVorher = Z.GetSeite(0);

	// Aufschlagfolge: A, B, B, A, A, B, B, ...
	const int Erwartet[12] = {0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0};
	for (int Punkt = 0; Punkt < 12; ++Punkt)
	{
		MB_PRUEFE_GLEICH(Z.GetAufschlaeger(), Erwartet[Punkt]);
		MB_PRUEFE(Z.GetAufschlagHaelfte() == (Punkt % 2 == 0 ? EAufschlagHaelfte::Einstand : EAufschlagHaelfte::Vorteil));
		const FPunktFolge PunktFolge = Z.PunktFuer(Punkt % 2);   // 6:6 nach 12 Punkten
		// Seitenwechsel nach 6 und 12 Punkten, ohne Pause
		MB_PRUEFE_GLEICH(PunktFolge.bSeitenwechsel, (Punkt + 1) % 6 == 0);
		MB_PRUEFE(PunktFolge.Pause == EPause::Keine);
	}
	MB_PRUEFE(Z.GetSeite(0) == SeiteVorher);   // zweimal gewechselt
	MB_PRUEFE_GLEICH(Z.GetPunkte(0), 6);
	MB_PRUEFE_GLEICH(Z.GetPunkte(1), 6);
	MB_PRUEFE(Z.Bedeutung(0).bSatzball == false);
	Z.PunktFuer(1);   // 6:7
	MB_PRUEFE(Z.Bedeutung(1).bSatzball);
	MB_PRUEFE(!Z.Bedeutung(1).bBreakball);
	Z.PunktFuer(0);   // 7:7
	Z.PunktFuer(1);   // 7:8
	const FPunktFolge Ende = Z.PunktFuer(1);   // 7:9
	MB_PRUEFE(Ende.bSatzGewonnen);
	MB_PRUEFE_GLEICH(Ende.Satzgewinner, 1);
	MB_PRUEFE(Ende.bSeitenwechsel);         // 13 Spiele: Wechsel am Ende des Tiebreaks
	MB_PRUEFE(Ende.Pause == EPause::Satzpause);
	const FSatzErgebnis& Satz = Z.GetAbgeschlosseneSaetze()[0];
	MB_PRUEFE_GLEICH(Satz.Spiele[0], 6);
	MB_PRUEFE_GLEICH(Satz.Spiele[1], 7);
	MB_PRUEFE(Satz.bMitTiebreak);
	MB_PRUEFE_GLEICH(Satz.TiebreakPunkte[0], 7);
	MB_PRUEFE_GLEICH(Satz.TiebreakPunkte[1], 9);
	// Wer im Tiebreak zuerst aufschlug, nimmt im ersten Spiel des naechsten Satzes auf
	MB_PRUEFE_GLEICH(Z.GetAufschlaeger(), 1 - Erster);
	MB_PRUEFE_GLEICH(Z.StandText(), std::string("6-7(7) 0-0 0-0"));
}

MB_TEST(ZaehlweiseMatchZweiGewinnsaetze, "Zaehlweise.MatchZweiGewinnsaetze")
{
	FZaehlweise Z(FZaehlRegeln(), 1);
	MBZaehlTest_SpieleBis(Z, 5, 4);
	MBZaehlTest_Spiel(Z, 0);
	MB_PRUEFE_GLEICH(Z.GetGewonneneSaetze(0), 1);
	MBZaehlTest_SpieleBis(Z, 5, 3);
	// Matchball fuer Spieler 0
	for (int Index = 0; Index < 3; ++Index)
	{
		Z.PunktFuer(0);
	}
	MB_PRUEFE(Z.Bedeutung(0).bMatchball);
	MB_PRUEFE(!Z.Bedeutung(0).bSatzball);
	const FPunktFolge Folge = Z.PunktFuer(0);
	MB_PRUEFE(Folge.bMatchGewonnen);
	MB_PRUEFE_GLEICH(Folge.Matchgewinner, 0);
	MB_PRUEFE(Z.IstBeendet());
	MB_PRUEFE_GLEICH(Z.GetSieger(), 0);
	MB_PRUEFE_GLEICH(Z.StandText(), std::string("6-4 6-3"));
	// Nach Matchende keine Wirkung mehr
	const FPunktFolge Danach = Z.PunktFuer(1);
	MB_PRUEFE_GLEICH(Danach.Punktgewinner, -1);
	MB_PRUEFE_GLEICH(Z.GetSieger(), 0);
}

MB_TEST(ZaehlweiseDreiGewinnsaetze, "Zaehlweise.DreiGewinnsaetze")
{
	FZaehlRegeln Regeln;
	Regeln.Gewinnsaetze = 3;
	FZaehlweise Z(Regeln, 0);
	for (int Satz = 0; Satz < 4; ++Satz)
	{
		const int Gewinner = Satz == 1 ? 1 : 0;
		for (int Spiel = 0; Spiel < 6; ++Spiel)
		{
			MBZaehlTest_Spiel(Z, Gewinner);
		}
	}
	MB_PRUEFE(Z.IstBeendet());
	MB_PRUEFE_GLEICH(Z.GetSieger(), 0);
	MB_PRUEFE_GLEICH(Z.GetGewonneneSaetze(0), 3);
	MB_PRUEFE_GLEICH(Z.GetGewonneneSaetze(1), 1);
}

MB_TEST(ZaehlweiseMatchTiebreak, "Zaehlweise.MatchTiebreak")
{
	FZaehlRegeln Regeln;
	Regeln.Entscheidungssatz = EEntscheidungssatz::MatchTiebreak;
	FZaehlweise Z(Regeln, 0);
	for (int Spiel = 0; Spiel < 6; ++Spiel)
	{
		MBZaehlTest_Spiel(Z, 0);
	}
	FPunktFolge Folge;
	for (int Spiel = 0; Spiel < 6; ++Spiel)
	{
		Folge = MBZaehlTest_Spiel(Z, 1);
	}
	MB_PRUEFE(Folge.bSatzGewonnen);
	MB_PRUEFE(Folge.bMatchTiebreakBeginnt);
	MB_PRUEFE(Z.IstMatchTiebreak());
	MB_PRUEFE_GLEICH(Z.GetTiebreakZiel(), 10);
	for (int Index = 0; Index < 9; ++Index)
	{
		Z.PunktFuer(0);
		Z.PunktFuer(1);
	}
	// 9:9 -> 10:9 -> 11:9
	MB_PRUEFE(!Z.IstBeendet());
	Z.PunktFuer(0);
	MB_PRUEFE(Z.Bedeutung(0).bMatchball);
	MB_PRUEFE(!Z.IstBeendet());
	Folge = Z.PunktFuer(0);
	MB_PRUEFE(Folge.bMatchGewonnen);
	MB_PRUEFE_GLEICH(Z.GetSieger(), 0);
	const FSatzErgebnis& Letzter = Z.GetAbgeschlosseneSaetze().back();
	MB_PRUEFE(Letzter.bMatchTiebreak);
	MB_PRUEFE_GLEICH(Letzter.TiebreakPunkte[0], 11);
	MB_PRUEFE_GLEICH(Letzter.TiebreakPunkte[1], 9);
	MB_PRUEFE_GLEICH(Z.StandText(), std::string("6-0 0-6 [11-9]"));
}

MB_TEST(ZaehlweiseEntscheidungssatzVarianten, "Zaehlweise.Entscheidungssatz")
{
	// Tiebreak bis 10 bei 6:6 im dritten Satz, normale Tiebreaks davor
	{
		FZaehlRegeln Regeln;
		Regeln.Entscheidungssatz = EEntscheidungssatz::Tiebreak10;
		FZaehlweise Z(Regeln, 0);
		MBZaehlTest_SpieleBis(Z, 6, 5);
		MBZaehlTest_Spiel(Z, 1);
		MB_PRUEFE(Z.IstTiebreak());
		MB_PRUEFE_GLEICH(Z.GetTiebreakZiel(), 7);
		for (int Index = 0; Index < 7; ++Index)
		{
			Z.PunktFuer(0);
		}
		MB_PRUEFE_GLEICH(Z.GetGewonneneSaetze(0), 1);
		for (int Spiel = 0; Spiel < 6; ++Spiel)
		{
			MBZaehlTest_Spiel(Z, 1);
		}
		MB_PRUEFE(Z.IstEntscheidungssatz());
		MBZaehlTest_SpieleBis(Z, 6, 5);
		MBZaehlTest_Spiel(Z, 1);
		MB_PRUEFE(Z.IstTiebreak());
		MB_PRUEFE_GLEICH(Z.GetTiebreakZiel(), 10);
		for (int Index = 0; Index < 7; ++Index)
		{
			Z.PunktFuer(0);
		}
		MB_PRUEFE(!Z.IstBeendet());
		for (int Index = 0; Index < 3; ++Index)
		{
			Z.PunktFuer(0);
		}
		MB_PRUEFE(Z.IstBeendet());
		MB_PRUEFE_GLEICH(Z.GetSieger(), 0);
	}
	// Ausspielen: kein Tiebreak im Entscheidungssatz
	{
		FZaehlRegeln Regeln;
		Regeln.Gewinnsaetze = 1;
		Regeln.Entscheidungssatz = EEntscheidungssatz::Ausspielen;
		FZaehlweise Z(Regeln, 0);
		MBZaehlTest_SpieleBis(Z, 6, 6);
		MB_PRUEFE(!Z.IstTiebreak());
		MBZaehlTest_Spiel(Z, 0);
		MBZaehlTest_Spiel(Z, 1);
		MBZaehlTest_Spiel(Z, 0);
		MB_PRUEFE(!Z.IstBeendet());
		MBZaehlTest_Spiel(Z, 0);
		MB_PRUEFE(Z.IstBeendet());
		MB_PRUEFE_GLEICH(Z.StandText(), std::string("9-7"));
	}
}

MB_TEST(ZaehlweiseKurzeSaetze, "Zaehlweise.KurzeSaetze")
{
	// 4 Spiele pro Satz: Tiebreak bei 4:4, Satz bei 4:2
	{
		FZaehlRegeln Regeln;
		Regeln.SpieleProSatz = 4;
		FZaehlweise Z(Regeln, 0);
		MBZaehlTest_SpieleBis(Z, 3, 2);
		MBZaehlTest_Spiel(Z, 0);
		MB_PRUEFE_GLEICH(Z.GetGewonneneSaetze(0), 1);
		MBZaehlTest_SpieleBis(Z, 4, 3);
		MBZaehlTest_Spiel(Z, 1);
		MB_PRUEFE(Z.IstTiebreak());
	}
	// 1 Spiel pro Satz: das Spiel entscheidet den Satz
	{
		FZaehlRegeln Regeln;
		Regeln.SpieleProSatz = 1;
		Regeln.Gewinnsaetze = 2;
		FZaehlweise Z(Regeln, 0);
		const FPunktFolge Folge = MBZaehlTest_Spiel(Z, 1);
		MB_PRUEFE(Folge.bSatzGewonnen);
		MB_PRUEFE(Folge.bSeitenwechsel);   // 1 Spiel = ungerade
		MBZaehlTest_Spiel(Z, 1);
		MB_PRUEFE(Z.IstBeendet());
		MB_PRUEFE_GLEICH(Z.GetSieger(), 1);
	}
	// Ungueltige Werte werden begrenzt
	{
		FZaehlRegeln Regeln;
		Regeln.SpieleProSatz = 9;
		Regeln.Gewinnsaetze = 0;
		FZaehlweise Z(Regeln, 0);
		MB_PRUEFE_GLEICH(Z.GetRegeln().SpieleProSatz, 6);
		MB_PRUEFE_GLEICH(Z.GetRegeln().Gewinnsaetze, 1);
	}
}

MB_TEST(ZaehlweiseNeueBaelle, "Zaehlweise.NeueBaelle")
{
	// Saetze 6:0, 0:6, 6:0 (ohne Tiebreak): neue Baelle nach Spiel 7 und 16
	FZaehlRegeln Dreisatz;
	Dreisatz.Gewinnsaetze = 3;
	FZaehlweise Z(Dreisatz, 0);
	int Spiel = 0;
	int ErstesMal = -1;
	int ZweitesMal = -1;
	for (int Index = 0; Index < 18 && !Z.IstBeendet(); ++Index)
	{
		const FPunktFolge Folge = MBZaehlTest_Spiel(Z, (Index / 6) % 2);
		++Spiel;
		if (Folge.bNeueBaelle)
		{
			if (ErstesMal < 0)
			{
				ErstesMal = Spiel;
			}
			else if (ZweitesMal < 0)
			{
				ZweitesMal = Spiel;
			}
		}
	}
	MB_PRUEFE_GLEICH(ErstesMal, 7);
	MB_PRUEFE_GLEICH(ZweitesMal, 16);

	// Faellig vor einem Tiebreak: Aufschub ins zweite Spiel des naechsten Satzes
	FZaehlRegeln Regeln;
	Regeln.NeueBaelleErstesMal = 12;
	FZaehlweise T(Regeln, 0);
	MBZaehlTest_SpieleBis(T, 6, 5);
	const FPunktFolge VorTiebreak = MBZaehlTest_Spiel(T, 1);
	MB_PRUEFE(VorTiebreak.bTiebreakBeginnt);
	MB_PRUEFE(!VorTiebreak.bNeueBaelle);
	FPunktFolge Folge;
	for (int Index = 0; Index < 7; ++Index)
	{
		Folge = T.PunktFuer(0);
	}
	MB_PRUEFE(Folge.bSatzGewonnen);
	MB_PRUEFE(!Folge.bNeueBaelle);
	Folge = MBZaehlTest_Spiel(T, 0);
	MB_PRUEFE(Folge.bNeueBaelle);
}

MB_TEST(ZaehlweiseBreakballBedeutung, "Zaehlweise.Bedeutung")
{
	FZaehlweise Z(FZaehlRegeln(), 0);
	Z.PunktFuer(1);
	Z.PunktFuer(1);
	Z.PunktFuer(0);
	Z.PunktFuer(1);   // 15:40
	MB_PRUEFE(Z.Bedeutung(1).bBreakball);
	MB_PRUEFE(!Z.Bedeutung(0).bBreakball);
	MB_PRUEFE(!Z.Bedeutung(1).bSatzball);
	MB_PRUEFE_GLEICH(Z.StandText(), std::string("0-0 15-40"));
}
