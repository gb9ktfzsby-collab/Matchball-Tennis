// Matchball Tennis - Kern: Schiedsrichter-Ansagen

#include "Officials/Kern/MBAnsagen.h"

namespace MB
{
	namespace
	{
		const char* MBAnsagen_PunktBaustein(int Punkte)
		{
			switch (Punkte)
			{
			case 0: return "love";
			case 1: return "fifteen";
			case 2: return "thirty";
			default: return "forty";
			}
		}

		const char* MBAnsagen_SatzBaustein(int SatzNummer)
		{
			switch (SatzNummer)
			{
			case 1: return "first_set";
			case 2: return "second_set";
			case 3: return "third_set";
			case 4: return "fourth_set";
			default: return "fifth_set";
			}
		}

		std::string MBAnsagen_Name(int Spieler)
		{
			return Spieler == 1 ? std::string("@1") : std::string("@0");
		}

		// Zahl fuer Spielstaende: 0 = "love"
		std::string MBAnsagen_SpielZahl(int Zahl)
		{
			return Zahl == 0 ? std::string("love") : ZahlBaustein(Zahl);
		}

		void MBAnsagen_Spielstand(FAnsage& Ansage, const FZaehlweise& Z)
		{
			const int A = Z.GetSpiele(0);
			const int B = Z.GetSpiele(1);
			if (A == B)
			{
				Ansage.Bausteine.push_back(ZahlBaustein(A));
				Ansage.Bausteine.push_back(A == 1 ? "game_all" : "games_all");
				return;
			}
			const int Fuehrender = A > B ? 0 : 1;
			const int Hoch = A > B ? A : B;
			const int Niedrig = A > B ? B : A;
			Ansage.Bausteine.push_back(MBAnsagen_Name(Fuehrender));
			Ansage.Bausteine.push_back("leads");
			Ansage.Bausteine.push_back(ZahlBaustein(Hoch));
			Ansage.Bausteine.push_back(Hoch == 1 ? "game_to" : "games_to");
			Ansage.Bausteine.push_back(MBAnsagen_SpielZahl(Niedrig));
		}
	}

	std::string ZahlBaustein(int Zahl)
	{
		const int Begrenzt = Zahl < 0 ? 0 : (Zahl > 40 ? 40 : Zahl);
		return "n" + std::to_string(Begrenzt);
	}

	bool IstSatzzeichen(const std::string& Baustein)
	{
		return Baustein == "," || Baustein == ".";
	}

	std::string FAnsage::Text(const std::map<std::string, std::string>& Texte, const std::string Namen[2]) const
	{
		std::string Ergebnis;
		bool bGross = true;
		for (const std::string& Baustein : Bausteine)
		{
			if (IstSatzzeichen(Baustein))
			{
				if (!Ergebnis.empty() && Ergebnis.back() != ',' && Ergebnis.back() != '.')
				{
					Ergebnis += Baustein;
				}
				bGross = Baustein == ".";
				continue;
			}
			std::string Teil;
			if (Baustein == "@0" || Baustein == "@1")
			{
				Teil = Namen[Baustein == "@1" ? 1 : 0];
			}
			else
			{
				const auto Treffer = Texte.find(Baustein);
				Teil = Treffer != Texte.end() ? Treffer->second : Baustein;
			}
			if (Teil.empty())
			{
				continue;
			}
			if (bGross && Teil[0] >= 'a' && Teil[0] <= 'z')
			{
				Teil[0] = static_cast<char>(Teil[0] - 'a' + 'A');
			}
			bGross = false;
			if (!Ergebnis.empty())
			{
				Ergebnis += ' ';
			}
			Ergebnis += Teil;
		}
		return Ergebnis;
	}

	FAnsage FAnsage::OhneNamen(int Aufschlaeger) const
	{
		FAnsage Ergebnis;
		for (std::size_t Index = 0; Index < Bausteine.size(); ++Index)
		{
			const std::string& Baustein = Bausteine[Index];
			const bool bName = Baustein == "@0" || Baustein == "@1";
			if (!bName)
			{
				Ergebnis.Bausteine.push_back(Baustein);
				continue;
			}
			const int Spieler = Baustein == "@1" ? 1 : 0;
			if (!Ergebnis.Bausteine.empty() && Ergebnis.Bausteine.back() == "advantage")
			{
				Ergebnis.Bausteine.push_back(Spieler == Aufschlaeger ? "server" : "receiver");
				continue;
			}
			// "@x leads ..." -> "..." (Name und "leads" entfallen)
			if (Index + 1 < Bausteine.size() && Bausteine[Index + 1] == "leads")
			{
				++Index;
			}
		}
		// Kein Satzzeichen am Ende stehen lassen ("one zero," -> "one zero")
		while (!Ergebnis.Bausteine.empty() && Ergebnis.Bausteine.back() == ",")
		{
			Ergebnis.Bausteine.pop_back();
		}
		return Ergebnis;
	}

	FAnsage AnsageNachPunkt(const FZaehlweise& Z, const FPunktFolge& Folge)
	{
		FAnsage Ansage;
		if (Folge.Punktgewinner < 0)
		{
			return Ansage;
		}

		if (Folge.bMatchGewonnen)
		{
			const int Sieger = Folge.Matchgewinner;
			const int Verlierer = 1 - Sieger;
			Ansage.Bausteine = {"game_set_and_match", MBAnsagen_Name(Sieger), ","};
			const std::vector<FSatzErgebnis>& Saetze = Z.GetAbgeschlosseneSaetze();
			if (Z.GetRegeln().Gewinnsaetze == 1 && Saetze.size() == 1 && !Saetze.front().bMatchTiebreak)
			{
				// Ein Satz: "six games to four" (bei 1 Spiel pro Satz: "one game to love")
				Ansage.Bausteine.push_back(ZahlBaustein(Saetze.front().Spiele[Sieger]));
				Ansage.Bausteine.push_back(Saetze.front().Spiele[Sieger] == 1 ? "game_to" : "games_to");
				Ansage.Bausteine.push_back(MBAnsagen_SpielZahl(Saetze.front().Spiele[Verlierer]));
				return Ansage;
			}
			if (Z.GetRegeln().Gewinnsaetze > 1)
			{
				Ansage.Bausteine.push_back(ZahlBaustein(Z.GetGewonneneSaetze(Sieger)));
				Ansage.Bausteine.push_back("sets_to");
				Ansage.Bausteine.push_back(MBAnsagen_SpielZahl(Z.GetGewonneneSaetze(Verlierer)));
			}
			for (const FSatzErgebnis& Satz : Saetze)
			{
				Ansage.Bausteine.push_back(",");
				if (Satz.bMatchTiebreak)
				{
					Ansage.Bausteine.push_back("match_tiebreak");
					Ansage.Bausteine.push_back(ZahlBaustein(Satz.TiebreakPunkte[Sieger]));
					Ansage.Bausteine.push_back(ZahlBaustein(Satz.TiebreakPunkte[Verlierer]));
					continue;
				}
				// Satzergebnisse wie auf der Tour: "six four, six love, love six" (0 Spiele = "love", "zero" nur im Tiebreak)
				Ansage.Bausteine.push_back(MBAnsagen_SpielZahl(Satz.Spiele[Sieger]));
				Ansage.Bausteine.push_back(MBAnsagen_SpielZahl(Satz.Spiele[Verlierer]));
			}
			return Ansage;
		}

		if (Folge.bSatzGewonnen)
		{
			const FSatzErgebnis& Satz = Z.GetAbgeschlosseneSaetze().back();
			const int Gewinner = Folge.Satzgewinner;
			const int SatzNummer = static_cast<int>(Z.GetAbgeschlosseneSaetze().size());
			Ansage.Bausteine = {"game_and", MBAnsagen_SatzBaustein(SatzNummer), MBAnsagen_Name(Gewinner)};
			if (Satz.bMatchTiebreak)
			{
				return Ansage;
			}
			Ansage.Bausteine.push_back(",");
			Ansage.Bausteine.push_back(ZahlBaustein(Satz.Spiele[Gewinner]));
			Ansage.Bausteine.push_back(Satz.Spiele[Gewinner] == 1 ? "game_to" : "games_to");
			Ansage.Bausteine.push_back(MBAnsagen_SpielZahl(Satz.Spiele[1 - Gewinner]));
			if (Folge.bMatchTiebreakBeginnt)
			{
				Ansage.Bausteine.push_back(".");
				Ansage.Bausteine.push_back("match_tiebreak");
			}
			return Ansage;
		}

		if (Folge.bSpielGewonnen)
		{
			Ansage.Bausteine = {"game", MBAnsagen_Name(Folge.Spielgewinner), "."};
			MBAnsagen_Spielstand(Ansage, Z);
			if (Folge.bTiebreakBeginnt)
			{
				Ansage.Bausteine.push_back(".");
				Ansage.Bausteine.push_back("tiebreak");
			}
			return Ansage;
		}

		if (Z.IstTiebreak())
		{
			const int A = Z.GetPunkte(0);
			const int B = Z.GetPunkte(1);
			if (A == B)
			{
				Ansage.Bausteine = {ZahlBaustein(A), "all"};
			}
			else
			{
				const int Fuehrender = A > B ? 0 : 1;
				Ansage.Bausteine = {ZahlBaustein(A > B ? A : B), ZahlBaustein(A > B ? B : A), ",", MBAnsagen_Name(Fuehrender)};
			}
			return Ansage;
		}

		if (Z.IstEinstand())
		{
			Ansage.Bausteine = {"deuce"};
			return Ansage;
		}
		const int Vorteil = Z.GetVorteil();
		if (Vorteil >= 0)
		{
			Ansage.Bausteine = {"advantage", MBAnsagen_Name(Vorteil)};
			return Ansage;
		}
		const int Aufschlaeger = Z.GetAufschlaeger();
		const int PunkteAuf = Z.GetPunkte(Aufschlaeger);
		const int PunkteRueck = Z.GetPunkte(1 - Aufschlaeger);
		if (Z.GetRegeln().bNoAd && PunkteAuf == 3 && PunkteRueck == 3)
		{
			Ansage.Bausteine = {"deuce", "deciding_point"};
			return Ansage;
		}
		if (PunkteAuf == PunkteRueck)
		{
			Ansage.Bausteine = {MBAnsagen_PunktBaustein(PunkteAuf), "all"};
			return Ansage;
		}
		Ansage.Bausteine = {MBAnsagen_PunktBaustein(PunkteAuf), MBAnsagen_PunktBaustein(PunkteRueck)};
		return Ansage;
	}

	FAnsage AnsageRuf(const char* Baustein)
	{
		FAnsage Ansage;
		Ansage.Bausteine.push_back(Baustein);
		return Ansage;
	}

	std::string LinienBaustein(ELinie Linie, double PositionY)
	{
		// Stuhl bei X > 0 mit Blick nach -X: links = nahe Seite (Y < 0), rechts = ferne Seite
		const bool bLinks = PositionY < 0.0;
		switch (Linie)
		{
		case ELinie::Grundlinie: return bLinks ? "left_baseline" : "right_baseline";
		case ELinie::Aufschlaglinie: return bLinks ? "left_service_line" : "right_service_line";
		case ELinie::SeitenlinieRechts: return "near_sideline";
		case ELinie::SeitenlinieLinks: return "far_sideline";
		case ELinie::Mittellinie: return "centre_service_line";
		case ELinie::Keine: break;
		}
		return "line";
	}

	FAnsage AnsageChallenge(int Spieler, ELinie Linie, double PositionY)
	{
		FAnsage Ansage;
		Ansage.Bausteine = {MBAnsagen_Name(Spieler), "is_challenging_the_call_on_the", LinienBaustein(Linie, PositionY)};
		return Ansage;
	}

	FAnsage AnsageChallengeErgebnis(const FStrittigeEntscheidung& Entscheidung, const FChallengeAuswertung& Auswertung)
	{
		FAnsage Ansage;
		if (!Auswertung.bErfolgreich)
		{
			Ansage.Bausteine = {Entscheidung.bWarIn ? "the_ball_was_good" : "the_ball_was_out"};
			return Ansage;
		}
		Ansage.Bausteine = {"correction", ",", Entscheidung.bWarIn ? "the_ball_was_good" : "the_ball_was_out"};
		switch (Auswertung.Folge)
		{
		case EChallengeFolge::ErsterAufschlag:
		case EChallengeFolge::PunktWiederholen:
			Ansage.Bausteine.push_back(".");
			Ansage.Bausteine.push_back("replay_the_point");
			Ansage.Bausteine.push_back(".");
			Ansage.Bausteine.push_back("first_serve");
			break;
		case EChallengeFolge::Aufschlagfehler:
			Ansage.Bausteine.push_back(".");
			Ansage.Bausteine.push_back("second_serve");
			break;
		default:
			break;
		}
		return Ansage;
	}
}
