# Stand der Entwicklung

**Stand:** 16.10.2026 · **Sitzung:** reine Cloud-Sitzung (README 0.0a, Weg 2) · **Version:** 1.0.0 (Entwicklung)
**Branch:** `claude/gifted-hamilton-uy5utv`

> ⚠️ **Alles, was Unreal betrifft, ist ungetestet – muss lokal gebaut werden.**
> Die Cloud kann Unreal nicht starten und nichts für Windows kompilieren. Was hier trotzdem geprüft werden konnte,
> steht unten ausdrücklich unter „In der Cloud geprüft“ – alles andere gilt nach README 0.0a als **nicht fertig**,
> bis es lokal gebaut und getestet ist.

---

## 1. Lokal auszuführen (in dieser Reihenfolge)

Alle Befehle in **PowerShell** im Ordner der Arbeitskopie (z. B. `C:\Dev\MatchballTennis\`, **nicht** unter OneDrive).
Bei Fehlern bitte die **komplette Ausgabe** (oder das genannte Log) zurückgeben.

| # | Befehl | Was passiert | Erwartet |
|---|---|---|---|
| 1 | `git clone https://github.com/gb9ktfzsby-collab/Matchball-Tennis C:\Dev\MatchballTennis`<br>`cd C:\Dev\MatchballTennis`<br>`git lfs install`<br>`git checkout claude/gifted-hamilton-uy5utv` | Arbeitskopie anlegen (bzw. `git pull`) | – |
| 2 | `powershell -ExecutionPolicy Bypass -File Unreal\Tools\pruefe_installation.ps1` | prüft UE 5.8, Blender 5.2, Visual Studio/MSVC, Windows SDK, Git/LFS, gh, lange Pfade, OneDrive, Speicher, Grafikkarte | Markdown-Block → bitte an mich schicken, kommt hier in Abschnitt 2 |
| 3 | `powershell -ExecutionPolicy Bypass -File Unreal\Tools\test.ps1 -NurKern` | Kern-Tests ohne Unreal (CMake + MSVC) | `57 Tests, 324818 Pruefungen, 0 fehlgeschlagen` |
| 4 | `powershell -ExecutionPolicy Bypass -File Unreal\Tools\build.ps1 -Projektdateien` | erzeugt die Projektdateien und kompiliert den Editor | `MatchballTennisEditor kompiliert` – **erster echter Kompilierlauf der Unreal-Schicht** |
| 5 | `powershell -ExecutionPolicy Bypass -File Unreal\Tools\assets.ps1` | Blender-Skripte (Würfel-Spieler, Schläger, Ball) → FBX + Vorschaubilder, dann Unreal-Import (Data Tables, Meshes, Map `M_Graubox`, Chunk-Labels) | alle Schritte `OK`; Vorschaubilder unter `Unreal\SourceAssets\_vorschau\` |
| 6 | `powershell -ExecutionPolicy Bypass -File Unreal\Tools\test.ps1` | Kern-Tests + Unreal-Automation-Tests (`MatchballTennis.*`) | alles grün |
| 7 | `Unreal\MatchballTennis.uproject` doppelklicken → **Play** | Training mit Ballmaschine auf dem Graubox-Platz | siehe Abschnitt 4 „Was du sehen solltest“ |
| 8 | `Unreal\Spiel für Freunde packen.bat` | Shipping-Build nach `C:\MatchballTennis-Build\1.0.0\`, ZIP auf den Desktop | `Matchball Tennis.exe` startet im randlosen Vollbild (Abnahme M0) |

Logs liegen unter `Unreal\Saved\Werkzeuge\` und `Unreal\Saved\Testberichte\`.

## 2. Installation (README 0.1)

**Noch nicht geprüft** – die Cloud sieht deinen PC nicht. Bitte Schritt 2 oben ausführen und die Ausgabe schicken;
sie wird dann hier eingetragen. Benötigt: Unreal Engine 5.8, Blender 5.2 LTS, Visual Studio 2022 mit
„Spieleentwicklung mit C++“ (MSVC laut Unreal-Vorgabe), Windows SDK, Git mit LFS, `gh` (angemeldet).
Optional: CMake (für `test.ps1 -NurKern`; im Visual Studio Installer „C++-CMake-Tools für Windows“).

## 3. Meilensteine

| # | Status | Kurz |
|---|---|---|
| **M0** | 🟡 geschrieben, **ungetestet** | Projekt, Konfiguration, Module, Werkzeug-Skripte, Blender-Pipeline, Unreal-Import, Packen |
| **M1** | 🟡 Kern fertig und getestet, Unreal-Schicht **ungetestet** | Platz (Graubox), Ballphysik, Vorhersage, Zählweise, TV-Kamera, Ballmaschine |
| M2–M11 | ⚪ offen | Kern-Bausteine für M3 (Hawk-Eye, Linienrichter, Challenge, Match-Ablauf) und M7 (Rangliste, Auslosung, Simulation) sind schon fertig und getestet |

### M0 – Projekt und Pipeline-Probe

| Teil | Dateien | Stand |
|---|---|---|
| Repository | `.gitignore`, `.gitattributes` (LFS), `CREDITS.md`, `Dokumente/` | ✅ |
| Unreal-Projekt | `Unreal/MatchballTennis.uproject` (UE 5.8, Enhanced Input, Python), `Config/Default*.ini` (DX12/SM6 + DX11-Rückfall, Lumen, randloses Vollbild, Chunks, Shipping), `Source/*.Target.cs`, `*.Build.cs` | 🟡 ungetestet |
| Werkzeuge | `Tools/pruefe_installation.ps1`, `build.ps1`, `test.ps1`, `assets.ps1`, `package.ps1`, `release.ps1` (Gerüst, M10), `elevenlabs-stimme.ps1`, gemeinsames Modul `MBGemeinsam.psm1`; Batch-Dateien `Spiel für Freunde packen.bat`, `Update veröffentlichen.bat`, `ElevenLabs-Stimme erzeugen.bat` | 🟡 auf GitHub-Windows-Runnern geprüft, soweit ohne Unreal möglich (siehe 5) |
| Blender | `blender/lib/mb_gemeinsam.py` (Export, Achsen, Maßstab, meta.json, Vorschau, LODs), `scripts/wuerfel_spieler.py`, `schlaeger.py`, `ball.py`, `skripte.json` | 🟡 siehe 5 |
| Unreal-Python | `Tools/Python/import_daten.py`, `import_assets.py`, `build_graubox.py`, `setze_chunks.py`, `import_alles.py` | 🟡 ungetestet |
| Update-Grundlage | `Data/chunks.json` (feste Chunk-IDs, README 19.4.1), `bGenerateChunks`, Build-Id + `build.json` + `installiert.json` in `package.ps1` | 🟡 |

### M1 – Platz, Ballphysik, Vorhersage, Zählweise, TV-Kamera, Ballmaschine

| Teil | Dateien | Stand |
|---|---|---|
| Ballphysik (240 Hz, RK4, Luftwiderstand, Magnus, Drall-Abnahme, Aufsprung mit Gleiten/Rollen, Netzband/-geflecht, Wind) | `Physics/Kern/MBBallPhysik.*` | ✅ getestet |
| Vorhersage, Treffpunktsuche | `Physics/Kern/MBVorhersage.*` | ✅ getestet |
| Schlagberechnung, Timing/Streuung | `Physics/Kern/MBSchlagberechnung.*` | ✅ getestet |
| Platzgeometrie (ITF-Maße) | `Physics/Kern/MBPlatz.*` | ✅ getestet |
| Zählweise (alle Optionen) | `Match/Kern/MBZaehlweise.*` | ✅ getestet |
| Graubox-Platz, Ball, Ballmaschine, Testspieler, TV-Kamera (4 Modi), Eingabe (Tastatur + Controller), Trainings-GameMode, Entwicklungs-HUD | `Venue/`, `Physics/MBBall.*`, `Player/`, `Camera/`, `Match/MBTrainingGameMode.*`, `UI/` | 🟡 ungetestet |
| Data Tables (8 Tabellen) + Rückfall auf Kern-Standardwerte | `Core/MBDatenTabellen.h`, `Core/MBGameInstance.*`, `Data/*.csv` | 🟡 (CSV ↔ Struktur in CI geprüft) |

**Abnahme M1 (README 21):** „Ball fliegt/springt plausibel (Tests gegen reale Richtwerte: Flugzeit, Absprunghöhe je
Belag, Aufschlagtempo), Zähl-Tests grün“ – die Tests dafür laufen und sind grün (`Dokumente/Werte.md`, Abschnitt 5);
die Abnahme im Spiel steht aus.

## 4. Was du beim ersten Start sehen solltest (Schritt 7)

* Blauer Hartplatz mit grünem Auslauf, weißen Linien, Netz mit Durchhang, Pfosten, Einzelstützen.
* Hinter der fernen Grundlinie die Ballmaschine; sie spielt alle ca. 3 s einen Ball abwechselnd auf Vor- und Rückhand.
* Gelbe/orange Kreise: vorhergesagte Aufsprungpunkte.
* Du (grauer Zylinder) läufst mit **WASD/Pfeilen/linkem Stick**; Schläge mit **J K L I U** bzw. **A B X Y LB/RB**,
  sobald „Ball in Reichweite“ erscheint. Power **Shift/Q/RT**, Präzision **Leertaste/E/LT**.
* **C** Kamera (TV, TV hoch, Spieler, Seite), **F5** nächstes Ballmaschinen-Programm, **F6** nächster Belag,
  **Esc/P** Pause, **F11** Vollbild/Fenster, **M** Ton.
* Oben links: Belag mit Court Pace Rating, Programm, Balltempo/-drall, Hawk-Eye-Urteil deines letzten Schlags.
* Startoptionen: `MatchballTennis.uproject -game -Belag=Sand -Programm=Topspin`.

## 5. In der Cloud geprüft

| Was | Wie | Ergebnis |
|---|---|---|
| Kern (Physik, Zählweise, Hawk-Eye, Linienrichter, Challenge, Match-Ablauf, Ansagen, Karriere) | 57 Tests mit GCC, Clang, GCC + Address/UB-Sanitizer; GitHub: Ubuntu (GCC) und **Windows (MSVC /W4 /WX)** | ✅ 57 Tests, 324 818 Prüfungen, 0 Fehler |
| Determinismus (README 6.1) | Fingerabdruck einer Flugbahn mit GCC, Clang, Debug/Release und **MSVC** | ✅ überall `599a2d964081de7e` (bitgleich) |
| Unity-Build wie in Unreal | alle Kern- und Testquellen in einer Übersetzungseinheit (GCC, Clang `-Wshadow-all`, MSVC) | ✅ keine Namenskollisionen |
| CSV ↔ Unreal-Strukturen | `Tools/pruefe_daten.py` (CI) | ✅ 8 Tabellen |
| PowerShell-Skripte | GitHub `windows-latest`: nur ASCII, Parser **Windows PowerShell 5.1** und PowerShell 7, PSScriptAnalyzer, `test.ps1 -NurKern` unter 5.1 und 7, saubere Abbrüche ohne Unreal, `package.ps1 -NurNachbearbeitung` an einem nachgebauten Spielordner (Umbenennen, Build-Id, `build.json`, `installiert.json`, ZIP) | siehe CI-Lauf |
| Build-Id | unabhängig in Python nachgerechnet | ✅ gleich |

**Nicht prüfbar in der Cloud:** Kompilieren der Unreal-Schicht (UHT/UBT/MSVC), Unreal-Python-Skripte im Editor,
FBX-Import, Packen, Aussehen und Spielgefühl.

## 6. Bekannte Risiken beim ersten lokalen Lauf

* **Unreal-API-Details 5.8:** Der Code ist gegen die APIs von UE 5.4–5.7 geschrieben (Enhanced Input, Automation-Test-Flags,
  Himmelslicht, DrawDebug). Kleine Abweichungen in 5.8 sind möglich → Kompilierfehler bitte schicken.
* **FBX-Import über Interchange** (Standard seit UE 5.5): `import_assets.py` hat eine Weiche; Achsen/Maßstab sind in
  `blender/lib/mb_gemeinsam.py` dokumentiert und müssen im Editor einmal angesehen werden.
* **Startprogramm umbenennen** (`MatchballTennis.exe` → `Matchball Tennis.exe`): beim ersten Packen prüfen (Entscheidungen B15).
* **DirectX 11**: Rückfall eingetragen, Lumen dort unzuverlässig (Entscheidungen B11).

## 7. Offene Fragen an dich

Alle mit Vorschlag in `Dokumente/Entscheidungen.md`:
A1 Zielhardware · A2 welche [NEU]-Punkte · A3 Mixamo (Vorschlag: verzichten) · A5 Originaldatei des Schlägers ·
A6 Kader abnehmen · A7 Karriere-Länge · A9 Update-Speicherort · B1 Challenge beim Aufschlag · B7 Linien in Ansagen ·
B8 Geschlecht/Vornamen von Holloway und Moreau.

## 8. Nächste Schritte

1. Rückmeldung zu Abschnitt 1 (Kompilier-/Importfehler beheben, bis M0 und M1 lokal abgenommen sind).
2. M2: Spielerfigur v1 aus Blender (Skelett, Gewichte), Motion Matching, Grundschläge mit Warping, Timing-Meter, einfache KI.
