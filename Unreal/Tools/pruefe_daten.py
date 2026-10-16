#!/usr/bin/env python3
"""Matchball Tennis - Konsistenzpruefung der Data-Table-CSV-Dateien.

Prueft fuer jede CSV-Datei in Unreal/Data/, die als Unreal-Data-Table importiert wird:
  * die Spaltennamen entsprechen exakt den UPROPERTY-Feldern der Zeilenstruktur
    in Source/MatchballTennis/Core/MBDatenTabellen.h (sonst verliert der Import Werte),
  * jede Zeile hat so viele Felder wie der Kopf,
  * Zeilennamen (erste Spalte) sind eindeutig und nicht leer,
  * Zahlenfelder (double/int32) und bool-Felder (True/False) sind gueltig.

Aufruf (Repository-Wurzel oder beliebig):  python Unreal/Tools/pruefe_daten.py
Exit-Code 0 = alles in Ordnung. Laeuft ohne Zusatzpakete (Python 3.8+), auch in der CI.
"""

import csv
import pathlib
import re
import sys

UNREAL = pathlib.Path(__file__).resolve().parents[1]
DATEN = UNREAL / "Data"
STRUKTUREN = UNREAL / "Source" / "MatchballTennis" / "Core" / "MBDatenTabellen.h"

# CSV-Datei -> Zeilenstruktur (muss zu Tools/Python/import_daten.py passen)
ZUORDNUNG = {
    "belaege": "FMBBelagZeile",
    "ball": "FMBBallZeile",
    "netz": "FMBNetzZeile",
    "schlagarten": "FMBSchlagartZeile",
    "timing": "FMBTimingZeile",
    "ballmaschine": "FMBBallmaschineZeile",
    "ansagen": "FMBAnsageZeile",
    "spieler": "FMBSpielerZeile",
}


def lies_strukturen():
    """Liefert {Strukturname: [(Typ, Feldname), ...]} aus MBDatenTabellen.h."""
    quelle = STRUKTUREN.read_text(encoding="utf-8")
    ergebnis = {}
    for treffer in re.finditer(r"struct MATCHBALLTENNIS_API (F\w+) : public FTableRowBase\s*\{(.*?)\n\};", quelle, re.S):
        felder = re.findall(r"UPROPERTY\([^)]*\)\s*([\w<>:]+)\s+(\w+)\s*(?:=|;)", treffer.group(2))
        ergebnis[treffer.group(1)] = felder
    return ergebnis


def pruefe_wert(typ, wert):
    if typ in ("double", "float"):
        float(wert)
    elif typ in ("int32", "int"):
        int(wert)
    elif typ == "bool":
        if wert not in ("True", "False", "true", "false"):
            raise ValueError("bool erwartet True/False")
    return True


def main():
    strukturen = lies_strukturen()
    fehler = []
    for datei, struktur in ZUORDNUNG.items():
        pfad = DATEN / f"{datei}.csv"
        if struktur not in strukturen:
            fehler.append(f"{struktur} nicht in MBDatenTabellen.h gefunden")
            continue
        if not pfad.exists():
            fehler.append(f"{pfad.name} fehlt")
            continue
        with pfad.open(encoding="utf-8", newline="") as handle:
            zeilen = list(csv.reader(handle))
        kopf, daten = zeilen[0], zeilen[1:]
        felder = dict((name, typ) for typ, name in strukturen[struktur])
        nur_csv = [spalte for spalte in kopf[1:] if spalte not in felder]
        nur_struktur = [name for name in felder if name not in kopf[1:]]
        if nur_csv or nur_struktur:
            fehler.append(f"{pfad.name}: Spalten passen nicht zu {struktur} (nur CSV: {nur_csv}, nur Struktur: {nur_struktur})")
        namen = set()
        for nummer, zeile in enumerate(daten, start=2):
            if not zeile:
                continue
            if len(zeile) != len(kopf):
                fehler.append(f"{pfad.name}:{nummer}: {len(zeile)} Felder, Kopf hat {len(kopf)}")
                continue
            if not zeile[0] or zeile[0] in namen:
                fehler.append(f"{pfad.name}:{nummer}: Zeilenname leer oder doppelt ({zeile[0]!r})")
            namen.add(zeile[0])
            for spalte, wert in zip(kopf[1:], zeile[1:]):
                typ = felder.get(spalte)
                if typ is None:
                    continue
                try:
                    pruefe_wert(typ, wert)
                except ValueError as ausnahme:
                    fehler.append(f"{pfad.name}:{nummer}: {spalte}={wert!r} ist kein gueltiger {typ} ({ausnahme})")
        print(f"{pfad.name:18s} -> {struktur:22s} {len(daten):3d} Zeilen")
    if fehler:
        print("\nFEHLER:")
        for eintrag in fehler:
            print("  - " + eintrag)
        return 1
    print("\nAlle Data-Table-CSV-Dateien sind konsistent.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
