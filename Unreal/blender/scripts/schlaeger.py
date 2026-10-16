# -*- coding: utf-8 -*-
"""
schlaeger.py - Tennisschläger (README 3.4, Meilenstein M0).

(a) immer:  Platzhalter SM_SchlaegerPlatzhalter - ovaler Rahmen mit Rechteck-Querschnitt, offenes Herz
            (zwei Streben), achteckiger Griff mit Endkappe, Saitenbett als dünne Fläche mit Alpha-Textur
            T_SchlaegerSaiten.png (1 UV-Einheit = 1 Saitenfeld, 16 Längs- x 19 Quersaiten).
            Maße: Länge 0,6858 m (27 Zoll), Kopf außen 0,26 x 0,33 m.
(b) eigenes Modell des Nutzers (das einzige übernommene Asset, README 3.1/3.4) -> SM_Schlaeger:
            Quelle in dieser Reihenfolge: --quelle <datei>, sonst die erste Datei in Vorgaben/Schlaeger/ mit
            Endung .blend, .fbx, .glb, .gltf, .obj (in dieser Rangfolge), sonst Vorgaben/racket.js (Base64-GLB
            aus dem Browserspiel, wird in eine temporäre .glb im Ausgabeordner dekodiert).
            Aufbereitung: alle Teile zu einem Objekt, Transformationen angewendet, längste Ausdehnung -> Z,
            dünnste -> Y, Griffende = schmaleres Ende nach unten, gleichmäßig auf 0,6858 m Länge skaliert,
            Ursprung am Griffende (Mitte der Endkappe). Material wird übernommen; die Texturen werden als
            T_Schlaeger_<Rolle>.<jpg|png> neben die FBX geschrieben und relativ referenziert. LODs per Decimate.

Achsen (beide Schläger): Ursprung = Griffende, Längsachse +Z (Griff unten, Kopf oben), Kopfbreite entlang X,
Saitenbett-Normale entlang Y. In Unreal kommt das genauso an (+Z Längsachse, X Breite, Y Normale; die
Y-Spiegelung beim Import ist nur der Wechsel nach linkshändig, nichts wird gespiegelt dargestellt). Am Knochen
racket_r des Skeletts zeigt lokal Z ebenfalls entlang des Schafts - der Schläger hängt ohne Versatz daran.

Erzeugt (Ziel in Unreal: /Game/Equipment/Schlaeger):
  _erzeugt/Equipment/Schlaeger/SM_SchlaegerPlatzhalter.fbx (+ T_SchlaegerSaiten.png, .meta.json)
  _erzeugt/Equipment/Schlaeger/SM_Schlaeger.fbx, SM_Schlaeger_LOD1..3.fbx, T_Schlaeger_*.jpg, .meta.json
  _erzeugt/Equipment/Schlaeger/Schlaeger.blend
  _vorschau/SM_SchlaegerPlatzhalter_*.png, _vorschau/SM_Schlaeger_*.png

Aufruf:
  blender.exe -b --factory-startup --python-exit-code 1 -P Unreal\\blender\\scripts\\schlaeger.py -- [--nur-platzhalter]
  python Unreal/blender/scripts/schlaeger.py --ausgabe <ordner> --engine CYCLES        (bpy als Modul)
"""

import base64
import math
import re
import shutil
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "lib"))
import mb_gemeinsam as mb  # noqa: E402

import bpy  # noqa: E402
import bmesh  # noqa: E402
import numpy as np  # noqa: E402
from mathutils import Matrix, Vector  # noqa: E402

ASSET = "SM_Schlaeger"
PLATZHALTER = "SM_SchlaegerPlatzhalter"
BEREICH = "Equipment/Schlaeger"
UNREAL_ZIEL = "/Game/Equipment/Schlaeger"
LAENGE = 0.6858  # 27 Zoll
FORMATE = (".blend", ".fbx", ".glb", ".gltf", ".obj")

VORNE = Vector((0.0, -1.0, 0.0))  # Saitenbett zeigt zur Kamera "vorne"
OBEN = Vector((0.0, 0.0, 1.0))


# ---------------------------------------------------------------------------------------------------------------------
# (a) Platzhalter
# ---------------------------------------------------------------------------------------------------------------------

def saiten_textur(ordner, groesse=64, staerke=6):
    """Kachelbare Saiten-Textur (RGBA): eine Längssaite am linken, eine Quersaite am unteren Rand."""
    bild = bpy.data.images.new("T_SchlaegerSaiten", groesse, groesse, alpha=True)
    px = np.zeros((groesse, groesse, 4), dtype=np.float32)
    farbe = np.array([0.93, 0.92, 0.86, 1.0], dtype=np.float32)
    profil = np.zeros(groesse, dtype=np.float32)
    for i in range(staerke):
        profil[i] = 0.75 + 0.25 * math.sin(math.pi * (i + 0.5) / staerke)  # leichte Rundung
    for y in range(groesse):
        for x in range(groesse):
            h = max(profil[x], profil[y] * 0.97)
            if h > 0.0:
                px[y, x] = farbe * np.array([h, h, h, 1.0], dtype=np.float32)
    bild.pixels.foreach_set(px.ravel())
    pfad = Path(ordner) / "T_SchlaegerSaiten.png"
    bild.filepath_raw = str(pfad)
    bild.file_format = 'PNG'
    bild.save()
    bild.filepath = str(pfad)
    mb.log("geschrieben: %s" % pfad)
    return bild, pfad


def saiten_material(bild):
    mat = mb.material_grundfarbe("M_SchlaegerSaiten", (0.93, 0.92, 0.86), rauheit=0.45, alpha=0.99,
                                 textur=bild.name + ".png", zweiseitig=True,
                                 hinweis="Alpha-Maske, 1 UV-Einheit = 1 Saitenfeld")
    baum = mat.node_tree
    bsdf = mb.principled(mat)
    tex = baum.nodes.new("ShaderNodeTexImage")
    tex.image = bild
    tex.extension = 'REPEAT'
    baum.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    baum.links.new(tex.outputs["Alpha"], bsdf.inputs["Alpha"])
    mb.material_alpha(mat)
    if hasattr(mat, "use_backface_culling"):
        mat.use_backface_culling = False
    return mat


def _prisma(bm, z0, z1, radius, ecken, mat, uv):
    unten = [bm.verts.new((radius * math.cos(2 * math.pi * (i + 0.5) / ecken),
                           radius * math.sin(2 * math.pi * (i + 0.5) / ecken), z0)) for i in range(ecken)]
    oben = [bm.verts.new((v.co.x, v.co.y, z1)) for v in unten]
    flaechen = []
    for i in range(ecken):
        j = (i + 1) % ecken
        f = bm.faces.new((unten[i], unten[j], oben[j], oben[i]))
        for loop, (du, dv) in zip(f.loops, ((0, 0), (1, 0), (1, 1), (0, 1))):
            loop[uv].uv = ((i + du) / ecken, (z0 if dv == 0 else z1) / LAENGE)
        flaechen.append(f)
    flaechen.append(bm.faces.new(list(reversed(unten))))
    flaechen.append(bm.faces.new(oben))
    for f in flaechen:
        f.material_index = mat
    return flaechen


def _strebe(bm, p0, p1, breite, tiefe, mat, uv):
    d = p1 - p0
    winkel = math.atan2(d.x, d.z)
    m = Matrix.Translation((p0 + p1) * 0.5) @ Matrix.Rotation(winkel, 4, 'Y')
    mb.bm_quader(bm, m, (breite, tiefe, d.length + 0.006), mat, uv_schicht=uv)


def platzhalter_bauen(ordner, laenge, kopf_breite, kopf_laenge):
    rahmen_mat = mb.material_grundfarbe("M_SchlaegerRahmen", (0.10, 0.14, 0.24), rauheit=0.32)
    griff_mat = mb.material_grundfarbe("M_SchlaegerGriff", (0.06, 0.06, 0.06), rauheit=0.85)
    bild, textur_pfad = saiten_textur(ordner)
    saiten_mat = saiten_material(bild)

    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    # Griff: achteckig, Umfang ca. 11 cm (Griffstärke L3), mit etwas dickerer Endkappe
    _prisma(bm, 0.0, 0.012, 0.0205, 8, 1, uv)
    _prisma(bm, 0.012, 0.195, 0.0180, 8, 1, uv)

    breite_rahmen, tiefe_rahmen = 0.012, 0.022
    a_m = kopf_breite * 0.5 - breite_rahmen * 0.5  # Mittellinie des Rahmens
    b_m = kopf_laenge * 0.5 - breite_rahmen * 0.5
    zc = laenge - kopf_laenge * 0.5
    # Herz: zwei Streben vom Griff zum Rahmen (Punkt auf der Mittellinie bei x = +-0,075)
    x_an = 0.075
    z_an = zc - b_m * math.sqrt(max(0.0, 1.0 - (x_an / a_m) ** 2))
    for s in (1.0, -1.0):
        _strebe(bm, Vector((s * 0.010, 0.0, 0.185)), Vector((s * x_an, 0.0, z_an)), 0.016, tiefe_rahmen, 0, uv)
    _prisma(bm, 0.180, 0.215, 0.0165, 8, 0, uv)  # Übergang Griff -> Herz

    # Rahmen: elliptischer Ring mit Rechteck-Querschnitt
    n = 96
    ringe = []
    for k in range(n):
        w = 2.0 * math.pi * k / n
        c = Vector((a_m * math.cos(w), 0.0, zc + b_m * math.sin(w)))
        nrm = Vector((math.cos(w) / a_m, 0.0, math.sin(w) / b_m)).normalized()
        ringe.append([bm.verts.new(c + nrm * (sa * breite_rahmen * 0.5) + Vector((0.0, sy * tiefe_rahmen * 0.5, 0.0)))
                      for sa, sy in ((1, 1), (1, -1), (-1, -1), (-1, 1))])
    rahmen_flaechen = []
    for k in range(n):
        r0, r1 = ringe[k], ringe[(k + 1) % n]
        for e in range(4):
            e2 = (e + 1) % 4
            f = bm.faces.new((r0[e], r1[e], r1[e2], r0[e2]))
            f.material_index = 0
            for loop, (dk, de) in zip(f.loops, ((0, 0), (1, 0), (1, 1), (0, 1))):
                loop[uv].uv = ((k + dk) / n * 4.0, (e + de) / 4.0)
            rahmen_flaechen.append(f)
    bmesh.ops.recalc_face_normals(bm, faces=rahmen_flaechen)

    # Saitenbett: Fächer aus Dreiecken in der Ebene y = 0, UV: 1 Einheit = 1 Saitenfeld
    a_s, b_s = a_m - breite_rahmen * 0.5 + 0.001, b_m - breite_rahmen * 0.5 + 0.001
    laengs, quer = 16, 19
    mitte = bm.verts.new((0.0, 0.0, zc))
    rand = [bm.verts.new((a_s * math.cos(2 * math.pi * k / n), 0.0, zc + b_s * math.sin(2 * math.pi * k / n)))
            for k in range(n)]

    def saiten_uv(co):
        return ((co.x + a_s) / (2 * a_s) * laengs, (co.z - (zc - b_s)) / (2 * b_s) * quer)

    for k in range(n):
        f = bm.faces.new((mitte, rand[(k + 1) % n], rand[k]))  # Normale zeigt nach -Y (Kamera "vorne")
        f.material_index = 2
        for loop in f.loops:
            loop[uv].uv = saiten_uv(loop.vert.co)

    obj = mb.objekt_aus_bmesh(PLATZHALTER, bm, [rahmen_mat, griff_mat, saiten_mat])
    mb.glatt_nach_winkel(obj.data, 35.0)
    return obj, textur_pfad, {"laengssaiten": laengs, "quersaiten": quer,
                              "kopf_innen_m": [round(2 * a_s, 4), round(2 * b_s, 4)],
                              "saitenflaeche_cm2": round(math.pi * a_s * b_s * 1e4, 1)}


# ---------------------------------------------------------------------------------------------------------------------
# (b) eigenes Schlägermodell
# ---------------------------------------------------------------------------------------------------------------------

def quelle_finden(args):
    """(pfad, art, beschreibung) der Modellquelle oder (None, None, None)."""
    if args.quelle:
        pfad = Path(args.quelle).expanduser().resolve()
        if not pfad.is_file():
            raise FileNotFoundError("Quelle nicht gefunden: %s" % pfad)
        art = "js" if pfad.suffix.lower() == ".js" else "datei"
        return pfad, art, "eigenes Schlaegermodell des Nutzers (%s)" % mb.repo_relativ(pfad)
    ordner = mb.VORGABEN_ORDNER / "Schlaeger"
    if ordner.is_dir():
        kandidaten = sorted((p for p in ordner.iterdir() if p.is_file() and p.suffix.lower() in FORMATE),
                            key=lambda p: (FORMATE.index(p.suffix.lower()), p.name.lower()))
        if kandidaten:
            pfad = kandidaten[0]
            return pfad, "datei", "eigenes Schlaegermodell des Nutzers (%s)" % mb.repo_relativ(pfad)
    js = mb.VORGABEN_ORDNER / "racket.js"
    if js.is_file():
        return js, "js", ("eigenes Schlaegermodell des Nutzers (Vorgaben/racket.js, Texturen fuer das "
                          "Browserspiel auf 1024 px verkleinert)")
    return None, None, None


def glb_aus_js(js, ziel):
    """Dekodiert das Base64-GLB aus racket.js ((window.MODEL_PACKS = ...)["racket"] = "<base64>")."""
    text = Path(js).read_text(encoding="utf-8", errors="replace")
    treffer = re.search(r"\[\s*[\"']racket[\"']\s*\]\s*=\s*[\"']([A-Za-z0-9+/=\s]+)[\"']", text)
    if treffer is None:
        treffer = re.search(r"[\"']([A-Za-z0-9+/=]{1000,})[\"']", text)
    if treffer is None:
        raise ValueError("kein Base64-Modell in %s gefunden" % js)
    daten = base64.b64decode(re.sub(r"\s+", "", treffer.group(1)), validate=True)
    if daten[:4] != b"glTF":
        raise ValueError("dekodierte Daten sind kein GLB (Kennung %r)" % daten[:4])
    Path(ziel).write_bytes(daten)
    mb.log("GLB dekodiert: %s (%.1f MB)" % (ziel, len(daten) / 1e6))
    return Path(ziel)


def modell_importieren(pfad):
    """Importiert eine Datei und gibt die neuen Objekte zurück."""
    vorher = set(bpy.data.objects)
    endung = pfad.suffix.lower()
    if endung in (".glb", ".gltf"):
        bpy.ops.import_scene.gltf(filepath=str(pfad))
    elif endung == ".fbx":
        bpy.ops.import_scene.fbx(filepath=str(pfad))
    elif endung == ".obj":
        if hasattr(bpy.ops.wm, "obj_import"):
            bpy.ops.wm.obj_import(filepath=str(pfad))
        else:
            bpy.ops.import_scene.obj(filepath=str(pfad))
    elif endung == ".blend":
        with bpy.data.libraries.load(str(pfad), link=False) as (von, zu):
            zu.objects = list(von.objects)
        for o in zu.objects:
            if o is not None and o.name not in bpy.context.scene.collection.objects:
                bpy.context.scene.collection.objects.link(o)
    else:
        raise ValueError("Format nicht unterstuetzt: %s" % pfad.name)
    return [o for o in bpy.data.objects if o not in vorher]


def _koordinaten(me):
    co = np.empty(len(me.vertices) * 3, dtype=np.float64)
    me.vertices.foreach_get("co", co)
    return co.reshape(-1, 3)


def ausrichten(obj, laenge):
    """Längste Ausdehnung -> Z, dünnste -> Y, Griff (schmaleres Ende) unten, Länge = laenge, Ursprung am
    Griffende. Rückgabe: Protokoll für meta.json."""
    me = obj.data
    co = _koordinaten(me)
    ausdehnung = co.max(axis=0) - co.min(axis=0)
    duenn, mittel, lang = (int(i) for i in np.argsort(ausdehnung))
    perm = Matrix(((0.0,) * 3,) * 3)
    perm[0][mittel] = 1.0
    perm[1][duenn] = 1.0
    perm[2][lang] = 1.0
    if perm.determinant() < 0:
        perm[0] = -perm[0]
    me.transform(perm.to_4x4())
    co = _koordinaten(me)
    z0, z1 = co[:, 2].min(), co[:, 2].max()
    band = (z1 - z0) * 0.10
    breite_unten = np.ptp(co[co[:, 2] < z0 + band, 0])
    breite_oben = np.ptp(co[co[:, 2] > z1 - band, 0])
    gedreht = bool(breite_unten > breite_oben)
    if gedreht:
        me.transform(Matrix.Rotation(math.pi, 4, 'X'))
        co = _koordinaten(me)
        z0, z1 = co[:, 2].min(), co[:, 2].max()
    faktor = laenge / (z1 - z0)
    me.transform(Matrix.Scale(faktor, 4))
    co = _koordinaten(me)
    z0 = co[:, 2].min()
    kappe = co[co[:, 2] < z0 + 0.01]
    ursprung = Vector((float(kappe[:, 0].mean()), float(kappe[:, 1].mean()), float(z0)))
    me.transform(Matrix.Translation(-ursprung))
    me.update()
    achsen = "XYZ"
    return {"achsen_zuordnung": {"x": achsen[mittel], "y": achsen[duenn], "z": achsen[lang]},
            "um_180_grad_gedreht": gedreht, "skalierung": round(faktor, 6),
            "ausdehnung_quelle": [round(float(v), 5) for v in ausdehnung]}


def _rolle(knoten):
    """Rolle einer Bildtextur anhand der Verknüpfungen zum Principled BSDF."""
    besucht, offen = set(), [(knoten, None)]
    while offen:
        n, ueber = offen.pop()
        if n.name in besucht:
            continue
        besucht.add(n.name)
        for ausgang in n.outputs:
            for link in ausgang.links:
                ziel = link.to_node
                if ziel.type == 'BSDF_PRINCIPLED':
                    eingang = link.to_socket.name
                    if eingang == "Base Color":
                        return "BaseColor"
                    if eingang in ("Roughness", "Metallic"):
                        return "MetallicRoughness" if ueber == 'SEPARATE' else eingang
                    if eingang == "Normal":
                        return "Normal"
                    if eingang == "Alpha":
                        return "Alpha"
                    if eingang.startswith("Emission"):
                        return "Emissive"
                    return eingang.replace(" ", "")
                art = ueber
                if ziel.type in ('SEPARATE_COLOR', 'SEPRGB', 'SEPARATE_RGB'):
                    art = 'SEPARATE'
                offen.append((ziel, art))
    return "Sonstige"


ENDUNGEN = {"JPEG": ".jpg", "PNG": ".png", "TARGA": ".tga", "BMP": ".bmp", "WEBP": ".webp", "OPEN_EXR": ".exr"}


def texturen_auslagern(obj, ordner, praefix="T_Schlaeger"):
    """Benennt das Material in M_Schlaeger um und schreibt alle Bildtexturen als Dateien neben die FBX."""
    texturen = []
    materialien = [m for m in obj.data.materials if m is not None]
    for i, mat in enumerate(materialien):
        mat.name = "M_Schlaeger" if len(materialien) == 1 else "M_Schlaeger_%d" % i
        if mat.node_tree is None:
            continue
        for knoten in mat.node_tree.nodes:
            if knoten.type != 'TEX_IMAGE' or knoten.image is None:
                continue
            bild = knoten.image
            rolle = _rolle(knoten)
            endung = ENDUNGEN.get(bild.file_format, ".png")
            name = "%s_%s" % (praefix, rolle) if len(materialien) == 1 else "%s_%d_%s" % (praefix, i, rolle)
            ziel = Path(ordner) / (name + endung)
            if bild.packed_file is not None:
                ziel.write_bytes(bytes(bild.packed_file.data))
                bild.unpack(method='REMOVE')
            else:
                quelle = Path(bpy.path.abspath(bild.filepath))
                if quelle.is_file():
                    if quelle.resolve() != ziel.resolve():
                        shutil.copyfile(quelle, ziel)
                else:
                    bild.filepath_raw = str(ziel)
                    bild.save()
            bild.filepath = str(ziel)
            bild.name = name
            bild.reload()
            farbraum = bild.colorspace_settings.name
            eintrag = {"datei": ziel.name, "rolle": rolle, "material": mat.name, "groesse_px": list(bild.size),
                       "srgb": farbraum not in ("Non-Color", "Linear", "Raw", "Linear Rec.709")}
            if rolle == "MetallicRoughness":
                eintrag["kanaele"] = "glTF: G = Rauheit, B = Metall"
            if rolle == "Normal":
                eintrag["konvention"] = "OpenGL (+Y); in Unreal flip_green_channel=True setzen"
            texturen.append(eintrag)
            mb.log("Textur %s -> %s" % (rolle, ziel))
    return texturen


def _einzelteile(me):
    bm = bmesh.new()
    bm.from_mesh(me)
    bm.verts.ensure_lookup_table()
    gesehen = np.zeros(len(bm.verts), dtype=bool)
    teile = 0
    for v in bm.verts:
        if gesehen[v.index]:
            continue
        teile += 1
        stapel = [v]
        gesehen[v.index] = True
        while stapel:
            x = stapel.pop()
            for e in x.link_edges:
                o = e.other_vert(x)
                if not gesehen[o.index]:
                    gesehen[o.index] = True
                    stapel.append(o)
    bm.free()
    return teile


def modell_aufbereiten(args, ordner):
    pfad, art, beschreibung = quelle_finden(args)
    if pfad is None:
        mb.log("Hinweis: kein eigenes Schlaegermodell gefunden (Vorgaben/Schlaeger/ oder Vorgaben/racket.js) - "
               "nur Platzhalter")
        return None
    tmp_glb = None
    if art == "js":
        tmp_glb = glb_aus_js(pfad, ordner / "_quelle_racket.glb")
        neu = modell_importieren(tmp_glb)
    else:
        neu = modell_importieren(pfad)
    meshes = [o for o in neu if o.type == 'MESH']
    if not meshes:
        raise RuntimeError("Quelle enthaelt kein Mesh: %s" % pfad)
    for o in meshes:
        mb.transformation_anwenden(o)
    for o in neu:
        if o.type != 'MESH':
            bpy.data.objects.remove(o, do_unlink=True)
    obj = mb.objekte_vereinen(meshes, ASSET)
    if obj.data.shape_keys:
        obj.shape_key_clear()
    protokoll = ausrichten(obj, args.laenge)
    texturen = texturen_auslagern(obj, ordner)
    if tmp_glb is not None and not args.behalte_glb:
        tmp_glb.unlink()
    protokoll["einzelteile"] = _einzelteile(obj.data)
    protokoll["quelle_datei"] = mb.repo_relativ(pfad)
    protokoll["quelle_format"] = "Base64-GLB in JavaScript" if art == "js" else pfad.suffix.lower()
    mb.log("Modell: %d Dreiecke, %d Einzelteile, Skalierung %.4f, Achsen %s"
           % (mb.dreiecke(obj), protokoll["einzelteile"], protokoll["skalierung"], protokoll["achsen_zuordnung"]))
    return obj, beschreibung, texturen, protokoll


# ---------------------------------------------------------------------------------------------------------------------
# Hauptprogramm
# ---------------------------------------------------------------------------------------------------------------------

def _vorschau(args, asset, obj):
    kopf_mitte = Vector((0.0, 0.0, args.laenge - 0.165))
    return mb.vorschau_rendern(args, asset, [obj], vorne=VORNE, oben=OBEN, nah=(kopf_mitte, 0.10), brennweite=60.0)


def main(args):
    mb.neue_szene()
    ordner = mb.ausgabeordner(args, BEREICH)
    lizenz_eigen = ("Eigentum des Nutzers - nur private Nutzung (README 22.3); Schlaegerlogo vor einer "
                    "Veroeffentlichung ersetzen")
    achsen_text = ("Ursprung = Griffende, Laengsachse +Z, Kopfbreite X, Saitenbett-Normale Y "
                   "(in Unreal ebenso); an racket_r ohne Versatz anhaengen")

    # (a) Platzhalter
    platz, saiten_pfad, saiten_info = platzhalter_bauen(ordner, args.laenge, args.kopf_breite, args.kopf_laenge)
    fbx_platz = ordner / (PLATZHALTER + ".fbx")
    export_platz = mb.exportiere_static_mesh([platz], fbx_platz)
    masse_platz = mb.masse([platz])
    mb.log("Platzhalter: %d Dreiecke, Masse %s" % (mb.dreiecke(platz), masse_platz))

    # (b) eigenes Modell
    modell = None if args.nur_platzhalter else modell_aufbereiten(args, ordner)
    stufen, export_modell, masse_modell, fbx_modell, lod_dateien = [], None, None, None, []
    if modell is not None:
        obj, beschreibung, texturen, protokoll = modell
        stufen = mb.lods_erzeugen(obj, args.lods, basisname=ASSET, glatt_winkel=40.0)
        fbx_modell = ordner / (ASSET + ".fbx")
        export_modell = mb.exportiere_static_mesh([obj], fbx_modell, texturen_einbetten=args.texturen_einbetten)
        lod_dateien.append({"stufe": 0, "datei": fbx_modell.name, "verhaeltnis": stufen[0]["verhaeltnis"],
                            "dreiecke": stufen[0]["dreiecke"]})
        for st in stufen[1:]:
            pfad = ordner / ("%s_LOD%d.fbx" % (ASSET, st["stufe"]))
            mb.exportiere_static_mesh([st["objekt"]], pfad, texturen_einbetten=args.texturen_einbetten)
            lod_dateien.append({"stufe": st["stufe"], "datei": pfad.name, "verhaeltnis": st["verhaeltnis"],
                                "dreiecke": st["dreiecke"]})
        masse_modell = mb.masse([obj])
        mb.log("Modell: Masse %s" % masse_modell)

    blend = mb.blend_speichern(args, ordner / "Schlaeger.blend")

    vorschau_platz = _vorschau(args, PLATZHALTER, platz)
    vorschau_modell = _vorschau(args, ASSET, modell[0]) if modell is not None else []
    if modell is not None and args.vorschau and len(stufen) > 1:
        letzte = stufen[-1]["objekt"]
        vorschau_modell += mb.vorschau_rendern(args, ASSET, [letzte], vorne=VORNE, oben=OBEN,
                                               ansichten=("vorne",), dateiname="%s_LOD%d" % (ASSET, len(stufen) - 1),
                                               brennweite=60.0)

    mb.meta_schreiben(
        ordner, PLATZHALTER, "static",
        quelle="neu erstellt per Skript (Unreal/blender/scripts/schlaeger.py), Platzhalter bis zum eigenen Modell",
        lizenz="eigenes Werk (Matchball Tennis)", unreal_ziel=UNREAL_ZIEL,
        parameter={"laenge_m": args.laenge, "kopf_aussen_m": [args.kopf_breite, args.kopf_laenge],
                   "saiten": saiten_info},
        dateien={"fbx": fbx_platz.name, "texturen": [{"datei": saiten_pfad.name, "rolle": "BaseColor+Alpha",
                                                      "srgb": True, "material": "M_SchlaegerSaiten"}],
                 "lods": [], "blend": blend.name if blend else None,
                 "vorschau": [mb.relativ_zur_ausgabe(args, p) for p in vorschau_platz]},
        materialien=mb.materialien_info([platz]), dreiecke=mb.dreiecke(platz), masse=masse_platz,
        ausrichtung=achsen_text, export=export_platz)

    if modell is not None:
        obj, beschreibung, texturen, protokoll = modell
        mb.meta_schreiben(
            ordner, ASSET, "static", quelle=beschreibung, lizenz=lizenz_eigen, unreal_ziel=UNREAL_ZIEL,
            parameter={"laenge_m": args.laenge, "lod_verhaeltnisse": list(args.lods),
                       "texturen_eingebettet": bool(args.texturen_einbetten), "aufbereitung": protokoll},
            dateien={"fbx": fbx_modell.name, "lods": lod_dateien, "texturen": texturen,
                     "blend": blend.name if blend else None,
                     "vorschau": [mb.relativ_zur_ausgabe(args, p) for p in vorschau_modell]},
            materialien=mb.materialien_info([obj]), dreiecke=stufen[0]["dreiecke"], masse=masse_modell,
            ausrichtung=achsen_text, zweiseitig=True,
            saiten="als Geometrie im Modell enthalten (Einzelteile siehe parameter.aufbereitung) - kein "
                   "Alpha-Material fuer LOD0 noetig; ferne LODs koennten spaeter eine Saitenflaeche mit Alpha bekommen",
            export=export_modell)


def _verhaeltnisse(text):
    werte = [float(x) for x in text.replace(";", ",").split(",") if x.strip()]
    if not werte or any(not (0.0 < w <= 1.0) for w in werte):
        raise ValueError("LOD-Verhaeltnisse muessen zwischen 0 und 1 liegen: %r" % text)
    return werte


def parser():
    p = mb.basis_parser("Tennisschlaeger: Platzhalter und eigenes Modell des Nutzers (FBX, LODs, meta.json)")
    p.add_argument("--quelle", default=None, help="Modelldatei statt Vorgaben/Schlaeger/ bzw. Vorgaben/racket.js")
    p.add_argument("--nur-platzhalter", action="store_true", help="nur SM_SchlaegerPlatzhalter erzeugen")
    p.add_argument("--laenge", type=float, default=LAENGE, help="Gesamtlaenge in m (Standard 0.6858 = 27 Zoll)")
    p.add_argument("--kopf-breite", type=float, default=0.26, help="Platzhalter: Kopfbreite aussen in m")
    p.add_argument("--kopf-laenge", type=float, default=0.33, help="Platzhalter: Kopflaenge aussen in m")
    p.add_argument("--lods", type=_verhaeltnisse, default=[1.0, 0.5, 0.25, 0.12],
                   help="LOD-Verhaeltnisse des eigenen Modells, z. B. 1.0,0.5,0.25,0.12")
    p.add_argument("--texturen-einbetten", action="store_true",
                   help="Texturen in die FBX einbetten (path_mode COPY + embed_textures) statt daneben")
    p.add_argument("--behalte-glb", action="store_true", help="aus racket.js dekodierte .glb nicht loeschen")
    return p


if __name__ == "__main__":
    mb.ausfuehren(main, parser())
