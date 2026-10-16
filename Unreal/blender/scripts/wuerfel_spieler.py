# -*- coding: utf-8 -*-
"""
wuerfel_spieler.py - M0-Pipeline-Probe "Würfel-Spieler" (README 21 M0, README 3.3 Skelett).

Erzeugt:
  _erzeugt/Characters/Wuerfel/SK_WuerfelSpieler.fbx            Skelett + Quader-Körper in Ruhepose (A-Pose)
  _erzeugt/Characters/Wuerfel/A_Wuerfel_Bereitschaft.fbx       Testanimation (2 s, nur Skelett)
  _erzeugt/Characters/Wuerfel/SK_WuerfelSpieler.blend          Quelle (mit Aktion A_Wuerfel_Bereitschaft)
  _erzeugt/Characters/Wuerfel/*.meta.json                      Ziel in Unreal: /Game/Characters/Wuerfel
  _vorschau/SK_WuerfelSpieler_{vorne,seitlich,dreiviertel,nah}.png
  _vorschau/A_Wuerfel_Bereitschaft_bogen.png                   Kontaktbogen: 4 Zeitpunkte, vorne und seitlich

Aufruf:
  blender.exe -b --factory-startup --python-exit-code 1 -P Unreal\\blender\\scripts\\wuerfel_spieler.py -- [--pose A|T]
  python Unreal/blender/scripts/wuerfel_spieler.py --ausgabe <ordner> --engine CYCLES      (bpy als Modul)

Skelett: Knochennamen und Hierarchie wie das UE5-Mannequin (root, pelvis, spine_01..spine_05, neck_01, head,
clavicle/upperarm/lowerarm/hand, thigh/calf/foot/ball je _l/_r) plus racket_r, racket_l (Schlägeraufnahme) und
ball_hand (Ball beim Aufschlag, linke Hand). Das UE5-Mannequin hat zusätzlich neck_02, Finger, Zehen und
ik_*/twist-Knochen - die folgen mit der echten Figur (figur.py).

Ausrichtung: Die Figur schaut in Blender nach +X (links = +Y) und damit in Unreal ebenfalls nach +X
(siehe mb_gemeinsam.ACHSEN_UND_MASSSTAB). Ruhepose: A-Pose (Arme 45 Grad unter der Waagerechten, Ellbogen
15 Grad nach vorn gebeugt, Knie 1,2 cm nach vorn) - das ist näher an der Ruhepose des UE5-Mannequins als eine
T-Pose und gibt IK-Ketten eine eindeutige Beugerichtung. Mit --pose T entsteht eine T-Pose.

Proportionen nach Drillis & Contini (1966), Anteile der Körpergröße H (Standard H = 1,85 m):
Scheitel 1,000 · Kinn 0,870 · Schulter (Akromion) 0,818 · Ellbogen 0,630 · Handgelenk 0,485 · Hüfte 0,530 ·
Knie 0,285 · Knöchel 0,039 · Schulterbreite 0,259 · Hüftbreite 0,191 · Fußlänge 0,152 · Fußbreite 0,055 ·
Oberarm 0,186 · Unterarm 0,146 · Hand 0,108 · Oberschenkel 0,245 · Unterschenkel 0,246.

Schlägeraufnahme racket_r/racket_l: Knochenursprung = Griffende des Schlägers, lokale Z-Achse = Schläger-
Längsachse (Griffende -> Kopf), lokale Y-Achse (Knochenrichtung) = Normale des Saitenbetts. SM_Schlaeger
(Ursprung am Griffende, Längsachse +Z) wird in Unreal ohne Versatz an den Knochen gehängt.
ball_hand: Knochenursprung = Ballmittelpunkt in der Handfläche, SM_Ball ohne Versatz anhängen.
"""

import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "lib"))
import mb_gemeinsam as mb  # noqa: E402

import bpy  # noqa: E402
import bmesh  # noqa: E402
from mathutils import Matrix, Vector  # noqa: E402

ASSET = "SK_WuerfelSpieler"
SKELETT = "SKEL_WuerfelSpieler"
ANIM = "A_Wuerfel_Bereitschaft"
BEREICH = "Characters/Wuerfel"
UNREAL_ZIEL = "/Game/Characters/Wuerfel"
H_REF = 1.85

# Drillis & Contini (1966): Anteile der Körpergröße
P = {
    "kinn": 0.870, "schulter": 0.818, "ellbogen": 0.630, "handgelenk": 0.485, "huefte": 0.530, "knie": 0.285,
    "knoechel": 0.039, "schulterbreite": 0.259, "hueftbreite": 0.191, "fusslaenge": 0.152, "fussbreite": 0.055,
    "oberarm": 0.186, "unterarm": 0.146, "hand": 0.108, "oberschenkel": 0.245, "unterschenkel": 0.246,
}

VORNE = Vector((1.0, 0.0, 0.0))
OBEN = Vector((0.0, 0.0, 1.0))

# Quader je Knochen im Knochenraum (Maße für H = 1,85 m, werden mit H skaliert):
# Name: (Breite lokal X, Tiefe lokal Z, Beginn entlang Y, Ende entlang Y [None = Knochenende], Material, Versatz Z)
# Rumpf/Beine/Arme: lokal Z zeigt nach vorn (+X Welt), lokal Y entlang des Knochens.
QUADER = {
    "pelvis": (0.34, 0.22, -0.10, None, "Hose", 0.0),
    "spine_01": (0.30, 0.20, 0.0, None, "Shirt", 0.0),
    "spine_02": (0.31, 0.20, 0.0, None, "Shirt", 0.0),
    "spine_03": (0.33, 0.21, 0.0, None, "Shirt", 0.0),
    "spine_04": (0.36, 0.22, 0.0, None, "Shirt", 0.0),
    "spine_05": (0.40, 0.22, 0.0, None, "Shirt", 0.0),
    "neck_01": (0.11, 0.11, -0.01, None, "Haut", 0.0),
    "head": (0.16, 0.21, -0.03, None, "Haut", 0.015),
    "clavicle_*": (0.085, 0.085, 0.0, None, "Shirt", 0.0),
    "upperarm_*": (0.10, 0.10, -0.02, None, "Shirt", 0.0),
    "lowerarm_*": (0.085, 0.085, 0.0, None, "Haut", 0.0),
    "hand_*": (0.035, 0.09, 0.0, 0.20, "Haut", 0.0),
    "thigh_*": (0.15, 0.15, 0.0, None, "Hose", 0.0),
    "calf_*": (0.11, 0.11, 0.0, None, "Haut", 0.0),
}
# Füße als weltachsparallele Quader (x von, x bis, halbe Breite, z von, z bis, Material)
FUSS_QUADER = {
    "foot_*": (-0.060, 0.137, 0.050, 0.0, 0.090, "Schuh"),
    "ball_*": (0.137, 0.222, 0.048, 0.0, 0.045, "Schuh"),
}
# Nase als Blickrichtungs-Marke am Kopf: (Mitte lokal x, y, z), (Größe x, y, z)
NASE = ((0.0, 0.07, 0.125), (0.03, 0.04, 0.03))

FARBEN = {
    "Haut": ((0.85, 0.66, 0.52), 0.55),
    "Shirt": ((0.90, 0.91, 0.93), 0.80),
    "Hose": ((0.12, 0.17, 0.33), 0.85),
    "Schuh": ((0.96, 0.96, 0.96), 0.60),
    "Akzent": ((0.80, 0.10, 0.08), 0.50),
}


# ---------------------------------------------------------------------------------------------------------------------
# Skelett
# ---------------------------------------------------------------------------------------------------------------------

def _senkrecht_anteil(v, achse):
    """Anteil von v senkrecht zu achse (normiert)."""
    r = v - achse * v.dot(achse)
    return r.normalized()


def skelett_plan(H, pose):
    """Knochen: Name -> dict(kopf, ende, eltern, z_richtung, verformen) in Metern (Blick +X, links +Y)."""
    k = H / H_REF
    plan = {}

    def knochen(name, kopf, ende, eltern, z_richtung=VORNE, verformen=True):
        plan[name] = {"kopf": Vector(kopf), "ende": Vector(ende), "eltern": eltern,
                      "z_richtung": None if z_richtung is None else Vector(z_richtung), "verformen": verformen}

    z_huefte = P["huefte"] * H
    knochen("root", (0, 0, 0), (0, 0.25 * k, 0), None, z_richtung=None, verformen=False)
    becken_ende = Vector((0, 0, z_huefte + 0.080 * k))
    knochen("pelvis", (0, 0, z_huefte), becken_ende, "root")
    z_hals = 0.835 * H
    stuetz = [becken_ende.z + (z_hals - becken_ende.z) * i / 5.0 for i in range(6)]
    eltern = "pelvis"
    for i in range(5):
        name = "spine_%02d" % (i + 1)
        knochen(name, (0, 0, stuetz[i]), (0, 0, stuetz[i + 1]), eltern)
        eltern = name
    z_schaedel = 0.886 * H
    knochen("neck_01", (0, 0, z_hals), (0.005 * k, 0, z_schaedel), "spine_05")
    knochen("head", (0.005 * k, 0, z_schaedel), (0.005 * k, 0, H), "neck_01")

    hand_dir = {}
    for seite, s in (("l", 1.0), ("r", -1.0)):
        schulter = Vector((-0.010 * k, s * 0.110 * H, (P["schulter"] - 0.020) * H))
        knochen("clavicle_" + seite, (0.025 * k, s * 0.020 * k, 0.808 * H), schulter, "spine_05")
        if pose == "T":
            d1 = Vector((0.0, s, 0.0))
            beuge = 0.0
        else:
            d1 = Vector((0.05, s * math.cos(math.radians(45)), -math.sin(math.radians(45)))).normalized()
            beuge = 15.0
        d2 = (d1 + _senkrecht_anteil(VORNE, d1) * math.tan(math.radians(beuge))).normalized()
        ellbogen = schulter + d1 * (P["oberarm"] * H)
        handgelenk = ellbogen + d2 * (P["unterarm"] * H)
        knochen("upperarm_" + seite, schulter, ellbogen, "clavicle_" + seite)
        knochen("lowerarm_" + seite, ellbogen, handgelenk, "upperarm_" + seite)
        knochen("hand_" + seite, handgelenk, handgelenk + d2 * (0.5 * P["hand"] * H), "lowerarm_" + seite)
        hand_dir[seite] = (handgelenk, d2)

        huefte = Vector((0.0, s * 0.0475 * H, z_huefte))
        knie = Vector((0.012 * k, s * 0.050 * H, P["knie"] * H))
        knoechel = Vector((0.0, s * 0.052 * H, P["knoechel"] * H))
        ballen = Vector((0.074 * H, s * 0.054 * H, 0.011 * H))
        spitze = Vector((0.120 * H, s * 0.054 * H, 0.011 * H))
        knochen("thigh_" + seite, huefte, knie, "pelvis")
        knochen("calf_" + seite, knie, knoechel, "thigh_" + seite)
        knochen("foot_" + seite, knoechel, ballen, "calf_" + seite, z_richtung=OBEN)
        knochen("ball_" + seite, ballen, spitze, "foot_" + seite, z_richtung=OBEN)

    # Schlägeraufnahmen: Ursprung = Griffende (3 cm hinter dem Handgelenk in der Hand),
    # Z = Schaftrichtung (Verlängerung des Unterarms), Y = Saitenbett-Normale (nach vorn).
    for seite in ("r", "l"):
        handgelenk, d2 = hand_dir[seite]
        griffende = handgelenk + d2 * (0.03 * k)
        normale = _senkrecht_anteil(VORNE, d2)
        knochen("racket_" + seite, griffende, griffende + normale * (0.12 * k), "hand_" + seite,
                z_richtung=d2, verformen=False)

    # Ball in der linken Handfläche: Handflächen-Normale zeigt zur Körpermitte (-Y bei links).
    handgelenk, d2 = hand_dir["l"]
    flaechen_normale = d2.cross(VORNE).normalized()
    if flaechen_normale.y > 0:
        flaechen_normale = -flaechen_normale
    ballmitte = handgelenk + d2 * (0.06 * k) + flaechen_normale * ((0.0335 + 0.018) * k)
    knochen("ball_hand", ballmitte, ballmitte + d2 * (0.05 * k), "hand_l", verformen=False)
    return plan


def armatur_bauen(plan):
    daten = bpy.data.armatures.new(SKELETT)
    daten.display_type = 'OCTAHEDRAL'
    arm = bpy.data.objects.new("Armature", daten)
    arm.show_in_front = True
    mb.verknuepfen(arm)
    bpy.context.view_layer.objects.active = arm
    arm.select_set(True)
    bpy.ops.object.mode_set(mode='EDIT')
    try:
        for name, kb in plan.items():
            eb = daten.edit_bones.new(name)
            eb.head = kb["kopf"]
            eb.tail = kb["ende"]
            eb.use_deform = kb["verformen"]
            eb.use_connect = False
            if kb["z_richtung"] is None:
                eb.roll = 0.0
            else:
                eb.align_roll(kb["z_richtung"])
        for name, kb in plan.items():
            if kb["eltern"]:
                daten.edit_bones[name].parent = daten.edit_bones[kb["eltern"]]
    finally:
        bpy.ops.object.mode_set(mode='OBJECT')
    for pb in arm.pose.bones:
        pb.rotation_mode = 'QUATERNION'
    return arm


# ---------------------------------------------------------------------------------------------------------------------
# Körper aus Quadern (starr gewichtet)
# ---------------------------------------------------------------------------------------------------------------------

def _eintraege(tabelle, name):
    for schluessel, wert in tabelle.items():
        if schluessel == name or (schluessel.endswith("_*") and name[:-2] == schluessel[:-2]
                                  and name[-2:] in ("_l", "_r")):
            return wert
    return None


def koerper_bauen(arm, H):
    k = H / H_REF
    materialien = {n: mb.material_grundfarbe("M_Wuerfel_" + n, farbe, rauheit=rau)
                   for n, (farbe, rau) in FARBEN.items()}
    reihenfolge = list(materialien)
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    gruppen = {}

    def quader(knochen_name, matrix, groesse, material):
        vorher = len(bm.verts)
        mb.bm_quader(bm, matrix, groesse, reihenfolge.index(material), uv_schicht=uv)
        bm.verts.index_update()
        gruppen.setdefault(knochen_name, []).extend(range(vorher, len(bm.verts)))

    for knochen in arm.data.bones:
        name = knochen.name
        q = _eintraege(QUADER, name)
        if q is not None:
            breite, tiefe, y0, y1, material, versatz = q
            laenge = knochen.length
            y0 = y0 * k
            y1 = laenge if y1 is None else y1 * k
            m = knochen.matrix_local @ Matrix.Translation((0.0, (y0 + y1) * 0.5, versatz * k))
            quader(name, m, (breite * k, y1 - y0, tiefe * k), material)
            if name == "head":
                (mx, my, mz), (gx, gy, gz) = NASE
                quader(name, knochen.matrix_local @ Matrix.Translation((mx * k, my * k, mz * k)),
                       (gx * k, gy * k, gz * k), "Akzent")
        f = _eintraege(FUSS_QUADER, name)
        if f is not None:
            x0, x1, halb, z0, z1, material = f
            y_mitte = knochen.head_local.y if name.startswith("foot") else knochen.parent.head_local.y
            mitte = Vector(((x0 + x1) * 0.5 * k, y_mitte, (z0 + z1) * 0.5 * k))
            quader(name, Matrix.Translation(mitte), ((x1 - x0) * k, 2 * halb * k, (z1 - z0) * k), material)

    obj = mb.objekt_aus_bmesh(ASSET, bm, [materialien[n] for n in reihenfolge])
    for knochen_name, indizes in gruppen.items():
        vg = obj.vertex_groups.new(name=knochen_name)
        vg.add(indizes, 1.0, 'REPLACE')
    obj.parent = arm
    mod = obj.modifiers.new("Armature", 'ARMATURE')
    mod.object = arm
    mod.use_vertex_groups = True
    mod.use_bone_envelopes = False
    return obj


# ---------------------------------------------------------------------------------------------------------------------
# Testanimation A_Wuerfel_Bereitschaft
# ---------------------------------------------------------------------------------------------------------------------

def _drehung(pb, achse, grad):
    """Drehung um eine Achse im Armaturraum (Ruhelage), als lokales Quaternion des Pose-Knochens."""
    b = pb.bone.matrix_local.to_3x3()
    r = Matrix.Rotation(math.radians(grad), 3, Vector(achse).normalized())
    return (b.inverted() @ r @ b).to_quaternion()


def _verschiebung(pb, welt):
    return pb.bone.matrix_local.to_3x3().inverted() @ Vector(welt)


def bereitschaft_pose(arm, t, dauer, H):
    """Bereitschaftsstellung: Knie gebeugt (Becken tiefer), Oberkörper leicht vorgeneigt, Arme vorn;
    dazu Gewichtsverlagerung links/rechts (eine Periode pro dauer) und leichte Armbewegung."""
    k = H / H_REF
    w = 2.0 * math.pi * t / dauer
    pbs = arm.pose.bones
    for pb in pbs:
        pb.location = (0.0, 0.0, 0.0)
        pb.rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
        pb.scale = (1.0, 1.0, 1.0)

    seitlich = 0.035 * k * math.sin(w)
    tief = -0.075 * k - 0.010 * k * (1.0 - math.cos(2.0 * w)) * 0.5
    becken = pbs["pelvis"]
    becken.location = _verschiebung(becken, (0.0, seitlich, tief))
    becken.rotation_quaternion = (_drehung(becken, VORNE, 3.0 * math.sin(w))
                                  @ _drehung(becken, (0, 1, 0), 6.0))
    for i in range(1, 6):
        pb = pbs["spine_%02d" % i]
        pb.rotation_quaternion = _drehung(pb, (0, 1, 0), 2.5) @ _drehung(pb, VORNE, -0.8 * math.sin(w))
    pbs["neck_01"].rotation_quaternion = _drehung(pbs["neck_01"], (0, 1, 0), -8.0)
    pbs["head"].rotation_quaternion = (_drehung(pbs["head"], (0, 1, 0), -10.5)
                                       @ _drehung(pbs["head"], VORNE, 1.0 * math.sin(w)))

    for seite, s in (("l", 1.0), ("r", -1.0)):
        oa = pbs["upperarm_" + seite]
        ua = pbs["lowerarm_" + seite]
        d1 = (oa.bone.tail_local - oa.bone.head_local).normalized()
        d2 = (ua.bone.tail_local - ua.bone.head_local).normalized()
        beugen_oa = d1.cross(VORNE)  # positive Drehung bringt den Arm nach vorn
        beugen_ua = d2.cross(VORNE)
        phase = 0.6 if seite == "l" else 0.6 + math.pi
        oa.rotation_quaternion = (_drehung(oa, beugen_oa, 30.0 + 4.0 * math.sin(w + phase))
                                  @ _drehung(oa, VORNE, -s * 22.0))
        ua.rotation_quaternion = _drehung(ua, beugen_ua, 50.0 + 3.0 * math.sin(w + phase + 0.6))


def bein_ik_einrichten(arm, H, dauer):
    """IK an den Unterschenkeln (Kette 2) mit festen Fußzielen in Bereitschafts-Spreizung und Polzielen vor den
    Knien; die Fußdrehung bleibt wie in der Ruhepose (Füße flach). Der Polwinkel hängt von der Knochenrolle ab
    und wird ausprobiert: gewählt wird der Winkel, bei dem lokal Z des Oberschenkels (zeigt in der Ruhepose nach
    vorn) am besten zum Polziel zeigt."""
    k = H / H_REF
    ziele, iks = {}, {}
    pole = {}
    for seite, s in (("l", 1.0), ("r", -1.0)):
        fuss = arm.data.bones["foot_" + seite]
        knoechel = fuss.head_local
        ziel = bpy.data.objects.new("_IK_Fuss_" + seite, None)
        mb.verknuepfen(ziel)
        ziel.matrix_world = (Matrix.Translation((knoechel.x, s * 0.092 * H, knoechel.z))
                             @ fuss.matrix_local.to_3x3().to_4x4())
        pol = bpy.data.objects.new("_IK_Knie_" + seite, None)
        mb.verknuepfen(pol)
        pol.location = (0.60 * k, s * 0.075 * H, P["knie"] * H)
        ik = arm.pose.bones["calf_" + seite].constraints.new('IK')
        ik.target = ziel
        ik.pole_target = pol
        ik.chain_count = 2
        ik.use_tail = True
        ik.use_stretch = False
        rot = arm.pose.bones["foot_" + seite].constraints.new('COPY_ROTATION')
        rot.target = ziel
        rot.owner_space = 'WORLD'
        rot.target_space = 'WORLD'
        ziele[seite] = ziel.matrix_world.translation.copy()
        iks[seite] = ik
        pole[seite] = pol.location.copy()

    bereitschaft_pose(arm, 0.0, dauer, H)
    bester = None
    for grad in (-90.0, 90.0, 0.0, 180.0):
        for ik in iks.values():
            ik.pole_angle = math.radians(grad)
        bpy.context.view_layer.update()
        wert = 0.0
        for seite in iks:
            pb = arm.pose.bones["thigh_" + seite]
            z_achse = (arm.matrix_world.to_3x3() @ pb.matrix.to_3x3() @ Vector((0.0, 0.0, 1.0))).normalized()
            knie = arm.matrix_world @ pb.tail
            wert += z_achse.dot((pole[seite] - knie).normalized())
        if bester is None or wert > bester[1]:
            bester = (grad, wert)
    for ik in iks.values():
        ik.pole_angle = math.radians(bester[0])
    mb.log("Bein-IK: Polwinkel %.0f Grad (Uebereinstimmung %.3f von 2)" % bester)
    return ziele


def animation_backen(arm, H, fps, dauer):
    """Pose je Frame setzen, IK auswerten, lokale Knochenmatrizen in eine neue Aktion schreiben."""
    szene = bpy.context.scene
    frames = list(range(1, int(round(dauer * fps)) + 2))  # 2 s bei 30 fps: 1..61 (Frame 61 = Frame 1)
    ziele = bein_ik_einrichten(arm, H, dauer)
    lokale = {pb.name: [] for pb in arm.pose.bones}
    pruefung = {"fussgleiten_max_mm": 0.0, "knie_vor_der_linie_min_mm": 1e9, "knie_seitlich_max_mm": 0.0}
    for f in frames:
        bereitschaft_pose(arm, (f - frames[0]) / float(fps), dauer, H)
        bpy.context.view_layer.update()
        for pb in arm.pose.bones:
            m = arm.convert_space(pose_bone=pb, matrix=pb.matrix, from_space='POSE', to_space='LOCAL')
            lokale[pb.name].append(m.copy())
        for seite in ("l", "r"):
            ist = arm.matrix_world @ arm.pose.bones["calf_" + seite].tail
            pruefung["fussgleiten_max_mm"] = max(pruefung["fussgleiten_max_mm"], (ist - ziele[seite]).length * 1000)
            huefte = arm.matrix_world @ arm.pose.bones["thigh_" + seite].head
            knie = arm.matrix_world @ arm.pose.bones["thigh_" + seite].tail
            vor = knie.x - (huefte.x + ist.x) * 0.5
            pruefung["knie_vor_der_linie_min_mm"] = min(pruefung["knie_vor_der_linie_min_mm"], vor * 1000)
            anteil = (huefte.z - knie.z) / max(huefte.z - ist.z, 1e-6)
            seitlich = abs(knie.y - (huefte.y + (ist.y - huefte.y) * anteil))
            pruefung["knie_seitlich_max_mm"] = max(pruefung["knie_seitlich_max_mm"], seitlich * 1000)

    for pb in arm.pose.bones:
        for c in list(pb.constraints):
            pb.constraints.remove(c)
    for name in ("_IK_Fuss_l", "_IK_Fuss_r", "_IK_Knie_l", "_IK_Knie_r"):
        hilfe = bpy.data.objects.get(name)
        if hilfe:
            bpy.data.objects.remove(hilfe, do_unlink=True)

    aktion = bpy.data.actions.new(ANIM)
    aktion.use_fake_user = True
    mb.aktion_zuweisen(arm, aktion)
    for i, f in enumerate(frames):
        for pb in arm.pose.bones:
            loc, rot, sca = lokale[pb.name][i].decompose()
            if i > 0:
                rot.make_compatible(pb.rotation_quaternion)
            pb.location = loc
            pb.rotation_quaternion = rot
            pb.scale = sca
            for pfad in ("location", "rotation_quaternion", "scale"):
                pb.keyframe_insert(pfad, frame=f, group=pb.name)
    mb.aktion_zuweisen(arm, aktion)
    szene.frame_start, szene.frame_end = frames[0], frames[-1]
    szene.frame_set(frames[0])
    pruefung = {k: round(v, 3) for k, v in pruefung.items()}
    mb.log("Animation %s: %d Frames, Fussgleiten max %.3f mm, Knie mind. %.1f mm vor und hoechstens %.1f mm "
           "seitlich der Hueft-Knoechel-Linie" % (ANIM, len(frames), pruefung["fussgleiten_max_mm"],
                                                  pruefung["knie_vor_der_linie_min_mm"],
                                                  pruefung["knie_seitlich_max_mm"]))
    if pruefung["fussgleiten_max_mm"] > 2.0:
        raise RuntimeError("Bein-IK haelt die Fuesse nicht fest (%.1f mm)" % pruefung["fussgleiten_max_mm"])
    if pruefung["knie_vor_der_linie_min_mm"] < 0.0:
        raise RuntimeError("Knie knicken nach hinten ein")
    if pruefung["knie_seitlich_max_mm"] > 30.0:
        raise RuntimeError("Knie weichen seitlich aus (%.1f mm)" % pruefung["knie_seitlich_max_mm"])
    return aktion, (frames[0], frames[-1]), pruefung


# ---------------------------------------------------------------------------------------------------------------------
# Vorschau-Hilfsobjekt: Schläger-Attrappe am Knochen racket_r (wird nicht exportiert)
# ---------------------------------------------------------------------------------------------------------------------

def schlaeger_attrappe(arm, knochen="racket_r"):
    mat = mb.material_grundfarbe("M_Vorschau_Schlaeger", (0.15, 0.17, 0.22), rauheit=0.4)
    bm = bmesh.new()
    mb.bm_quader(bm, Matrix.Translation((0, 0, 0.0975)), (0.030, 0.026, 0.195))
    mb.bm_quader(bm, Matrix.Translation((0, 0, 0.28)), (0.018, 0.018, 0.17))
    a, b, zc, n = 0.124, 0.159, 0.6858 - 0.165, 36
    for i in range(n):
        w0, w1 = 2 * math.pi * i / n, 2 * math.pi * (i + 1) / n
        p0 = Vector((a * math.cos(w0), 0, zc + b * math.sin(w0)))
        p1 = Vector((a * math.cos(w1), 0, zc + b * math.sin(w1)))
        mitte, d = (p0 + p1) * 0.5, p1 - p0
        winkel = math.atan2(d.x, d.z)
        m = Matrix.Translation(mitte) @ Matrix.Rotation(winkel, 4, 'Y')
        mb.bm_quader(bm, m, (0.012, 0.022, d.length + 0.004))
    obj = mb.objekt_aus_bmesh("_Vorschau_Schlaeger", bm, [mat])
    pb = arm.pose.bones[knochen]
    obj.parent = arm
    obj.parent_type = 'BONE'
    obj.parent_bone = knochen
    obj.matrix_parent_inverse = Matrix.Identity(4)
    obj.matrix_basis = Matrix.Translation((0.0, -pb.bone.length, 0.0))  # Knochen-Elternraum beginnt am Ende
    return obj


def _rahmen_ueber_frames(objekte, bereich):
    """Gemeinsame Huelle ueber alle Frames (feste Kamera fuer den Kontaktbogen)."""
    szene = bpy.context.scene
    lo = Vector((1e9, 1e9, 1e9))
    hi = -lo
    for f in range(bereich[0], bereich[1] + 1, 2):
        szene.frame_set(f)
        a, b = mb.welt_bbox(objekte)
        lo = Vector(map(min, lo, a))
        hi = Vector(map(max, hi, b))
    return lo, hi


# ---------------------------------------------------------------------------------------------------------------------
# Hauptprogramm
# ---------------------------------------------------------------------------------------------------------------------

def main(args):
    szene = mb.neue_szene(fps=args.fps)
    H = float(args.groesse)
    ordner = mb.ausgabeordner(args, BEREICH)
    plan = skelett_plan(H, args.pose)
    arm = armatur_bauen(plan)
    koerper = koerper_bauen(arm, H)
    bpy.context.view_layer.update()
    mb.log("Skelett: %d Knochen, Koerper: %d Dreiecke, Hoehe %.3f m"
           % (len(arm.data.bones), mb.dreiecke(koerper), mb.welt_bbox([koerper])[1].z))

    aktion, bereich, pruefung = animation_backen(arm, H, args.fps, args.dauer)

    fbx_sk = ordner / (ASSET + ".fbx")
    fbx_anim = ordner / (ANIM + ".fbx")
    exp_sk = mb.exportiere_skelett(arm, [koerper], fbx_sk)
    exp_anim = mb.exportiere_skelett(arm, [koerper], fbx_anim, aktion=aktion, frame_bereich=bereich,
                                     take_name=ANIM)

    arm.data.pose_position = 'REST'
    bpy.context.view_layer.update()
    masse_ruhe = mb.masse([koerper])
    blend = mb.blend_speichern(args, ordner / (ASSET + ".blend"))

    vorschau = []
    bogen = None
    if args.vorschau:
        attrappe = schlaeger_attrappe(arm)
        bpy.context.view_layer.update()
        kopf = arm.data.bones["head"].head_local
        vorschau = mb.vorschau_rendern(args, ASSET, [koerper, attrappe], vorne=VORNE, oben=OBEN,
                                       nah=(kopf + Vector((0.0, 0.0, -0.10 * H)), 0.17 * H))
        arm.data.pose_position = 'POSE'
        rahmen = _rahmen_ueber_frames([koerper, attrappe], bereich)
        zeilen = []
        zeitpunkte = [bereich[0] + i * (bereich[1] - bereich[0]) // 4 for i in range(4)]
        einzelbilder = []
        for ansicht in ("vorne", "seitlich"):
            zeile = []
            for f in zeitpunkte:
                szene.frame_set(f)
                bilder = mb.vorschau_rendern(args, ANIM, [koerper, attrappe], vorne=VORNE, oben=OBEN,
                                             ansichten=(ansicht,), aufloesung=max(256, args.aufloesung // 2),
                                             dateiname="%s_f%02d" % (ANIM, f), rahmen_bbox=rahmen)
                zeile.extend(bilder)
            zeilen.append(zeile)
            einzelbilder.extend(zeile)
        bogen = mb.kontaktbogen(zeilen, mb.vorschauordner(args) / ("%s_bogen.png" % ANIM))
        for p in einzelbilder:
            p.unlink()
        szene.frame_set(bereich[0])
        bpy.data.objects.remove(attrappe, do_unlink=True)

    gemeinsam = dict(
        quelle="neu erstellt per Skript (Unreal/blender/scripts/wuerfel_spieler.py), M0-Pipeline-Probe",
        lizenz="eigenes Werk (Matchball Tennis)",
        unreal_ziel=UNREAL_ZIEL,
    )
    knochen_info = [{"name": b.name, "eltern": b.parent.name if b.parent else None,
                     "verformt": b.use_deform} for b in arm.data.bones]
    mb.meta_schreiben(
        ordner, ASSET, "skeletal",
        parameter={"groesse_m": H, "pose": args.pose, "proportionen": "Drillis & Contini (1966)",
                   "proportionen_anteile": P, "blickrichtung": "+X (Blender und Unreal)"},
        dateien={"fbx": fbx_sk.name, "blend": blend.name if blend else None,
                 "vorschau": [mb.relativ_zur_ausgabe(args, p) for p in vorschau]},
        skelett=SKELETT, skelett_ziel="%s/%s" % (UNREAL_ZIEL, SKELETT), knochen=knochen_info,
        anhaenge_punkte={
            "racket_r": "Ursprung = Griffende, lokal Z = Schlaeger-Laengsachse, lokal Y = Saitenbett-Normale; "
                        "SM_Schlaeger ohne Versatz anhaengen",
            "racket_l": "wie racket_r fuer Linkshaender",
            "ball_hand": "Ursprung = Ballmittelpunkt in der linken Handflaeche; SM_Ball ohne Versatz anhaengen"},
        materialien=mb.materialien_info([koerper]), dreiecke=mb.dreiecke(koerper), masse=masse_ruhe,
        export=exp_sk, **gemeinsam)
    mb.meta_schreiben(
        ordner, ANIM, "animation",
        parameter={"dauer_s": args.dauer, "fps": args.fps, "schleife": True,
                   "inhalt": "Bereitschaftsstellung, Gewichtsverlagerung links/rechts, Arme leicht bewegt",
                   "erzeugung": "FK-Kurven + Bein-IK je Frame ausgewertet und als Aktion gebacken"},
        dateien={"fbx": fbx_anim.name, "blend": blend.name if blend else None,
                 "vorschau": [mb.relativ_zur_ausgabe(args, bogen)] if bogen else []},
        skelett=SKELETT, skelett_ziel="%s/%s" % (UNREAL_ZIEL, SKELETT), skelett_mesh=ASSET,
        frames=list(bereich), pruefung=pruefung, export=exp_anim, **gemeinsam)


def parser():
    p = mb.basis_parser("M0-Probe: Wuerfel-Spieler (Skelett mit UE5-Knochennamen, Quader-Koerper, Testanimation)")
    p.add_argument("--groesse", type=float, default=1.85, help="Koerpergroesse in m (Standard 1.85)")
    p.add_argument("--pose", choices=("A", "T"), default="A", help="Ruhepose: A (Standard) oder T")
    p.add_argument("--fps", type=int, default=30, help="Bildrate der Animation (Standard 30)")
    p.add_argument("--dauer", type=float, default=2.0, help="Dauer der Testanimation in s (Standard 2)")
    return p


if __name__ == "__main__":
    mb.ausfuehren(main, parser())
