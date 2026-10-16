// Matchball Tennis - Kern: Challenge-Regeln

#include "Officials/Kern/MBChallenge.h"

#include "Core/Kern/MBMathe.h"

namespace MB
{
	bool WarGutGegeben(EEntscheidungsArt Art)
	{
		return Art == EEntscheidungsArt::AufschlagGutGegeben || Art == EEntscheidungsArt::BallGutGegeben;
	}

	int Benachteiligter(const FStrittigeEntscheidung& Entscheidung)
	{
		const int Gegner = 1 - Entscheidung.Schlagender;
		switch (Entscheidung.Art)
		{
		case EEntscheidungsArt::AufschlagFehlerGerufen:
		case EEntscheidungsArt::BallAusGerufen:
			return Entscheidung.Schlagender;
		case EEntscheidungsArt::AufschlagGutGegeben:
		case EEntscheidungsArt::BallGutGegeben:
			return Gegner;
		}
		return Gegner;
	}

	FChallengeAuswertung WerteChallengeAus(const FStrittigeEntscheidung& Entscheidung)
	{
		FChallengeAuswertung Auswertung;
		const bool bGutGegeben = WarGutGegeben(Entscheidung.Art);
		Auswertung.bErfolgreich = bGutGegeben != Entscheidung.bWarIn;
		if (!Auswertung.bErfolgreich)
		{
			Auswertung.Folge = EChallengeFolge::EntscheidungBleibt;
			return Auswertung;
		}

		const int Aufschlaeger = Entscheidung.Schlagender;
		const int Challenger = Benachteiligter(Entscheidung);
		switch (Entscheidung.Art)
		{
		case EEntscheidungsArt::AufschlagFehlerGerufen:
			if (Entscheidung.bRueckschlaegerChancenlos)
			{
				Auswertung.Folge = EChallengeFolge::AssFuerAufschlaeger;
				Auswertung.Punktgewinner = Aufschlaeger;
			}
			else
			{
				Auswertung.Folge = EChallengeFolge::ErsterAufschlag;
			}
			break;
		case EEntscheidungsArt::AufschlagGutGegeben:
			if (Entscheidung.bZweiterAufschlag)
			{
				Auswertung.Folge = EChallengeFolge::Doppelfehler;
				Auswertung.Punktgewinner = Challenger;
			}
			else
			{
				Auswertung.Folge = EChallengeFolge::Aufschlagfehler;
			}
			break;
		case EEntscheidungsArt::BallAusGerufen:
			if (Entscheidung.bKlarerWinner)
			{
				Auswertung.Folge = EChallengeFolge::PunktFuerSchlagenden;
				Auswertung.Punktgewinner = Entscheidung.Schlagender;
			}
			else
			{
				Auswertung.Folge = EChallengeFolge::PunktWiederholen;
			}
			break;
		case EEntscheidungsArt::BallGutGegeben:
			Auswertung.Folge = EChallengeFolge::PunktFuerChallenger;
			Auswertung.Punktgewinner = Challenger;
			break;
		}
		return Auswertung;
	}

	FChallengeKonto::FChallengeKonto(int InProSatz, int InZusatzImTiebreak, bool bInAktiv)
		: ProSatz(InProSatz)
		, ZusatzImTiebreak(InZusatzImTiebreak)
		, bAktiv(bInAktiv)
	{
		NeuerSatz();
	}

	void FChallengeKonto::NeuerSatz()
	{
		Konto[0] = ProSatz;
		Konto[1] = ProSatz;
	}

	void FChallengeKonto::TiebreakBeginnt()
	{
		Konto[0] += ZusatzImTiebreak;
		Konto[1] += ZusatzImTiebreak;
	}

	int FChallengeKonto::Verbleibend(int Spieler) const
	{
		return (Spieler == 0 || Spieler == 1) ? Konto[Spieler] : 0;
	}

	bool FChallengeKonto::KannChallengen(int Spieler, const FStrittigeEntscheidung& Entscheidung) const
	{
		if (!bAktiv || (Spieler != 0 && Spieler != 1))
		{
			return false;
		}
		return Konto[Spieler] > 0 && Benachteiligter(Entscheidung) == Spieler;
	}

	bool FChallengeKonto::Challenge(int Spieler, const FStrittigeEntscheidung& Entscheidung, FChallengeAuswertung& OutAuswertung)
	{
		if (!KannChallengen(Spieler, Entscheidung))
		{
			OutAuswertung = FChallengeAuswertung();
			return false;
		}
		OutAuswertung = WerteChallengeAus(Entscheidung);
		if (!OutAuswertung.bErfolgreich)
		{
			--Konto[Spieler];
		}
		return true;
	}

	double KIChallengeWahrscheinlichkeit(const FStrittigeEntscheidung& Entscheidung, int Verbleibend, int VollesKonto, bool bWichtigerPunkt, const FKIChallengeParameter& Parameter)
	{
		if (Verbleibend <= 0)
		{
			return 0.0;
		}
		const double Abstand = Betrag(Entscheidung.Abstand);
		const bool bFalsch = WarGutGegeben(Entscheidung.Art) != Entscheidung.bWarIn;

		double Chance = 0.0;
		if (Abstand <= Parameter.MaxAbstand && Parameter.MaxAbstand > 0.0)
		{
			Chance = Parameter.GrundChance * (1.0 - Abstand / Parameter.MaxAbstand);
			Chance *= bFalsch ? Parameter.FaktorWennFalsch : Parameter.FaktorWennRichtig;
			// Mit weniger verbleibenden Challenges vorsichtiger
			const double Konto = VollesKonto > 0 ? static_cast<double>(Verbleibend) / static_cast<double>(VollesKonto) : 1.0;
			Chance *= Verbleibend == 1 ? Parameter.LetzteChallengeFaktor : MinWert(1.0, 0.6 + 0.4 * Konto);
		}
		if (bWichtigerPunkt && Abstand <= Parameter.StrategischMaxAbstand)
		{
			Chance += Parameter.StrategischeChance;
		}
		return Begrenze(Chance, 0.0, 1.0);
	}
}
