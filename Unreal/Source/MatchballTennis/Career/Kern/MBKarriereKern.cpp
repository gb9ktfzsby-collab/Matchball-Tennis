// Matchball Tennis - Kern: Karriere-Logik

#include "Career/Kern/MBKarriereKern.h"

#include <algorithm>

namespace MB
{
	int Gesamtwert(const FSpielerwerte& Werte, const FWertGewichte& Gewichte)
	{
		const double Summe = Gewichte.Summe();
		if (Summe <= 0.0)
		{
			return 0;
		}
		const double Gewichtet = Werte.Aufschlag * Gewichte.Aufschlag + Werte.Vorhand * Gewichte.Vorhand + Werte.Rueckhand * Gewichte.Rueckhand
			+ Werte.Volley * Gewichte.Volley + Werte.Schnelligkeit * Gewichte.Schnelligkeit + Werte.Ausdauer * Gewichte.Ausdauer
			+ Werte.Nerven * Gewichte.Nerven + Werte.Spin * Gewichte.Spin + Werte.Return * Gewichte.Return;
		const double Durchschnitt = Gewichtet / Summe;
		const int Gerundet = static_cast<int>(Durchschnitt + 0.5);
		return Begrenze(Gerundet, 0, 99);
	}

	FRanglistenEintrag RanglistenPunkte(int SpielerId, const std::vector<FTurnierErgebnis>& Ergebnisse, int AktuelleWoche, const FRanglistenRegeln& Regeln)
	{
		FRanglistenEintrag Eintrag;
		Eintrag.SpielerId = SpielerId;
		const int ErsteWoche = AktuelleWoche - Regeln.Wochen + 1;
		std::vector<int> Punkte;
		int BestesFinale = -1;
		for (const FTurnierErgebnis& Ergebnis : Ergebnisse)
		{
			if (Ergebnis.SpielerId != SpielerId || Ergebnis.Woche < ErsteWoche || Ergebnis.Woche > AktuelleWoche)
			{
				continue;
			}
			Eintrag.BestesErgebnis = MaxWert(Eintrag.BestesErgebnis, Ergebnis.Punkte);
			if (Ergebnis.bFinals && Regeln.bFinalsZusaetzlich)
			{
				BestesFinale = MaxWert(BestesFinale, Ergebnis.Punkte);
				continue;
			}
			Punkte.push_back(Ergebnis.Punkte);
		}
		std::sort(Punkte.begin(), Punkte.end(), [](int A, int B) { return A > B; });
		const std::size_t Anzahl = Punkte.size() < static_cast<std::size_t>(MaxWert(Regeln.BesteErgebnisse, 0)) ? Punkte.size() : static_cast<std::size_t>(MaxWert(Regeln.BesteErgebnisse, 0));
		for (std::size_t Index = 0; Index < Anzahl; ++Index)
		{
			Eintrag.Punkte += Punkte[Index];
		}
		Eintrag.GewerteteTurniere = static_cast<int>(Anzahl);
		if (BestesFinale >= 0)
		{
			Eintrag.Punkte += BestesFinale;
			++Eintrag.GewerteteTurniere;
		}
		return Eintrag;
	}

	std::vector<FRanglistenEintrag> BerechneRangliste(const std::vector<int>& Spieler, const std::vector<FTurnierErgebnis>& Ergebnisse, int AktuelleWoche, const FRanglistenRegeln& Regeln)
	{
		std::vector<FRanglistenEintrag> Liste;
		Liste.reserve(Spieler.size());
		for (int Id : Spieler)
		{
			Liste.push_back(RanglistenPunkte(Id, Ergebnisse, AktuelleWoche, Regeln));
		}
		std::sort(Liste.begin(), Liste.end(), [](const FRanglistenEintrag& A, const FRanglistenEintrag& B)
		{
			if (A.Punkte != B.Punkte)
			{
				return A.Punkte > B.Punkte;
			}
			if (A.BestesErgebnis != B.BestesErgebnis)
			{
				return A.BestesErgebnis > B.BestesErgebnis;
			}
			return A.SpielerId < B.SpielerId;
		});
		for (std::size_t Index = 0; Index < Liste.size(); ++Index)
		{
			Liste[Index].Rang = static_cast<int>(Index) + 1;
		}
		return Liste;
	}

	std::vector<int> KanonischeSetzordnung(int Feldgroesse)
	{
		std::vector<int> Ordnung = {1, 2};
		if (Feldgroesse < 2)
		{
			return std::vector<int>{1};
		}
		while (static_cast<int>(Ordnung.size()) < Feldgroesse)
		{
			const int Groesse = static_cast<int>(Ordnung.size());
			std::vector<int> Neu;
			Neu.reserve(static_cast<std::size_t>(Groesse) * 2u);
			for (int Nummer : Ordnung)
			{
				Neu.push_back(Nummer);
				Neu.push_back(2 * Groesse + 1 - Nummer);
			}
			Ordnung.swap(Neu);
		}
		// Untere Haelfte spiegeln: Nummer 2 steht dann ganz unten (Konvention auf der Tour)
		std::reverse(Ordnung.begin() + static_cast<std::ptrdiff_t>(Ordnung.size() / 2u), Ordnung.end());
		return Ordnung;
	}

	namespace
	{
		void MBKarriere_Mische(std::vector<int>& Werte, FZufall& Zufall)
		{
			for (int Index = static_cast<int>(Werte.size()) - 1; Index > 0; --Index)
			{
				const int Ziel = Zufall.Ganzzahl(0, Index);
				const int Merker = Werte[static_cast<std::size_t>(Index)];
				Werte[static_cast<std::size_t>(Index)] = Werte[static_cast<std::size_t>(Ziel)];
				Werte[static_cast<std::size_t>(Ziel)] = Merker;
			}
		}
	}

	std::vector<int> Auslosung(const std::vector<int>& TeilnehmerNachRang, int Feldgroesse, int AnzahlGesetzte, FZufall& Zufall)
	{
		const int Anzahl = static_cast<int>(TeilnehmerNachRang.size());
		int Feld = 2;
		while (Feld < Feldgroesse || Feld < Anzahl)
		{
			Feld *= 2;
		}
		std::vector<int> Positionen(static_cast<std::size_t>(Feld), -2);   // -2 = noch frei
		const int Freilose = Feld - Anzahl;
		int Gesetzte = MinWert(MaxWert(AnzahlGesetzte, Freilose), Anzahl);
		Gesetzte = MinWert(Gesetzte, Feld / 2);

		const std::vector<int> Kanon = KanonischeSetzordnung(Feld);
		// Setzgruppen: 1 | 2 | 3-4 | 5-8 | 9-16 | 17-32 | ...
		int GruppeVon = 1;
		int GruppeBis = 1;
		while (GruppeVon <= Gesetzte)
		{
			std::vector<int> Plaetze;
			for (int Position = 0; Position < Feld; ++Position)
			{
				const int Nummer = Kanon[static_cast<std::size_t>(Position)];
				if (Nummer >= GruppeVon && Nummer <= GruppeBis)
				{
					Plaetze.push_back(Position);
				}
			}
			MBKarriere_Mische(Plaetze, Zufall);
			const int Letzter = MinWert(GruppeBis, Gesetzte);
			for (int Setz = GruppeVon; Setz <= Letzter; ++Setz)
			{
				const int Platz = Plaetze[static_cast<std::size_t>(Setz - GruppeVon)];
				Positionen[static_cast<std::size_t>(Platz)] = TeilnehmerNachRang[static_cast<std::size_t>(Setz - 1)];
				// Freilos fuer die besten Gesetzten: Gegner in Runde 1 bleibt leer
				if (Setz <= Freilose)
				{
					Positionen[static_cast<std::size_t>(Platz ^ 1)] = -1;
				}
			}
			GruppeVon = GruppeBis + 1;
			GruppeBis = GruppeVon == 2 ? 2 : GruppeBis * 2;
		}

		// Ungesetzte zufaellig auf die freien Plaetze
		std::vector<int> Ungesetzte(TeilnehmerNachRang.begin() + Gesetzte, TeilnehmerNachRang.end());
		MBKarriere_Mische(Ungesetzte, Zufall);
		std::size_t Naechster = 0;
		for (int& Eintrag : Positionen)
		{
			if (Eintrag == -2)
			{
				Eintrag = Naechster < Ungesetzte.size() ? Ungesetzte[Naechster++] : -1;
			}
		}
		return Positionen;
	}

	double AufschlagPunktChance(const FSimSpieler& A, const FSimSpieler& B, const FSimParameter& Parameter)
	{
		const double GesamtA = static_cast<double>(Gesamtwert(A.Werte));
		const double GesamtB = static_cast<double>(Gesamtwert(B.Werte));
		double Chance = Parameter.GrundAufschlagPunkt;
		Chance += Parameter.AufschlagGegenReturn * (A.Werte.Aufschlag - B.Werte.Return);
		Chance += Parameter.GesamtDifferenz * (GesamtA - GesamtB);
		Chance += Parameter.FormEinfluss * (A.Form - B.Form);
		Chance += A.BelagBonus - B.BelagBonus;
		return Begrenze(Chance, Parameter.Min, Parameter.Max);
	}

	FSimErgebnis SimuliereMatch(const FSimSpieler& Spieler0, const FSimSpieler& Spieler1, const FZaehlRegeln& Regeln, const FSimParameter& Parameter, FZufall& Zufall)
	{
		FSimErgebnis Ergebnis;
		const FSimSpieler* Spieler[2] = {&Spieler0, &Spieler1};
		FZaehlweise Stand(Regeln, Zufall.Ganzzahl(0, 1));
		const double Chance[2] = {AufschlagPunktChance(Spieler0, Spieler1, Parameter), AufschlagPunktChance(Spieler1, Spieler0, Parameter)};
		for (int Punkt = 0; Punkt < 5000 && !Stand.IstBeendet(); ++Punkt)
		{
			const int Aufschlaeger = Stand.GetAufschlaeger();
			const int Rueckschlaeger = 1 - Aufschlaeger;
			double P = Chance[Aufschlaeger];
			// Druckpunkte: Nerven entscheiden ein wenig mit
			const FPunktBedeutung BedeutungAuf = Stand.Bedeutung(Aufschlaeger);
			const FPunktBedeutung BedeutungRueck = Stand.Bedeutung(Rueckschlaeger);
			const bool bDruck = BedeutungRueck.bBreakball || BedeutungAuf.bSatzball || BedeutungRueck.bSatzball || BedeutungAuf.bMatchball || BedeutungRueck.bMatchball;
			if (bDruck)
			{
				P += Parameter.NervenDruck * (Spieler[Aufschlaeger]->Werte.Nerven - Spieler[Rueckschlaeger]->Werte.Nerven);
				P = Begrenze(P, Parameter.Min, Parameter.Max);
			}
			const bool bAufschlaegerGewinnt = Zufall.Chance(P);
			const int Gewinner = bAufschlaegerGewinnt ? Aufschlaeger : Rueckschlaeger;
			if (bAufschlaegerGewinnt)
			{
				const double Faktor = Spieler[Aufschlaeger]->Werte.Aufschlag / 75.0;
				if (Zufall.Chance(0.10 * Faktor * Faktor))
				{
					++Ergebnis.Asse[Aufschlaeger];
				}
			}
			else
			{
				Zufall.Naechste();   // gleich viele Zufallszahlen pro Punkt
			}
			++Ergebnis.Punkte[Gewinner];
			Stand.PunktFuer(Gewinner);
		}
		Ergebnis.Sieger = Stand.GetSieger();
		Ergebnis.Saetze = Stand.GetAbgeschlosseneSaetze();
		Ergebnis.Stand = Stand.StandText();
		return Ergebnis;
	}
}
