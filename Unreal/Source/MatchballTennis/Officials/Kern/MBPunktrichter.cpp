// Matchball Tennis - Kern: Punktrichter

#include "Officials/Kern/MBPunktrichter.h"

namespace MB
{
	FPunktrichter::FPunktrichter(const FPlatz& InPlatz, const FBallParameter& InBall, const FLinienrichterParameter& InLinienrichter)
		: Platz(InPlatz)
		, Ball(InBall)
		, Linienrichter(InLinienrichter)
	{
	}

	void FPunktrichter::NeuerSchlag(const FSchlagKontext& InKontext)
	{
		Kontext = InKontext;
		bAktiv = true;
		bNetzBeruehrt = false;
		bErsterAufsprungBeurteilt = false;
		bAbgeschlossen = false;
		NetzQuerungen = 0;
		LetzteGutEntscheidung = FRichterEntscheidung();
	}

	FRichterEntscheidung FPunktrichter::SchlagVerloren(EPunktGrund Grund) const
	{
		FRichterEntscheidung Entscheidung;
		const int Gegner = 1 - Kontext.Schlagender;
		if (Kontext.bAufschlag && !Kontext.bZweiterAufschlag)
		{
			Entscheidung.Signal = ERichterSignal::Fehler;
			Entscheidung.Grund = Grund;
			return Entscheidung;
		}
		Entscheidung.Signal = ERichterSignal::PunktEnde;
		Entscheidung.Punktgewinner = Gegner;
		Entscheidung.Grund = Kontext.bAufschlag ? EPunktGrund::Doppelfehler : Grund;
		return Entscheidung;
	}

	FRichterEntscheidung FPunktrichter::BeurteileErstenAufsprung(const FBallEreignis& Ereignis, FZufall& Zufall)
	{
		bErsterAufsprungBeurteilt = true;

		// Auf der eigenen Seite aufgesprungen: nicht uebers Netz
		if (SeitenVorzeichen(Kontext.Seite) * Ereignis.Position.Y > 0.0)
		{
			bAbgeschlossen = true;
			return SchlagVerloren(bNetzBeruehrt ? EPunktGrund::Netz : EPunktGrund::NichtUebersNetz);
		}

		const FFeld Feld = Kontext.bAufschlag ? Platz.Aufschlagfeld(Kontext.Seite, Kontext.Haelfte) : Platz.Einzelfeld(Gegenseite(Kontext.Seite));
		const FKontaktflaeche Flaeche = BerechneKontaktflaeche(Ereignis, Ball);
		const FLinienUrteil Wahrheit = BeurteileKontaktflaeche(Flaeche, Feld);
		const FLinienEntscheidung Linie = EntscheideLinie(Wahrheit, Linienrichter, Zufall);

		FRichterEntscheidung Entscheidung;
		Entscheidung.bLinienentscheidung = true;
		Entscheidung.Linie = Linie;
		Entscheidung.Kontaktflaeche = Flaeche;
		Entscheidung.Strittig.Schlagender = Kontext.Schlagender;
		Entscheidung.Strittig.bWarIn = Wahrheit.bIn;
		Entscheidung.Strittig.bZweiterAufschlag = Kontext.bZweiterAufschlag;
		Entscheidung.Strittig.Abstand = Wahrheit.Abstand;

		if (Linie.bGutGegeben)
		{
			if (Kontext.bAufschlag && bNetzBeruehrt)
			{
				// Let: Aufschlag mit Netzberuehrung, im Feld - wird wiederholt (nicht challengebar)
				Entscheidung.Signal = ERichterSignal::Let;
				bAbgeschlossen = true;
				return Entscheidung;
			}
			Entscheidung.Signal = ERichterSignal::BallGut;
			Entscheidung.Strittig.Art = Kontext.bAufschlag ? EEntscheidungsArt::AufschlagGutGegeben : EEntscheidungsArt::BallGutGegeben;
			// Challengebar wird die Gut-Entscheidung erst, wenn der Punkt mit diesem Ball endet
			Entscheidung.bStrittig = false;
			LetzteGutEntscheidung = Entscheidung;
			LetzteGutEntscheidung.bStrittig = !Linienrichter.bElektronisch;
			return Entscheidung;
		}

		// Ausgerufen: Aufschlagfehler bzw. Punkt fuer den Gegner
		bAbgeschlossen = true;
		FRichterEntscheidung Aus = SchlagVerloren(EPunktGrund::Aus);
		Aus.bLinienentscheidung = true;
		Aus.Linie = Linie;
		Aus.Kontaktflaeche = Flaeche;
		Aus.Strittig = Entscheidung.Strittig;
		Aus.Strittig.Art = Kontext.bAufschlag ? EEntscheidungsArt::AufschlagFehlerGerufen : EEntscheidungsArt::BallAusGerufen;
		Aus.bAusgerufen = true;
		Aus.bStrittig = !Linienrichter.bElektronisch;
		return Aus;
	}

	FRichterEntscheidung FPunktrichter::Ereignis(const FBallEreignis& Ereignis, FZufall& Zufall)
	{
		FRichterEntscheidung Leer;
		if (!bAktiv || bAbgeschlossen)
		{
			return Leer;
		}

		switch (Ereignis.Art)
		{
		case EBallEreignisArt::Netzband:
		case EBallEreignisArt::Netzgeflecht:
			if (!bErsterAufsprungBeurteilt)
			{
				bNetzBeruehrt = true;
			}
			return Leer;

		case EBallEreignisArt::NetzUeberquert:
			++NetzQuerungen;
			return Leer;

		case EBallEreignisArt::Aufsprung:
			if (!bErsterAufsprungBeurteilt)
			{
				return BeurteileErstenAufsprung(Ereignis, Zufall);
			}
			// Zweiter Aufsprung, ohne dass der Gegner geschlagen hat: Punkt fuer den Schlagenden
			[[fallthrough]];

		case EBallEreignisArt::Ruhe:
		{
			if (!bErsterAufsprungBeurteilt)
			{
				// Ball kam ohne Aufsprung zur Ruhe/verliess den Bereich (z. B. im Netz liegen geblieben)
				bAbgeschlossen = true;
				return SchlagVerloren(bNetzBeruehrt ? EPunktGrund::Netz : EPunktGrund::Aus);
			}
			bAbgeschlossen = true;
			FRichterEntscheidung Entscheidung = LetzteGutEntscheidung;
			Entscheidung.Signal = ERichterSignal::PunktEnde;
			Entscheidung.Punktgewinner = Kontext.Schlagender;
			Entscheidung.Grund = Kontext.bAufschlag ? EPunktGrund::Ass : EPunktGrund::Winner;
			Entscheidung.bAusgerufen = false;
			return Entscheidung;
		}
		}
		return Leer;
	}
}
