# -*- coding: utf-8 -*-
"""
import_alles.py - kompletter Unreal-Import in einem Lauf (README 0.2, 3.2, 19.4.1).
Wird von Tools/assets.ps1 aufgerufen:

    UnrealEditor-Cmd.exe <Repo>\\Unreal\\MatchballTennis.uproject -run=pythonscript
        -script="<Repo>/Unreal/Tools/Python/import_alles.py" -unattended -nosplash -nullrhi

Optionen im -script-Wert, z. B. -script="<...>/import_alles.py --neu":
    --neu           Map M_Graubox auch dann neu bauen, wenn sie schon existiert
    --nur <liste>   nur diese Schritte: daten, assets, map, chunks (z. B. --nur daten,chunks)
    --interchange   FBX ueber Interchange importieren (siehe import_assets.py)
    --ev100 <wert>  feste Belichtung der Map (siehe build_graubox.py)

Schritte in dieser Reihenfolge:
    1. daten   import_daten.py    Data Tables /Game/Data/DT_* aus Unreal/Data/*.csv
    2. assets  import_assets.py   Blender-Assets aus Unreal/SourceAssets/_erzeugt/**/*.meta.json
    3. map     build_graubox.py   /Game/Maps/M_Graubox - nur wenn sie fehlt oder mit --neu
    4. chunks  setze_chunks.py    PrimaryAssetLabels laut Unreal/Data/chunks.json
Schlaegt ein Schritt fehl, laufen die uebrigen trotzdem (damit alle Probleme auf einmal sichtbar werden);
am Ende steht eine Gesamtuebersicht im Log und das Skript wirft MBFehler -> Exit-Code != 0.
"""

import importlib
import os
import sys
import time


def _skriptordner():
    try:
        return os.path.dirname(os.path.abspath(__file__))
    except NameError:
        import unreal
        return os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Tools", "Python")


if _skriptordner() not in sys.path:
    sys.path.insert(0, _skriptordner())

import unreal  # noqa: E402

# Im laufenden Editor bleiben Module zwischen zwei Aufrufen geladen - geaenderte Skripte neu laden.
for _name in ("mb_unreal", "import_daten", "import_assets", "build_graubox", "setze_chunks"):
    if _name in sys.modules:
        importlib.reload(sys.modules[_name])

import mb_unreal as mb  # noqa: E402
import import_daten  # noqa: E402
import import_assets  # noqa: E402
import build_graubox  # noqa: E402
import setze_chunks  # noqa: E402

SCHRITTE = ("daten", "assets", "map", "chunks")


def _bilanz_text(bilanz):
    if isinstance(bilanz, mb.Bilanz):
        return "%d OK, %d mit Hinweis, %d uebersprungen" % (
            bilanz.anzahl(mb.Bilanz.OK), bilanz.anzahl(mb.Bilanz.HINWEIS), bilanz.anzahl(mb.Bilanz.UEBERSPRUNGEN))
    return str(bilanz)


def _map_schritt(neu, ev100):
    if build_graubox.map_vorhanden() and not neu:
        mb.info("%s existiert schon - wird nicht neu gebaut (dafuer: --neu)." % build_graubox.MAP_PFAD)
        return "vorhanden, unveraendert (--neu baut neu)"
    return build_graubox.ausfuehren(neu=True, ev100=ev100)


def _offene_pakete_speichern():
    """Sicherheitsnetz: noch ungespeicherte Content-Pakete (z. B. vom Importer angelegte Materialien) speichern.
    Maps nicht - die speichert build_graubox.py selbst; eine unbenannte Map wuerde nach einem Dateinamen fragen."""
    try:
        ok = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(False, True)
        if not ok:
            mb.hinweis("Nicht alle geaenderten Pakete konnten gespeichert werden.")
    except Exception as ausnahme:
        mb.hinweis("save_dirty_packages: %s" % ausnahme)


def ausfuehren(neu=False, nur=None, interchange=False, ev100=None):
    nur = [n.lower() for n in (nur or [])]
    unbekannt = [n for n in nur if n not in SCHRITTE]
    if unbekannt:
        mb.abbrechen("Unbekannte Schritte in --nur: %s (moeglich: %s)" % (", ".join(unbekannt), ", ".join(SCHRITTE)))
    if ev100 is None:
        ev100 = build_graubox.BELICHTUNG_EV100

    mb.umgebung_melden()
    if not mb.modul_geladen():
        mb.hinweis("Die C++-Typen von %s sind nicht sichtbar - Data Tables werden scheitern. Zuerst "
                   "Tools\\build.ps1 ausfuehren (Editor-Ziel kompilieren)." % mb.MODUL_NAME)

    plan = (
        ("daten", "Data Tables", lambda: import_daten.ausfuehren()),
        ("assets", "Assets", lambda: import_assets.ausfuehren(interchange=interchange)),
        ("map", "Map M_Graubox", lambda: _map_schritt(neu, ev100)),
        ("chunks", "Chunk-Labels", lambda: setze_chunks.ausfuehren()),
    )
    gesamt = mb.Bilanz("Import gesamt")
    for kurz, titel, funktion in plan:
        if nur and kurz not in nur:
            gesamt.uebersprungen(titel, "nicht in --nur")
            continue
        mb.schritt("Schritt %s: %s" % (kurz, titel))
        start = time.time()
        try:
            ergebnis = funktion()
            gesamt.ok(titel, "%s (%s)" % (_bilanz_text(ergebnis), mb.dauer_text(time.time() - start)))
        except Exception as ausnahme:
            if not isinstance(ausnahme, mb.MBFehler):
                mb.fehler("%s: unerwarteter Fehler: %s" % (titel, mb.ausnahme_text(ausnahme)))
            gesamt.fehlgeschlagen(titel, "%s (%s)" % (mb.ausnahme_text(ausnahme), mb.dauer_text(time.time() - start)))

    _offene_pakete_speichern()
    mb.schritt("Gesamtergebnis")
    gesamt.ausgeben()
    if gesamt.anzahl_fehler:
        raise mb.MBFehler("Import: %d von %d Schritten fehlgeschlagen - Details oben im Log (Zeilen mit [MB])."
                          % (gesamt.anzahl_fehler, len(gesamt.eintraege)))
    mb.info("Import vollstaendig: alle Schritte OK.")
    return gesamt


def main(argv=None):
    p = mb.parser("Kompletter Unreal-Import: Data Tables, Assets, Map, Chunk-Labels")
    p.add_argument("--neu", action="store_true", help="Map M_Graubox neu bauen, auch wenn sie existiert")
    p.add_argument("--nur", default="", help="nur diese Schritte: daten, assets, map, chunks")
    p.add_argument("--interchange", action="store_true", help="FBX ueber Interchange importieren")
    p.add_argument("--ev100", type=float, default=None, help="feste Belichtung der Map (EV100)")
    argumente = mb.argumente_lesen(p, argv)
    return ausfuehren(neu=argumente.neu, nur=mb.liste_aus_text(argumente.nur), interchange=argumente.interchange,
                      ev100=argumente.ev100)


if __name__ == "__main__":
    mb.als_skript_ausfuehren(main, "Unreal-Import (import_alles.py)")
