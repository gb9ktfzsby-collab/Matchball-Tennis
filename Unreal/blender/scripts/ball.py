# -*- coding: utf-8 -*-
"""
ball.py - Tennisball, neu modelliert (README 3.4, 24.2; Meilenstein M0).

Erzeugt:
  _erzeugt/Equipment/Ball/SM_Ball.fbx               LOD0 (mit Formschlüssel "gestaucht")
  _erzeugt/Equipment/Ball/SM_Ball_LOD1..3.fbx       weitere LOD-Stufen (Decimate 0.5 / 0.25 / 0.12)
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
    läuft ein dünnes Rohr (eigenes Material M_BallNaht), das in einer flachen Rille im Filz liegt.
  * Filz-Material M_BallFilz (Optic Yellow) mit prozeduralem Rauschen für Fussel und "Sheen" für den
    hellen Fussel-Rand. In Unreal baut das Import-Skript ein eigenes Material; meta.json enthält die Werte.
  * Formschlüssel "gestaucht": entlang lokal Z um 30 % gestaucht, in X/Y um 1/sqrt(0,7) = 1,195 verbreitert
    (Volumen bleibt gleich). Unreal-Static-Meshes kennen keine Morph Targets - da die Form eine reine
    Ellipsoid-Skalierung ist, erzeugt das Spiel sie durch Skalieren der Ball-Komponente entlang der Stoß-
    normalen (Z 0,70, quer 1,195). Wer den Formschlüssel direkt nutzen will, importiert als Skeletal Mesh.
"""

import math
import sys
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


def kugel_bauen(bm, radius, n, uv):
    """Quad-Sphere mit n x n Vierecken je Würfelseite und 3x2-UV-Atlas (Rand 0,01)."""
    rand = 0.01
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
        for i in range(n):
            for j in range(n):
                f = bm.faces.new((gitter[i][j], gitter[i + 1][j], gitter[i + 1][j + 1], gitter[i][j + 1]))
                f.material_index = 0
                for loop, (di, dj) in zip(f.loops, ((0, 0), (1, 0), (1, 1), (0, 1))):
                    u = (spalte + rand + (1 - 2 * rand) * (i + di) / n) / 3.0
                    v = (zeile + rand + (1 - 2 * rand) * (j + dj) / n) / 2.0
                    loop[uv].uv = (u, v)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=radius * 1e-5)


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
    mat = mb.material_grundfarbe("M_BallFilz", OPTIC_YELLOW_SRGB, rauheit=0.92,
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
    naht = mb.material_grundfarbe("M_BallNaht", NAHT_SRGB, rauheit=0.6, hinweis="Gummi-Naht")
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    kugel_bauen(bm, radius, args.unterteilung, uv)
    punkte, tangenten = nahtkurve(args.naht_b, args.naht_punkte)
    rille_eindruecken(bm, radius, punkte, tiefe=args.rille_tiefe, breite=args.rille_breite)
    nahtrohr_bauen(bm, radius, punkte, tangenten, rohr_radius=args.naht_radius, versenkt=args.naht_versenkt,
                   ring=8, uv=uv, material_index=1)
    ball = mb.objekt_aus_bmesh(ASSET, bm, [filz, naht])
    mb.glatt_nach_winkel(ball.data, 75.0)
    mb.log("Ball: Durchmesser %.4f m, %d Dreiecke" % (args.durchmesser, mb.dreiecke(ball)))

    stufen = mb.lods_erzeugen(ball, args.lods, basisname=ASSET, glatt_winkel=75.0)
    stauch = None
    for st in stufen:
        stauch = gestaucht_hinzufuegen(st["objekt"], args.stauchung)

    fbx = ordner / (ASSET + ".fbx")
    export = mb.exportiere_static_mesh([ball], fbx)
    lod_dateien = [{"stufe": 0, "datei": fbx.name, "verhaeltnis": stufen[0]["verhaeltnis"],
                    "dreiecke": stufen[0]["dreiecke"]}]
    for st in stufen[1:]:
        pfad = ordner / ("%s_LOD%d.fbx" % (ASSET, st["stufe"]))
        mb.exportiere_static_mesh([st["objekt"]], pfad)
        lod_dateien.append({"stufe": st["stufe"], "datei": pfad.name, "verhaeltnis": st["verhaeltnis"],
                            "dreiecke": st["dreiecke"]})
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
                   "stauchung": args.stauchung, "lod_verhaeltnisse": list(args.lods)},
        dateien={"fbx": fbx.name, "lods": lod_dateien, "blend": blend.name if blend else None,
                 "vorschau": [mb.relativ_zur_ausgabe(args, p) for p in vorschau]},
        materialien=mb.materialien_info([ball]),
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
    return p


if __name__ == "__main__":
    mb.ausfuehren(main, parser())
