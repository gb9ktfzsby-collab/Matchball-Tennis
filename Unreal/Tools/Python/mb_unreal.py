# -*- coding: utf-8 -*-
"""
mb_unreal.py - gemeinsame Hilfen der Unreal-Editor-Python-Skripte von Matchball Tennis
(README 0.2, 3.2, 19.4.1).

Aufruf der Skripte lokal (Windows, Editor ohne Oberflaeche, siehe Tools/assets.ps1):

    UnrealEditor-Cmd.exe <Repo>\\Unreal\\MatchballTennis.uproject -run=pythonscript
        -script="<Repo>/Unreal/Tools/Python/import_alles.py" -unattended -nosplash -nullrhi

Argumente fuer das Skript stehen mit im -script-Wert:  -script="<...>/import_alles.py --neu"
Im laufenden Editor geht auch:  Ausgabe-Log -> Python -> py "<...>/Tools/Python/import_alles.py"

Regeln fuer alle Skripte in Tools/Python/
  * Unreal Engine 5.4 bis 5.8 (eingebautes Python 3.11), nur stabile APIs:
    unreal.get_editor_subsystem(unreal.EditorAssetSubsystem) (Rueckfall unreal.EditorAssetLibrary),
    unreal.AssetToolsHelpers.get_asset_tools(), LevelEditorSubsystem, EditorActorSubsystem.
    Alles, was es nicht in jeder dieser Versionen gibt, steht in try/except mit klarer Meldung.
  * Fehler: MBFehler (RuntimeError) werfen. Das PythonScriptCommandlet meldet dann
    "Python script executed with errors" und der Prozess endet mit Exit-Code != 0
    (unreal.SystemLibrary.quit_editor() geht im Commandlet nicht, sys.exit() ist dort unsicher).
  * Meldungen: info() -> unreal.log, hinweis() -> unreal.log_warning, fehler() -> unreal.log_error.
    Fehlerzeilen ("LogPython: Error: ...") zaehlen im Commandlet als Fehler und werden von assets.ps1
    angezeigt - deshalb fehler() nur fuer echte Fehler benutzen.
  * Pfade im Projekt: /Game/<Ordner>/<Asset> (nur A-Z, a-z, 0-9, _ und -; keine Umlaute).
  * Erzeugte Assets sind reproduzierbar (README 0.2): ein vorhandenes Asset wird ersetzt bzw. aktualisiert,
    nie von Hand nachbearbeitet.
"""

import argparse
import csv
import datetime
import io
import json
import os
import re
import sys
import time
from pathlib import Path

import unreal

PRAEFIX = "[MB] "
UPROJECT_NAME = "MatchballTennis.uproject"
MODUL_NAME = "MatchballTennis"
UNTERSTUETZTE_VERSIONEN = ((5, 4), (5, 8))

_PROJEKT_ORDNER = None
_PFADE = None
_SUBSYSTEME = {}


class MBFehler(RuntimeError):
    """Fehler, der das Skript (und damit das Commandlet) mit Exit-Code != 0 beendet."""


# ---------------------------------------------------------------------------------------------------------------------
# Meldungen
# ---------------------------------------------------------------------------------------------------------------------

def info(text):
    unreal.log(PRAEFIX + str(text))


def hinweis(text):
    unreal.log_warning(PRAEFIX + str(text))


def fehler(text):
    unreal.log_error(PRAEFIX + str(text))


def abbrechen(text):
    """Meldet einen Fehler und wirft MBFehler."""
    fehler(text)
    raise MBFehler(str(text))


def schritt(titel):
    info("")
    info("=" * 100)
    info(titel)
    info("=" * 100)


def dauer_text(sekunden):
    sekunden = max(0.0, float(sekunden))
    if sekunden < 60.0:
        return "%.1f s" % sekunden
    minuten, rest = divmod(int(round(sekunden)), 60)
    return "%d min %02d s" % (minuten, rest)


def ausnahme_text(ausnahme):
    """Kurztext einer Ausnahme: 'Typ: Meldung' (MBFehler nur mit Meldung)."""
    if isinstance(ausnahme, MBFehler):
        return str(ausnahme)
    return "%s: %s" % (type(ausnahme).__name__, ausnahme)


class Bilanz(object):
    """Sammelt Ergebnisse eines Schritts (OK / HINWEIS / UEBERSPRUNGEN / FEHLER) und gibt eine Tabelle aus."""

    OK = "OK"
    HINWEIS = "HINWEIS"
    UEBERSPRUNGEN = "UEBERSPRUNGEN"
    FEHLER = "FEHLER"

    def __init__(self, titel):
        self.titel = titel
        self.eintraege = []

    def eintragen(self, status, name, details=""):
        self.eintraege.append((status, str(name), str(details)))

    def ok(self, name, details=""):
        self.eintragen(self.OK, name, details)

    def uebersprungen(self, name, details=""):
        self.eintragen(self.UEBERSPRUNGEN, name, details)

    def mit_hinweis(self, name, details=""):
        self.eintragen(self.HINWEIS, name, details)

    def fehlgeschlagen(self, name, details=""):
        self.eintragen(self.FEHLER, name, details)

    def anzahl(self, status):
        return sum(1 for e in self.eintraege if e[0] == status)

    @property
    def anzahl_fehler(self):
        return self.anzahl(self.FEHLER)

    def ausgeben(self):
        info("")
        info("%s - Ergebnis:" % self.titel)
        if not self.eintraege:
            info("  (keine Eintraege)")
        for status, name, details in self.eintraege:
            zeile = "  %-14s %-34s %s" % (status, name, details)
            if status == self.FEHLER:
                fehler(zeile.strip())
            elif status == self.HINWEIS:
                hinweis(zeile.strip())
            else:
                info(zeile)
        info("  Summe: %d OK, %d mit Hinweis, %d uebersprungen, %d Fehler"
             % (self.anzahl(self.OK), self.anzahl(self.HINWEIS), self.anzahl(self.UEBERSPRUNGEN), self.anzahl_fehler))


# ---------------------------------------------------------------------------------------------------------------------
# Projekt- und Repository-Pfade
# ---------------------------------------------------------------------------------------------------------------------

def _ordner_aus_datei():
    """Unreal/Tools/Python/mb_unreal.py -> Unreal/"""
    return Path(__file__).resolve().parents[2]


def projekt_ordner():
    """Ordner mit MatchballTennis.uproject (= <Repo>/Unreal).

    Quelle ist unreal.Paths.project_dir() (das im Editor geladene Projekt). Liegen die Skripte in einer anderen
    Arbeitskopie als das geladene Projekt, gibt es einen Hinweis - Daten und Assets kommen dann aus dem Projekt."""
    global _PROJEKT_ORDNER
    if _PROJEKT_ORDNER is not None:
        return _PROJEKT_ORDNER
    datei_ordner = _ordner_aus_datei()
    kandidat = None
    try:
        roh = unreal.Paths.project_dir()
        voll = unreal.Paths.convert_relative_path_to_full(roh)
        if voll:
            kandidat = Path(voll).resolve()
    except Exception as ausnahme:  # nur ausserhalb des Editors (Tests) denkbar
        hinweis("unreal.Paths.project_dir() nicht verfuegbar (%s) - nehme den Ordner der Skripte." % ausnahme)
    if kandidat is None or not (kandidat / UPROJECT_NAME).is_file():
        if kandidat is not None:
            hinweis("Im Projektordner %s liegt keine %s - nehme den Ordner der Skripte." % (kandidat, UPROJECT_NAME))
        kandidat = datei_ordner
    elif _normiert(kandidat) != _normiert(datei_ordner):
        hinweis("Die Skripte liegen in %s, geladen ist aber das Projekt %s - Daten und Assets kommen aus dem "
                "geladenen Projekt." % (datei_ordner, kandidat))
    if not (kandidat / UPROJECT_NAME).is_file():
        abbrechen("Projektdatei %s nicht gefunden (gesucht in %s)." % (UPROJECT_NAME, kandidat))
    _PROJEKT_ORDNER = kandidat
    return _PROJEKT_ORDNER


def _normiert(pfad):
    return os.path.normcase(str(Path(pfad).resolve()))


class Pfade(object):
    """Alle Pfade, die die Skripte brauchen (entspricht Get-MBPfade in Tools/MBGemeinsam.psm1)."""

    def __init__(self, unreal_ordner):
        self.unreal = Path(unreal_ordner)
        self.repo = self.unreal.parent
        self.uproject = self.unreal / UPROJECT_NAME
        self.daten = self.unreal / "Data"
        self.chunks_json = self.daten / "chunks.json"
        self.content = self.unreal / "Content"
        self.source_assets = self.unreal / "SourceAssets"
        self.erzeugt = self.source_assets / "_erzeugt"
        self.vorschau = self.source_assets / "_vorschau"
        self.tools_python = self.unreal / "Tools" / "Python"
        self.vorgaben = self.repo / "Vorgaben"


def pfade():
    global _PFADE
    if _PFADE is None:
        _PFADE = Pfade(projekt_ordner())
    return _PFADE


def relativ(pfad):
    """Pfad relativ zur Repository-Wurzel (fuer Meldungen), sonst unveraendert."""
    try:
        return Path(pfad).resolve().relative_to(pfade().repo.resolve()).as_posix()
    except Exception:
        return str(pfad)


def text_lesen(pfad):
    """Textdatei als UTF-8 (mit oder ohne BOM), Zeilenenden vereinheitlicht auf \\n."""
    pfad = Path(pfad)
    try:
        roh = pfad.read_bytes()
    except OSError as ausnahme:
        abbrechen("Datei nicht lesbar: %s (%s)" % (pfad, ausnahme))
    try:
        text = roh.decode("utf-8-sig")
    except UnicodeDecodeError as ausnahme:
        abbrechen("Datei ist kein UTF-8: %s (%s)" % (pfad, ausnahme))
    return text.replace("\r\n", "\n").replace("\r", "\n")


def json_lesen(pfad):
    text = text_lesen(pfad)
    try:
        return json.loads(text)
    except ValueError as ausnahme:
        abbrechen("Kein gueltiges JSON: %s (%s)" % (pfad, ausnahme))


def csv_zeilen(text):
    """CSV-Text -> Liste von Zeilen (Listen), leere Zeilen weggelassen. Anfuehrungszeichen wie Unreal."""
    return [z for z in csv.reader(io.StringIO(text)) if z and any(feld.strip() for feld in z)]


# ---------------------------------------------------------------------------------------------------------------------
# Unreal-Pfade (/Game/...)
# ---------------------------------------------------------------------------------------------------------------------

_SPIELPFAD = re.compile(r"^/Game(/[A-Za-z0-9_\-]+)+$")


def spielpfad(pfad, was="Pfad", wurzel_erlaubt=False):
    """Prueft einen Ordner- oder Asset-Pfad unter /Game und gibt ihn ohne Schraegstrich am Ende zurueck.
    wurzel_erlaubt=True laesst auch "/Game" selbst zu (z. B. zum Durchsuchen des ganzen Contents)."""
    if not isinstance(pfad, str):
        abbrechen("%s ist kein Text: %r" % (was, pfad))
    pfad = pfad.strip().rstrip("/")
    if wurzel_erlaubt and pfad == "/Game":
        return pfad
    if not _SPIELPFAD.match(pfad):
        abbrechen("%s ungueltig: '%s' (erwartet /Game/<Ordner>/..., nur A-Z, a-z, 0-9, _ und -)" % (was, pfad))
    return pfad


def asset_pfad(ordner, name):
    """('/Game/Data', 'DT_Ball') -> '/Game/Data/DT_Ball'"""
    return spielpfad("%s/%s" % (str(ordner).rstrip("/"), name), "Asset-Pfad")


def paket_pfad(pfad):
    """'/Game/Data/DT_Ball.DT_Ball' -> '/Game/Data/DT_Ball' (Objektteil entfernen)."""
    pfad = str(pfad)
    letzter = pfad.rsplit("/", 1)
    if len(letzter) == 2 and "." in letzter[1]:
        return letzter[0] + "/" + letzter[1].split(".", 1)[0]
    return pfad


def objekt_pfad(pfad):
    """'/Game/Data/DT_Ball' -> '/Game/Data/DT_Ball.DT_Ball'"""
    paket = paket_pfad(pfad)
    return "%s.%s" % (paket, paket.rsplit("/", 1)[-1])


def name_von(pfad):
    return paket_pfad(pfad).rsplit("/", 1)[-1]


def ordner_von(pfad):
    return paket_pfad(pfad).rsplit("/", 1)[0]


# ---------------------------------------------------------------------------------------------------------------------
# Subsysteme und Werkzeuge des Editors
# ---------------------------------------------------------------------------------------------------------------------

def _editor_subsystem(klassenname):
    if klassenname in _SUBSYSTEME:
        return _SUBSYSTEME[klassenname]
    klasse = getattr(unreal, klassenname, None)
    subsystem = None
    if klasse is not None:
        try:
            subsystem = unreal.get_editor_subsystem(klasse)
        except Exception as ausnahme:
            hinweis("%s nicht verfuegbar: %s" % (klassenname, ausnahme))
    _SUBSYSTEME[klassenname] = subsystem
    return subsystem


def editor_subsystem(klassenname):
    """Beliebiges Editor-Subsystem per Klassenname (z. B. "StaticMeshEditorSubsystem") oder None."""
    return _editor_subsystem(klassenname)


def asset_subsystem():
    """EditorAssetSubsystem (UE 5.0+); Rueckfall: unreal.EditorAssetLibrary (gleiche Funktionsnamen, statisch)."""
    subsystem = _editor_subsystem("EditorAssetSubsystem")
    if subsystem is not None:
        return subsystem
    bibliothek = getattr(unreal, "EditorAssetLibrary", None)
    if bibliothek is None:
        abbrechen("Weder EditorAssetSubsystem noch EditorAssetLibrary verfuegbar - laeuft das Skript im Unreal "
                  "Editor (UnrealEditor-Cmd.exe -run=pythonscript)?")
    return bibliothek


def asset_tools():
    try:
        werkzeuge = unreal.AssetToolsHelpers.get_asset_tools()
    except Exception as ausnahme:
        abbrechen("AssetTools nicht verfuegbar: %s" % ausnahme)
    if werkzeuge is None:
        abbrechen("AssetTools nicht verfuegbar.")
    return werkzeuge


def level_subsystem():
    """LevelEditorSubsystem (UE 5.0+) oder None."""
    return _editor_subsystem("LevelEditorSubsystem")


def actor_subsystem():
    subsystem = _editor_subsystem("EditorActorSubsystem")
    if subsystem is None:
        abbrechen("EditorActorSubsystem nicht verfuegbar (Unreal Editor 5.0+ noetig).")
    return subsystem


def editor_welt(pflicht=True):
    """Die Welt des Editors (im Commandlet die zuletzt geladene/neu angelegte Map)."""
    welt = None
    subsystem = _editor_subsystem("UnrealEditorSubsystem")
    if subsystem is not None:
        try:
            welt = subsystem.get_editor_world()
        except Exception as ausnahme:
            hinweis("UnrealEditorSubsystem.get_editor_world(): %s" % ausnahme)
    if welt is None:
        bibliothek = getattr(unreal, "EditorLevelLibrary", None)
        if bibliothek is not None:
            try:
                welt = bibliothek.get_editor_world()
            except Exception:
                welt = None
    if welt is None and pflicht:
        abbrechen("Keine Editor-Welt verfuegbar.")
    return welt


# ---------------------------------------------------------------------------------------------------------------------
# Assets: pruefen, anlegen, ueberschreiben, speichern
# ---------------------------------------------------------------------------------------------------------------------

def ordner_anlegen(ordner):
    """Legt einen Content-Ordner an (falls noetig). Gibt True zurueck, wenn er neu angelegt wurde."""
    ordner = spielpfad(ordner, "Ordner")
    subsystem = asset_subsystem()
    try:
        if subsystem.does_directory_exist(ordner):
            return False
    except Exception:
        pass
    if not subsystem.make_directory(ordner):
        abbrechen("Ordner konnte nicht angelegt werden: %s" % ordner)
    info("Ordner angelegt: %s" % ordner)
    return True


def asset_existiert(pfad):
    return bool(asset_subsystem().does_asset_exist(paket_pfad(pfad)))


def asset_laden(pfad, pflicht=True):
    pfad = paket_pfad(pfad)
    objekt = None
    if asset_existiert(pfad):
        objekt = asset_subsystem().load_asset(pfad)
    if objekt is None and pflicht:
        abbrechen("Asset nicht gefunden bzw. nicht ladbar: %s" % pfad)
    return objekt


def klassenname(objekt):
    if objekt is None:
        return "None"
    try:
        return objekt.get_class().get_name()
    except Exception:
        return type(objekt).__name__


def asset_loeschen(pfad, grund=""):
    pfad = paket_pfad(pfad)
    if not asset_existiert(pfad):
        return False
    if not asset_subsystem().delete_asset(pfad):
        abbrechen("Asset konnte nicht geloescht werden: %s%s" % (pfad, (" (" + grund + ")") if grund else ""))
    hinweis("Asset geloescht: %s%s" % (pfad, (" - " + grund) if grund else ""))
    return True


def vorhandenes_asset(pfad, klasse):
    """Sicheres Ueberschreiben, Schritt 1.

    - kein Asset unter dem Pfad          -> None (der Aufrufer legt neu an)
    - Asset passender Klasse (klasse)   -> das geladene Asset (der Aufrufer aktualisiert es an Ort und Stelle,
                                           damit Verweise anderer Assets gueltig bleiben)
    - Asset anderer Klasse              -> wird mit Hinweis geloescht -> None
    Ein nicht ladbares Asset (z. B. aus einer neueren Engine-Version) fuehrt zum Abbruch statt zum Loeschen."""
    pfad = paket_pfad(pfad)
    if not asset_existiert(pfad):
        return None
    objekt = asset_subsystem().load_asset(pfad)
    if objekt is None:
        abbrechen("Asset %s existiert, laesst sich aber nicht laden - bitte im Editor pruefen bzw. die Datei unter "
                  "Content/ loeschen und neu erzeugen." % pfad)
    if isinstance(objekt, klasse):
        return objekt
    asset_loeschen(pfad, "falscher Typ %s statt %s, wird neu erzeugt" % (klassenname(objekt), klasse.__name__))
    return None


def neues_asset(name, ordner, klasse, fabrik=None):
    """Legt ein neues Asset mit AssetTools.create_asset an (Ordner wird bei Bedarf angelegt)."""
    ordner = spielpfad(ordner, "Ordner")
    ordner_anlegen(ordner)
    pfad = asset_pfad(ordner, name)
    if asset_existiert(pfad):
        abbrechen("Asset existiert schon (vorher vorhandenes_asset() aufrufen): %s" % pfad)
    try:
        objekt = asset_tools().create_asset(name, ordner, klasse, fabrik)
    except Exception as ausnahme:
        abbrechen("Asset %s (%s) konnte nicht angelegt werden: %s" % (pfad, klasse.__name__, ausnahme))
    if objekt is None:
        abbrechen("Asset %s (%s) konnte nicht angelegt werden (create_asset lieferte None)." % (pfad, klasse.__name__))
    info("Asset angelegt: %s (%s)" % (pfad, klasse.__name__))
    return objekt


def umbenennen(alt, neu):
    """Benennt ein Asset um und raeumt den dabei entstehenden Redirector auf (soweit moeglich)."""
    alt, neu = paket_pfad(alt), paket_pfad(neu)
    if alt == neu:
        return True
    if asset_existiert(neu):
        abbrechen("Umbenennen %s -> %s nicht moeglich: Ziel existiert schon." % (alt, neu))
    ordner_anlegen(ordner_von(neu))
    if not asset_subsystem().rename_asset(alt, neu):
        abbrechen("Umbenennen fehlgeschlagen: %s -> %s" % (alt, neu))
    info("Umbenannt: %s -> %s" % (alt, neu))
    redirector_aufraeumen(alt)
    return True


def redirector_aufraeumen(pfad):
    """Entfernt einen Redirector unter pfad (nach Umbenennen), damit Content/ sauber bleibt."""
    pfad = paket_pfad(pfad)
    if not asset_existiert(pfad):
        return
    redirector = None
    try:
        redirector = unreal.find_object(None, objekt_pfad(pfad), follow_redirectors=False)
    except TypeError:
        try:
            redirector = unreal.find_object(None, objekt_pfad(pfad))
        except Exception:
            redirector = None
    except Exception:
        redirector = None
    if redirector is not None and klassenname(redirector) == "ObjectRedirector":
        try:
            asset_tools().fixup_referencers([redirector])
        except Exception as ausnahme:
            hinweis("Redirector %s: fixup_referencers fehlgeschlagen (%s)" % (pfad, ausnahme))
    if asset_existiert(pfad):
        try:
            if asset_subsystem().delete_asset(pfad):
                info("Redirector entfernt: %s" % pfad)
                return
        except Exception as ausnahme:
            hinweis("Redirector %s nicht loeschbar (%s)" % (pfad, ausnahme))
        hinweis("Redirector %s bleibt liegen - harmlos; im Editor: Ordner rechts anklicken -> 'Fix Up Redirectors'."
                % pfad)


def speichern(asset_oder_pfad):
    """Speichert ein geladenes Asset oder ein Asset per Pfad - immer, auch wenn es unveraendert ist."""
    subsystem = asset_subsystem()
    if isinstance(asset_oder_pfad, str):
        pfad = paket_pfad(asset_oder_pfad)
        ok = subsystem.save_asset(pfad, False)
    else:
        pfad = paket_pfad(asset_oder_pfad.get_path_name())
        ok = subsystem.save_loaded_asset(asset_oder_pfad, False)
    if not ok:
        abbrechen("Speichern fehlgeschlagen: %s (Datei schreibgeschuetzt oder im Editor gesperrt?)" % pfad)
    return pfad


def ordner_speichern(ordner, rekursiv=False):
    """Speichert alle geaenderten Assets eines Ordners (z. B. vom FBX-Import angelegte Materialien)."""
    ordner = spielpfad(ordner, "Ordner")
    try:
        ok = asset_subsystem().save_directory(ordner, True, rekursiv)
    except Exception as ausnahme:
        hinweis("save_directory(%s): %s" % (ordner, ausnahme))
        return False
    if not ok:
        hinweis("Nicht alle Assets in %s konnten gespeichert werden." % ordner)
    return bool(ok)


def assets_im_ordner(ordner, rekursiv=False):
    """Paketpfade aller Assets in einem Ordner (ohne Unterordner-Eintraege); "/Game" = ganzer Content."""
    ordner = spielpfad(ordner, "Ordner", wurzel_erlaubt=True)
    try:
        if not asset_subsystem().does_directory_exist(ordner):
            return []
    except Exception:
        pass
    try:
        liste = asset_subsystem().list_assets(ordner, rekursiv, False)
    except Exception as ausnahme:
        hinweis("list_assets(%s): %s" % (ordner, ausnahme))
        return []
    return sorted(set(paket_pfad(str(p)) for p in liste))


# ---------------------------------------------------------------------------------------------------------------------
# Eigenschaften
# ---------------------------------------------------------------------------------------------------------------------

def beschreibung(objekt):
    try:
        return "%s %s" % (klassenname(objekt), objekt.get_name())
    except Exception:
        return klassenname(objekt)


def setze(objekt, name, wert, pflicht=True, was=None):
    """set_editor_property mit klarer Meldung. pflicht=False: nur Hinweis, wenn es die Eigenschaft
    in dieser Engine-Version nicht gibt. Gibt True zurueck, wenn der Wert gesetzt wurde."""
    try:
        objekt.set_editor_property(name, wert)
        return True
    except Exception as ausnahme:
        text = "%s: Eigenschaft '%s' nicht setzbar (%s)" % (was or beschreibung(objekt), name, ausnahme)
        if pflicht:
            abbrechen(text)
        hinweis(text)
        return False


def setze_alle(objekt, werte, pflicht=True, was=None):
    """Mehrere Eigenschaften setzen; Rueckgabe: Liste der nicht gesetzten Namen."""
    fehlend = []
    for name, wert in werte:
        if not setze(objekt, name, wert, pflicht=pflicht, was=was):
            fehlend.append(name)
    return fehlend


def lies(objekt, name, standard=None):
    try:
        return objekt.get_editor_property(name)
    except Exception:
        return standard


def setze_pfad(objekt, punktpfad, wert, was=None):
    """Setzt eine verschachtelte Eigenschaft ('mesh_pipeline.import_static_meshes') - immer optional
    (fuer Einstellungen, deren Namen sich zwischen Engine-Versionen aendern, z. B. Interchange)."""
    teile = punktpfad.split(".")
    ziel = objekt
    for teil in teile[:-1]:
        naechstes = lies(ziel, teil)
        if naechstes is None:
            naechstes = getattr(ziel, teil, None)
        if naechstes is None:
            hinweis("%s: '%s' gibt es in dieser Engine-Version nicht (bei '%s')" % (
                was or beschreibung(objekt), teil, punktpfad))
            return False
        ziel = naechstes
    return setze(ziel, teile[-1], wert, pflicht=False, was="%s.%s" % (was or beschreibung(objekt),
                                                                      ".".join(teile[:-1]) or "-"))


def enum_wert(enum_name, wert_name, pflicht=True):
    """unreal.<enum_name>.<wert_name> mit klarer Meldung, falls es den Wert nicht gibt."""
    enum = getattr(unreal, enum_name, None)
    wert = getattr(enum, wert_name, None) if enum is not None else None
    if wert is None and pflicht:
        abbrechen("unreal.%s.%s gibt es in dieser Engine-Version nicht." % (enum_name, wert_name))
    return wert


# ---------------------------------------------------------------------------------------------------------------------
# Konsole und Engine-Version
# ---------------------------------------------------------------------------------------------------------------------

def konsolenbefehl(befehl):
    try:
        unreal.SystemLibrary.execute_console_command(editor_welt(pflicht=False), befehl)
        return True
    except Exception as ausnahme:
        hinweis("Konsolenbefehl '%s' fehlgeschlagen: %s" % (befehl, ausnahme))
        return False


def cvar_bool(name):
    """Wert einer Konsolenvariable als bool; None, wenn er sich nicht lesen laesst.
    Achtung: Eine fehlende Variable liest Unreal als False (mit Warnung im Log)."""
    try:
        return bool(unreal.SystemLibrary.get_console_variable_bool_value(name))
    except Exception:
        pass
    try:
        return bool(unreal.SystemLibrary.get_console_variable_int_value(name))
    except Exception:
        return None


def engine_version():
    """(major, minor, Text), z. B. (5, 8, '5.8.0-...')"""
    try:
        text = str(unreal.SystemLibrary.get_engine_version())
    except Exception:
        return (0, 0, "unbekannt")
    treffer = re.match(r"\s*(\d+)\.(\d+)", text)
    if not treffer:
        return (0, 0, text)
    return (int(treffer.group(1)), int(treffer.group(2)), text)


def umgebung_melden():
    major, minor, text = engine_version()
    info("Unreal Engine %s, Python %s" % (text, sys.version.split()[0]))
    info("Projekt: %s" % pfade().uproject)
    if (major, minor) == (0, 0):
        hinweis("Engine-Version unbekannt.")
    elif not (UNTERSTUETZTE_VERSIONEN[0] <= (major, minor) <= UNTERSTUETZTE_VERSIONEN[1]):
        hinweis("Die Skripte sind fuer Unreal Engine %d.%d bis %d.%d geschrieben, geladen ist %d.%d."
                % (UNTERSTUETZTE_VERSIONEN[0] + UNTERSTUETZTE_VERSIONEN[1] + (major, minor)))
    return major, minor


def modul_geladen():
    """True, wenn die C++-Typen von MatchballTennis in Python sichtbar sind (Editor-Ziel kompiliert)."""
    if getattr(unreal, "MBBelagZeile", None) is not None:
        return True
    try:
        return unreal.find_object(None, "/Script/%s" % MODUL_NAME) is not None
    except Exception:
        return False


# ---------------------------------------------------------------------------------------------------------------------
# Argumente und Ausfuehrung
# ---------------------------------------------------------------------------------------------------------------------

def parser(beschreibung_text):
    """argparse ohne -h und ohne sys.exit (beides ist im Unreal-Commandlet ungeeignet)."""
    try:
        return argparse.ArgumentParser(description=beschreibung_text, add_help=False, exit_on_error=False)
    except TypeError:  # aeltere Python-Versionen
        return argparse.ArgumentParser(description=beschreibung_text, add_help=False)


def argumente_lesen(argparser, argv=None):
    """Liest die Skript-Argumente (Unreal legt sie nach sys.argv[1:]). Unbekannte -> Hinweis."""
    if argv is None:
        argv = sys.argv[1:]
    argv = [str(a) for a in argv if str(a).strip()]
    try:
        argumente, unbekannt = argparser.parse_known_args(argv)
    except (argparse.ArgumentError, SystemExit) as ausnahme:
        abbrechen("Ungueltige Argumente %s: %s\n%s" % (argv, ausnahme, argparser.format_usage()))
    if unbekannt:
        hinweis("Unbekannte Argumente werden ignoriert: %s" % " ".join(unbekannt))
    return argumente


def liste_aus_text(text):
    """'a, b;c' -> ['a', 'b', 'c'] (klein geschrieben)."""
    if not text:
        return []
    return [t.strip().lower() for t in re.split(r"[,;\s]+", text) if t.strip()]


def als_skript_ausfuehren(funktion, titel):
    """Rahmen fuer 'if __name__ == "__main__"': Kopf, Dauer, klare Fehlermeldung, Exception nach aussen."""
    start = time.time()
    schritt("Matchball Tennis - %s (%s)" % (titel, datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")))
    try:
        ergebnis = funktion()
    except MBFehler as ausnahme:
        fehler("%s abgebrochen nach %s: %s" % (titel, dauer_text(time.time() - start), ausnahme))
        raise
    except Exception as ausnahme:
        # Den vollstaendigen Traceback gibt Unreal selbst aus ("LogPython: Error: Traceback ...")
        fehler("%s abgebrochen nach %s - unerwarteter Fehler: %s" % (
            titel, dauer_text(time.time() - start), ausnahme_text(ausnahme)))
        raise
    info("%s fertig nach %s." % (titel, dauer_text(time.time() - start)))
    return ergebnis
