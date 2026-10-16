// Matchball Tennis - Zeilen-Strukturen der Data Tables (README 0.2 "datengetrieben")
//
// Jede Struktur entspricht einer CSV-Datei in Data/. Die Spaltennamen der CSV muessen exakt den
// Eigenschaftsnamen entsprechen; die erste Spalte ("Name") ist der Zeilenname.
// Import: Tools/Python/import_daten.py legt daraus /Game/Data/DT_<Name> an.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"

#include "MBDatenTabellen.generated.h"

// Data/belaege.csv
USTRUCT(BlueprintType)
struct MATCHBALLTENNIS_API FMBBelagZeile : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Belag")
	FString Anzeigename;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Belag")
	double Reibung = 0.58;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Belag")
	double Stosszahl = 0.82;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Belag")
	double StosszahlReferenz = 8.27;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Belag")
	double StosszahlAbnahme = 0.008;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Belag")
	double RollReibung = 0.6;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Belag")
	double Rutschweite = 0.35;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Belag")
	bool bQuietschen = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Belag")
	bool bStaub = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Belag")
	bool bFussabdruecke = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Belag")
	bool bHalle = false;
};

// Data/ball.csv
USTRUCT(BlueprintType)
struct MATCHBALLTENNIS_API FMBBallZeile : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball")
	double Masse = 0.0577;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball")
	double Radius = 0.0335;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball")
	double Luftdichte = 1.204;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball")
	double Widerstandsbeiwert = 0.507;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball")
	double TraegheitsFaktor = 0.55;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball")
	double DrallAbnahmeProMeter = 0.004;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball")
	double Schwerkraft = 9.81;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball")
	double BallSteifigkeit = 30000.0;
};

// Data/netz.csv
USTRUCT(BlueprintType)
struct MATCHBALLTENNIS_API FMBNetzZeile : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Netz")
	double BandRadius = 0.010;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Netz")
	double BandStosszahl = 0.30;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Netz")
	double BandTangentialFaktor = 0.75;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Netz")
	double GeflechtStosszahl = 0.10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Netz")
	double GeflechtDaempfung = 0.25;
};

// Data/schlagarten.csv
USTRUCT(BlueprintType)
struct MATCHBALLTENNIS_API FMBSchlagartZeile : public FTableRowBase
{
	GENERATED_BODY()

	// Abfluggetempo bei Spielerwert 50 bzw. 99 (km/h); dazwischen linear
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Schlag")
	double TempoBeiWert50 = 90.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Schlag")
	double TempoBeiWert99 = 130.0;

	// Drall in U/min: > 0 Topspin, < 0 Unterschnitt
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Schlag")
	double DrallUpm = 1500.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Schlag")
	double SeitendrallUpm = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Schlag")
	bool bHoherBogen = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Schlag")
	double MinNetzAbstand = 0.10;

	// Standard-Ziellaenge: Abstand des Aufsprungs vom Netz (m)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Schlag")
	double ZielTiefe = 9.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Schlag")
	double PowerTempoFaktor = 1.12;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Schlag")
	double PraezisionTempoFaktor = 0.92;

	// Welcher Spielerwert bestimmt den Schlag (Vorhand, Rueckhand, Volley, Aufschlag)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Schlag")
	FString Wert = TEXT("Vorhand");
};

// Data/timing.csv
USTRUCT(BlueprintType)
struct MATCHBALLTENNIS_API FMBTimingZeile : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	double GruenHalbfenster_ms = 30.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	double GelbHalbfenster_ms = 75.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	double Grundstreuung = 0.20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	double SeitlichProSekunde = 22.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	double LaengenStreuung = 0.30;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	double LaengeProSekunde = 6.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	double PowerFenster = 0.70;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	double PowerStreuung = 1.45;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	double PraezisionFenster = 1.35;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	double PraezisionStreuung = 0.65;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	double FensterBeiWert50 = 0.85;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	double FensterBeiWert99 = 1.20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	double StreuungBeiWert50 = 1.40;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	double StreuungBeiWert99 = 0.70;

	// Volleys: groesseres Fenster (README 5.6); Halbvolley: kleines Fenster
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	double VolleyFensterFaktor = 1.30;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	double HalbvolleyFensterFaktor = 0.70;
};

// Data/ballmaschine.csv (Programme der Ballmaschine, README 13.2)
USTRUCT(BlueprintType)
struct MATCHBALLTENNIS_API FMBBallmaschineZeile : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballmaschine")
	FString Anzeigename;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballmaschine")
	double Tempo_kmh = 80.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballmaschine")
	double Drall_upm = 1500.0;

	// Abschusshoehe ueber dem Boden (m)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballmaschine")
	double Hoehe = 1.0;

	// Vorhand, Rueckhand, Abwechselnd, Zufall
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballmaschine")
	FString Position = TEXT("Abwechselnd");

	// Abstand des Aufsprungs vom Netz (m)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballmaschine")
	double ZielTiefe = 8.5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballmaschine")
	double Takt_s = 3.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballmaschine")
	bool bHoherBogen = false;
};

// Data/ansagen.csv
USTRUCT(BlueprintType)
struct MATCHBALLTENNIS_API FMBAnsageZeile : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ansage")
	FString Text;
};

// Data/spieler.csv (Kader, README 9.1)
USTRUCT(BlueprintType)
struct MATCHBALLTENNIS_API FMBSpielerZeile : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spieler") FString Vorname;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spieler") FString Nachname;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spieler") FString Tour;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spieler") FString Nation;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spieler") int32 Alter = 25;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spieler") int32 Groesse_cm = 185;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spieler") FString Schlaghand;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spieler") FString Rueckhand_Art;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spieler") int32 Balltipper = 2;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spieler") FString Spielstil;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spieler") FString Lieblingsbelag;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spieler") FString Persoenlichkeit;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spieler") FString Grundkoerper;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spieler") FString Outfit;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spieler") FString Oberteil;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spieler") FString Unterteil;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spieler") FString Frisur;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spieler") FString Haarfarbe;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Werte") double Aufschlag = 50.0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Werte") double Vorhand = 50.0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Werte") double Rueckhand = 50.0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Werte") double Volley = 50.0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Werte") double Schnelligkeit = 50.0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Werte") double Ausdauer = 50.0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Werte") double Nerven = 50.0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Werte") double Spin = 50.0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Werte") double Return = 50.0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Werte") int32 Gesamtwert = 50;
};
