# -*- coding: utf-8 -*-
"""
setze_chunks.py - legt die Primary Asset Labels fuer die feste Paket-Aufteilung an (README 19.4.1).

Aufruf (lokal):
    UnrealEditor-Cmd.exe <Repo>\\Unreal\\MatchballTennis.uproject -run=pythonscript
        -script="<Repo>/Unreal/Tools/Python/setze_chunks.py" -unattended -nosplash -nullrhi
    Option im -script-Wert:  --datei <anderes chunks.json>
Normalerweise ruft Tools/assets.ps1 -> import_alles.py dieses Skript als letzten Schritt auf.

Quelle: Unreal/Data/chunks.json
    {"chunks": [{"id": 0, "name": "Kern", "beschreibung": "...",
                 "ordner": ["/Game/Data", "/Game/UI", "/Game/Maps"]}, ...]}

Je Ordner entsteht <Ordner>/PAL_Chunk<ID> (unreal.PrimaryAssetLabel) mit
    Rules.chunk_id = ID, Rules.cook_rule = ALWAYS_COOK, label_assets_in_my_directory = True
(markiert alle Assets im Ordner und in seinen Unterordnern). Fehlende Ordner werden angelegt.
Beim Packen (bGenerateChunks=True in Config/DefaultGame.ini) entsteht daraus je Chunk eine eigene
pakchunk<ID>-Windows.pak/.utoc/.ucas. Die Asset-Manager-Grundeinstellung von Unreal (BaseGame.ini) sucht
PrimaryAssetLabels unter /Game - dafuer ist keine weitere Einstellung noetig.

Regeln (README 19.4.1): Chunk-IDs aendern sich NIE. Das Skript prueft chunks.json deshalb streng
(eindeutige IDs >= 0, jeder Ordner nur in einem Chunk, gueltige /Game-Pfade) und meldet es deutlich, wenn ein
Ordner vorher ein Label mit einer anderen ID hatte. Veraltete Labels PAL_Chunk<n> (Ordner nicht mehr in
chunks.json oder andere ID) werden geloescht - chunks.json ist die einzige Quelle der Wahrheit.
"""

import os
import re
import sys


def _skriptordner():
    try:
        return os.path.dirname(os.path.abspath(__file__))
    except NameError:
        import unreal
        return os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Tools", "Python")


if _skriptordner() not in sys.path:
    sys.path.insert(0, _skriptordner())

import unreal  # noqa: E402

import mb_unreal as mb  # noqa: E402

LABEL_PRAEFIX = "PAL_Chunk"
_LABEL_NAME = re.compile(r"^%s(\d+)$" % LABEL_PRAEFIX)
_SPIELPFAD = re.compile(r"^/Game(/[A-Za-z0-9_\-]+)+$")


# =====================================================================================================================
# chunks.json lesen und pruefen (reines Python - offline testbar)
# =====================================================================================================================

class ChunkFehler(ValueError):
    """chunks.json ist ungueltig."""


class Chunk(object):
    def __init__(self, nummer, name, ordner, beschreibung=""):
        self.id = nummer
        self.name = name
        self.ordner = ordner
        self.beschreibung = beschreibung

    def label_pfad(self, ordner):
        return "%s/%s%d" % (ordner, LABEL_PRAEFIX, self.id)

    def __repr__(self):
        return "Chunk(%d, %s, %s)" % (self.id, self.name, self.ordner)


def chunks_pruefen(daten, quelle="chunks.json"):
    """Prueft den Inhalt von chunks.json und liefert eine Liste Chunk (nach ID sortiert)."""
    if not isinstance(daten, dict) or not isinstance(daten.get("chunks"), list):
        raise ChunkFehler("%s: erwartet {\"chunks\": [ ... ]}" % quelle)
    if not daten["chunks"]:
        raise ChunkFehler("%s: keine Chunks eingetragen" % quelle)
    ergebnis = []
    ids = {}
    ordner_zu_chunk = {}
    for nummer, eintrag in enumerate(daten["chunks"], start=1):
        wo = "%s, Eintrag %d" % (quelle, nummer)
        if not isinstance(eintrag, dict):
            raise ChunkFehler("%s: kein Objekt" % wo)
        roh_id = eintrag.get("id")
        if isinstance(roh_id, bool) or not isinstance(roh_id, int) or roh_id < 0:
            raise ChunkFehler("%s: 'id' muss eine ganze Zahl >= 0 sein, ist %r" % (wo, roh_id))
        if roh_id in ids:
            raise ChunkFehler("%s: Chunk-ID %d kommt doppelt vor (auch bei '%s')" % (wo, roh_id, ids[roh_id]))
        name = eintrag.get("name")
        if not isinstance(name, str) or not name.strip():
            raise ChunkFehler("%s (ID %d): 'name' fehlt" % (wo, roh_id))
        ordner_liste = eintrag.get("ordner")
        if not isinstance(ordner_liste, list) or not ordner_liste:
            raise ChunkFehler("%s (ID %d, %s): 'ordner' muss eine nicht leere Liste sein" % (wo, roh_id, name))
        ordner_sauber = []
        for ordner in ordner_liste:
            if not isinstance(ordner, str):
                raise ChunkFehler("%s (ID %d): Ordner ist kein Text: %r" % (wo, roh_id, ordner))
            ordner = ordner.strip().rstrip("/")
            if not _SPIELPFAD.match(ordner):
                raise ChunkFehler("%s (ID %d): Ordner '%s' ungueltig (erwartet /Game/..., nur A-Z, a-z, 0-9, _, -)"
                                  % (wo, roh_id, ordner))
            schluessel = ordner.lower()
            if schluessel in ordner_zu_chunk:
                raise ChunkFehler("%s: Ordner %s steht in Chunk %d und Chunk %d - jeder Ordner gehoert zu genau "
                                  "einem Chunk" % (wo, ordner, ordner_zu_chunk[schluessel], roh_id))
            ordner_zu_chunk[schluessel] = roh_id
            ordner_sauber.append(ordner)
        beschreibung = eintrag.get("beschreibung", "")
        ids[roh_id] = name.strip()
        ergebnis.append(Chunk(roh_id, name.strip(), ordner_sauber, str(beschreibung or "")))
    return sorted(ergebnis, key=lambda c: c.id)


def verschachtelte_ordner(chunks):
    """Paare (aussen, innen) von Ordnern verschiedener Chunks, bei denen einer im anderen liegt.
    Unreal erlaubt das (Prioritaeten), die feste Aufteilung soll es aber vermeiden -> nur Hinweis."""
    alle = [(o, c.id) for c in chunks for o in c.ordner]
    paare = []
    for aussen, id_aussen in alle:
        for innen, id_innen in alle:
            if id_aussen != id_innen and innen.lower().startswith(aussen.lower() + "/"):
                paare.append((aussen, id_aussen, innen, id_innen))
    return paare


def chunks_lesen(pfad):
    """chunks.json lesen (UTF-8) und pruefen."""
    import json
    try:
        with open(str(pfad), "r", encoding="utf-8-sig") as datei:
            daten = json.load(datei)
    except (OSError, ValueError) as ausnahme:
        raise ChunkFehler("%s nicht lesbar bzw. kein gueltiges JSON (%s)" % (pfad, ausnahme))
    return chunks_pruefen(daten, os.path.basename(str(pfad)))


# =====================================================================================================================
# Labels in Unreal anlegen
# =====================================================================================================================

def _label_anlegen(name, ordner):
    fabrik = None
    fabrik_klasse = getattr(unreal, "DataAssetFactory", None)
    if fabrik_klasse is not None:
        fabrik = fabrik_klasse()
        if not mb.setze(fabrik, "data_asset_class", unreal.PrimaryAssetLabel, pflicht=False, was="DataAssetFactory"):
            fabrik = None
    return mb.neues_asset(name, ordner, unreal.PrimaryAssetLabel, fabrik)


def _label_einstellen(label, chunk):
    was = "%s" % label.get_name()
    regeln = mb.lies(label, "rules")
    if regeln is None:
        mb.abbrechen("%s: Eigenschaft 'rules' nicht lesbar." % was)
    mb.setze(regeln, "chunk_id", int(chunk.id), was=was + ".rules")
    mb.setze(regeln, "cook_rule", mb.enum_wert("PrimaryAssetCookRule", "ALWAYS_COOK"), was=was + ".rules")
    mb.setze(regeln, "apply_recursively", True, pflicht=False, was=was + ".rules")
    mb.setze(label, "rules", regeln, was=was)
    mb.setze(label, "label_assets_in_my_directory", True, was=was)
    mb.setze(label, "is_runtime_label", False, pflicht=False, was=was)


def _label_pruefen(pfad, chunk):
    label = mb.asset_laden(pfad)
    regeln = mb.lies(label, "rules")
    chunk_id = mb.lies(regeln, "chunk_id") if regeln is not None else None
    kochen = mb.lies(regeln, "cook_rule") if regeln is not None else None
    ordner_flag = mb.lies(label, "label_assets_in_my_directory")
    if chunk_id != chunk.id or not ordner_flag:
        mb.abbrechen("%s: nach dem Speichern chunk_id=%s, label_assets_in_my_directory=%s (erwartet %d, True)"
                     % (pfad, chunk_id, ordner_flag, chunk.id))
    soll = mb.enum_wert("PrimaryAssetCookRule", "ALWAYS_COOK")
    if kochen != soll:
        mb.abbrechen("%s: cook_rule=%s statt ALWAYS_COOK" % (pfad, kochen))


def _vorhandene_labels():
    """{Paketpfad: ID} aller PAL_Chunk<n> unter /Game."""
    ergebnis = {}
    for pfad in mb.assets_im_ordner("/Game", rekursiv=True):
        treffer = _LABEL_NAME.match(mb.name_von(pfad))
        if treffer:
            ergebnis[pfad] = int(treffer.group(1))
    return ergebnis


def ausfuehren(datei=None):
    """Legt alle Labels laut chunks.json an. Rueckgabe: mb.Bilanz. Wirft MBFehler bei Fehlern."""
    datei = datei or mb.pfade().chunks_json
    if not os.path.isfile(str(datei)):
        mb.abbrechen("%s fehlt (README 19.4.1)." % datei)
    try:
        chunks = chunks_lesen(datei)
    except ChunkFehler as ausnahme:
        mb.abbrechen(str(ausnahme))
    for aussen, id_aussen, innen, id_innen in verschachtelte_ordner(chunks):
        mb.hinweis("Ordner %s (Chunk %d) liegt in %s (Chunk %d) - besser vermeiden." % (
            innen, id_innen, aussen, id_aussen))
    mb.info("%s: %d Chunks, %d Ordner" % (mb.relativ(datei), len(chunks), sum(len(c.ordner) for c in chunks)))

    bilanz = mb.Bilanz("Chunks (%s)" % os.path.basename(str(datei)))
    vorher = _vorhandene_labels()
    soll = {}
    for chunk in chunks:
        for ordner in chunk.ordner:
            soll[chunk.label_pfad(ordner)] = chunk

    # 1. Veraltete Labels entfernen (anderer Ordner oder andere ID)
    for pfad, alte_id in sorted(vorher.items()):
        if pfad in soll:
            continue
        ordner = mb.ordner_von(pfad)
        neue = [c for c in chunks if ordner in c.ordner]
        if neue:
            mb.hinweis("Ordner %s wechselt von Chunk %d zu Chunk %d. README 19.4.1: Chunk-IDs aendern sich nie - "
                       "bitte pruefen, ob das gewollt ist (Spieler laden das Paket dann neu)."
                       % (ordner, alte_id, neue[0].id))
        try:
            mb.asset_loeschen(pfad, "veraltetes Chunk-Label")
            bilanz.mit_hinweis(mb.name_von(pfad) + " in " + ordner, "veraltet, geloescht")
        except Exception as ausnahme:
            bilanz.fehlgeschlagen(pfad, mb.ausnahme_text(ausnahme))

    # 2. Labels anlegen bzw. aktualisieren
    for chunk in chunks:
        for ordner in chunk.ordner:
            pfad = chunk.label_pfad(ordner)
            name = "Chunk %2d %-18s %s" % (chunk.id, chunk.name[:18], ordner)
            try:
                mb.ordner_anlegen(ordner)
                label = mb.vorhandenes_asset(pfad, unreal.PrimaryAssetLabel)
                aktion = "aktualisiert"
                if label is None:
                    label = _label_anlegen(mb.name_von(pfad), ordner)
                    aktion = "neu"
                _label_einstellen(label, chunk)
                mb.speichern(label)
                _label_pruefen(pfad, chunk)
                bilanz.ok(name, aktion)
            except Exception as ausnahme:
                if not isinstance(ausnahme, mb.MBFehler):
                    mb.fehler("%s: %s" % (pfad, mb.ausnahme_text(ausnahme)))
                bilanz.fehlgeschlagen(name, mb.ausnahme_text(ausnahme))

    bilanz.ausgeben()
    if bilanz.anzahl_fehler:
        raise mb.MBFehler("Chunks: %d Fehler (siehe Tabelle oben)." % bilanz.anzahl_fehler)
    return bilanz


def main(argv=None):
    p = mb.parser("Primary Asset Labels fuer die Chunks aus Data/chunks.json anlegen")
    p.add_argument("--datei", default="", help="anderes chunks.json (Standard Unreal/Data/chunks.json)")
    argumente = mb.argumente_lesen(p, argv)
    mb.umgebung_melden()
    datei = argumente.datei or None
    if datei and not os.path.isabs(datei):
        datei = str(mb.pfade().repo / datei)
    return ausfuehren(datei)


if __name__ == "__main__":
    mb.als_skript_ausfuehren(main, "Chunk-Labels setzen (setze_chunks.py)")
