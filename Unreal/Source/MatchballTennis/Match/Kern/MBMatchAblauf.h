// Matchball Tennis - Kern: Match-Zustandsmaschine (README 2.3)
//
//   Vorbereitung -> Aufschlagvorbereitung (Balltippen) -> Aufschlag -> Ballwechsel -> Punktende
//   -> (Challenge-Fenster -> Videobeweis) -> Ansage -> Zwischen den Punkten -> ...
//   plus Seitenwechsel, Satzpause, Matchende.
//
// Jeder Zustand hat klare Ein- und Ausgaenge. Die Zustandsmaschine kennt keine Grafik: das Spiel meldet
// Schlaege und Ball-Ereignisse, die Maschine entscheidet (ueber Punktrichter, Linienrichter, Challenge und
// Zaehlweise) und liefert Meldungen fuer Anzeige, Stimme und Animation. Der Punkt wird erst in der Ansage
// gezaehlt - vorher kann eine Challenge ihn noch aendern.
//
// Determinismus: Alle Zufallsentscheidungen nutzen einen Generator, der vor jedem Aufschlag aus
// Match-Startwert und Aufschlagnummer neu gestartet wird (online auf beiden PCs identisch).

#pragma once

#include "Match/Kern/MBZaehlweise.h"
#include "Officials/Kern/MBAnsagen.h"
#include "Officials/Kern/MBChallenge.h"
#include "Officials/Kern/MBPunktrichter.h"

#include <cstdint>
#include <vector>

namespace MB
{
	enum class EMatchZustand : unsigned char
	{
		Vorbereitung,
		Aufschlagvorbereitung,
		Aufschlag,
		Ballwechsel,
		Punktende,
		ChallengeFenster,
		Videobeweis,
		Ansage,
		ZwischenDenPunkten,
		Seitenwechsel,
		Satzpause,
		Matchende
	};

	const char* MatchZustandName(EMatchZustand Zustand);

	// Dauer der Abschnitte in Sekunden (Data/ablauf.csv)
	struct FAblaufZeiten
	{
		double Vorbereitung = 3.0;
		double Punktende = 1.2;
		double ChallengeFenster = 4.0;          // [NEU] ca. 4 s
		double Videobeweis = 7.0;
		double Ansage = 1.8;
		double NachFehler = 1.5;                // bis zum zweiten Aufschlag
		double ZwischenDenPunkten = 7.0;
		double SeitenwechselOhnePause = 6.0;    // nach dem 1. Spiel eines Satzes und im Tiebreak
		double Seitenwechsel = 25.0;            // mit Pause (Spieler setzen sich)
		double Satzpause = 35.0;
	};

	struct FMatchEinstellungen
	{
		FZaehlRegeln Regeln;
		int ErsterAufschlaeger = 0;
		EPlatzseite SeiteSpieler0 = EPlatzseite::Nah;
		FAblaufZeiten Zeiten;
		FLinienrichterParameter Linienrichter;
		bool bChallenges = true;                // aus = keine Challenges (oder elektronische Linienrichter)
		int ChallengesProSatz = 3;
		int ChallengesZusatzTiebreak = 1;
		// Das Challenge-Fenster oeffnet nur bei knappen Entscheidungen (Abstand zur Linie bis hier, m).
		// Weiter entfernt ist der Linienrichter immer richtig (README 7.1) - eine Challenge per Taste
		// bleibt waehrend "Punktende" trotzdem moeglich.
		double ChallengeFensterMaxAbstand = 0.30;
		std::uint64_t Startwert = 1;            // Match-Startwert (online: vom Gastgeber)
		FPlatz Platz;
		FBallParameter Ball;
	};

	enum class EAusgangArt : unsigned char
	{
		Keiner,
		Punkt,          // Punkt fuer Punktgewinner
		Fehler,         // erster Aufschlag Fehler -> zweiter Aufschlag
		Let,            // Aufschlag wiederholen (gleicher Aufschlag)
		Wiederholung    // Punkt mit erstem Aufschlag wiederholen (nach Challenge)
	};

	struct FAusgang
	{
		EAusgangArt Art = EAusgangArt::Keiner;
		int Punktgewinner = -1;
		EPunktGrund Grund = EPunktGrund::Keiner;
	};

	enum class EMeldungArt : unsigned char
	{
		Zustand,                // neuer Zustand (Feld Zustand)
		Ruf,                    // Linienrichter/Schiedsrichter-Ruf ("fault", "out", "let") - Feld Ansage
		PunktGezaehlt,          // Feld Folge, Spieler = Punktgewinner, Grund
		Ansage,                 // Schiedsrichter-Ansage (Feld Ansage)
		ChallengeMoeglich,      // Spieler darf challengen (Fenster offen)
		ChallengeAngefordert,   // Feld Ansage = "X is challenging ..."
		ChallengeErgebnis,      // Feld Challenge, Entscheidung (Kontaktflaeche, Abstand)
		NeueBaelle,
		Matchende               // Spieler = Sieger
	};

	struct FAblaufMeldung
	{
		EMeldungArt Art = EMeldungArt::Zustand;
		EMatchZustand Zustand = EMatchZustand::Vorbereitung;
		int Spieler = -1;
		EPunktGrund Grund = EPunktGrund::Keiner;
		FAnsage Ansage;
		FPunktFolge Folge;
		FRichterEntscheidung Entscheidung;
		FChallengeAuswertung Challenge;
	};

	class FMatchAblauf
	{
	public:
		explicit FMatchAblauf(const FMatchEinstellungen& InEinstellungen);

		// Zeit fortschreiten lassen (Sekunden). Timer-gesteuerte Uebergaenge passieren hier.
		void Fortschreiten(double Dt);

		// --- Eingaben aus dem Spiel ------------------------------------------------------------
		// Aufschlaeger trifft den Ball (nur im Zustand Aufschlagvorbereitung)
		bool AufschlagGeschlagen();
		// Spieler schlaegt den Ball zurueck (nur wenn der Ball im Spiel ist und er an der Reihe ist)
		bool BallGeschlagen(int Spieler);
		// Ball-Ereignis aus der Simulation
		void BallEreignis(const FBallEreignis& Ereignis);
		// Challenge (Taste H / Y): nur waehrend Punktende oder Challenge-Fenster
		bool ChallengeAnfordern(int Spieler);
		// Vom Spiel ermittelt (Erreichbarkeit), bevor das Challenge-Fenster schliesst
		void SetzeChallengeUmstaende(bool bKlarerWinner, bool bRueckschlaegerChancenlos);
		// Zwischensequenz ueberspringen (Ansage, zwischen den Punkten, Seitenwechsel, Videobeweis)
		void Ueberspringen();
		// Aufgabe (online: Esc -> Aufgeben); Gegner gewinnt
		void Aufgabe(int Spieler);

		// --- Abfragen ----------------------------------------------------------------------------
		EMatchZustand GetZustand() const { return Zustand; }
		double GetZeitImZustand() const { return ZeitImZustand; }
		const FZaehlweise& GetStand() const { return Stand; }
		const FChallengeKonto& GetChallenges() const { return Konto; }
		bool IstZweiterAufschlag() const { return bZweiterAufschlag; }
		int GetAufschlaeger() const { return Stand.GetAufschlaeger(); }
		int GetLetzterSchlagender() const { return LetzterSchlagender; }
		int GetSchlaegeImBallwechsel() const { return SchlaegeImBallwechsel; }
		bool IstBallImSpiel() const;
		int GetSieger() const { return Sieger; }
		std::uint64_t GetAufschlagNummer() const { return AufschlagNummer; }
		const FAusgang& GetAusgang() const { return Ausgang; }
		const FRichterEntscheidung& GetLetzteEntscheidung() const { return LetzteEntscheidung; }
		const FMatchEinstellungen& GetEinstellungen() const { return Einstellungen; }

		// Gesammelte Meldungen abholen (leert die Liste)
		std::vector<FAblaufMeldung> HoleMeldungen();

	private:
		void Wechsle(EMatchZustand Neu);
		void Melde(const FAblaufMeldung& Meldung) { Meldungen.push_back(Meldung); }
		void PunktentscheidungEingetroffen(const FRichterEntscheidung& Entscheidung);
		void NachPunktende();
		void BeginneAnsage();
		void NachAnsage();
		int ChallengeBerechtigter() const;

		FMatchEinstellungen Einstellungen;
		FZaehlweise Stand;
		FChallengeKonto Konto;
		FPunktrichter Richter;
		FZufall Zufall;

		EMatchZustand Zustand = EMatchZustand::Vorbereitung;
		double ZeitImZustand = 0.0;
		double ZustandDauer = 0.0;

		bool bZweiterAufschlag = false;
		int LetzterSchlagender = -1;
		int SchlaegeImBallwechsel = 0;
		std::uint64_t AufschlagNummer = 0;
		int Sieger = -1;

		FAusgang Ausgang;
		FRichterEntscheidung LetzteEntscheidung;
		bool bChallengeAngefordert = false;
		FChallengeAuswertung ChallengeErgebnis;
		bool bKlarerWinner = false;
		bool bRueckschlaegerChancenlos = false;
		FPunktFolge LetzteFolge;

		std::vector<FAblaufMeldung> Meldungen;
	};
}
