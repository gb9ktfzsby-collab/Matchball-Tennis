# -*- coding: utf-8 -*-
"""
import_assets.py - importiert die per Blender-Skript erzeugten Assets nach Unreal (README 0.2, 3.2, 3.4).

Aufruf (lokal):
    UnrealEditor-Cmd.exe <Repo>\\Unreal\\MatchballTennis.uproject -run=pythonscript
        -script="<Repo>/Unreal/Tools/Python/import_assets.py" -unattended -nosplash -nullrhi
    Optionen im -script-Wert:  --nur SM_Ball,SK_WuerfelSpieler   --interchange   --wurzel <Ordner>
Normalerweise ruft Tools/assets.ps1 -> import_alles.py dieses Skript auf.

Quelle: jede Datei Unreal/SourceAssets/_erzeugt/**/<Name>.meta.json (geschrieben von
Unreal/blender/lib/mb_gemeinsam.py: meta_schreiben). Gelesen werden beide Schreibweisen:

    Blender-Pipeline (aktuell)                     Kurzform
    "asset": "SM_Ball"                             "name": "SM_Ball"
    "typ": "static" | "skeletal" | "animation"     (gleich; zusaetzlich "textur" fuer einzelne Bilder)
    "dateien": {"fbx": "SM_Ball.fbx",              "datei": "SM_Ball.fbx"
                "lods": [{"stufe": 1, "datei": "SM_Ball_LOD1.fbx"}, ...],
                "texturen": [{"datei": "T_x.png", "rolle": "Normal", "srgb": false,
                              "konvention": "OpenGL ..."}]}
    "unreal_ziel": "/Game/Equipment/Ball"          "ziel": "/Game/Equipment/Ball"
    "skelett_ziel": "/Game/.../SKEL_X"             "skelett": "/Game/.../SKEL_X" (oder nur der Name)
    "zweiseitig": true (alle Materialien zweiseitig), "quelle", "lizenz", "datum", "parameter" (nur protokolliert)
    "materialien": [{"name": "M_SchlaegerSaiten", "zweiseitig": true, "blend": "masked"}, ...]
        je Material: "zweiseitig" -> Two Sided; "blend"/"blend_modus": "masked" bzw. "maskiert": true bzw. ein
        Eintrag "alpha" < 1 (so schreibt mb_gemeinsam.material_grundfarbe Alpha-Materialien) -> Blend Mode
        Masked, Opacity Mask = Alphakanal der Grundfarbe-Textur (Saitenbett, Entscheidungen B18).

Was passiert (in dieser Reihenfolge: Texturen, Static Meshes, Skeletal Meshes, Animationen):
  * static    -> Static Mesh <ziel>/<name>, KEINE Kollision (Ball und Schlaeger sind reine Darstellung, die
                 Ballphysik ist eine eigene Simulation), LOD1..n aus den LOD-Dateien, Materialien und Texturen
                 aus der FBX.
  * skeletal  -> Skeletal Mesh <ziel>/<name> mit Skelett SKEL_<Name> (vorhandenes Skelett wird
                 wiederverwendet, ein neu entstandenes "<name>_Skeleton" wird umbenannt), kein Physics Asset.
  * animation -> Anim Sequence <ziel>/<name> auf dem Skelett aus "skelett_ziel"/"skelett" (muss vorher als
                 Skeletal Mesh importiert sein - die Reihenfolge oben sorgt dafuer).
  * texturen  -> jede Textur aus "dateien.texturen" als <ziel>/<Dateiname ohne Endung>, mit passenden
                 Einstellungen: Normal-Map (TC_Normalmap, linear, gruener Kanal gespiegelt bei OpenGL-
                 Konvention), Masken wie MetallicRoughness (TC_Masks, linear), sonst Farbe (sRGB).
Vorhandene Assets werden ersetzt (gleicher Pfad, Verweise bleiben gueltig); ein Asset falschen Typs unter dem
Zielpfad wird geloescht und neu angelegt.

Achsen und Massstab: wie in Unreal/blender/lib/mb_gemeinsam.py (ACHSEN_UND_MASSSTAB) beschrieben enthalten die
FBX-Dateien bereits Zentimeter, Z oben, Front -Y. Import deshalb mit Convert Scene AN, Force Front X Axis AUS,
Convert Scene Unit AUS, Skalierung 1.

FBX-Import und Interchange (Weiche):
  Ab UE 5.5 importiert Unreal FBX standardmaessig ueber Interchange (Konsolenvariable
  Interchange.FeatureFlags.Import.FBX = 1). Das Skript schaltet sie fuer die Dauer des Imports auf 0, damit der
  klassische FBX-Importer mit unreal.FbxImportUI genau die Einstellungen oben nimmt, und stellt sie danach
  wieder her. Laesst sie sich nicht abschalten (oder mit --interchange), laeuft derselbe Import ueber
  Interchange; Unreal uebersetzt die FbxImportUI-Einstellungen dann selbst. Entsteht dabei kein Asset, folgt
  ein zweiter Versuch direkt ueber unreal.InterchangeManager mit einer InterchangeGenericAssetsPipeline.
  Welcher Weg tatsaechlich benutzt wurde, steht je Asset im Log ("Importweg").

Fehler bei einem Asset werden gesammelt; am Ende wirft das Skript MBFehler (Exit-Code != 0).
"""

import json
import os
import re
import sys
from pathlib import Path


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

META_ENDUNG = ".meta.json"
FBX_CVAR = "Interchange.FeatureFlags.Import.FBX"

#: Typ -> Rang (Reihenfolge), Unreal-Klassenname, erwartete Namens-Praefixe (README 3.2)
TYPEN = {
    "textur": (0, "Texture2D", ("T_",)),
    "static": (1, "StaticMesh", ("SM_",)),
    "skeletal": (2, "SkeletalMesh", ("SK_",)),
    "animation": (3, "AnimSequence", ("A_",)),
}
TYP_ALIASE = {
    "static": "static", "staticmesh": "static", "static_mesh": "static", "sm": "static",
    "skeletal": "skeletal", "skeletalmesh": "skeletal", "skeletal_mesh": "skeletal", "sk": "skeletal",
    "animation": "animation", "anim": "animation", "animsequence": "animation",
    "textur": "textur", "texture": "textur", "texture2d": "textur",
}
_NAME = re.compile(r"^[A-Za-z0-9_\-]+$")
_SPIELPFAD = re.compile(r"^/Game(/[A-Za-z0-9_\-]+)+$")


# =====================================================================================================================
# Teil 1: meta.json lesen (reines Python, ohne Unreal - offline testbar)
# =====================================================================================================================

class MetaFehler(ValueError):
    """Ungueltige oder unvollstaendige meta.json."""


_MASKEN_WOERTER = {"metallic", "metall", "roughness", "rough", "rauheit", "occlusion", "ao", "orm", "mask", "maske",
                   "masken", "alpha", "opacity", "specular"}
_FARB_WOERTER = {"base", "basecolor", "color", "colour", "farbe", "albedo", "diffuse", "emissive", "emission"}


def _woerter(text):
    """'MetallicRoughness' -> {'metallic', 'roughness', ...}; 'BaseColor+Alpha' -> {'base', 'color', 'alpha', ...}"""
    text = str(text or "")
    teile = re.findall(r"[A-Z]+(?![a-z])|[A-Z]?[a-z]+|[0-9]+", text)
    return set(t.lower() for t in teile) | {text.lower()}


class Textur(object):
    def __init__(self, datei, rolle="", srgb=None, konvention="", material="", name=None):
        self.datei = Path(datei)
        self.name = name or self.datei.stem
        self.rolle = str(rolle or "")
        self.srgb = srgb
        self.konvention = str(konvention or "")
        self.material = str(material or "")

    @property
    def ist_normal(self):
        return "normal" in _woerter(self.rolle)

    @property
    def ist_maske(self):
        """Lineare Datentextur (MetallicRoughness, AO, ORM, Maske, Alpha ...) - nicht bei Farbe oder Normal-Map."""
        woerter = _woerter(self.rolle)
        if "normal" in woerter or woerter & _FARB_WOERTER:
            return False
        return bool(woerter & _MASKEN_WOERTER)

    @property
    def opengl(self):
        return "opengl" in self.konvention.lower()

    def __repr__(self):
        return "Textur(%s, %s)" % (self.datei.name, self.rolle)


class MaterialRegel(object):
    """Einstellungen eines Materials laut meta.json ('materialien')."""

    def __init__(self, name, zweiseitig=False, maskiert=False):
        self.name = name
        self.zweiseitig = zweiseitig
        self.maskiert = maskiert

    def __repr__(self):
        return "MaterialRegel(%s, zweiseitig=%s, maskiert=%s)" % (self.name, self.zweiseitig, self.maskiert)


def _material_regel(eintrag):
    """Ein Eintrag aus 'materialien' -> MaterialRegel oder None (nichts einzustellen)."""
    if not isinstance(eintrag, dict) or not isinstance(eintrag.get("name"), str) or not eintrag["name"].strip():
        return None
    blend = str(eintrag.get("blend") or eintrag.get("blend_modus") or eintrag.get("blend_mode") or "").strip().lower()
    alpha = eintrag.get("alpha")
    maskiert = (blend in ("masked", "maskiert", "maske") or bool(_bool_oder_none(eintrag.get("maskiert")))
                or (isinstance(alpha, (int, float)) and not isinstance(alpha, bool) and alpha < 1.0))
    zweiseitig = bool(_bool_oder_none(eintrag.get("zweiseitig")))
    if not (maskiert or zweiseitig):
        return None
    return MaterialRegel(eintrag["name"].strip(), zweiseitig, maskiert)


class AssetMeta(object):
    """Normalisierte Beschreibung eines zu importierenden Assets."""

    def __init__(self, meta_pfad, name, typ, datei, ziel):
        self.meta_pfad = Path(meta_pfad)
        self.name = name
        self.typ = typ
        self.datei = Path(datei)
        self.ziel = ziel
        self.asset_pfad = "%s/%s" % (ziel, name)
        self.skelett_pfad = None
        self.lods = []          # [(stufe, Path)] ab Stufe 1
        self.texturen = []      # [Textur]
        self.zweiseitig = False
        self.materialien = {}   # Materialname -> MaterialRegel
        self.frames = None
        self.quelle = ""
        self.lizenz = ""
        self.hinweise = []

    @property
    def rang(self):
        return TYPEN[self.typ][0]

    @property
    def klassenname(self):
        return TYPEN[self.typ][1]

    def __repr__(self):
        return "AssetMeta(%s, %s -> %s)" % (self.typ, self.datei.name, self.asset_pfad)


def _erster_text(daten, *schluessel):
    for s in schluessel:
        wert = daten.get(s)
        if isinstance(wert, str) and wert.strip():
            return wert.strip()
    return None


def _pfad_pruefen(pfad, was, meta_name):
    pfad = str(pfad).strip().rstrip("/")
    if "." in pfad.rsplit("/", 1)[-1]:
        pfad = pfad.rsplit(".", 1)[0]
    if not _SPIELPFAD.match(pfad):
        raise MetaFehler("%s: %s '%s' ungueltig (erwartet /Game/<Ordner>/..., nur A-Z, a-z, 0-9, _ und -)"
                         % (meta_name, was, pfad))
    return pfad


def _bool_oder_none(wert):
    if wert is None:
        return None
    if isinstance(wert, bool):
        return wert
    if isinstance(wert, (int, float)):
        return bool(wert)
    text = str(wert).strip().lower()
    if text in ("true", "ja", "1", "yes"):
        return True
    if text in ("false", "nein", "0", "no"):
        return False
    return None


def meta_lesen(pfad):
    """Liest und prueft eine meta.json. Wirft MetaFehler mit verstaendlicher Meldung."""
    pfad = Path(pfad)
    meta_name = pfad.name
    if not meta_name.endswith(META_ENDUNG):
        raise MetaFehler("%s: Dateiname muss auf %s enden" % (meta_name, META_ENDUNG))
    try:
        daten = json.loads(pfad.read_text(encoding="utf-8-sig"))
    except (OSError, ValueError) as ausnahme:
        raise MetaFehler("%s: nicht lesbar bzw. kein gueltiges JSON (%s)" % (meta_name, ausnahme))
    if not isinstance(daten, dict):
        raise MetaFehler("%s: erwartet ein JSON-Objekt" % meta_name)
    stamm = meta_name[:-len(META_ENDUNG)]
    dateien = daten.get("dateien") if isinstance(daten.get("dateien"), dict) else {}

    name = _erster_text(daten, "asset", "name") or stamm
    if not _NAME.match(name):
        raise MetaFehler("%s: Asset-Name '%s' ungueltig (nur A-Z, a-z, 0-9, _ und -)" % (meta_name, name))

    typ_roh = str(daten.get("typ", "")).strip().lower()
    typ = TYP_ALIASE.get(typ_roh)
    if typ is None:
        raise MetaFehler("%s: unbekannter typ '%s' (erlaubt: static, skeletal, animation, textur)"
                         % (meta_name, daten.get("typ")))

    datei = _erster_text(daten, "datei") or _erster_text(dateien, "fbx", "datei", "bild")
    if datei is None and typ != "textur":
        datei = name + ".fbx"
    if datei is None:
        raise MetaFehler("%s: keine Datei angegeben ('datei')" % meta_name)
    datei_pfad = (pfad.parent / datei).resolve()
    if not datei_pfad.is_file():
        raise MetaFehler("%s: Datei fehlt: %s" % (meta_name, datei_pfad))
    if typ != "textur" and datei_pfad.suffix.lower() != ".fbx":
        raise MetaFehler("%s: erwartet eine .fbx-Datei, nicht %s" % (meta_name, datei_pfad.name))

    ziel_roh = _erster_text(daten, "ziel", "unreal_ziel")
    if ziel_roh is None:
        raise MetaFehler("%s: Zielordner fehlt ('unreal_ziel' bzw. 'ziel', z. B. /Game/Equipment/Ball)" % meta_name)
    ziel = _pfad_pruefen(ziel_roh, "Zielordner", meta_name)

    meta = AssetMeta(pfad, name, typ, datei_pfad, ziel)
    meta.quelle = str(daten.get("quelle") or "")
    meta.lizenz = str(daten.get("lizenz") or "")
    meta.zweiseitig = bool(_bool_oder_none(daten.get("zweiseitig")))
    if isinstance(daten.get("frames"), list) and len(daten["frames"]) == 2:
        meta.frames = (daten["frames"][0], daten["frames"][1])

    if name != stamm:
        meta.hinweise.append("Dateiname %s passt nicht zum Asset-Namen %s" % (meta_name, name))
    if not name.startswith(TYPEN[typ][2]):
        meta.hinweise.append("Name %s sollte mit %s beginnen (README 3.2)" % (name, " oder ".join(TYPEN[typ][2])))
    if not meta.lizenz:
        meta.hinweise.append("keine Lizenz angegeben (README 0.2: Lizenzen mitschreiben)")

    # Skelett
    if typ in ("skeletal", "animation"):
        skelett = _erster_text(daten, "skelett_ziel", "skelett")
        if skelett is None:
            if typ == "animation":
                raise MetaFehler("%s: Animation ohne Skelett ('skelett_ziel' bzw. 'skelett')" % meta_name)
            skelett = "SKEL_" + (name[3:] if name.startswith("SK_") else name)
        if not skelett.startswith("/"):
            skelett = "%s/%s" % (ziel, skelett)
        meta.skelett_pfad = _pfad_pruefen(skelett, "Skelett", meta_name)
        if not meta.skelett_pfad.rsplit("/", 1)[-1].startswith("SKEL_"):
            meta.hinweise.append("Skelett %s sollte mit SKEL_ beginnen (README 3.2)" % meta.skelett_pfad)

    # LODs (nur Static Meshes; Stufe 0 ist die Hauptdatei)
    lods = dateien.get("lods") if isinstance(dateien.get("lods"), list) else []
    stufen = {}
    for eintrag in lods:
        if not isinstance(eintrag, dict):
            raise MetaFehler("%s: LOD-Eintrag ist kein Objekt: %r" % (meta_name, eintrag))
        try:
            stufe = int(eintrag.get("stufe"))
        except (TypeError, ValueError):
            raise MetaFehler("%s: LOD-Eintrag ohne gueltige 'stufe': %r" % (meta_name, eintrag))
        lod_datei = eintrag.get("datei")
        if not isinstance(lod_datei, str) or not lod_datei.strip():
            raise MetaFehler("%s: LOD%d ohne 'datei'" % (meta_name, stufe))
        lod_pfad = (pfad.parent / lod_datei.strip()).resolve()
        if stufe == 0:
            if lod_pfad != datei_pfad:
                meta.hinweise.append("LOD0-Datei %s weicht von der Hauptdatei ab - es zaehlt die Hauptdatei"
                                     % lod_pfad.name)
            continue
        if stufe < 0 or stufe in stufen:
            raise MetaFehler("%s: LOD-Stufe %d ungueltig oder doppelt" % (meta_name, stufe))
        if not lod_pfad.is_file():
            raise MetaFehler("%s: LOD%d-Datei fehlt: %s" % (meta_name, stufe, lod_pfad))
        stufen[stufe] = lod_pfad
    if stufen:
        if typ != "static":
            meta.hinweise.append("LOD-Dateien werden nur fuer Static Meshes importiert - ignoriert")
        elif sorted(stufen) != list(range(1, len(stufen) + 1)):
            raise MetaFehler("%s: LOD-Stufen muessen lueckenlos ab 1 nummeriert sein, gefunden %s"
                             % (meta_name, sorted(stufen)))
        else:
            meta.lods = [(s, stufen[s]) for s in sorted(stufen)]

    # Texturen
    texturen = dateien.get("texturen") if isinstance(dateien.get("texturen"), list) else []
    if typ == "textur":
        # Einzelne Textur: die Datei selbst ist die erste Textur und bekommt den Asset-Namen
        texturen = [dict(daten, datei=str(datei_pfad), _name=name)] + list(texturen)
    gesehen = set()
    for eintrag in texturen:
        if not isinstance(eintrag, dict) or not isinstance(eintrag.get("datei"), str):
            raise MetaFehler("%s: Textur-Eintrag ohne 'datei': %r" % (meta_name, eintrag))
        tex_pfad = (pfad.parent / eintrag["datei"].strip()).resolve()
        if not tex_pfad.is_file():
            raise MetaFehler("%s: Textur fehlt: %s" % (meta_name, tex_pfad))
        textur = Textur(tex_pfad, eintrag.get("rolle"), _bool_oder_none(eintrag.get("srgb")),
                        eintrag.get("konvention"), eintrag.get("material"), eintrag.get("_name"))
        if not _NAME.match(textur.name):
            raise MetaFehler("%s: Texturname '%s' ungueltig (nur A-Z, a-z, 0-9, _ und -)" % (meta_name, textur.name))
        if textur.name.lower() in gesehen:
            continue
        gesehen.add(textur.name.lower())
        if not textur.name.startswith("T_"):
            meta.hinweise.append("Textur %s sollte mit T_ beginnen (README 3.2)" % textur.name)
        meta.texturen.append(textur)

    # Material-Einstellungen
    for eintrag in daten.get("materialien") if isinstance(daten.get("materialien"), list) else []:
        regel = _material_regel(eintrag)
        if regel is not None:
            meta.materialien[regel.name] = regel
    return meta


def meta_dateien_finden(wurzel):
    """Alle *.meta.json unter wurzel (rekursiv), sortiert."""
    wurzel = Path(wurzel)
    if not wurzel.is_dir():
        return []
    return sorted((p for p in wurzel.rglob("*" + META_ENDUNG) if p.is_file()), key=lambda p: str(p).lower())


def metas_ordnen(metas):
    """Reihenfolge: Texturen, Static Meshes, Skeletal Meshes, Animationen; doppelte Zielpfade -> MetaFehler."""
    gesehen = {}
    for meta in metas:
        schluessel = meta.asset_pfad.lower()
        if schluessel in gesehen:
            raise MetaFehler("%s und %s erzeugen beide %s" % (
                gesehen[schluessel].meta_pfad.name, meta.meta_pfad.name, meta.asset_pfad))
        gesehen[schluessel] = meta
    return sorted(metas, key=lambda m: (m.rang, m.ziel.lower(), m.name.lower()))


# =====================================================================================================================
# Teil 2: Import in Unreal
# =====================================================================================================================

class FbxWeiche(object):
    """Schaltet Interchange fuer FBX waehrend des Imports ab (klassischer Importer mit FbxImportUI) und stellt
    den alten Wert danach wieder her. legacy: True = klassischer Importer erwartet, False = Interchange."""

    def __init__(self, interchange_erzwingen=False):
        self.interchange_erzwingen = interchange_erzwingen
        self.vorher = None
        self.legacy = True
        self.umgeschaltet = False

    def __enter__(self):
        self.vorher = mb.cvar_bool(FBX_CVAR)
        if self.interchange_erzwingen:
            self.legacy = False
            if not self.vorher:
                mb.konsolenbefehl("%s 1" % FBX_CVAR)
                self.umgeschaltet = True
            mb.info("FBX-Import: Interchange (--interchange). FbxImportUI-Einstellungen uebersetzt Unreal selbst.")
            return self
        if self.vorher:
            mb.konsolenbefehl("%s 0" % FBX_CVAR)
            self.umgeschaltet = True
            if mb.cvar_bool(FBX_CVAR):
                self.legacy = False
                mb.hinweis("FBX-Import: %s laesst sich nicht auf 0 setzen - Import ueber Interchange; die "
                           "FbxImportUI-Einstellungen uebersetzt Unreal selbst. Ergebnis im Editor ansehen "
                           "(Massstab, Achsen, Kollision)." % FBX_CVAR)
            else:
                mb.info("FBX-Import: Interchange fuer diesen Lauf abgeschaltet (%s 0) - klassischer FBX-Importer "
                        "mit FbxImportUI." % FBX_CVAR)
        else:
            mb.info("FBX-Import: klassischer FBX-Importer (%s ist 0 oder gibt es in dieser Version nicht)." % FBX_CVAR)
        return self

    def __exit__(self, typ, wert, tb):
        if self.umgeschaltet:
            mb.konsolenbefehl("%s %d" % (FBX_CVAR, 1 if self.vorher else 0))
        return False


def _klasse(meta):
    klasse = getattr(unreal, meta.klassenname, None)
    if klasse is None:
        mb.abbrechen("unreal.%s gibt es nicht." % meta.klassenname)
    return klasse


def _importweg(asset):
    daten = mb.lies(asset, "asset_import_data")
    name = mb.klassenname(daten)
    if "Interchange" in name:
        return "Interchange"
    if name.startswith("Fbx"):
        return "klassischer FBX-Importer"
    return "unbekannt (%s)" % name


def _import_task(datei, ziel, name, optionen=None):
    """Ein AssetImportTask (automatisch, ersetzt Vorhandenes). Rueckgabe: gemeldete Paketpfade."""
    task = unreal.AssetImportTask()
    mb.setze_alle(task, [("filename", str(datei)), ("destination_path", ziel), ("destination_name", name),
                         ("replace_existing", True), ("automated", True), ("save", False)], was="AssetImportTask")
    # ohne das nimmt ein Re-Import die alten Einstellungen des vorhandenen Assets statt unserer
    mb.setze(task, "replace_existing_settings", True, pflicht=False, was="AssetImportTask")
    if optionen is not None:
        mb.setze(task, "options", optionen, was="AssetImportTask")
    mb.info("Importiere %s -> %s/%s" % (mb.relativ(datei), ziel, name))
    mb.asset_tools().import_asset_tasks([task])
    pfade = mb.lies(task, "imported_object_paths") or []
    return [mb.paket_pfad(str(p)) for p in pfade]


def _fbx_optionen(meta, skelett=None):
    """unreal.FbxImportUI fuer den klassischen FBX-Importer (und als Vorlage fuer Interchange)."""
    ui = unreal.FbxImportUI()
    was = "FbxImportUI"
    mit_material = meta.typ != "animation"
    mb.setze_alle(ui, [("automated_import_should_detect_type", False),
                       ("import_materials", mit_material),
                       ("import_textures", mit_material),
                       ("create_physics_asset", False)], was=was)
    if meta.typ == "static":
        mb.setze_alle(ui, [("import_mesh", True), ("import_as_skeletal", False), ("import_animations", False),
                           ("mesh_type_to_import", mb.enum_wert("FBXImportType", "FBXIT_STATIC_MESH"))], was=was)
        daten = mb.lies(ui, "static_mesh_import_data")
        if daten is None:
            mb.abbrechen("FbxImportUI.static_mesh_import_data fehlt.")
        mb.setze_alle(daten, [("auto_generate_collision", False), ("combine_meshes", True)], was="StaticMeshImportData")
        mb.setze_alle(daten, [("generate_lightmap_u_vs", True), ("remove_degenerates", True)], pflicht=False,
                      was="StaticMeshImportData")
    elif meta.typ == "skeletal":
        mb.setze_alle(ui, [("import_mesh", True), ("import_as_skeletal", True), ("import_animations", False),
                           ("mesh_type_to_import", mb.enum_wert("FBXImportType", "FBXIT_SKELETAL_MESH"))], was=was)
        if skelett is not None:
            mb.setze(ui, "skeleton", skelett, was=was)
        daten = mb.lies(ui, "skeletal_mesh_import_data")
        if daten is None:
            mb.abbrechen("FbxImportUI.skeletal_mesh_import_data fehlt.")
        mb.setze_alle(daten, [("import_morph_targets", True), ("use_t0_as_ref_pose", False),
                              ("update_skeleton_reference_pose", False), ("import_meshes_in_bone_hierarchy", True)],
                      pflicht=False, was="SkeletalMeshImportData")
    elif meta.typ == "animation":
        if skelett is None:
            mb.abbrechen("Animation %s ohne Skelett." % meta.name)
        mb.setze_alle(ui, [("import_mesh", False), ("import_as_skeletal", True), ("import_animations", True),
                           ("mesh_type_to_import", mb.enum_wert("FBXImportType", "FBXIT_ANIMATION")),
                           ("skeleton", skelett)], was=was)
        daten = mb.lies(ui, "anim_sequence_import_data")
        if daten is None:
            mb.abbrechen("FbxImportUI.anim_sequence_import_data fehlt.")
        mb.setze(daten, "animation_length", mb.enum_wert("FBXAnimationLengthImportType", "FBXALIT_EXPORTED_TIME"),
                 was="AnimSequenceImportData")
        mb.setze_alle(daten, [("import_bone_tracks", True), ("use_default_sample_rate", False),
                              ("remove_redundant_keys", False), ("import_meshes_in_bone_hierarchy", True),
                              ("delete_existing_morph_target_curves", False)], pflicht=False,
                      was="AnimSequenceImportData")
    else:
        mb.abbrechen("Typ %s hat keine FBX-Optionen." % meta.typ)
    # Achsen und Massstab (Begruendung: mb_gemeinsam.ACHSEN_UND_MASSSTAB) - fuer jeden Typ gleich
    mb.setze_alle(daten, [("convert_scene", True), ("force_front_x_axis", False), ("convert_scene_unit", False),
                          ("import_uniform_scale", 1.0)], was="FBX-Importdaten")
    normalen = mb.enum_wert("FBXNormalImportMethod", "FBXNIM_IMPORT_NORMALS", pflicht=False)
    if normalen is not None and meta.typ != "animation":
        mb.setze(daten, "normal_import_method", normalen, pflicht=False, was="FBX-Importdaten")
    return ui


def _interchange_pipeline(meta, skelett=None):
    """InterchangeGenericAssetsPipeline fuer den Rueckfall-Import. Eigenschaftsnamen schwanken zwischen
    UE 5.4 und 5.8 - fehlende werden nur gemeldet (setze_pfad)."""
    klasse = getattr(unreal, "InterchangeGenericAssetsPipeline", None)
    if klasse is None:
        mb.hinweis("unreal.InterchangeGenericAssetsPipeline gibt es nicht - Standard-Pipeline wird benutzt.")
        return None
    pipeline = klasse()
    was = "InterchangePipeline"
    erzwingen = {"static": "IFMT_STATIC_MESH", "skeletal": "IFMT_SKELETAL_MESH", "animation": "IFMT_NONE"}
    wert = mb.enum_wert("InterchangeForceMeshType", erzwingen[meta.typ], pflicht=False)
    if wert is not None:
        mb.setze_pfad(pipeline, "common_meshes_properties.force_all_mesh_as_type", wert, was)
    mb.setze_pfad(pipeline, "import_offset_uniform_scale", 1.0, was)
    statisch = meta.typ == "static"
    skelettal = meta.typ == "skeletal"
    animation = meta.typ == "animation"
    for punktpfad, wert in (("mesh_pipeline.import_static_meshes", statisch),
                            ("mesh_pipeline.import_skeletal_meshes", skelettal),
                            ("mesh_pipeline.combine_static_meshes", True),
                            ("mesh_pipeline.import_collision", False),
                            ("mesh_pipeline.create_physics_asset", False),
                            ("animation_pipeline.import_animations", animation),
                            ("common_skeletal_meshes_and_animations_properties.import_only_animations", animation)):
        mb.setze_pfad(pipeline, punktpfad, wert, was)
    if skelett is not None:
        mb.setze_pfad(pipeline, "common_skeletal_meshes_and_animations_properties.skeleton", skelett, was)
    return pipeline


def _interchange_import(meta, skelett=None):
    """Zweiter Versuch direkt ueber den InterchangeManager. True, wenn Unreal Erfolg meldet."""
    manager_klasse = getattr(unreal, "InterchangeManager", None)
    parameter_klasse = getattr(unreal, "ImportAssetParameters", None)
    if manager_klasse is None or parameter_klasse is None:
        mb.hinweis("Interchange ist in dieser Engine-Version nicht per Python erreichbar.")
        return False
    try:
        manager = manager_klasse.get_interchange_manager_scripted()
        quelle = manager_klasse.create_source_data(str(meta.datei))
        parameter = parameter_klasse()
        mb.setze(parameter, "is_automated", True, pflicht=False, was="ImportAssetParameters")
        mb.setze(parameter, "replace_existing", True, pflicht=False, was="ImportAssetParameters")
        pipeline = _interchange_pipeline(meta, skelett)
        if pipeline is not None:
            if not mb.setze(parameter, "override_pipelines", [unreal.SoftObjectPath(pipeline.get_path_name())],
                            pflicht=False, was="ImportAssetParameters"):
                mb.setze(parameter, "override_pipelines", [pipeline], pflicht=False, was="ImportAssetParameters")
        mb.info("Interchange-Import %s -> %s" % (mb.relativ(meta.datei), meta.ziel))
        ok = manager.import_asset(meta.ziel, quelle, parameter)
        try:
            manager.wait_until_all_tasks_done(False)
        except Exception:
            pass
        return bool(ok)
    except Exception as ausnahme:
        mb.hinweis("Interchange-Import von %s fehlgeschlagen: %s" % (meta.datei.name, mb.ausnahme_text(ausnahme)))
        return False


def _ergebnis_finden(asset_pfad, klasse, vorher, gemeldet):
    """Das importierte Asset unter asset_pfad; hat der Importer einen anderen Namen gewaehlt (z. B. Interchange),
    wird ein eindeutiger neuer Kandidat passender Klasse dorthin umbenannt. None = nichts entstanden."""
    if mb.asset_existiert(asset_pfad):
        objekt = mb.asset_laden(asset_pfad)
        if isinstance(objekt, klasse):
            return objekt
        mb.abbrechen("%s ist nach dem Import ein %s statt %s." % (asset_pfad, mb.klassenname(objekt), klasse.__name__))
    nachher = set(mb.assets_im_ordner(mb.ordner_von(asset_pfad), rekursiv=True))
    kandidaten = []
    for pfad in sorted((nachher - set(vorher)) | set(gemeldet)):
        if pfad == asset_pfad or not mb.asset_existiert(pfad):
            continue
        objekt = mb.asset_laden(pfad, pflicht=False)
        if isinstance(objekt, klasse):
            kandidaten.append(pfad)
    if not kandidaten:
        return None
    if len(kandidaten) > 1:
        mb.abbrechen("Import von %s hat mehrere %s angelegt (%s) - welches gemeint ist, ist unklar."
                     % (asset_pfad, klasse.__name__, ", ".join(kandidaten)))
    mb.hinweis("Der Importer hat %s statt %s angelegt - wird umbenannt." % (kandidaten[0], asset_pfad))
    mb.umbenennen(kandidaten[0], asset_pfad)
    return mb.asset_laden(asset_pfad)


def _importieren(meta, optionen, skelett, weiche, klasse):
    """Gemeinsamer Ablauf fuer FBX: Import-Task, Ergebnis suchen, ggf. Interchange-Rueckfall."""
    vorher = mb.assets_im_ordner(meta.ziel, rekursiv=True)
    gemeldet = _import_task(meta.datei, meta.ziel, meta.name, optionen)
    asset = _ergebnis_finden(meta.asset_pfad, klasse, vorher, gemeldet)
    if asset is None:
        mb.hinweis("%s: der Import-Task (%s) hat kein %s erzeugt (gemeldet: %s)." % (
            meta.name, "klassischer FBX-Importer" if weiche.legacy else "Interchange", klasse.__name__,
            gemeldet or "nichts"))
        if _interchange_import(meta, skelett):
            asset = _ergebnis_finden(meta.asset_pfad, klasse, vorher, [])
    if asset is None:
        mb.abbrechen("%s: Import ohne Ergebnis. Im Log nach 'LogFbx' bzw. 'LogInterchange' suchen; Datei: %s"
                     % (meta.name, meta.datei))
    return asset


def _static_mesh_werkzeug():
    werkzeug = mb.editor_subsystem("StaticMeshEditorSubsystem")
    if werkzeug is None:
        werkzeug = getattr(unreal, "EditorStaticMeshLibrary", None)
    return werkzeug


def _static_nachbearbeiten(mesh, meta, notizen):
    werkzeug = _static_mesh_werkzeug()
    if werkzeug is None:
        mb.hinweis("StaticMeshEditorSubsystem fehlt - LODs und Kollision nicht bearbeitet.")
        notizen.append("LODs/Kollision nicht bearbeitet")
        return
    # Erst alle alten LODs ausser LOD0 entfernen, damit genau die LODs aus meta.json uebrig bleiben
    try:
        werkzeug.remove_lods(mesh)
    except Exception as ausnahme:
        mb.hinweis("%s: alte LODs nicht entfernbar (%s) - sie werden ueberschrieben." % (meta.name, ausnahme))
    for stufe, datei in meta.lods:
        try:
            index = werkzeug.import_lod(mesh, stufe, str(datei))
        except Exception as ausnahme:
            index = -1
            mb.hinweis("%s: LOD%d aus %s: %s" % (meta.name, stufe, datei.name, mb.ausnahme_text(ausnahme)))
        if index is None or index < 0:
            mb.hinweis("%s: LOD%d aus %s nicht importiert." % (meta.name, stufe, datei.name))
            notizen.append("LOD%d fehlt" % stufe)
    # Keine Kollision: Ball und Schlaeger sind reine Darstellung (auch eine evtl. alte Kollision entfernen)
    kollision_weg = False
    try:
        kollision_weg = bool(werkzeug.remove_collisions(mesh))
        if not kollision_weg:
            mb.hinweis("%s: remove_collisions meldet einen Fehler." % meta.name)
    except Exception as ausnahme:
        mb.hinweis("%s: Kollision nicht entfernbar: %s" % (meta.name, ausnahme))
    try:
        lods = int(werkzeug.get_lod_count(mesh))
    except Exception:
        lods = None
    if lods is not None:
        notizen.append("%d LOD%s" % (lods, "" if lods == 1 else "s"))
        if lods != 1 + len(meta.lods):
            mb.hinweis("%s hat %d LODs, erwartet %d." % (meta.name, lods, 1 + len(meta.lods)))
    notizen.append("ohne Kollision" if kollision_weg else "Kollision pruefen")


def _skelett_sichern(mesh, meta, notizen):
    """Sorgt dafuer, dass das Skelett des Meshes SKEL_<Name> heisst (README 3.2)."""
    skelett = mb.lies(mesh, "skeleton")
    if skelett is None:
        skelett = getattr(mesh, "skeleton", None)
    if skelett is None:
        try:
            skelett = mesh.get_skeleton()
        except Exception:
            skelett = None
    if skelett is None:
        mb.abbrechen("%s: Skeletal Mesh ohne Skelett." % meta.name)
    ist = mb.paket_pfad(skelett.get_path_name())
    if ist == meta.skelett_pfad:
        notizen.append("Skelett %s" % mb.name_von(ist))
        return skelett
    if mb.asset_existiert(meta.skelett_pfad):
        mb.hinweis("%s benutzt das Skelett %s statt des vorhandenen %s (Knochen geaendert?). Wenn das so gewollt "
                   "ist: %s loeschen und neu importieren." % (meta.name, ist, meta.skelett_pfad, meta.skelett_pfad))
        notizen.append("Skelett abweichend: %s" % mb.name_von(ist))
        return skelett
    mb.speichern(mesh)
    mb.speichern(skelett)
    mb.umbenennen(ist, meta.skelett_pfad)
    skelett = mb.asset_laden(meta.skelett_pfad)
    mb.speichern(skelett)
    notizen.append("Skelett %s (neu)" % mb.name_von(meta.skelett_pfad))
    return skelett


def _animation_pruefen(animation, meta, notizen):
    skelett = mb.lies(animation, "skeleton")
    if skelett is not None and mb.paket_pfad(skelett.get_path_name()) != meta.skelett_pfad:
        mb.hinweis("%s haengt am Skelett %s statt %s." % (meta.name, skelett.get_path_name(), meta.skelett_pfad))
        notizen.append("Skelett abweichend")
    try:
        bilder = int(unreal.AnimationLibrary.get_num_frames(animation))
        notizen.append("%d Frames" % bilder)
        if meta.frames is not None:
            erwartet = int(meta.frames[1]) - int(meta.frames[0])
            if abs(bilder - erwartet) > 1:
                mb.hinweis("%s: %d Frames, laut meta.json %d (Frames %s)." % (
                    meta.name, bilder, erwartet, list(meta.frames)))
    except Exception:
        pass


def _textur_einstellen(textur_asset, textur):
    was = "Textur %s" % textur.name
    if textur.ist_normal:
        mb.setze(textur_asset, "srgb", False, was=was)
        mb.setze(textur_asset, "compression_settings",
                 mb.enum_wert("TextureCompressionSettings", "TC_NORMALMAP"), was=was)
        gruppe = mb.enum_wert("TextureGroup", "TEXTUREGROUP_WORLD_NORMAL_MAP", pflicht=False)
        if gruppe is not None:
            mb.setze(textur_asset, "lod_group", gruppe, pflicht=False, was=was)
        mb.setze(textur_asset, "flip_green_channel", bool(textur.opengl), pflicht=False, was=was)
        return "Normal-Map%s" % (" (gruen gespiegelt)" if textur.opengl else "")
    if textur.ist_maske:
        mb.setze(textur_asset, "srgb", False, was=was)
        mb.setze(textur_asset, "compression_settings", mb.enum_wert("TextureCompressionSettings", "TC_MASKS"),
                 pflicht=False, was=was)
        return "Maske (linear)"
    srgb = True if textur.srgb is None else bool(textur.srgb)
    mb.setze(textur_asset, "srgb", srgb, was=was)
    return "Farbe (%s)" % ("sRGB" if srgb else "linear")


def _texturen_importieren(meta, notizen):
    klasse = getattr(unreal, "Texture2D")
    for textur in meta.texturen:
        pfad = mb.asset_pfad(meta.ziel, textur.name)
        mb.vorhandenes_asset(pfad, unreal.Texture)
        vorher = mb.assets_im_ordner(meta.ziel, rekursiv=True)
        gemeldet = _import_task(textur.datei, meta.ziel, textur.name, None)
        asset = _ergebnis_finden(pfad, klasse, vorher, gemeldet)
        if asset is None:
            mb.abbrechen("Textur %s wurde nicht importiert (%s)." % (textur.name, textur.datei))
        art = _textur_einstellen(asset, textur)
        mb.speichern(asset)
        mb.info("Textur %s: %s" % (pfad, art))
    if meta.texturen:
        notizen.append("%d Textur%s" % (len(meta.texturen), "" if len(meta.texturen) == 1 else "en"))


def _materialien_des_assets(asset):
    """{name (klein): Material} aus den Material-Slots (Material- und Slotname)."""
    gefunden = {}
    for slot in mb.lies(asset, "static_materials") or mb.lies(asset, "materials") or []:
        material = mb.lies(slot, "material_interface")
        if material is None:
            continue
        gefunden.setdefault(material.get_name().lower(), material)
        slotname = mb.lies(slot, "material_slot_name")
        if slotname:
            gefunden.setdefault(str(slotname).lower(), material)
    return gefunden


def _maskieren(material, meta):
    """Blend Mode Masked; Opacity Mask = bisherige Opacity-Verbindung bzw. Alpha (A) der Grundfarbe-Textur."""
    geaendert = False
    maskiert = mb.enum_wert("BlendMode", "BLEND_MASKED")
    if mb.lies(material, "blend_mode") != maskiert:
        mb.setze(material, "blend_mode", maskiert, was="Material %s" % material.get_name())
        geaendert = True
    bibliothek = getattr(unreal, "MaterialEditingLibrary", None)
    eigenschaft = getattr(unreal, "MaterialProperty", None)
    if bibliothek is None or eigenschaft is None:
        mb.hinweis("%s: MaterialEditingLibrary fehlt - Opacity Mask von %s im Editor pruefen." % (
            meta.name, material.get_name()))
        return geaendert
    try:
        if bibliothek.get_material_property_input_node(material, eigenschaft.MP_OPACITY_MASK) is not None:
            return geaendert
        quelle = bibliothek.get_material_property_input_node(material, eigenschaft.MP_OPACITY)
        ausgang = ""
        if quelle is not None:
            try:
                ausgang = bibliothek.get_material_property_input_node_output_name(material, eigenschaft.MP_OPACITY)
            except Exception:
                ausgang = ""
        else:
            quelle = bibliothek.get_material_property_input_node(material, eigenschaft.MP_BASE_COLOR)
        textur_knoten = getattr(unreal, "MaterialExpressionTextureBase", None)
        if quelle is not None and textur_knoten is not None and isinstance(quelle, textur_knoten):
            ausgang = "A"   # Alphakanal = Bedeckung durch Saiten
        if quelle is None or not bibliothek.connect_material_property(quelle, ausgang, eigenschaft.MP_OPACITY_MASK):
            mb.hinweis("%s: Opacity Mask von %s nicht verbunden - im Material-Editor pruefen." % (
                meta.name, material.get_name()))
            return geaendert
        mb.info("%s: Opacity Mask <- %s.%s" % (material.get_name(), quelle.get_name(), ausgang or "(erster Ausgang)"))
        return True
    except Exception as ausnahme:
        mb.hinweis("%s: Opacity Mask von %s nicht verbunden (%s)." % (meta.name, material.get_name(), ausnahme))
        return geaendert


def _materialien_einstellen(asset, meta, notizen):
    """Zweiseitig und Masked laut meta.json (Material-Regeln bzw. "zweiseitig" fuer alle Materialien)."""
    vorhanden = _materialien_des_assets(asset)
    regeln = dict((n, MaterialRegel(r.name, r.zweiseitig, r.maskiert)) for n, r in meta.materialien.items())
    if meta.zweiseitig:
        for material in vorhanden.values():
            regel = regeln.setdefault(material.get_name(), MaterialRegel(material.get_name()))
            regel.zweiseitig = True
    zweiseitig = maskiert = 0
    for name, regel in sorted(regeln.items()):
        material = vorhanden.get(name.lower()) or mb.asset_laden(mb.asset_pfad(meta.ziel, name), pflicht=False)
        if material is None:
            mb.hinweis("%s: Material %s laut meta.json nicht gefunden (Name in Blender = Name in Unreal?)." % (
                meta.name, name))
            continue
        if not isinstance(material, unreal.Material):
            mb.hinweis("%s: %s ist eine Material-Instanz (%s) - Zweiseitig/Masked bitte im Eltern-Material setzen."
                       % (meta.name, material.get_name(), mb.klassenname(material)))
            continue
        geaendert = False
        if regel.zweiseitig and not mb.lies(material, "two_sided"):
            geaendert = mb.setze(material, "two_sided", True, pflicht=False) or geaendert
        if regel.zweiseitig:
            zweiseitig += 1
        if regel.maskiert:
            geaendert = _maskieren(material, meta) or geaendert
            maskiert += 1
        if geaendert:
            try:
                unreal.MaterialEditingLibrary.recompile_material(material)
            except Exception as ausnahme:
                mb.hinweis("%s: Material %s nicht neu uebersetzt: %s" % (meta.name, material.get_name(), ausnahme))
            mb.speichern(material)
    if zweiseitig:
        notizen.append("%d Material%s zweiseitig" % (zweiseitig, "" if zweiseitig == 1 else "ien"))
    if maskiert:
        notizen.append("%d Material%s Masked" % (maskiert, "" if maskiert == 1 else "ien"))


def asset_importieren(meta, weiche):
    """Importiert ein Asset laut meta. Rueckgabe: Kurztext fuer die Bilanz."""
    for text in meta.hinweise:
        mb.hinweis("%s: %s" % (meta.meta_pfad.name, text))
    mb.info("%s (%s) aus %s | Quelle: %s | Lizenz: %s" % (meta.asset_pfad, meta.typ, mb.relativ(meta.meta_pfad),
                                                          meta.quelle or "-", meta.lizenz or "-"))
    mb.ordner_anlegen(meta.ziel)
    klasse = _klasse(meta)
    notizen = []

    if meta.typ == "textur":
        _texturen_importieren(meta, notizen)
        mb.ordner_speichern(meta.ziel)
        return ", ".join(notizen)

    mb.vorhandenes_asset(meta.asset_pfad, klasse)
    skelett = None
    if meta.typ == "skeletal":
        skelett = mb.vorhandenes_asset(meta.skelett_pfad, unreal.Skeleton)
        if skelett is not None:
            mb.info("Vorhandenes Skelett wird benutzt: %s" % meta.skelett_pfad)
    elif meta.typ == "animation":
        skelett = mb.asset_laden(meta.skelett_pfad, pflicht=False)
        if not isinstance(skelett, unreal.Skeleton):
            mb.abbrechen("%s: Skelett %s fehlt - zuerst das Skeletal Mesh importieren (meta.json mit typ "
                         "'skeletal')." % (meta.name, meta.skelett_pfad))

    asset = _importieren(meta, _fbx_optionen(meta, skelett), skelett, weiche, klasse)
    weg = _importweg(asset)
    mb.info("%s: Importweg %s" % (meta.asset_pfad, weg))

    if meta.typ == "static":
        _static_nachbearbeiten(asset, meta, notizen)
    elif meta.typ == "skeletal":
        _skelett_sichern(asset, meta, notizen)
    elif meta.typ == "animation":
        _animation_pruefen(asset, meta, notizen)

    _texturen_importieren(meta, notizen)
    if meta.zweiseitig or meta.materialien:
        _materialien_einstellen(asset, meta, notizen)

    mb.speichern(asset)
    mb.ordner_speichern(meta.ziel)
    notizen.append(weg)
    return ", ".join(notizen)


def ausfuehren(nur=None, wurzel=None, interchange=False):
    """Importiert alle Assets mit meta.json unter wurzel (Standard: Unreal/SourceAssets/_erzeugt).
    nur: Liste von Asset-Namen (klein geschrieben) oder None. Wirft MBFehler bei Fehlern."""
    if wurzel:
        wurzel = Path(wurzel)
        if not wurzel.is_absolute():  # relativ zur Repository-Wurzel (Unreal laeuft in Engine/Binaries/Win64)
            wurzel = mb.pfade().repo / wurzel
    else:
        wurzel = mb.pfade().erzeugt
    nur = [n.lower() for n in (nur or [])]
    bilanz = mb.Bilanz("Assets (%s)" % mb.relativ(wurzel))
    dateien = meta_dateien_finden(wurzel)
    if not dateien:
        mb.hinweis("Keine *.meta.json unter %s - zuerst die Blender-Skripte ausfuehren (Tools\\assets.ps1)."
                   % wurzel)
        bilanz.mit_hinweis("(keine Assets)", "nichts zu importieren")
        bilanz.ausgeben()
        return bilanz

    metas = []
    for datei in dateien:
        try:
            metas.append(meta_lesen(datei))
        except MetaFehler as ausnahme:
            mb.fehler(str(ausnahme))
            bilanz.fehlgeschlagen(datei.name, str(ausnahme))
    try:
        metas = metas_ordnen(metas)
    except MetaFehler as ausnahme:
        mb.fehler(str(ausnahme))
        bilanz.fehlgeschlagen("(Zuordnung)", str(ausnahme))
        metas = []
    if nur:
        bekannt = set(m.name.lower() for m in metas)
        for name in nur:
            if name not in bekannt:
                mb.fehler("--nur: %s gibt es nicht (vorhanden: %s)" % (name, ", ".join(m.name for m in metas)))
                bilanz.fehlgeschlagen(name, "unbekanntes Asset")
        metas = [m for m in metas if m.name.lower() in nur]
    mb.info("%d Assets zu importieren: %s" % (len(metas), ", ".join(m.name for m in metas) or "-"))

    if metas:
        with FbxWeiche(interchange_erzwingen=interchange) as weiche:
            for meta in metas:
                try:
                    details = asset_importieren(meta, weiche)
                    bilanz.ok(meta.name, details)
                except Exception as ausnahme:
                    if not isinstance(ausnahme, mb.MBFehler):
                        mb.fehler("%s: %s" % (meta.name, mb.ausnahme_text(ausnahme)))
                    bilanz.fehlgeschlagen(meta.name, mb.ausnahme_text(ausnahme))

    bilanz.ausgeben()
    if bilanz.anzahl_fehler:
        raise mb.MBFehler("Assets: %d Fehler (siehe Tabelle oben)." % bilanz.anzahl_fehler)
    return bilanz


def main(argv=None):
    p = mb.parser("Blender-Assets (meta.json + FBX) nach Unreal importieren")
    p.add_argument("--nur", default="", help="nur diese Assets, z. B. SM_Ball,SK_WuerfelSpieler")
    p.add_argument("--wurzel", default="", help="anderer Suchordner statt Unreal/SourceAssets/_erzeugt")
    p.add_argument("--interchange", action="store_true", help="FBX ueber Interchange importieren (nicht abschalten)")
    argumente = mb.argumente_lesen(p, argv)
    mb.umgebung_melden()
    return ausfuehren(nur=mb.liste_aus_text(argumente.nur), wurzel=argumente.wurzel or None,
                      interchange=argumente.interchange)


if __name__ == "__main__":
    mb.als_skript_ausfuehren(main, "Assets importieren (import_assets.py)")
