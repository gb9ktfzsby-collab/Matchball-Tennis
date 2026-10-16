// Matchball Tennis - Kern: Punktrichter - entscheidet aus den Ball-Ereignissen eines Schlags,
// ob der Ball im Spiel ist, ein Fehler/Let vorliegt oder der Punkt endet (README 5, 7).
//
// Ablauf je Schlag: NeuerSchlag() beim Treffen, dann jedes Ball-Ereignis an Ereignis() geben.
// Der Linienrichter (mit Fehlerquote) entscheidet beim ersten Aufsprung auf der Gegenseite;
// die Wahrheit (Hawk-Eye-Kontaktflaeche) wird fuer eine spaetere Challenge mitgefuehrt.

#pragma once

#include "Core/Kern/MBZufall.h"
#include "Officials/Kern/MBChallenge.h"
#include "Officials/Kern/MBLinienrichter.h"
#include "Physics/Kern/MBBallPhysik.h"
#include "Physics/Kern/MBKontaktflaeche.h"
#include "Physics/Kern/MBPlatz.h"

namespace MB
{
	struct FSchlagKontext
	{
		int Schlagender = 0;
		EPlatzseite Seite = EPlatzseite::Nah;            // Seite des Schlagenden
		bool bAufschlag = false;
		EAufschlagHaelfte Haelfte = EAufschlagHaelfte::Einstand;
		bool bZweiterAufschlag = false;
	};

	enum class EPunktGrund : unsigned char
	{
		Keiner,
		Ass,                 // Aufschlag nicht erreicht
		Doppelfehler,
		Winner,              // zweiter Aufsprung beim Gegner bzw. nicht erreicht
		Aus,                 // als Aus gegeben
		Netz,                // im Netz haengen geblieben bzw. nicht darueber
		NichtUebersNetz,     // auf der eigenen Seite aufgesprungen
		Challenge,           // durch Challenge entschieden
		Aufgabe              // Aufgabe (online: Esc -> Aufgeben)
	};

	enum class ERichterSignal : unsigned char
	{
		Keins,               // nichts zu entscheiden (z. B. Flug, Netz ueberquert)
		BallGut,             // erster Aufsprung gut - Ball ist im Spiel
		Fehler,              // Aufschlagfehler (1. Aufschlag) - es folgt der 2. Aufschlag
		Let,                 // Aufschlag beruehrte das Netz und war gut - Wiederholung
		PunktEnde
	};

	struct FRichterEntscheidung
	{
		ERichterSignal Signal = ERichterSignal::Keins;
		int Punktgewinner = -1;
		EPunktGrund Grund = EPunktGrund::Keiner;
		bool bAusgerufen = false;            // Linienrichter rief "Out"/"Fault" (fuer Stimme und Geste)
		bool bLinienentscheidung = false;    // es gab eine Linienentscheidung bei diesem Signal
		FLinienEntscheidung Linie;
		FKontaktflaeche Kontaktflaeche;      // fuer Hawk-Eye-Darstellung
		bool bStrittig = false;              // eine challengebare Entscheidung liegt vor
		FStrittigeEntscheidung Strittig;
	};

	class FPunktrichter
	{
	public:
		FPunktrichter() = default;
		FPunktrichter(const FPlatz& InPlatz, const FBallParameter& InBall, const FLinienrichterParameter& InLinienrichter);

		// Neuer Schlag: vorherige Ballentscheidungen sind damit abgeschlossen
		void NeuerSchlag(const FSchlagKontext& Kontext);

		// Ein Ball-Ereignis der Simulation verarbeiten. Liefert ein Signal, sobald etwas zu entscheiden ist.
		FRichterEntscheidung Ereignis(const FBallEreignis& Ereignis, FZufall& Zufall);

		// Ist der aktuelle Schlag schon (z. B. als gut) beurteilt worden?
		bool ErsterAufsprungBeurteilt() const { return bErsterAufsprungBeurteilt; }
		bool IstAbgeschlossen() const { return bAbgeschlossen; }
		// Hat der Ball seit dem Schlag das Netz (ungerade oft) ueberquert, liegt er auf der Gegenseite
		bool BallAufGegenseite() const { return (NetzQuerungen % 2) == 1; }
		const FSchlagKontext& GetKontext() const { return Kontext; }
		const FLinienrichterParameter& GetLinienrichter() const { return Linienrichter; }

	private:
		FRichterEntscheidung BeurteileErstenAufsprung(const FBallEreignis& Ereignis, FZufall& Zufall);
		FRichterEntscheidung SchlagVerloren(EPunktGrund Grund) const;

		FPlatz Platz;
		FBallParameter Ball;
		FLinienrichterParameter Linienrichter;
		FSchlagKontext Kontext;
		bool bAktiv = false;
		bool bNetzBeruehrt = false;
		bool bErsterAufsprungBeurteilt = false;
		bool bAbgeschlossen = false;
		int NetzQuerungen = 0;
		FRichterEntscheidung LetzteGutEntscheidung;   // fuer eine Challenge nach Ass/Winner
	};
}
