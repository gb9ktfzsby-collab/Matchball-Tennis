# -*- coding: utf-8 -*-
"""
schlaeger.py - Tennisschläger (README 3.4, Meilenstein M0).

(a) immer:  Platzhalter SM_SchlaegerPlatzhalter - ovaler Rahmen mit Rechteck-Querschnitt, offenes Herz
            (zwei Streben), achteckiger Griff mit Endkappe, Saitenbett als dünne Fläche mit Alpha-Textur
            T_SchlaegerPlatzhalter_Saiten_BaseColor.png (kachelnd, 1 UV-Einheit = 1 Saitenfeld, 16 Längs- x
            19 Quersaiten). Maße: Länge 0,6858 m (27 Zoll), Kopf außen 0,26 x 0,33 m.
(b) eigenes Modell des Nutzers (das einzige übernommene Asset, README 3.1/3.4) -> SM_Schlaeger:
            Quelle in dieser Reihenfolge: --quelle <datei>, sonst die erste Datei in Vorgaben/Schlaeger/ mit
            Endung .blend, .fbx, .glb, .gltf, .obj (in dieser Rangfolge), sonst Vorgaben/racket.js (Base64-GLB
            aus dem Browserspiel, wird in eine temporäre .glb im Ausgabeordner dekodiert).
            Aufbereitung: alle Teile zu einem Objekt, Transformationen angewendet, längste Ausdehnung -> Z,
            dünnste -> Y, Griffende = schmaleres Ende nach unten, gleichmäßig auf 0,6858 m Länge skaliert,
            Ursprung am Griffende (Mitte der Endkappe). Material wird übernommen; die Texturen werden als
            T_Schlaeger_<Rolle>.<jpg|png> neben die FBX geschrieben und relativ referenziert.
(c) Saitenbett des eigenen Modells als Alpha-Material (README 3.4): Die Saiten-Geometrie (Saiten und Ösenköpfe,
            Erkennung siehe saitenteile_finden) wird mit Cycles (selected-to-active) auf eine ebene Saitenfläche
            gebacken - T_Schlaeger_Saiten_BaseColor.png (RGB = Grundfarbe inkl. Logo, A = Bedeckung durch Saiten)
            und T_Schlaeger_Saiten_Normal.png - und danach entfernt. Material M_Schlaeger_Saiten: zweiseitig,
            in Unreal Blend Mode "Masked" mit Opacity Mask = Alphakanal. Das Herstellerlogo steht auf den Saiten
            und bleibt dadurch erhalten (private Nutzung, README 22.3). Alle LOD-Stufen nutzen dieselbe
            Saitenfläche; nur der Rahmen wird per Decimate reduziert. --saiten-geometrie behält die alten Saiten.

Achsen (beide Schläger): Ursprung = Griffende, Längsachse +Z (Griff unten, Kopf oben), Kopfbreite entlang X,
Saitenbett-Normale entlang Y. In Unreal kommt das genauso an (+Z Längsachse, X Breite, Y Normale; die
Y-Spiegelung beim Import ist nur der Wechsel nach linkshändig, nichts wird gespiegelt dargestellt). Am Knochen
racket_r des Skeletts zeigt lokal Z ebenfalls entlang des Schafts - der Schläger hängt ohne Versatz daran.
Beide Saitenflächen liegen in der Ebene der Saiten, ihre Flächennormale zeigt nach -Y (Kamera "vorne"), U läuft
entlang +X, V entlang +Z (von vorne gesehen ungespiegelt).

Erzeugt (Ziel in Unreal: /Game/Equipment/Schlaeger):
  _erzeugt/Equipment/Schlaeger/SM_SchlaegerPlatzhalter.fbx (+ T_SchlaegerPlatzhalter_Saiten_BaseColor.png, .meta.json)
  _erzeugt/Equipment/Schlaeger/SM_Schlaeger.fbx, SM_Schlaeger_LOD1..3.fbx, T_Schlaeger_*.jpg,
                               T_Schlaeger_Saiten_BaseColor.png, T_Schlaeger_Saiten_Normal.png, .meta.json
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

SAITEN_FARBE_SRGB = (0.93, 0.92, 0.86)
#: Unreal: Opacity Mask Clip Value des maskierten Saitenmaterials (Alpha ist praktisch binaer, 0,5 = Kante mittig)
OPACITY_CLIP = 0.5


# ---------------------------------------------------------------------------------------------------------------------
# Gemeinsam für beide Saitenbetten (gleicher Aufbau: Material M_<Stamm>_Saiten, Textur T_<Stamm>_Saiten_BaseColor
# mit Bedeckung im Alphakanal, optional T_<Stamm>_Saiten_Normal; zweiseitig; Unreal: Blend Mode Masked)
# ---------------------------------------------------------------------------------------------------------------------

def bild_speichern(name, px, pfad, srgb=True):
    """Schreibt px (H x W x 3|4, Werte 0..1, unterste Zeile zuerst wie in Blender) als 8-Bit-PNG.

    Die Werte werden unverändert als Bytes gespeichert (bei srgb=True also bereits sRGB-kodiert erwartet);
    Alpha ist "straight" (nicht vormultipliziert)."""
    h, b, k = px.shape
    bild = bpy.data.images.new(name, b, h, alpha=(k == 4))
    bild.colorspace_settings.name = 'sRGB' if srgb else 'Non-Color'
    bild.alpha_mode = 'STRAIGHT'
    voll = np.ones((h, b, 4), dtype=np.float32)
    voll[..., :k] = px
    bild.pixels.foreach_set(voll.ravel())
    bild.filepath_raw = str(pfad)
    bild.file_format = 'PNG'
    bild.save()
    bild.filepath = str(pfad)
    mb.log("geschrieben: %s (%d x %d, %s)" % (pfad, b, h, "RGBA" if k == 4 else "RGB"))
    return bild


def saiten_material(name, bild_farbe, bild_normal=None, kachelnd=False, hinweis=""):
    """Saitenmaterial (für Platzhalter und eigenes Modell gleich aufgebaut).

    Base Color = RGB von bild_farbe, Alpha = Alphakanal (Bedeckung durch Saiten), optional Normal-Map
    (OpenGL, Non-Color). Zweiseitig. In Eevee "Dithered", in Cycles echte Transparenz; in Unreal Blend Mode
    "Masked" mit Opacity Mask = Alphakanal (Clip OPACITY_CLIP), Two Sided."""
    mat = mb.material_grundfarbe(name, SAITEN_FARBE_SRGB, rauheit=0.45, zweiseitig=True, hinweis=hinweis)
    baum = mat.node_tree
    bsdf = mb.principled(mat)
    tex = baum.nodes.new("ShaderNodeTexImage")
    tex.image = bild_farbe
    tex.extension = 'REPEAT' if kachelnd else 'EXTEND'
    baum.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    baum.links.new(tex.outputs["Alpha"], bsdf.inputs["Alpha"])
    if bild_normal is not None:
        tex_n = baum.nodes.new("ShaderNodeTexImage")
        tex_n.image = bild_normal
        tex_n.extension = tex.extension
        bild_normal.colorspace_settings.name = 'Non-Color'
        nmap = baum.nodes.new("ShaderNodeNormalMap")
        baum.links.new(tex_n.outputs["Color"], nmap.inputs["Color"])
        baum.links.new(nmap.outputs["Normal"], bsdf.inputs["Normal"])
    mb.material_alpha(mat)
    if hasattr(mat, "use_backface_culling"):
        mat.use_backface_culling = False
    unreal = {"blend_mode": "Masked", "opacity_mask": "Alphakanal von %s" % Path(bild_farbe.filepath_raw).name,
              "opacity_mask_clip_value": OPACITY_CLIP, "two_sided": True}
    mb.material_info_setzen(mat, textur=Path(bild_farbe.filepath_raw).name, alpha="Opacity Mask (maskiert)",
                            unreal=unreal)
    return mat


def saiten_textur_eintraege(material, farbe_datei, normal_datei=None, groesse_px=None, kachelnd=False):
    """meta.json-Einträge (dateien.texturen) eines Saitenbetts: BaseColor, Alpha (gleiche Datei, Kanal A), Normal."""
    gemeinsam = {"material": material}
    if groesse_px is not None:
        gemeinsam["groesse_px"] = list(groesse_px)
    if kachelnd:
        gemeinsam["adressierung"] = "Wrap (kachelnd, 1 UV-Einheit = 1 Saitenfeld)"
    else:
        gemeinsam["adressierung"] = "Clamp (UV 0..1 = Saitenflaeche)"
    eintraege = [
        dict(gemeinsam, datei=farbe_datei, rolle="BaseColor", kanaele="RGB", srgb=True,
             hinweis="RGB = Grundfarbe der Saiten (auch unter transparenten Texeln aufgefuellt), A = Bedeckung"),
        dict(gemeinsam, datei=farbe_datei, rolle="Alpha", kanaele="A", srgb=True,
             unreal="Opacity Mask: Alphakanal dieser Textur an Opacity Mask, Blend Mode Masked, Clip %.2f, "
                    "Two Sided (Alphakanal ist immer linear)" % OPACITY_CLIP),
    ]
    if normal_datei:
        eintraege.append(dict(gemeinsam, datei=normal_datei, rolle="Normal", kanaele="RGB", srgb=False,
                              konvention="OpenGL (+Y); in Unreal flip_green_channel=True setzen",
                              unreal="Compression Normalmap, sRGB aus"))
    return eintraege


def _flaeche_nach_vorne(bm, flaechen):
    """Dreht die Flächen so, dass ihre Normale nach -Y (Kamera "vorne") zeigt."""
    bm.normal_update()
    falsch = [f for f in flaechen if f.normal.y > 0.0]
    if falsch:
        bmesh.ops.reverse_faces(bm, faces=falsch, flip_multires=False)
        bm.normal_update()


# ---------------------------------------------------------------------------------------------------------------------
# (a) Platzhalter
# ---------------------------------------------------------------------------------------------------------------------

def saiten_textur(ordner, groesse=64, staerke=6):
    """Kachelbare Saiten-Textur (RGBA): eine Längssaite am linken, eine Quersaite am unteren Rand.

    RGB trägt überall die Saitenfarbe (auch wo Alpha 0 ist), damit Filterung und Mip-Maps keinen dunklen Rand
    erzeugen; Alpha = Bedeckung."""
    px = np.zeros((groesse, groesse, 4), dtype=np.float32)
    farbe = np.array(SAITEN_FARBE_SRGB, dtype=np.float32)
    profil = np.zeros(groesse, dtype=np.float32)
    for i in range(staerke):
        profil[i] = 0.75 + 0.25 * math.sin(math.pi * (i + 0.5) / staerke)  # leichte Rundung
    hell = np.maximum(profil[None, :], profil[:, None] * 0.97)  # [y, x]
    bedeckt = hell > 0.0
    px[..., :3] = farbe * np.where(bedeckt, hell, 0.9)[..., None]
    px[..., 3] = bedeckt.astype(np.float32)
    pfad = Path(ordner) / "T_SchlaegerPlatzhalter_Saiten_BaseColor.png"
    bild = bild_speichern(pfad.stem, px, pfad, srgb=True)
    return bild, pfad


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
    rahmen_mat = mb.material_grundfarbe("M_SchlaegerPlatzhalter_Rahmen", (0.10, 0.14, 0.24), rauheit=0.32)
    griff_mat = mb.material_grundfarbe("M_SchlaegerPlatzhalter_Griff", (0.06, 0.06, 0.06), rauheit=0.85)
    bild, textur_pfad = saiten_textur(ordner)
    saiten_mat = saiten_material("M_SchlaegerPlatzhalter_Saiten", bild, kachelnd=True,
                                 hinweis="Alpha-Maske kachelnd, 1 UV-Einheit = 1 Saitenfeld")

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

    saiten_flaechen = []
    for k in range(n):
        f = bm.faces.new((mitte, rand[k], rand[(k + 1) % n]))
        f.material_index = 2
        for loop in f.loops:
            loop[uv].uv = saiten_uv(loop.vert.co)
        saiten_flaechen.append(f)
    _flaeche_nach_vorne(bm, saiten_flaechen)  # Normale nach -Y (Kamera "vorne"), wie beim eigenen Modell

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


# ---------------------------------------------------------------------------------------------------------------------
# (c) Saitenbett des eigenen Modells als Alpha-Fläche (README 3.4)
# ---------------------------------------------------------------------------------------------------------------------

#: Schwellen der Saitenerkennung in Metern (nach dem Skalieren auf 27 Zoll); Begründung in saitenteile_finden()
RAHMEN_MIN_LAENGE = 0.10        # Rahmenteil: größte Ausdehnung über 10 cm ...
RAHMEN_MIN_DICKE = 0.008        # ... und quer zur Saitenebene dicker als 8 mm
SAITE_MAX_DICKE = 0.006         # Saitenteil: quer zur Saitenebene dünner als 6 mm ...
SAITE_MAX_ABSTAND = 0.003       # ... Mitte höchstens 3 mm neben der Saitenebene ...
INNEN_TOLERANZ = 0.0005         # ... und mindestens ein Punkt 0,5 mm innerhalb der Innenkontur des Rahmens
SAITE_MIN_LAENGE = 0.10         # (nur Protokoll) lange Saitenteile = einzelne Längs-/Quersaiten
SEKTOREN = 360                  # Innenkontur: 1 Strahl je Grad um die Kopfmitte
KONTUR_TIEFEN = (-0.008, -0.006, -0.004, -0.002, 0.0, 0.002, 0.004, 0.006, 0.008)  # Strahlebenen um die Saitenebene
WAND_TIEFE = 0.004              # Innenwand für den Flächenrand: Minimum der Ebenen bis +-4 mm (Ösenlöcher überbrückt)
FLAECHE_ECKEN = 120             # Randpunkte der Saitenfläche (3-Grad-Schritte)
FLAECHE_UEBERSTAND = 0.0025     # Saitenfläche ragt 2,5 mm in den Rahmen hinein (dort verdeckt, keine Lücke) ...
FLAECHE_ABSTAND_AUSSEN = 0.003  # ... bleibt aber mindestens 3 mm hinter der Außenseite des Rahmens
BACKEN_ABSTAND = 0.006          # Strahlen starten 6 mm vor der Saitenebene (Saiten liegen in +-3 mm) ...
BACKEN_REICHWEITE = 0.014       # ... und enden 8 mm dahinter


def _inseln(me):
    """Zusammenhängende Teile (über Kanten verbundene Punkte): (Inselnummer je Punkt, Anzahl Inseln)."""
    nv = len(me.vertices)
    kanten = np.empty(len(me.edges) * 2, dtype=np.int64)
    me.edges.foreach_get("vertices", kanten)
    kanten = kanten.reshape(-1, 2)
    lab = np.arange(nv)
    while True:  # Label-Propagation mit Pointer-Jumping: jede Insel endet beim kleinsten Punktindex
        a = np.minimum(lab[kanten[:, 0]], lab[kanten[:, 1]])
        alt = lab.copy()
        np.minimum.at(lab, kanten[:, 0], a)
        np.minimum.at(lab, kanten[:, 1], a)
        lab = lab[lab]
        if np.array_equal(lab, alt):
            break
    _, insel = np.unique(lab, return_inverse=True)
    return insel, (int(insel.max()) + 1 if nv else 0)


def _polygon_insel(me, insel_je_punkt):
    start = np.empty(len(me.polygons), dtype=np.int64)
    me.polygons.foreach_get("loop_start", start)
    anzahl = np.empty(len(me.polygons), dtype=np.int64)
    me.polygons.foreach_get("loop_total", anzahl)
    ecken = np.empty(len(me.loops), dtype=np.int64)
    me.loops.foreach_get("vertex_index", ecken)
    return insel_je_punkt[ecken[start]], anzahl - 2


def _sektor(x, z, mitte, sektoren):
    w = np.arctan2(z - mitte[1], x - mitte[0])
    return ((w + math.pi) / (2.0 * math.pi) * sektoren).astype(np.int64) % sektoren, np.hypot(x - mitte[0], z - mitte[1])


def _sektor_winkel(k):
    return -math.pi + (k + 0.5) * 2.0 * math.pi / SEKTOREN


def _rahmen_kontur(co, me, rahmen_polygone, mitte, y0):
    """Innen- und Außenkontur des Rahmens in der Saitenebene per Strahl gegen die Rahmenflächen (BVH).

    Die Punkte des Rahmens liegen an den Kopfseiten mehrere Grad auseinander - ein Minimum über Punkte je Sektor
    wäre dort zu groß. Je Grad ein Strahl von der Kopfmitte nach außen, auf mehreren Ebenen parallel zur
    Saitenebene (KONTUR_TIEFEN); dazu ein Strahl von außen nach innen auf der Saitenebene.
    Rückgabe (je Sektor, Meter): sichtbar = kleinster Radius über alle Ebenen (Rand der Öffnung von vorne),
    wand = kleinster Radius bis +-WAND_TIEFE, aussen = Außenseite des Rahmens auf der Saitenebene."""
    from mathutils.bvhtree import BVHTree
    punkte = [tuple(p) for p in co]
    polys = [tuple(p.vertices) for p, r in zip(me.polygons, rahmen_polygone) if r]
    bvh = BVHTree.FromPolygons(punkte, polys, all_triangles=False)
    xc, zc = mitte
    sichtbar = np.full(SEKTOREN, np.inf)
    wand = np.full(SEKTOREN, np.inf)
    aussen = np.full(SEKTOREN, np.inf)
    fern = 1.0
    for k in range(SEKTOREN):
        w = _sektor_winkel(k)
        richtung = Vector((math.cos(w), 0.0, math.sin(w)))
        for dy in KONTUR_TIEFEN:
            treffer = bvh.ray_cast(Vector((xc, y0 + dy, zc)), richtung, fern)
            if treffer[0] is not None:
                sichtbar[k] = min(sichtbar[k], treffer[3])
                if abs(dy) <= WAND_TIEFE + 1e-9:
                    wand[k] = min(wand[k], treffer[3])
        for dy in (-0.001, 0.0, 0.001):
            start = Vector((xc, y0 + dy, zc)) + richtung * fern
            treffer = bvh.ray_cast(start, -richtung, fern)
            if treffer[0] is not None:
                r = fern - treffer[3]
                if r > 0.0:
                    aussen[k] = r if not np.isfinite(aussen[k]) else max(aussen[k], r)
    idx = np.arange(SEKTOREN)
    for werte in (sichtbar, wand, aussen):
        ok = np.isfinite(werte)
        if ok.sum() < SEKTOREN // 2:
            return None
        werte[:] = np.interp(idx, idx[ok], werte[ok], period=SEKTOREN)
    # einzelne Ausreißer (Strahl durch eine Naht zwischen zwei Rahmenteilen) per zyklischem 3er-Median glätten
    for werte in (sichtbar, wand):
        werte[:] = np.median(np.stack([np.roll(werte, -1), werte, np.roll(werte, 1)]), axis=0)
    return sichtbar, wand, aussen


def saitenteile_finden(obj):
    """Findet die Saitengeometrie des eigenen Modells. Rückgabe: dict (Polygonmaske, Ebene, Kontur,
    Protokoll) oder None, wenn das Modell keine Saiten als Geometrie enthält.

    Kriterium - am Modell aus Vorgaben/racket.js geprüft (1346 Inseln, 115 912 Dreiecke, ein einziges Material):
      Das Modell besteht aus zusammenhängenden Einzelteilen (Inseln). Nach ausrichten() liegt das Saitenbett in
      der Ebene y = y0 (Normale Y, Kopf oben).
      1. Rahmen = Inseln mit größter Ausdehnung > 10 cm UND Dicke quer zur Saitenebene > 8 mm: Rahmenhälften vorne/
         hinten, Innenwand mit den Saitenlöchern, Herzstücke, Steg, Griff (10 Inseln, 10-39 mm dick).
      2. Kandidaten = alle anderen Inseln, die dünn sind (Y-Dicke < 6 mm; Saiten sind wegen der Verflechtung
         2,6-4,5 mm "dick", Ösenteile <= 4 mm) und deren Mitte höchstens 3 mm neben der Saitenebene liegt. Die
         Griff-Endkappe (36 mm dick) fällt heraus. y0 = Median der Mitten aller langen dünnen Inseln.
      3. Innenkontur des Kopfes: je Grad ein Strahl von der Kopfmitte gegen die Rahmenflächen, auf Ebenen bis
         8 mm vor und hinter der Saitenebene; der kürzeste Treffer ist der von vorne sichtbare Rand der Öffnung
         (unten schließt der Steg sie ab). Siehe _rahmen_kontur().
      4. Saitenteil = Kandidat mit mindestens einem Punkt 0,5 mm innerhalb der Innenkontur, also alles, was von
         vorne in der Öffnung zu sehen ist: 16 Längs- und 19 Quersaiten (38 Inseln mit 140-154 Punkten, 8 796
         Dreiecke) und die Ösenköpfe an der Innenwand (rund 820 Kleinteile, rund 23 200 Dreiecke). Die Saiten-
         schlaufen in der äußeren Rahmenrille, die Ösenenden außen und bündig in der Wand sitzende Ösenteile
         (rund 480 Kleinteile, rund 8 500 Dreiecke) liegen nicht über der Öffnung und bleiben Geometrie.
      Material oder UV-Bereich taugen hier nicht als Kriterium: das Modell hat nur ein Material ("racket"), und
      die UV-Inseln der Saiten liegen über den ganzen Atlas verstreut zwischen den Rahmenteilen (u 0,2-1,0)."""
    me = obj.data
    if len(me.vertices) == 0:
        return None
    co = np.empty(len(me.vertices) * 3, dtype=np.float64)
    me.vertices.foreach_get("co", co)
    co = co.reshape(-1, 3)
    insel, anzahl = _inseln(me)
    lo = np.full((anzahl, 3), np.inf)
    hi = np.full((anzahl, 3), -np.inf)
    np.minimum.at(lo, insel, co)
    np.maximum.at(hi, insel, co)
    ausd = hi - lo
    mitte = (lo + hi) * 0.5
    laenge = ausd.max(axis=1)
    poly_insel, poly_dreiecke = _polygon_insel(me, insel)
    dreiecke_je_insel = np.bincount(poly_insel, weights=poly_dreiecke, minlength=anzahl)

    rahmen = (laenge > RAHMEN_MIN_LAENGE) & (ausd[:, 1] > RAHMEN_MIN_DICKE)
    duenn = ausd[:, 1] < SAITE_MAX_DICKE
    lang_duenn = duenn & (laenge > SAITE_MIN_LAENGE) & ~rahmen
    if lang_duenn.sum() < 8 or rahmen.sum() == 0:
        mb.log("Saitenerkennung: keine Saiten als Geometrie gefunden (%d lange duenne Teile) - Modell bleibt, "
               "wie es ist" % int(lang_duenn.sum()))
        return None
    y0 = float(np.median(mitte[lang_duenn, 1]))
    kopf_lo = lo[lang_duenn].min(axis=0)
    kopf_hi = hi[lang_duenn].max(axis=0)
    kopf_mitte = (float((kopf_lo[0] + kopf_hi[0]) * 0.5), float((kopf_lo[2] + kopf_hi[2]) * 0.5))
    kandidat = duenn & ~rahmen & (np.abs(mitte[:, 1] - y0) < SAITE_MAX_ABSTAND)

    # Innenkontur des Kopfes (Strahlen gegen die Rahmenflächen)
    kontur = _rahmen_kontur(co, me, rahmen[poly_insel], kopf_mitte, y0)
    if kontur is None:
        mb.log("Saitenerkennung: Rahmen umschliesst den Kopf nicht - Modell bleibt, wie es ist")
        return None
    sichtbar, wand, aussen = kontur

    w_v = np.arctan2(co[:, 2] - kopf_mitte[1], co[:, 0] - kopf_mitte[0])
    r_v = np.hypot(co[:, 0] - kopf_mitte[0], co[:, 2] - kopf_mitte[1])
    sek_w = np.array([_sektor_winkel(k) for k in range(SEKTOREN)])
    grenze_v = np.interp(w_v, sek_w, sichtbar, period=2.0 * math.pi)
    innen_v = r_v < grenze_v - INNEN_TOLERANZ
    insel_innen = np.bincount(insel, weights=innen_v.astype(np.float64), minlength=anzahl) > 0
    saite = kandidat & insel_innen
    lange = saite & (laenge > SAITE_MIN_LAENGE)
    klein = saite & ~lange
    bleibt = kandidat & ~saite

    # Rand der Saitenfläche: FLAECHE_ECKEN Punkte; Radius = größte Innenwand im Umfeld + Überstand, höchstens bis
    # FLAECHE_ABSTAND_AUSSEN vor die Außenseite (die Fläche endet also verdeckt im Rahmen)
    schritt = SEKTOREN // FLAECHE_ECKEN
    winkel, radien = [], []
    gekappt = 0
    for k in range(FLAECHE_ECKEN):
        mitte_k = k * schritt + (schritt - 1) * 0.5
        umfeld = [(k * schritt + d) % SEKTOREN for d in range(-1, schritt + 1)]
        winkel.append(-math.pi + (mitte_k + 0.5) * 2.0 * math.pi / SEKTOREN)
        r = float(wand[umfeld].max()) + FLAECHE_UEBERSTAND
        r_max = float(aussen[umfeld].min()) - FLAECHE_ABSTAND_AUSSEN
        if r > r_max:
            r = max(r_max, float(wand[umfeld].max()))
            gekappt += 1
        radien.append(r)

    def _summe(maske):
        return {"inseln": int(maske.sum()), "dreiecke": int(dreiecke_je_insel[maske].sum())}

    protokoll = {
        "kriterium": ("Inseln mit Y-Dicke < %.0f mm, Mitte <= %.0f mm neben der Saitenebene, mindestens ein Punkt "
                      "%.1f mm innerhalb der Innenkontur des Rahmens (Rahmen = Inseln > %.0f cm lang und > %.0f mm "
                      "dick)" % (SAITE_MAX_DICKE * 1e3, SAITE_MAX_ABSTAND * 1e3, INNEN_TOLERANZ * 1e3,
                                 RAHMEN_MIN_LAENGE * 1e2, RAHMEN_MIN_DICKE * 1e3)),
        "inseln_gesamt": int(anzahl), "rahmen": _summe(rahmen), "saitenteile": _summe(saite),
        "davon_saiten": _summe(lange), "davon_oesen_kleinteile": _summe(klein),
        "duenne_teile_aussen_bleiben": _summe(bleibt),
        "saitenebene_y_m": round(y0, 5), "kopfmitte_xz_m": [round(v, 5) for v in kopf_mitte],
        "innenkontur_halbachsen_m": [round(float(sichtbar[[0, SEKTOREN // 2]].mean()), 4),
                                     round(float(sichtbar[[SEKTOREN // 4, 3 * SEKTOREN // 4]].mean()), 4)],
        "flaechenrand_an_aussenseite_gekappt": gekappt,
    }
    mb.log("Saitenerkennung: %d Inseln, davon %d Saiten (%d Dreiecke) + %d Oesen/Kleinteile (%d Dreiecke); "
           "%d duenne Teile ausserhalb bleiben" % (anzahl, protokoll["davon_saiten"]["inseln"],
                                                   protokoll["davon_saiten"]["dreiecke"],
                                                   protokoll["davon_oesen_kleinteile"]["inseln"],
                                                   protokoll["davon_oesen_kleinteile"]["dreiecke"],
                                                   protokoll["duenne_teile_aussen_bleiben"]["inseln"]))
    return {"poly_maske": saite[poly_insel], "ebene_y": y0, "kopf_mitte": kopf_mitte,
            "rand_winkel": winkel, "rand_radien": radien, "protokoll": protokoll}


def polygone_abtrennen(obj, maske, name):
    """Verschiebt die Polygone mit maske=True in ein neues Objekt (bmesh; eigene Normalen und UVs bleiben)."""
    neu_me = obj.data.copy()
    neu_me.name = name
    for me, behalten in ((obj.data, ~maske), (neu_me, maske)):
        bm = bmesh.new()
        bm.from_mesh(me)
        bm.faces.ensure_lookup_table()
        weg = [f for f in bm.faces if not behalten[f.index]]
        bmesh.ops.delete(bm, geom=weg, context='FACES')  # entfernt auch frei gewordene Kanten und Punkte
        bm.to_mesh(me)
        bm.free()
        me.update()
    neu = bpy.data.objects.new(name, neu_me)
    return mb.verknuepfen(neu)


def saitenflaeche_bauen(name, erkennung, uv_name, rand_uv):
    """Ebene Saitenfläche (Fächer um die Kopfmitte) in der Saitenebene, Normale -Y.

    UV: Projektion auf X/Z, Umriss füllt 0..1 bis auf rand_uv (U = +X, V = +Z, von vorne ungespiegelt)."""
    xc, zc = erkennung["kopf_mitte"]
    y0 = erkennung["ebene_y"]
    punkte = [(xc + r * math.cos(w), zc + r * math.sin(w))
              for w, r in zip(erkennung["rand_winkel"], erkennung["rand_radien"])]
    xs = [p[0] for p in punkte]
    zs = [p[1] for p in punkte]
    x0, x1, z0, z1 = min(xs), max(xs), min(zs), max(zs)

    def uv_von(x, z):
        return (rand_uv + (1.0 - 2.0 * rand_uv) * (x - x0) / (x1 - x0),
                rand_uv + (1.0 - 2.0 * rand_uv) * (z - z0) / (z1 - z0))

    bm = bmesh.new()
    uv = bm.loops.layers.uv.new(uv_name)
    mitte = bm.verts.new((xc, y0, zc))
    ring = [bm.verts.new((x, y0, z)) for x, z in punkte]
    flaechen = []
    for k in range(len(ring)):
        f = bm.faces.new((mitte, ring[k], ring[(k + 1) % len(ring)]))
        f.smooth = False
        for loop in f.loops:
            loop[uv].uv = uv_von(loop.vert.co.x, loop.vert.co.z)
        flaechen.append(f)
    _flaeche_nach_vorne(bm, flaechen)
    obj = mb.objekt_aus_bmesh(name, bm, [])
    groesse = (x1 - x0, z1 - z0)
    return obj, {"breite_m": round(groesse[0], 4), "hoehe_m": round(groesse[1], 4), "randpunkte": len(ring),
                 "dreiecke": len(ring)}


def _emission_material(name, quelle=None):
    """Bake-Hilfsmaterial: Emission = Base Color von quelle (Kopie, Texturknoten bleiben) bzw. weiß."""
    if quelle is not None and quelle.node_tree is not None:
        mat = quelle.copy()
        mat.name = name
        baum = mat.node_tree
        bsdf = next((n for n in baum.nodes if n.type == 'BSDF_PRINCIPLED'), None)
    else:
        mat = bpy.data.materials.new(name)
        baum = mb._baum(mat)
        bsdf = None
    ausgabe = next((n for n in baum.nodes if n.type == 'OUTPUT_MATERIAL'), None) or \
        baum.nodes.new("ShaderNodeOutputMaterial")
    emission = baum.nodes.new("ShaderNodeEmission")
    emission.inputs["Strength"].default_value = 1.0
    emission.inputs["Color"].default_value = (1.0, 1.0, 1.0, 1.0)
    if bsdf is not None:
        eingang = bsdf.inputs["Base Color"]
        if eingang.links:
            baum.links.new(eingang.links[0].from_socket, emission.inputs["Color"])
        else:
            emission.inputs["Color"].default_value = eingang.default_value
    baum.links.new(emission.outputs["Emission"], ausgabe.inputs["Surface"])
    return mat


def _bild_lesen(bild):
    b, h = bild.size
    px = np.empty(b * h * 4, dtype=np.float32)
    bild.pixels.foreach_get(px)
    return px.reshape(h, b, 4)


def _auffuellen(farbe, gewicht):
    """Push-Pull: füllt Texel ohne Gewicht mit dem Mittel der nächstgelegenen bedeckten Texel (für Filterung und
    Mip-Maps einer maskierten Textur - sonst entstehen dunkle Säume)."""
    ebenen = []
    c = farbe * gewicht[..., None]
    w = gewicht.astype(np.float64)
    while min(w.shape) > 1:
        ebenen.append((c, w))
        h2, b2 = w.shape[0] // 2, w.shape[1] // 2
        c = c[:h2 * 2, :b2 * 2].reshape(h2, 2, b2, 2, -1).sum(axis=(1, 3))
        w = w[:h2 * 2, :b2 * 2].reshape(h2, 2, b2, 2).sum(axis=(1, 3))
    erg = c / np.maximum(w, 1e-12)[..., None]
    for c, w in reversed(ebenen):
        hoch = np.repeat(np.repeat(erg, 2, axis=0), 2, axis=1)
        hoch = np.pad(hoch, ((0, w.shape[0] - hoch.shape[0]), (0, w.shape[1] - hoch.shape[1]), (0, 0)), mode='edge')
        eigen = c / np.maximum(w, 1e-12)[..., None]
        a = np.clip(w, 0.0, 1.0)[..., None]
        erg = eigen * a + hoch * (1.0 - a)
    return erg


def saiten_backen(flaeche, saiten, ordner, aufloesung, samples=4):
    """Backt die Saiten (saiten, selected) mit Cycles auf die Saitenfläche (active).

    Drei Durchgänge mit derselben Strahlgeometrie (Start 6 mm vor der Ebene auf der Vorderseite, Richtung -Normale):
      Bedeckung: Saiten leuchten weiß -> 1 wo ein Strahl trifft, 0 sonst (binär je Texel, also scharfe Kanten),
      Farbe:     Emission = Base Color der Saiten (Textur mit Logo) -> sRGB,
      Normale:   Tangentenraum der Fläche, OpenGL (+Y).
    Danach: Farbe unter transparenten Texeln per Push-Pull aufgefüllt, Normale dort flach.
    Rückgabe: (Bild BaseColor RGBA, Bild Normal, Protokoll)."""
    szene = bpy.context.scene
    alt_engine = szene.render.engine
    szene.render.engine = 'CYCLES'
    szene.cycles.device = 'CPU'
    szene.cycles.samples = int(samples)
    bake = szene.render.bake
    bake.margin = 0
    bake.use_clear = True

    hilfe = bpy.data.materials.new("_MB_Backziel")
    baum = mb._baum(hilfe)
    ziel_knoten = baum.nodes.new("ShaderNodeTexImage")
    flaeche.data.materials.clear()
    flaeche.data.materials.append(hilfe)
    original = list(saiten.data.materials)
    weiss = _emission_material("_MB_Backen_Weiss")
    farbe = _emission_material("_MB_Backen_Farbe", quelle=original[0] if original else None)

    def durchgang(typ, name, nicht_farbe, saiten_mat, **zusatz):
        bild = bpy.data.images.new(name, aufloesung, aufloesung, alpha=False, float_buffer=False)
        bild.colorspace_settings.name = 'Non-Color' if nicht_farbe else 'sRGB'
        ziel_knoten.image = bild
        baum.nodes.active = ziel_knoten
        for i in range(len(saiten.data.materials)):
            saiten.data.materials[i] = saiten_mat if saiten_mat is not None else original[i]
        mb.objektmodus()
        vl = bpy.context.view_layer
        for o in vl.objects:
            o.select_set(False)
        for o in (saiten, flaeche):
            o.hide_set(False)
            o.hide_render = False
            o.select_set(True)
        vl.objects.active = flaeche
        mb.log("Backe %s (%d px, Cycles, selected-to-active) ..." % (name, aufloesung))
        with bpy.context.temp_override(active_object=flaeche, object=flaeche, selected_objects=[saiten, flaeche],
                                       selected_editable_objects=[saiten, flaeche]):
            erg = bpy.ops.object.bake(type=typ, use_selected_to_active=True, use_cage=False,
                                      cage_extrusion=BACKEN_ABSTAND, max_ray_distance=BACKEN_REICHWEITE,
                                      margin=0, use_clear=True, target='IMAGE_TEXTURES', **zusatz)
        if 'FINISHED' not in erg:
            raise RuntimeError("Backen fehlgeschlagen: %s" % name)
        px = _bild_lesen(bild)
        bpy.data.images.remove(bild)
        return px

    try:
        bedeckung = durchgang('EMIT', "_MB_Bedeckung", True, weiss)[..., 0] > 0.5
        farbe_px = durchgang('EMIT', "_MB_Farbe", False, farbe)[..., :3]
        normal_px = durchgang('NORMAL', "_MB_Normal", True, None, normal_space='TANGENT',
                              normal_r='POS_X', normal_g='POS_Y', normal_b='POS_Z')[..., :3]
    finally:
        for i, m in enumerate(original):
            saiten.data.materials[i] = m
        flaeche.data.materials.clear()
        for m in (hilfe, weiss, farbe):
            bpy.data.materials.remove(m)
        szene.render.engine = alt_engine

    gewicht = bedeckung.astype(np.float64)
    rgba = np.empty(bedeckung.shape + (4,), dtype=np.float32)
    rgba[..., :3] = np.clip(_auffuellen(farbe_px.astype(np.float64), gewicht), 0.0, 1.0)
    rgba[..., 3] = gewicht
    normal = np.where(bedeckung[..., None], normal_px, np.array([0.5, 0.5, 1.0], dtype=np.float32))

    bild_farbe = bild_speichern("T_Schlaeger_Saiten_BaseColor", rgba, Path(ordner) / "T_Schlaeger_Saiten_BaseColor.png")
    bild_normal = bild_speichern("T_Schlaeger_Saiten_Normal", normal, Path(ordner) / "T_Schlaeger_Saiten_Normal.png",
                                 srgb=False)
    anteil = float(bedeckung.mean())
    mb.log("Saitenbett gebacken: %.1f %% der Texel von Saiten bedeckt" % (100.0 * anteil))
    return bild_farbe, bild_normal, {"aufloesung_px": aufloesung, "samples": int(samples),
                                     "bedeckung_anteil": round(anteil, 4),
                                     "strahl_start_vor_ebene_m": BACKEN_ABSTAND,
                                     "strahl_reichweite_m": BACKEN_REICHWEITE,
                                     "verfahren": "Cycles selected-to-active: EMIT (Bedeckung, Farbe), NORMAL "
                                                  "(Tangentenraum, OpenGL); Farbe per Push-Pull aufgefuellt"}


def saitenbett_backen(obj, ordner, args):
    """Ersetzt die Saitengeometrie von obj durch eine Alpha-Saitenfläche. Rückgabe: dict oder None."""
    dreiecke_vorher = mb.dreiecke(obj)
    erkennung = saitenteile_finden(obj)
    if erkennung is None:
        return None
    uv_name = obj.data.uv_layers.active.name if obj.data.uv_layers.active else "UVMap"
    saiten = polygone_abtrennen(obj, erkennung["poly_maske"], "Quelle_Saitengeometrie")
    flaeche, flaeche_info = saitenflaeche_bauen(ASSET + "_Saitenflaeche", erkennung, uv_name,
                                                rand_uv=4.0 / args.saiten_aufloesung)
    bild_farbe, bild_normal, backen = saiten_backen(flaeche, saiten, ordner, args.saiten_aufloesung,
                                                    samples=args.saiten_samples)
    material = saiten_material("M_Schlaeger_Saiten", bild_farbe, bild_normal,
                               hinweis="Saitenbett des eigenen Modells, gebacken (Logo auf den Saiten)")
    flaeche.data.materials.append(material)
    # Saitengeometrie bleibt nur als ausgeblendete Quelle in der .blend (nicht exportiert, nicht gerendert)
    saiten.hide_render = True
    saiten.hide_set(True)
    texturen = saiten_textur_eintraege(material.name, Path(bild_farbe.filepath_raw).name,
                                       Path(bild_normal.filepath_raw).name, groesse_px=bild_farbe.size)
    protokoll = dict(erkennung["protokoll"])
    protokoll.update({"dreiecke_modell_vorher": dreiecke_vorher, "dreiecke_rahmen": mb.dreiecke(obj),
                      "flaeche": flaeche_info, "backen": backen})
    mb.log("Saiten entfernt: %d -> %d Dreiecke (+ %d fuer die Saitenflaeche)"
           % (dreiecke_vorher, protokoll["dreiecke_rahmen"], flaeche_info["dreiecke"]))
    return {"flaeche": flaeche, "material": material, "texturen": texturen, "protokoll": protokoll,
            "quelle": saiten}


def flaeche_anhaengen(ziel, flaeche):
    """Hängt eine Kopie der Saitenfläche an ziel an (gleiches Material, gleiche UV-Schicht, eigene Normalen bleiben)."""
    kopie = flaeche.copy()
    kopie.data = flaeche.data.copy()
    mb.verknuepfen(kopie)
    name = ziel.name
    return mb.objekte_vereinen([ziel, kopie], name)


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
    protokoll["dreiecke_quelle"] = mb.dreiecke(obj)
    saiten = None if args.saiten_geometrie else saitenbett_backen(obj, ordner, args)
    protokoll["einzelteile"] = _einzelteile(obj.data)
    protokoll["quelle_datei"] = mb.repo_relativ(pfad)
    protokoll["quelle_format"] = "Base64-GLB in JavaScript" if art == "js" else pfad.suffix.lower()
    mb.log("Modell: %d Dreiecke (Quelle %d), %d Einzelteile, Skalierung %.4f, Achsen %s"
           % (mb.dreiecke(obj), protokoll["dreiecke_quelle"], protokoll["einzelteile"], protokoll["skalierung"],
              protokoll["achsen_zuordnung"]))
    return obj, beschreibung, texturen, protokoll, saiten


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

    # (b) eigenes Modell, (c) Saitenbett als Alpha-Fläche
    modell = None if args.nur_platzhalter else modell_aufbereiten(args, ordner)
    stufen, export_modell, masse_modell, fbx_modell, lod_dateien = [], None, None, None, []
    if modell is not None:
        obj, beschreibung, texturen, protokoll, saiten = modell
        if saiten is not None:
            obj.data.materials.append(saiten["material"])  # Slot 1, damit alle LOD-Kopien ihn haben
        # nur der Rahmen wird dezimiert; jede Stufe bekommt danach dieselbe Saitenfläche
        stufen = mb.lods_erzeugen(obj, args.lods, basisname=ASSET, glatt_winkel=40.0)
        if saiten is not None:
            for st in stufen:
                st["objekt"] = flaeche_anhaengen(st["objekt"], saiten["flaeche"])
                st["dreiecke"] = mb.dreiecke(st["objekt"])
                mb.log("LOD%d mit Saitenflaeche: %d Dreiecke" % (st["stufe"], st["dreiecke"]))
            obj = stufen[0]["objekt"]
            saiten["flaeche"].hide_render = True
            saiten["flaeche"].hide_set(True)
            saiten["protokoll"]["dreiecke_lod0_nachher"] = stufen[0]["dreiecke"]
            modell = (obj, beschreibung, texturen, protokoll, saiten)
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
        dateien={"fbx": fbx_platz.name,
                 "texturen": saiten_textur_eintraege("M_SchlaegerPlatzhalter_Saiten", saiten_pfad.name,
                                                     groesse_px=(64, 64), kachelnd=True),
                 "lods": [], "blend": blend.name if blend else None,
                 "vorschau": [mb.relativ_zur_ausgabe(args, p) for p in vorschau_platz]},
        materialien=mb.materialien_info([platz]), dreiecke=mb.dreiecke(platz), masse=masse_platz,
        ausrichtung=achsen_text,
        saiten=_saiten_hinweis("M_SchlaegerPlatzhalter_Saiten", saiten_pfad.name, None,
                               "Saitenflaeche im Platzhalter (kachelnde Textur, 1 UV-Einheit = 1 Saitenfeld)"),
        export=export_platz)

    if modell is not None:
        obj, beschreibung, texturen, protokoll, saiten = modell
        if saiten is not None:
            texturen = texturen + saiten["texturen"]
            protokoll["saitenbett"] = saiten["protokoll"]
            saiten_text = _saiten_hinweis(
                "M_Schlaeger_Saiten", "T_Schlaeger_Saiten_BaseColor.png", "T_Schlaeger_Saiten_Normal.png",
                "Saitengeometrie (Saiten und Oesenkoepfe) auf eine ebene Saitenflaeche gebacken und entfernt; das "
                "Logo steht auf den Saiten und ist in der BaseColor enthalten. Alle LOD-Stufen nutzen dieselbe "
                "Flaeche (Materialslot 1); Dreiecke LOD0 %d -> %d"
                % (saiten["protokoll"]["dreiecke_modell_vorher"], saiten["protokoll"]["dreiecke_lod0_nachher"]))
        else:
            saiten_text = "als Geometrie im Modell enthalten (--saiten-geometrie oder keine Saiten erkannt)"
        mb.meta_schreiben(
            ordner, ASSET, "static", quelle=beschreibung, lizenz=lizenz_eigen, unreal_ziel=UNREAL_ZIEL,
            parameter={"laenge_m": args.laenge, "lod_verhaeltnisse": list(args.lods),
                       "texturen_eingebettet": bool(args.texturen_einbetten), "aufbereitung": protokoll},
            dateien={"fbx": fbx_modell.name, "lods": lod_dateien, "texturen": texturen,
                     "blend": blend.name if blend else None,
                     "vorschau": [mb.relativ_zur_ausgabe(args, p) for p in vorschau_modell]},
            materialien=mb.materialien_info([obj]), dreiecke=stufen[0]["dreiecke"], masse=masse_modell,
            ausrichtung=achsen_text, zweiseitig=True, saiten=saiten_text, export=export_modell)


def _saiten_hinweis(material, farbe_datei, normal_datei, beschreibung):
    """Gleicher meta.json-Block 'saiten' für Platzhalter und eigenes Modell (Hinweis für den Unreal-Import)."""
    erg = {"beschreibung": beschreibung, "material": material, "basecolor": farbe_datei,
           "unreal": {"blend_mode": "Masked", "opacity_mask": "Alphakanal von %s" % farbe_datei,
                      "opacity_mask_clip_value": OPACITY_CLIP, "two_sided": True,
                      "hinweis": "Opacity Mask (maskiert, kein Translucent): Alpha der BaseColor an Opacity Mask"}}
    if normal_datei:
        erg["normal"] = normal_datei
        erg["unreal"]["normal"] = "%s an Normal (flip_green_channel=True)" % normal_datei
    return erg


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
    p.add_argument("--saiten-geometrie", action="store_true",
                   help="Saiten des eigenen Modells als Geometrie behalten (kein Backen auf die Alpha-Saitenflaeche)")
    p.add_argument("--saiten-aufloesung", type=_zweierpotenz, default=2048,
                   help="Kantenlaenge der gebackenen Saitentexturen in Pixel (Zweierpotenz, Standard 2048)")
    p.add_argument("--saiten-samples", type=int, default=4, help="Cycles-Samples beim Backen der Saiten (Standard 4)")
    return p


def _zweierpotenz(text):
    wert = int(text)
    if wert < 64 or wert & (wert - 1):
        raise ValueError("Aufloesung muss eine Zweierpotenz >= 64 sein: %r" % text)
    return wert


if __name__ == "__main__":
    mb.ausfuehren(main, parser())
