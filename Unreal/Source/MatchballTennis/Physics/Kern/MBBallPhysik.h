// Matchball Tennis - Kern: deterministische Ballsimulation (README 6)
//
// - fester Zeitschritt (Standard 240 Hz), Runge-Kutta 4. Ordnung, unabhaengig von der Bildrate
// - Kraefte: Schwerkraft, Luftwiderstand (proportional v^2), Magnus-Kraft aus Drall, Drall-Abnahme
// - Absprung je Belag: Stosszahl (senkrecht, geschwindigkeitsabhaengig) und Gleitreibung mit
//   Uebergang ins Rollen (Drall veraendert den Absprung: Topspin schnell nach vorn, Stopp springt zurueck)
// - Netz: Netzband als Zylinder (Netzroller), Netzgeflecht als weiche Flaeche
// - Ereignisse werden innerhalb eines Schritts zeitgenau bestimmt (kein Durchtunneln des Netzes)
//
// Alle Zahlenwerte kommen aus Data/ (Ball, Belaege); die Herleitung steht in Dokumente/Werte.md.

#pragma once

#include "Core/Kern/MBMathe.h"
#include "Physics/Kern/MBPlatz.h"

#include <cstdint>
#include <string>
#include <vector>

namespace MB
{
	// Eigenschaften von Ball und Luft
	struct FBallParameter
	{
		double Masse = 0.0577;                // kg (ITF: 56,0-59,4 g)
		double Radius = 0.0335;               // m  (ITF: 6,54-6,86 cm Durchmesser)
		double Luftdichte = 1.204;            // kg/m^3 (20 Grad C, Meereshoehe)
		double Widerstandsbeiwert = 0.507;    // c_W neuer Ball im Flug (Cross & Lindsey)
		double TraegheitsFaktor = 0.55;       // I = Faktor * m * r^2 (Hohlkugel mit Filz)
		double DrallAbnahmeProMeter = 0.004;  // Anteil des Dralls, der pro geflogenem Meter verloren geht
		double Schwerkraft = 9.81;            // m/s^2
		double BallSteifigkeit = 30000.0;     // N/m, dynamisch (fuer Kontaktzeit und Abdruck)

		double Querschnitt() const { return KreiszahlPi * Radius * Radius; }
	};

	// Absprung-Eigenschaften eines Belags
	struct FBelagParameter
	{
		std::string Name = "Hart";
		double Reibung = 0.60;                     // Gleitreibungszahl mu (ITF-Bereich 0,5-0,8)
		double Stosszahl = 0.82;                   // senkrechte Stosszahl e bei Referenzgeschwindigkeit
		double StosszahlReferenz = 8.27;           // m/s Normalgeschwindigkeit (ITF-Pruefung: 30 m/s unter 16 Grad)
		double StosszahlAbnahme = 0.008;           // Abnahme von e je m/s ueber der Referenz
		double RollReibung = 0.6;                  // m/s^2 Verzoegerung beim Rollen

		// e fuer eine gegebene senkrechte Aufprallgeschwindigkeit (Betrag), begrenzt auf [0,3; 0,95]
		double StosszahlBei(double NormalGeschwindigkeit) const;

		// ITF Court Pace Rating: CPR = 100 (1 - mu) + 150 (0,81 - e)
		double CourtPaceRating() const;

		// Passt die Reibung so an, dass der Belag das gewuenschte CPR erreicht (Turnier-Feineinstellung)
		void SetzeCourtPace(double ZielCpr);
	};

	// Netz-Eigenschaften
	struct FNetzParameter
	{
		double BandRadius = 0.010;        // m, Netzband ueber dem Drahtseil als Zylinder
		double BandStosszahl = 0.30;      // Rueckprall vom Netzband (senkrecht zur Oberflaeche)
		double BandTangentialFaktor = 0.75;
		double GeflechtStosszahl = 0.10;  // Ball ins Netz: faellt nahezu senkrecht zurueck
		double GeflechtDaempfung = 0.25;
	};

	struct FBallZustand
	{
		FVek3 Position;      // Ballmittelpunkt (m)
		FVek3 Geschwindigkeit;
		FVek3 Drall;         // Winkelgeschwindigkeit (rad/s), rechte-Hand-Regel im Kern-System
		bool bRollt = false; // liegt auf dem Boden auf und rollt
		bool bRuht = false;  // Simulation beendet (liegt still oder hat den Bereich verlassen)
	};

	enum class EBallEreignisArt : unsigned char
	{
		Aufsprung,      // Bodenkontakt
		Netzband,       // Beruehrung des Netzbands (Netzroller moeglich)
		Netzgeflecht,   // Ball ins Netz
		NetzUeberquert, // Ballmittelpunkt hat die Netzebene ueberquert (ohne Netzberuehrung in diesem Moment)
		Ruhe            // Ball liegt still oder hat den Simulationsbereich verlassen
	};

	struct FBallEreignis
	{
		EBallEreignisArt Art = EBallEreignisArt::Aufsprung;
		std::int64_t Schritt = 0;     // Schritt, in dem das Ereignis lag
		double Zeit = 0.0;            // genaue Zeit in s seit Simulationsbeginn
		FVek3 Position;               // Ballmittelpunkt beim Ereignis
		FVek3 GeschwindigkeitVorher;  // beim Aufsprung: kurz vor dem Kontakt
		FVek3 GeschwindigkeitNachher;
		FVek3 DrallVorher;
		FVek3 DrallNachher;
		double Gleitweg = 0.0;        // Aufsprung: Weg, den der Ball waehrend des Kontakts gleitet (m)
	};

	class FBallSimulation
	{
	public:
		static constexpr int StandardFrequenz = 240;

		FBallSimulation();
		FBallSimulation(const FBallParameter& InBall, const FBelagParameter& InBelag, const FNetzParameter& InNetz = FNetzParameter(), int InFrequenz = StandardFrequenz);

		// Startet einen neuen Flug (z. B. nach einem Schlag); Zeit und Ereignisse werden zurueckgesetzt
		void Starte(const FBallZustand& Start);

		// Setzt den Zustand mitten im Flug, ohne Zeit und Ereignisse zurueckzusetzen
		void SetzeZustand(const FBallZustand& Neu) { Zustand = Neu; }

		// Ein fester Zeitschritt. Neue Ereignisse werden an Ereignisse angehaengt.
		void Schritt();

		// So viele Schritte, bis die Zeit Zielzeit erreicht ist (oder der Ball ruht)
		void SimuliereBis(double Zielzeit);

		const FBallZustand& GetZustand() const { return Zustand; }
		double GetZeit() const { return static_cast<double>(Schritte) * Zeitschritt; }
		std::int64_t GetSchritte() const { return Schritte; }
		double GetZeitschritt() const { return Zeitschritt; }
		const std::vector<FBallEreignis>& GetEreignisse() const { return Ereignisse; }
		void LeereEreignisse() { Ereignisse.clear(); }

		const FBallParameter& GetBall() const { return Ball; }
		const FBelagParameter& GetBelag() const { return Belag; }
		const FNetzParameter& GetNetz() const { return Netz; }
		void SetzeBelag(const FBelagParameter& InBelag) { Belag = InBelag; }

		// Wind (m/s) - [NEU] kleiner Einfluss, Standard aus
		void SetzeWind(const FVek3& InWind) { Wind = InWind; }

		// Grenzen des Simulationsbereichs; ausserhalb ruht der Ball
		double BereichHalbeBreite = 20.0;
		double BereichHalbeLaenge = 30.0;

		// Beschleunigung und Drall-Aenderung fuer einen Zustand (oeffentlich fuer Tests)
		void Ableitung(const FVek3& Geschwindigkeit, const FVek3& Drall, FVek3& OutBeschleunigung, FVek3& OutDrallAenderung) const;

		// Absprung-Rechnung fuer einen Bodenkontakt (oeffentlich fuer Tests und Hawk-Eye)
		void BerechneAufsprung(const FVek3& V, const FVek3& W, FVek3& OutV, FVek3& OutW, double& OutGleitweg) const;

		// Kontaktzeit am Boden fuer die Ballsteifigkeit (halbe Schwingungsperiode)
		double Kontaktzeit() const;

	private:
		struct FTeilschritt
		{
			FVek3 Position;
			FVek3 Geschwindigkeit;
			FVek3 Drall;
		};

		FTeilschritt Integriere(const FTeilschritt& Start, double Dauer) const;
		void SchrittMitDauer(double Dauer, int Tiefe);
		void Rollen(double Dauer);
		void PruefeRuhe();
		void FuegeEreignisHinzu(EBallEreignisArt Art, double Zeitanteil, const FVek3& Pos, const FVek3& VVorher, const FVek3& VNachher, const FVek3& WVorher, const FVek3& WNachher, double Gleitweg);

		FBallParameter Ball;
		FBelagParameter Belag;
		FNetzParameter Netz;
		FVek3 Wind;
		FBallZustand Zustand;
		double Zeitschritt = 1.0 / StandardFrequenz;
		std::int64_t Schritte = 0;
		double SchrittBeginnZeit = 0.0;   // Zeit am Anfang des laufenden Schritts
		double SchrittVerbraucht = 0.0;   // im laufenden Schritt bereits simulierte Zeit
		std::vector<FBallEreignis> Ereignisse;

		// Hilfswerte (aus Parametern vorberechnet)
		double WiderstandsKonstante = 0.0;  // 0,5 rho A c_W / m
		double AuftriebsKonstante = 0.0;    // 0,5 rho A / m
	};
}
