// Matchball Tennis - Testhilfe: liest die Werte-Tabellen aus Data/ (CSV), damit die Tests genau die
// Zahlen pruefen, die auch das Spiel benutzt (Unreal importiert dieselben CSV-Dateien als Data Tables).

#pragma once

#include "Physics/Kern/MBBallPhysik.h"

#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#if !defined(MB_KERN_EIGENSTAENDIG)
#include "Misc/Paths.h"
#endif

namespace MBTest
{
	// Verzeichnis Data/ des Unreal-Projekts
	inline std::string DatenVerzeichnis()
	{
#if defined(MB_KERN_EIGENSTAENDIG)
		return std::string(MB_DATEN_PFAD);
#else
		const FString Pfad = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("Data")));
		return std::string(TCHAR_TO_UTF8(*Pfad));
#endif
	}

	using FZeile = std::map<std::string, std::string>;

	// Minimaler CSV-Leser: Komma-getrennt, erste Zeile = Kopf, Felder ohne Kommas in Anfuehrungszeichen
	inline std::vector<FZeile> LiesCsv(const std::string& Dateiname)
	{
		std::vector<FZeile> Zeilen;
		std::ifstream Datei(DatenVerzeichnis() + "/" + Dateiname);
		if (!Datei)
		{
			return Zeilen;
		}
		auto Zerlege = [](const std::string& Text)
		{
			std::vector<std::string> Felder;
			std::string Feld;
			bool bInAnfuehrung = false;
			for (char Zeichen : Text)
			{
				if (Zeichen == '"')
				{
					bInAnfuehrung = !bInAnfuehrung;
				}
				else if (Zeichen == ',' && !bInAnfuehrung)
				{
					Felder.push_back(Feld);
					Feld.clear();
				}
				else if (Zeichen != '\r')
				{
					Feld.push_back(Zeichen);
				}
			}
			Felder.push_back(Feld);
			return Felder;
		};
		std::string Zeile;
		std::vector<std::string> Kopf;
		while (std::getline(Datei, Zeile))
		{
			if (Zeile.empty() || Zeile[0] == '#')
			{
				continue;
			}
			const std::vector<std::string> Felder = Zerlege(Zeile);
			if (Kopf.empty())
			{
				Kopf = Felder;
				continue;
			}
			FZeile Eintrag;
			for (std::size_t Index = 0; Index < Kopf.size() && Index < Felder.size(); ++Index)
			{
				Eintrag[Kopf[Index]] = Felder[Index];
			}
			Zeilen.push_back(Eintrag);
		}
		return Zeilen;
	}

	inline double Zahl(const FZeile& Zeile, const std::string& Spalte, double Ersatz = 0.0)
	{
		const auto Treffer = Zeile.find(Spalte);
		if (Treffer == Zeile.end() || Treffer->second.empty())
		{
			return Ersatz;
		}
		return std::strtod(Treffer->second.c_str(), nullptr);
	}

	inline std::string Text(const FZeile& Zeile, const std::string& Spalte)
	{
		const auto Treffer = Zeile.find(Spalte);
		return Treffer == Zeile.end() ? std::string() : Treffer->second;
	}

	inline MB::FBallParameter LadeBall()
	{
		MB::FBallParameter Ball;
		const std::vector<FZeile> Zeilen = LiesCsv("ball.csv");
		if (!Zeilen.empty())
		{
			const FZeile& Z = Zeilen.front();
			Ball.Masse = Zahl(Z, "Masse", Ball.Masse);
			Ball.Radius = Zahl(Z, "Radius", Ball.Radius);
			Ball.Luftdichte = Zahl(Z, "Luftdichte", Ball.Luftdichte);
			Ball.Widerstandsbeiwert = Zahl(Z, "Widerstandsbeiwert", Ball.Widerstandsbeiwert);
			Ball.TraegheitsFaktor = Zahl(Z, "TraegheitsFaktor", Ball.TraegheitsFaktor);
			Ball.DrallAbnahmeProMeter = Zahl(Z, "DrallAbnahmeProMeter", Ball.DrallAbnahmeProMeter);
			Ball.Schwerkraft = Zahl(Z, "Schwerkraft", Ball.Schwerkraft);
			Ball.BallSteifigkeit = Zahl(Z, "BallSteifigkeit", Ball.BallSteifigkeit);
		}
		return Ball;
	}

	inline MB::FNetzParameter LadeNetz()
	{
		MB::FNetzParameter Netz;
		const std::vector<FZeile> Zeilen = LiesCsv("netz.csv");
		if (!Zeilen.empty())
		{
			const FZeile& Z = Zeilen.front();
			Netz.BandRadius = Zahl(Z, "BandRadius", Netz.BandRadius);
			Netz.BandStosszahl = Zahl(Z, "BandStosszahl", Netz.BandStosszahl);
			Netz.BandTangentialFaktor = Zahl(Z, "BandTangentialFaktor", Netz.BandTangentialFaktor);
			Netz.GeflechtStosszahl = Zahl(Z, "GeflechtStosszahl", Netz.GeflechtStosszahl);
			Netz.GeflechtDaempfung = Zahl(Z, "GeflechtDaempfung", Netz.GeflechtDaempfung);
		}
		return Netz;
	}

	// Belag nach Name ("Hart", "Sand", "Rasen", "Halle"); bOk = false, wenn nicht gefunden
	inline MB::FBelagParameter LadeBelag(const std::string& Name, bool* bOk = nullptr)
	{
		MB::FBelagParameter Belag;
		bool bGefunden = false;
		for (const FZeile& Z : LiesCsv("belaege.csv"))
		{
			if (Text(Z, "Name") != Name)
			{
				continue;
			}
			Belag.Name = Name;
			Belag.Reibung = Zahl(Z, "Reibung", Belag.Reibung);
			Belag.Stosszahl = Zahl(Z, "Stosszahl", Belag.Stosszahl);
			Belag.StosszahlReferenz = Zahl(Z, "StosszahlReferenz", Belag.StosszahlReferenz);
			Belag.StosszahlAbnahme = Zahl(Z, "StosszahlAbnahme", Belag.StosszahlAbnahme);
			Belag.RollReibung = Zahl(Z, "RollReibung", Belag.RollReibung);
			bGefunden = true;
			break;
		}
		if (bOk != nullptr)
		{
			*bOk = bGefunden;
		}
		return Belag;
	}

	inline MB::FBallSimulation Simulation(const std::string& Belag)
	{
		return MB::FBallSimulation(LadeBall(), LadeBelag(Belag), LadeNetz());
	}
}
