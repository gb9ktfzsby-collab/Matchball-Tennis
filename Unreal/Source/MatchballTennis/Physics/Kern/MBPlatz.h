// Matchball Tennis - Kern: Platzgeometrie nach ITF-Regel 1 (README 12.1 und 24.1)
//
// Alle Masse werden - wie in den ITF-Regeln - bis zur Aussenkante der Linien gemessen;
// die Linien liegen also innerhalb der Felder und gehoeren zum Feld ("Linie = gut").
// Koordinaten siehe Core/Kern/MBMathe.h (Netz bei Y = 0, nahe Seite Y < 0).

#pragma once

#include "Core/Kern/MBMathe.h"

namespace MB
{
	namespace Platzmass
	{
		constexpr double Laenge = 23.77;
		constexpr double HalbeLaenge = Laenge * 0.5;               // 11,885 m
		constexpr double BreiteEinzel = 8.23;
		constexpr double HalbeBreiteEinzel = BreiteEinzel * 0.5;   // 4,115 m
		constexpr double BreiteDoppel = 10.97;
		constexpr double HalbeBreiteDoppel = BreiteDoppel * 0.5;   // 5,485 m
		constexpr double AufschlaglinieVomNetz = 6.40;
		constexpr double NetzhoeheMitte = 0.914;
		constexpr double NetzhoehePfosten = 1.07;
		constexpr double PfostenAusserhalbDoppel = 0.914;
		constexpr double PfostenX = HalbeBreiteDoppel + PfostenAusserhalbDoppel;              // 6,399 m
		constexpr double EinzelstuetzeAusserhalbEinzel = 0.914;
		constexpr double EinzelstuetzeX = HalbeBreiteEinzel + EinzelstuetzeAusserhalbEinzel;  // 5,029 m
		constexpr double MittelmarkeLaenge = 0.10;
		constexpr double MittelmarkeBreite = 0.05;
		constexpr double AuslaufHintenMin = 6.40;
		constexpr double AuslaufSeitlichMin = 3.66;
	}

	// Linienbreiten (ITF: 2,5-5 cm, Grundlinie bis 10 cm; Mittellinie und Mittelmarke 5 cm)
	struct FLinienbreiten
	{
		double Grundlinie = 0.10;
		double Seitenlinie = 0.05;
		double Aufschlaglinie = 0.05;
		double Mittellinie = 0.05;
	};

	enum class EPlatzseite : unsigned char
	{
		Nah,   // Y < 0
		Fern   // Y > 0
	};

	constexpr EPlatzseite Gegenseite(EPlatzseite Seite)
	{
		return Seite == EPlatzseite::Nah ? EPlatzseite::Fern : EPlatzseite::Nah;
	}

	// +1 fuer die ferne Seite, -1 fuer die nahe Seite (Richtung "von der Mitte weg")
	constexpr double SeitenVorzeichen(EPlatzseite Seite)
	{
		return Seite == EPlatzseite::Fern ? 1.0 : -1.0;
	}

	// Welche Haelfte des Aufschlagfelds: Einstand (rechts vom Aufschlaeger) oder Vorteil (links)
	enum class EAufschlagHaelfte : unsigned char
	{
		Einstand,
		Vorteil
	};

	// Benannte Linien (fuer Ansagen und Hawk-Eye-Anzeige)
	enum class ELinie : unsigned char
	{
		Keine,
		Grundlinie,
		SeitenlinieLinks,   // aus Sicht der nahen Grundlinie: X < 0
		SeitenlinieRechts,  // X > 0
		Aufschlaglinie,
		Mittellinie
	};

	// Achsparalleles Feld inklusive Linien. Die Seite am Netz (Y = 0) ist keine Linie.
	struct FFeld
	{
		double MinX = 0.0;
		double MaxX = 0.0;
		double MinY = 0.0;
		double MaxY = 0.0;

		// Welche Kante welcher Linie entspricht (ELinie::Keine = Netzseite)
		ELinie LinieMinX = ELinie::Keine;
		ELinie LinieMaxX = ELinie::Keine;
		ELinie LinieMinY = ELinie::Keine;
		ELinie LinieMaxY = ELinie::Keine;

		bool Enthaelt(double X, double Y) const { return X >= MinX && X <= MaxX && Y >= MinY && Y <= MaxY; }

		// Vorzeichenbehafteter Abstand eines Punkts zum Feldrand:
		//  > 0: ausserhalb (euklidischer Abstand zum Feld)
		//  <= 0: innerhalb; Betrag = Abstand zur naechsten LINIE (die Netzseite zaehlt nicht)
		// NaechsteLinie liefert die massgebliche Linie.
		double VorzeichenAbstand(double X, double Y, ELinie* NaechsteLinie) const;
	};

	class FPlatz
	{
	public:
		FPlatz() = default;
		explicit FPlatz(const FLinienbreiten& InLinien) : Linien(InLinien) {}

		const FLinienbreiten& GetLinien() const { return Linien; }

		// Einzelfeld auf der angegebenen Seite (Ziel eines Grundschlags dorthin)
		FFeld Einzelfeld(EPlatzseite Seite) const;

		// Aufschlagfeld, in das ein Aufschlaeger von der Seite AufschlagSeite in der angegebenen
		// Haelfte (Einstand = er steht rechts der Mittelmarke) diagonal aufschlagen muss.
		FFeld Aufschlagfeld(EPlatzseite AufschlagSeite, EAufschlagHaelfte Haelfte) const;

		// Netzoberkante (Oberkante des Netzbands) an Position X.
		// Vereinfachung: geradlinig von 0,914 m (Mitte) zu 1,07 m an den Einzelstuetzen, aussen konstant.
		static double NetzOberkante(double X);

		// Breite des Netzes (bis zu den Pfosten)
		static constexpr double NetzHalbeBreite() { return Platzmass::PfostenX; }

		// Standardposition des Aufschlaegers (Fussmitte), knapp hinter der Grundlinie
		FVek3 AufschlagPosition(EPlatzseite Seite, EAufschlagHaelfte Haelfte, double AbstandZurMitte = 0.8) const;

	private:
		FLinienbreiten Linien;
	};
}
