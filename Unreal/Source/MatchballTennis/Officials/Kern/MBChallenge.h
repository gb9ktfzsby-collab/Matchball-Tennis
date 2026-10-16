// Matchball Tennis - Kern: Challenge-Regeln (README 7.2)
//
// - Challengen darf nur, gegen wen entschieden wurde.
// - 3 Challenges pro Satz, +1 im Tiebreak; erfolgreiche Challenges bleiben erhalten.
// - Aufschlag faelschlich als Fehler gegeben: Wiederholung mit erstem Aufschlag - bzw. Ass, wenn der
//   Rueckschlaeger chancenlos war.
// - Im Ballwechsel faelschlich Aus gerufen (Ball war gut): Punkt wird wiederholt; war der Schlag ein
//   klarer Winner, bekommt der Schlagende den Punkt.            [README: PRUEFEN/NEU]
// - Ball faelschlich gut gegeben (war aus): Punkt fuer den Challenger.
// - Aufschlag faelschlich gut gegeben (war aus): Aufschlagfehler (beim 1. Aufschlag folgt der 2.;
//   beim 2. Aufschlag Doppelfehler = Punkt fuer den Challenger). Auslegung, siehe Dokumente/Entscheidungen.md.

#pragma once

#include "Core/Kern/MBZufall.h"

namespace MB
{
	enum class EEntscheidungsArt : unsigned char
	{
		AufschlagFehlerGerufen,   // Aufschlag als "Fault" gegeben
		AufschlagGutGegeben,      // Aufschlag gut gegeben (z. B. Ass, Rueckschlaeger hat nicht erreicht)
		BallAusGerufen,           // Ball im Ballwechsel als "Out" gegeben
		BallGutGegeben            // Ball im Ballwechsel gut gegeben, der Punkt endete danach
	};

	struct FStrittigeEntscheidung
	{
		EEntscheidungsArt Art = EEntscheidungsArt::BallAusGerufen;
		int Schlagender = 0;                    // Spieler (0/1), der den Ball geschlagen hat
		bool bWarIn = true;                     // tatsaechliches Ergebnis (Hawk-Eye)
		bool bZweiterAufschlag = false;         // nur fuer Aufschlag-Entscheidungen
		bool bRueckschlaegerChancenlos = false; // Aufschlag war "gut": haette der Rueckschlaeger ihn nicht erreicht?
		bool bKlarerWinner = false;             // Ballwechsel: Schlag haette der Gegner nicht mehr erreicht
		double Abstand = 0.0;                   // m zur Entscheidungsgrenze (fuer KI und Anzeige)
	};

	enum class EChallengeFolge : unsigned char
	{
		EntscheidungBleibt,      // Challenge erfolglos
		ErsterAufschlag,         // Punkt wird mit erstem Aufschlag wiederholt
		AssFuerAufschlaeger,     // Aufschlag war gut und unerreichbar: Punkt Aufschlaeger
		Aufschlagfehler,         // Aufschlag war aus: Fehler (1. -> 2. Aufschlag)
		Doppelfehler,            // Aufschlag war aus und es war der 2.: Punkt Rueckschlaeger
		PunktWiederholen,        // Ballwechsel wird wiederholt (mit erstem Aufschlag)
		PunktFuerSchlagenden,    // klarer Winner
		PunktFuerChallenger      // Ball war aus
	};

	struct FChallengeAuswertung
	{
		bool bErfolgreich = false;
		EChallengeFolge Folge = EChallengeFolge::EntscheidungBleibt;
		int Punktgewinner = -1;   // -1: kein Punkt vergeben (Wiederholung, 2. Aufschlag oder Entscheidung bleibt)
	};

	// Gegen wen wurde entschieden? (nur dieser Spieler darf challengen)
	int Benachteiligter(const FStrittigeEntscheidung& Entscheidung);

	// Ob die Entscheidung "gut" lautete
	bool WarGutGegeben(EEntscheidungsArt Art);

	// Wertet eine Challenge aus (unabhaengig vom Konto)
	FChallengeAuswertung WerteChallengeAus(const FStrittigeEntscheidung& Entscheidung);

	class FChallengeKonto
	{
	public:
		explicit FChallengeKonto(int InProSatz = 3, int InZusatzImTiebreak = 1, bool bInAktiv = true);

		void NeuerSatz();
		void TiebreakBeginnt();
		void SetzeAktiv(bool bInAktiv) { bAktiv = bInAktiv; }

		bool IstAktiv() const { return bAktiv; }
		int Verbleibend(int Spieler) const;

		bool KannChallengen(int Spieler, const FStrittigeEntscheidung& Entscheidung) const;

		// Fuehrt die Challenge durch (prueft KannChallengen); bei Misserfolg sinkt das Konto.
		// Liefert false, wenn die Challenge nicht erlaubt war.
		bool Challenge(int Spieler, const FStrittigeEntscheidung& Entscheidung, FChallengeAuswertung& OutAuswertung);

	private:
		int ProSatz = 3;
		int ZusatzImTiebreak = 1;
		bool bAktiv = true;
		int Konto[2] = {3, 3};
	};

	// KI-Verhalten (README 7.2): challengt knappe Baelle mit einer Wahrscheinlichkeit, die vom Abstand und
	// den uebrigen Challenges abhaengt; nicht bei klaren Baellen; selten strategisch bei wichtigen Punkten.
	struct FKIChallengeParameter
	{
		double MaxAbstand = 0.08;          // m: darueber nie (klarer Ball)
		double GrundChance = 0.55;         // bei Abstand 0 und vollem Konto
		double FaktorWennFalsch = 1.0;     // KI "sieht" Fehlentscheidungen eher
		double FaktorWennRichtig = 0.30;
		double LetzteChallengeFaktor = 0.5;
		double StrategischeChance = 0.04;  // zusaetzlich bei wichtigen Punkten (Break-/Satz-/Matchball)
		double StrategischMaxAbstand = 0.20;
	};

	double KIChallengeWahrscheinlichkeit(const FStrittigeEntscheidung& Entscheidung, int Verbleibend, int VollesKonto, bool bWichtigerPunkt, const FKIChallengeParameter& Parameter);
}
