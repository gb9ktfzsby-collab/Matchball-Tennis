// Matchball Tennis - Kern: Schiedsrichter-Ansagen als Bausteine (README 16.2, 7.2)
//
// Eine Ansage ist eine Folge von Baustein-IDs (z. B. "fifteen", "love" oder "game", "@0").
// "@0"/"@1" stehen fuer den Namen von Spieler 0/1. Die Texte zu den IDs stehen in Data/ansagen.csv;
// das ElevenLabs-Skript erzeugt daraus je Baustein eine WAV-Datei, die Wiedergabe setzt sie zusammen.
// Fehlt ein Name-Baustein, nutzt das Spiel die kuerzere Ansage ohne Namen (siehe OhneNamen()).

#pragma once

#include "Match/Kern/MBZaehlweise.h"
#include "Officials/Kern/MBChallenge.h"

#include <map>
#include <string>
#include <vector>

namespace MB
{
	struct FAnsage
	{
		std::vector<std::string> Bausteine;

		bool IstLeer() const { return Bausteine.empty(); }

		// Text mit Namen; unbekannte IDs bleiben als ID stehen
		std::string Text(const std::map<std::string, std::string>& Texte, const std::string Namen[2]) const;

		// Fallback ohne Namen: "Game" statt "Game Adler", "Advantage server/receiver" statt "Advantage Adler"
		FAnsage OhneNamen(int Aufschlaeger) const;
	};

	// "," und "." sind Satzzeichen: kein Audio-Baustein, nur eine kurze Pause bzw. Text-Satzzeichen
	bool IstSatzzeichen(const std::string& Baustein);

	// Baustein-ID fuer eine Zahl ("n0" .. "n40")
	std::string ZahlBaustein(int Zahl);

	// Ansage nach einem Punkt. Z ist der Stand NACH dem Punkt, Folge das Ergebnis von PunktFuer().
	FAnsage AnsageNachPunkt(const FZaehlweise& Z, const FPunktFolge& Folge);

	// Einzelne Rufe: "fault", "out", "let", "time", "quiet_please", "new_balls_please" ...
	FAnsage AnsageRuf(const char* Baustein);

	// Linie aus Sicht des Schiedsrichterstuhls (Stuhl an der Netzseite bei X > 0, Blick nach -X):
	// nahe Grundlinie = "left_baseline", ferne = "right_baseline"
	std::string LinienBaustein(ELinie Linie, double PositionY);

	// "<Name> is challenging the call on the left baseline"
	FAnsage AnsageChallenge(int Spieler, ELinie Linie, double PositionY);

	// "Correction, the ball was good. Replay the point. First serve." bzw. Bestaetigung
	FAnsage AnsageChallengeErgebnis(const FStrittigeEntscheidung& Entscheidung, const FChallengeAuswertung& Auswertung);
}
