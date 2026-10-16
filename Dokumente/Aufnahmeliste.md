# Aufnahmeliste – eigene Tonaufnahmen (README 16.1)

Diese Liste sagt dir genau, **was** du **wo**, **wie oft** und **wie** mit dem Handy aufnehmen sollst. Alle
Geräusche des Spiels werden neu und selbst erstellt (README 16.1, 22.1) – keine Klangbibliotheken, keine
Mitschnitte aus Fernsehen, YouTube oder echten Turnieren.

* **Stand:** Vorschlag zur Abnahme (16.10.2026). Das Bereinigungs- und Importskript für Aufnahmen
  (Schnitt, Rauschminderung, Lautstärke, Varianten – README 16.1) entsteht erst mit **M4**. Bis dahin
  werden die Aufnahmen nur nach den Regeln unten abgelegt und versioniert.
* **Ablage:** `Unreal/SourceAssets/Audio/Aufnahmen/<kategorie>/` – diese Dateien sind **Quellen** und werden
  über Git LFS versioniert (`*.wav` steht in `.gitattributes`).
* **Format:** WAV (PCM), 48 kHz, 24 Bit – bzw. was dein Handy liefert, umgewandelt nach WAV (Abschnitt 8).
* **Namen:** `<kategorie>_<variante>_<ort>_<NN>.wav`, z. B. `schlag_sauber_hart_01.wav` (Abschnitt 7).
* **Aufwand:** ca. **9 Stunden reine Aufnahmezeit** (davon ca. 7 h Pflicht), verteilt auf **6 Termine**,
  mit Aufbau und Wegen ca. **11–12 Stunden** (Abschnitt 12).

Markierungen: **[OPTIONAL]** = schön, aber nicht nötig · **[PRÜFEN]** = bitte vor Ort bzw. an deinem Gerät prüfen.

---

## Inhalt

1. [Was aufgenommen wird – und was nicht](#1-was-aufgenommen-wird--und-was-nicht)
2. [Ausrüstung und Handy-Einstellungen](#2-ausrüstung-und-handy-einstellungen)
3. [Handy richtig aufstellen: Abstand und Ausrichtung](#3-handy-richtig-aufstellen-abstand-und-ausrichtung)
4. [Umgebung: Wind, Ruhe, Uhrzeit](#4-umgebung-wind-ruhe-uhrzeit)
5. [Ablauf einer Aufnahme-Serie](#5-ablauf-einer-aufnahme-serie)
6. [Pegel und Clipping](#6-pegel-und-clipping)
7. [Dateinamen und Ablage](#7-dateinamen-und-ablage)
8. [Format und Umwandlung](#8-format-und-umwandlung)
9. [Die Aufnahmeliste im Detail](#9-die-aufnahmeliste-im-detail)
10. [Drehplan: sechs Termine](#10-drehplan-sechs-termine)
11. [Checkliste zum Abhaken](#11-checkliste-zum-abhaken)
12. [Geschätzte Gesamtdauer und Speicherbedarf](#12-geschätzte-gesamtdauer-und-speicherbedarf)
13. [Nach der Aufnahme: aufs Repository](#13-nach-der-aufnahme-aufs-repository)
14. [Was später daraus wird (M4)](#14-was-später-daraus-wird-m4)

---

## 1. Was aufgenommen wird – und was nicht

**Aufgenommen werden** alle Geräusche, die nah am Platz entstehen und die man mit wenigen Leuten selbst
erzeugen kann: Ballschläge, Aufsprünge, Netz, Schritte, Rutschen, Quietschen, Schläger, Ballkorb, ein kleines
Publikum im Verein (Applaus, Raunen, Lachen), optional Spieler-Laute, und ruhige Umgebungsgeräusche (Atmo).
Grundsatz: **nah, trocken, einzeln** – jedes Ereignis für sich, mit Stille davor und danach. Mischen,
Verteilen im Raum, Hall und Lautstärke macht später das Spiel.

**Nicht aufgenommen werden** (das entsteht per Synthese bzw. Schichtung – README 16.1, oder per ElevenLabs – README 16.2):

| Was | Warum nicht | Woher es stattdessen kommt |
|---|---|---|
| Großes Stadionpublikum (tausende Zuschauer), Grundrauschen im Stadion | lässt sich nicht selbst aufnehmen; Mitschnitte von Turnieren sind fremde Werke | aus vielen eigenen Aufnahmen weniger Personen **geschichtet** (Positionen, Tonhöhe, Zeitversatz) und per MetaSounds bzw. Python-Audiosynthese ergänzt |
| „Ohhh“/Raunen von tausenden | wie oben | Schichtung der Vereins-Aufnahmen (Kategorie `publikum`) |
| Hall der Anlagen (Arena, Centre Court, Hallen) | jede Anlage braucht ihren eigenen Raum | Audio-Volumes/Submix-Effekte (README 16.1); deine Klatsch-Impulse (9.15, optional) helfen nur beim Abstimmen |
| Schiedsrichter-Stimme, Stadionsprecher | – | ElevenLabs bzw. Windows-Sprachausgabe (README 16.2, `Dokumente/Ansagen.md`) |
| Musik, Menü-/UI-Klänge, Hawk-Eye-Effekte | kein Platzgeräusch | Synthese (M4/M6) |
| Echte Profis, TV-Kommentar, Werbe-Jingles, Marken | fremde Rechte (README 0.3) | – |

**Personen:** Wer zu hören ist (Publikum, Spieler-Laute, Zuspieler), muss vorher einverstanden sein. Keine
Namen rufen, keine privaten Gespräche aufnehmen; Kinder nur mit Erlaubnis der Eltern. Für Aufnahmen auf der
Vereinsanlage bzw. in der Halle vorher den Verein fragen (Platz bzw. Hallenstunde reservieren).

---

## 2. Ausrüstung und Handy-Einstellungen

**Mitnehmen:**

- Handy (voller Akku, mindestens 3 GB frei), Powerbank, Kopfhörer zum Probehören
- kleines Handy-Stativ oder Klemmhalterung (für Zaun, Netzpfosten, Bank) – **das Handy nie in der Hand halten**
  (Griffgeräusche); notfalls auf eine zusammengelegte Tasche oder ein Handtuch legen
- Windschutz: ein Stück Schaumstoff oder eine dünne Socke über die Mikrofon-Öffnung (Unterkante)
- Handtuch als Unterlage für Aufnahmen in Bodennähe (dämpft Körperschall)
- eigener Schläger, wenn möglich ein zweiter (anderes Modell/andere Saitenspannung = mehr Abwechslung)
- **neue Bälle** (2 Dosen, eine davon ungeöffnet für „Dose öffnen“) und einige **gebrauchte** Bälle
- Tennisschuhe: Allcourt/Hartplatzsohle und – falls vorhanden – Sandplatzsohle (Fischgrät)
- ein Helfer, der Bälle zuspielt (oder die Ballmaschine des Vereins), für das Publikum 5–15 Personen
- Zettel oder Notiz-App für das Aufnahmeprotokoll (Abschnitt 7)

**App:** Nimm eine Aufnahme-App, die **WAV (PCM)** speichert und bei der man **automatische Pegelanpassung
(AGC), Rauschunterdrückung, „Sprachverbesserung“ bzw. „Audio-Zoom“ abschalten** kann – solche
Bearbeitungen verfälschen kurze, laute Geräusche (Ballschlag) stark. **[PRÜFEN]** an deinem Handy:

- iPhone: Die eingebaute App „Sprachmemos“ kann unter *Einstellungen → Apps → Sprachmemos → Audioqualität*
  auf **„Verlustfrei“** gestellt werden (Datei `.m4a` mit ALAC, muss danach umgewandelt werden, Abschnitt 8).
  Apps mit direkter WAV-Aufnahme und Pegelregler sind besser.
- Android: Apps mit Einstellung „WAV/PCM“, „Abtastrate 48 kHz“ und einer Option für die **unbearbeitete
  Mikrofonquelle** („Unprocessed“/„Raw“, „Voice Recognition“ statt „Camcorder“) sind ideal.

**Einstellungen:**

| Einstellung | Wert |
|---|---|
| Dateiformat | WAV (PCM) – sonst die verlustfreie Stufe (ALAC/FLAC), nie MP3/AAC mit niedriger Qualität |
| Abtastrate | 48 kHz (44,1 kHz geht auch, wird umgewandelt) |
| Bit-Tiefe | 24 Bit (oder 32 Bit Float); liefert die App nur 16 Bit, ist das in Ordnung |
| Kanäle | **Mono** für Einzelgeräusche; **Stereo** für Umgebung (Atmo) und Publikum, falls das Handy zwei Mikrofone nutzt (dann quer halten) |
| Automatik | AGC, Rauschunterdrückung, Limiter, „Wind-Filter“, „Sprachverbesserung“ **aus**; Pegel manuell, falls möglich |
| Handy | **Flugmodus** (kein Funkbrummen, keine Anrufe), Benachrichtigungen und Tastentöne aus, Bildschirm-Sperre aus bzw. lang |

Speicherbedarf (WAV, 48 kHz): Mono 24 Bit ≈ **8,6 MB pro Minute**, Mono 16 Bit ≈ 5,8 MB/min, Stereo das Doppelte.

---

## 3. Handy richtig aufstellen: Abstand und Ausrichtung

**Wo ist das Mikrofon?** Bei den meisten Handys an der **Unterkante** (neben dem Ladeanschluss), ein zweites
oft oben bzw. hinten bei der Kamera. Test: Aufnahme starten, nacheinander leicht in jede Öffnung pusten,
anhören – wo es am lautesten ist, sitzt das Hauptmikrofon. Dieses Mikrofon zeigt **zur Schallquelle**,
es wird **nicht abgedeckt** (Hülle, Finger, Stativklemme).

**Zwei Perspektiven:**

- **nah** (Standard) – so hört der eigene Spieler bzw. die TV-Kamera hinter ihm den Klang. Im Dateinamen ohne Zusatz.
- **fern** – so klingt es von der anderen Platzseite (Gegner). Handy ca. **12 m** entfernt, z. B. an der
  gegenüberliegenden Grundlinie oder am Netzpfosten. Im Dateinamen mit `_fern` (Abschnitt 7).

**Abstände und Ausrichtung je Geräusch:**

| Geräusch | Abstand zur Quelle | Höhe des Handys | Position / Ausrichtung |
|---|---|---|---|
| Ballschlag (Grundschlag, Slice, Stopp) | **1,5 m** vom Treffpunkt (fern: 12 m) | Hüfthöhe (ca. 1 m) | seitlich **vor** dem Spieler auf der Schlagseite, schräg nach vorn versetzt – nie in Schlag- oder Flugrichtung; Unterkante zeigt auf den Treffpunkt |
| Power-Schlag, Aufschlag, Schmetterball | **2–3 m** (sehr laut; fern: 12 m) | Hüfthöhe | seitlich hinter dem Spieler (Aufschlag: neben der Grundlinie, außerhalb des Doppelfelds) |
| Volley, Halbvolley | 1,5 m | Hüfthöhe | am Netzpfosten befestigt bzw. neben dem Spieler am Netz |
| Leerschwung (Luft) | 0,5–1 m | Schulterhöhe | seitlich, der Schläger zieht am Handy vorbei (Abstand halten!) |
| Aufsprung | **1–1,5 m** vom Aufsprungpunkt | 1 m | seitlich, Unterkante zum Aufsprungpunkt; [OPTIONAL] zusätzlich Bodenperspektive: Handy auf Handtuch, 1 m entfernt |
| Ball auftippen, fangen | 1,5 m | 1 m | vor dem Spieler, leicht seitlich |
| Ball rollt | 0,5 m neben der Rollbahn | auf Handtuch am Boden | Ball rollt am Handy vorbei |
| Netz (Geflecht, Netzband) | 1 m vom Auftreffpunkt | Netzhöhe (ca. 0,9 m) | am Netzpfosten bzw. auf Stativ neben dem Netz |
| Schritte, Split-Step, Abbremsen | 2 m | 1 m | Spieler läuft an der Längsseite am Handy vorbei bzw. darauf zu |
| Rutschen, Quietschen | 2–3 m | 1 m | seitlich vom Rutsch-/Stopppunkt (**Sicherheitsabstand**) |
| Schläger fällt, Schläger abklopfen | 1,5 m | 1 m | seitlich |
| Zaun, Wand, Ballkorb, Balldose | 1,5–2 m | 1 m | seitlich |
| Publikum (Applaus, Raunen, Lachen) | **3–5 m** zur Gruppe (fern: 10 m) | Kopfhöhe sitzender Personen (ca. 1,2 m) | Gruppe steht bzw. sitzt im Halbkreis **vor** dem Handy; Stereo: quer |
| Spieler-Laute | 1 m vor dem Mund, leicht seitlich (ca. 30°) | Mundhöhe | seitlich versetzt gegen Plopp- und Atemgeräusche |
| Umgebung (Atmo) | – | 1,5 m | frei stehend, mindestens 3 m von Wänden; Stereo: quer |

**Schutz:** Bälle treffen gern das Handy. Bei Schlägen das Handy hinter den Netzpfosten, eine Tasche oder
die Bank stellen bzw. vom Helfer 2 m seitlich (außerhalb der Flugbahn) auf dem Stativ halten lassen.

---

## 4. Umgebung: Wind, Ruhe, Uhrzeit

- **Wind:** nur bei **fast Windstille** (unter ca. 10 km/h – Blätter bewegen sich kaum). Windgeräusche am
  Mikrofon (tiefes Wummern, Knacken) lassen sich später **nicht** entfernen. Windschutz immer drauf.
  Ausnahme: die Atmo „Wind“ (9.15) – dann das Handy im Windschatten aufstellen und nur den Wind in Bäumen
  und Hecken aufnehmen.
- **Ruhe:** Aufnahmen auf dem Platz, wenn die Nachbarplätze **frei** sind – meist früh morgens (7–9 Uhr)
  oder unter der Woche mittags. Bei Rasenmäher, Laubbläser, Flugzeug, Martinshorn, lauter Straße, Musik:
  Serie abbrechen und wiederholen. Ausnahme: die Atmo „Nachbarplätze“ (9.15) braucht gerade Betrieb.
- **Uhrzeit:** Vögel für die Atmo **früh morgens** (Frühling/Sommer, ca. 6–8 Uhr); Abend-Atmo (Grillen,
  Flutlicht) nach Sonnenuntergang im Sommer.
- **Wetter und Boden:** trocken – nasser Sand und ein nasser Hartplatz klingen anders. Ab ca. 12 °C
  (kalte Bälle klingen dumpfer). Regen nur für die Atmo „Regen“ (9.15).
- **Halle:** gemietete Hallenstunde, Türen zu. Lüftung bzw. Gebläse wenn möglich aus – **außer** für die
  Atmo der Halle (9.15), die genau dieses Grundgeräusch einfangen soll.
- **Raumklang:** Draußen mindestens 3 m Abstand zu Wänden, Zäunen mit Planen und dem Clubhaus halten, damit
  die Aufnahmen trocken sind (den Hall setzt das Spiel je Anlage selbst). In der Halle ist der echte
  Hallenhall bei Aufsprung, Schritten und Quietschen **gewollt** (README 12.5: mehr Hall in der Halle) –
  Ballschläge aber bitte draußen aufnehmen.

---

## 5. Ablauf einer Aufnahme-Serie

Eine **Datei = eine Serie** mit mehreren gleichartigen Einzelereignissen (z. B. 15 saubere Vorhand-Schläge).
Das spätere Bereinigungsskript schneidet die Serie an den Pausen in Einzelereignisse. Damit das klappt:

1. Handy aufstellen, Flugmodus prüfen, Aufnahme starten, **3 s still** sein.
2. **Ansage** (Klappe) sprechen: z. B. „Schlag sauber, Hartplatz, Serie eins“.
3. **Referenzklatschen:** einmal kräftig in die Hände klatschen, ca. 1 m vor dem Handy, immer gleich stark.
   Es dient später als Lautstärke-Bezug zwischen den Terminen und zum Abgleich, falls du mit zwei Handys
   gleichzeitig (nah und fern) aufnimmst.
4. Mindestens **3 s Stille** – alles davor verwirft das Skript (Konvention für M4).
5. Die Ereignisse: **vor und nach jedem Ereignis mindestens 2 s Stille** (in Gedanken „einundzwanzig,
   zweiundzwanzig“ zählen), Ausklingen nicht abschneiden.
6. Am Ende 2 s Stille, Aufnahme stoppen. Eine Serie höchstens ca. **3 Minuten** lang.
7. **Raumklang:** an jedem neuen Aufnahmeort einmal **30 s Stille** aufnehmen (nichts tun, nicht reden) →
   `umgebung_stille_<ort>_NN.wav`. Daraus lernt das Skript das Grundrauschen für die Rauschminderung.
8. Nach der **ersten** Serie an jedem Ort mit Kopfhörern **probehören**: Pegel, Clipping (Abschnitt 6),
   Wind, Störgeräusche. Dann weitermachen.
9. Fehlversuche nicht löschen und neu aufnehmen, sondern im Takt bleiben – das Skript bzw. du sortieren
   später aus. Missglückte **ganze** Serien vor dem Commit löschen.

**Abwechslung ist alles:** Im Spiel wird jedes Geräusch zufällig aus mehreren Varianten gewählt. Deshalb
innerhalb einer Serie leicht variieren (Tempo, Treffpunkt, Ball neu/alt, Schläger, Spieler, Position auf
dem Platz) – aber nicht die Kategorie mischen.

---

## 6. Pegel und Clipping

- **Ziel:** die lautesten Spitzen bei etwa **−12 bis −6 dBFS** (in vielen Apps: Pegelanzeige im oberen Drittel,
  nie ganz am Anschlag).
- **Clipping** (Übersteuerung) erkennst du an **abgeflachten Spitzen** in der Wellenform und einem kratzigen,
  verzerrten Klang. Übersteuerte Aufnahmen sind unbrauchbar.
- **Zuerst das Lauteste testen:** Schmetterball, Power-Schlag, Aufschlag, Applaus aus der Nähe.
- Hat die App keinen Pegelregler, ist der **Abstand der Regler**: doppelter Abstand ≈ 6 dB leiser.
- **AGC-Pumpen:** Wird das Grundrauschen nach jedem Schlag hörbar lauter („Atmen“), ist die automatische
  Pegelanpassung noch an – in den App-Einstellungen suchen; geht es nicht aus, im Protokoll vermerken.
- Nicht selbst normalisieren, entrauschen oder schneiden – das macht das Skript reproduzierbar (README 0.2).

---

## 7. Dateinamen und Ablage

**Ordner:** `Unreal/SourceAssets/Audio/Aufnahmen/<kategorie>/` – ein Unterordner je Kategorie (erstes Feld
des Dateinamens).

**Schema:**

```
<kategorie>_<variante>_<ort>_<NN>.wav          nahe Perspektive (Standard)
<kategorie>_<variante>_<ort>_fern_<NN>.wav     ferne Perspektive (ca. 12 m bzw. 10 m)
```

- Nur **Kleinbuchstaben a–z und Ziffern**, Felder mit `_` getrennt, **innerhalb eines Felds kein `_`**
  (also `halbvolley`, `applausleise`), keine Umlaute (ae, oe, ue, ss), keine Leerzeichen.
- `<NN>` ist die **Seriennummer**, zweistellig, je Kombination aus Kategorie, Variante, Ort (und `fern`)
  ab `01` gezählt.
- Beispiele: `schlag_sauber_hart_01.wav`, `schlag_power_hart_fern_02.wav`, `aufsprung_schnell_sand_01.wav`,
  `quietschen_kurz_halle_03.wav`, `publikum_raunen_verein_01.wav`, `umgebung_voegel_draussen_01.wav`.
- Am einfachsten gleich in der App so benennen bzw. die Ansage (Abschnitt 5, Schritt 2) zum Umbenennen nutzen.

**Ort-Kürzel:**

| Kürzel | Bedeutung |
|---|---|
| `hart` | Hartplatz draußen (Acryl/Kunststoff auf Asphalt) |
| `sand` | Sandplatz (Ziegelmehl) |
| `rasen` | echter Rasen-Tennisplatz |
| `garten` | **Ersatz** für Rasen: kurz gemähter, trockener Gartenrasen bzw. Wiese – nur wenn kein Rasenplatz erreichbar ist |
| `halle` | Tennishalle (Hart, Teppich oder Granulat – im Protokoll vermerken) |
| `verein` | Vereinsgelände/Terrasse/Clubhaus (Publikum, Ablegen, sonstiges) |
| `draussen` | allgemeine Außen-Atmo (Umgebung) |
| `m1`, `m2`, `w1`, `w2` … | **nur bei Spieler-Lauten:** statt des Orts die Person (männlich/weiblich, nummeriert) |

**Kategorien (Ordner) und Varianten:** siehe Abschnitt 9 – Kurzüberblick:

| Ordner | Inhalt | Varianten |
|---|---|---|
| `schlag` | Ballschläge | `sauber`, `power`, `rahmen`, `slice`, `stopp`, `volley`, `halbvolley`, `smash`, `aufschlag` |
| `schlaeger` | Schläger ohne Ball | `schwung`, `faellt`, `ablegen`, `abklopfen` |
| `aufsprung` | Aufsprung des Balls | `fallen`, `schnell`, `hoch`, `linie` |
| `auftippen` | Ball vor dem Aufschlag | `tippen`, `fangen`, `schlaeger` |
| `rollen` | Ball rollt | `langsam`, `schnell` |
| `netz` | Ball und Netz | `geflecht`, `band`, `pfosten` |
| `ball` | sonstige Ballgeräusche | `zaun`, `wand`, `korb`, `dose` |
| `schritt` | Schritte | `gehen`, `laufen`, `seit`, `split`, `stopp`, `drehen` |
| `rutschen` | Rutschen | `lang`, `mittel`, `kurz` |
| `quietschen` | Schuhquietschen | `kurz`, `lang`, `doppel` |
| `ballmaschine` | Ballmaschine | `motor`, `auswurf` |
| `platzpflege` | Sandplatz abziehen | `schleppnetz`, `besen`, `wasser` |
| `publikum` | Publikum im Verein | `applausleise`, `applaus`, `applauslaut`, `jubel`, `raunen`, `staunen`, `enttaeuscht`, `lachen`, `murmeln`, `stille`, `einzeln`, `takt` |
| `spieler` | Spieler-Laute | `kurz`, `lang`, `atmen`, `jubel`, `frust`, `erschoepft` |
| `umgebung` | Atmo, Raumklang | `stille`, `voegel`, `wind`, `nachbarplaetze`, `abend`, `regen`, `fahne`, `raum`, `geblaese`, `impuls` |

**Protokoll:** Lege neben die Aufnahmen eine Datei `Unreal/SourceAssets/Audio/Aufnahmen/protokoll.csv` an
(UTF-8, eine Zeile je Datei). Sie ersetzt die `.meta.json` für Quellaufnahmen und ist die Grundlage für den
Eintrag in `CREDITS.md` („selbst aufgenommen“). Vorlage:

```csv
Datei,Datum,Anlage,Belag,Handy,App,Format,Wetter,Temperatur_C,Personen,Bemerkung
schlag_sauber_hart_01.wav,2026-10-20,TC Beispiel Platz 3,Hart,Handy-Modell,App-Name,WAV 48 kHz 24 Bit mono,windstill sonnig,16,Spieler + Zuspieler,neue Baelle; AGC aus
```

**Versionierung:** Ins Repository kommen **nur `.wav`-Dateien** (und `protokoll.csv`). `.m4a`, `.mp3`, `.flac`
und `.aac` stehen zwar zur Sicherheit unter LFS in `.gitattributes`, verbrauchen aber nur doppelt Speicher – vorher
umwandeln (Abschnitt 8) und die Originale außerhalb des Repositorys aufheben (z. B. `C:\Dev\Aufnahmen-Original\`);
`.3gp` steht nicht unter LFS. Die Originale im Repository werden **nie** bearbeitet; geschnittene und
bereinigte Fassungen erzeugt das Skript unter `Unreal/SourceAssets/_erzeugt/Audio/` (nicht versioniert).

---

## 8. Format und Umwandlung

**Ziel:** WAV, PCM, **48 kHz**, **24 Bit** (16 Bit, wenn das Handy nicht mehr liefert – Hochrechnen auf
24 Bit bringt keine Qualität, nur größere Dateien), Mono bzw. Stereo wie in Abschnitt 2.

**Wenn das Handy etwas anderes liefert** (`.m4a`, `.flac`, `.3gp`, 44,1 kHz …), auf dem PC umwandeln:

- **ffmpeg** (kostenlos; Installation z. B. `winget install Gyan.FFmpeg` **[PRÜFEN]** Paketname), dann in
  PowerShell im Ordner der Arbeitskopie:

  ```powershell
  # Wandelt alle Nicht-WAV-Aufnahmen in WAV 48 kHz / 24 Bit um (Original bleibt liegen, wird nicht ueberschrieben)
  Get-ChildItem -Recurse -Path Unreal\SourceAssets\Audio\Aufnahmen -Include *.m4a,*.flac,*.aac,*.3gp,*.mp3 |
    ForEach-Object {
      $ziel = [System.IO.Path]::ChangeExtension($_.FullName, '.wav')
      ffmpeg -hide_banner -n -i $_.FullName -ar 48000 -c:a pcm_s24le $ziel
    }
  ```

  Für eine 44,1-kHz-WAV mit 16 Bit reicht `-ar 48000 -c:a pcm_s16le`. Danach die Originale aus dem
  Repository-Ordner herausnehmen (siehe 7, Versionierung).
- **Audacity** (kostenlos) geht auch: Datei öffnen → Projekt-Abtastrate 48000 Hz → *Exportieren als WAV* →
  „Signed 24-bit PCM“. **Keine** Effekte anwenden.

**Nie** über Messenger (WhatsApp, Signal …) oder E-Mail vom Handy auf den PC schicken – das komprimiert
die Dateien. Kabel, AirDrop oder ein Cloud-Ordner mit **Originaldatei** verwenden.

---

## 9. Die Aufnahmeliste im Detail

Spalte **Anzahl** = so viele **brauchbare** Einzelereignisse braucht das Spiel (Varianten). Nimm etwa das
**1,5-Fache** auf (Fehlversuche, Auswahl). „Serie“ = eine Datei (Abschnitt 5). Abstände: Abschnitt 3.

### 9.1 Ballschläge (`schlag`) – Hartplatz, mit Zuspieler oder Ballmaschine

Ort: `hart` (draußen, Nachbarplätze frei). Treffqualität und Schlagart je Variante (README 16.1, 5.4–5.7).

| Variante | Was genau | Anzahl nah | Anzahl fern | Hinweise |
|---|---|---|---|---|
| `sauber` | normale Grundschläge, sauber im Sweetspot – Vorhand und Rückhand, flach und Topspin gemischt | 30 | 15 | der wichtigste Klang des Spiels: ruhig und gleichmäßig spielen |
| `power` | harte, volle Schläge (Power-Schlag) | 20 | 10 | Abstand 2–3 m (Clipping!) |
| `rahmen` | Rahmentreffer (Mishits): Ball mit Rahmenkante bzw. am Schlägerhals treffen | 15 | 5 | Trick: Ball zuwerfen lassen und bewusst mit dem Rahmen spielen; auch Treffer am Rand der Saitenfläche |
| `slice` | Slice-Grundschläge (Unterschnitt, „bürstend“) | 15 | 5 | einhändig |
| `stopp` | weiche Schläge: Stopp, Lob, Touch | 15 | – | sehr leise → Abstand 1 m |
| `volley` | Volleys Vorhand/Rückhand am Netz (kurz, „gepunched“) | 20 | 5 | Handy am Netzpfosten |
| `halbvolley` | Halbvolley direkt nach dem Aufsprung **[OPTIONAL]** | 10 | – | Aufsprung und Schlag fast gleichzeitig |
| `smash` | Schmetterbälle (aus dem Stand und mit Sprung) | 15 | 5 | sehr laut → 3 m, Lobs zuspielen lassen |
| `aufschlag` | Aufschläge flach, Kick und Slice gemischt (Treffer + Schwung) | 20 | 10 | Handy neben der Grundlinie, außerhalb des Doppelfelds; Art im Protokoll vermerken |

**Pflicht-Summe:** 150 nah + 55 fern (+ 10 Halbvolley). Zeit: ca. **90 Minuten**.
Tipp: Wenn ein zweites Handy da ist, **nah und fern gleichzeitig** aufnehmen (Referenzklatschen auf beiden).

### 9.2 Schläger ohne Ball (`schlaeger`)

| Variante | Was genau | Ort | Anzahl | Hinweise |
|---|---|---|---|---|
| `schwung` | Leerschwung durch die Luft („Wusch“): Vorhand, Rückhand, Aufschlagbewegung | `hart` | 15 | 0,5–1 m, Schläger zieht seitlich vorbei – Abstand halten |
| `faellt` | Schläger fällt aus ca. 1 m Höhe auf den Boden (flach und auf die Kante) | `hart`, `sand`, `rasen` bzw. `garten`, `halle` | je 5 | nur auf weichem Belag mit wertvollem Schläger üben; sonst einen alten Schläger nehmen |
| `ablegen` | Schläger auf Bank bzw. Tasche legen | `verein` | 5 | für den Seitenwechsel |
| `abklopfen` | mit dem Schläger Sand von den Schuhsohlen klopfen (README 11) | `sand` | 10 | einzelne und doppelte Schläge |

Zeit: ca. **15 Minuten** (über die Termine verteilt).

### 9.3 Aufsprung je Belag (`aufsprung`)

Orte: `hart`, `sand`, `rasen` (sonst `garten`), `halle`. Je Belag:

| Variante | Was genau | Anzahl je Belag | Hinweise |
|---|---|---|---|
| `fallen` | Ball aus 1 m und 2 m Höhe fallen lassen (fangen, bevor er ein zweites Mal aufspringt) | 10 | Handy 1–1,5 m seitlich, 1 m hoch |
| `schnell` | Ball schräg und kräftig auf den Boden werfen bzw. schlagen – wie ein schneller Grundschlag | 10 | dazu 5 × `fern` (12 m): `aufsprung_schnell_<ort>_fern_NN.wav` |
| `hoch` | Ball 4–5 m hoch werfen (Lob-Aufsprung) | 5 | – |
| `linie` | Aufsprung genau auf der Linie (Sand: Linienband; Rasen: Kreide; Hart: Farbe) | 5 | auf Sand klingt das Linienband deutlich anders |

Anzahl: 30 + 5 fern je Belag, zusammen 140. Zeit: ca. **15 Minuten je Belag = 60 Minuten**.
[OPTIONAL] Zusätzlich je Belag 5 × Bodenperspektive (Handy auf Handtuch, 1 m entfernt) – im Protokoll vermerken.

### 9.4 Ball auftippen vor dem Aufschlag (`auftippen`)

Orte: `hart`, `sand`, `rasen` bzw. `garten`, `halle`. Wie auf der Tour: 1–4 Mal tippen, dann fangen (README 5.5).

| Variante | Was genau | Anzahl je Belag | Hinweise |
|---|---|---|---|
| `tippen` | Folgen mit 1, 2, 3 und 4 Tipps und Fangen am Ende | 10 Folgen | zwischen den Folgen 2 s Stille |
| `fangen` | Ball nur fangen (aus der Hand hochwerfen und fangen, Ball vom Ballkind fangen) | 5 (nur auf `hart`) | klingt auf allen Belägen gleich |
| `schlaeger` | Ball mit dem Schläger aufheben bzw. mit dem Schläger auftippen **[OPTIONAL]** (README 4.1) | 5 (nur auf `hart`) | – |

Zeit: ca. **8 Minuten je Belag = 30 Minuten**.

### 9.5 Ball rollt (`rollen`)

Orte: `hart`, `sand`, `rasen` bzw. `garten`, `halle`. Je Belag 3 × `langsam` (ausrollen lassen) und 3 × `schnell`
(Ball kräftig über den Platz rollen, z. B. zum Ballkind). Handy auf Handtuch 0,5 m neben der Rollbahn.
Zeit: ca. **15 Minuten** zusammen.

### 9.6 Netz (`netz`)

Ort: `hart` ([OPTIONAL] zusätzlich `halle`). Handy am Netzpfosten bzw. 1 m neben dem Auftreffpunkt in Netzhöhe.

| Variante | Was genau | Anzahl | Hinweise |
|---|---|---|---|
| `geflecht` | Ball fliegt ins Netz – langsam und schnell, unten und oben | 20 | aus 5–10 m ins Netz spielen lassen |
| `band` | Netzband (Netzroller): Ball streift die Netzkante und fällt drüber bzw. zurück | 15 | aus kurzer Entfernung gezielt aufs Band werfen/spielen; auch „Ball läuft auf dem Band“ |
| `pfosten` | Ball trifft den Netzpfosten **[OPTIONAL]** | 5 | – |

Zeit: ca. **20 Minuten**.

### 9.7 Sonstige Ballgeräusche (`ball`)

| Variante | Was genau | Ort | Anzahl | Hinweise |
|---|---|---|---|---|
| `zaun` | Ball prallt gegen Zaun bzw. Windschutzplane hinter der Grundlinie | `hart` | 10 | Aus-Bälle, die hinten anschlagen |
| `wand` | Ball prallt gegen Hallenwand bzw. Trennvorhang | `halle` | 10 | – |
| `korb` | Ballkorb: Bälle hineinschütten, Korb rütteln, Bälle herausnehmen (README 12.4) | `hart` | 5 | – |
| `dose` | **neue** Balldose öffnen („Pssst“ + Deckel) und Bälle herausnehmen – passt zu „New balls, please“ | `verein` | 5 | dafür die ungeöffneten Dosen mitnehmen |

Zeit: ca. **20 Minuten**.

### 9.8 Schritte je Belag (`schritt`)

Orte: `hart`, `sand`, `rasen` bzw. `garten`, `halle`. Mit Tennisschuhen (auf Sand die Sandsohle). Je Belag:

| Variante | Was genau | Anzahl je Belag | Hinweise |
|---|---|---|---|
| `gehen` | normales Gehen zwischen den Punkten (5–10 m hin und zurück) | 2 Serien à 30 s | – |
| `laufen` | Sprint über 10–15 m mit Antritt | 10 Läufe | am Handy vorbei |
| `seit` | Seitschritte im Athletenstand, links und rechts | 10 Folgen | – |
| `split` | Split-Step (kleiner Sprung, Landung auf beiden Füßen) | 15 | – |
| `stopp` | Abbremsen aus dem Lauf, Ausfallschritt | 10 | auf Hart/Halle entsteht dabei oft Quietschen – passt |
| `drehen` | Drehung bzw. Richtungswechsel (auf Sand: Knirschen) | 10 | – |

Zeit: ca. **15 Minuten je Belag = 60 Minuten**.

### 9.9 Rutschen (`rutschen`)

**Sicherheit zuerst:** nur rutschen, wer das kann; auf Hart und Halle nur kurze, kontrollierte Rutscher.
Auf Rasen nur, wenn er trocken ist.

| Variante | Ort | Was genau | Anzahl |
|---|---|---|---|
| `lang` | `sand` | langer Rutscher im Ausfallschritt in den Ball, mit Knirschen und Auslaufen (README 5.8) | 15 |
| `mittel` | `rasen` (sonst `garten`) | mittlerer Rutscher | 10 |
| `kurz` | `hart` | kurzer Rutscher mit Quietschen | 10 |
| `kurz` | `halle` | kurzer Rutscher (mit Hallenhall) | 10 |

Zeit: ca. **30 Minuten** (mit Pausen).

### 9.10 Schuhquietschen (`quietschen`)

Orte: `hart` und `halle` (README 16.1: Hartplatz/Halle). Je Ort:

| Variante | Was genau | Anzahl je Ort |
|---|---|---|
| `kurz` | ein kurzes Quietschen (Antritt, kleiner Stopp) | 15 |
| `lang` | langes Quietschen beim harten Abbremsen | 10 |
| `doppel` | zwei schnelle Quietscher beim Richtungswechsel | 10 |

Zeit: ca. **10 Minuten je Ort = 20 Minuten**. Saubere, frische Sohlen quietschen am besten; in der Halle ist das einer der typischsten Klänge.

### 9.11 Ballmaschine (`ballmaschine`) **[OPTIONAL – falls der Verein eine hat]**

Für den Trainingsmodus (README 13.2). Ort: `hart` oder `halle`.
`motor`: 2 × 1 min Laufgeräusch mit Ballzufuhr (Handy 2 m seitlich); `auswurf`: 20 Ballauswürfe („Plopp“),
verschiedene Tempo- und Drall-Einstellungen. Zeit: ca. **10 Minuten**.

### 9.12 Platzpflege auf Sand (`platzpflege`) **[OPTIONAL]**

Für die Satzpause auf Sand (README 12.3 [NEU]). Ort: `sand`.
`schleppnetz`: 2 Serien à 1 min Platz abziehen (Handy 3 m neben der Bahn); `besen`: 10 Linien fegen;
`wasser`: 1 min Bewässerung bzw. Schlauch. Zeit: ca. **15 Minuten** (am besten, wenn der Platzwart ohnehin abzieht).

### 9.13 Publikum im Verein (`publikum`)

Ort: `verein` (Terrasse, Tribüne, Bank am Platz) mit **5–15 Personen**, Stereo falls möglich. Es geht nicht
um ein großes Publikum, sondern um **viele saubere, unterschiedliche Takes einer kleinen Gruppe** – daraus
werden später tausende Zuschauer geschichtet. Deshalb:

- jede Variante bei **zwei Abständen**: nah (3–5 m) und `fern` (10 m),
- die Gruppe zwischen den Serien **umstellen** (links, Mitte, rechts vom Handy, stehend und sitzend),
- Personen mischen (Männer, Frauen, tiefe und hohe Stimmen), Klatschtempo und -stärke variieren lassen,
- **keine Namen**, keine echten Spieler, keine Sprache, die man verstehen kann – außer kurzen Rufen wie
  „Yes!“, „Wow!“, „Come on!“.

| Variante | Was genau | Anzahl nah | Anzahl fern |
|---|---|---|---|
| `applausleise` | höflicher, kurzer Applaus (2–3 s), z. B. nach einem Doppelfehler des Gegners | 10 | 5 |
| `applaus` | normaler Applaus nach einem guten Punkt (4–6 s) | 10 | 5 |
| `applauslaut` | begeisterter Applaus mit Jubel und Rufen nach langem Ballwechsel (8–12 s) | 8 | 4 |
| `jubel` | kurzer Aufschrei nach einem tollen Schlag („Yes!“, „Wow!“) | 8 | 4 |
| `raunen` | „Ohhh“ bei knappen Bällen bzw. vor dem Videobeweis | 8 | 4 |
| `staunen` | bewunderndes „Ahhh“ (Hawk-Eye: Ball war doch gut) | 8 | 4 |
| `enttaeuscht` | fallendes „Oh…“ (Heimspieler verliert den Punkt) | 8 | 4 |
| `lachen` | kurzes Lachen (kurioser Netzroller, Ball trifft Ballkind) | 8 | 4 |
| `murmeln` | unverständliches Gemurmel – alle reden gleichzeitig leise durcheinander (1–2 min) | 3 | 2 |
| `stille` | „Stille vor dem Aufschlag“: alle still, nur Einzelgeräusche (Husten, Räuspern, Stuhl, Flasche) – 1 min | 3 | 2 |
| `einzeln` | **[OPTIONAL]** jede Person klatscht allein 10 Mal – hilft der Synthese | 1 Serie je Person | – |
| `takt` | **[OPTIONAL]** rhythmisches Klatschen | 3 | – |

Zeit: ca. **60 Minuten** (mit Umstellen). Vorher allen kurz erklären, worum es geht, und ihr Einverständnis
einholen (siehe Abschnitt 1).

### 9.14 Spieler-Laute (`spieler`) **[OPTIONAL – README 16.1 [NEU]]**

Je Person, an der Ort-Stelle steht das Personen-Kürzel (`m1`, `w1` …). Ideal: mindestens **2 Männer- und
2 Frauenstimmen**. Aufnahme draußen auf dem Platz oder in einem stark gedämpften Raum (Vorhänge, Sofa),
Handy 1 m vor dem Mund, leicht seitlich. Am natürlichsten klingt es, wenn dabei wirklich geschlagen wird
(Leerschwung); dann den Schlag-Wusch nicht zu laut werden lassen.

| Variante | Was genau | Anzahl je Person |
|---|---|---|
| `kurz` | kurzer Anstrengungslaut beim Schlag („hah“, „hu“) | 15 |
| `lang` | langer Laut bei Aufschlag/Power-Schlag („u-aah“) | 10 |
| `atmen` | hörbares Ausatmen, Schnaufen zwischen den Punkten | 10 |
| `jubel` | „Come on!“, „Yes!“ (README 11: Faust ballen) | 8 |
| `frust` | Stöhnen, „Ahh!“, enttäuschtes Ausatmen – **keine Schimpfwörter** | 8 |
| `erschoepft` | Keuchen nach einem langen Ballwechsel (30 s) | 3 |

Zeit: ca. **15 Minuten je Person** (2 Personen = 30 Minuten).

### 9.15 Umgebung und Raumklang (`umgebung`)

Stereo, Handy frei stehend in 1,5 m Höhe. Je Aufnahme **2–3 Minuten** ohne Unterbrechung (die Atmo wird im
Spiel nahtlos geschleift). Niemand redet, niemand läuft am Handy vorbei.

| Variante | Ort | Was genau | Anzahl | Pflicht? |
|---|---|---|---|---|
| `stille` | jeder Aufnahmeort (`hart`, `sand`, `rasen`/`garten`, `halle`, `verein`) | 30 s Raumklang/Grundrauschen für die Rauschminderung (Abschnitt 5, Schritt 7) | 1 je Ort und Termin | **Pflicht** |
| `voegel` | `draussen` | Vögel früh morgens auf bzw. neben der Anlage (Bäume, Hecken) – für Sandplatz, Rasen und Training | 3 × 2 min | Pflicht |
| `wind` | `draussen` | leichter Wind in Bäumen und Hecken, Handy im Windschatten mit Windschutz | 3 × 2 min | Pflicht |
| `nachbarplaetze` | `hart` oder `sand` | Training mit Betrieb: entfernte Ballwechsel auf Nachbarplätzen, Stimmen unverständlich (README 12.4) | 3 × 3 min | Pflicht |
| `raum` | `halle` | leere Halle: Grundgeräusch, Lüftung, Licht (README 12.5) | 2 × 2 min | Pflicht |
| `abend` | `draussen` | Abend im Sommer (Grillen, Flutlicht-Brummen) für Abendmatches (README 12.7) | 2 × 2 min | [OPTIONAL] |
| `regen` | `draussen` bzw. `halle` | Regen auf Plane, Schirm bzw. Hallendach (README 12.7: Regenunterbrechung) | 2 × 2 min | [OPTIONAL] |
| `fahne` | `draussen` | Fahne bzw. Plane flattert im Wind | 2 × 1 min | [OPTIONAL] |
| `geblaese` | `halle` | Traglufthalle: Gebläse (README 12.5, Trainingshalle) | 1 × 2 min | [OPTIONAL] |
| `impuls` | `halle` und `hart` | Hall-Messung: 5 laute Klatscher bzw. zwei Holzbretter zusammenschlagen, jeweils **5 s** ausklingen lassen, an 3 Stellen im Raum | 3 Serien je Ort | [OPTIONAL] |

Zeit: ca. **60 Minuten** (über die Termine verteilt; Vögel und Abend zu eigener Uhrzeit).

---

## 10. Drehplan: sechs Termine

Die Liste ist so sortiert, dass jeder Termin an **einem Ort** stattfindet. Reihenfolge beliebig;
Termin A zuerst, weil dort die meisten und wichtigsten Klänge entstehen.

| Termin | Ort | Inhalt (Abschnitt) | reine Aufnahmezeit | mit Aufbau |
|---|---|---|---|---|
| **A** | Hartplatz draußen (Verein), Helfer zum Zuspielen | Ballschläge 9.1, Schwung 9.2, Aufsprung Hart 9.3, Auftippen Hart 9.4, Rollen Hart 9.5, Netz 9.6, Zaun/Korb 9.7, Schritte Hart 9.8, Rutschen Hart 9.9, Quietschen Hart 9.10, Schläger fällt Hart, Stille, [Ballmaschine 9.11] | ca. 3 h 15 min | ca. 4 h |
| **B** | Sandplatz | Aufsprung, Auftippen, Rollen, Schritte, Rutschen lang, Schläger fällt, Abklopfen, Stille, [Platzpflege 9.12] | ca. 1 h 15 min | ca. 1 h 45 min |
| **C** | Rasenplatz – sonst Gartenrasen (`garten`) | Aufsprung, Auftippen, Rollen, Schritte, Rutschen mittel, Schläger fällt, Stille | ca. 1 h | ca. 1 h 30 min |
| **D** | Tennishalle (gemietete Stunde) | Aufsprung, Auftippen, Rollen, Schritte, Quietschen, Rutschen kurz, Wand, Schläger fällt, Raum, Stille, [Netz Halle, Gebläse, Impuls] | ca. 1 h 15 min | ca. 1 h 45 min |
| **E** | Verein (Clubabend, Mannschaftstreffen, Grillfest) | Publikum 9.13, Ablegen, Dose, Stille, [Spieler-Laute 9.14] | ca. 1 h 30 min | ca. 2 h |
| **F** | draußen, verteilt | Vögel (früh morgens), Wind, Nachbarplätze (bei Betrieb), [Abend, Regen, Fahne] | ca. 45 min | ca. 1 h |

---

## 11. Checkliste zum Abhaken

Hinter jedem Punkt: brauchbare Anzahl (aufnehmen ca. 1,5-fach). `[OPT]` = optional.

### Vorbereitung
- [ ] App installiert und eingestellt: WAV, 48 kHz, 24 Bit (oder 16 Bit), AGC/Rauschunterdrückung aus
- [ ] Hauptmikrofon am Handy gefunden (Pustetest)
- [ ] Probeaufnahme: Smash aus 3 m – kein Clipping
- [ ] Stativ/Klemme, Windschutz, Handtuch, Powerbank, Kopfhörer eingepackt
- [ ] Neue Bälle (2 Dosen, eine ungeöffnet) + gebrauchte Bälle, zweiter Schläger
- [ ] Platz, Sandplatz, (Rasen), Halle beim Verein reserviert; Helfer und Publikum gefragt
- [ ] `protokoll.csv` angelegt

### Termin A – Hartplatz
- [ ] Stille Hart (30 s)
- [ ] Schlag sauber: 30 nah, 15 fern
- [ ] Schlag power: 20 nah, 10 fern
- [ ] Schlag rahmen: 15 nah, 5 fern
- [ ] Schlag slice: 15 nah, 5 fern
- [ ] Schlag stopp (Stopp/Lob/Touch): 15
- [ ] Schlag volley: 20 nah, 5 fern
- [ ] [OPT] Schlag halbvolley: 10
- [ ] Schlag smash: 15 nah, 5 fern
- [ ] Schlag aufschlag: 20 nah, 10 fern
- [ ] Schläger schwung: 15
- [ ] Schläger fällt (Hart): 5
- [ ] Aufsprung Hart: fallen 10, schnell 10 (+5 fern), hoch 5, linie 5
- [ ] Auftippen Hart: 10 Folgen; fangen 5; [OPT] mit Schläger 5
- [ ] Rollen Hart: 3 langsam, 3 schnell
- [ ] Netz: geflecht 20, band 15, [OPT] pfosten 5
- [ ] Ball zaun: 10; Ball korb: 5
- [ ] Schritte Hart: gehen 2 × 30 s, laufen 10, seit 10, split 15, stopp 10, drehen 10
- [ ] Rutschen kurz (Hart): 10
- [ ] Quietschen Hart: kurz 15, lang 10, doppel 10
- [ ] [OPT] Ballmaschine: motor 2 × 1 min, auswurf 20
- [ ] Probehören, Protokoll ergänzt

### Termin B – Sandplatz
- [ ] Stille Sand (30 s)
- [ ] Aufsprung Sand: fallen 10, schnell 10 (+5 fern), hoch 5, linie (Linienband) 5
- [ ] Auftippen Sand: 10 Folgen
- [ ] Rollen Sand: 3 langsam, 3 schnell
- [ ] Schritte Sand: gehen 2 × 30 s, laufen 10, seit 10, split 15, stopp 10, drehen 10
- [ ] Rutschen lang (Sand): 15
- [ ] Schläger fällt (Sand): 5
- [ ] Schläger abklopfen: 10
- [ ] [OPT] Platzpflege: schleppnetz 2 × 1 min, besen 10, wasser 1 min
- [ ] Probehören, Protokoll ergänzt

### Termin C – Rasen (bzw. Gartenrasen `garten`)
- [ ] Stille Rasen (30 s)
- [ ] Aufsprung Rasen: fallen 10, schnell 10 (+5 fern), hoch 5, linie (Kreide) 5
- [ ] Auftippen Rasen: 10 Folgen
- [ ] Rollen Rasen: 3 langsam, 3 schnell
- [ ] Schritte Rasen: gehen 2 × 30 s, laufen 10, seit 10, split 15, stopp 10, drehen 10
- [ ] Rutschen mittel (Rasen): 10 – nur bei trockenem Rasen
- [ ] Schläger fällt (Rasen): 5
- [ ] Probehören, Protokoll ergänzt

### Termin D – Halle
- [ ] Stille Halle (30 s); Hallenbelag im Protokoll notiert
- [ ] Aufsprung Halle: fallen 10, schnell 10 (+5 fern), hoch 5, linie 5
- [ ] Auftippen Halle: 10 Folgen
- [ ] Rollen Halle: 3 langsam, 3 schnell
- [ ] Schritte Halle: gehen 2 × 30 s, laufen 10, seit 10, split 15, stopp 10, drehen 10
- [ ] Quietschen Halle: kurz 15, lang 10, doppel 10
- [ ] Rutschen kurz (Halle): 10
- [ ] Ball wand: 10
- [ ] Schläger fällt (Halle): 5
- [ ] Umgebung raum: 2 × 2 min
- [ ] [OPT] Netz in der Halle, Gebläse (Traglufthalle), Impuls 3 Serien
- [ ] Probehören, Protokoll ergänzt

### Termin E – Verein (Publikum)
- [ ] Einverständnis aller Personen eingeholt
- [ ] Stille Verein (30 s)
- [ ] Applaus leise: 10 nah, 5 fern
- [ ] Applaus normal: 10 nah, 5 fern
- [ ] Applaus laut: 8 nah, 4 fern
- [ ] Jubel: 8 nah, 4 fern
- [ ] Raunen „Ohhh“: 8 nah, 4 fern
- [ ] Staunen „Ahhh“: 8 nah, 4 fern
- [ ] Enttäuscht „Oh…“: 8 nah, 4 fern
- [ ] Lachen: 8 nah, 4 fern
- [ ] Murmeln: 3 nah, 2 fern
- [ ] Stille vor dem Aufschlag: 3 nah, 2 fern
- [ ] [OPT] einzeln (je Person), takt 3
- [ ] Schläger ablegen: 5; Balldose öffnen: 5
- [ ] [OPT] Spieler-Laute je Person (m1, m2, w1, w2): kurz 15, lang 10, atmen 10, jubel 8, frust 8, erschöpft 3
- [ ] Probehören, Protokoll ergänzt

### Termin F – Umgebung
- [ ] Vögel früh morgens: 3 × 2 min
- [ ] Wind: 3 × 2 min
- [ ] Nachbarplätze: 3 × 3 min
- [ ] [OPT] Abend: 2 × 2 min
- [ ] [OPT] Regen: 2 × 2 min
- [ ] [OPT] Fahne: 2 × 1 min

### Abschluss
- [ ] Alle Dateien nach Schema benannt und in `Unreal/SourceAssets/Audio/Aufnahmen/<kategorie>/`
- [ ] Nicht-WAV umgewandelt, Originale aus dem Repository-Ordner entfernt
- [ ] Missglückte Serien gelöscht
- [ ] `protokoll.csv` vollständig
- [ ] `git lfs ls-files` zeigt die neuen WAV-Dateien → Commit und Push (Abschnitt 13)

---

## 12. Geschätzte Gesamtdauer und Speicherbedarf

| Kategorie | Abschnitt | Zeit (min) | davon optional |
|---|---|---:|---:|
| Ballschläge | 9.1 | 90 | 5 |
| Schläger ohne Ball | 9.2 | 15 | – |
| Aufsprung (4 Beläge) | 9.3 | 60 | – |
| Auftippen (4 Beläge) | 9.4 | 30 | 5 |
| Ball rollt | 9.5 | 15 | – |
| Netz | 9.6 | 20 | 5 |
| Sonstige Ballgeräusche | 9.7 | 20 | – |
| Schritte (4 Beläge) | 9.8 | 60 | – |
| Rutschen | 9.9 | 30 | – |
| Schuhquietschen | 9.10 | 20 | – |
| Ballmaschine | 9.11 | 10 | 10 |
| Platzpflege Sand | 9.12 | 15 | 15 |
| Publikum | 9.13 | 60 | 10 |
| Spieler-Laute (2 Personen) | 9.14 | 30 | 30 |
| Umgebung und Raumklang | 9.15 | 60 | 25 |
| **Summe** | | **535 (≈ 9 h)** | **105** |

* **Pflicht:** ca. **430 Minuten ≈ 7 h** reine Aufnahmezeit; mit allen optionalen Punkten ca. 9 h.
* **Mit Aufbau, Probehören und Wegen** (ca. 30 min je Termin): ca. **11–12 Stunden** an 6 Terminen (Tabelle in Abschnitt 10).
* **Ergebnis:** rund **1 100 brauchbare Einzelereignisse** plus ca. 45 Minuten Atmo, Gemurmel und Stille.
* **Speicher:** Die Dateien enthalten viel Stille (2 s je Ereignis). Geschätzt ca. 80 min Mono-Aufnahmen
  (≈ 0,7 GB bei 24 Bit, ≈ 0,45 GB bei 16 Bit) und ca. 60 min Stereo (Atmo, Publikum; ≈ 1 GB bzw. 0,7 GB) –
  zusammen also **ca. 1–1,7 GB** im Repository (Git LFS). Das passt in das kostenlose LFS-Kontingent
  (`Dokumente/Entscheidungen.md`, A11), liegt aber über der dort genannten Schätzung „deutlich unter 1 GB“.
  Sparen lässt sich mit 16 Bit für alles und Publikum in Mono (dann ca. 1 GB).

---

## 13. Nach der Aufnahme: aufs Repository

1. Dateien per Kabel/AirDrop auf den PC kopieren (nicht per Messenger, Abschnitt 8).
2. Nach Schema umbenennen und in `Unreal/SourceAssets/Audio/Aufnahmen/<kategorie>/` legen; bei Bedarf umwandeln.
3. Kurz reinhören (Anfang/Ende der Serie, laute Stellen), missglückte Serien löschen.
4. `protokoll.csv` ergänzen.
5. In PowerShell im Ordner der Arbeitskopie:

   ```powershell
   git lfs install
   git add Unreal/SourceAssets/Audio/Aufnahmen
   git lfs ls-files            # die neuen .wav muessen hier stehen (sonst NICHT committen)
   git commit -m "Tonaufnahmen: Termin A Hartplatz"
   git push
   ```

6. Claude Code Bescheid geben – mit M4 bereinigt und importiert das Skript die Aufnahmen.

---

## 14. Was später daraus wird (M4)

Zur Einordnung, was das Bereinigungs- und Importskript (README 16.1) mit den Aufnahmen machen wird:

1. Anfang bis zur ersten 3-s-Pause verwerfen (Ansage, Referenzklatschen), Referenzklatschen als
   Lautstärke-Bezug merken.
2. Serie an den Pausen in Einzelereignisse schneiden, Anfang/Ende mit kurzen Blenden.
3. Rauschminderung mit dem Profil aus der `stille`-Aufnahme desselben Orts.
4. Lautheit je Kategorie angleichen, bei Bedarf Mono.
5. Varianten erzeugen (leichte Tonhöhen- und Lautstärke-Abweichungen) und als WAV nach
   `Unreal/SourceAssets/_erzeugt/Audio/<kategorie>/` schreiben, mit `.meta.json` (Quelle = Aufnahme + Protokollzeile).
6. Import nach Unreal: Effekte nach `/Game/Audio/Effekte/<kategorie>/`, Publikum nach `/Game/Audio/Publikum/`
   (beide Chunk 4, `Data/chunks.json`), Atmo nach `/Game/Audio/Effekte/umgebung/` (Vorschlag).
7. MetaSounds wählen im Spiel zufällig unter den Varianten, mischen je nach Abstand nah/fern und legen den
   Hall der Anlage darüber; das große Publikum entsteht durch Schichtung der Vereins-Aufnahmen.
