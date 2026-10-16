# Architektur der Unreal-Fassung

Ergänzt README 2.3 und 20 um die Regeln, nach denen der Code tatsächlich aufgebaut ist.

## 1. Zwei Schichten: Kern und Unreal

```
Unreal/Source/MatchballTennis/
  <Bereich>/Kern/   reines C++ (std::), keine Unreal-Header  ── deterministische Spiellogik
  <Bereich>/        Unreal-Klassen (AActor, UObject ...)      ── Darstellung, Eingabe, Ton, UI
```

**Kern** (`*/Kern/`): Ballphysik, Vorhersage, Schlagberechnung, Kontaktfläche/Hawk-Eye, Platzgeometrie,
Zählweise, Match-Zustandsmaschine, Punktrichter, Linienrichter, Challenge, Ansagen-Bausteine,
Rangliste, Auslosung, Match-Simulation, Zufall. Regeln:

* keine Unreal-Header, nur Standard-C++ (C++20),
* im Simulationspfad nur +, −, ×, ÷, √ – **deterministisch** auf jedem PC (README 6.1),
* Zufall nur über `MB::FZufall` (PCG32) mit Startwert – online pro Aufschlag aus Match-Startwert
  und Aufschlagnummer abgeleitet,
* Kommentare ohne Umlaute (MSVC-Codepage), Namen deutsch, Unreal-Stil (F-Präfix, PascalCase, b-Bools),
* Hilfsfunktionen in `.cpp` mit eindeutigem Präfix (`MBSchlag_…`), weil Unreal mehrere `.cpp` zu einer
  Einheit zusammenfasst (Unity Build).

**Unreal-Schicht**: liest Werte aus den Data Tables, gibt sie an den Kern weiter, stellt den Kern-Zustand
dar (Ball, Figuren, Kamera, HUD) und übersetzt Eingaben in Kern-Aufrufe.

## 2. Koordinaten

| | Kern | Unreal |
|---|---|---|
| Einheit | Meter | Zentimeter |
| Händigkeit | rechtshändig | linkshändig |
| Ursprung | Platzmitte unter dem Netz, Boden | gleich |
| quer | X (rechts aus Sicht der nahen Grundlinie) | Y |
| längs | Y (zur fernen Grundlinie, nahe Seite Y < 0) | X |
| hoch | Z | Z |

Umrechnung nur über `Core/MBKernBruecke.h` (`NachUnreal`, `NachKern`, `DrehungFuerDarstellung`).

## 3. Daten

`Unreal/Data/*.csv` → `Tools/Python/import_daten.py` → `/Game/Data/DT_<Name>` (Zeilenstrukturen in
`Core/MBDatenTabellen.h`). `UMBGameInstance` lädt die Tabellen beim Start und fällt auf die
Standardwerte des Kerns zurück, wenn eine Tabelle fehlt. Die Kern-Tests lesen **dieselben** CSV-Dateien.

## 4. Tests

Ein Testquelltext, zwei Wege (`Source/MatchballTennisTests/Kern/MBTestRahmen.h`):

* **Unreal Automation** (Modul `MatchballTennisTests`, Editor): `Tools/test.ps1` bzw. Session Frontend,
  Filter „MatchballTennis".
* **Eigenständig** ohne Unreal: `Tools/KernTests` (CMake) – läuft in der Cloud-Sitzung, auf GitHub
  (`.github/workflows/kern-tests.yml`, GCC + MSVC) und lokal.

## 5. Zeitablauf im Spiel

* Ballsimulation mit festem Zeitschritt 240 Hz im GameMode (Akkumulator, höchstens 0,25 s nachholen),
  Darstellung interpoliert zwischen den letzten zwei Schritten.
* Match-Ablauf (`Match/Kern/MBMatchAblauf`): Zustände nach README 2.3; der Punkt wird erst in der
  „Ansage" gezählt, damit eine Challenge ihn noch ändern kann; Meldungen (Rufe, Ansagen, Challenge,
  Seitenwechsel …) holt die Unreal-Schicht ab und setzt sie in Ton, Animation und Einblendungen um.

## 6. Assets

Alles in `Content/` entsteht per Skript (README 0.2):
`blender/scripts/*.py` → `SourceAssets/_erzeugt/**` (+ `*.meta.json`) → `Tools/Python/import_assets.py`
→ `Content/**`. Level: `Tools/Python/build_graubox.py` (M1), später `build_venue.py` (M4).
Chunks für das Update-System: `Data/chunks.json` → `Tools/Python/setze_chunks.py`.
