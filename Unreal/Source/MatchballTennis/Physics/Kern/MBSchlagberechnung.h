// Matchball Tennis - Kern: Schlagberechnung (README 5.3, 5.4 und 6.2)
//
// Aus Ziel, Schlagart, Power/Praezision, Timing-Fehler und Spielerwerten wird die Abfluggeschwindigkeit
// samt Drall gesucht, mit der der Ball das (gestreute) Ziel erreicht und das Netz ueberquert.
//
// Ablauf:
//  1. ErmittleStreuung(): Timing-Fehler -> seitliche Abweichung (zufaellig links ODER rechts) und
//     leichte Laengenstreuung; Bewertung gruen/gelb/rot fuer das Timing-Meter.
//  2. BerechneSchlag(): sucht per Simulation die Abflugrichtung (Steigung + seitliche Korrektur) fuer
//     das gestreute Ziel; ist der Ball zu flach fuers Netz, wird das Tempo schrittweise gesenkt.
//
// Ohne Winkelfunktionen: Richtungen werden als Vektoren (Steigung = vz / vh) beschrieben.

#pragma once

#include "Core/Kern/MBZufall.h"
#include "Physics/Kern/MBBallPhysik.h"

namespace MB
{
	// Schlagarten (README 5.4); Aufschlag-Varianten und Netzschlaege eigene Eintraege
	enum class ESchlagArt : unsigned char
	{
		Flach,
		Topspin,
		Slice,
		Lob,
		Stopp,
		Volley,
		Halbvolley,
		Schmetterball,
		AufschlagFlach,
		AufschlagKick,
		AufschlagSlice
	};

	constexpr int AnzahlSchlagArten = 11;

	const char* SchlagArtName(ESchlagArt Art);

	struct FSchlagAnfrage
	{
		FVek3 Treffpunkt;                       // Ballmittelpunkt im Moment des Schlags
		double ZielX = 0.0;                     // gewuenschter Aufsprungpunkt
		double ZielY = 0.0;
		double Tempo = 30.0;                    // gewuenschtes Abfluggetempo (m/s)
		double MinTempo = 12.0;                 // darf bis hierhin gesenkt werden, um ueber das Netz zu kommen
		double Topspin = 0.0;                   // rad/s; > 0 Vorwaertsdrall, < 0 Rueckwaertsdrall (Slice/Stopp)
		double Seitendrall = 0.0;               // rad/s um die Hochachse; > 0 zieht (von oben gesehen gegen den Uhrzeigersinn) nach links
		bool bHoherBogen = false;               // Lob: obere Loesung der Flugbahn
		double MinNetzAbstand = 0.10;           // Ballunterkante mind. so hoch ueber der Netzoberkante (m)
	};

	struct FSchlagErgebnis
	{
		FBallZustand Abflug;                    // Startzustand fuer die Simulation
		FVek3 Aufsprung;                        // vorhergesagter erster Aufsprung (Ballmittelpunkt)
		double Flugzeit = 0.0;                  // s bis zum ersten Aufsprung
		double NetzAbstand = -1.0;              // Ballunterkante ueber Netzoberkante beim Ueberqueren (m); < 0: Netz beruehrt/nicht ueberquert
		double TatsaechlichesTempo = 0.0;       // m/s nach evtl. Absenkung
		bool bZielErreicht = false;             // Ziel bis auf Toleranz getroffen
		bool bNetzFrei = false;                 // ueberquert das Netz ohne Beruehrung
	};

	// Sucht die Abflugrichtung (und ggf. ein niedrigeres Tempo). Deterministisch, feste Iterationszahlen.
	FSchlagErgebnis BerechneSchlag(const FBallSimulation& Vorlage, const FSchlagAnfrage& Anfrage);

	// Drallvektor fuer eine Flugrichtung: Topspin um die Achse (z x h), Seitendrall um z
	FVek3 DrallFuerRichtung(const FVek3& Richtung, double Topspin, double Seitendrall);

	// --- Timing und Streuung (README 5.3) -------------------------------------------------------

	enum class ETimingBewertung : unsigned char
	{
		Gruen,  // perfekt
		Gelb,   // leicht frueh/spaet
		Rot     // deutlich daneben
	};

	struct FStreuParameter
	{
		double GruenHalbfenster = 0.030;        // s: |Fehler| <= Wert -> gruen
		double GelbHalbfenster = 0.075;         // s: |Fehler| <= Wert -> gelb, sonst rot
		double Grundstreuung = 0.20;            // m seitlich, auch bei perfektem Timing (Spielerwert)
		double SeitlichProSekunde = 22.0;       // m seitliche Abweichung je s Fehler ausserhalb von gruen
		double LaengenStreuung = 0.30;          // m Grund-Laengenstreuung
		double LaengeProSekunde = 6.0;          // m zusaetzliche Laengenstreuung je s Fehler
	};

	// Faktoren fuer Power/Praezision und Spielerwert (0-99). Werte aus Data/timing.csv.
	struct FStreuModifikatoren
	{
		double PowerFenster = 0.70;             // Fenster * Faktor
		double PowerStreuung = 1.45;            // Streuung * Faktor
		double PraezisionFenster = 1.35;
		double PraezisionStreuung = 0.65;
		double FensterBeiWert50 = 0.85;         // Spielerwert 50 -> Faktor, 99 -> FensterBeiWert99
		double FensterBeiWert99 = 1.20;
		double StreuungBeiWert50 = 1.40;
		double StreuungBeiWert99 = 0.70;
		double MatchFenster = 1.0;              // Einstellung "Timing-Fenster" fuers ganze Match
	};

	// Wendet Power/Praezision, Spielerwert (0-99) und Match-Einstellung an
	FStreuParameter WendeModifikatorenAn(const FStreuParameter& Grund, const FStreuModifikatoren& Mod, bool bPower, bool bPraezision, double Spielerwert);

	struct FStreuErgebnis
	{
		double ZielX = 0.0;
		double ZielY = 0.0;
		double Seitlich = 0.0;                  // m, + = links aus Sicht des Schlagenden
		double Laenge = 0.0;                    // m, + = laenger
		ETimingBewertung Bewertung = ETimingBewertung::Gruen;
	};

	ETimingBewertung BewerteTiming(double TimingFehler, const FStreuParameter& Parameter);

	// TimingFehler in s (< 0 zu frueh, > 0 zu spaet); die Richtung der seitlichen Abweichung ist zufaellig.
	FStreuErgebnis StreueZiel(const FVek3& Treffpunkt, double ZielX, double ZielY, double TimingFehler, const FStreuParameter& Parameter, FZufall& Zufall);
}
