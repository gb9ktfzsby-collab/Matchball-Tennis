# -*- coding: utf-8 -*-
"""
mb_gemeinsam.py - gemeinsame Hilfsfunktionen der Blender-Pipeline von Matchball Tennis
(README 3.1 bis 3.4, Meilenstein M0).

Einbinden in ein Skript unter Unreal/blender/scripts/:

    import sys
    from pathlib import Path
    sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "lib"))
    import mb_gemeinsam as mb

Aufruf (Windows, Blender 5.2 LTS, ohne Oberfläche):

    blender.exe -b --factory-startup --python-exit-code 1 -P Unreal\\blender\\scripts\\ball.py -- --engine AUTO

Aufruf als Python-Modul (``pip install bpy==5.2.*`` unter Python 3.13, z. B. für Tests ohne Blender-Programm).
Im Modul-Modus gibt es kein "--"; die Argumente stehen direkt hinter dem Skript:

    python Unreal/blender/scripts/ball.py --ausgabe /tmp/test --engine CYCLES

Zusätzlich lassen sich alle gemeinsamen Argumente über Umgebungsvariablen setzen (praktisch für
assets.ps1 und für den Modul-Modus): MB_AUSGABE, MB_ENGINE, MB_SAMPLES, MB_AUFLOESUNG, MB_VORSCHAU (0/1),
MB_BLEND (0/1) und MB_ARGS (beliebige weitere Argumente als eine Zeichenkette). Reihenfolge:
Befehlszeile > Umgebungsvariable > Standardwert.

Achsen und Maßstab: siehe ACHSEN_UND_MASSSTAB weiter unten (gilt für jeden FBX-Export dieses Projekts).
"""

import argparse
import contextlib
import ctypes.util
import datetime
import json
import math
import os
import shlex
import sys
import traceback
from pathlib import Path

import bpy
import bmesh
from mathutils import Matrix, Quaternion, Vector

# ---------------------------------------------------------------------------------------------------------------------
# Pfade
# ---------------------------------------------------------------------------------------------------------------------

#: Unreal/blender/lib/mb_gemeinsam.py -> parents[0]=lib, [1]=blender, [2]=Unreal, [3]=Repository-Wurzel
LIB_ORDNER = Path(__file__).resolve().parent
BLENDER_ORDNER = LIB_ORDNER.parent
UNREAL_ORDNER = BLENDER_ORDNER.parent
REPO_WURZEL = UNREAL_ORDNER.parent
VORGABEN_ORDNER = REPO_WURZEL / "Vorgaben"
STANDARD_AUSGABE = UNREAL_ORDNER / "SourceAssets"

#: Faktor Meter -> Zentimeter (1 m = 100 Unreal-Einheiten)
FAKTOR_CM = 100.0

ANSICHTEN = ("vorne", "seitlich", "dreiviertel", "nah")

ACHSEN_UND_MASSSTAB = """
Achsen und Massstab (gilt fuer alle FBX-Exporte von Matchball Tennis)
--------------------------------------------------------------------
Blender: rechtshaendig, Z oben, Meter.  Unreal: linkshaendig, Z oben, X vorne, Y rechts, Zentimeter.

axis_up='Z', axis_forward='Y'
  Das ist Blenders eigenes Achsensystem. Die FBX-Datei meldet damit "Z oben, Front -Y, rechtshaendig" -
  genau das System, auf das der Unreal-FBX-Import umrechnet (Option "Force Front X Axis" AUS, Standard).
  Unreal dreht deshalb nichts (keine Zusatzrotation an Wurzelknoten oder Skelett) und spiegelt nur Y,
  um nach linkshaendig zu wechseln:
        Blender (x, y, z) [m]  ->  Unreal (100*x, -100*y, 100*z) [cm]
  Blender +X = Unreal +X (vorne), Blender +Z = Unreal +Z (oben), Blender +Y = Unreal -Y (beides "links",
  wenn man nach +X schaut). Figuren werden in Blender mit Blick nach +X gebaut und schauen in Unreal nach +X
  (Actor-Vorwaertsrichtung; keine -90-Grad-Drehung der Mesh-Komponente wie beim Epic-Mannequin noetig).
  Die Blender-Vorgabe (axis_forward='-Z', axis_up='Y') ergibt rechnerisch dasselbe, laesst Unreal aber das
  Achsensystem umrechnen; bei Skeletten landet die Umrechnung dann als Drehung im Wurzelknoten.

Massstab: Jede Datei enthaelt bereits Zentimeter (UnitScaleFactor = 1, alle Knoten mit Skalierung 1).
  Exportiert werden temporaere Kopien, deren Geometrie, Knochen, Objekt-Positionen und Positions-Schluessel
  mit 100 multipliziert sind. apply_unit_scale=True rechnet fuer die Meter-Szene noch einmal x100 hinein;
  global_scale=0.01 hebt genau das auf; apply_scale_options='FBX_SCALE_NONE' haelt den FBX-Einheitenfaktor
  bei 1. So ist das Ergebnis unabhaengig von Unreals Option "Convert Scene Unit" (Legacy-Import wie
  Interchange). Mit den Blender-Vorgaben (Meter, FBX_SCALE_NONE, ohne Kopien) bekaeme das Armatur-Objekt die
  Skalierung 100, die Unreal in den Wurzelknochen uebernimmt - angehaengte Objekte waeren dann 100x zu gross;
  mit FBX_SCALE_ALL waere die Datei in Metern und nur mit "Convert Scene Unit" richtig gross.

Knochen: primary_bone_axis='Y', secondary_bone_axis='X' (Blender-Konvention, keine Achskorrektur; auch die
  Vorgabe von Epics "Send to Unreal"). Ein Knochen zeigt in Blender entlang lokal +Y und liegt in Unreal
  wegen der Y-Spiegelung entlang lokal -Y; lokal Z bleibt Z. IK Retargeter und Control Rig kommen mit
  beliebigen Knochenachsen zurecht; Animationscode darf sich aber nicht auf die Achsen des Epic-Mannequins
  (Knochen entlang X) verlassen.
  add_leaf_bones=False (keine "_end"-Knochen), armature_nodetype='NULL' (das Armatur-Objekt wird kein
  Knochen, oberster Knochen ist "root"), use_armature_deform_only=False (Hilfsknochen ohne Gewichte wie
  racket_r, racket_l, ball_hand bleiben erhalten). Das Armatur-Objekt heisst "Armature".

Animation: bake_anim=True, bake_anim_use_all_bones=True (jeder Knochen in jedem Frame),
  bake_anim_use_nla_strips=False und bake_anim_use_all_actions=False (genau die zugewiesene Aktion),
  bake_anim_simplify_factor=0.0 (keine Vereinfachung), bake_anim_step=1.0, force_startend_keying=True.
  Der Take heisst wie die Aktion (die Szene wird waehrend des Exports kurz so benannt). Animations-FBX
  enthalten nur das Skelett; beim Import in Unreal das vorhandene Skelett waehlen und "Import Mesh" aus.

Sonstiges: mesh_smooth_type='FACE' (Glaettungsgruppen, vermeidet die Unreal-Warnung "No smoothing group"),
  use_tspace=False (Unreal berechnet Tangenten selbst), use_triangles=False, use_mesh_modifiers=True,
  path_mode='RELATIVE' (Texturen liegen neben der FBX) bzw. 'COPY' mit embed_textures=True auf Wunsch.
""".strip()

ACHSEN_KURZ = {
    "blender": "rechtshaendig, Z oben, Meter",
    "unreal": "linkshaendig, Z oben, X vorne, Y rechts, Zentimeter",
    "umrechnung": "Blender (x, y, z) m -> Unreal (100x, -100y, 100z) cm",
    "fbx_axis_forward": "Y",
    "fbx_axis_up": "Z",
    "fbx_einheit": "Zentimeter (UnitScaleFactor 1, keine Knotenskalierung)",
    "unreal_import": "Convert Scene an (Standard), Force Front X Axis AUS, Convert Scene Unit egal",
}


def log(text):
    """Einheitliche Meldung (nur ASCII verwenden, damit PowerShell-Protokolle lesbar bleiben)."""
    print("[MB] " + str(text), flush=True)


# ---------------------------------------------------------------------------------------------------------------------
# Argumente
# ---------------------------------------------------------------------------------------------------------------------

def _env_text(name, standard=None):
    wert = os.environ.get(name)
    return wert if wert not in (None, "") else standard


def _env_bool(name, standard):
    wert = os.environ.get(name)
    if wert is None or wert == "":
        return standard
    return wert.strip().lower() not in ("0", "nein", "no", "false", "aus", "off")


def _env_int(name, standard):
    wert = os.environ.get(name)
    try:
        return int(wert) if wert not in (None, "") else standard
    except ValueError:
        log("Warnung: Umgebungsvariable %s=%r ist keine Zahl - nehme %r" % (name, wert, standard))
        return standard


def modul_modus():
    """True, wenn bpy als Python-Modul laeuft (pip install bpy) und nicht im Blender-Programm."""
    return not bpy.app.binary_path


def basis_parser(beschreibung):
    """ArgumentParser mit den gemeinsamen Argumenten aller Blender-Skripte."""
    p = argparse.ArgumentParser(description=beschreibung, allow_abbrev=False)
    p.add_argument("--ausgabe", default=_env_text("MB_AUSGABE", str(STANDARD_AUSGABE)),
                   help="Wurzel fuer _erzeugt/ und _vorschau/ (Standard: Unreal/SourceAssets; Umgebung: MB_AUSGABE)")
    p.add_argument("--engine", default=_env_text("MB_ENGINE", "AUTO").upper(),
                   choices=("AUTO", "EEVEE", "CYCLES", "WORKBENCH"),
                   help="Render-Engine fuer die Vorschaubilder (Umgebung: MB_ENGINE)")
    p.add_argument("--samples", type=int, default=_env_int("MB_SAMPLES", 0),
                   help="Samples der Vorschau (0 = Standard der Engine; Umgebung: MB_SAMPLES)")
    p.add_argument("--aufloesung", type=int, default=_env_int("MB_AUFLOESUNG", 1024),
                   help="Kantenlaenge der Vorschaubilder in Pixel (Umgebung: MB_AUFLOESUNG)")
    p.add_argument("--keine-vorschau", dest="vorschau", action="store_false",
                   default=_env_bool("MB_VORSCHAU", True), help="keine Vorschaubilder rendern (MB_VORSCHAU=0)")
    p.add_argument("--kein-blend", dest="blend", action="store_false",
                   default=_env_bool("MB_BLEND", True), help="keine .blend-Datei speichern (MB_BLEND=0)")
    return p


def roh_argumente():
    """Argumente fuer das Skript: hinter "--" (Blender), sonst im Modul-Modus alles hinter dem Skriptnamen."""
    argv = list(sys.argv)
    if "--" in argv:
        eigene = argv[argv.index("--") + 1:]
    elif modul_modus() and argv and argv[0] not in ("-c", "-m", ""):
        eigene = argv[1:]
    else:
        eigene = []
    zusatz = _env_text("MB_ARGS")
    if zusatz:
        # Umgebung zuerst, damit die Befehlszeile gewinnt (argparse nimmt den letzten Wert)
        eigene = shlex.split(zusatz, posix=(os.name != "nt")) + eigene
    return eigene


def argumente_lesen(parser):
    args = parser.parse_args(roh_argumente())
    args.ausgabe = Path(args.ausgabe).expanduser().resolve()
    return args


def ausfuehren(hauptfunktion, parser):
    """Startet hauptfunktion(args). Fehler fuehren ohne Oberflaeche zu Exit-Code 1 (fuer assets.ps1)."""
    try:
        args = argumente_lesen(parser)
        log("Blender %s, Skript %s, Ausgabe %s" % (bpy.app.version_string, Path(sys.argv[0]).name
                                                    if modul_modus() else "-P", args.ausgabe))
        hauptfunktion(args)
        log("FERTIG")
    except SystemExit:
        raise
    except Exception:
        traceback.print_exc()
        log("FEHLER - Skript abgebrochen (Meldung siehe oben)")
        if bpy.app.background:
            sys.stdout.flush()
            sys.exit(1)
        raise


# ---------------------------------------------------------------------------------------------------------------------
# Szene, Einheiten, Ausgabeordner
# ---------------------------------------------------------------------------------------------------------------------

def objektmodus():
    obj = bpy.context.view_layer.objects.active if bpy.context.view_layer else None
    if obj is not None and obj.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')


def szene_leeren():
    """Entfernt alle Objekte und Datenbloecke der aktuellen Datei (ohne Factory-Reset).

    Fuer reproduzierbare Ergebnisse Blender trotzdem mit --factory-startup starten (eigene Einstellungen
    und Add-ons des Nutzers bleiben dann aussen vor)."""
    objektmodus()
    szene = bpy.context.scene
    for andere in list(bpy.data.scenes):
        if andere != szene:
            bpy.data.scenes.remove(andere)
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    for coll in list(bpy.data.collections):
        bpy.data.collections.remove(coll)
    sammlungen = ("meshes", "armatures", "materials", "actions", "cameras", "lights", "curves", "node_groups",
                  "textures", "worlds", "shape_keys", "grease_pencils", "lattices", "metaballs", "fonts")
    for name in sammlungen:
        daten = getattr(bpy.data, name, None)
        if daten is None:
            continue
        for block in list(daten):
            try:
                daten.remove(block)
            except (RuntimeError, ReferenceError, TypeError):
                pass
    for bild in list(bpy.data.images):
        if bild.type not in {'RENDER_RESULT', 'COMPOSITING'}:
            bpy.data.images.remove(bild)
    if hasattr(bpy.data, "orphans_purge"):
        bpy.data.orphans_purge(do_local_ids=True, do_linked_ids=True, do_recursive=True)
    szene.name = "Scene"
    return szene


def einheiten_setzen(fps=30):
    """Metrisches System, 1 Blender-Einheit = 1 m, feste Bildrate."""
    szene = bpy.context.scene
    us = szene.unit_settings
    us.system = 'METRIC'
    us.scale_length = 1.0
    us.length_unit = 'METERS'
    if hasattr(us, "mass_unit"):
        us.mass_unit = 'KILOGRAMS'
    szene.render.fps = int(fps)
    szene.render.fps_base = 1.0
    return szene


def neue_szene(fps=30):
    """szene_leeren() + einheiten_setzen() - Standardbeginn jedes Skripts."""
    szene_leeren()
    return einheiten_setzen(fps)


def ausgabeordner(args, bereich):
    """<ausgabe>/_erzeugt/<bereich>/ (bereich z. B. "Equipment/Ball") - wird angelegt."""
    ordner = Path(args.ausgabe) / "_erzeugt" / Path(bereich)
    ordner.mkdir(parents=True, exist_ok=True)
    return ordner


def vorschauordner(args):
    ordner = Path(args.ausgabe) / "_vorschau"
    ordner.mkdir(parents=True, exist_ok=True)
    return ordner


def repo_relativ(pfad):
    """Pfad relativ zur Repository-Wurzel (mit /), sonst absolut."""
    try:
        return Path(pfad).resolve().relative_to(REPO_WURZEL).as_posix()
    except ValueError:
        return Path(pfad).resolve().as_posix()


def relativ_zur_ausgabe(args, pfad):
    """Pfad relativ zur Ausgabewurzel (z. B. "_vorschau/SM_Ball_vorne.png") - fuer meta.json."""
    try:
        return Path(pfad).resolve().relative_to(Path(args.ausgabe).resolve()).as_posix()
    except ValueError:
        return Path(pfad).resolve().as_posix()


def blend_speichern(args, pfad):
    if not args.blend:
        return None
    pfad = Path(pfad)
    pfad.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(pfad), copy=True, compress=True)
    log("geschrieben: %s" % pfad)
    return pfad


# ---------------------------------------------------------------------------------------------------------------------
# Objekte, Geometrie
# ---------------------------------------------------------------------------------------------------------------------

def verknuepfen(obj, sammlung=None):
    (sammlung or bpy.context.scene.collection).objects.link(obj)
    return obj


def objekt_aus_bmesh(name, bm, materialien=(), sammlung=None):
    """Neues Mesh-Objekt aus einem bmesh (bm wird freigegeben)."""
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    for mat in materialien:
        me.materials.append(mat)
    obj = bpy.data.objects.new(name, me)
    return verknuepfen(obj, sammlung)


def bm_quader(bm, matrix, groesse, material_index=0, uv_schicht=None):
    """Quader im bmesh: Mittelpunkt/Ausrichtung aus matrix (4x4), Kantenlaengen groesse=(x, y, z)."""
    sx, sy, sz = groesse
    m = Matrix(matrix) @ Matrix.Diagonal((sx, sy, sz, 1.0))
    erg = bmesh.ops.create_cube(bm, size=1.0, matrix=m, calc_uvs=uv_schicht is not None)
    for v in erg["verts"]:
        for f in v.link_faces:
            f.material_index = material_index
    return erg["verts"]


def glatt_nach_winkel(me, winkel_grad=30.0):
    """Glatte Schattierung mit harten Kanten ab winkel_grad (ersetzt 'Auto Smooth', Blender 4.1+)."""
    bm = bmesh.new()
    bm.from_mesh(me)
    grenze = math.radians(winkel_grad)
    for f in bm.faces:
        f.smooth = True
    for e in bm.edges:
        if not e.is_manifold:
            e.smooth = True
            continue
        e.smooth = e.calc_face_angle(0.0) <= grenze
    bm.to_mesh(me)
    bm.free()
    me.update()


def dreiecke(obj):
    me = obj.data
    return sum(len(p.vertices) - 2 for p in me.polygons)


def welt_bbox(objekte):
    """Achsparallele Huelle (min, max) der ausgewerteten Objekte in Weltkoordinaten."""
    dg = bpy.context.evaluated_depsgraph_get()
    punkte = []
    for obj in objekte:
        oe = obj.evaluated_get(dg)
        if oe.type not in {'MESH', 'CURVE', 'SURFACE', 'FONT', 'META'}:
            continue
        punkte.extend(oe.matrix_world @ Vector(ecke) for ecke in oe.bound_box)
    if not punkte:
        return Vector((-0.5, -0.5, -0.5)), Vector((0.5, 0.5, 0.5))
    lo = Vector((min(p.x for p in punkte), min(p.y for p in punkte), min(p.z for p in punkte)))
    hi = Vector((max(p.x for p in punkte), max(p.y for p in punkte), max(p.z for p in punkte)))
    return lo, hi


def masse(objekte):
    lo, hi = welt_bbox(objekte)
    d = hi - lo
    return {"x_m": round(d.x, 5), "y_m": round(d.y, 5), "z_m": round(d.z, 5),
            "min_m": [round(v, 5) for v in lo], "max_m": [round(v, 5) for v in hi]}


def transformation_anwenden(obj):
    """Objekt-Transformation in die Mesh-Daten uebernehmen (ohne Operator); Objekt danach in der Identitaet."""
    mw = obj.matrix_world.copy()
    obj.parent = None
    if obj.type == 'MESH':
        obj.data.transform(mw, shape_keys=True)
        if mw.determinant() < 0:
            obj.data.flip_normals()
    obj.matrix_world = Matrix.Identity(4)


def objekte_vereinen(objekte, name):
    """Mehrere Mesh-Objekte zu einem verbinden (Operator mit Kontext-Override)."""
    objekte = [o for o in objekte if o.type == 'MESH']
    if not objekte:
        raise RuntimeError("keine Mesh-Objekte zum Vereinen")
    ziel = objekte[0]
    if len(objekte) > 1:
        objektmodus()
        for o in bpy.context.view_layer.objects:
            o.select_set(False)
        for o in objekte:
            o.hide_set(False)
            o.select_set(True)
        bpy.context.view_layer.objects.active = ziel
        with bpy.context.temp_override(active_object=ziel, object=ziel, selected_objects=objekte,
                                       selected_editable_objects=objekte):
            bpy.ops.object.join()
    ziel.name = name
    ziel.data.name = name
    return ziel


# ---------------------------------------------------------------------------------------------------------------------
# Materialien
# ---------------------------------------------------------------------------------------------------------------------

_MATERIAL_INFO = {}


def srgb_zu_linear(c):
    c = float(c)
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def _baum(mat):
    if mat.node_tree is None:  # nur aeltere Blender-Versionen
        mat.use_nodes = True
    return mat.node_tree


def principled(mat):
    baum = _baum(mat)
    for n in baum.nodes:
        if n.type == 'BSDF_PRINCIPLED':
            return n
    ausgabe = next((n for n in baum.nodes if n.type == 'OUTPUT_MATERIAL'), None)
    if ausgabe is None:
        ausgabe = baum.nodes.new("ShaderNodeOutputMaterial")
    bsdf = baum.nodes.new("ShaderNodeBsdfPrincipled")
    baum.links.new(bsdf.outputs["BSDF"], ausgabe.inputs["Surface"])
    return bsdf


def eingang(knoten, *namen):
    """Erster vorhandener Eingang (Namen unterscheiden sich zwischen Blender-Versionen)."""
    for n in namen:
        if n in knoten.inputs:
            return knoten.inputs[n]
    return None


def material_grundfarbe(name, farbe_srgb, rauheit=0.5, metallisch=0.0, alpha=1.0, **zusatz):
    """Einfaches Principled-Material mit Grundfarbe (sRGB 0..1). Merkt sich die Werte fuer meta.json."""
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    bsdf = principled(mat)
    lin = [srgb_zu_linear(c) for c in farbe_srgb[:3]]
    bsdf.inputs["Base Color"].default_value = (lin[0], lin[1], lin[2], 1.0)
    bsdf.inputs["Roughness"].default_value = float(rauheit)
    bsdf.inputs["Metallic"].default_value = float(metallisch)
    if alpha < 1.0:
        bsdf.inputs["Alpha"].default_value = float(alpha)
        material_alpha(mat)
    mat.diffuse_color = (lin[0], lin[1], lin[2], float(alpha))
    mat.roughness = float(rauheit)
    mat.metallic = float(metallisch)
    info = {"grundfarbe_srgb": [round(float(c), 4) for c in farbe_srgb[:3]], "rauheit": float(rauheit),
            "metallisch": float(metallisch)}
    if alpha < 1.0:
        info["alpha"] = float(alpha)
    info.update(zusatz)
    _MATERIAL_INFO[mat.name] = info
    return mat


def material_alpha(mat):
    """Alpha in Eevee zulassen (Blender 4.2+: surface_render_method, aelter: blend_method)."""
    if hasattr(mat, "surface_render_method"):
        mat.surface_render_method = 'DITHERED'
    elif hasattr(mat, "blend_method"):
        mat.blend_method = 'HASHED'


def material_info_setzen(mat, **werte):
    _MATERIAL_INFO.setdefault(mat.name, {}).update(werte)


def materialien_info(objekte):
    """Materialliste fuer meta.json (Slot-Reihenfolge = Unreal-Materialslots)."""
    erg = []
    gesehen = set()
    for obj in objekte:
        for slot_index, mat in enumerate(getattr(obj.data, "materials", [])):
            if mat is None or mat.name in gesehen:
                continue
            gesehen.add(mat.name)
            eintrag = {"name": mat.name, "slot": slot_index}
            eintrag.update(_MATERIAL_INFO.get(mat.name, {}))
            erg.append(eintrag)
    return erg


# ---------------------------------------------------------------------------------------------------------------------
# LODs
# ---------------------------------------------------------------------------------------------------------------------

def lods_erzeugen(obj, verhaeltnisse=(1.0, 0.5, 0.25, 0.12), basisname=None, glatt_winkel=None):
    """LOD-Stufen per Decimate (Collapse). Rueckgabe: Liste von dicts {stufe, objekt, verhaeltnis, dreiecke}.

    Stufe 0 ist obj selbst (bei Verhaeltnis 1.0 unveraendert). Weitere Stufen sind Kopien "<basis>_LOD<n>".
    Formschluessel gehen beim Dezimieren verloren - deshalb erst LODs bilden, dann Formschluessel je LOD
    anlegen. glatt_winkel: optional glatte Schattierung mit harten Kanten ab diesem Winkel."""
    basis = basisname or obj.name
    stufen = []
    sammlung = obj.users_collection[0] if obj.users_collection else bpy.context.scene.collection
    for i, verh in enumerate(verhaeltnisse):
        verh = float(verh)
        if i == 0:
            ziel = obj
            if verh < 0.999:
                _dezimieren(ziel, verh)
        else:
            ziel = obj.copy()
            ziel.data = obj.data.copy()
            if ziel.data.shape_keys:
                ziel.shape_key_clear()
            ziel.name = "%s_LOD%d" % (basis, i)
            ziel.data.name = ziel.name
            sammlung.objects.link(ziel)
            _dezimieren(ziel, verh)
        if glatt_winkel is not None and i > 0:
            glatt_nach_winkel(ziel.data, glatt_winkel)
        stufen.append({"stufe": i, "objekt": ziel, "verhaeltnis": verh, "dreiecke": dreiecke(ziel)})
        log("LOD%d %s: Verhaeltnis %.2f, %d Dreiecke" % (i, ziel.name, verh, stufen[-1]["dreiecke"]))
    return stufen


def _dezimieren(obj, verhaeltnis):
    mod = obj.modifiers.new("MB_Decimate", 'DECIMATE')
    mod.decimate_type = 'COLLAPSE'
    mod.ratio = max(0.001, min(1.0, verhaeltnis))
    mod.use_collapse_triangulate = True
    dg = bpy.context.evaluated_depsgraph_get()
    dg.update()
    neu = bpy.data.meshes.new_from_object(obj.evaluated_get(dg), preserve_all_data_layers=True, depsgraph=dg)
    alt = obj.data
    obj.modifiers.remove(mod)
    neu.name = alt.name
    obj.data = neu
    if alt.users == 0:
        bpy.data.meshes.remove(alt)


# ---------------------------------------------------------------------------------------------------------------------
# Aktionen (Blender 4.2 bis 5.x: ab 4.4 "slotted actions", ab 5.0 ohne action.fcurves)
# ---------------------------------------------------------------------------------------------------------------------

def aktion_zuweisen(obj, aktion):
    ad = obj.animation_data or obj.animation_data_create()
    ad.action = aktion
    if aktion is not None and hasattr(ad, "action_slot") and len(getattr(aktion, "slots", ())) > 0:
        if ad.action_slot is None:
            ad.action_slot = aktion.slots[0]


def aktion_fcurves(aktion):
    """Alle F-Curves einer Aktion, unabhaengig von der Blender-Version."""
    if "slots" in aktion.bl_rna.properties:
        from bpy_extras import anim_utils
        kurven = []
        for slot in aktion.slots:
            beutel = anim_utils.action_get_channelbag_for_slot(aktion, slot)
            if beutel is not None:
                kurven.extend(beutel.fcurves)
        if kurven or "fcurves" not in aktion.bl_rna.properties:
            return kurven
    return list(aktion.fcurves)


def _aktion_skalieren(aktion, faktor):
    for fc in aktion_fcurves(aktion):
        if fc.data_path == "location" or fc.data_path.endswith(".location"):
            for kp in fc.keyframe_points:
                kp.co[1] *= faktor
                kp.handle_left[1] *= faktor
                kp.handle_right[1] *= faktor
            fc.update()


# ---------------------------------------------------------------------------------------------------------------------
# FBX-Export
# ---------------------------------------------------------------------------------------------------------------------

def fbx_einstellungen():
    """Gemeinsame FBX-Einstellungen (Begruendung: ACHSEN_UND_MASSSTAB)."""
    return dict(
        use_selection=True,
        use_visible=False,
        use_active_collection=False,
        global_scale=1.0 / FAKTOR_CM,
        apply_unit_scale=True,
        apply_scale_options='FBX_SCALE_NONE',
        use_space_transform=True,
        bake_space_transform=False,
        axis_forward='Y',
        axis_up='Z',
        object_types={'MESH'},
        use_mesh_modifiers=True,
        use_mesh_modifiers_render=True,
        mesh_smooth_type='FACE',
        use_subsurf=False,
        use_mesh_edges=False,
        use_tspace=False,
        use_triangles=False,
        use_custom_props=False,
        add_leaf_bones=False,
        primary_bone_axis='Y',
        secondary_bone_axis='X',
        use_armature_deform_only=False,
        armature_nodetype='NULL',
        bake_anim=False,
        bake_anim_use_all_bones=True,
        bake_anim_use_nla_strips=False,
        bake_anim_use_all_actions=False,
        bake_anim_force_startend_keying=True,
        bake_anim_step=1.0,
        bake_anim_simplify_factor=0.0,
        path_mode='RELATIVE',
        embed_textures=False,
        batch_mode='OFF',
    )


def _fuer_meta(einstellungen):
    erg = {}
    for k, v in einstellungen.items():
        erg[k] = sorted(v) if isinstance(v, (set, frozenset)) else v
    erg["daten_vor_export"] = "temporaere Kopien x100 (cm), Netto-Faktor 1"
    return erg


def _auswaehlen(objekte):
    vl = bpy.context.view_layer
    for o in vl.objects:
        o.select_set(False)
    for o in objekte:
        o.hide_set(False)
        o.hide_viewport = False
        o.select_set(True)
    vl.objects.active = objekte[0] if objekte else None


@contextlib.contextmanager
def _zentimeter_kopien(objekte):
    """Temporaere Exportkopien in Zentimetern (x100) mit den Original-Namen.

    Geometrie (inkl. Formschluessel), Knochen, Objekt-Positionen und Eltern-Inversen werden skaliert,
    Drehungen bleiben. Die Originale bekommen solange den Zusatz "__mb_original"."""
    szene = bpy.context.scene
    skal = Matrix.Scale(FAKTOR_CM, 4)
    kopien = {}
    umbenannt = []
    try:
        for obj in objekte:
            name = obj.name
            obj.name = name + "__mb_original"
            umbenannt.append((obj, name))
            k = obj.copy()
            if obj.data is not None:
                daten_name = obj.data.name
                obj.data.name = daten_name + "__mb_original"
                umbenannt.append((obj.data, daten_name))
                k.data = obj.data.copy()
                k.data.name = daten_name
            k.name = name
            szene.collection.objects.link(k)
            kopien[obj] = k
        for obj, k in kopien.items():
            if obj.parent is not None and obj.parent in kopien:
                k.parent = kopien[obj.parent]
            elif obj.parent is not None:
                welt = obj.matrix_world.copy()
                k.parent = None
                k.matrix_world = welt
            mb_ = k.matrix_basis.copy()
            mb_.translation = mb_.translation * FAKTOR_CM
            k.matrix_basis = mb_
            mpi = k.matrix_parent_inverse.copy()
            mpi.translation = mpi.translation * FAKTOR_CM
            k.matrix_parent_inverse = mpi
            if k.type == 'MESH':
                k.data.transform(skal, shape_keys=True)
            elif k.type == 'ARMATURE':
                k.data.transform(skal)
                for pb in k.pose.bones:
                    pb.location = pb.location * FAKTOR_CM
            for mod in k.modifiers:
                if getattr(mod, "object", None) in kopien:
                    mod.object = kopien[mod.object]
        bpy.context.view_layer.update()
        yield kopien
    finally:
        for k in list(kopien.values()):
            daten = k.data
            if daten is not None:
                daten.name = daten.name + "__mb_kopie"
            ad = k.animation_data
            if ad is not None and ad.action is not None and ad.action.name.endswith("__mb_cm"):
                aktion = ad.action
                ad.action = None
                bpy.data.actions.remove(aktion)
            bpy.data.objects.remove(k, do_unlink=True)
            if daten is not None and daten.users == 0:
                if isinstance(daten, bpy.types.Mesh):
                    bpy.data.meshes.remove(daten)
                elif isinstance(daten, bpy.types.Armature):
                    bpy.data.armatures.remove(daten)
        for obj, name in umbenannt:
            obj.name = name


def _fbx_schreiben(pfad, einstellungen):
    pfad = Path(pfad)
    pfad.parent.mkdir(parents=True, exist_ok=True)
    erg = bpy.ops.export_scene.fbx(filepath=str(pfad), **einstellungen)
    if 'FINISHED' not in erg or not pfad.is_file():
        raise RuntimeError("FBX-Export fehlgeschlagen: %s" % pfad)
    log("geschrieben: %s" % pfad)


def exportiere_static_mesh(objekte, pfad, texturen_einbetten=False):
    """Static Mesh(es) als FBX in cm. Gibt die verwendeten Einstellungen fuer meta.json zurueck."""
    objektmodus()
    objekte = [o for o in objekte if o.type == 'MESH']
    einst = fbx_einstellungen()
    einst["object_types"] = {'MESH'}
    if texturen_einbetten:
        einst["path_mode"] = 'COPY'
        einst["embed_textures"] = True
    with _zentimeter_kopien(objekte) as kopien:
        _auswaehlen(list(kopien.values()))
        _fbx_schreiben(pfad, einst)
    return _fuer_meta(einst)


def exportiere_skelett(armatur, meshes, pfad, aktion=None, frame_bereich=None, take_name=None,
                       mit_mesh=None):
    """Skelett-Mesh bzw. Animation als FBX in cm.

    aktion=None: Skelett + gewichtete Meshes in Ruhepose (SK_*.fbx, ohne Animation).
    aktion=<Action>: Animations-FBX (A_*.fbx) mit genau dieser Aktion als Take (Name take_name bzw.
    Aktionsname), Frames frame_bereich=(start, ende) bzw. Bereich der Aktion. Standard: nur das Skelett."""
    objektmodus()
    szene = bpy.context.scene
    if mit_mesh is None:
        mit_mesh = aktion is None
    meshes = list(meshes) if mit_mesh else []
    einst = fbx_einstellungen()
    einst["object_types"] = {'ARMATURE', 'MESH'} if meshes else {'ARMATURE'}
    alt = {"name": szene.name, "start": szene.frame_start, "ende": szene.frame_end,
           "frame": szene.frame_current}
    try:
        with _zentimeter_kopien([armatur] + meshes) as kopien:
            ak = kopien[armatur]
            if aktion is None:
                ak.animation_data_clear()
                for pb in ak.pose.bones:
                    pb.matrix_basis = Matrix.Identity(4)
                ak.data.pose_position = 'REST'
                einst["bake_anim"] = False
            else:
                kopie = aktion.copy()
                kopie.name = aktion.name + "__mb_cm"
                _aktion_skalieren(kopie, FAKTOR_CM)
                aktion_zuweisen(ak, kopie)
                ak.data.pose_position = 'POSE'
                if frame_bereich is None:
                    frame_bereich = tuple(int(round(f)) for f in aktion.frame_range)
                szene.frame_start, szene.frame_end = int(frame_bereich[0]), int(frame_bereich[1])
                szene.name = take_name or aktion.name
                einst["bake_anim"] = True
            bpy.context.view_layer.update()
            _auswaehlen(list(kopien.values()))
            _fbx_schreiben(pfad, einst)
    finally:
        szene.name = alt["name"]
        szene.frame_start, szene.frame_end = alt["start"], alt["ende"]
        szene.frame_set(alt["frame"])
    meta = _fuer_meta(einst)
    if aktion is not None:
        meta["take"] = take_name or aktion.name
        meta["frames"] = [int(frame_bereich[0]), int(frame_bereich[1])]
    return meta


# ---------------------------------------------------------------------------------------------------------------------
# meta.json
# ---------------------------------------------------------------------------------------------------------------------

def meta_schreiben(ordner, asset, typ, quelle, lizenz, unreal_ziel, parameter=None, dateien=None, **zusatz):
    """Schreibt <ordner>/<asset>.meta.json (ASCII-sicher). typ: "static", "skeletal" oder "animation"."""
    if typ not in ("static", "skeletal", "animation"):
        raise ValueError("unbekannter Typ %r" % typ)
    skript = Path(sys.argv[0]).resolve() if modul_modus() else _aufrufendes_skript()
    daten = {
        "asset": asset,
        "typ": typ,
        "quelle": quelle,
        "lizenz": lizenz,
        "datum": datetime.datetime.now(datetime.timezone.utc).replace(microsecond=0).isoformat(),
        "skript": repo_relativ(skript) if skript else None,
        "blender": bpy.app.version_string,
        "parameter": parameter or {},
        "dateien": dateien or {},
        "unreal_ziel": unreal_ziel.rstrip("/"),
        "unreal_objekt": "%s/%s.%s" % (unreal_ziel.rstrip("/"), asset, asset),
        "achsen": ACHSEN_KURZ,
    }
    daten.update(zusatz)
    pfad = Path(ordner) / ("%s.meta.json" % asset)
    with open(pfad, "w", encoding="utf-8", newline="\n") as f:
        json.dump(daten, f, indent=2, ensure_ascii=True, default=_json_hilfe)
        f.write("\n")
    log("geschrieben: %s" % pfad)
    return pfad


def _json_hilfe(wert):
    if isinstance(wert, Path):
        return wert.as_posix()
    if isinstance(wert, (Vector,)):
        return [round(v, 6) for v in wert]
    if isinstance(wert, (set, frozenset, tuple)):
        return sorted(wert) if isinstance(wert, (set, frozenset)) else list(wert)
    return str(wert)


def _aufrufendes_skript():
    for rahmen in reversed(traceback.extract_stack()):
        name = Path(rahmen.filename)
        if name.suffix == ".py" and name.resolve() != Path(__file__).resolve() and name.exists():
            return name.resolve()
    return None


# ---------------------------------------------------------------------------------------------------------------------
# Vorschaubilder (README 3.1: vorne, seitlich, dreiviertel, nah)
# ---------------------------------------------------------------------------------------------------------------------

def _gpu_wahrscheinlich():
    """Eevee braucht OpenGL/Vulkan. Unter Windows/macOS ist das praktisch immer da; unter Linux ohne
    Bildschirm nur mit EGL (z. B. Mesa). Ein fehlendes EGL laesst Blender hart abstuerzen - deshalb vorher pruefen."""
    if sys.platform.startswith("win") or sys.platform == "darwin":
        return True
    if os.environ.get("DISPLAY") or os.environ.get("WAYLAND_DISPLAY"):
        return True
    return ctypes.util.find_library("EGL") is not None


def render_engine_setzen(wunsch="AUTO", samples=0):
    """Setzt die Render-Engine der Szene und gibt den Bezeichner zurueck.

    EEVEE: BLENDER_EEVEE_NEXT (Blender 4.2 bis 4.4) bzw. BLENDER_EEVEE (5.x).
    AUTO:  Eevee, wenn eine GPU wahrscheinlich ist, sonst Cycles auf der CPU mit wenigen Samples."""
    szene = bpy.context.scene
    wunsch = (wunsch or "AUTO").upper()
    eevee = ["BLENDER_EEVEE_NEXT", "BLENDER_EEVEE"]
    if wunsch == "AUTO":
        kandidaten = (eevee if _gpu_wahrscheinlich() else []) + ["CYCLES"]
    elif wunsch == "EEVEE":
        kandidaten = eevee + ["CYCLES"]
    elif wunsch == "WORKBENCH":
        kandidaten = ["BLENDER_WORKBENCH"]
    else:
        kandidaten = ["CYCLES"]
    gewaehlt = None
    for k in kandidaten:
        try:
            szene.render.engine = k
            gewaehlt = k
            break
        except (TypeError, ValueError):
            continue
    if gewaehlt is None:
        raise RuntimeError("keine passende Render-Engine gefunden (%s)" % kandidaten)
    if gewaehlt == "CYCLES":
        c = szene.cycles
        c.device = 'CPU'
        c.samples = samples or 48
        if hasattr(c, "use_adaptive_sampling"):
            c.use_adaptive_sampling = True
        if hasattr(c, "use_denoising"):
            c.use_denoising = True
        if hasattr(c, "max_bounces"):
            c.max_bounces = 6
    elif gewaehlt.startswith("BLENDER_EEVEE"):
        e = szene.eevee
        if hasattr(e, "taa_render_samples"):
            e.taa_render_samples = samples or 64
    return gewaehlt


def _farbverwaltung(szene):
    """'Standard': Grundfarben erscheinen unverfaelscht (AgX entsaettigt kraeftige Farben wie Optic Yellow
    sichtbar - fuer die Abnahme von Materialfarben ungeeignet)."""
    vs = szene.view_settings
    for vt in ("Standard", "AgX", "Filmic"):
        try:
            vs.view_transform = vt
            break
        except TypeError:
            continue
    try:
        vs.look = 'None'
    except TypeError:
        pass
    vs.exposure = 0.0
    vs.gamma = 1.0


def _welt_neutral(szene, grau_kamera=0.22, licht=0.35):
    """Neutraler grauer Hintergrund fuer die Kamera, schwaecheres Umgebungslicht fuer die Beleuchtung."""
    welt = bpy.data.worlds.get("MB_Vorschau_Welt") or bpy.data.worlds.new("MB_Vorschau_Welt")
    if welt.node_tree is None:
        welt.use_nodes = True
    baum = welt.node_tree
    baum.nodes.clear()
    aus = baum.nodes.new("ShaderNodeOutputWorld")
    hg_licht = baum.nodes.new("ShaderNodeBackground")
    hg_licht.inputs["Color"].default_value = (1.0, 1.0, 1.0, 1.0)
    hg_licht.inputs["Strength"].default_value = licht
    hg_kamera = baum.nodes.new("ShaderNodeBackground")
    hg_kamera.inputs["Color"].default_value = (grau_kamera, grau_kamera, grau_kamera, 1.0)
    hg_kamera.inputs["Strength"].default_value = 1.0
    pfad = baum.nodes.new("ShaderNodeLightPath")
    mix = baum.nodes.new("ShaderNodeMixShader")
    baum.links.new(pfad.outputs["Is Camera Ray"], mix.inputs[0])
    baum.links.new(hg_licht.outputs["Background"], mix.inputs[1])
    baum.links.new(hg_kamera.outputs["Background"], mix.inputs[2])
    baum.links.new(mix.outputs[0], aus.inputs["Surface"])
    szene.world = welt
    return welt


def blickrichtung(vorne, oben, azimut_grad, hoehe_grad):
    """Einheitsvektor vom Objekt zur Kamera: vorne um 'oben' gedreht (positiv = gegen den Uhrzeigersinn
    von oben gesehen) und um hoehe_grad angehoben."""
    h = Quaternion(oben, math.radians(azimut_grad)) @ vorne
    e = math.radians(hoehe_grad)
    return (h * math.cos(e) + oben * math.sin(e)).normalized()


def _kamera_abstand(punkte, mitte, richtung, oben, halber_winkel, fuellung):
    vorwaerts = -richtung
    rechts = vorwaerts.cross(oben)
    if rechts.length < 1e-6:
        rechts = vorwaerts.orthogonal()
    rechts.normalize()
    hoch = rechts.cross(vorwaerts).normalized()
    t = math.tan(halber_winkel) * fuellung
    abstand = 0.0
    for p in punkte:
        rel = p - mitte
        x, y, z = rel.dot(rechts), rel.dot(hoch), rel.dot(richtung)
        abstand = max(abstand, z + max(abs(x), abs(y)) / t)
    return max(abstand, 1e-3)


def _ecken(lo, hi):
    return [Vector((x, y, z)) for x in (lo.x, hi.x) for y in (lo.y, hi.y) for z in (lo.z, hi.z)]


#: Ansicht -> (Azimut, Hoehe). Azimut -90 = rechte Seite des Assets (von vorne gesehen links im Bild).
ANSICHT_WINKEL = {"vorne": (0.0, 6.0), "seitlich": (-90.0, 6.0), "dreiviertel": (-40.0, 22.0), "nah": (-30.0, 15.0)}


def vorschau_rendern(args, asset, objekte, vorne=(0.0, -1.0, 0.0), oben=(0.0, 0.0, 1.0), nah=None,
                     ansichten=ANSICHTEN, aufloesung=None, dateiname=None, brennweite=70.0, fuellung=0.86,
                     nur_diese=True, rahmen_objekte=None, rahmen_bbox=None):
    """Rendert Vorschaubilder nach <ausgabe>/_vorschau/<asset>_<ansicht>.png und gibt die Pfade zurueck.

    vorne: Richtung, in die das Asset schaut (Kamera "vorne" steht dort). nah: (mitte, radius) fuer die
    Nahaufnahme, sonst oberes Drittel der Huelle. nur_diese: alle anderen Objekte beim Rendern ausblenden.
    rahmen_objekte: Objekte, nach deren Huelle die Kamera ausgerichtet wird (Standard: objekte).
    rahmen_bbox: feste Huelle (min, max) statt rahmen_objekte - z. B. fuer gleichbleibende Kamera bei Animationen."""
    if not args.vorschau:
        return []
    szene = bpy.context.scene
    engine = render_engine_setzen(args.engine, args.samples)
    _farbverwaltung(szene)
    _welt_neutral(szene)
    r = szene.render
    r.resolution_x = r.resolution_y = int(aufloesung or args.aufloesung)
    r.resolution_percentage = 100
    r.film_transparent = False
    r.image_settings.file_format = 'PNG'
    r.image_settings.color_mode = 'RGB'
    r.image_settings.color_depth = '8'

    vorne = Vector(vorne).normalized()
    oben = Vector(oben).normalized()
    sammlung = bpy.data.collections.new("_MB_Vorschau")
    szene.collection.children.link(sammlung)
    kam_daten = bpy.data.cameras.new("_MB_Kamera")
    kam_daten.lens = brennweite
    kam_daten.sensor_fit = 'AUTO'
    kam_daten.sensor_width = 36.0
    kam = bpy.data.objects.new("_MB_Kamera", kam_daten)
    sammlung.objects.link(kam)
    szene.camera = kam
    lichter = []
    for name, az, hoehe, staerke, winkel, schatten in (("Fuehrung", 35.0, 40.0, 3.2, 12.0, True),
                                                        ("Aufhell", -55.0, 15.0, 1.1, 35.0, False),
                                                        ("Kante", 155.0, 40.0, 2.6, 8.0, False)):
        ld = bpy.data.lights.new("_MB_" + name, type='SUN')
        ld.energy = staerke
        ld.angle = math.radians(winkel)
        if hasattr(ld, "use_shadow"):
            ld.use_shadow = schatten
        lo = bpy.data.objects.new("_MB_" + name, ld)
        d = blickrichtung(vorne, oben, az, hoehe)
        lo.rotation_euler = (-d).to_track_quat('-Z', 'Y').to_euler()
        sammlung.objects.link(lo)
        lichter.append(lo)

    versteckt = []
    if nur_diese:
        behalten = set(objekte) | set(rahmen_objekte or ()) | {kam} | set(lichter)
        for o in szene.objects:
            if o not in behalten and o.type in {'MESH', 'CURVE', 'SURFACE', 'FONT', 'META', 'EMPTY'} \
                    and not o.hide_render:
                o.hide_render = True
                versteckt.append(o)

    lo_, hi_ = rahmen_bbox if rahmen_bbox is not None else welt_bbox(rahmen_objekte or objekte)
    mitte_ges = (lo_ + hi_) * 0.5
    halb = math.atan(18.0 / brennweite)
    bilder = []
    try:
        for ansicht in ansichten:
            az, hoehe = ANSICHT_WINKEL.get(ansicht, (0.0, 10.0))
            richtung = blickrichtung(vorne, oben, az, hoehe)
            if ansicht == "nah":
                if nah is not None:
                    mitte, radius = Vector(nah[0]), float(nah[1])
                else:
                    groesse = hi_ - lo_
                    hoch_anteil = oben.dot(groesse)
                    mitte = mitte_ges + oben * (hoch_anteil * 0.33)
                    radius = max(groesse.length * 0.18, 1e-3)
                punkte = _ecken(mitte - Vector((radius,) * 3), mitte + Vector((radius,) * 3))
            else:
                mitte = mitte_ges
                punkte = _ecken(lo_, hi_)
            abstand = _kamera_abstand(punkte, mitte, richtung, oben, halb, fuellung)
            kam.location = mitte + richtung * abstand
            kam.rotation_euler = (-richtung).to_track_quat('-Z', 'Y').to_euler()
            kam_daten.clip_start = max(abstand * 0.01, 1e-4)
            kam_daten.clip_end = abstand * 20.0
            ziel = vorschauordner(args) / ("%s_%s.png" % (dateiname or asset, ansicht))
            r.filepath = str(ziel)
            bpy.ops.render.render(write_still=True)
            if not ziel.is_file():
                raise RuntimeError("Vorschau nicht geschrieben: %s" % ziel)
            log("Vorschau (%s): %s" % (engine, ziel))
            bilder.append(ziel)
    finally:
        for o in versteckt:
            o.hide_render = False
        for o in list(sammlung.objects):
            daten = o.data
            bpy.data.objects.remove(o, do_unlink=True)
            if isinstance(daten, bpy.types.Camera):
                bpy.data.cameras.remove(daten)
            elif isinstance(daten, bpy.types.Light):
                bpy.data.lights.remove(daten)
        bpy.data.collections.remove(sammlung)
    return bilder


def kontaktbogen(bilder_zeilen, ziel):
    """Setzt gleich grosse PNGs (Liste von Zeilen) zu einem Bild zusammen (oben links = bilder_zeilen[0][0])."""
    import numpy as np
    geladen = [[bpy.data.images.load(str(p), check_existing=False) for p in zeile] for zeile in bilder_zeilen]
    try:
        b, h = geladen[0][0].size
        zeilen, spalten = len(geladen), max(len(z) for z in geladen)
        bogen = np.zeros((zeilen * h, spalten * b, 4), dtype=np.float32)
        bogen[..., 3] = 1.0
        for zi, zeile in enumerate(geladen):
            for si, bild in enumerate(zeile):
                px = np.empty(b * h * 4, dtype=np.float32)
                bild.pixels.foreach_get(px)
                y0 = (zeilen - 1 - zi) * h  # Blender-Pixel beginnen unten
                bogen[y0:y0 + h, si * b:(si + 1) * b, :] = px.reshape(h, b, 4)
        neu = bpy.data.images.new("_MB_Kontaktbogen", spalten * b, zeilen * h, alpha=False)
        neu.pixels.foreach_set(bogen.ravel())
        neu.filepath_raw = str(ziel)
        neu.file_format = 'PNG'
        neu.save()
        bpy.data.images.remove(neu)
    finally:
        for zeile in geladen:
            for bild in zeile:
                bpy.data.images.remove(bild)
    log("Kontaktbogen: %s" % ziel)
    return Path(ziel)
