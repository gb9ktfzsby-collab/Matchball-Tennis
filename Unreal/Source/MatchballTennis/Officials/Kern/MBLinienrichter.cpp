// Matchball Tennis - Kern: Linienrichter

#include "Officials/Kern/MBLinienrichter.h"

namespace MB
{
	double LinienrichterFehlerwahrscheinlichkeit(double Abstand, const FLinienrichterParameter& Parameter)
	{
		if (Parameter.bElektronisch || Parameter.MaxAbstand <= 0.0)
		{
			return 0.0;
		}
		const double D = Betrag(Abstand);
		if (D > Parameter.MaxAbstand)
		{
			return 0.0;
		}
		return Parameter.MaxFehler * (1.0 - D / Parameter.MaxAbstand);
	}

	FLinienEntscheidung EntscheideLinie(const FLinienUrteil& Wahrheit, const FLinienrichterParameter& Parameter, FZufall& Zufall)
	{
		FLinienEntscheidung Entscheidung;
		Entscheidung.Wahrheit = Wahrheit;
		Entscheidung.Fehlerwahrscheinlichkeit = LinienrichterFehlerwahrscheinlichkeit(Wahrheit.Abstand, Parameter);
		// Immer genau eine Zahl ziehen (Gleichschritt online), auch wenn p = 0 ist
		Entscheidung.bFehlentscheidung = Zufall.Gleich01() < Entscheidung.Fehlerwahrscheinlichkeit;
		Entscheidung.bGutGegeben = Entscheidung.bFehlentscheidung ? !Wahrheit.bIn : Wahrheit.bIn;
		return Entscheidung;
	}
}
