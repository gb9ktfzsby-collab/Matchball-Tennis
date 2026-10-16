# -*- coding: utf-8 -*-
"""
import_daten.py - legt die Data Tables /Game/Data/DT_<Name> aus Unreal/Data/*.csv an bzw. aktualisiert sie
(README 0.2 "datengetrieben", Dokumente/Architektur.md Abschnitt 3).

Aufruf (lokal):
    UnrealEditor-Cmd.exe <Repo>\\Unreal\\MatchballTennis.uproject -run=pythonscript
        -script="<Repo>/Unreal/Tools/Python/import_daten.py" -unattended -nosplash -nullrhi
    Nur einzelne Tabellen:  -script="<...>/import_daten.py --nur belaege,ball"
Normalerweise ruft Tools/assets.ps1 -> import_alles.py dieses Skript auf.

Zuordnung CSV -> Zeilenstruktur (Source/MatchballTennis/Core/MBDatenTabellen.h) -> Data Table
(muss zu Tools/pruefe_daten.py, Core/MBGameInstance.cpp und dem Test MatchballTennis.Unreal.DatenTabellen passen):

    belaege.csv      FMBBelagZeile         /Game/Data/DT_Belaege
    ball.csv         FMBBallZeile          /Game/Data/DT_Ball
    netz.csv         FMBNetzZeile          /Game/Data/DT_Netz
    schlagarten.csv  FMBSchlagartZeile     /Game/Data/DT_Schlagarten
    timing.csv       FMBTimingZeile        /Game/Data/DT_Timing
    ballmaschine.csv FMBBallmaschineZeile  /Game/Data/DT_Ballmaschine
    ansagen.csv      FMBAnsageZeile        /Game/Data/DT_Ansagen
    spieler.csv      FMBSpielerZeile       /Game/Data/DT_Spieler

Neue Tabellen: Steht eine weitere CSV in Tools/pruefe_daten.py (ZUORDNUNG, von der CI gegen MBDatenTabellen.h
geprueft), wird sie automatisch mit importiert - Tabellenname nach Konvention DT_<Teile gross geschrieben>,
z. B. anlagen.csv -> DT_Anlagen, punkte_atp.csv -> DT_PunkteAtp. Alle anderen CSV-Dateien in Data/ werden mit
Hinweis uebersprungen (noch keine Zeilenstruktur).

Wie die Zeilenstruktur gefunden wird (beide Wege brauchen das kompilierte Modul MatchballTennis, Tools/build.ps1):
  Variante A (Standard): unreal.MBBelagZeile.static_struct()
      Unreal erzeugt beim Laden des Moduls Python-Typen fuer alle USTRUCT(BlueprintType) - Name ohne "F".
  Variante B (Rueckfall): unreal.find_object / unreal.load_object(None, "/Script/MatchballTennis.MBBelagZeile")
      der Objektpfad einer C++-Struktur ist /Script/<Modul>.<Name ohne F>; geht auch ohne Python-Typ.
  Welche Variante gegriffen hat, steht im Log.

Ablauf je Tabelle:
  1. CSV als UTF-8 lesen (BOM erlaubt), Kopf und Zeilennamen pruefen (erste Spalte "Name" = Zeilenname,
     nicht leer, eindeutig).
  2. Vorhandene Tabelle mit derselben Struktur wird an Ort und Stelle neu gefuellt (Verweise bleiben gueltig);
     eine Tabelle mit anderer Struktur oder ein Asset anderen Typs wird geloescht und neu angelegt
     (unreal.DataTableFactory mit struct = Zeilenstruktur).
  3. Fuellen mit unreal.DataTableFunctionLibrary.fill_data_table_from_csv_string (Unreal-CSV-Import:
     Spalten = Eigenschaftsnamen, bool als True/False, Texte mit Komma in Anfuehrungszeichen).
  4. Pruefen: Zeilenzahl und Zeilennamen wie in der CSV, sonst Fehler.
  5. Speichern.
Am Ende eine Tabelle mit allen Ergebnissen; schlaegt eine Tabelle fehl, wirft das Skript MBFehler
(Exit-Code != 0), nachdem alle anderen Tabellen bearbeitet wurden.
"""

import importlib.util
import os
import sys


def _skriptordner():
    try:
        return os.path.dirname(os.path.abspath(__file__))
    except NameError:  # __file__ fehlt nur bei sehr ungewoehnlichem Aufruf
        import unreal
        return os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Tools", "Python")


if _skriptordner() not in sys.path:
    sys.path.insert(0, _skriptordner())

import unreal  # noqa: E402

import mb_unreal as mb  # noqa: E402

ZIELORDNER = "/Game/Data"

#: CSV-Dateiname (ohne .csv) -> (Python-Name der Zeilenstruktur, Data-Table-Name)
ZUORDNUNG = {
    "belaege": ("MBBelagZeile", "DT_Belaege"),
    "ball": ("MBBallZeile", "DT_Ball"),
    "netz": ("MBNetzZeile", "DT_Netz"),
    "schlagarten": ("MBSchlagartZeile", "DT_Schlagarten"),
    "timing": ("MBTimingZeile", "DT_Timing"),
    "ballmaschine": ("MBBallmaschineZeile", "DT_Ballmaschine"),
    "ansagen": ("MBAnsageZeile", "DT_Ansagen"),
    "spieler": ("MBSpielerZeile", "DT_Spieler"),
}


def tabellenname(stamm):
    """Konvention fuer neue Tabellen: 'punkte_atp' -> 'DT_PunkteAtp', 'anlagen' -> 'DT_Anlagen'."""
    return "DT_" + "".join(teil[:1].upper() + teil[1:] for teil in stamm.split("_") if teil)


def _pruefe_daten_zuordnung():
    """ZUORDNUNG aus Tools/pruefe_daten.py ({csv: 'FMB...Zeile'}) oder {} (mit Hinweis)."""
    pfad = mb.pfade().unreal / "Tools" / "pruefe_daten.py"
    if not pfad.is_file():
        return {}
    try:
        spec = importlib.util.spec_from_file_location("mb_pruefe_daten", str(pfad))
        modul = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(modul)
        zuordnung = getattr(modul, "ZUORDNUNG", {})
        return dict(zuordnung) if isinstance(zuordnung, dict) else {}
    except Exception as ausnahme:
        mb.hinweis("Tools/pruefe_daten.py nicht lesbar (%s) - nur die feste Zuordnung wird benutzt." % ausnahme)
        return {}


def zuordnung():
    """CSV-Name -> (Struktur ohne F, Data Table): ZUORDNUNG oben plus neue Eintraege aus Tools/pruefe_daten.py."""
    ergebnis = dict(ZUORDNUNG)
    for stamm, struktur in sorted(_pruefe_daten_zuordnung().items()):
        stamm = str(stamm).lower()
        name = str(struktur)[1:] if str(struktur).startswith("F") else str(struktur)
        if stamm in ergebnis:
            if ergebnis[stamm][0] != name:
                mb.hinweis("%s.csv: pruefe_daten.py nennt %s, import_daten.py F%s - es gilt import_daten.py."
                           % (stamm, struktur, ergebnis[stamm][0]))
            continue
        ergebnis[stamm] = (name, tabellenname(stamm))
        mb.info("%s.csv: Zuordnung aus pruefe_daten.py -> %s / %s" % (stamm, struktur, tabellenname(stamm)))
    return ergebnis


# ---------------------------------------------------------------------------------------------------------------------
# Struktur finden
# ---------------------------------------------------------------------------------------------------------------------

def struktur_finden(name):
    """Liefert (UScriptStruct, Variante) oder (None, Begruendung). name ohne "F", z. B. "MBBelagZeile"."""
    gruende = []
    # Variante A: Python-Typ des USTRUCT
    typ = getattr(unreal, name, None)
    if typ is not None:
        try:
            struktur = typ.static_struct()
            if struktur is not None:
                return struktur, "A: unreal.%s.static_struct()" % name
            gruende.append("unreal.%s.static_struct() lieferte None" % name)
        except Exception as ausnahme:
            gruende.append("unreal.%s.static_struct(): %s" % (name, ausnahme))
    else:
        gruende.append("unreal.%s gibt es nicht" % name)
    # Variante B: Objektpfad der C++-Struktur
    objektpfad = "/Script/%s.%s" % (mb.MODUL_NAME, name)
    for funktion_name in ("find_object", "load_object"):
        funktion = getattr(unreal, funktion_name, None)
        if funktion is None:
            continue
        try:
            struktur = funktion(None, objektpfad)
        except Exception as ausnahme:
            gruende.append("unreal.%s(None, '%s'): %s" % (funktion_name, objektpfad, ausnahme))
            continue
        if struktur is not None and isinstance(struktur, unreal.ScriptStruct):
            return struktur, "B: unreal.%s(None, '%s')" % (funktion_name, objektpfad)
        gruende.append("unreal.%s(None, '%s') lieferte %s" % (funktion_name, objektpfad, mb.klassenname(struktur)))
    return None, "; ".join(gruende)


def zeilen_struktur(tabelle):
    """Zeilenstruktur einer vorhandenen Data Table (oder None, wenn sie sich nicht lesen laesst)."""
    struktur = mb.lies(tabelle, "row_struct")
    if struktur is not None:
        return struktur
    try:
        return unreal.DataTableFunctionLibrary.get_data_table_row_struct(tabelle)
    except Exception:
        return None


def _gleiche_struktur(a, b):
    try:
        return a.get_path_name() == b.get_path_name()
    except Exception:
        return False


# ---------------------------------------------------------------------------------------------------------------------
# CSV
# ---------------------------------------------------------------------------------------------------------------------

class CsvInhalt(object):
    def __init__(self, pfad, text, kopf, zeilennamen):
        self.pfad = pfad
        self.text = text
        self.kopf = kopf
        self.zeilennamen = zeilennamen


def csv_lesen(pfad):
    """Liest eine Data-Table-CSV und prueft Kopf und Zeilennamen. Wirft MBFehler mit Zeilenangabe."""
    text = mb.text_lesen(pfad)
    zeilen = mb.csv_zeilen(text)
    name = os.path.basename(str(pfad))
    if not zeilen:
        mb.abbrechen("%s ist leer." % name)
    kopf = [feld.strip() for feld in zeilen[0]]
    if len(kopf) < 2:
        mb.abbrechen("%s: Kopfzeile braucht mindestens zwei Spalten (Name + Werte): %s" % (name, kopf))
    leere = [i + 1 for i, feld in enumerate(kopf) if not feld]
    if leere:
        mb.abbrechen("%s: leere Spaltennamen in der Kopfzeile (Spalte %s)." % (name, leere))
    doppelt = sorted(set(feld for feld in kopf if kopf.count(feld) > 1))
    if doppelt:
        mb.abbrechen("%s: doppelte Spaltennamen %s." % (name, doppelt))
    namen = []
    gesehen = set()
    for nummer, zeile in enumerate(zeilen[1:], start=2):
        if len(zeile) != len(kopf):
            mb.abbrechen("%s, Datenzeile %d: %d Felder, die Kopfzeile hat %d." % (
                name, nummer - 1, len(zeile), len(kopf)))
        zeilenname = zeile[0].strip()
        if not zeilenname:
            mb.abbrechen("%s, Datenzeile %d: Zeilenname (erste Spalte) ist leer." % (name, nummer - 1))
        if zeilenname.lower() in gesehen:
            mb.abbrechen("%s, Datenzeile %d: Zeilenname '%s' ist doppelt (Unreal unterscheidet keine Gross-/"
                         "Kleinschreibung)." % (name, nummer - 1, zeilenname))
        gesehen.add(zeilenname.lower())
        namen.append(zeilenname)
    if not namen:
        mb.abbrechen("%s enthaelt keine Datenzeilen." % name)
    return CsvInhalt(pfad, text, kopf, namen)


# ---------------------------------------------------------------------------------------------------------------------
# Eine Tabelle
# ---------------------------------------------------------------------------------------------------------------------

def tabelle_importieren(csv_pfad, struktur_name, tabellen_name):
    """Legt /Game/Data/<tabellen_name> an bzw. aktualisiert sie. Rueckgabe: Kurztext fuer die Bilanz."""
    inhalt = csv_lesen(csv_pfad)
    struktur, variante = struktur_finden(struktur_name)
    if struktur is None:
        mb.abbrechen("Zeilenstruktur F%s nicht gefunden (%s). Ist das Modul %s kompiliert und geladen? "
                     "Zuerst Tools\\build.ps1 ausfuehren." % (struktur_name, variante, mb.MODUL_NAME))
    mb.info("%s: Struktur %s (Variante %s)" % (os.path.basename(str(csv_pfad)), struktur.get_name(), variante))

    pfad = mb.asset_pfad(ZIELORDNER, tabellen_name)
    tabelle = mb.vorhandenes_asset(pfad, unreal.DataTable)
    aktion = "aktualisiert"
    if tabelle is not None:
        vorhanden = zeilen_struktur(tabelle)
        if vorhanden is None:
            mb.hinweis("%s: Zeilenstruktur nicht lesbar - Tabelle wird neu angelegt." % pfad)
            mb.asset_loeschen(pfad, "Zeilenstruktur nicht lesbar")
            tabelle = None
        elif not _gleiche_struktur(vorhanden, struktur):
            mb.asset_loeschen(pfad, "Struktur %s statt %s" % (vorhanden.get_path_name(), struktur.get_path_name()))
            tabelle = None
    if tabelle is None:
        fabrik = unreal.DataTableFactory()
        mb.setze(fabrik, "struct", struktur, was="DataTableFactory")
        tabelle = mb.neues_asset(tabellen_name, ZIELORDNER, unreal.DataTable, fabrik)
        aktion = "neu angelegt"

    try:
        ok = unreal.DataTableFunctionLibrary.fill_data_table_from_csv_string(tabelle, inhalt.text)
    except Exception as ausnahme:
        mb.abbrechen("%s: fill_data_table_from_csv_string fehlgeschlagen: %s" % (pfad, ausnahme))
    if ok is False:
        mb.abbrechen("%s: Unreal meldet Probleme beim CSV-Import (Details im Log unter 'LogDataTable'). Spalten "
                     "pruefen: python Unreal/Tools/pruefe_daten.py" % pfad)

    try:
        zeilen = [str(n) for n in unreal.DataTableFunctionLibrary.get_data_table_row_names(tabelle)]
    except Exception as ausnahme:
        mb.abbrechen("%s: Zeilennamen nicht lesbar: %s" % (pfad, ausnahme))
    if len(zeilen) != len(inhalt.zeilennamen):
        mb.abbrechen("%s: %d Zeilen nach dem Import, die CSV hat %d." % (pfad, len(zeilen), len(inhalt.zeilennamen)))
    fehlend = sorted(set(n.lower() for n in inhalt.zeilennamen) - set(n.lower() for n in zeilen))
    if fehlend:
        mb.abbrechen("%s: Zeilen fehlen nach dem Import: %s" % (pfad, ", ".join(fehlend[:10])))

    mb.speichern(tabelle)
    mb.info("%s %s: %d Zeilen aus %s" % (pfad, aktion, len(zeilen), mb.relativ(csv_pfad)))
    return "%d Zeilen, %s" % (len(zeilen), aktion)


# ---------------------------------------------------------------------------------------------------------------------
# Alle Tabellen
# ---------------------------------------------------------------------------------------------------------------------

def ausfuehren(nur=None, daten_ordner=None):
    """Importiert alle bekannten CSV-Dateien. nur: Liste von CSV-Namen (ohne .csv) oder None fuer alle.
    Rueckgabe: mb.Bilanz. Wirft MBFehler, wenn eine Tabelle fehlschlaegt."""
    ordner = daten_ordner or mb.pfade().daten
    tabellen = zuordnung()
    nur = [n.lower() for n in (nur or [])]
    unbekannt = [n for n in nur if n not in tabellen]
    if unbekannt:
        mb.abbrechen("Unbekannte Tabellen in --nur: %s (bekannt: %s)" % (
            ", ".join(unbekannt), ", ".join(sorted(tabellen))))
    if not os.path.isdir(str(ordner)):
        mb.abbrechen("Datenordner fehlt: %s" % ordner)

    bilanz = mb.Bilanz("Data Tables (%s)" % ZIELORDNER)
    mb.ordner_anlegen(ZIELORDNER)
    dateien = sorted(n for n in os.listdir(str(ordner)) if n.lower().endswith(".csv"))
    gefunden = set()
    for dateiname in dateien:
        stamm = dateiname[:-4].lower()
        if stamm not in tabellen:
            if not nur:
                mb.hinweis("%s uebersprungen: keine Zeilenstruktur zugeordnet (ZUORDNUNG in import_daten.py und "
                           "MBDatenTabellen.h ergaenzen)." % dateiname)
                bilanz.uebersprungen(dateiname, "keine Zeilenstruktur zugeordnet")
            continue
        gefunden.add(stamm)
        if nur and stamm not in nur:
            continue
        struktur_name, tabellen_name = tabellen[stamm]
        try:
            details = tabelle_importieren(os.path.join(str(ordner), dateiname), struktur_name, tabellen_name)
            bilanz.ok("%s -> %s" % (dateiname, tabellen_name), details)
        except Exception as ausnahme:
            if not isinstance(ausnahme, mb.MBFehler):
                mb.fehler("%s: %s" % (dateiname, mb.ausnahme_text(ausnahme)))
            bilanz.fehlgeschlagen("%s -> %s" % (dateiname, tabellen_name), mb.ausnahme_text(ausnahme))

    for stamm in sorted(tabellen):
        if stamm not in gefunden and (not nur or stamm in nur):
            text = "Data/%s.csv fehlt - %s kann nicht erzeugt werden (das Spiel nimmt dann Kern-Standardwerte)." % (
                stamm, tabellen[stamm][1])
            mb.fehler(text)
            bilanz.fehlgeschlagen("%s.csv" % stamm, "Datei fehlt")

    bilanz.ausgeben()
    if bilanz.anzahl_fehler:
        raise mb.MBFehler("Data Tables: %d Fehler (siehe Tabelle oben)." % bilanz.anzahl_fehler)
    return bilanz


def main(argv=None):
    p = mb.parser("Data Tables aus Unreal/Data/*.csv anlegen bzw. aktualisieren")
    p.add_argument("--nur", default="", help="nur diese CSV-Dateien, z. B. belaege,ball")
    argumente = mb.argumente_lesen(p, argv)
    mb.umgebung_melden()
    return ausfuehren(nur=mb.liste_aus_text(argumente.nur))


if __name__ == "__main__":
    mb.als_skript_ausfuehren(main, "Data Tables importieren (import_daten.py)")
