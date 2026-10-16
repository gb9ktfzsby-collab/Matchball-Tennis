# Matchball Tennis – Unreal-Fassung

**Bauplan und vollständige Spielbeschreibung für die Neuentwicklung mit Unreal Engine 5 und Blender.**

Dieses Dokument beschreibt *alles*, was das Spiel am Ende können soll. Es ist zugleich die Arbeitsanweisung für
Claude Code: Lies es vollständig, bevor du anfängst, und halte dich an Reihenfolge, Pipeline und Regeln.
Die Unreal-Fassung ist eine **komplett neue Fassung, die von Grund auf in der Cloud entsteht – ohne Referenz zum
bisherigen Spiel**. Die bisherigen Fassungen (Browser, Godot) laufen weiter **lokal auf meinem PC**, sind nicht Teil
des Repositorys und werden **weder gelesen noch verändert noch als Vorlage benutzt**. Maßgeblich ist allein dieses
Dokument: Regeln, Spielgefühl, Modi und Zählweise stehen hier vollständig; alle Zahlenwerte, Tabellen, Figuren, Grafiken,
Animationen und Klänge werden neu erarbeitet. **Einzige Vorgabe von außen: das Schlägermodell** (3.4). Die neue Fassung
hat eine eigene Versionszählung (Start: 1.0) und kann nicht mit den alten Fassungen online spielen.

Markierungen in diesem Dokument:

- ohne Markierung = **fester Bestandteil** der neuen Fassung (Regeln und Verhalten, die ich so haben will)
- **[NEU]** = Ausbau bzw. Vorschlag über den festen Kern hinaus – darf gestrichen werden, wenn er nicht gewollt ist
- **[PRÜFEN]** = Wert oder Detail, das Claude Code vor der Umsetzung recherchieren bzw. mit mir abstimmen muss

---

## Inhalt

0. [Arbeitsanweisung für Claude Code](#0-arbeitsanweisung-für-claude-code)
1. [Vision und Grundsätze](#1-vision-und-grundsätze)
2. [Technik: Unreal Engine 5](#2-technik-unreal-engine-5)
3. [Asset-Pipeline mit Blender](#3-asset-pipeline-mit-blender)
4. [Animationen](#4-animationen)
5. [Spielmechanik und Regeln](#5-spielmechanik-und-regeln)
6. [Ballphysik](#6-ballphysik)
7. [Linienrichter, Challenge und Videobeweis](#7-linienrichter-challenge-und-videobeweis)
8. [Zählweise](#8-zählweise)
9. [Spieler, Kader und KI](#9-spieler-kader-und-ki)
10. [Kleidung und Outfits](#10-kleidung-und-outfits)
11. [Verhalten zwischen den Punkten](#11-verhalten-zwischen-den-punkten)
12. [Anlagen, Plätze und Hallen](#12-anlagen-plätze-und-hallen)
13. [Spielmodi](#13-spielmodi)
14. [Karrieremodus](#14-karrieremodus)
15. [Online gegen Freunde](#15-online-gegen-freunde)
16. [Ton, Schiedsrichter-Stimme, Publikum](#16-ton-schiedsrichter-stimme-publikum)
17. [Menüs, HUD und Controller](#17-menüs-hud-und-controller)
18. [Grafik-Einstellungen und Leistung](#18-grafik-einstellungen-und-leistung)
19. [Start, Speichern, Weitergeben, Updates](#19-start-speichern-weitergeben-updates)
20. [Projektstruktur](#20-projektstruktur)
21. [Meilensteine](#21-meilensteine)
22. [Lizenzen und Quellen](#22-lizenzen-und-quellen)
23. [Offene Entscheidungen](#23-offene-entscheidungen)
24. [Anhang: Maße und Konstanten](#24-anhang-maße-und-konstanten)

---

## 0. Arbeitsanweisung für Claude Code

### 0.0a Arbeitsweise: Entwicklung in der Cloud, Spiel lokal

- **Entwickelt wird in Claude-Code-Sitzungen in der Cloud – von Grund auf neu, ohne Zugriff auf die alten Fassungen.** Dort entstehen Code (C++), Skripte (Blender-Python,
  Unreal-Python, PowerShell), Daten (CSV/JSON) und Dokumente; alles liegt im GitHub-Repository (0.0).
- **Das Spiel selbst läuft lokal auf meinem PC.** Unreal Engine, Blender, Visual Studio und die gepackten Builds gibt es
  nur dort. Die Cloud kann Unreal und Blender **nicht** starten, nichts kompilieren und nichts spielen.
- Es gibt deshalb zwei Wege, und Claude Code nimmt den, der gerade möglich ist:
  1. **Sitzung mit verbundenem PC:** Ist die Sitzung mit meinem Computer verknüpft (Claude Desktop-App), führt Claude
     Code Befehle direkt dort aus (`build.ps1`, `assets.ps1`, Blender headless, Unreal-Cmd, Tests), liest die
     Ergebnisse und Vorschaubilder selbst und korrigiert. Der Arbeitsordner dort ist die lokale Arbeitskopie
     (`C:\Dev\MatchballTennis\`, siehe 0.0).
  2. **Reine Cloud-Sitzung:** Claude Code schreibt und committet nur. Er kennzeichnet in `STAND.md` klar alles als
     **„ungetestet – muss lokal gebaut werden“** und legt eine kurze Liste **„Lokal auszuführen“** an (z. B.
     `git pull`, `Tools\build.ps1`, `Tools\assets.ps1`, Tests). Ich führe das aus und gebe Fehlermeldungen bzw.
     Screenshots zurück.
- **Nie raten:** Was nicht lokal gebaut und getestet wurde, gilt nicht als fertig. Meilenstein-Abnahmen (21) finden
  immer lokal statt.
- **Große Dateien** (gerenderte Vorschaubilder, Videos, Aufnahmen) kommen über das Repository bzw. werden von mir
  hochgeladen, nicht über Cloud-Zwischenspeicher.
- **Zugangsdaten** (EOS, Signatur-Schlüssel, ElevenLabs, GitHub) liegen nur lokal und werden nie in die Cloud geschickt
  oder committet.

### 0.0 Projektort: GitHub-Repository

Alles zur neuen Fassung liegt in **einem privaten GitHub-Repository**. Unter diesem Link findest du alles –
diese README, die Vorgaben und das Unreal-Projekt:

```
https://github.com/gb9ktfzsby-collab/Matchball-Tennis
```

Aufbau des Repositorys:

```
/                         (Wurzel des Repositorys)
  README.md               dieses Dokument
  STAND.md                aktueller Stand (von Claude Code gepflegt)
  CREDITS.md              alle fremden Quellen mit Lizenz
  Vorgaben/               was ich selbst beisteuere: Schlaeger/ (das Schlägermodell), später ggf. Aufnahmen, Logos
  Dokumente/              von Claude Code erzeugte Dokumente (Aufnahmeliste, Entscheidungen, Werte-Tabellen)
  Unreal/                 das neue Unreal-Projekt (Aufbau siehe 20)
  .gitignore  .gitattributes
```

**Erstbefüllung:** Die alten Fassungen kommen **nicht** ins Repository. Ich lege nur das **Schlägermodell** (Originaldatei
mit allen Texturen) nach `Vorgaben/Schlaeger/` und committe es. Bis es da ist, arbeitet Claude Code mit einem
Platzhalter-Schläger (einfacher Quader mit Saitenbett) und baut alles andere zuerst.

**Arbeitskopie auf jedem PC:**

- Das Repository wird **außerhalb von OneDrive** mit kurzem Pfad geklont, z. B. `C:\Dev\MatchballTennis\`.
  Grund: Unreal erzeugt beim Arbeiten mehrere GB Zwischendateien, die OneDrive sonst ständig mitsynchronisiert
  (langsam, gesperrte Dateien, voller Speicher), und Unreal bekommt bei sehr langen Pfaden Probleme
  (Windows-Grenze 260 Zeichen). Prüfen, ob „Lange Pfade“ (`LongPathsEnabled`) aktiviert ist.
- **`.gitignore`** schließt aus: `Unreal/Binaries/`, `Unreal/Intermediate/`, `Unreal/Saved/`, `Unreal/DerivedDataCache/`,
  `Unreal/Build/`, `Unreal/.vs/`, `*.sln`, Zugangsdaten (EOS-Konfiguration, Signatur-Schlüssel, ElevenLabs).
  Diese Ordner erzeugt Unreal auf jedem PC selbst neu.
- **Git LFS** (`.gitattributes`) für große Binärdateien: `.blend`, `.fbx`, `.bvh`, `.wav`, `.ogg`, `.png`, `.jpg`, `.exr`,
  `.uasset`, `.umap`, `.pak`, `.zip`. **[PRÜFEN]** Das kostenlose LFS-Kontingent von GitHub ist begrenzt – vor dem
  ersten großen Push Größe abschätzen und mir sagen, ob es reicht. Da `Content/` komplett aus Skripten entsteht
  (0.2), kann `Unreal/Content/` notfalls ganz aus dem Repository herausbleiben und auf jedem PC neu erzeugt werden.
- **Gepackte Builds** (`package.ps1`, `release.ps1`) gehen in einen lokalen Ordner außerhalb des Repositorys,
  z. B. `C:\MatchballTennis-Build\`, und werden nie eingecheckt.
- Das **Update-Repository** für Freunde (19.4.2) ist ein **zweites, eigenes** Repository nur mit fertigen
  Spieldateien – das private Entwicklungs-Repository bleibt privat.

**Arbeiten mit Git:**

- Nach jedem abgeschlossenen Arbeitsschritt ein **Commit** mit deutscher, verständlicher Nachricht
  (z. B. „Ballphysik: Magnus-Kraft und Drall-Abnahme umgesetzt“), am Ende jeder Session **Push**.
- `STAND.md` wird bei jedem Push mit aktualisiert.
- **Wechsel zwischen PCs:** vor dem Wechsel alles pushen, auf dem anderen PC zuerst `git pull`, dann
  `Tools\build.ps1` (erzeugt die lokalen Ordner neu). Nie gleichzeitig auf zwei PCs am selben Stand arbeiten.
- Größere Umbauten in einem eigenen Branch, nach dem Test in `main` zusammenführen.
- **Hinweis:** Cloud ↔ lokal siehe 0.0a. Läuft eine Sitzung nur in der Cloud, kann sie weder Unreal noch Blender
  starten – ungetestetes in `STAND.md` vermerken.

### 0.1 Bevor du anfängst

1. Prüfe, was installiert ist, und schreibe das Ergebnis in `STAND.md`:
   - Unreal Engine **5.8** (Pfad, z. B. `C:\Program Files\Epic Games\UE_5.8`) – 5.8 ist die letzte große UE5-Version.
   - Blender **5.2 LTS** (Pfad zu `blender.exe`).
   - Visual Studio 2022 mit „Spieleentwicklung mit C++“ bzw. die von UE 5.8 geforderte MSVC-Version.
   - Git (mit Git LFS für Binärdateien) und die GitHub-Kommandozeile `gh` (angemeldet).
   - Python ist in Blender und im Unreal Editor eingebaut – kein separates Python nötig.
2. **Es gibt kein Altprojekt als Vorlage.** Alle Zahlenwerte (Physik, Schlagwerte, Timing-Fenster, KI, Spielerwerte,
   Turnierkalender, Punkte, Preisgelder, Level-Kurve) werden **neu hergeleitet**: aus den Vorgaben in diesem Dokument,
   aus realen Tennisdaten (Recherche) und durch Ausprobieren im Spiel. Siehe 24.3. Jede Herleitung kommt in
   `Dokumente/Werte.md`; die Werte selbst in `Data/` (nicht im Code).
3. Frage nach, wenn etwas aus Abschnitt 23 (Offene Entscheidungen) die aktuelle Aufgabe betrifft.

### 0.2 Wie du arbeitest

- **Alles muss reproduzierbar aus Skripten entstehen.** Du kannst den Unreal Editor und Blender nicht mit der Maus
  bedienen und in der Cloud gar nicht starten (0.0a). Deshalb gilt:
  - Blender-Assets entstehen über Python-Skripte, die headless laufen:
    `blender.exe -b -P blender/scripts/<skript>.py -- <argumente>`
  - Der Import nach Unreal, das Anlegen von Materialien, Material-Instanzen, Animations-Assets und das Aufbauen
    der Level läuft über Editor-Python-Skripte:
    `UnrealEditor-Cmd.exe MatchballTennis.uproject -run=pythonscript -script="Tools/Python/<skript>.py"`
  - Optional: Das **Unreal MCP Server Plugin** (experimentell, seit UE 5.8) kann dir direkten Zugriff auf den
    laufenden Editor geben. Nutze es, wenn es stabil läuft – die Skripte bleiben trotzdem die Quelle der Wahrheit.
- **C++ zuerst.** Spiellogik, Physik, Zählweise, KI, Karriere, Online und UI-Logik in C++. Blueprints nur dort,
  wo Unreal sie zwingend braucht (z. B. Animation Blueprints), und dann so dünn wie möglich, Logik in C++-Basisklassen.
- **Keine handgebauten .uasset-Dateien.** Alles, was in `Content/` liegt, wird von einem Skript erzeugt und kann
  jederzeit neu erzeugt werden. Die Quelldateien (`.blend`, `.py`, Texturen, Audio) liegen in `SourceAssets/` bzw. `blender/`.
- **Datengetrieben.** Spieler, Turniere, Anlagen, Outfits, Schlagwerte und KI-Profile in Data Tables
  (aus CSV/JSON in `Data/` importiert) – nicht hart im Code.
- **Kleine, testbare Schritte.** Nach jedem Meilenstein-Schritt: kompilieren, Automation-Tests laufen lassen,
  `STAND.md` aktualisieren (was fertig ist, was offen ist, bekannte Fehler).
- **Tests.** Physik, Zählweise, Linienrichter-Statistik, Challenge-Regeln, Ranglisten-Berechnung und
  Online-Determinismus bekommen Unit-Tests (Unreal Automation Framework, `Source/MatchballTennisTests/`).
- **Lizenzen mitschreiben.** Jede Datei aus fremder Quelle wird sofort in `CREDITS.md` eingetragen
  (Quelle, Urheber, Lizenz, was geändert wurde).
- **Downloads:** Wenn eine Quelle ein Konto braucht (z. B. Mixamo, Epic Developer Portal, ElevenLabs), sag mir
  genau, was ich herunterladen bzw. anlegen soll und wohin die Datei kommt.

### 0.3 Was nicht passieren darf

- Keine Marken, Logos oder echten Spielernamen in Grafiken (Ausnahme siehe 22.3).
- Keine Assets mit unklarer Lizenz.
- **Keine fremden Assets ohne meine Zustimmung.** Alles wird neu und selbst erstellt; einzige Ausnahme ist das
  Schlägermodell aus `Vorgaben/Schlaeger/`.
- **Keine Regeländerungen** gegenüber diesem Dokument ohne Rückfrage – außer bei den mit **[NEU]** markierten Punkten.
- Die alten Fassungen nicht suchen, nicht öffnen, nicht nachbauen.

---

## 1. Vision und Grundsätze

Matchball Tennis ist ein realistisches 3D-Tennisspiel für Windows mit Fernsehkamera-Optik. Es soll sich anfühlen
wie eine Tennisübertragung, in der man selbst spielt: echte Bewegungen, echte Regeln, echte Plätze mit Abnutzung,
ein Schiedsrichter mit Stimme, Publikum, Hawk-Eye, eine lange Karriere von ITF-Turnieren bis zu den Finals.

Grundsätze:

1. **Timing statt Knopfdrücken.** Jeder Schlag ist eine Timing-Entscheidung (Taste halten, im grünen Bereich loslassen).
2. **Ehrliche Physik.** Der Ball fliegt nach echter Physik (Schwerkraft, Luftwiderstand, Magnus-Effekt, Absprung
   je nach Belag). Animationen passen sich dem Ball an – nie umgekehrt.
3. **Fernsehgefühl.** Kamera, Grafikeinblendungen, Ansagen und Abläufe zwischen den Punkten wie bei einer Übertragung.
4. **Alles selbst gemacht.** Figuren, Kleidung, Anlagen, Ball, Texturen, Animationen und Klänge werden neu erstellt –
   **einzige übernommene Datei ist das eigene Schlägermodell**. Fremde Assets nur als Ausweichlösung nach Rückfrage.
5. **Komplett mit Controller bedienbar.**
6. **Läuft auch auf schwächerer Hardware** (Onboard-Grafik) mit reduzierten Einstellungen flüssig.

---

## 2. Technik: Unreal Engine 5

### 2.1 Version und Plattform

- **Unreal Engine 5.8**, C++-Projekt `MatchballTennis`.
- Zielplattform: **Windows 11, 64 Bit**. Eingabe: Tastatur und Xbox-/PlayStation-Controller.
- Ausgabe: eine gepackte `Matchball Tennis.exe` (Shipping-Build), startet im randlosen Vollbild.

### 2.2 Verwendete Engine-Systeme und Plugins

| Bereich | System | Zweck |
|---|---|---|
| Eingabe | **Enhanced Input** | Tastatur + Controller, Halten/Loslassen für Timing, Kontexte für Menü/Match/Challenge |
| UI | **CommonUI** + UMG (in C++ aufgebaut) | Controller-Navigation, Fokus, Bildschirmtastatur, Eingabesymbole je nach Gerät |
| Animation | **Animation Blueprint** (dünn) + C++ `UAnimInstance` | Zustände, Mischung, Layer |
| Laufen | **Motion Matching** (Pose Search) | natürliche Laufbewegung, Seitschritte, Rückwärtslaufen, Abbremsen |
| Schläge | **Anim Montages** + **Motion Warping** | Schlagbewegung so verbiegen, dass der Schläger den Ball am berechneten Punkt trifft |
| Feinanpassung | **Control Rig** / Full-Body-IK | Hand am Griff, linke Hand am Schläger, Füße auf dem Boden, Blick zum Ball |
| Retargeting | **IK Retargeter** | fremde Bewegungsdaten auf das eigene Skelett übertragen |
| Kleidung | **Chaos Cloth** (bzw. Cloth Asset / Dataflow) | Stoff schwingt nach (Rock-Volants, Shirtsaum, Shorts) |
| Effekte | **Niagara** | Sandstaub beim Rutschen, Ballabdrücke, Kreide-/Staubwolke an der Linie, Hawk-Eye-Spur |
| Ton | **MetaSounds** + Audio Mixer | Schläge, Schritte je Belag, Publikum, Hall je Anlage |
| Licht | **Lumen** / **Lumen Lite** / gebackenes Licht | je nach Grafikstufe (siehe 18) |
| Materialien | **Substrate** | Haut, Stoff, Belag, Netz |
| Online | **Online Subsystem EOS** (Epic Online Services) | Lobby mit Code, P2P-Verbindung mit Relay |
| Daten | **Data Tables** (aus CSV/JSON) | Spieler, Turniere, Anlagen, Outfits |
| Speichern | `USaveGame` | Optionen, Karriere, Statistiken |

**Nicht** verwenden: MetaHuman (Figuren sollen aus Blender kommen), Chaos-Physik für den Ball (eigene,
deterministische Simulation – siehe 6).

### 2.3 Code-Architektur (C++-Module)

```
Source/
  MatchballTennis/           Hauptmodul
    Core/                    GameInstance, Einstellungen, Speichern, Versions-Build
    Match/                   Match-Ablauf (Zustandsmaschine), Zählweise, Seitenwechsel
    Physics/                 Ballsimulation, Vorhersage, Schlagberechnung, Kontaktfläche
    Player/                  Spieler-Pawn, Bewegung, Schlagauswahl, Timing-Meter
    Animation/               AnimInstance, Montage-Steuerung, Warping, IK
    AI/                      KI-Gegner (Positionierung, Schlagwahl, Fehlerquote, Challenges)
    Officials/               Linienrichter, Schiedsrichter, Hawk-Eye, Ansagen
    Camera/                  TV-Kamera, Alternativkameras, Videobeweis-Kamera
    Venue/                   Anlagen, Belag-Parameter, Abnutzung, Publikum, Anzeigetafeln
    Clothing/                Outfits, Drucke, Schnitte
    Career/                  Karriere, Kalender, Rangliste, Auslosung, Simulation
    Online/                  Lobby, Verbindung, Synchronisierung
    UI/                      Menüs, HUD, Bildschirmtastatur
    Audio/                   Klänge, Schiedsrichter-Stimme
  MatchballTennisTests/      Automation-Tests
```

**Match-Zustandsmaschine** (in `Match/`): `Vorbereitung → Aufschlagvorbereitung (Balltippen) → Aufschlag →
Ballwechsel → Punktende → (Challenge-Fenster → Videobeweis) → Ansage → Zwischen den Punkten → …`
plus `Seitenwechsel`, `Satzpause`, `Matchende`. Jeder Zustand ist eine eigene Klasse/Funktion mit klaren Ein- und Ausgängen,
damit Online-Abgleich und Wiederholungen einfach sind.

### 2.4 Bauen

- `Tools/build.ps1` – kompiliert Editor und Spiel (UnrealBuildTool).
- `Tools/package.ps1` – Shipping-Build über `RunUAT.bat BuildCookRun`, Ausgabe in den lokalen Ordner `C:\MatchballTennis-Build\` (außerhalb des Repositorys, siehe 0.0),
  danach Prüfsumme berechnen (siehe 15.4).
- `Tools/assets.ps1` – baut alle Blender-Assets neu und importiert sie (siehe 3).
- `Tools/release.ps1` – veröffentlicht ein Update (siehe 19.4.6), aufgerufen über `Update veröffentlichen.bat`.
- `Updater/build.ps1` – baut `Matchball Tennis Updater.exe` (siehe 19.4.5).

---

## 3. Asset-Pipeline mit Blender

### 3.1 Grundregel

**Alles wird neu und selbst in Blender gemacht – und zwar per Python-Skript**, damit jedes Asset neu
erzeugt, angepasst und variiert werden kann. **Einzige Ausnahme: das eigene Schlägermodell** (3.4).

Fremde Dateien (Basis-Meshes, Fotoscans, Bewegungsaufnahmen, Klänge) sind nur eine **Ausweichlösung**: wenn etwas
trotz mehrerer Versuche nicht in brauchbarer Qualität gelingt, mir zeigen, woran es scheitert, eine freie Quelle
vorschlagen und **erst nach meiner Zustimmung** verwenden.

**Qualitätsschleife für jedes Asset:** Das Blender-Skript rendert Vorschaubilder (headless mit Eevee: vorne, seitlich,
Dreiviertel, Nahaufnahme) als PNG nach `SourceAssets/_vorschau/`. Claude Code schaut sie sich selbst an, bessert nach
und zeigt sie mir zur Abnahme, bevor das Asset als fertig gilt.

### 3.2 Ablauf

```
blender/scripts/*.py   ──(blender -b -P)──►  SourceAssets/**/*.blend + *.fbx
                                                     │
Tools/Python/import_*.py ──(UnrealEditor-Cmd)──►  Content/**  (Meshes, Skelette, Materialien, Anims)
                                                     │
Tools/Python/build_*.py  ──(UnrealEditor-Cmd)──►  Content/Maps/**  (Level werden aus Daten aufgebaut)
```

- Maßstab: Blender in **Metern**, Export als FBX mit Skalierung so, dass 1 m = 100 Unreal-Einheiten (cm).
- Achsen: Vorwärts −Y bzw. nach Unreal-Konvention; das Exportskript legt das einheitlich fest.
- Namenskonvention: `SK_` Skeletal Mesh, `SKEL_` Skelett, `SM_` Static Mesh, `M_` Material, `MI_` Material-Instanz,
  `T_` Textur, `A_` Animation, `AM_` Montage, `ABP_` Animation Blueprint, `NS_` Niagara, `MS_` MetaSound.
- Jedes Blender-Skript schreibt neben das Asset eine kleine `*.meta.json` (Quelle, Lizenz, Datum, Parameter).

### 3.3 Spielerfiguren

**Ziel:** realistische, sportliche Figuren mit eigenem Look, die zum Tennis passen.

- **Grundkörper – komplett selbst gebaut:** ein eigener **Figuren-Generator** in Blender-Python
  (`blender/scripts/figur.py`) baut den Körper von Grund auf:
  - Basis-Mesh mit sauberer, animationsgerechter Topologie (Vierecke, Kantenschleifen an Gelenken, um Augen und Mund,
    genug Auflösung an Händen und Schultern),
  - Formen über eigene Shape Keys: Geschlecht, Größe, Muskeln, Körperfett, Proportionen, Gesichtsformen,
  - Detail über skriptgesteuertes Sculpting bzw. Displacement (Muskeln, Sehnen, Knöchel, Schlüsselbein),
  - anatomisch stimmige Proportionen nach Anatomie-Maßen **[PRÜFEN]** (z. B. Kopfhöhen-Kanon, Gelenkpositionen).
  Kein MakeHuman/MPFB und keine fremden Basis-Meshes. Damit ist jede Figur eigenständig und reproduzierbar.
- **Reihenfolge:** zuerst Körper in grober Form + Skelett + Testanimation (damit die Topologie sich beim Bewegen
  bewährt), dann Details, dann Gesicht und Hände – die beiden schwierigsten Teile früh zur Abnahme zeigen.
- **Ausweichlösung (nur nach Rückfrage):** Gelingen realistische Körper mit dem eigenen Generator nicht, als
  Ausgangs-Mesh MPFB (CC0) verwenden und darauf aufbauen.
- **Skelett:** ein gemeinsames Skelett für alle Figuren, **benannt und aufgebaut wie das Standard-Skelett von
  Unreal** (UE5-Mannequin-Knochennamen), damit Retargeting von Epic-Beispielanimationen und fremden Bewegungsdaten
  ohne Umwege klappt. Zusätzliche Knochen: `racket_r`/`racket_l` (Schlägeraufnahme), Schlägergriff-Hilfsknochen,
  `ball_hand` (Ball beim Aufschlag), Kleidungsknochen für Rock/Saum (falls Cloth nicht reicht).
- **Gewichtung:** automatische Gewichte aus Blender („Mit automatischen Gewichten“) als Basis, per Skript an Schultern, Hüfte und Handgelenk
  nachgeschärft (Tennis hat extreme Schulter- und Hüftrotationen – dort darf nichts einknicken).
- **Gesicht:** einfache Gesichtsausdrücke als Shape Keys/Morph Targets: neutral, Anstrengung (beim Schlag),
  Freude, Ärger, Enttäuschung, Konzentration, Ausatmen. Augen schauen zum Ball (Look-At im Control Rig).
- **Haare:** Haarkarten (Hair Cards) in Blender modelliert, mehrere Frisuren (kurz, mittel, Pferdeschwanz, Zopf,
  Dutt, Locken, Glatze), mit Stirnband/Kappe/Visor als Varianten. Pferdeschwanz/Zopf schwingt nach (Physik-Knochen).
- **Haut:** eigenes Hautmaterial (Substrate, Subsurface), Hauttöne als Parameter, Detailkarten (Poren, Adern an
  Unterarmen) prozedural in Blender gebacken.
- **Vielfalt:** mindestens **6 Grundkörper** (3 Männer, 3 Frauen, unterschiedliche Statur), daraus per
  Parametern alle Kader-Spieler und die Karriere-Figur. **[NEU]** Für die Karriere-Erstellung werden einige
  Körper- und Gesichts-Shape-Keys als Regler exportiert (Größe über Knochenskalierung, Statur, Gesichtsform, Hautton,
  Frisur, Haarfarbe).
- **LODs:** 4 Stufen (LOD0 nah ~60–80k Dreiecke inkl. Kleidung, LOD3 für Publikum-Entfernung) – per Skript mit
  Decimate erzeugt.
- **Schuhe:** ein Paar weiße Tennisschuhe mit Söckchen, in Blender selbst modelliert (Sohle mit Profil, je Belag
  eigene Sohlenkarte: Fischgrät für Sand, Noppen für Rasen), für alle Figuren.

### 3.4 Schläger und Ball

- **Schläger – das einzige übernommene Asset:** mein eigenes Schlägermodell aus `Vorgaben/Schlaeger/`
  (Originaldatei in voller Qualität) übernehmen, in Blender aufbereiten (Maßstab,
  Ursprung am Griffende, Achse entlang des Schlägers), Saitenbett als Alpha-Material, LODs. Das Schlägerlogo des
  eigenen Modells bleibt (private Nutzung, siehe 22.3).
- **Ball – neu erstellt:** der Ball wird neu in Blender modelliert
  (exakte Maße, siehe 24.2, mit Naht als Geometrie bzw. Normal-Map).
- **Schläger-Varianten [NEU]:** 6–8 Lackierungen als Material-Instanzen (Farbe, Akzent, Saitenfarbe), auswählbar
  in Schnellem Match und Karriere.
- **Ball:** Filz-Material mit Naht, leichter Fussel-Rand (Shell-Fur oder Opazitäts-Rand), Verformung beim Aufprall
  (Shape Key „gestaucht“ für Schläger- und Bodenkontakt, 1–2 Frames).
- **Ball-Abnutzung [NEU]:** neue Bälle nach 7/9 Spielen (wie auf der Tour) – frische Bälle sind leuchtender,
  alte matter und etwas schmutzig (auf Sand rötlich). Physik bleibt gleich.

### 3.5 Anlagen-Bausteine

In Blender als **modulares Baukastensystem** modelliert, damit aus wenigen Teilen viele Anlagen entstehen:

- Platz (Belag-Fläche mit exakten Linien als Maske, siehe 24), Netz (Pfosten, Netzband, Mittelband, Netzgeflecht
  als Alpha-Material, leicht durchhängend), Einzelstützen.
- Schiedsrichterstuhl, Linienrichterstühle, Spielerbänke mit Schirmen, Handtuchboxen **[NEU]**, Ballkinder-Positionen.
- Banden (mit Werbeflächen für eigene, erfundene Marken), Tribünensegmente (gerade/Ecke, mit Schalensitzen bzw.
  Steinstufen bzw. Holzbänken), Treppen, Logen, Kamerapodeste, Dachring, Flutlichtmasten, Videowände,
  klassische Anzeigetafel, Efeu-Wand, Zaun mit Windschutz, Clubhaus, Bäume und Hecken (selbst in Blender, z. B. per Geometry
  Nodes), Fahnen.
- **Hallen-Bausteine [NEU]:** Hallendach mit Stahlträgern, Hallenwände, Hallenbeleuchtung (Lichtbänder,
  Strahler), verdunkelte Tribünen mit Spot auf den Platz (Finals-Stil).
- Eigene Werbe- und Markenlogos (erfundene Sponsoren) als Texturen – selbst in Blender/als SVG erstellt.

### 3.6 Texturen

- **Selbst gemacht:** prozedurale Materialien in Blender (Rauschen, Muster, Abnutzung) und auf Texturen gebacken
  (Linienabnutzung, Rutschspuren, Gummistreifen, Fußwege auf Rasen).
- **Auch die Feinstruktur wird selbst erzeugt** (statt Fotoscans): Sandkörner und
  Abziehspuren, Grashalme (Geometry Nodes → auf Textur gebacken), körnige Acryl-Beschichtung, Stoffgewebe von
  Polo/T-Shirt/Shorts, Filz des Balls, Haut-Poren. Jeweils als Farb-, Normal- und Rauheitskarte in 2K/4K gebacken.
- **Ausweichlösung (nur nach Rückfrage):** CC0-Fotoscans von ambientCG oder Poly Haven, falls eine Oberfläche
  prozedural nicht überzeugend wird.
- **Laufzeit-Abnutzung:** Fußabdrücke, Rutschspuren und Ballabdrücke werden während des Matches in eine
  **Render Target**-Maske gemalt (Decals bzw. Runtime Virtual Texture) und vom Belag-Material ausgewertet.

### 3.7 Publikum

- 8–12 einfache Zuschauerfiguren (aus den eigenen Grundkörpern, stark reduziert), mit Kleidungsfarben als Parameter
  (Sommer, Herbst, Halle; Hüte, Sonnenbrillen, Fächer).
- Bewegungen als **Vertex-Animation-Texturen** oder geteilte Animationen (sitzen, schauen links/rechts dem Ball
  nach, klatschen, aufspringen, jubeln, „Ohhh“) – in Blender gebacken, damit tausende Zuschauer möglich sind.
- Instanziert (Instanced Static Meshes bzw. Animation Sharing), Dichte je Grafikstufe und je Turnier-Runde
  (Qualifikation fast leer, Finale voll).

---

## 4. Animationen

### 4.1 Grundsatz: alle Bewegungen selbst animieren

Alle Animationen werden **neu und selbst erstellt** – auch die Schläge. Fremde Bewegungsdaten (Tennis-MoCap,
Rocketbox, Bandai Namco, CMU …) werden nicht verwendet, außer als freigegebene Ausweichlösung (4.1a).

**Werkzeuge:**

- **Keyframe-Animation per Skript:** Posen als Daten (`blender/anims/*.json`: Gelenkwinkel je Schlüsselbild und
  Zeitpunkt), ein Blender-Skript baut daraus die Aktion mit sauberer Interpolation, Überschwingen, Nachziehen und
  Gewichtsverlagerung. Alternativ bzw. zum Feinschliff Control Rig in Unreal.
- **Prozedurale Bewegung** wo sinnvoll: Gang- und Laufzyklen aus Parametern (Schrittlänge, Tempo, Hüfthöhe,
  Armschwung, Oberkörperneigung), Seitschritte, Atmen, Blick zum Ball, Split-Step – teils zur Laufzeit per Control Rig.
- **Physik/IK:** Nachschwingen von Haaren und Kleidung, Fußplatzierung, Anpassung des Ausfallschritts.

**Schläge biomechanisch richtig:** Jeder Schlag folgt der **kinetischen Kette** (Beine → Hüfte → Rumpf → Schulter →
Ellbogen → Handgelenk, jeweils zeitversetzt), der Schlägerkopf erreicht am Treffpunkt die höchste Geschwindigkeit.
Phasen: Bereitschaft → Ausholen (Schulterdrehung) → Beschleunigung → Treffpunkt → Durchschwung → Rückkehr in die
Bereitschaft. Gelenkwinkel und Zeiten an Fachwissen zur Tennis-Biomechanik ausrichten **[PRÜFEN]**.

**Variation:** je Schlag 2–3 Varianten über Parameter (Ausholweite, Tempo, Körperhaltung, offener/neutraler Stand),
dazu Spielerprofile (z. B. kurzes oder weites Ausholen), damit nicht alle gleich aussehen.

**Qualitätsschleife:** Jede Animation wird aus Blender als Bildfolge/kurzes Video gerendert (Ansicht seitlich und von
hinten), von Claude Code geprüft und mir zur Abnahme gezeigt. Am schwierigsten und deshalb früh zu zeigen:
Aufschlag, Vorhand, beidhändige Rückhand.

**Zu erstellende Animationen (Übersicht):**

- **Laufen:** Stand, Bereitschaft, Split-Step, Seitschritte links/rechts, Kreuzschritt, Sprint, Abbremsen,
  Rückwärtslaufen, Drehen, Gehen.
- **Grundschläge:** Vorhand, Rückhand einhändig und beidhändig – jeweils flach, Topspin, Lob; Slice und Stopp
  einhändig – jeweils Treffhöhe tief/normal/hoch.
- **Aufschlag:** Balltippen (1–4 Mal), Ballwurf, flach/Kick/Slice; Return-Stellung.
- **Netz:** Volleys Vorhand/Rückhand tief und hoch, Halbvolley, Schmetterball aus dem Stand, mit Sprung und nach dem
  Aufsprung.
- **Rutschen und Ausfallschritt:** Sand lang, Rasen mittel, Hartplatz kurz.
- **Zwischen den Punkten:** Gehen nach Stimmung (stolz, fröhlich, traurig, wütend, müde, unsicher) und alle Gesten
  aus Abschnitt 11, Challenge-Geste, linke Hand am Griff/Schlägerhals, Schläger in die andere Hand nehmen,
  Handtuch nehmen **[NEU]**, Saiten richten **[NEU]**, Ball mit dem Schläger aufheben **[NEU]**, Sitzen und Trinken
  beim Seitenwechsel **[NEU]**.
- **Matchende:** Jubel, Winken (ein-/beidhändig), Verbeugen, Handschlag am Netz **[NEU]**.
- **Personen auf der Anlage:** Schiedsrichter, Linienrichter, Ballkinder **[NEU]**, Publikum (sitzen, Kopf folgt dem
  Ball, klatschen, aufspringen, jubeln).

### 4.1a Ausweichlösung: fremde Bewegungsdaten (nur nach Rückfrage)

Nur wenn eine Bewegung trotz mehrerer Versuche nicht in brauchbarer Qualität gelingt, mir die Vorschau zeigen und
eine dieser freien Quellen vorschlagen. **Verwendet wird sie erst nach meiner Zustimmung** (auf das eigene Skelett
übertragen per IK Retargeter):

| Quelle | Inhalt | Lizenz | Hinweis |
|---|---|---|---|
| Tennis-MoCap (Pulgarin-Giraldo et al. 2017, github.com/jdpulgarin/Tennis-MoCap) | Vorhand, ein-/beidhändige Rückhand, Aufschlag mit Grundstellung, Schmetterball, Volleys, Return-/Wartestellung – 16 Spielerinnen und Spieler | CC BY-SA 3.0 | Nennung + gleiche Lizenz bei Weitergabe |
| Microsoft Rocketbox Animation Library | Stehen, Gehen, müdes Gehen, Laufen, Jubeln, Ärger, Gesten (Durchatmen, Achselzucken, Kopfschütteln, Nicken, Hocke, Staub abklopfen, mit dem Schiedsrichter diskutieren, Winken) | MIT | – |
| Bandai Namco Research Motiondataset 1 | Gangarten nach Stimmung (stolz, fröhlich, traurig, wütend, müde, unsicher), Verbeugen, beidhändiges Winken | CC BY-NC 4.0 | nur nicht-kommerziell |
| CMU Graphics Lab Motion Capture Database | Seitschritte im Athletenstand, Rückwärtsgehen, Ausfallschritt | frei nutzbar | – |
| Epic **Game Animation Sample** (Unreal) **[NEU]** | hochwertige Lauf-, Start-, Stopp- und Drehbewegungen für Motion Matching | Unreal-Lizenz (nur in Unreal-Projekten) | nur falls die eigenen Laufbewegungen für Motion Matching nicht reichen |
| Mixamo (Adobe) **[NEU, optional]** | Ergänzungen, falls etwas fehlt (z. B. Jubel-Varianten) | frei nutzbar mit Adobe-Konto | muss ich selbst herunterladen |

Wenn etwas weder selbst gelingt noch eine Ausweichlösung freigegeben ist: melden, nicht zusammenbasteln.

### 4.2 Bewegungsdaten aufbereiten

- Nur bei freigegebener Ausweichlösung: fremde BVH/FBX-Daten in `SourceAssets/Animations/<Quelle>/` ablegen, ein Blender-Skript bereinigt sie
  (Maßstab, Wurzelbewegung, Fußrutschen, Schnitt auf die relevanten Frames) und exportiert sie.
- Retargeting in Unreal per Python (IK Retargeter-Asset je Quelle).
- Jeder Schlag bekommt **Anim Notifies**: `Ausholen-Start`, `Treffpunkt` (exakter Kontaktframe), `Durchschwung-Ende`,
  `Rückkehr-möglich`. Der Treffpunkt-Frame ist die Grundlage für Timing und Warping.
- Für jeden Schlagtyp **mehrere Varianten** (mindestens 3, über Parameter erzeugt – siehe 4.1),
  die nach Spielerprofil verteilt werden, damit nicht alle gleich aussehen.

### 4.3 Bewegungssystem im Spiel

- **Laufen:** Motion Matching über eine Datenbank aus Seitschritten (Athletenstand), Vorwärts-Sprint, Rückwärtslaufen,
  Ausfallschritt, Abbremsen, Drehen, Split-Step. Die Laufrichtung relativ zum Netz bestimmt die Auswahl (Tennis-Spieler
  laufen fast immer seitlich mit Blick zum Netz).
- **Schläge:** Montage wird so gestartet, dass der Treffpunkt-Frame mit dem Moment zusammenfällt, in dem der Ball den
  berechneten Treffpunkt erreicht (Abspielgeschwindigkeit ±20 % anpassen). **Motion Warping** verschiebt/dreht die
  Figur in den letzten Frames so, dass der Schlägerkopf den Ball wirklich trifft. Treffhöhe wird über Varianten (tief,
  normal, hoch) plus IK abgedeckt.
- **Oberkörper/Unterkörper getrennt:** Unterkörper läuft weiter (Motion Matching), Oberkörper spielt den Schlag
  (Layered Blend per Bone), bei Volleys und Rutschen Ganzkörper-Montage.
- **IK:** Füße auf dem Boden (auch beim Rutschen), linke Hand am Schläger bei beidhändiger Rückhand, Kopf/Augen
  verfolgen den Ball.
- **Rechtshänder/Linkshänder [NEU]:** Spiegelung der Animationen (Mirror Data Table), Schlaghand als Spielerwert.

---

## 5. Spielmechanik und Regeln

### 5.1 Steuerung Tastatur

| Taste | Aktion |
|---|---|
| W A S D / Pfeile | Laufen · beim Schlag: Ziel (vor = lang, zurück = kurz, links/rechts = Richtung) |
| J | Flach |
| K | Topspin |
| L | Slice |
| I | Lob |
| U | Stopp |
| Shift / Q halten | Power-Schlag |
| Leertaste / E halten | Präzisions-Schlag |
| H (nach dem Punkt) | Challenge bzw. Videobeweis |
| C | Kamera wechseln |
| M | Ton an/aus |
| Esc / P | Pause |
| F11 | Vollbild / Fenster |

### 5.2 Steuerung Controller

| Taste (Xbox / PlayStation) | Aktion |
|---|---|
| Linker Stick / Steuerkreuz | Laufen · Ziel |
| A / ✕ | Flach |
| B / ○ | Topspin |
| X / □ | Slice |
| Y / △ | Lob · nach dem Punkt: Challenge |
| LB / RB | Stopp |
| RT halten | Power |
| LT halten | Präzision |
| Rechter Stick klicken **[NEU]** | Kamera wechseln |
| Start / Options | Pause |

Menüs: Steuerkreuz/Stick bewegen, A auswählen, B zurück, LB/RB Werte ändern, Namen über Bildschirmtastatur.
**[NEU]** Vibration beim Treffen (stärker bei Power, kurz beim Netzband), abschaltbar.

### 5.3 Timing

- Schlagtaste **früh drücken und halten**, im **grünen Bereich** des Timing-Meters **loslassen**.
- Das Meter zeigt den Weg bis zum idealen Treffpunkt; grüner Bereich = perfekt, gelb = leicht früh/spät,
  rot = deutlich daneben.
- **Timing-Fehler:** Der Ball weicht seitlich vom Ziel ab – **zufällig nach links oder rechts**, damit beide Seiten
  gleich riskant sind. Größe der Abweichung wächst mit dem Timing-Fehler; zusätzlich leichte Längenstreuung.
- **Power** (Shift/Q bzw. RT): mehr Tempo, kleineres grünes Fenster, größere Streuung.
- **Präzision** (Leertaste/E bzw. LT): weniger Tempo, größeres Fenster, kleinere Streuung, Ziel näher an die Linien möglich.
- Das Timing-Fenster lässt sich für das ganze Match einstellen (Schnelles Match, Online: gilt für beide).
- Spielerwerte (Vorhand, Rückhand, Volley …) verändern Fenstergröße und Streuung.

### 5.4 Schlagarten

| Schlag | Flugbahn | Absprung | Besonderheit |
|---|---|---|---|
| Flach | gerade, schnell | flach, schnell | wenig Fehlertoleranz über dem Netz |
| Topspin | hoher Bogen, taucht ab | hoch, nach vorne beschleunigend | sicherer, auf Sand stärker |
| Slice | flach, schwebend | flach, rutscht weg, bleibt tief | einhändig für alle Spieler |
| Lob | sehr hoch | – | defensiv (Unterschnitt) oder offensiv (Topspin, mit Power) |
| Stopp | kurz übers Netz | kaum Vorwärtsdrang, Rückwärtsdrall | einhändig für alle Spieler |

- **Zielen:** Richtung (links/rechts) und Länge (vor = lang, zurück = kurz) beim Loslassen.
- **Rückhand:** die meisten Spieler beidhändig; **Slice und Stopp spielen alle einhändig**;
  **Holloway** und **Moreau** spielen die einhändige Rückhand.

### 5.5 Aufschlag

- Vor dem Aufschlag tippt der Spieler den Ball wie auf der Tour auf – **je Spieler 1–4 Mal** (Spielerwert).
- Die Schlagtaste darf schon währenddessen gedrückt werden – nach dem Fangen folgen Ausholen und Ballwurf.
- Aufschlagarten über die Schlagtasten: flach (schnell), Kick/Topspin, Slice; Richtung mit dem Stick
  (außen / Körper / Mitte, „T“).
- Erster Aufschlag riskanter und schneller, zweiter sicherer. Doppelfehler möglich.
- **Let** (Netzberührung, Ball im Feld): wiederholen, Ansage „Let“.
- **Fußfehler [NEU, optional]:** sehr selten, nur wenn der Spieler beim Aufschlag zu früh über die Linie läuft
  (KI fast nie). Standardmäßig aus.
- **Aufschlag-Zeitlimit [NEU, optional]:** 25-Sekunden-Uhr wie auf der Tour, nur in Karriere ab Tour-Ebene,
  abschaltbar.
- Tempo-Anzeige (km/h) nach jedem Aufschlag als TV-Einblendung **[NEU]**.

### 5.6 Volley

- Mit W ans Netz laufen – dort wird der Ball mit denselben Tasten **direkt aus der Luft** genommen.
- Am Netz reicht der Arm weiter (**Ausfallschritt**), und das Timing-Fenster ist bei Volleys **größer**.
- Halbvolley **[NEU]** wenn der Ball direkt vor den Füßen aufspringt: eigene Animation, kleines Fenster.

### 5.7 Schmetterball

- Hohe Bälle am Netz und Lobs werden **über dem Kopf geschmettert** – auch aus dem Mittelfeld, bei sehr hohen
  Bällen **mit Sprung**.
- Sehr hoch abspringende Bälle auch **nach dem Aufsprung**.
- Der Smash geht **immer flach und mit voller Wucht**.
- Rückwärtslaufen unter dem Lob (Seitschritte rückwärts, Hand zeigt zum Ball **[NEU]**).

### 5.8 Laufen und Rutschen

- Grundstellung, Split-Step beim gegnerischen Treffpunkt, Seitschritte im Athletenstand, Sprint bei weiten Wegen.
- **Rutschen nur bei Bällen, die eigentlich unerreichbar aussehen** (weiter Weg in knapper Zeit) und die man dann
  doch noch erwischt: Der Spieler rutscht im tiefen Ausfallschritt in den Ball, Hüfte in Rutschrichtung –
  - **Sand:** weit, mit Staub, Knirschen und Rutschspur,
  - **Rasen:** mittel,
  - **Hartplatz:** kurz, mit Schuhquietschen.
- Schritte sind je nach Belag zu hören; auf Sand und Rasen bleiben **Fußabdrücke** zurück.
- **Ausdauer [NEU]:** sinkt in langen Ballwechseln und Matches, erholt sich zwischen den Punkten und beim
  Seitenwechsel; niedrige Ausdauer = etwas langsamer und kleineres Timing-Fenster. Anzeige dezent im HUD.

### 5.9 Kamera

- Standard: **Fernsehkamera** hinter dem eigenen Spieler, leicht erhöht, folgt seitlich.
- Läuft man weit hinter die Grundlinie, fährt die Kamera **in gleicher Höhe mit zurück**; Bande, Tribüne und Publikum
  hinter dem Spieler werden dann **ausgeblendet** (Dither-Fade), damit die Sicht frei bleibt.
- C wechselt zwischen: TV-Kamera (Standard), hohe TV-Kamera, Spielerkamera (näher, tiefer), Seitenkamera **[NEU]**.
- Zwischen den Punkten Regie-Schnitte **[NEU]**: Nahaufnahme des Punktgewinners, Publikum, Spielerbox, Zeitlupe
  von besonderen Punkten (Ass, Winner nach langem Ballwechsel, Rutsch-Rettung) – mit Taste überspringbar.
- Online sieht **jeder sich selbst vorne**.

### 5.10 Ballmarken

- Aus-Markierung und Ballabdrücke auf Sand bleiben **nur bis zum nächsten Punkt** liegen.
- Auf Sand zeigt der Schiedsrichter bei strittigen Bällen **[NEU, optional]** auf die Marke (wie auf Sand üblich,
  statt Hawk-Eye – siehe 7.4).

---

## 6. Ballphysik

### 6.1 Grundsatz

Eigene, **deterministische Simulation mit festem Zeitschritt** (z. B. 240 Hz, Sub-Steps), in C++ ohne Abhängigkeit
von der Bildrate. Gleiche Eingaben ergeben auf jedem PC exakt dasselbe Ergebnis (wichtig für Online und Videobeweis).
Konstanten und Formeln werden neu hergeleitet (reale Ballphysik, 24.3) und in `Data/` abgelegt; die Herleitung steht in `Dokumente/Werte.md`.

### 6.2 Modell

- Ball: Durchmesser ≈ 6,7 cm, Masse ≈ 57–58 g.
- Kräfte im Flug: Schwerkraft, Luftwiderstand (∝ v²), **Magnus-Kraft** aus Drall (Topspin drückt nach unten,
  Slice trägt), Drall-Abnahme über die Zeit.
- **Absprung** je Belag: Stoßzahl (vertikal) und Reibung (horizontal) bestimmen Höhe und Tempo nach dem Aufsprung;
  Drall verändert den Absprung (Topspin springt hoch und schnell nach vorn, Slice bleibt tief, Stopp springt
  zurück). Werte je Belag (Hart / Sand / Rasen / Halle) in einer Data Table.
- **Netz:** Netzband als Zylinder; Netzroller (Ball springt vom Band hoch und fällt über das Netz oder zurück),
  Ball ins Netz fällt zurück.
- **Vorhersage:** Für KI, Animation und Timing-Meter wird die Flugbahn vorausberechnet (gleiche Simulation, schneller
  Durchlauf): Aufsprungpunkt, Höhe am möglichen Treffpunkt, Zeit bis zum Treffpunkt.
- **Schlagberechnung:** Aus Ziel, Schlagart, Power/Präzision, Timing-Fehler und Spielerwerten wird die
  Abfluggeschwindigkeit + Drall gesucht, mit der der Ball das (gestreute) Ziel erreicht und das Netz überquert.
- **Kontaktfläche beim Aufsprung** (für Hawk-Eye): Der Ball wird gestaucht; die Kontaktfläche ist ein **länglicher
  Abdruck in Flugrichtung**, schnelle Bälle rutschen länger. Länge und Breite aus Tempo und Aufprallwinkel
  (eigene Formel, in `Dokumente/Werte.md` begründet).

---

## 7. Linienrichter, Challenge und Videobeweis

### 7.1 Linienrichter

- Bei Bällen **nahe der Linie (bis 15 cm Abstand)** liegt der Linienrichter in **90 %** der Fälle richtig und in **10 %**
  daneben – **je knapper der Ball, desto eher falsch**. Weiter weg ist jede Entscheidung richtig.
- Umsetzung (Vorschlag, Durchschnitt 10 %): Fehlerwahrscheinlichkeit `p(d) = 0,2 × (1 − d / 15 cm)` für `d ≤ 15 cm`,
  sonst 0 – Mittelwert und Verlauf **[PRÜFEN]** per Simulation (Ziel: im Durchschnitt über alle knappen Bälle ca. 10 % Fehler).
- Ansage per Stimme und Geste (Arm seitlich ausstrecken = Aus, Hände zusammen/nach unten = gut), „Fault“ beim Aufschlag.
- **Linienrichter als Figuren [NEU]:** sitzen bzw. stehen an den Linien, stehen beim Aus-Ruf auf. Bei der Option
  „Elektronische Linienrichter“ (moderne Tour) gibt es keine Fehler, keine Challenges, nur die automatische Ansage –
  als Einstellung je Turnier/Match.

### 7.2 Challenge-Regeln

- Nach dem Punkt **H** (Controller **Y**) drücken; das Challenge-Fenster ist kurz (**[NEU]** ca. 4 Sekunden, mit
  kleinem Symbol angezeigt).
- **Challengen darf nur, gegen wen entschieden wurde.**
- **3 Challenges pro Satz, +1 im Tiebreak**; **erfolgreiche Challenges bleiben erhalten**.
- **Aufschlag fälschlich als Fehler gegeben:** Wiederholung mit **erstem Aufschlag** – bzw. **als Ass gewertet**, wenn der
  Rückschläger chancenlos war.
- **[PRÜFEN/NEU] Im Ballwechsel fälschlich Aus gerufen** (Ball war gut): Punkt wird wiederholt; war der Schlag des
  Spielers ein klarer Winner, bekommt er den Punkt.
- **Ball fälschlich gut gegeben** (war aus): Punkt für den Challenger.
- Ansagen des Schiedsrichters: „… is challenging the call on the left baseline“ (Linie wird genannt),
  „Correction, the ball was good.“ bzw. „… was out.“, „Replay the point. First serve.“
- Die **KI nutzt das System ebenfalls** (challengt knappe Bälle mit einer Wahrscheinlichkeit, die vom Abstand und
  von den übrigen Challenges abhängt; nicht bei klaren Bällen; selten auch strategisch bei wichtigen Punkten).

### 7.3 Videobeweis (Hawk-Eye)

1. Der Spieler **hebt die Hand**, der Schiedsrichter sagt die Challenge an.
2. Schnitt auf einen **virtuellen Platz** (ohne Publikum, stilisiert, Belagfarbe der Anlage).
3. Der Ball fliegt in **Zeitlupe** und zieht eine **orange Spur** hinter sich her.
4. Die Kamera geht **tief an den Aufsprung**, dann in die **Draufsicht**.
5. Wie bei Hawk-Eye zählt die **Kontaktfläche des zusammengedrückten Balls**: länglicher Abdruck in Flugrichtung;
   **„In“, sobald diese Fläche die Linie berührt.** Die Anzeige nennt den **Abstand zur Linie** (in mm).
6. Ergebnis groß eingeblendet („IN“ / „OUT“), Publikum reagiert („Ohhh“ / Applaus).
7. **[NEU]** Die Sequenz läuft gleichzeitig auf den Videowänden der Anlage; sie ist mit Taste überspringbar
   (Ergebnis wird dann sofort gezeigt).

### 7.4 Sand-Besonderheit [NEU, optional]

Auf Sand gibt es auf der echten Tour oft kein Hawk-Eye, sondern der Schiedsrichter prüft die Ballmarke. Option je
Turnier: „Ballmarke prüfen“ – Schiedsrichter steigt vom Stuhl, Kamera zeigt die Marke aus der Nähe, gleiche
Entscheidungslogik wie Hawk-Eye. Standard: Hawk-Eye überall.

---

## 8. Zählweise

- Punkte: 0, 15, 30, 40, Einstand, Vorteil, Spiel.
- **Gewinnsätze** einstellbar (1, 2, 3), **Spiele pro Satz** einstellbar (z. B. 1–6; Standard 6).
- **Tiebreak** bei Gleichstand der Spiele (Standard 6:6) bis 7 mit 2 Punkten Abstand; Aufschlagwechsel nach dem
  ersten und dann alle zwei Punkte; Seitenwechsel alle 6 Punkte.
- **Seitenwechsel** nach jedem ungeraden Spiel; nach dem ersten Spiel eines Satzes ohne Pause, sonst kurze Pause
  (Spieler setzen sich, trinken – **[NEU]**).
- **[NEU, Optionen]**
  - Entscheidungssatz: Tiebreak bis 10 bei 6:6 (wie bei den Grand Slams), normaler Tiebreak oder „ausspielen“.
  - Match-Tiebreak statt drittem Satz (wie im Doppel/Teamwettbewerben).
  - No-Ad (Entscheidungspunkt bei Einstand).
- Ansagen und Anzeige wie auf der Tour (Satzstand, Spielstand, Aufschlag-Markierung, Breakball/Satzball/Matchball-
  Einblendung **[NEU]**).
- Alles in `Match/Scoring` als reine Logik ohne Grafik – vollständig getestet.

---

## 9. Spieler, Kader und KI

### 9.1 Kader

- Fester Kader, **neu entworfen** (Namen, Nation, Werte, Spielstil, Standard-Outfit, Anzahl Balltipper, Rückhand-Art,
  Schlaghand). Vorgaben:
  - **Leon Adler** und **Lara Falk** – die 99er-Spieler (Gesamtwert 99),
  - **Holloway** und **Moreau** – einhändige Rückhand,
  - die übrigen Spieler legt Claude Code als Vorschlag an (`Dokumente/Kader.md`) und zeigt sie mir zur Abnahme.
- Alle Namen bleiben erfunden.
- **[NEU]** Kader auf mindestens 10 Männer und 10 Frauen auslegen (für Schnelles Match), weitere Karriere-Gegner
  werden prozedural erzeugt (Name aus Namenslisten je Land, Werte nach Rangliste, Aussehen aus den Grundkörpern).

### 9.2 Spielerwerte

| Wert | Wirkung |
|---|---|
| Aufschlag | Tempo, Genauigkeit, Fenster beim Aufschlag |
| Vorhand / Rückhand | Fenster, Tempo, Streuung der Grundschläge |
| Volley | Fenster und Reichweite am Netz, Schmetterball |
| Schnelligkeit | Laufgeschwindigkeit, Antritt |
| Ausdauer | Erholung, Abfall in langen Matches **[NEU]** |
| Nerven | Streuung bei Break-/Satz-/Matchbällen **[NEU]** |
| Spin | Drall-Menge bei Topspin/Slice |
| Return **[NEU]** | Fenster beim Return |

Gesamtwert 0–99 (Durchschnitt der gewichteten Werte). Dazu: Spielstil (Grundlinie, Serve-and-Volley, Allround,
Konter), Lieblingsbelag (kleiner Bonus).

### 9.3 KI

- Positionierung (zur Mitte zurück, Winkel abdecken), Schlagwahl nach Situation (Lage, Höhe, Gegnerposition,
  Spielstil), Ziel mit Streuung abhängig von Werten und Schwierigkeit.
- **Schwierigkeitsgrade:** Leicht, Mittel, Schwer, Profi (Zahlenwerte neu abstimmen, per Probespielen); in der Karriere ergibt sich
  die Stärke aus den Gegnerwerten.
- KI macht Fehler „menschlich“: mehr unter Druck, bei weiten Wegen, bei niedriger Ausdauer.
- KI nutzt Challenges (7.2) und zeigt dieselben Reaktionen zwischen den Punkten (11).

---

## 10. Kleidung und Outfits

### 10.1 Drucke

Bedruckte Outfits mit Mustern, die **nahtlos um Körper und Ärmel** laufen, dazu ein **kleines Emblem auf der Brust**:
Querstreifen, Diagonal, Pfeil, Farbverlauf als Halbton, Seitenteile, Schulterpasse, Punkte, Pinselstriche, Zickzack,
Karo, Wellen, Farbkleckse, Blätter, Uni.

**In Unreal:** Die Muster werden als **Material-Funktionen** umgesetzt (prozedural bzw. aus kachelbaren Masken), gesteuert
über Material-Parameter (Grundfarbe, Druckfarbe = Akzent, Muster, Skalierung, Drehung). Die UV-Abwicklung der
Kleidungsstücke wird in Blender so angelegt, dass Muster über Nähte durchlaufen (oder Triplanar/Objekt-Raum-Projektion).
Damit entfällt das Malen von Texturen zur Laufzeit – Outfitwechsel sind sofort sichtbar.

### 10.2 Schnitte (echte Kleidungsstücke)

Komplett neu in Blender modelliert – nichts wird übernommen; Vorbild ist übliche Tenniskleidung – und für jeden Grundkörper angepasst:

- Oberteile: **Polo** (Kragen, Knopfleiste), **T-Shirt**, **Tanktop** (Schlüsselloch-Ausschnitt), **[NEU]** Kleid (Tenniskleid).
- Unterteile: **Shorts**, **kurze Shorts** (mit Kordel), **Stufenrock** (drei Volants), **[NEU]** Faltenrock.
- Säume, Kragen und Volants sind echte Geometrie; Falten- und Nahtkarten, feine Stoffstruktur, Faltenschatten.
- **Stoff schwingt** beim Laufen, Abbremsen und Drehen nach (Chaos Cloth auf Rock-Volants, Saum und Shorts;
  auf schwächerer Grafik nur Knochen-Nachschwingen).
- Unter der Kleidung wird der Körper ausgeblendet bzw. eingezogen (Masken je Kleidungsstück), damit nichts durchsticht.
- **Spielerinnen stecken das Oberteil in den Rock, über Shorts hängt es darüber.**
- Alle Figuren tragen weiße Tennisschuhe mit Söckchen (3.3).
- **[NEU]** Accessoires: Stirnband, Schweißbänder, Kappe, Visor, Sonnenbrille (nur draußen), Armbanduhr.
- **[NEU]** Kleidung wird in langen Matches leicht verschwitzt (Material-Parameter), auf Sand an Socken/Schuhen
  rötlich eingestaubt.

### 10.3 Auswahl

- Jeder Spieler hat ein eigenes **Standard-Outfit**.
- Schnelles Match: **OUTFIT** und **SCHNITT** auf der Spielerkarte wählbar.
- Online-Lobby: jeder wählt Spieler, Outfit und Schnitt.
- Karriere: bei der Erstellung (Druck, Hose: Uni / Seitenstreifen / wie das Shirt, Druckfarbe = Akzent) und jederzeit
  in der Karriere-Übersicht.
- **[NEU]** Karriere: Sponsoren-Outfits als Belohnung, eigene Sets speichern.
- Grafikoption **Kleidung**: „Aufgemalt“ (sparsam, Kleidung nur als Textur auf dem Körper) oder „Echte Kleidungsstücke“
  (ab Mittel), dazu „Stoff-Simulation an/aus“.

---

## 11. Verhalten zwischen den Punkten

- Nach dem Punkt geht der **Sieger stolz oder fröhlich** zurück, der **Verlierer traurig, wütend oder unsicher**;
  nach **langen Ballwechseln** gehen **beide müde und langsamer**.
- Gesten: Kopfschütteln, am Kopf kratzen, Gesicht abwischen, anerkennend nicken oder applaudieren (nach einem tollen
  Schlag des Gegners), in die Hocke gehen nach einem langen verlorenen Ballwechsel, auf Sand nach dem Rutschen den
  Staub abklopfen, mit dem Schiedsrichter diskutieren (nach falscher Entscheidung), Faust ballen („Come on!“),
  Durchatmen, Achselzucken.
- Nach dem **Matchball**: winken, mit beiden Händen winken, sich verbeugen; Verlierer gratuliert am Netz
  (**[NEU]** Handschlag am Netz, dann Handschlag mit dem Schiedsrichter).
- **[NEU]** Weitere Abläufe: Handtuch vom Ballkind/aus der Box holen, Saiten richten, Bälle vom Ballkind bekommen und
  zwei Bälle für den Aufschlag auswählen, beim Seitenwechsel auf die Bank setzen und trinken.
- Auswahl der Reaktion nach Spielstand, Wichtigkeit des Punktes, Länge des Ballwechsels und Persönlichkeit
  (ruhig / emotional) des Spielers.
- Zwischen-Punkt-Abläufe sind überspringbar (Taste), der Aufschläger kann jederzeit bereit sein.

---

## 12. Anlagen, Plätze und Hallen

### 12.1 Der Platz

- Exakte Maße (siehe 24): Einzelfeld 23,77 × 8,23 m, Doppelfeld 10,97 m breit, Netz 0,914 m in der Mitte,
  1,07 m an den Pfosten; Auslauf hinter der Grundlinie und seitlich je nach Anlage.
- Linien 5 cm breit, Grundlinie bis 10 cm **[PRÜFEN]**.

### 12.2 Beläge

| Belag | Tempo | Absprung | Rutschen | Spuren |
|---|---|---|---|---|
| Hartplatz (draußen) | mittel–schnell | mittel–hoch | kurz, Quietschen | Gummistreifen, Ballabdrücke |
| Sand | langsam | hoch | weit, Staub | Fußabdrücke, Rutschspuren, Ballmarken |
| Rasen | schnell | tief | mittel | Fußabdrücke, abgetretene Stellen |
| Halle (Hart) **[NEU]** | schnell | mittel | kurz, Quietschen, Hall | Gummistreifen |

**[NEU]** Feineinstellung je Turnier (Court-Pace-Wert), damit z. B. ein schneller und ein langsamer Hartplatz
sich unterscheiden.

### 12.3 Abnutzung

Alle Plätze sind abgenutzt wie nach einigen Turniertagen:

- **Hartplatz:** abgeriebene Farbe, Gummistreifen und Ballabdrücke hinter der Grundlinie.
- **Sand:** Fußspuren, Rutschspuren, eingestaubte Linien; **[NEU]** zwischen den Sätzen wird der Platz abgezogen
  (Platzwarte mit Schleppnetz, Linien gefegt) – kurze Zwischensequenz, überspringbar.
- **Rasen:** braun abgetretene Grundlinie und der Weg der Serve-and-Volley-Spieler; **[NEU]** Abnutzung nimmt mit der
  Turnierwoche zu (erste Runde fast grün, Finale braun an der Grundlinie).

Darüber die Feinstruktur der Oberflächen (selbst erstellte Detailtexturen, siehe 3.6): Sandkörner und Abziehspuren, Grashalme, körnige
Acryl-Beschichtung – ab „Hoch“ mit Relief und Glanz, bei „Mittel“ als Helligkeit, bei „Niedrig“ ohne.
Laufzeit-Spuren aus dem aktuellen Match kommen dazu (3.6).

### 12.4 Die Anlagen

1. **Hartplatz – Arena:** blaue Schalensitze, Dachring, Flutlicht, Videowände.
2. **Sandplatz – Court Central:** grüne Banden, helle Steintribünen, Fahnen, Bäume.
3. **Rasen – Centre Court:** Efeu-Wände, Dach über den oberen Reihen, klassische Anzeigetafel.
4. **Training:** eigene Trainingsanlage **ohne Publikum** – Zaun mit Windschutz, Nachbarplätze, Clubhaus, Flutlicht,
   Ballkorb – auf dem gewählten Belag.

### 12.5 Hallen [NEU]

Für die Hallensaison im Herbst, die Finals und einige Winter-Turniere:

5. **Halle – Arena Indoor:** Hartplatz in einer großen Mehrzweckhalle, dunkle Tribünen, Lichtbänder unter dem Dach,
   großer Videowürfel über dem Platz, Bodenprojektion/Lichtshow beim Einlauf.
6. **Halle – Finals:** wie 5, aber komplett verdunkeltes Publikum, Spot nur auf den Platz, dunkler Belag (z. B.
   dunkelblau/schwarz), Einlauf mit Rauch und Licht.
7. **Halle – Turnierhalle (klein):** kleinere Halle mit Holzwänden und Klapptribünen für Challenger-/ITF-Hallenturniere,
   auch als Hallen-Sandplatz verwendbar (Winter-Turniere).
8. **Halle – Trainingshalle:** Traglufthalle bzw. Leichtbauhalle mit Nachbarplätzen, für das Training im Winter.

Hallen-Besonderheiten: kein Wind, kein Sonnenschatten, mehr Hall im Ton (Quietschen, Ballschläge, Publikum lauter),
Licht gleichmäßig von oben.

### 12.6 Platzgrößen je Turnierstufe [NEU]

Damit die Karriere sich steigert, gibt es pro Anlage Varianten:

| Stufe | Platz | Publikum |
|---|---|---|
| ITF / Qualifikation | Außenplatz mit kleiner Tribüne (2–5 Reihen), Zaun, wenige Zuschauer | 0–200 |
| Challenger / WTA 125, frühe Runden der Tour | Nebenplatz mit Tribüne auf 1–2 Seiten | 500–3.000 |
| Tour, späte Runden | Hauptplatz der Anlage | 5.000–15.000 |
| Grand Slam / Finals | große Arena bzw. Centre Court, volle Ränge, Dach | 15.000+ |

Alle Varianten entstehen aus denselben Bausteinen (3.5) per Aufbau-Skript (`Tools/Python/build_venue.py`) aus einer
Data Table: Belag, Belagfarbe, Tribünenart/-farbe, Größe, Dach, Halle ja/nein, Banden- und Sponsorenfarben,
Anzeigetafel-Typ, Bäume/Fahnen. So bekommt **jedes Turnier ein eigenes Aussehen**, ohne dass jedes einzeln gebaut wird.

### 12.7 Tageszeit und Wetter [NEU]

- Draußen: Tag (Sonne mit Schatten auf dem Platz), später Nachmittag, Abend mit Flutlicht. Tageszeit je Match aus dem
  Turnierplan (Nachtsession bei großen Turnieren).
- Wetter: sonnig, bewölkt, leichter Wind (kleiner Einfluss auf Lobs und Ballwurf beim Aufschlag, abschaltbar).
  **Regen** unterbricht ein Match draußen (Planen werden aufgezogen, Fortsetzung) – auf Plätzen mit Dach wird das Dach
  geschlossen. Nur in der Karriere, abschaltbar.

### 12.8 Personen auf der Anlage

- Schiedsrichter auf dem Stuhl (Kopfbewegungen, Gesten, steigt bei Ballmarke/Matchende herunter).
- Linienrichter (7.1).
- **[NEU]** Ballkinder: laufen Netzbälle ab, rollen Bälle zum Aufschläger, reichen Handtücher, stehen an Netz und
  Ecken – einfache Figuren mit wenigen Animationen.
- Publikum (3.7) reagiert: verfolgt den Ball mit dem Kopf, Applaus nach Punkten (stärker bei langen Ballwechseln und
  wichtigen Punkten), „Ohhh“ bei knappen Bällen, Stille vor dem Aufschlag, „Quiet please“ des Schiedsrichters bei
  Unruhe **[NEU]**.
- Spielerbox **[NEU]**: Trainer und Familie des Spielers in einer Loge, reagieren auf Punkte.

---

## 13. Spielmodi

### 13.1 Schnelles Match

- Spieler 1 und Gegner wählen (Kader), je **Outfit** und **Schnitt**.
- Einstellungen: Anlage/Belag, Tageszeit **[NEU]**, Gewinnsätze, Spiele pro Satz, Entscheidungssatz-Regel **[NEU]**,
  Schwierigkeit, Timing-Fenster, Challenges an/aus bzw. elektronische Linienrichter **[NEU]**.
- **[NEU]** Lokales 2-Spieler-Match an einem PC (Tastatur + Controller oder zwei Controller) mit geteilter oder
  gemeinsamer Kamera – optional, nur wenn der Aufwand vertretbar ist.

### 13.2 Training

- Ballmaschine auf der Trainingsanlage (draußen oder **[NEU]** in der Trainingshalle), Belag wählbar.
- Einstellungen der Ballmaschine: Tempo, Drall, Höhe, Position (Vorhand/Rückhand/abwechselnd/zufällig), Takt.
- **[NEU]** Übungen mit Zielen und Punktewertung: Zielfelder treffen, Aufschlag-Ziele, Volley-Drill, Stopp-Übung,
  Schmetterball-Übung. Bestwerte werden gespeichert; in der Karriere geben sie Erfahrungspunkte.

### 13.3 Karriere

Siehe Abschnitt 14.

### 13.4 Online

Siehe Abschnitt 15.

### 13.5 Hauptmenü-Vorschau

Beim Auswählen eines Menüpunkts laufen im Hintergrund **kurze simulierte Gameplay-Szenen** (zwei KI-Spieler, passende
Anlage, Kamerafahrten) – z. B. Karriere: großes Stadion; Training: Ballmaschine; Online: zwei Spieler am Netz.

---

## 14. Karrieremodus

### 14.1 Spieler erstellen

- Vor- und Nachname (Bildschirmtastatur), Land, **Tour: ATP (Männer) oder WTA (Frauen)**.
- Aussehen: Grundkörper, Größe, Statur, Gesicht, Hautton, Frisur, Haarfarbe (3.3).
- Schlaghand (rechts/links) **[NEU]**, Rückhand einhändig/beidhändig, Anzahl Balltipper (1–4), Spielstil.
- Outfit (Druck, Hose: Uni / Seitenstreifen / wie das Shirt, Druckfarbe = Akzent) und Schnitt.
- Startwerte: niedrig (Gesamt ~50 **[PRÜFEN]**), einige Startpunkte frei verteilbar.
- **Schwierigkeit der Karriere [NEU]:** bestimmt KI-Stärke und Erfahrungspunkte.

### 14.2 Aufbau der Tour

Der Weg geht **von der ITF über Challenger / WTA 125 bis zur ATP- bzw. WTA-Tour** mit Grand Slams, Masters 1000 bzw.
WTA 1000 und den Finals. Echte Turniernamen sind im Karrieremodus erlaubt (privates Projekt, siehe 22.3).

| Stufe ATP | Stufe WTA | Siegerpunkte (Richtwert) |
|---|---|---|
| ITF M15 / M25 | ITF W15 – W100 | 15 / 25 bzw. 15–100 |
| Challenger 50 – 175 | WTA 125 | 50–175 bzw. 125 |
| ATP 250 | WTA 250 | 250 |
| ATP 500 | WTA 500 | 500 |
| Masters 1000 | WTA 1000 | 1000 |
| Grand Slam | Grand Slam | 2000 |
| ATP Finals | WTA Finals | bis 1500 (ungeschlagen) |

Punkte je Runde und Preisgelder aus den aktuellen offiziellen Tabellen von ATP und WTA recherchieren und in `Data/`
ablegen **[PRÜFEN]** (Echte Daten sind für dieses private Projekt in Ordnung, siehe 22.3).

### 14.3 Saisonkalender

- Kalender in **Wochen** (Januar bis November), Turniere je Woche aus einer Data Table (Name, Stadt, Land, Stufe,
  Belag, draußen/Halle, Anlagen-Variante, Feldgröße, Qualifikation ja/nein, Preisgeld, Datum).
- Belag-Abschnitte wie auf der echten Tour: Hartplatz-Start (Australien), Hallen im Februar, Hartplatz-Frühjahr,
  Sandsaison (April–Juni), kurze Rasensaison (Juni–Juli), Hartplatz-Sommer (Nordamerika), Asien-Herbst, Hallensaison
  in Europa, Finals im November. **[NEU]** Hallenturniere nutzen die Hallen aus 12.5.
- Pro Woche wählt der Spieler ein Turnier, für das er zugelassen ist – oder **Trainingswoche / Pause [NEU]**.
- **[NEU]** Saisonplanung: Übersicht über die nächsten Wochen mit Meldeschluss, Zulassungsgrenzen und erwarteten Gegnern.

### 14.4 Zulassung, Qualifikation, Auslosung

- **Zulassung** nach Weltranglistenplatz: jedes Turnier hat eine Grenze fürs Hauptfeld und für die Qualifikation.
- **Qualifikation** (2–3 Runden) vor dem Hauptfeld; Lucky Loser bei Absagen **[NEU]**.
- **Wildcards [NEU]:** gelegentlich bei Heimturnieren oder nach guten Ergebnissen angeboten.
- **Setzliste** nach Rangliste, **Auslosung** nach echten Regeln (Gesetzte verteilt, Freilose bei kleineren Feldern).
- Turnierbaum-Ansicht mit allen Ergebnissen.

### 14.5 Matches und Simulation

- Eigene Matches werden gespielt – **[NEU]** oder auf Wunsch simuliert (Ergebnis aus Werten, Form, Belag, Zufall;
  dann weniger Erfahrungspunkte).
- Alle anderen Matches (andere Spieler, ganze Turniere in anderen Wochen) werden im Hintergrund simuliert, damit sich
  die Rangliste bewegt.
- Format je Turnier: Grand Slams bei Männern 3 Gewinnsätze, sonst 2; Spiele pro Satz in den Karriere-Optionen
  verkürzbar (z. B. 3 oder 4 Spiele), damit eine Karriere spielbar bleibt.

### 14.6 Weltrangliste

- **Rollierend über 52 Wochen**, die besten Ergebnisse zählen (Anzahl wie auf der echten Tour, **[PRÜFEN]**), Pflichtturniere
  für Top-Spieler optional **[NEU]**.
- Komplett simulierte Rangliste (z. B. 500 Spieler je Tour), mit Bewegung nach jeder Woche.
- Ansichten: Rangliste, eigener Verlauf (Grafik), **Race** zu den Finals **[NEU]**.
- **Finals:** die besten 8 der Saison-Race, Gruppenphase (2 Gruppen à 4) + Halbfinale + Finale.

### 14.7 Fortschritt

- **Preisgeld** pro Runde, Kontostand.
- **[NEU] Ausgaben:** Reisekosten je Turnier (nach Entfernung/Kontinent), Trainer, Physio – zwingen zu sinnvoller
  Planung in der ITF-Phase; später durch Sponsoren ausgeglichen. Abschaltbar.
- **Level** und **Erfahrungspunkte** aus Matches (Sieg, Runde, Gegnerstärke), Training und Übungen.
- **Fähigkeitspunkte** pro Level → Werte steigern (9.2). **[NEU]** Fähigkeitsbaum mit besonderen Fähigkeiten
  (z. B. „Rutsch-Spezialist“, „Netzläufer“, „Kick-Aufschlag“, „Nervenstark“), max. 3 gleichzeitig aktiv.
- **[NEU] Trainer:** anwerbbar, mit Schwerpunkt (schnellere Steigerung eines Bereichs).
- **[NEU] Form und Erschöpfung:** viele Turniere in Folge senken die Frische; Pausen erholen. Kleine, zeitlich
  begrenzte Verletzungen nur als Option (standardmäßig aus).
- **[NEU] Sponsoren:** ab bestimmten Ranglistenplätzen Angebote (Geld pro Saison, Outfit-/Schlägerdesign),
  Bedingungen (z. B. Top 100 halten).

### 14.8 Karriere-Übersicht

Hub-Menü mit: nächstes Turnier / Kalender, Rangliste, Spielerprofil (Werte, Fähigkeiten), Outfit & Schnitt ändern
(jederzeit), Statistiken (Siege/Niederlagen je Belag, Asse, Doppelfehler, Breakbälle, Challenges), **[NEU]**
Trophäenschrank (3D-Pokale aus Blender je Turnierstufe), **[NEU]** Nachrichten (kurze Meldungen: „Erster Titel auf der
Challenger-Tour“, Rivalen, Rekorde), **[NEU]** Rivalen (Kopf-an-Kopf-Bilanz), Speichern.

### 14.9 Besondere Momente [NEU]

- Einlauf bei großen Turnieren (Kamerafahrt, Stadionsprecher-Ansage per ElevenLabs-Bausteinen).
- Siegerehrung nach Finals (Pokal hochheben, Konfetti bei Grand Slams).
- Saisonende-Zusammenfassung.

### 14.10 Speicherstände

- Mehrere Karrieren (z. B. 3 Plätze), automatisches Speichern nach jedem Match und jeder Woche.
- Eine Karriere aus dem Browser- oder Godot-Spiel wird **nicht** übernommen.

---

## 15. Online gegen Freunde

### 15.1 Ablauf

1. Einer klickt **SPIEL ERSTELLEN** und bekommt einen **Code aus vier Zeichen** (z. B. `K7FQ`).
2. Der Freund gibt den Code bei **BEITRETEN** ein. Beide landen in der **Lobby**: jeder wählt seinen Spieler, sein Outfit
   und den Schnitt. **Belag, Gewinnsätze, Spiele pro Satz und das Timing-Fenster** (gilt für beide) stellt der
   **Gastgeber** ein und startet das Match.
3. Nach dem Match geht es mit **ZUR LOBBY** gleich weiter (Revanche), **VERLASSEN** trennt die Verbindung.

### 15.2 Technik in Unreal

- **Epic Online Services** (kostenlos): Lobby mit dem Code als Suchattribut, Verbindung **direkt P2P** (NAT-Traversal),
  bei Problemen (strenge Firewall, Handy-Hotspot) automatisch über **EOS-Relay** (etwas mehr Verzögerung).
  Anmeldung über eine anonyme Geräte-ID (kein Epic-Konto für Spieler nötig) **[PRÜFEN]**.
- Ich lege dafür ein Produkt im Epic Developer Portal an; die Zugangsdaten kommen in eine nicht versionierte
  Konfigurationsdatei.
- **Synchronisierung** (nicht klassisches Server-Replizieren):
  - **Jeder PC steuert den eigenen Spieler** (Position wird laufend gesendet und auf der Gegenseite geglättet).
  - **Schläge werden als Ereignis gesendet** (Zeitpunkt, Treffpunkt, Abfluggeschwindigkeit, Drall); die
    **Ballphysik rechnet auf beiden PCs exakt gleich** (deterministisch, 6.1).
  - **Über Aus, Netz und Punkt entscheidet der PC, zu dem der Ball fliegt**; er sendet das Ergebnis.
  - Linienrichter-Zufall wird mit einem gemeinsamen Zufalls-Startwert pro Punkt erzeugt, damit beide dasselbe sehen.
- **Challengen darf nur, gegen wen entschieden wurde** (3 pro Satz, wie 7.2).
- **Esc** öffnet online nur das Menü – das **Match läuft weiter**. **Aufgeben** = Punktsieg des Freundes.
- Bricht die Verbindung ab, kommt man zurück ins Online-Menü (**[NEU]** mit 20 Sekunden Wiederverbindungsversuch).
- Jeder sieht sich selbst vorne.
- **[NEU]** Ping-Anzeige in der Lobby, Hinweis bei Relay-Verbindung.

### 15.3 Nur gleiche Versionen

- Beim Beitreten wird der **Build verglichen**: eine Prüfsumme über die gepackten Spieldateien (`.pak`/`.utoc`/`.ucas`
  und die `.exe`), angezeigt unten rechts im Online-Menü, z. B. „Version 2.0 · Build 6e5689499cda“.
- Ist auch nur eine Datei anders, **lehnt das Spiel ab** und zeigt beide Versionen an – und bietet, wenn man selbst die
  ältere Version hat, **direkt das Update an** (siehe 19.4.7).
- Nur Unreal ↔ Unreal (keine Verbindung zur Browser- oder Godot-Fassung).

### 15.4 Spiel weitergeben

- **`Spiel für Freunde packen.bat`** → auf dem Desktop liegt `Matchball Tennis <Version> (<Build>).zip`.
- Entpacken, `Matchball Tennis.exe` starten. Windows warnt beim ersten Start evtl. („Unbekannter Herausgeber“) →
  „Weitere Informationen“ → „Trotzdem ausführen“.
- Die ZIP enthält keine Quelldateien, keine `SourceAssets/`, aber den Updater und `installiert.json`.
- Das ist nur für die **Erstinstallation** nötig – alle weiteren Versionen kommen über das Update-System (19.4).

---

## 16. Ton, Schiedsrichter-Stimme, Publikum

### 16.1 Geräusche

- Ballschläge je Schlagart und Treffqualität (sauber / Rahmen / Power), Aufsprung je Belag, Netz, Netzband.
- Schritte je Belag, Schuhquietschen (Hartplatz/Halle), Knirschen und Rutschen auf Sand.
- Spieler-Laute bei Anstrengung **[NEU]** (selbst aufgenommen oder freie Quellen, je Spieler Lautstärke/Häufigkeit).
- Publikum: Grundrauschen, Applaus (gedämpft mit Stadionhall als Hintergrundgeräusch), „Ohhh“, Jubel, Stille vor dem
  Aufschlag.
- Hall je Anlage (Arena, offener Platz, Halle) über Audio-Volumes/Submix-Effekte.
- **Alle Klänge werden neu und selbst erstellt** (keine fremden Klangbibliotheken):
  - **Eigene Aufnahmen:** Claude Code schreibt mir eine Aufnahmeliste (`Dokumente/Aufnahmeliste.md`: was, wo, wie oft,
    Abstand zum Handy) – z. B. Ballschläge (sauber, Rahmen, hart), Aufsprung auf Hartplatz/Sand/Rasen, Schritte,
    Quietschen, Rutschen auf Sand, Ball ins Netz, Netzband, Ball auftippen, Applaus und Raunen im Verein.
    Ein Skript bereinigt die Aufnahmen (Rauschen, Lautstärke, Schnitt, Varianten) und importiert sie.
  - **Selbst erzeugt (Synthese):** was sich nicht aufnehmen lässt – großes Stadionpublikum, Grundrauschen, Raunen,
    Hall der Anlagen – wird per MetaSounds bzw. Python-Audiosynthese erzeugt bzw. aus vielen eigenen Aufnahmen
    geschichtet (wenige klatschende Personen → tausende Zuschauer).
  - **Ausweichlösung (nur nach Rückfrage):** CC0-Klänge, z. B. BigSoundBank.

### 16.2 Schiedsrichter-Stimme

- Englische Ansagen wie auf der Tour: Punktestand („Fifteen–Love“, „Deuce“, „Advantage Adler“, „Game Falk“,
  „Game and first set Adler, six games to four“), „Fault“, „Out“, „Let“, „Time“, „Quiet please“ **[NEU]**,
  „Ladies and gentlemen …“ **[NEU]**, Challenge-Ansagen (7.2), „Game, set and match …“.
- **ElevenLabs (optional):** `ElevenLabs-Stimme erzeugen.bat` → Schlüssel eingeben (wird nicht gespeichert),
  Standard-Stimme „Alice“ (britisch), andere Stimme per Parameter, eigene Namen per Parameter.
  Das Skript erzeugt **Bausteine** (Satzteile + Namen) als WAV und legt sie in `Content/Audio/Umpire/` (Import per Skript).
  Bei neuen Ansagen erzeugt es **nur die fehlenden Bausteine**.
- In den **Optionen** steht bei „Schiedsrichter-Stimme“: AUS / AN · WINDOWS / AN · ELEVENLABS, und welche Namen noch
  keine Aufnahme haben.
- **Fallback** für fehlende Bausteine: Windows-Sprachausgabe über das Unreal-Text-to-Speech-Plugin **[PRÜFEN]**;
  sonst wird die kürzere Ansage ohne Namen verwendet („Game“, „Advantage server“).
- **[NEU]** Stadionsprecher für Einlauf und Siegerehrung (eigene ElevenLabs-Stimme).

---

## 17. Menüs, HUD und Controller

### 17.1 Menüs

- **Hauptmenü:** KARRIERE, SCHNELLES MATCH, ONLINE, TRAINING, OPTIONEN, CREDITS **[NEU]**, BEENDEN – mit
  Hintergrund-Vorschauszenen (13.5).
- **Optionen:** Grafik (18), Ton (Gesamt, Effekte, Publikum, Stimme, Musik **[NEU]**), Schiedsrichter-Stimme,
  Steuerung (Tastenbelegung ändern **[NEU]**, Vibration), Spiel (Timing-Fenster-Standard, Zwischensequenzen
  überspringen, Einblendungen), Sprache (Deutsch; Schiedsrichter immer Englisch).
- **Pause-Menü:** Weiter, Steuerung, Optionen, Aufgeben, Hauptmenü.
- **Match-Ende:** Ergebnis, **[NEU]** Statistik wie im TV (Asse, Doppelfehler, 1. Aufschlag %, Winner, unerzwungene
  Fehler, Breakbälle, gewonnene Punkte, längster Ballwechsel, schnellster Aufschlag), Revanche / Weiter.
- Das ganze Spiel ist **mit Controller bedienbar**: Steuerkreuz/Stick, A auswählen, B zurück, LB/RB Werte ändern,
  Namen über eine **Bildschirmtastatur**, Rückfragen („Wirklich aufgeben?“) mit A/B.
- Eingabesymbole passen sich automatisch dem zuletzt benutzten Gerät an (Tastatur / Xbox / PlayStation).
- Optik: klare TV-Sport-Grafik, große Schrift, eigene Farben und Logo **[NEU]** (Logo „Matchball Tennis“ selbst gestaltet).

### 17.2 HUD im Match

- Spielstand-Einblendung oben links wie im TV (Namen, Flaggen **[NEU]**, Sätze, Spiele, Punkte, Aufschlag-Punkt).
- Timing-Meter am eigenen Spieler (grün/gelb/rot).
- Ziel-Markierung beim Zielen (dezent, abschaltbar **[NEU]**).
- Verbleibende Challenges.
- Aufschlag-Tempo nach dem Aufschlag **[NEU]**.
- Einblendungen: Breakball, Satzball, Matchball, „Challenge“, Ergebnis Videobeweis.
- FPS-Anzeige (Option).

---

## 18. Grafik-Einstellungen und Leistung

### 18.1 Einstellungen

Unter **Optionen → Grafik-Einstellungen**, Vorgabe **Niedrig / Mittel / Hoch / Ultra**, dazu einzeln:

| Einstellung | Stufen | Umsetzung in Unreal |
|---|---|---|
| Auflösung | Automatisch, 50–100 % | Automatisch = dynamische Auflösung mit Ziel 60 FPS (senkt/hebt laufend) |
| Schatten | Aus, Niedrig, Mittel, Hoch | Shadow Quality / Virtual Shadow Maps ab Hoch |
| Kantenglättung | Aus, FXAA, TSR | Anti-Aliasing-Methode |
| Licht/Reflexionen | Gebacken, Lumen Lite, Lumen | Global Illumination + Reflexionsmethode |
| Spielerfiguren | Niedrig–Hoch | LOD-Bias, Haarkarten-Qualität, Gesichtsdetails |
| Kleidung | Aufgemalt, Echte Kleidungsstücke (+ Stoff-Simulation) | Kleidungs-Meshes, Chaos Cloth an/aus |
| Schläger & Ball | Niedrig–Hoch | Saiten-Detail, Ball-Fussel, Verformung |
| Publikum | Aus, Wenig, Mittel, Voll · statisch/animiert | Instanzen-Dichte, Vertex-Animation an/aus |
| Belag-Details | Niedrig–Hoch **[NEU]** | Relief, Glanz, Laufzeit-Spuren |
| Effekte | Niedrig–Hoch **[NEU]** | Niagara-Partikel (Staub) |
| Bildrate | 30, 60, 120, Unbegrenzt, VSync | Frame Rate Limit |
| FPS-Anzeige | An/Aus | eigenes HUD-Element |

### 18.2 Hardware-Erkennung

- **Onboard-Grafik** (z. B. Intel Iris Xe) wird über den Grafikadapter-Namen erkannt → Start mit **„Mittel“**
  (bzw. „Niedrig“, falls die Messung beim ersten Start das verlangt).
- **[NEU]** Beim ersten Start kurzer automatischer Leistungstest (Unreal Hardware Benchmark), Vorschlag einer Stufe.

### 18.3 Leistungsziele

- **60 FPS** auf Onboard-Grafik mit „Niedrig“/„Mittel“ (1080p mit dynamischer Auflösung).
- Dafür auf Niedrig/Mittel: **gebackenes Licht** (GPU Lightmass) für die statischen Anlagen – die Stadien bewegen sich
  nicht, nur Tageszeit-Varianten werden je einmal gebacken –, keine Virtual Shadow Maps, kein Lumen, reduziertes
  Publikum, Kleidung je nach Stufe.
- Hoch/Ultra: Lumen Lite bzw. Lumen, Virtual Shadow Maps, Nanite für Tribünen und Umgebung, volles Publikum.
- **Vorladen gegen Ruckler:** Figuren, Outfits und Shader werden im Menü vorbereitet (PSO-Cache beim Packen
  erzeugen, Assets der nächsten Anlage asynchron laden), damit der Spielerwechsel und der Matchstart nicht ruckeln.

---

## 19. Start, Speichern, Weitergeben, Updates

### 19.1 Start

- **`Matchball Tennis.exe`** doppelklicken → das Spiel startet im randlosen Vollbild.
  **F11** wechselt zwischen Vollbild und Fenster, **BEENDEN** im Hauptmenü oder **Alt+F4** schließt es.
- Für den Desktop: Rechtsklick auf die .exe → „Senden an“ → „Desktop (Verknüpfung erstellen)“.
- **Keine Internetverbindung nötig** – außer für Online-Spiele und die Update-Suche.
- Windows 11 mit DirectX 12 (Fallback DirectX 11 für ältere Onboard-Grafik **[PRÜFEN]**).

### 19.2 Speichern

- Spielstände und Einstellungen unter `%LocalAppData%\Matchball Tennis\Saved\` (eigener Pfad statt Unreal-Standard,
  damit es leicht auffindbar ist). Dieser Ordner wird **von Updates nie angefasst**.
- Jeder Speicherstand trägt eine **Speicherstand-Version** (Ganzzahl). Ändert ein Update das Format (z. B. neue
  Karriere-Felder), wandelt das Spiel alte Stände beim Laden **automatisch** um (eine Umwandlungsfunktion je
  Versionssprung, mit Tests). Vor jeder Umwandlung wird eine Sicherung `*.bak` angelegt.

### 19.3 Erstinstallation

- Für die **allererste** Installation bleibt **`Spiel für Freunde packen.bat`** (15.4): ZIP mit dem kompletten Spiel.
- Alle späteren Versionen kommen über das Update-System (19.4) – niemand muss das Spiel noch einmal ganz herunterladen.

### 19.4 Update-System (eigener Updater)

**Ziel:** Hat jemand das Spiel einmal installiert, bekommt er neue Versionen per Klick im Spiel, und es werden **nur die
geänderten Teile** heruntergeladen – bei einer neuen Halle z. B. nur das Hallen-Paket, nicht mehrere GB.

#### 19.4.1 Aufteilung des Spiels in Pakete (Chunks)

Unreal packt standardmäßig alles in eine einzige große Datei – dann wäre jedes Update so groß wie das ganze Spiel.
Deshalb wird beim Packen in **feste Pakete** aufgeteilt (Asset Manager / Primary Asset Labels mit Chunk-IDs, eine
`.pak`/`.utoc`/`.ucas`-Gruppe je Chunk):

| Chunk | Inhalt |
|---|---|
| 0 | Kern: Engine-Grundlagen, Menüs, UI, Daten-Tabellen (Spieler, Turniere, Texte) |
| 1 | Spielerfiguren (Körper, Haare, Gesichter) |
| 2 | Kleidung, Schuhe, Schläger, Ball |
| 3 | Animationen |
| 4 | Ton: Effekte, Publikum |
| 5 | Schiedsrichter- und Stadionsprecher-Stimme |
| 10 | Anlage Hartplatz-Arena |
| 11 | Anlage Sandplatz Court Central |
| 12 | Anlage Rasen Centre Court |
| 13 | Training (draußen) |
| 14–17 | Hallen (Arena Indoor, Finals, Turnierhalle, Trainingshalle) |
| 20 | Gemeinsame Anlagen-Bausteine (Tribünen, Netz, Stühle, Publikum-Figuren) |

Regeln:
- Die Zuordnung steht in `Data/chunks.json` und wird vom Import-Skript automatisch gesetzt. **Chunk-IDs ändern sich nie**;
  neue Inhalte bekommen eine neue ID oder kommen in den passenden bestehenden Chunk.
- Die Programmdatei (`.exe`) ist ein eigener Download-Teil. Code-Änderungen bedeuten also immer, dass die .exe neu
  geladen wird – das ist deutlich kleiner als das ganze Spiel.
- **[PRÜFEN] Gleiche Inhalte = gleiche Datei:** Unreal erzeugt beim Packen nicht immer byte-genau dieselbe Datei, auch
  wenn sich nichts geändert hat. Deshalb merkt sich das Veröffentlichungs-Skript für jeden Chunk eine Prüfsumme über
  **seine Quell-Assets**. Ist die gleich wie in der letzten Version, wird die **alte Paketdatei unverändert
  übernommen** – so lädt niemand ein Paket herunter, an dem sich inhaltlich nichts getan hat.
- **[NEU, optional]** Für große geänderte Dateien (vor allem die .exe) kann später ein Binär-Diff (z. B. HDiffPatch)
  ergänzt werden, sodass nur die geänderten Bytes übertragen werden.

#### 19.4.2 Wo die Updates liegen

- Ein **eigenes, öffentliches GitHub-Repository nur für fertige Spieldateien** (z. B. `matchball-tennis-updates`,
  **ohne Quellcode**), jede Version ist ein **GitHub Release**. Kostenlos, schnell, keine eigene Server-Wartung.
- Darin liegt immer aktuell die Datei **`manifest.json`** (über eine feste Adresse erreichbar, z. B. als Asset des
  neuesten Releases: `…/releases/latest/download/manifest.json`).
- Alternative, falls GitHub nicht gewünscht: beliebiger Webspace oder Cloudflare R2 – der Updater braucht nur
  HTTPS-Downloads. **[Entscheidung, siehe 23]**

#### 19.4.3 Das Manifest

```json
{
  "version": "2.1.0",
  "build": "6e5689499cda",
  "datum": "2026-11-02",
  "speicherstand_version": 4,
  "updater_mindestversion": "1.0.0",
  "neuigkeiten": [
    "Neue Turnierhalle für Challenger-Hallenturniere",
    "Ballkinder rollen jetzt die Bälle zum Aufschläger",
    "Fehler behoben: Challenge-Zähler im Tiebreak"
  ],
  "dateien": [
    { "pfad": "Matchball Tennis.exe", "groesse": 98234112, "sha256": "…", "url": "https://…/v2.1.0/MatchballTennis.exe" },
    { "pfad": "MatchballTennis/Content/Paks/pakchunk14-Windows.pak", "groesse": 412233728, "sha256": "…", "url": "https://…/v2.0.0/pakchunk14-Windows.pak" }
  ],
  "loeschen": [ "MatchballTennis/Content/Paks/pakchunk99-Windows.pak" ],
  "signatur": "…"
}
```

- Jede Datei hat Pfad, Größe, **SHA-256** und **Download-Adresse**. Unveränderte Dateien zeigen einfach weiter auf das
  Release, in dem sie zuletzt geändert wurden – sie werden nicht doppelt hochgeladen.
- `loeschen` listet Dateien, die es nicht mehr gibt.
- **Signatur:** Das Manifest wird mit einem privaten Schlüssel (Ed25519) signiert, der nur auf meinem PC liegt
  (nicht im Repository!). Der öffentliche Schlüssel ist im Spiel und im Updater eingebaut. Ein Manifest ohne gültige
  Signatur wird **verworfen** – so kann niemand über einen gekaperten Download-Ort Schadsoftware unterschieben.

#### 19.4.4 Ablauf im Spiel

1. Beim Start fragt das Spiel im Hintergrund das Manifest ab (Zeitlimit 5 Sekunden; ohne Internet: stillschweigend
   weiter, keine Fehlermeldung).
2. Ist `version` neuer als die installierte, erscheint im Hauptmenü ein Hinweis
   **„Update verfügbar – Version 2.1.0 · ca. 410 MB“** mit **WAS IST NEU** und **JETZT UPDATEN** (mit Controller bedienbar).
3. Das Spiel berechnet vorher, was wirklich fehlt: Es vergleicht das Manifest mit der lokal gespeicherten Liste
   `installiert.json` (Pfad + SHA-256 der installierten Dateien). Angezeigt wird nur die **Größe der tatsächlich zu
   ladenden Dateien**.
4. **JETZT UPDATEN** → das Spiel aktualisiert bei Bedarf zuerst den Updater selbst (der läuft ja gerade nicht),
   startet dann `Matchball Tennis Updater.exe` mit Spielordner und Manifest-Adresse und **beendet sich**.
5. Nach dem Update startet das Spiel neu und zeigt einmal **„Neu in Version 2.1.0“** mit den Neuigkeiten.
- In den **Optionen**: „Automatisch nach Updates suchen“ (an/aus), Knopf „Jetzt nach Updates suchen“, Anzeige der
  installierten Version und des Builds.
- **Später erinnern:** Ein abgelehntes Update wird bis zum nächsten Spielstart nicht erneut angezeigt.
- In einem laufenden Match oder Online-Spiel wird **nie** ein Update-Hinweis eingeblendet.

#### 19.4.5 Der Updater

Ein kleines eigenes Programm **`Matchball Tennis Updater.exe`** (C#/WinForms wie der alte Launcher, gebaut mit dem
in Windows enthaltenen C#-Compiler, oder C++ – Hauptsache klein und ohne Zusatz-Installationen), liegt neben der Spiel-.exe.

1. Wartet, bis das Spiel beendet ist.
2. Lädt das Manifest erneut und **prüft die Signatur**.
3. Lädt nur die Dateien, deren SHA-256 sich von `installiert.json` unterscheidet, in einen Zwischenordner `_update\`
   – mit Fortschrittsbalken (gesamt + aktuelle Datei, MB/s, Restzeit) und **Fortsetzen nach Abbruch** (HTTP-Range).
4. Prüft jede heruntergeladene Datei per SHA-256. Fehler → Datei bis zu 3-mal neu laden, sonst Abbruch ohne Änderung.
5. **Austausch erst, wenn alles vollständig und geprüft ist:** alte Dateien nach `_alt\` verschieben, neue an ihren
   Platz, Dateien aus `loeschen` entfernen, `installiert.json` neu schreiben.
6. Schlägt beim Austausch etwas fehl → **alles aus `_alt\` zurückspielen** (das Spiel bleibt immer startbar).
7. Aufräumen, Spiel starten.
- Fenster mit Logo, „Matchball Tennis wird aktualisiert …“, Abbrechen-Knopf (Abbrechen = nichts wird verändert).
- **Reparieren [NEU]:** Startparameter `--reparieren` prüft alle Dateien gegen das Manifest und lädt beschädigte oder
  fehlende neu. Erreichbar über Optionen → „Spieldateien prüfen“.
- Der Updater schreibt ein Protokoll nach `%LocalAppData%\Matchball Tennis\Logs\updater.log`.
- Windows-Warnung „Unbekannter Herausgeber“ betrifft nur die Erstinstallation; der Updater lädt keine Programme aus
  anderen Quellen als dem Manifest.

#### 19.4.6 Veröffentlichen eines Updates (für mich)

**`Update veröffentlichen.bat`** doppelklicken (ruft `Tools/release.ps1` auf):

1. Fragt nach der **neuen Versionsnummer** (Vorschlag: letzte + 1) und den **Neuigkeiten** (eine Zeile pro Punkt).
2. Baut und packt das Spiel (`package.ps1`), mit fester Chunk-Aufteilung.
3. Vergleicht jeden Chunk über die Quell-Asset-Prüfsummen mit der letzten Version und übernimmt unveränderte Pakete (19.4.1).
4. Berechnet SHA-256 aller Dateien, schreibt `manifest.json`, **signiert** es.
5. Lädt über die GitHub-Kommandozeile (`gh release create`) **nur neue/geänderte Dateien** plus Manifest als neues Release hoch.
6. Erzeugt zusätzlich die Komplett-ZIP für neue Freunde (19.3) und legt sie auf den Desktop.
7. Zeigt eine Zusammenfassung: „Update 2.1.0 veröffentlicht – Download für bestehende Spieler: 412 MB von 6,8 GB“.

- Beim ersten Mal erzeugt `release.ps1` das Schlüsselpaar und erklärt, wo der private Schlüssel sicher aufbewahrt
  werden muss (geht er verloren, kann kein Update mehr veröffentlicht werden → Sicherungskopie!).
- **[NEU, optional] Testkanal:** ein zweites Manifest `manifest-test.json` für Versionen, die erst ein Freund
  ausprobiert; im Spiel per Option „Test-Updates erhalten“ umschaltbar.

#### 19.4.7 Zusammenspiel mit Online

- Beim Beitreten wird weiterhin der Build verglichen (15.3).
- Hat **man selbst** die ältere Version: Statt nur abzulehnen zeigt das Spiel
  **„Dein Freund spielt Version 2.1.0, du hast 2.0.0 – jetzt updaten?“** → direkt zum Updater.
- Hat **der Freund** die ältere Version: Hinweis „Dein Freund muss erst auf Version 2.1.0 updaten“.

---

## 20. Projektstruktur

Wurzel des Repositorys siehe 0.0 (README, STAND, CREDITS, `Vorgaben/`, `Dokumente/`, `Unreal/`).
Das Unreal-Projekt:

```
Unreal/
  MatchballTennis.uproject
  Config/                   Default*.ini (Eingabe, Grafik-Stufen, EOS ohne Zugangsdaten)
  Source/                   C++ (siehe 2.3)
  Content/                  NUR per Skript erzeugt
    Characters/ Clothing/ Equipment/ Venues/ Animations/ Audio/ UI/ Maps/ Data/ FX/
  Data/                     CSV/JSON: Spieler, Turniere, Anlagen, Outfits, Belag-Physik, KI, Texte
  SourceAssets/             Quelldateien: .blend, .fbx, Texturen, Audio, fremde Bewegungsdaten (mit Lizenzen)
  blender/
    scripts/                Blender-Python: figur.py, kleidung.py, schuhe.py, schlaeger.py, anlage_*.py, publikum.py …
    lib/                    gemeinsame Hilfsfunktionen (Export, Maßstab, Namensregeln)
  Tools/
    Python/                 Unreal-Editor-Skripte: import_*.py, build_venue.py, build_materials.py …
    build.ps1  package.ps1  assets.ps1
    elevenlabs-stimme.ps1
    release.ps1             Update veröffentlichen (19.4.6)
  Updater/                  Quellcode + build.ps1 für „Matchball Tennis Updater.exe“ (19.4.5)
  Data/chunks.json          feste Zuordnung Inhalte → Pakete (19.4.1)
  ElevenLabs-Stimme erzeugen.bat
  Spiel für Freunde packen.bat
  Update veröffentlichen.bat
```

Git mit LFS und `.gitignore` wie in 0.0 beschrieben.

Nur lokal, **nicht im Repository**: `C:\MatchballTennis-Build\` (gepackte Spiele, Release-Dateien),
Derived Data Cache unter `%LocalAppData%\UnrealEngine\`, `Unreal/Binaries`, `Intermediate`, `Saved`.

---

## 21. Meilensteine

Jeder Meilenstein endet mit einem lauffähigen Stand, Tests grün und aktualisiertem `STAND.md`.

| # | Inhalt | Abnahme |
|---|---|---|
| **M0** | Projekt anlegen, Pipeline-Probe: Blender-Skript erzeugt einen Würfel-Spieler + Schläger → FBX → Python-Import → gepackte .exe startet | `assets.ps1` und `package.ps1` laufen ohne Handarbeit durch |
| **M1** | Platz mit exakten Maßen (Graubox), Ballphysik, Vorhersage, Zählweise, TV-Kamera, Ballmaschine | Ball fliegt/springt plausibel (Tests gegen reale Richtwerte: Flugzeit, Absprunghöhe je Belag, Aufschlagtempo), Zähl-Tests grün |
| **M2** | Spielerfigur v1 (ein Grundkörper, Skelett, Gewichte), Motion Matching fürs Laufen, Grundschläge mit Warping, Timing-Meter, einfache KI | Komplettes Match gegen die KI spielbar, Schläger trifft sichtbar den Ball |
| **M3** | Aufschlag mit Balltippen, Volley, Halbvolley, Schmetterball (mit Sprung), Rutschen, Stopp/Lob, Linienrichter, Challenge, Hawk-Eye | Alle Regeln aus 5 und 7 umgesetzt und getestet |
| **M4** | Anlagen: Arena, Court Central, Centre Court, Training (Bausteine + Aufbau-Skript), Abnutzung, Laufzeit-Spuren, Publikum, Ton, Schiedsrichter (+ ElevenLabs) | Alle 4 Anlagen spielbar, 60 FPS auf „Mittel“ (Zielhardware) |
| **M5** | Alle Grundkörper, Kader-Aussehen, Haare, Gesichter, Kleidung (Drucke + Schnitte + Stoff), Schuhe, Zwischen-Punkt-Verhalten | Alle Outfits/Schnitte auf allen Figuren ohne Durchstechen |
| **M6** | Menüs (CommonUI), Controller-Bedienung, Bildschirmtastatur, Optionen, Grafik-Einstellungen, Speichern, Hauptmenü-Vorschau, Match-Statistik | Alles ohne Maus bedienbar |
| **M7** | Karriere: Erstellung, Kalender, Zulassung, Qualifikation, Auslosung, Simulation, Rangliste, Preisgeld, Level, Fähigkeiten, Übersicht, Finals | Eine Saison komplett durchspielbar/simulierbar, Ranglisten-Tests grün |
| **M8** | Hallen (12.5), Platzgrößen je Turnierstufe (12.6), Tageszeit/Wetter, Ballkinder, Einlauf/Siegerehrung | Jedes Turnier hat sein eigenes Aussehen |
| **M9** | Online (EOS-Lobby, Code, P2P/Relay, Synchronisierung, Versionsprüfung, Revanche), Spiel packen | Zwei PCs in verschiedenen Netzen spielen ein ganzes Match ohne Abweichung |
| **M10** | Update-System: Chunk-Aufteilung, Manifest mit Signatur, Update-Hinweis im Spiel, Updater (inkl. Fortsetzen, Rückfall, Reparieren), `Update veröffentlichen.bat`, Speicherstand-Umwandlung | Testlauf: Version 1 installieren, Version 2 mit nur einer geänderten Anlage veröffentlichen → nur dieses Paket wird geladen, Spielstand bleibt erhalten; abgebrochenes Update lässt das Spiel startbar |
| **M11** | Feinschliff: Leistung, Vorladen, Credits-Bildschirm, Fehlerliste, Optik der Menüs, Abnahme-Test aller Modi | Release-Build für Freunde |

---

## 22. Lizenzen und Quellen

### 22.1 Selbst gemacht

**Alles** – Figuren (eigener Generator), Kleidung, Schuhe, Ball, Anlagen, Hallen, Publikum, Pokale, Logos,
erfundene Sponsoren, Texturen, Animationen, Klänge (eigene Aufnahmen und Synthese), Schiedsrichter-Stimme
(ElevenLabs bzw. Windows-Sprachausgabe). **Einzige übernommene Datei:** das eigene Schlägermodell.

### 22.2 Fremde Quellen – nur als Ausweichlösung nach Rückfrage

Standardmäßig wird keine dieser Quellen verwendet. Wird eine nach meiner Zustimmung doch genutzt, kommt sie in
`CREDITS.md` und in den Credits-Bildschirm:

| Quelle | Lizenz | Pflicht bei Weitergabe |
|---|---|---|
| Tennis-MoCap (Pulgarin-Giraldo et al. 2017) | CC BY-SA 3.0 | Nennen und diese Daten unter derselben Lizenz weitergeben |
| Microsoft Rocketbox Animation Library | MIT | Lizenztext beilegen |
| Bandai Namco Research Motiondataset 1 | CC BY-NC 4.0 | Nennen; **nur nicht-kommerziell** |
| CMU Graphics Lab Motion Capture Database | frei nutzbar | Nennung empfohlen |
| Epic Game Animation Sample | Unreal-Engine-Lizenz | nur in Unreal-Projekten |
| MPFB / MakeHuman (Grundkörper, Skelett-Gewichte) | CC0 | – |
| ambientCG, Poly Haven (Texturen) | CC0 | – |
| BigSoundBank (Klänge) | CC0 | – |
| Mixamo (falls genutzt) | Adobe-Nutzungsbedingungen | Dateien nicht einzeln weitergeben |
| ElevenLabs (Stimme) | nach Tarif der Nutzungsbedingungen | bei kostenloser Stufe Nennung prüfen **[PRÜFEN]** |

Kleidung, Figuren und alles andere sind neu modelliert; es werden keine Community-Kleidungsstücke verwendet.

### 22.3 Private Nutzung

Echte Turniernamen im Karrieremodus und das Schlägerlogo des eigenen Schlägermodells sind für ein **privates Projekt**
in Ordnung. **Vor einer Veröffentlichung ersetzen** (erfundene Turniernamen, eigenes Logo); da alles andere selbst
erstellt ist, sind das die einzigen beiden Punkte. Falls als Ausweichlösung Daten mit CC BY-NC (Bandai Namco) oder
CC BY-SA (Tennis-MoCap) genutzt werden, ändern sich diese Bedingungen – dann vor einer Veröffentlichung prüfen.

---

## 23. Offene Entscheidungen

Diese Punkte klärt Claude Code mit mir, sobald der jeweilige Meilenstein ansteht:

1. **Zielhardware:** Auf welchem PC wird entwickelt und gespielt (Grafikkarte)? Unreal-Editor auf reiner
   Onboard-Grafik ist sehr zäh – ggf. Entwicklung auf stärkerem PC, Test auf dem Laptop.
2. **Welche [NEU]-Punkte** sollen rein? (Vorschlag: Hallen, Platzgrößen je Stufe, Ballkinder, Statistik, Ausdauer,
   Wildcards, Trophäen zuerst; Wetter, Verletzungen, lokales 2-Spieler-Match zuletzt.)
3. **Mixamo** nutzen oder ganz darauf verzichten?
4. **Epic Developer Portal** – Produkt für EOS anlegen (dafür brauche ich eine Schritt-für-Schritt-Anleitung).
5. **Schlägermodell:** Datei nach `Vorgaben/Schlaeger/` committen (Format? .blend/.fbx/.glb) – Platzhalter bis dahin.
6. **Kader:** Vorschlag von Claude Code (`Dokumente/Kader.md`) abnehmen, Erweiterung auf 10 + 10?
7. **Karriere-Länge:** wie viele Saisons, Karriereende (Alter/Rücktritt) ja/nein?
8. **Doppel** – bewusst nicht enthalten. Später vielleicht?
9. **Update-Speicherort:** öffentliches GitHub-Repository nur mit Spieldateien (Vorschlag) oder eigener Webspace /
   Cloudflare R2? Dafür brauche ich ggf. ein GitHub-Konto und die GitHub-Kommandozeile (`gh`).
10. **Updater-Grundgerüst früh einbauen?** Vorschlag: Chunk-Aufteilung schon ab M0 festlegen, damit die erste
    Version, die Freunde bekommen, bereits update-fähig ist.
11. **GitHub-Repository:** Link eintragen (0.0), sobald es angelegt ist. Reicht das LFS-Kontingent, oder bleibt
    `Unreal/Content/` außerhalb des Repositorys und wird auf jedem PC aus den Skripten erzeugt?

---

## 24. Anhang: Maße und Konstanten

### 24.1 Platz (offizielle Maße)

| Maß | Wert |
|---|---|
| Länge (Grundlinie zu Grundlinie) | 23,77 m |
| Breite Einzel | 8,23 m |
| Breite Doppel | 10,97 m |
| Aufschlaglinie vom Netz | 6,40 m |
| Netzhöhe Mitte | 0,914 m |
| Netzhöhe an den Pfosten | 1,07 m |
| Pfosten außerhalb der Doppellinie | 0,914 m |
| Linienbreite | 2,5–5 cm, Grundlinie bis 10 cm |
| Mittelmarke | 10 cm lang, 5 cm breit |
| Auslauf hinter der Grundlinie (Turnier) | mind. 6,40 m (Stadion meist mehr) |
| Auslauf seitlich (Turnier) | mind. 3,66 m |

### 24.2 Ball

| Maß | Wert |
|---|---|
| Durchmesser | 6,54–6,86 cm |
| Masse | 56,0–59,4 g |

### 24.3 Werte, die Claude Code neu herleiten muss

Es gibt kein Altprojekt, aus dem man Zahlen abschreiben kann. Vorgehen für jeden der folgenden Bereiche: (1) reale
Richtwerte recherchieren, (2) Startwert in `Data/` setzen, (3) im Spiel ausprobieren und nachstellen, (4) Herleitung
und Änderungen in `Dokumente/Werte.md` festhalten, (5) wichtige Werte mit Tests absichern.

| Bereich | Worauf es ankommt | Richtwerte / Quellen **[PRÜFEN]** |
|---|---|---|
| Ballphysik | Luftwiderstand, Magnus-Kraft, Drall-Abnahme, Stoßzahl und Reibung je Belag | reale Tennisball-Aerodynamik (Veröffentlichungen, ITF-Ballspezifikation), ITF Court Pace Rating je Belag |
| Aufschlag- und Schlagtempo | Bandbreite von Hobby- bis Weltklasse-Niveau | Aufschläge ca. 150–230 km/h, Grundschläge ca. 80–150 km/h |
| Timing-Fenster, Streuung | Fairness, Spielgefühl | durch Probespielen; je Spielerwert, Power, Präzision |
| Linienrichter-Kurve | Ø 10 % Fehler bei Bällen bis 15 cm | Simulation (7.1) |
| Kontaktfläche (Hawk-Eye) | Länge/Breite des Abdrucks aus Tempo und Winkel | Ballverformung, Reibung auf dem Belag |
| KI | Positionierung, Fehlerquote, Challenge-Verhalten je Schwierigkeit | durch Probespielen, Bots-gegen-Bots-Simulation |
| Spielerwerte, Kader | Verteilung 50–99, Spielstile | `Dokumente/Kader.md` zur Abnahme |
| Turnierkalender, Punkte, Preisgelder | echte Tour als Vorbild | aktuelle ATP/WTA-Tabellen (Recherche) |
| Level-Kurve, Fähigkeitspunkte | Tempo der Karriere (eine Saison ≈ ? Stunden) | durch Probespielen |
| Schiedsrichter-Ansagen | vollständiger Textbaukasten (Englisch) | offizielle Tennis-Ansagen als Vorbild |
| Outfit-Muster und -Farben | Liste aus Abschnitt 10.1 | eigener Entwurf |

**Bots-gegen-Bots-Test:** Sobald KI und Physik laufen, spielen zwei KI-Spieler tausende Matches headless gegeneinander.
Daraus werden Aufschlagquoten, Assanteile, Ballwechsellängen und Break-Raten mit realen Tennisstatistiken verglichen
und die Werte nachgestellt – das ersetzt den Abgleich mit einem Altprojekt.
