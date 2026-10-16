// Matchball Tennis - Kern: Zaehlweise (README 8), reine Logik ohne Grafik
//
// - Punkte 0, 15, 30, 40, Einstand, Vorteil, Spiel; [NEU] No-Ad (Entscheidungspunkt bei Einstand)
// - Gewinnsaetze 1-3, Spiele pro Satz 1-6 (Standard 6)
// - Tiebreak bei N:N bis 7 mit 2 Punkten Abstand; Aufschlagwechsel nach dem ersten und dann alle zwei Punkte;
//   Seitenwechsel alle 6 Punkte; wer im Tiebreak zuerst aufschlug, nimmt im ersten Spiel des naechsten Satzes auf
// - Seitenwechsel nach jedem ungeraden Spiel eines Satzes (der Tiebreak zaehlt als ein Spiel);
//   nach dem ersten Spiel eines Satzes ohne Pause, sonst kurze Pause; nach einem Satz Satzpause
// - [NEU] Entscheidungssatz: normaler Tiebreak (Standard), Tiebreak bis 10, ausspielen oder Match-Tiebreak (bis 10)
// - [NEU] Neue Baelle nach 7, dann alle 9 Spiele (nicht vor einem Tiebreak - dann im 2. Spiel des naechsten Satzes)
// - Sonderfall 1 Spiel pro Satz: wer das Spiel gewinnt, gewinnt den Satz (kein Tiebreak)

#pragma once

#include "Physics/Kern/MBPlatz.h"

#include <string>
#include <vector>

namespace MB
{
	enum class EEntscheidungssatz : unsigned char
	{
		Tiebreak,         // wie alle Saetze (Standard)
		Tiebreak10,       // bei N:N Tiebreak bis 10 (Grand-Slam-Regel)
		Ausspielen,       // kein Tiebreak, 2 Spiele Vorsprung
		MatchTiebreak     // statt des Entscheidungssatzes ein Match-Tiebreak bis 10
	};

	struct FZaehlRegeln
	{
		int Gewinnsaetze = 2;                         // 1, 2 oder 3
		int SpieleProSatz = 6;                        // 1-6
		int TiebreakBis = 7;
		int LangerTiebreakBis = 10;                   // Tiebreak10 und Match-Tiebreak
		bool bNoAd = false;
		EEntscheidungssatz Entscheidungssatz = EEntscheidungssatz::Tiebreak;
		int NeueBaelleErstesMal = 7;                  // 0 = keine Ballwechsel-Hinweise
		int NeueBaelleDanach = 9;

		// Begrenzt alle Werte auf gueltige Bereiche
		void Normalisiere();
	};

	enum class EPause : unsigned char
	{
		Keine,
		Seitenwechsel,    // kurze Pause (Spieler setzen sich)
		Satzpause
	};

	// Folgen eines gespielten Punkts
	struct FPunktFolge
	{
		int Punktgewinner = -1;
		bool bSpielGewonnen = false;
		int Spielgewinner = -1;
		bool bBreak = false;                // Spiel vom Rueckschlaeger gewonnen (nicht im Tiebreak)
		bool bSatzGewonnen = false;
		int Satzgewinner = -1;
		bool bMatchGewonnen = false;
		int Matchgewinner = -1;
		bool bTiebreakBeginnt = false;
		bool bMatchTiebreakBeginnt = false;
		bool bSeitenwechsel = false;        // Spieler wechseln die Seiten (vor dem naechsten Punkt)
		EPause Pause = EPause::Keine;
		bool bNeueBaelle = false;           // vor dem naechsten Spiel neue Baelle
		bool bAufschlagwechsel = false;     // naechster Punkt hat einen anderen Aufschlaeger
	};

	struct FSatzErgebnis
	{
		int Spiele[2] = {0, 0};
		bool bMitTiebreak = false;
		int TiebreakPunkte[2] = {0, 0};
		bool bMatchTiebreak = false;        // Satz wurde als Match-Tiebreak gespielt (Spiele 1:0 bzw. 0:1)
		int Gewinner = -1;
	};

	// Wichtigkeit des naechsten Punkts aus Sicht eines Spielers
	struct FPunktBedeutung
	{
		bool bBreakball = false;    // Spieler ist Rueckschlaeger und gewinnt mit dem Punkt das Spiel
		bool bSatzball = false;     // ... den Satz (aber nicht das Match)
		bool bMatchball = false;    // ... das Match
		bool bEntscheidungspunkt = false; // No-Ad bei Einstand
	};

	class FZaehlweise
	{
	public:
		FZaehlweise();
		FZaehlweise(const FZaehlRegeln& InRegeln, int InErsterAufschlaeger, EPlatzseite InSeiteSpieler0 = EPlatzseite::Nah);

		// Punkt fuer Spieler (0 oder 1). Nach Matchende ohne Wirkung.
		FPunktFolge PunktFuer(int Spieler);

		// --- Abfragen -------------------------------------------------------------------------
		const FZaehlRegeln& GetRegeln() const { return Regeln; }
		int GetAufschlaeger() const;
		int GetRueckschlaeger() const { return 1 - GetAufschlaeger(); }
		EAufschlagHaelfte GetAufschlagHaelfte() const;
		EPlatzseite GetSeite(int Spieler) const;
		bool IstTiebreak() const { return bTiebreak; }
		bool IstMatchTiebreak() const { return bTiebreak && bMatchTiebreakLaeuft; }
		bool IstEntscheidungssatz() const;
		bool IstBeendet() const { return Sieger >= 0; }
		int GetSieger() const { return Sieger; }

		// Punkte im laufenden Spiel (bzw. Tiebreak)
		int GetPunkte(int Spieler) const;
		// Spiele im laufenden Satz
		int GetSpiele(int Spieler) const;
		int GetGewonneneSaetze(int Spieler) const;
		int GetSatzNummer() const { return static_cast<int>(Saetze.size()) + 1; }
		const std::vector<FSatzErgebnis>& GetAbgeschlosseneSaetze() const { return Saetze; }
		int GetGespielteSpiele() const { return SpieleGesamt; }
		int GetPunkteImSpielGesamt() const { return Punkte[0] + Punkte[1]; }

		// Ziel-Punktzahl des laufenden Tiebreaks (7 oder 10)
		int GetTiebreakZiel() const { return TiebreakZiel; }

		// Anzeige der Punkte wie im TV: "0", "15", "30", "40", "AD" (bzw. Tiebreak-Zahl)
		std::string PunktText(int Spieler) const;
		// Kurzer Gesamtstand, z. B. "6-4 3-2 30-15" (Spieler 0 zuerst)
		std::string StandText() const;

		FPunktBedeutung Bedeutung(int Spieler) const;

		// Liegt Einstand vor (beide mind. 40, gleich viele Punkte)? Nicht im Tiebreak.
		bool IstEinstand() const;
		// Hat ein Spieler Vorteil? Liefert -1, wenn nicht.
		int GetVorteil() const;

	private:
		void SpielGewonnen(int Gewinner, FPunktFolge& Folge);
		void SatzGewonnen(int Gewinner, FPunktFolge& Folge);
		void BeginneNeuesSpiel(FPunktFolge& Folge);
		bool SatzEntschieden(int& OutGewinner) const;
		bool TiebreakFaellig() const;
		void PruefeNeueBaelle(FPunktFolge& Folge, bool bNaechstesIstTiebreak);

		FZaehlRegeln Regeln;
		int Punkte[2] = {0, 0};
		int Spiele[2] = {0, 0};
		int SaetzeGewonnen[2] = {0, 0};
		std::vector<FSatzErgebnis> Saetze;
		int SpielAufschlaeger = 0;            // Aufschlaeger des laufenden Spiels (bzw. Erster im Tiebreak)
		bool bTiebreak = false;
		bool bMatchTiebreakLaeuft = false;
		int TiebreakZiel = 7;
		int Sieger = -1;
		EPlatzseite SeiteSpieler0 = EPlatzseite::Nah;
		int SpieleGesamt = 0;
		int SpieleSeitBallwechsel = 0;
		bool bErsterBallwechselErfolgt = false;
		bool bBallwechselAufgeschoben = false;
	};

	// Punkt-Zahl nach Tennis-Zaehlung (0, 15, 30, 40) fuer 0..3 Punkte
	const char* TennisPunktName(int Punkte);
}
