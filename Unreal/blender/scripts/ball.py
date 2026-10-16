# -*- coding: utf-8 -*-
"""
ball.py - Tennisball, neu modelliert (README 3.4, 24.2; Meilenstein M0).

Erzeugt:
  _erzeugt/Equipment/Ball/SM_Ball.fbx               LOD0 (mit Formschlüssel "gestaucht")
  _erzeugt/Equipment/Ball/SM_Ball_LOD1..3.fbx       weitere LOD-Stufen (Decimate 0.5 / 0.25 / 0.12)
  _erzeugt/Equipment/Ball/T_Ball_Fussel_Maske.png   Opazitätsmaske der Fussel-Hülle (8 Bit grau, linear)
  _erzeugt/Equipment/Ball/SM_Ball.blend
  _erzeugt/Equipment/Ball/SM_Ball.meta.json         Ziel in Unreal: /Game/Equipment/Ball
  _vorschau/SM_Ball_{vorne,seitlich,dreiviertel,nah}.png  und SM_Ball_gestaucht.png

Aufruf:
  blender.exe -b --factory-startup --python-exit-code 1 -P Unreal\\blender\\scripts\\ball.py -- [--durchmesser 0.067]
  python Unreal/blender/scripts/ball.py --ausgabe <ordner> --engine CYCLES        (bpy als Modul)

Aufbau:
  * Kugel als "Quad-Sphere" (Würfel mit gleichwinkliger Projektion auf die Kugel): gleichmäßige Vierecke ohne
    Pole, UVs als 3x2-Atlas (eine Kachel je Würfelseite). Durchmesser 6,7 cm (ITF: 6,54 bis 6,86 cm).
  * Naht als Geometrie: die klassische Tennisball-Kurve auf der Einheitskugel
        x = a·cos t + b·cos 3t,  y = a·sin t − b·sin 3t,  z = 2·sqrt(a·b)·sin 2t   mit a + b = 1
    (liegt für jedes b exakt auf der Kugel und teilt sie in zwei gleiche "Hundeknochen"). Entlang der Kurve
    läuft ein dünnes Rohr (eigenes Material M_Ball_Naht), das in einer flachen Rille im Filz liegt.
  * Filz-Material M_Ball_Filz (Optic Yellow) mit prozeduralem Rauschen für Fussel und "Sheen" für den
    hellen Fussel-Rand. In Unreal baut das Import-Skript ein eigenes Material; meta.json enthält die Werte.
  * Fussel-Rand (README 3.4, "Shell-Fur oder Opazitäts-Rand"): eine zweite Hülle 0,7 mm über dem Filz im
    selben Mesh, Materialslot 2 M_Ball_Fussel (Blend Mode Masked, Opacity Mask = T_Ball_Fussel_Maske, zweiseitig).
    Die Maske ist selbst erzeugtes 3D-Rauschen (Fusselflecken von 0,1-0,2 mm), entlang der Naht auf 0 (dort ist
    Gummi, kein Filz). Begründung der Wahl:
      - Eine Hülle funktioniert in Unreal ohne Sondertechnik: normales Static Mesh, maskiertes Material, kein
        Translucency-Sortieren, keine Vertex-Farben, kein Niagara. Ein reiner Opazitäts-Rand (Fresnel, transluzent)
        bräuchte ein durchscheinendes Material oder eine zweite Ballgeometrie und liefert keine echte Kontur-
        Unschärfe; mehrere Shells wären dichter, kosten aber je Lage ein weiteres Mal die Hülle.
      - Die Hülle ist eine eigene, gröbere Quad-Sphere (LOD0 16 statt 32 Vierecke je Würfelkante = 3 072
        Dreiecke, LOD1 12 = 1 728; Abweichung der Sehnen von der Kugel unter 0,1 mm): Sie braucht weder die Rille
        noch die Naht-Details und kostet so nur +17 % auf LOD0. Die LODs werden aus Filz + Naht dezimiert (wie
        ohne Hülle) und bekommen die Hülle danach angehängt - sie verbraucht kein Decimate-Budget.
      - Nur LOD0 und LOD1 tragen die Hülle (--fussel-lods 2): Ab LOD2 ist der Ball wenige Pixel groß, ein
        0,7-mm-Rand wäre unsichtbar und kostet nur Überzeichnung (Masked = zweiter Durchgang durch die Fläche).
    Die Hülle gehört zum selben Mesh - der Formschlüssel "gestaucht" und jede Skalierung der Ball-Komponente
    nehmen sie automatisch mit.
  * Formschlüssel "gestaucht": entlang lokal Z um 30 % gestaucht, in X/Y um 1/sqrt(0,7) = 1,195 verbreitert
    (Volumen bleibt gleich). Unreal-Static-Meshes kennen keine Morph Targets - da die Form eine reine
    Ellipsoid-Skalierung ist, erzeugt das Spiel sie durch Skalieren der Ball-Komponente entlang der Stoß-
    normalen (Z 0,70, quer 1,195). Wer den Formschlüssel direkt nutzen will, importiert als Skeletal Mesh.
  * Maße: Filz 6,7 cm Durchmesser; die Fussel-Hülle macht die Bounds 1,4 mm größer (0,0684 m). Die Ball-
    Komponente deshalb nicht aus den Bounds skalieren - das Mesh ist bereits in cm (Maßstab 1).
"""

import math
import struct
import sys
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "lib"))
import mb_gemeinsam as mb  # noqa: E402

import bpy  # noqa: E402
import bmesh  # noqa: E402
import numpy as np  # noqa: E402  (in Blender enthalten)
from mathutils import Vector  # noqa: E402

ASSET = "SM_Ball"
BEREICH = "Equipment/Ball"
UNREAL_ZIEL = "/Game/Equipment/Ball"

OPTIC_YELLOW_SRGB = (0.80, 0.95, 0.12)
NAHT_SRGB = (0.93, 0.93, 0.89)
FUSSEL_SRGB = (0.84, 0.96, 0.27)   # Fasern etwas heller und weniger gesättigt als der Filz
SLOT_FILZ, SLOT_NAHT, SLOT_FUSSEL = 0, 1, 2
#: Unreal: Opacity Mask Clip Value der Fussel-Hülle (Maske hat weiche 0..1-Kanten, Schwelle in der Mitte)
OPACITY_CLIP = 0.5
UV_RAND = 0.01  # Rand jeder Würfelkachel im 3x2-UV-Atlas

# Würfelseiten: (Normale, U-Achse, V-Achse) mit U x V = Normale (Flächen zeigen nach außen)
SEITEN = (
    ((1, 0, 0), (0, 1, 0), (0, 0, 1)),
    ((-1, 0, 0), (0, -1, 0), (0, 0, 1)),
    ((0, 1, 0), (-1, 0, 0), (0, 0, 1)),
    ((0, -1, 0), (1, 0, 0), (0, 0, 1)),
    ((0, 0, 1), (1, 0, 0), (0, 1, 0)),
    ((0, 0, -1), (1, 0, 0), (0, -1, 0)),
)


def nahtkurve(b, punkte):
    """Punkte (N x 3, Einheitskugel) und Tangenten der Tennisball-Nahtkurve."""
    a = 1.0 - b
    t = np.linspace(0.0, 2.0 * math.pi, punkte, endpoint=False)
    w = 2.0 * math.sqrt(a * b)
    p = np.stack([a * np.cos(t) + b * np.cos(3 * t), a * np.sin(t) - b * np.sin(3 * t), w * np.sin(2 * t)], axis=1)
    d = np.stack([-a * np.sin(t) - 3 * b * np.sin(3 * t), a * np.cos(t) - 3 * b * np.cos(3 * t),
                  2 * w * np.cos(2 * t)], axis=1)
    p /= np.linalg.norm(p, axis=1)[:, None]
    d /= np.linalg.norm(d, axis=1)[:, None]
    return p, d


def kugel_bauen(bm, radius, n, uv, material_index=SLOT_FILZ):
    """Quad-Sphere mit n x n Vierecken je Würfelseite und 3x2-UV-Atlas (Rand UV_RAND je Kachel)."""
    rand = UV_RAND
    neue = []
    for kachel, (nrm, ua, va) in enumerate(SEITEN):
        nrm, ua, va = Vector(nrm), Vector(ua), Vector(va)
        spalte, zeile = kachel % 3, kachel // 3
        gitter = []
        for i in range(n + 1):
            reihe = []
            for j in range(n + 1):
                s = math.tan((-1.0 + 2.0 * i / n) * math.pi / 4.0)
                t = math.tan((-1.0 + 2.0 * j / n) * math.pi / 4.0)
                reihe.append(bm.verts.new((nrm + ua * s + va * t).normalized() * radius))
            gitter.append(reihe)
            neue.extend(reihe)
        for i in range(n):
            for j in range(n):
                f = bm.faces.new((gitter[i][j], gitter[i + 1][j], gitter[i + 1][j + 1], gitter[i][j + 1]))
                f.material_index = material_index
                for loop, (di, dj) in zip(f.loops, ((0, 0), (1, 0), (1, 1), (0, 1))):
                    u = (spalte + rand + (1 - 2 * rand) * (i + di) / n) / 3.0
                    v = (zeile + rand + (1 - 2 * rand) * (j + dj) / n) / 2.0
                    loop[uv].uv = (u, v)
    bmesh.ops.remove_doubles(bm, verts=neue, dist=radius * 1e-5)  # nur die eigenen Punkte verschweißen


def rille_eindruecken(bm, radius, naht_punkte, tiefe, breite):
    """Drückt den Filz entlang der Naht ein (Gauß-Profil): tiefe und breite (1/e-Halbbreite) in Metern."""
    verts = list(bm.verts)
    pos = np.array([v.co[:] for v in verts])
    richt = pos / np.linalg.norm(pos, axis=1)[:, None]
    abstand = np.full(len(verts), np.inf)
    for start in range(0, len(naht_punkte), 256):
        teil = naht_punkte[start:start + 256]
        d = np.linalg.norm(richt[:, None, :] - teil[None, :, :], axis=2).min(axis=1)
        abstand = np.minimum(abstand, d)
    abstand *= radius
    neu_r = radius - tiefe * np.exp(-(abstand / breite) ** 2)
    for v, r, u in zip(verts, neu_r, richt):
        v.co = Vector(u * r)


def nahtrohr_bauen(bm, radius, naht_punkte, tangenten, rohr_radius, versenkt, ring, uv, material_index):
    """Rohr entlang der Naht: Mittellinie bei radius - versenkt, Querschnitt mit 'ring' Ecken."""
    anzahl = len(naht_punkte)
    ringe = []
    for k in range(anzahl):
        u = Vector(naht_punkte[k])
        t = Vector(tangenten[k])
        t = (t - u * t.dot(u)).normalized()
        b = t.cross(u).normalized()
        mitte = u * (radius - versenkt)
        ringe.append([bm.verts.new(mitte + (u * math.cos(2 * math.pi * r / ring)
                                            + b * math.sin(2 * math.pi * r / ring)) * rohr_radius)
                      for r in range(ring)])
    for k in range(anzahl):
        a_ring, b_ring = ringe[k], ringe[(k + 1) % anzahl]
        for r in range(ring):
            r2 = (r + 1) % ring
            f = bm.faces.new((a_ring[r], b_ring[r], b_ring[r2], a_ring[r2]))
            f.material_index = material_index
            for loop, (dk, dr) in zip(f.loops, ((0, 0), (1, 0), (1, 1), (0, 1))):
                loop[uv].uv = ((k + dk) / anzahl * 8.0, (r + dr) / ring)


def filz_material():
    mat = mb.material_grundfarbe("M_Ball_Filz", OPTIC_YELLOW_SRGB, rauheit=0.92,
                                 sheen=0.8, sheen_rauheit=0.45, hinweis="Optic Yellow, Filz mit Sheen und Fussel-Rauschen")
    baum = mat.node_tree
    bsdf = mb.principled(mat)
    for name, wert in (("Sheen Weight", 0.8), ("Sheen", 0.8), ("Sheen Roughness", 0.45)):
        e = mb.eingang(bsdf, name)
        if e is not None:
            e.default_value = wert
    koord = baum.nodes.new("ShaderNodeTexCoord")
    grob = baum.nodes.new("ShaderNodeTexNoise")
    grob.inputs["Scale"].default_value = 320.0
    grob.inputs["Detail"].default_value = 4.0
    fein = baum.nodes.new("ShaderNodeTexNoise")
    fein.inputs["Scale"].default_value = 2600.0
    fein.inputs["Detail"].default_value = 8.0
    rampe = baum.nodes.new("ShaderNodeValToRGB")
    hell = [mb.srgb_zu_linear(c) for c in OPTIC_YELLOW_SRGB]
    rampe.color_ramp.elements[0].position = 0.35
    rampe.color_ramp.elements[0].color = (hell[0] * 0.78, hell[1] * 0.80, hell[2] * 0.70, 1.0)
    rampe.color_ramp.elements[1].position = 0.75
    rampe.color_ramp.elements[1].color = (hell[0] * 1.05, hell[1] * 1.03, hell[2] * 1.10, 1.0)
    relief = baum.nodes.new("ShaderNodeBump")
    relief.inputs["Strength"].default_value = 0.35
    relief.inputs["Distance"].default_value = 0.0004
    baum.links.new(koord.outputs["Object"], grob.inputs["Vector"])
    baum.links.new(koord.outputs["Object"], fein.inputs["Vector"])
    baum.links.new(grob.outputs["Fac"], rampe.inputs["Fac"])
    baum.links.new(rampe.outputs["Color"], bsdf.inputs["Base Color"])
    baum.links.new(fein.outputs["Fac"], relief.inputs["Height"])
    baum.links.new(relief.outputs["Normal"], bsdf.inputs["Normal"])
    return mat


# ---------------------------------------------------------------------------------------------------------------------
# Fussel-Hülle (Shell) mit Opazitätsmaske
# ---------------------------------------------------------------------------------------------------------------------

def _hash01(ix, iy, iz, saat):
    """Deterministischer Gitter-Hash -> [0, 1) (SplitMix-artig, numpy uint64 mit Überlauf)."""
    h = (ix.astype(np.uint64) * np.uint64(0x9E3779B185EBCA87)) ^ \
        (iy.astype(np.uint64) * np.uint64(0xC2B2AE3D27D4EB4F)) ^ \
        (iz.astype(np.uint64) * np.uint64(0x165667B19E3779F9)) ^ np.uint64(saat)
    for faktor in (0xFF51AFD7ED558CCD, 0xC4CEB9FE1A85EC53):
        h ^= h >> np.uint64(33)
        h *= np.uint64(faktor)
    h ^= h >> np.uint64(33)
    return (h >> np.uint64(11)).astype(np.float64) / float(1 << 53)


def wertrauschen(p, zelle, saat):
    """3D-Wertrauschen (Value Noise) mit Zellgröße zelle (m) an den Punkten p (N x 3), Werte 0..1.

    Im Raum statt in UV berechnet - dadurch nahtlos über die Kachelgrenzen des UV-Atlas hinweg."""
    q = p / zelle
    basis = np.floor(q)
    f = q - basis
    f = f * f * (3.0 - 2.0 * f)
    basis = basis.astype(np.int64)
    erg = np.zeros(len(p))
    for dx in (0, 1):
        for dy in (0, 1):
            for dz in (0, 1):
                w = (f[:, 0] if dx else 1.0 - f[:, 0]) * (f[:, 1] if dy else 1.0 - f[:, 1]) * \
                    (f[:, 2] if dz else 1.0 - f[:, 2])
                erg += w * _hash01(basis[:, 0] + dx, basis[:, 1] + dy, basis[:, 2] + dz, saat)
    return erg


def atlas_richtungen(breite, hoehe):
    """Kugelrichtung (H x W x 3) jedes Texels des 3x2-Würfelatlas (Umkehrung von kugel_bauen)."""
    u = (np.arange(breite) + 0.5) / breite
    v = (np.arange(hoehe) + 0.5) / hoehe
    uu, vv = np.meshgrid(u, v)
    spalte = np.clip(np.floor(uu * 3.0), 0, 2)
    zeile = np.clip(np.floor(vv * 2.0), 0, 1)
    kachel = (zeile * 3 + spalte).astype(np.int64)
    fi = np.clip((uu * 3.0 - spalte - UV_RAND) / (1.0 - 2.0 * UV_RAND), 0.0, 1.0)
    fj = np.clip((vv * 2.0 - zeile - UV_RAND) / (1.0 - 2.0 * UV_RAND), 0.0, 1.0)
    s = np.tan((-1.0 + 2.0 * fi) * math.pi / 4.0)
    t = np.tan((-1.0 + 2.0 * fj) * math.pi / 4.0)
    nrm = np.array([x[0] for x in SEITEN], dtype=np.float64)[kachel]
    ua = np.array([x[1] for x in SEITEN], dtype=np.float64)[kachel]
    va = np.array([x[2] for x in SEITEN], dtype=np.float64)[kachel]
    d = nrm + ua * s[..., None] + va * t[..., None]
    return d / np.linalg.norm(d, axis=2)[..., None]


def fussel_maske(args, radius):
    """Opazitätsmaske der Fussel-Hülle im UV-Atlas der Hülle: Werte 0..1 (H x W, unterste Zeile zuerst).

    Fusselflecken = zwei Lagen 3D-Wertrauschen (Zellen 0,16 mm und 0,07 mm), so geschwellt, dass
    args.fussel_dichte der Fläche bedeckt ist; weiche Kante (Opacity Mask Clip 0,5 liegt auf der Schwelle).
    Entlang der Naht (Gummi) fällt die Dichte auf 0: frei bis 1,6 mm vom Nahtmittel, voll ab 2,8 mm."""
    breite = int(args.fussel_aufloesung)
    hoehe = breite // 2
    richt = atlas_richtungen(breite, hoehe).reshape(-1, 3)
    naht, _ = nahtkurve(args.naht_b, 1536)
    cos_max = np.empty(len(richt))
    naht32 = naht.T.astype(np.float32)
    for start in range(0, len(richt), 8192):  # 8192 x 1536 Skalarprodukte je Block (50 MB)
        cos_max[start:start + 8192] = (richt[start:start + 8192].astype(np.float32) @ naht32).max(axis=1)
    abstand = np.arccos(np.clip(cos_max, -1.0, 1.0)) * radius
    frei, voll = 0.0016, 0.0028
    x = np.clip((abstand - frei) / (voll - frei), 0.0, 1.0)
    dichte = x * x * (3.0 - 2.0 * x)
    p = richt * (radius + args.fussel_hoehe)
    zellen = (0.00016, 0.00007)  # feine Koernung; die feinere Lage liegt nahe der Texelgroesse (~0,08 mm)
    rauschen = 0.62 * wertrauschen(p, zellen[0], 1) + 0.38 * wertrauschen(p, zellen[1], 2)
    schwelle = float(np.quantile(rauschen[dichte > 0.999], 1.0 - args.fussel_dichte))
    kante = 0.035
    wert = np.clip((rauschen - schwelle) / kante + 0.5, 0.0, 1.0) * dichte
    anteil = float((wert >= OPACITY_CLIP).mean())
    return wert.reshape(hoehe, breite), {"aufloesung_px": [breite, hoehe], "rauschzellen_m": list(zellen),
                                         "schwelle": round(schwelle, 4), "kante": kante,
                                         "naht_frei_bis_m": frei, "naht_voll_ab_m": voll,
                                         "bedeckter_anteil_atlas": round(anteil, 4)}


def png_grau_schreiben(pfad, werte):
    """8-Bit-Graustufen-PNG (werte 0..1, H x W, unterste Zeile zuerst wie in Blender) ohne Farbverwaltung."""
    daten = np.clip(np.rint(werte * 255.0), 0, 255).astype(np.uint8)[::-1]  # PNG: oberste Zeile zuerst
    hoehe, breite = daten.shape
    roh = b"".join(b"\x00" + zeile.tobytes() for zeile in daten)

    def block(art, inhalt):
        return struct.pack(">I", len(inhalt)) + art + inhalt + struct.pack(">I", zlib.crc32(art + inhalt) & 0xFFFFFFFF)

    Path(pfad).write_bytes(b"\x89PNG\r\n\x1a\n" + block(b"IHDR", struct.pack(">IIBBBBB", breite, hoehe, 8, 0, 0, 0, 0))
                           + block(b"IDAT", zlib.compress(roh, 9)) + block(b"IEND", b""))
    mb.log("geschrieben: %s (%d x %d, grau)" % (pfad, breite, hoehe))


def fussel_material(bild):
    """M_Ball_Fussel: Fasern der Hülle, Alpha = Maske (Unreal: Blend Mode Masked, Opacity Mask, Two Sided)."""
    mat = mb.material_grundfarbe("M_Ball_Fussel", FUSSEL_SRGB, rauheit=1.0, spiegelung=0.0, sheen=0.6,
                                 sheen_rauheit=0.4, sheen_farbe="Grundfarbe", zweiseitig=True,
                                 hinweis="Fussel-Huelle 0,7 mm ueber dem Filz, Opazitaet aus Maske; Fasern streuen "
                                         "diffus (Specular 0), sonst wirkt der Rand unter streifendem Blick glasig")
    baum = mat.node_tree
    bsdf = mb.principled(mat)
    farbe_lin = [mb.srgb_zu_linear(c) for c in FUSSEL_SRGB] + [1.0]
    for namen, wert in ((("Sheen Weight", "Sheen"), 0.6), (("Sheen Roughness",), 0.4),
                        (("Specular IOR Level", "Specular"), 0.0), (("Sheen Tint",), farbe_lin)):
        e = mb.eingang(bsdf, *namen)
        if e is not None and (e.type == 'RGBA') == isinstance(wert, list):  # Sheen Tint ist erst ab 4.0 eine Farbe
            e.default_value = wert
    tex = baum.nodes.new("ShaderNodeTexImage")
    tex.image = bild
    tex.extension = 'EXTEND'
    baum.links.new(tex.outputs["Color"], bsdf.inputs["Alpha"])
    mb.material_alpha(mat)
    if hasattr(mat, "use_backface_culling"):
        mat.use_backface_culling = False
    mb.material_info_setzen(mat, textur=Path(bild.filepath_raw).name, alpha="Opacity Mask (maskiert)",
                            unreal={"blend_mode": "Masked", "opacity_mask": "R von %s" % Path(bild.filepath_raw).name,
                                    "opacity_mask_clip_value": OPACITY_CLIP, "two_sided": True,
                                    "hinweis": "Shading wie Filz (Sheen/Cloth), Grundfarbe konstant"})
    return mat


def huelle_unterteilung(args, stufe):
    """Vierecke je Würfelkante der Hülle in einer LOD-Stufe: LOD0 args.fussel_unterteilung, je Stufe x0,75."""
    return max(8, int(round(args.fussel_unterteilung * 0.75 ** stufe)))


def huelle_anhaengen(ziel, radius, unterteilung, material):
    """Baut eine Fussel-Hülle (Quad-Sphere, Materialslot hinter Filz und Naht) und hängt sie an ziel an."""
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new(ziel.data.uv_layers.active.name if ziel.data.uv_layers.active else "UVMap")
    kugel_bauen(bm, radius, unterteilung, uv, material_index=0)
    huelle = mb.objekt_aus_bmesh(ziel.name + "_Huelle", bm, [material])
    mb.glatt_nach_winkel(huelle.data, 75.0)
    return mb.objekte_vereinen([ziel, huelle], ziel.name)  # Material landet in Slot SLOT_FUSSEL


def gestaucht_hinzufuegen(obj, stauchung):
    """Formschlüssel 'gestaucht': Z * (1 - stauchung), X/Y * 1/sqrt(1 - stauchung) (Volumen konstant)."""
    fz = 1.0 - stauchung
    fxy = 1.0 / math.sqrt(fz)
    if obj.data.shape_keys is None:
        obj.shape_key_add(name="Basis", from_mix=False)
    sk = obj.shape_key_add(name="gestaucht", from_mix=False)
    co = np.empty(len(obj.data.vertices) * 3, dtype=np.float32)
    obj.data.vertices.foreach_get("co", co)
    co = co.reshape(-1, 3)
    co[:, 0] *= fxy
    co[:, 1] *= fxy
    co[:, 2] *= fz
    sk.data.foreach_set("co", co.ravel())
    sk.slider_min, sk.slider_max = 0.0, 1.0
    sk.value = 0.0
    return {"z": round(fz, 4), "xy": round(fxy, 4)}


def main(args):
    mb.neue_szene()
    ordner = mb.ausgabeordner(args, BEREICH)
    radius = args.durchmesser * 0.5

    filz = filz_material()
    naht = mb.material_grundfarbe("M_Ball_Naht", NAHT_SRGB, rauheit=0.6, hinweis="Gummi-Naht")
    materialien = [filz, naht]
    fussel_info, textur_eintraege = None, []
    if args.fussel_lods > 0:
        maske, fussel_info = fussel_maske(args, radius)
        maske_pfad = ordner / "T_Ball_Fussel_Maske.png"
        png_grau_schreiben(maske_pfad, maske)
        bild = bpy.data.images.load(str(maske_pfad), check_existing=False)
        bild.name = maske_pfad.stem
        bild.colorspace_settings.name = 'Non-Color'
        materialien.append(fussel_material(bild))
        textur_eintraege.append({"datei": maske_pfad.name, "rolle": "Alpha", "kanaele": "R (grau)", "srgb": False,
                                 "material": "M_Ball_Fussel", "groesse_px": fussel_info["aufloesung_px"],
                                 "adressierung": "Clamp (3x2-Wuerfelatlas der Huelle)",
                                 "unreal": "Opacity Mask: R an Opacity Mask, Blend Mode Masked, Clip %.2f, Two Sided; "
                                           "Compression Masks/Grayscale, sRGB aus" % OPACITY_CLIP})
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    kugel_bauen(bm, radius, args.unterteilung, uv, material_index=SLOT_FILZ)
    punkte, tangenten = nahtkurve(args.naht_b, args.naht_punkte)
    rille_eindruecken(bm, radius, punkte, tiefe=args.rille_tiefe, breite=args.rille_breite)
    nahtrohr_bauen(bm, radius, punkte, tangenten, rohr_radius=args.naht_radius, versenkt=args.naht_versenkt,
                   ring=8, uv=uv, material_index=SLOT_NAHT)
    ball = mb.objekt_aus_bmesh(ASSET, bm, materialien[:2])
    mb.glatt_nach_winkel(ball.data, 75.0)
    mb.log("Ball: Durchmesser %.4f m (Filz), %d Dreiecke Filz + Naht" % (args.durchmesser, mb.dreiecke(ball)))

    # LODs aus Filz + Naht (Verhältnisse wie bisher); danach bekommen die ersten args.fussel_lods Stufen eine
    # eigene Hülle (gleiches 3x2-Atlas-Schema, eigenes Material und eigene Textur T_Ball_Fussel_Maske)
    stufen = mb.lods_erzeugen(ball, args.lods, basisname=ASSET, glatt_winkel=75.0)
    for st in stufen:
        st["fussel_huelle"] = fussel_info is not None and st["stufe"] < args.fussel_lods
        if st["fussel_huelle"]:
            n = huelle_unterteilung(args, st["stufe"])
            st["objekt"] = huelle_anhaengen(st["objekt"], radius + args.fussel_hoehe, n, materialien[SLOT_FUSSEL])
            st["dreiecke"] = mb.dreiecke(st["objekt"])
            st["huelle_unterteilung"] = n
        mb.log("LOD%d %s: %d Dreiecke, Fussel-Huelle %s" % (st["stufe"], st["objekt"].name, st["dreiecke"],
                                                             "ja" if st["fussel_huelle"] else "nein"))
    ball = stufen[0]["objekt"]
    stauch = None
    for st in stufen:
        stauch = gestaucht_hinzufuegen(st["objekt"], args.stauchung)  # nimmt die Hülle mit (selbes Mesh)

    fbx = ordner / (ASSET + ".fbx")
    export = mb.exportiere_static_mesh([ball], fbx)
    lod_dateien = [{"stufe": 0, "datei": fbx.name, "verhaeltnis": stufen[0]["verhaeltnis"],
                    "dreiecke": stufen[0]["dreiecke"], "fussel_huelle": stufen[0].get("fussel_huelle", False)}]
    for st in stufen[1:]:
        pfad = ordner / ("%s_LOD%d.fbx" % (ASSET, st["stufe"]))
        mb.exportiere_static_mesh([st["objekt"]], pfad)
        lod_dateien.append({"stufe": st["stufe"], "datei": pfad.name, "verhaeltnis": st["verhaeltnis"],
                            "dreiecke": st["dreiecke"], "fussel_huelle": st.get("fussel_huelle", False)})
    masse = mb.masse([ball])
    blend = mb.blend_speichern(args, ordner / (ASSET + ".blend"))

    vorschau = []
    if args.vorschau:
        vorne = Vector((0.0, -1.0, 0.0))
        oben = Vector((0.0, 0.0, 1.0))
        nah_richtung = mb.blickrichtung(vorne, oben, *mb.ANSICHT_WINKEL["nah"])
        naechster = max(range(len(punkte)), key=lambda k: Vector(punkte[k]).dot(nah_richtung))
        nah_mitte = Vector(punkte[naechster]) * radius
        vorschau = mb.vorschau_rendern(args, ASSET, [ball], vorne=vorne, oben=oben,
                                       nah=(nah_mitte, radius * 0.45), brennweite=85.0)
        ball.data.shape_keys.key_blocks["gestaucht"].value = 1.0
        vorschau += mb.vorschau_rendern(args, ASSET, [ball], vorne=vorne, oben=oben, ansichten=("seitlich",),
                                        dateiname=ASSET + "_gestaucht", brennweite=85.0)
        ball.data.shape_keys.key_blocks["gestaucht"].value = 0.0
        ziel = vorschau[-1].with_name(ASSET + "_gestaucht.png")
        vorschau[-1].replace(ziel)
        vorschau[-1] = ziel

    mb.meta_schreiben(
        ordner, ASSET, "static",
        quelle="neu modelliert per Skript (Unreal/blender/scripts/ball.py)",
        lizenz="eigenes Werk (Matchball Tennis)",
        unreal_ziel=UNREAL_ZIEL,
        parameter={"durchmesser_m": args.durchmesser, "itf_bereich_m": [0.0654, 0.0686],
                   "unterteilung_je_wuerfelseite": args.unterteilung,
                   "naht": {"kurve": "x=a cos t + b cos 3t, y=a sin t - b sin 3t, z=2 sqrt(ab) sin 2t, a=1-b",
                            "b": args.naht_b, "punkte": args.naht_punkte, "rohr_radius_m": args.naht_radius,
                            "versenkt_m": args.naht_versenkt, "rille_tiefe_m": args.rille_tiefe,
                            "rille_breite_m": args.rille_breite},
                   "stauchung": args.stauchung, "lod_verhaeltnisse": list(args.lods),
                   "fussel": None if fussel_info is None else dict(
                       fussel_info, huelle_hoehe_m=args.fussel_hoehe, huelle_unterteilung=args.fussel_unterteilung,
                       lods_mit_huelle=args.fussel_lods, dichte=args.fussel_dichte,
                       huelle_unterteilung_je_lod=[huelle_unterteilung(args, i) for i in range(min(args.fussel_lods,
                                                                                                   len(args.lods)))],
                       durchmesser_huelle_m=round(args.durchmesser + 2.0 * args.fussel_hoehe, 5),
                       materialslot=SLOT_FUSSEL)},
        dateien={"fbx": fbx.name, "lods": lod_dateien, "texturen": textur_eintraege,
                 "blend": blend.name if blend else None,
                 "vorschau": [mb.relativ_zur_ausgabe(args, p) for p in vorschau]},
        materialien=mb.materialien_info([ball]),
        fussel=None if fussel_info is None else {
            "verfahren": "Shell: zweite Huelle %.1f mm ueber dem Filz im selben Mesh (Materialslot %d M_Ball_Fussel, "
                         "maskiert), nur LOD0..LOD%d" % (args.fussel_hoehe * 1e3, SLOT_FUSSEL,
                                                          min(args.fussel_lods, len(args.lods)) - 1),
            "unreal": {"material": "M_Ball_Fussel", "blend_mode": "Masked",
                       "opacity_mask": "R von T_Ball_Fussel_Maske.png", "opacity_mask_clip_value": OPACITY_CLIP,
                       "two_sided": True},
            "durchmesser_filz_m": args.durchmesser,
            "hinweis": "Bounds enthalten die Huelle (%.4f m) - Ball nicht aus den Bounds skalieren"
                       % (args.durchmesser + 2.0 * args.fussel_hoehe)},
        formschluessel={"gestaucht": {"faktoren": stauch, "achse": "lokal Z",
                                      "hinweis": "Unreal-Static-Meshes haben keine Morph Targets: gleiche Form "
                                                 "durch Skalierung der Ball-Komponente (Z %.3f, quer %.3f) "
                                                 "entlang der Stossnormalen; Ballmitte bei Bodenkontakt um "
                                                 "%.4f m * Gewicht absenken" % (stauch["z"], stauch["xy"],
                                                                                args.stauchung * radius)}},
        dreiecke=stufen[0]["dreiecke"], masse=masse, kollision="keine (Ballphysik ist eigene Simulation)",
        export=export)


def _verhaeltnisse(text):
    werte = [float(x) for x in text.replace(";", ",").split(",") if x.strip()]
    if not werte or any(not (0.0 < w <= 1.0) for w in werte):
        raise ValueError("LOD-Verhaeltnisse muessen zwischen 0 und 1 liegen: %r" % text)
    return werte


def parser():
    p = mb.basis_parser("Tennisball (Naht als Geometrie, Filz, Formschluessel 'gestaucht', LODs)")
    p.add_argument("--durchmesser", type=float, default=0.067, help="Durchmesser in m (ITF 0.0654-0.0686)")
    p.add_argument("--unterteilung", type=int, default=32, help="Vierecke je Wuerfelkante der Kugel (Standard 32)")
    p.add_argument("--naht-b", type=float, default=0.30, help="Parameter b der Nahtkurve (0 < b < 0.5)")
    p.add_argument("--naht-punkte", type=int, default=384, help="Stuetzpunkte der Naht")
    p.add_argument("--naht-radius", type=float, default=0.0010, help="Radius des Nahtrohrs in m")
    p.add_argument("--naht-versenkt", type=float, default=0.0008, help="Nahtmitte unter der Kugelflaeche in m")
    p.add_argument("--rille-tiefe", type=float, default=0.0005, help="Tiefe der Rille im Filz in m")
    p.add_argument("--rille-breite", type=float, default=0.0018, help="Halbbreite (1/e) der Rille in m")
    p.add_argument("--stauchung", type=float, default=0.30, help="Stauchung fuer den Formschluessel (0.30 = 30 %%)")
    p.add_argument("--lods", type=_verhaeltnisse, default=[1.0, 0.5, 0.25, 0.12],
                   help="LOD-Verhaeltnisse, z. B. 1.0,0.5,0.25,0.12")
    p.add_argument("--fussel-hoehe", type=float, default=0.0007,
                   help="Abstand der Fussel-Huelle ueber dem Filz in m (0.0005-0.001, Standard 0.0007)")
    p.add_argument("--fussel-unterteilung", type=int, default=16,
                   help="Vierecke je Wuerfelkante der Fussel-Huelle auf LOD0 (Standard 16 = 3072 Dreiecke; "
                        "je weitere Stufe x0.75)")
    p.add_argument("--fussel-lods", type=int, default=2,
                   help="Anzahl LOD-Stufen mit Fussel-Huelle ab LOD0 (Standard 2 = LOD0 und LOD1, 0 = keine Huelle)")
    p.add_argument("--fussel-dichte", type=float, default=0.42, help="bedeckter Anteil der Huelle (Standard 0.42)")
    p.add_argument("--fussel-aufloesung", type=int, default=2048,
                   help="Breite der Fussel-Maske in Pixel (Hoehe = Breite/2, Standard 2048)")
    return p


if __name__ == "__main__":
    mb.ausfuehren(main, parser())
