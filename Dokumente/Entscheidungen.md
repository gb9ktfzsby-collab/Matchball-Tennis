# Entscheidungen und offene Fragen

Teil A sind die **offenen Entscheidungen aus README 23** mit meinem Vorschlag. Teil B sind
**Auslegungen**, die ich treffen musste, weil die README an einer Stelle nicht eindeutig ist – bitte
jeweils bestätigen oder korrigieren. Nichts davon ändert eine feste Regel der README; wo eine Regel
betroffen sein könnte, steht es ausdrücklich dabei.

Status: ✅ entschieden/umgesetzt · ❓ wartet auf dich · ⏳ kommt mit dem genannten Meilenstein

---

## Teil A – Offene Entscheidungen (README 23)

### A1. Zielhardware ❓
Auf welchem PC wird entwickelt und gespielt (Grafikkarte, Prozessor, RAM)? Unreal-Editor auf reiner
Onboard-Grafik ist sehr zäh. **Vorschlag:** Entwicklung auf dem stärksten PC, Abnahme „60 FPS auf
Mittel" (M4) auf dem Laptop. `Unreal/Tools/pruefe_installation.ps1` liest Grafikkarte und Treiber aus –
bitte einmal auf jedem PC ausführen und die Ausgabe schicken.

### A2. Welche [NEU]-Punkte sollen rein? ❓
Vorschlag wie README: zuerst Hallen, Platzgrößen je Stufe, Ballkinder, Statistik, Ausdauer, Wildcards,
Trophäen; zuletzt Wetter, Verletzungen, lokales 2-Spieler-Match.
**Bereits im Kern vorhanden (abschaltbar, kostet nichts):** No-Ad, Match-Tiebreak, Entscheidungssatz-
Varianten, neue Bälle nach 7/9 Spielen (als Hinweis), Challenge-Fenster ca. 4 s, Belag „Halle",
Court-Pace-Feineinstellung je Turnier, Wind-Parameter in der Physik (Standard aus), Aufgabe online.

### A3. Mixamo ❓
**Vorschlag: verzichten.** Alle Bewegungen werden selbst animiert (README 4.1); Mixamo bräuchte ein
Adobe-Konto, und die Dateien dürfen nicht einzeln weitergegeben werden. Falls eine Bewegung trotz
mehrerer Versuche nicht gelingt, frage ich vorher (README 4.1a).

### A4. Epic Developer Portal (EOS) ⏳ M9
Eine Schritt-für-Schritt-Anleitung steht in `Dokumente/EOS-Anleitung.md`. Die Zugangsdaten kommen in
`Unreal/Config/Windows/WindowsEngine.ini` (Vorlage `WindowsEngine.ini.vorlage`), die per `.gitignore`
nie committet wird.

### A5. Schlägermodell ❓
Im Repository liegt `Vorgaben/racket.js` – das ist dein Schläger aus dem Browserspiel als eingebettete
GLB-Datei mit auf **1024 × 1024 verkleinerten Texturen**. Die README verlangt die **Originaldatei in
voller Qualität** in `Vorgaben/Schlaeger/`. Bitte die Originaldatei (bevorzugt .blend, sonst .fbx/.glb)
mit allen Texturen dort ablegen. Bis dahin nutzt `blender/scripts/schlaeger.py` die racket.js (und
zusätzlich gibt es den Platzhalter-Schläger). Die leere Datei `Vorgaben/test` kann gelöscht werden.

### A6. Kader ❓
Vorschlag mit 10 Herren und 10 Damen in `Dokumente/Kader.md` und `Unreal/Data/spieler.csv` – bitte
abnehmen (Namen, Nationen, Werte, Spielstile, Outfits).

### A7. Karriere-Länge ❓
**Vorschlag:** offenes Ende (beliebig viele Saisons), Karriereende freiwillig über das Menü; optional
**[NEU]** Rücktritt ab 34 Jahren angeboten, wenn die Werte durch das Alter sinken. Eine Saison soll mit
verkürzten Sätzen in ca. 8–12 Stunden spielbar sein (Level-Kurve wird danach abgestimmt).

### A8. Doppel ✅
Nicht enthalten (README). Der Platz hat trotzdem schon die Doppellinien; die Zählweise kennt No-Ad und
Match-Tiebreak – ein späteres Doppel wäre also nicht verbaut.

### A9. Update-Speicherort ❓ (⏳ M10)
**Vorschlag:** eigenes öffentliches GitHub-Repository nur mit Spieldateien
(`gb9ktfzsby-collab/matchball-tennis-updates`), Releases mit `manifest.json`. Kostenlos, keine Wartung.

### A10. Updater-Grundgerüst früh ✅
Umgesetzt: Chunk-Aufteilung ab M0 – `bGenerateChunks=True` in `DefaultGame.ini`, feste Zuordnung in
`Unreal/Data/chunks.json`, `Tools/Python/setze_chunks.py` legt die PrimaryAssetLabels an;
`package.ps1` schreibt Build-Id und `installiert.json`. Damit ist schon die erste Version für Freunde
update-fähig.

### A11. GitHub-Repository und LFS ✅ (bitte bestätigen)
Link eingetragen: https://github.com/gb9ktfzsby-collab/Matchball-Tennis

**LFS-Abschätzung (README 0.0 [PRÜFEN]):** Das kostenlose Konto hat laut aktueller GitHub-Dokumentation
**10 GiB Speicher und 10 GiB Bandbreite pro Monat** (ältere Seiten nennen noch 1 GiB). Ein fertiges
`Unreal/Content/` dieses Spiels schätze ich auf 5–8 GB (Figuren mit 2K/4K-Texturen, 8 Anlagen,
Animationen, Ton). Weil LFS **jede Version** einer geänderten Binärdatei aufbewahrt, wäre das Kontingent
nach ein, zwei kompletten Neuerzeugungen voll – und jeder `git pull` auf dem zweiten PC kostet Bandbreite.

**Deshalb (README erlaubt das ausdrücklich):** `Unreal/Content/` und die per Skript erzeugten
`.blend`/`.fbx` (`Unreal/SourceAssets/_erzeugt/`) stehen in `.gitignore` und werden auf jedem PC mit
`Tools\assets.ps1` neu erzeugt. Versioniert (über LFS) werden nur echte Quellen: `Vorgaben/`, deine
Tonaufnahmen und die Vorschaubilder – voraussichtlich deutlich unter 1 GB. Wer `Content/` doch im
Repository haben will: die zwei Zeilen in `.gitignore` löschen.

---

## Teil B – Auslegungen (bitte bestätigen)

### B1. Challenge: Aufschlag fälschlich „gut" gegeben (war aus) ❓
README 7.2 sagt allgemein „Ball fälschlich gut gegeben (war aus): Punkt für den Challenger". Beim
**ersten** Aufschlag wäre das im echten Tennis aber kein Punkt, sondern ein **Fehler** (es folgt der
zweite Aufschlag). Umgesetzt ist die Tennisregel: 1. Aufschlag → Fehler, 2. Aufschlag → Doppelfehler =
Punkt für den Challenger. Für Bälle im Ballwechsel gilt die README-Regel unverändert.

### B2. Challenge-Fenster ✅
Das Fenster (ca. 4 s, README 7.2 [NEU]) öffnet sich nur bei **knappen** Entscheidungen (Abstand zur
Linie bis 30 cm). Weiter weg ist der Linienrichter laut README 7.1 immer richtig, eine Challenge wäre
sinnlos und würde nur den Spielfluss bremsen. Während „Punktende" kann trotzdem jederzeit mit H/Y
challengt werden.

### B3. Abstand d beim Linienrichter ✅
d = Abstand der Ball-Kontaktfläche zur Entscheidungsgrenze (Außenkante der Linie), auf beiden Seiten.
Ein Ball, der die Linie gerade noch streift, gilt als „sehr knapp"; ein Ball mitten auf der Linie nicht.

### B4. Let ✅
Aufschlag berührt das Netz und landet im richtigen Feld → „Let", derselbe Aufschlag (1. oder 2.) wird
wiederholt; nicht challengebar.

### B5. 1 Spiel pro Satz ✅
Bei „Spiele pro Satz = 1" entscheidet das eine Spiel den Satz (kein Tiebreak). Für 2–6 gilt:
N Spiele mit 2 Vorsprung, Tiebreak bei N:N (wie bei kurzen Sätzen der ITF).

### B6. Neue Bälle ✅ [NEU]
Nach 7, dann alle 9 Spiele; wäre der Wechsel vor einem Tiebreak fällig, erfolgt er im zweiten Spiel des
nächsten Satzes (ITF-Regel). Wirkt nur auf Optik und Ansage („New balls, please").

### B7. Linien in Ansagen ❓
„… is challenging the call on the **left baseline**": „links/rechts" aus Sicht des Schiedsrichterstuhls
(Stuhl an der Netzseite rechts aus Sicht der nahen Grundlinie). Seitenlinien heißen „near/far sideline".
Bitte prüfen, ob dir das so gefällt (Dokumente/Ansagen.md).

### B8. Geschlecht von Holloway und Moreau ❓
Die README nennt keine Vornamen. Vorschlag: **Rhys Holloway** (Herren) und **Camille Moreau** (Damen).
Nationen von Adler (AUT) und Falk (GER) ebenfalls Vorschlag.

### B9. Pause ✅
Offline pausiert Esc/P das Spiel; online läuft das Match weiter (README 15.2) – das wird mit M9 umgesetzt.

### B10. Timing in M1 ✅
Im M1-Training schlägt der Spieler beim **Drücken**, mit perfektem Timing (nur Grundstreuung). Das
Halten/Loslassen mit Timing-Meter kommt mit der Spielfigur in M2.

### B11. DirectX 11 ⏳ M6
DX12 ist Standard; DX11 (SM5) ist als Rückfall für ältere Onboard-Grafik mit eingetragen
(README 19.1 [PRÜFEN]). Forenberichte zu UE 5.5/5.7 melden, dass Lumen auf DX11 unzuverlässig ist –
auf DX11 wird deshalb mit gebackenem Licht gearbeitet (passt zu den Stufen Niedrig/Mittel, README 18.3).

### B12. Windows-Sprachausgabe als Rückfall ⏳ M4
Unreal hat ein Plugin „Text To Speech" (liegt unter Experimental, Klasse `UTextToSpeechEngineSubsystem`,
Kanäle müssen aktiviert werden). Ob es unter Windows SAPI nutzt, ist nicht dokumentiert – wird in M4
lokal getestet; sonst bleibt der Rückfall „kürzere Ansage ohne Namen".

### B13. ElevenLabs ✅
Kostenloser Tarif: nur nicht-kommerziell, Nennung „elevenlabs.io" erforderlich (steht in CREDITS.md und
kommt in den Credits-Bildschirm). Für ein privates Projekt passt das; vor einer Veröffentlichung müsste
ein bezahlter Tarif genutzt werden.

### B14. Speicherort der Spielstände ⏳ M6
README 19.2 verlangt `%LocalAppData%\Matchball Tennis\Saved\` (mit Leerzeichen). Unreal nutzt von sich
aus `%LocalAppData%\MatchballTennis\Saved\`. Umsetzung in M6: eigenes Speichern über
`SaveGameToMemory` + Datei im gewünschten Ordner.

### B15. Ausführbare Datei ✅
Unreal erzeugt `MatchballTennis.exe` (Startprogramm) und `MatchballTennis\Binaries\Win64\MatchballTennis-Win64-Shipping.exe`.
`package.ps1` benennt das Startprogramm in **„Matchball Tennis.exe"** um (das Startprogramm findet die
eigentliche .exe über einen eingebetteten Pfad, der Name ist frei) **[PRÜFEN]** beim ersten Packen.
