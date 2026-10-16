# Blender-Pipeline (README 3.1–3.4, Meilenstein M0)

Alle Blender-Assets entstehen per Python-Skript, ohne Oberfläche, und können jederzeit neu erzeugt werden.

```
blender/
  lib/mb_gemeinsam.py        gemeinsame Hilfen: Argumente, Szene, Einheiten, FBX-Export, meta.json, Vorschau, LODs
  scripts/wuerfel_spieler.py M0-Probe: Skelett mit UE5-Knochennamen, Quader-Körper, Testanimation
  scripts/schlaeger.py       Platzhalter-Schläger + eigenes Schlägermodell (Vorgaben/Schlaeger/ bzw. Vorgaben/racket.js),
                             Saitenbett als gebackene Alpha-Fläche
  scripts/ball.py            Tennisball mit Naht, Filz, Fussel-Hülle (Shell), Formschlüssel „gestaucht“, LODs
  skripte.json               Reihenfolge und Argumente für Tools/assets.ps1
  anims/                     später: Posen als Daten (README 4.1)
```

## Aufruf

Normalerweise über `Unreal\Tools\assets.ps1` (alle Skripte aus `skripte.json`). Einzeln, aus `C:\Dev\MatchballTennis\Unreal`:

```
blender.exe -b --factory-startup --python-exit-code 1 -P blender\scripts\ball.py -- --engine AUTO
```

- `--factory-startup`: eigene Einstellungen und Add-ons bleiben außen vor (die Skripte leeren die Szene selbst,
  machen aber keinen Factory-Reset).
- `--python-exit-code 1`: ein Fehler im Skript beendet Blender mit Code 1 (sonst meldet Blender immer 0).
- Alles hinter `--` sind Argumente des Skripts.

Gemeinsame Argumente (alle Skripte; `--help` zeigt die skripteigenen):

| Argument | Umgebungsvariable | Standard | Bedeutung |
|---|---|---|---|
| `--ausgabe <ordner>` | `MB_AUSGABE` | `Unreal/SourceAssets` | Wurzel für `_erzeugt/` und `_vorschau/` |
| `--engine AUTO\|EEVEE\|CYCLES\|WORKBENCH` | `MB_ENGINE` | `AUTO` | Render-Engine der Vorschau. AUTO = Eevee (mit Grafikkarte), sonst Cycles auf der CPU |
| `--samples <n>` | `MB_SAMPLES` | Eevee 64, Cycles 48 | Samples der Vorschau |
| `--aufloesung <px>` | `MB_AUFLOESUNG` | `1024` | Kantenlänge der Vorschaubilder |
| `--keine-vorschau` | `MB_VORSCHAU=0` | – | keine Bilder rendern |
| `--kein-blend` | `MB_BLEND=0` | – | keine `.blend` speichern |
| – | `MB_ARGS` | – | weitere Argumente als eine Zeichenkette |

Reihenfolge: Befehlszeile vor Umgebungsvariable vor Standardwert.

Skripteigene Argumente: `wuerfel_spieler.py --groesse 1.85 --pose A|T --fps 30 --dauer 2`,
`schlaeger.py --quelle <datei> --nur-platzhalter --laenge 0.6858 --lods 1,0.5,0.25,0.12 --texturen-einbetten
--saiten-aufloesung 2048 --saiten-samples 4 --saiten-geometrie`,
`ball.py --durchmesser 0.067 --naht-b 0.30 --stauchung 0.30 --lods 1,0.5,0.25,0.12 --fussel-hoehe 0.0007
--fussel-unterteilung 16 --fussel-lods 2 --fussel-dichte 0.42 --fussel-aufloesung 2048`.

### Ohne Blender-Programm testen (bpy als Python-Modul)

```
python3.13 -m venv bpyvenv && bpyvenv/bin/pip install bpy==5.2.2
bpyvenv/bin/python Unreal/blender/scripts/ball.py --ausgabe /tmp/test --engine CYCLES
```

Im Modul-Modus gibt es kein `--`; die Argumente stehen direkt hinter dem Skript (oder in den Umgebungsvariablen).
Eevee braucht dort OpenGL über EGL (unter Linux z. B. Mesa `libegl1`); fehlt EGL, wählt `AUTO` Cycles,
weil Eevee ohne EGL Blender hart abstürzen lässt.

## Ausgabe

```
SourceAssets/_erzeugt/<Bereich>/   FBX, LOD-FBX, Texturen, .blend, <Asset>.meta.json   (nicht versioniert, .gitignore)
SourceAssets/_vorschau/            <Asset>_vorne|seitlich|dreiviertel|nah.png, 1024 x 1024 (versioniert, LFS)
```

| Skript | Bereich | Assets | Ziel in Unreal |
|---|---|---|---|
| `wuerfel_spieler.py` | `Characters/Wuerfel` | `SK_WuerfelSpieler` (Skelett `SKEL_WuerfelSpieler`), `A_Wuerfel_Bereitschaft` | `/Game/Characters/Wuerfel` |
| `schlaeger.py` | `Equipment/Schlaeger` | `SM_SchlaegerPlatzhalter`, `SM_Schlaeger` (+ `_LOD1..3`), `T_Schlaeger_*`, `T_Schlaeger_Saiten_BaseColor`, `T_Schlaeger_Saiten_Normal`, `T_SchlaegerPlatzhalter_Saiten_BaseColor` | `/Game/Equipment/Schlaeger` |
| `ball.py` | `Equipment/Ball` | `SM_Ball` (+ `_LOD1..3`), `T_Ball_Fussel_Maske` | `/Game/Equipment/Ball` |

Zusätzliche Bilder: `A_Wuerfel_Bereitschaft_bogen.png` (4 Zeitpunkte, vorne und seitlich),
`SM_Ball_gestaucht.png`, `SM_Schlaeger_LOD3_vorne.png`.

### meta.json

Jedes Asset hat `<Asset>.meta.json` (UTF-8, nur ASCII). Wichtige Felder:

- Kurzform: `name`, `typ` (`static` | `skeletal` | `animation`), `ziel` (Unreal-Ordner), `datei` (Haupt-FBX)
- `quelle`, `lizenz`, `datum`, `skript`, `blender`, `parameter`
- `dateien`: `fbx`, `lods` (Liste mit `stufe`, `datei`, `verhaeltnis`, `dreiecke`), `texturen` (Datei, Rolle
  `BaseColor` | `Alpha` | `Normal` | …, Kanäle, sRGB ja/nein, Material, Hinweise), `blend`, `vorschau` (relativ zur
  Ausgabewurzel). Steckt die Bedeckung im Alphakanal der BaseColor, steht dieselbe Datei zweimal da: einmal mit
  Rolle `BaseColor` (`kanaele` RGB), einmal mit Rolle `Alpha` (`kanaele` A, Hinweis „Opacity Mask“).
- `materialien` (Slot-Reihenfolge, Grundfarbe sRGB, Rauheit, Metall, Alpha, zweiseitig), `masse`, `dreiecke`
- `export`: alle FBX-Einstellungen, `achsen`: Umrechnung Blender → Unreal
- Skelett: `skelett`, `skelett_ziel`, `knochen`, `anhaenge_punkte`; Animation: `frames`, `skelett_mesh`, `pruefung`

LOD-Stufen liegen als eigene Dateien `<Asset>_LOD<n>.fbx` vor (der Blender-FBX-Export kennt keine LOD-Gruppen);
der Import hängt sie per `import_lod` an. Der Import liest die `*.meta.json`, nicht einfach alle `*.fbx`.

## Alpha-Materialien: Saitenbett und Fussel-Rand (README 3.4)

**Saitenbett (`schlaeger.py`).** Beide Schläger haben eine ebene Saitenfläche in der Saitenebene (Normale −Y, U entlang
+X, V entlang +Z) mit gleich aufgebautem Material: `M_<Stamm>_Saiten`, BaseColor-Textur `T_<Stamm>_Saiten_BaseColor.png`
(RGB = Saitenfarbe, A = Bedeckung), beim eigenen Modell zusätzlich `T_Schlaeger_Saiten_Normal.png`.

- Platzhalter: kachelnde 64-px-Textur (1 UV-Einheit = 1 Saitenfeld), Material `M_SchlaegerPlatzhalter_Saiten`.
- Eigenes Modell: Die Saitengeometrie wird erkannt (dünne Inseln in der Saitenebene, die in die Kopföffnung ragen;
  Innenkontur per Strahl gegen den Rahmen – Kriterium und Zahlen in `saitenteile_finden()` und in der meta.json unter
  `parameter.aufbereitung.saitenbett`), mit Cycles *selected-to-active* auf 2048 × 2048 gebacken (Bedeckung binär je
  Texel, Farbe inkl. Logo, Normale im Tangentenraum) und danach entfernt. Ösenköpfe zählen mit; Saitenschlaufen in der
  äußeren Rahmenrille bleiben Geometrie. Alle vier LODs nutzen dieselbe Fläche (Slot 1), nur der Rahmen wird dezimiert.
  Beim Modell aus `racket.js`: LOD0 115 912 → 84 050 Dreiecke.

**Fussel-Rand (`ball.py`).** Zweite Hülle 0,7 mm über dem Filz im selben Mesh, Materialslot 2 `M_Ball_Fussel`,
Opazität aus `T_Ball_Fussel_Maske.png` (8 Bit grau, linear; selbst erzeugtes 3D-Rauschen, an der Naht 0). Nur LOD0
(3 072 Dreiecke Hülle) und LOD1 (1 728); der Formschlüssel „gestaucht“ nimmt die Hülle mit. Begründung im Kopf von
`ball.py`.

**Einstellungen in Unreal** (auch in der meta.json unter `saiten.unreal`, `fussel.unreal`, `materialien[].unreal`):

| Material | Blend Mode | Opacity Mask | Clip | Two Sided | Texturen |
|---|---|---|---|---|---|
| `M_Schlaeger_Saiten`, `M_SchlaegerPlatzhalter_Saiten` | Masked | A von `…_Saiten_BaseColor` | 0,5 | ja | BaseColor sRGB an; Normal: Normalmap, sRGB aus, Flip Green Channel |
| `M_Ball_Fussel` | Masked | R von `T_Ball_Fussel_Maske` | 0,5 | ja | Maske: Masks/Grayscale, sRGB aus |

## Achsen und Maßstab

Ausführlich in `lib/mb_gemeinsam.py` (`ACHSEN_UND_MASSSTAB`). Kurz:

- Blender arbeitet in **Metern**, Z oben. Die FBX-Dateien sind in **Zentimetern** (UnitScaleFactor 1, keine
  Skalierung an Knoten): exportiert werden temporäre ×100-Kopien, `global_scale = 0.01` hebt den Faktor 100 von
  `apply_unit_scale` auf. Ergebnis in Unreal: 1 m = 100 Einheiten, unabhängig von „Convert Scene Unit“.
- `axis_up='Z'`, `axis_forward='Y'` (Blenders eigenes System = Importsystem von Unreal). Unreal dreht nichts und
  spiegelt nur Y: **Blender (x, y, z) m → Unreal (100x, −100y, 100z) cm**. Blender +X = Unreal +X (vorne).
- Figuren schauen in Blender nach **+X** und damit auch in Unreal (keine −90°-Drehung der Mesh-Komponente).
- Knochen: `primary_bone_axis='Y'`, `secondary_bone_axis='X'`, `add_leaf_bones=False`, Armatur-Objekt
  `Armature` als Null-Knoten (oberster Knochen `root`).
- Unreal-Import: „Convert Scene“ an (Standard), **„Force Front X Axis“ aus**, Normalen importieren.
- Schläger: Ursprung am Griffende, Längsachse +Z, Kopfbreite X, Saitenbett-Normale Y. Der Knochen `racket_r`
  hat dieselbe Ausrichtung (Ursprung = Griffende, lokal Z = Schaft) – Schläger ohne Versatz anhängen.
  `ball_hand` = Ballmittelpunkt in der linken Hand.

## Qualitätsschleife (README 3.1)

1. Skript laufen lassen (`assets.ps1 -Skript ball -NurBlender`).
2. Vorschaubilder in `SourceAssets/_vorschau/` ansehen (Claude Code liest sie selbst), nachbessern, erneut laufen lassen.
3. Erst nach der Abnahme durch den Nutzer gilt das Asset als fertig; die Bilder werden mit committet (Git LFS).

Selbstprüfungen in den Skripten: `wuerfel_spieler.py` bricht ab, wenn die Bein-IK die Füße mehr als 2 mm
gleiten lässt oder die Knie nach hinten bzw. seitlich ausweichen (Werte stehen in der meta.json unter `pruefung`).
Die Vorschau nutzt die Farbdarstellung „Standard“, damit Materialfarben unverfälscht erscheinen.

## Bekannte Grenzen (M0)

- Unreal-Static-Meshes haben keine Morph Targets: Der Formschlüssel `gestaucht` des Balls ist eine reine
  Ellipsoid-Skalierung (Z 0,70, quer 1,195) und wird im Spiel durch Skalieren der Ball-Komponente erzeugt.
- Die Metall/Rauheit-Textur des eigenen Schlägers steht nicht in der FBX (der Exporter folgt nur direkten
  Verbindungen); sie liegt als `T_Schlaeger_MetallicRoughness.jpg` daneben und ist in der meta.json aufgeführt.
  Die Normal-Map ist im OpenGL-Format (in Unreal „Flip Green Channel“).
- Saitenbett und Fussel-Rand sind maskierte Materialien (siehe unten). Von hinten erscheint das Logo auf den Saiten
  gespiegelt – wie beim Originalmodell, bei dem es als Farbe auf den Saiten liegt.
- Die Fussel-Hülle macht die Bounds des Balls 1,4 mm größer (0,0684 statt 0,067 m); den Ball nicht aus den Bounds
  skalieren. Masked-Materialien verlieren mit den Mip-Stufen Deckung – Saiten und Fussel werden in der Ferne dünner
  bzw. verschwinden (gewollt; ab LOD2 hat der Ball keine Hülle mehr).
- Der Würfel-Spieler hat nur die Knochen des UE5-Mannequins, die M0 braucht (ohne `neck_02`, Finger, Zehen,
  `ik_*` und Twist-Knochen).
