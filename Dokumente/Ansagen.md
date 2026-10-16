# Schiedsrichter-Ansagen – der englische Textbaukasten (README 16.2, 7.2, 24.3)

Dieses Dokument beschreibt **alle Bausteine**, aus denen der Schiedsrichter (Chair Umpire) seine englischen
Ansagen zusammensetzt, **wie** sie zusammengesetzt werden, gibt **Beispiele**, **Vorschläge** für noch fehlende
Ansagen, einen **Vergleich mit den offiziellen ITF-Ansagen** und **Hinweise zur Aussprache** für ElevenLabs.

* **Quellen im Repository:** `Unreal/Data/ansagen.csv` (98 Bausteine, Spalten `Name` = Baustein-ID, `Text`),
  `Unreal/Source/MatchballTennis/Officials/Kern/MBAnsagen.cpp` (Zusammensetzung),
  `Unreal/Source/MatchballTennis/Match/Kern/MBMatchAblauf.cpp` (wann welche Ansage kommt),
  `Unreal/Tools/elevenlabs-stimme.ps1` (erzeugt die WAV-Bausteine).
* **Stand:** 16.10.2026. Alle Beispiele in Abschnitt 4 wurden in der Cloud mit dem **echten Kern-Code**
  (`MBAnsagen.cpp` + `MBZaehlweise.cpp`, GCC) und der aktuellen `ansagen.csv` erzeugt – nicht von Hand geschrieben.
* **Nichts in diesem Dokument ändert die CSV oder den Code.** Vorschläge (Abschnitte 7 und 8) sind
  ausdrücklich als **VORSCHLAG** markiert; ihre IDs stehen **noch nicht** in `ansagen.csv`.

Markierungen: **[PRÜFEN]** = beim Anhören bzw. mit dir zu klären · **[NEU]** = Ausbau laut README.

---

## Inhalt

1. [So funktioniert der Baukasten](#1-so-funktioniert-der-baukasten)
2. [Alle Bausteine (Stand `ansagen.csv`)](#2-alle-bausteine-stand-ansagencsv)
3. [Wie Ansagen zusammengesetzt werden](#3-wie-ansagen-zusammengesetzt-werden)
4. [Beispiele vollständiger Ansagen](#4-beispiele-vollständiger-ansagen)
5. [Namens-Bausteine](#5-namens-bausteine)
6. [Rückfall ohne Namen](#6-rückfall-ohne-namen)
7. [VORSCHLAG: fehlende Ansagen und neue Bausteine](#7-vorschlag-fehlende-ansagen-und-neue-bausteine)
8. [Vergleich mit den offiziellen ITF-Ansagen – Verbesserungsvorschläge](#8-vergleich-mit-den-offiziellen-itf-ansagen--verbesserungsvorschläge)
9. [Aussprache und ElevenLabs](#9-aussprache-und-elevenlabs)
10. [Offene Punkte für dich](#10-offene-punkte-für-dich)
11. [Quellen](#11-quellen)

---

## 1. So funktioniert der Baukasten

* Eine **Ansage** ist eine Folge von **Baustein-IDs**, z. B. `fifteen love` oder `game @0 . @0 leads n3 games_to n2`.
* Jede ID hat in `ansagen.csv` einen **Text**. `elevenlabs-stimme.ps1` erzeugt daraus je ID eine WAV-Datei
  `Unreal/SourceAssets/Audio/Umpire/<id>.wav` (+ `<id>.meta.json`); `Tools\assets.ps1` importiert sie nach
  `/Game/Audio/Umpire/` (Chunk 5 „Stimmen“, `Data/chunks.json`). Die Wiedergabe spielt die WAVs nacheinander ab.
* **`@0` / `@1`** sind Platzhalter für den **Namen** von Spieler 0 bzw. 1. Gesprochen wird der Nachname aus
  `Data/spieler.csv` über den Namens-Baustein `name_<nachname>.wav` (Abschnitt 5).
* **`,` und `.`** sind **Satzzeichen-Bausteine**: Sie haben **keine** Audiodatei, sondern bedeuten eine kurze
  **Pause** (Komma) bzw. eine längere **Pause** (Punkt). Im Text erscheinen sie als Satzzeichen; nach `.` beginnt
  der nächste Baustein groß. Doppelte Satzzeichen werden zusammengefasst.
  **VORSCHLAG** für die Pausenlängen (Umsetzung mit M4, beim Anhören abstimmen **[PRÜFEN]**):
  Komma **0,25 s**, Punkt **0,5 s**, zwischen zwei Bausteinen ohne Satzzeichen **0,05 s** (nur Stille kürzen,
  keine zusätzliche Pause).
* Fehlt ein Namens-Baustein, nutzt das Spiel die **kürzere Ansage ohne Namen** (README 16.2, Abschnitt 6).
* Wer die Ansagen auslöst (`MBMatchAblauf.cpp`): **Rufe** sofort beim Punktende (`fault`, `out`, `let`),
  die **Ansage nach dem Punkt** (Punktstand, Spiel, Satz, Match), **„New balls, please“** vor dem Spiel mit neuen
  Bällen, die **Challenge-Ansage** beim Anfordern und das **Challenge-Ergebnis** nach dem Videobeweis.

---

## 2. Alle Bausteine (Stand `ansagen.csv`)

Spalte **Verwendung**: wo der Baustein im Code vorkommt. „vorbereitet“ = steht in der CSV (wird also schon als
WAV erzeugt), wird aber noch von keiner Ansage benutzt.

### 2.1 Punkte im Spiel

| ID | Text | Verwendung |
|---|---|---|
| `love` | love | Punktstand 0; auch 0 Spiele („two games to love“, „two sets to love“) |
| `fifteen` | fifteen | Punktstand 15 |
| `thirty` | thirty | Punktstand 30 |
| `forty` | forty | Punktstand 40 |
| `all` | all | Gleichstand im Spiel und im Tiebreak („fifteen all“, „three all“) |
| `deuce` | deuce | Einstand |
| `advantage` | advantage | Vorteil + Name bzw. `server`/`receiver` |
| `deciding_point` | deciding point | No-Ad: Entscheidungspunkt bei Einstand |

### 2.2 Spiel und Spielstand

| ID | Text | Verwendung |
|---|---|---|
| `game` | game | Spielende („Game Adler“) |
| `leads` | leads | „Adler leads three games to two“ |
| `game_to` | game to | Führender hat genau 1 Spiel („one game to love“) |
| `games_to` | games to | sonst („three games to two“, „six games to four“) |
| `game_all` | game all | „one game all“ |
| `games_all` | games all | „three games all“ |

### 2.3 Tiebreak

| ID | Text | Verwendung |
|---|---|---|
| `tiebreak` | tie-break | Beginn eines Tiebreaks (auch Tiebreak bis 10 im Entscheidungssatz) |
| `match_tiebreak` | match tie-break | Beginn des Match-Tiebreaks und im Endergebnis |

### 2.4 Satz und Match

| ID | Text | Verwendung |
|---|---|---|
| `game_and` | game and | „Game and first set …“ |
| `first_set` | first set | Satzende Satz 1 |
| `second_set` | second set | Satzende Satz 2 |
| `third_set` | third set | Satzende Satz 3 |
| `fourth_set` | fourth set | Satzende Satz 4 |
| `fifth_set` | fifth set | Satzende Satz 5 |
| `game_set_and_match` | game, set and match | Matchende |
| `sets_to` | sets to | „two sets to one“ im Matchergebnis |

### 2.5 Rufe und Ansagen an das Publikum

| ID | Text | Verwendung |
|---|---|---|
| `fault` | fault | Aufschlagfehler (auch beim Doppelfehler) – Ruf |
| `out` | out | Ball im Ballwechsel aus – Ruf |
| `let` | let | Netzberührung beim Aufschlag, Ball im Feld – Ruf |
| `time` | time | vorbereitet (Ende der Pause beim Seitenwechsel, README 16.2) |
| `quiet_please` | quiet please | vorbereitet (Unruhe im Publikum, README 12.8 [NEU]) |
| `thank_you` | thank you | vorbereitet (nach „Quiet please“, wenn es ruhig ist) |
| `new_balls_please` | new balls, please | vor dem Spiel mit neuen Bällen (nach 7, dann alle 9 Spiele) |
| `ladies_and_gentlemen` | ladies and gentlemen | vorbereitet (README 16.2 [NEU]) |

### 2.6 Challenge und Videobeweis

| ID | Text | Verwendung |
|---|---|---|
| `is_challenging_the_call_on_the` | is challenging the call on the | „Adler is challenging the call on the …“ |
| `left_baseline` | left baseline | Grundlinie links aus Sicht des Stuhls (nahe Grundlinie) |
| `right_baseline` | right baseline | Grundlinie rechts aus Sicht des Stuhls (ferne Grundlinie) |
| `left_service_line` | left service line | Aufschlaglinie links (nahe Hälfte) |
| `right_service_line` | right service line | Aufschlaglinie rechts (ferne Hälfte) |
| `near_sideline` | near sideline | Seitenlinie auf der Seite des Stuhls |
| `far_sideline` | far sideline | Seitenlinie gegenüber dem Stuhl |
| `centre_service_line` | centre service line | Mittellinie (Aufschlag) |
| `line` | line | Rückfall, wenn keine Linie bekannt ist („… on the line“) |
| `correction` | correction | erfolgreiche Challenge: „Correction, …“ |
| `the_ball_was_good` | the ball was good | Ergebnis: Ball war gut |
| `the_ball_was_out` | the ball was out | Ergebnis: Ball war aus |
| `replay_the_point` | replay the point | Punkt wird wiederholt |
| `first_serve` | first serve | … mit erstem Aufschlag |
| `second_serve` | second serve | Aufschlag war doch aus → zweiter Aufschlag |

### 2.7 Rückfall ohne Namen

| ID | Text | Verwendung |
|---|---|---|
| `server` | server | „Advantage server“ statt „Advantage Adler“ |
| `receiver` | receiver | „Advantage receiver“ |

### 2.8 Ablauf, Verwarnungen, Aufgabe

| ID | Text | Verwendung |
|---|---|---|
| `players_ready` | players ready | vorbereitet |
| `play` | play | vorbereitet |
| `warm_up_time` | two minutes | vorbereitet (Einspielzeit; ID und Text passen nicht zusammen, siehe V11) |
| `code_violation` | code violation | vorbereitet |
| `time_violation` | time violation | vorbereitet (z. B. 25-Sekunden-Uhr, README 5.5 [NEU]) |
| `warning` | warning | vorbereitet |
| `retired` | retired | vorbereitet |
| `wins_by_retirement` | wins by retirement | vorbereitet |

### 2.9 Zahlen

Für Spielstände, Tiebreak-Punkte, Sätze und Satzergebnisse. Über 40 wird auf `n40` begrenzt (ein Tiebreak bis
40 reicht praktisch immer).

| ID | Text | ID | Text | ID | Text | ID | Text |
|---|---|---|---|---|---|---|---|
| `n0` | zero | `n11` | eleven | `n21` | twenty-one | `n31` | thirty-one |
| `n1` | one | `n12` | twelve | `n22` | twenty-two | `n32` | thirty-two |
| `n2` | two | `n13` | thirteen | `n23` | twenty-three | `n33` | thirty-three |
| `n3` | three | `n14` | fourteen | `n24` | twenty-four | `n34` | thirty-four |
| `n4` | four | `n15` | fifteen | `n25` | twenty-five | `n35` | thirty-five |
| `n5` | five | `n16` | sixteen | `n26` | twenty-six | `n36` | thirty-six |
| `n6` | six | `n17` | seventeen | `n27` | twenty-seven | `n37` | thirty-seven |
| `n7` | seven | `n18` | eighteen | `n28` | twenty-eight | `n38` | thirty-eight |
| `n8` | eight | `n19` | nineteen | `n29` | twenty-nine | `n39` | thirty-nine |
| `n9` | nine | `n20` | twenty | `n30` | thirty | `n40` | forty |
| `n10` | ten | | | | | | |

`n0` („zero“) kommt nur im **Tiebreak** (und im Match-Tiebreak-Ergebnis) vor; bei Punkten, Spielen, Sätzen und
den Satzergebnissen der Match-Ansage heißt 0 immer `love` (V3, umgesetzt). `n15`, `n30`, `n40` haben denselben Text wie `fifteen`, `thirty`, `forty`
(werden trotzdem getrennt erzeugt, ca. 16 Zeichen).

**Summe:** 8 + 6 + 2 + 8 + 8 + 15 + 2 + 8 + 41 = **98 Bausteine**, zusammen **921 Zeichen** Text.

---

## 3. Wie Ansagen zusammengesetzt werden

Grundlage: `AnsageNachPunkt()` in `MBAnsagen.cpp` bekommt den Stand **nach** dem Punkt und die Folgen des Punkts
(Spiel/Satz/Match gewonnen, Tiebreak beginnt). Es gilt immer genau **eine** der folgenden Regeln, von oben nach unten:

| # | Situation | Bausteine | Beispiel |
|---|---|---|---|
| 1 | **Match gewonnen** | `game_set_and_match @S ,` + Matchstand (unten) | „Game, set and match Adler, two sets to one, six four, three six, seven six“ |
| 2 | **Satz gewonnen** | `game_and <n>_set @S , <Spiele S> games_to <Spiele V>` (+ `. match_tiebreak`, wenn danach der Match-Tiebreak beginnt) | „Game and first set Adler, six games to four“ |
| 3 | **Spiel gewonnen** | `game @S .` + Spielstand (unten) (+ `. tiebreak`, wenn danach der Tiebreak beginnt) | „Game Adler. Adler leads three games to two“ |
| 4 | **Tiebreak läuft** (auch Match-Tiebreak) | Führender zuerst: `<a> <b> , @Führender`; gleich: `<a> all` | „Three two, Falk“ · „One all“ |
| 5 | **Einstand** | `deuce` | „Deuce“ |
| 6 | **Vorteil** | `advantage @V` | „Advantage Adler“ |
| 7 | **No-Ad, 40:40** | `deuce deciding_point` | „Deuce deciding point“ |
| 8 | **Gleichstand** im Spiel | `<Punkte> all` | „Fifteen all“ |
| 9 | **sonst** | Punkte des **Aufschlägers zuerst**: `<Auf> <Rück>` | „Thirty fifteen“ · „Love forty“ |

`@S` = Sieger bzw. Gewinner, `@V` = Spieler mit Vorteil, `<a>`/`<b>` = Zahlbausteine `n0`…`n40`.

**Spielstand nach einem Spiel** (Regel 3):

* gleich: `<n> game_all` bei 1:1, sonst `<n> games_all` – „One game all“, „Four games all“;
* sonst: `@Führender leads <hoch> game_to|games_to <niedrig>` – `game_to` nur, wenn der Führende genau 1 Spiel hat;
  0 Spiele heißen `love` – „Adler leads one game to love“, „Holloway leads four games to three“.

**Matchstand** (Regel 1):

* **1 Gewinnsatz** (ein normaler Satz): `<Spiele S> games_to <Spiele V>` – „six games to four“;
* **2 oder 3 Gewinnsätze**: `<Sätze S> sets_to <Sätze V>` (0 = `love`), danach **je Satz** `, <Spiele S> <Spiele V>`
  aus Sicht des Siegers – „two sets to one, six four, three six, seven six“; ein Match-Tiebreak erscheint als
  `, match_tiebreak <Punkte S> <Punkte V>` – „…, match tie-break ten six“.

**Rufe** (`MBMatchAblauf.cpp`, sofort beim Punktende): `let` (Netz beim Aufschlag, Ball im Feld), sonst beim
Aufschlag `fault` (Fehler **und** Doppelfehler), sonst `out`, wenn der Ball im Ballwechsel aus gerufen wurde.
Vor dem Spiel mit neuen Bällen: `new_balls_please`.

**Challenge** (`AnsageChallenge`): `@C is_challenging_the_call_on_the <linie>`. Die Linie wird aus Sicht des
Schiedsrichterstuhls benannt (Stuhl an der Netzseite rechts aus Sicht der nahen Grundlinie, Blick über den Platz):
nahe Grundlinie = **left baseline**, ferne = **right baseline**, entsprechend die Aufschlaglinien; Seitenlinie auf
der Stuhlseite = **near sideline**, gegenüber = **far sideline**; Mittellinie = **centre service line**
(Entscheidung B7 – noch offen, bitte bestätigen).

**Challenge-Ergebnis** (`AnsageChallengeErgebnis`):

| Ergebnis der Auswertung | Bausteine | Danach |
|---|---|---|
| erfolglos, Ball war gut | `the_ball_was_good` | normale Ansage nach dem Punkt |
| erfolglos, Ball war aus | `the_ball_was_out` | normale Ansage nach dem Punkt |
| erfolgreich → Punkt wiederholen bzw. 1. Aufschlag wiederholen | `correction , the_ball_was_good . replay_the_point . first_serve` | – |
| erfolgreich → Aufschlag war doch aus (es war der 1.) | `correction , the_ball_was_out . second_serve` | – |
| erfolgreich → Ass, klarer Winner | `correction , the_ball_was_good` | Ansage nach dem Punkt (Punkt für den Schlagenden) |
| erfolgreich → Doppelfehler, Punkt für den Challenger | `correction , the_ball_was_out` | Ansage nach dem Punkt |

---

## 4. Beispiele vollständiger Ansagen

Erzeugt mit dem Kern-Code (Spieler 0 / Spieler 1 = Adler / Holloway bzw. Falk / Moreau). „Text“ ist genau das,
was `FAnsage::Text()` liefert (ohne Schlusspunkt); „Ohne Namen“ ist `OhneNamen()` für den Fall, dass die
Namens-Bausteine fehlen.

| Nr | Situation | Bausteine | Text | Ohne Namen |
|---|---|---|---|---|
| 1 | Aufschläger Adler gewinnt den 1. Punkt | `fifteen love` | Fifteen love | = |
| 2 | Rückschläger gewinnt den 1. Punkt | `love fifteen` | Love fifteen | = |
| 3 | 15:15 | `fifteen all` | Fifteen all | = |
| 4 | 30:15 | `thirty fifteen` | Thirty fifteen | = |
| 5 | 15:40 (zwei Breakbälle) | `fifteen forty` | Fifteen forty | = |
| 6 | 40:30 | `forty thirty` | Forty thirty | = |
| 7 | 40:40 | `deuce` | Deuce | = |
| 8 | Vorteil Aufschläger Adler | `advantage @0` | Advantage Adler | Advantage server |
| 9 | Vorteil Rückschläger Holloway | `advantage @1` | Advantage Holloway | Advantage receiver |
| 10 | Spiel Adler, 1:0 | `game @0 . @0 leads n1 game_to love` | Game Adler. Adler leads one game to love | Game. One game to love |
| 11 | Spiel Holloway, 1:1 | `game @1 . n1 game_all` | Game Holloway. One game all | Game. One game all |
| 12 | Spiel Adler, 3:2 | `game @0 . @0 leads n3 games_to n2` | Game Adler. Adler leads three games to two | Game. Three games to two |
| 13 | Spiel Holloway, 3:4 | `game @1 . @1 leads n4 games_to n3` | Game Holloway. Holloway leads four games to three | Game. Four games to three |
| 14 | Spiel Adler, 4:4 | `game @0 . n4 games_all` | Game Adler. Four games all | Game. Four games all |
| 15 | Spiel Holloway, 6:6 → Tiebreak | `game @1 . n6 games_all . tiebreak` | Game Holloway. Six games all. Tie-break | Game. Six games all. Tie-break |
| 16 | Tiebreak 1:0 Adler | `n1 n0 , @0` | One zero, Adler | One zero |
| 17 | Tiebreak 1:1 | `n1 all` | One all | = |
| 18 | Tiebreak 3:2 Falk | `n3 n2 , @0` | Three two, Falk | Three two |
| 19 | Tiebreak 6:3 Adler (Satzbälle) | `n6 n3 , @0` | Six three, Adler | Six three |
| 20 | langer Tiebreak 11:10 | `n11 n10 , @0` | Eleven ten, Adler | Eleven ten |
| 21 | Satz Adler 6:4 | `game_and first_set @0 , n6 games_to n4` | Game and first set Adler, six games to four | Game and first set, six games to four |
| 22 | 2. Satz Holloway 6:3 | `game_and second_set @1 , n6 games_to n3` | Game and second set Holloway, six games to three | Game and second set, six games to three |
| 23 | Satz Adler im Tiebreak | `game_and first_set @0 , n7 games_to n6` | Game and first set Adler, seven games to six | Game and first set, seven games to six |
| 24 | 1. Satz Moreau 6:0 | `game_and first_set @1 , n6 games_to love` | Game and first set Moreau, six games to love | Game and first set, six games to love |
| 25 | kurzer Satz (4 Spiele) | `game_and first_set @0 , n4 games_to love` | Game and first set Falk, four games to love | Game and first set, four games to love |
| 26 | Match Adler 6:4 3:6 7:6 | `game_set_and_match @0 , n2 sets_to n1 , n6 n4 , n3 n6 , n7 n6` | Game, set and match Adler, two sets to one, six four, three six, seven six | Game, set and match, two sets to one, six four, three six, seven six |
| 27 | Match Adler 7:6 7:6 | `game_set_and_match @0 , n2 sets_to love , n7 n6 , n7 n6` | Game, set and match Adler, two sets to love, seven six, seven six | Game, set and match, two sets to love, seven six, seven six |
| 28 | Match Moreau 6:0 7:5 | `game_set_and_match @1 , n2 sets_to love , n6 love , n7 n5` | Game, set and match Moreau, two sets to love, six love, seven five | Game, set and match, two sets to love, six love, seven five |
| 29 | Match Adler 6:0 6:0 6:0 (3 Gewinnsätze) | `game_set_and_match @0 , n3 sets_to love , n6 love , n6 love , n6 love` | Game, set and match Adler, three sets to love, six love, six love, six love | Game, set and match, three sets to love, six love, six love, six love |
| 30 | Match mit 1 Gewinnsatz | `game_set_and_match @0 , n6 games_to n4` | Game, set and match Falk, six games to four | Game, set and match, six games to four |
| 31 | 1 Gewinnsatz, Tiebreak bis 10 gewonnen | `game_set_and_match @0 , n7 games_to n6` | Game, set and match Falk, seven games to six | Game, set and match, seven games to six |
| 32 | 2. Satz Falk 6:4, Match-Tiebreak folgt | `game_and second_set @0 , n6 games_to n4 . match_tiebreak` | Game and second set Falk, six games to four. Match tie-break | Game and second set, six games to four. Match tie-break |
| 33 | Match-Tiebreak 4:4 | `n4 all` | Four all | = |
| 34 | Match-Tiebreak 7:6 Falk | `n7 n6 , @0` | Seven six, Falk | Seven six |
| 35 | Match Falk mit Match-Tiebreak | `game_set_and_match @0 , n2 sets_to n1 , love n6 , n6 n4 , match_tiebreak n10 n6` | Game, set and match Falk, two sets to one, love six, six four, match tie-break ten six | Game, set and match, two sets to one, love six, six four, match tie-break ten six |
| 36 | No-Ad, 40:40 | `deuce deciding_point` | Deuce deciding point | = |
| 37 | Challenge, nahe Grundlinie | `@0 is_challenging_the_call_on_the left_baseline` | Adler is challenging the call on the left baseline | Is challenging the call on the left baseline |
| 38 | Challenge, ferne Grundlinie | `@1 is_challenging_the_call_on_the right_baseline` | Holloway is challenging the call on the right baseline | Is challenging the call on the right baseline |
| 39 | Challenge, Aufschlaglinie | `@1 is_challenging_the_call_on_the left_service_line` | Holloway is challenging the call on the left service line | Is challenging the call on the left service line |
| 40 | Challenge, Seitenlinie / Mittellinie | `@0 is_challenging_the_call_on_the near_sideline` · `… centre_service_line` | Adler is challenging the call on the near sideline · … on the centre service line | Is challenging … |
| 41 | Challenge erfolgreich, Ball war gut | `correction , the_ball_was_good . replay_the_point . first_serve` | Correction, the ball was good. Replay the point. First serve | = |
| 42 | Challenge erfolgreich, 1. Aufschlag war aus | `correction , the_ball_was_out . second_serve` | Correction, the ball was out. Second serve | = |
| 43 | Challenge erfolgreich, Ass bzw. klarer Winner | `correction , the_ball_was_good` | Correction, the ball was good | = |
| 44 | Challenge erfolglos | `the_ball_was_out` | The ball was out | = |
| 45 | Rufe | `fault` · `out` · `let` · `new_balls_please` | Fault · Out · Let · New balls, please | = |

„=“ bedeutet: ohne Namen identisch. Die Beispiele 28, 29 und 35 sprechen 0 Spiele seit der Korrektur V3 als
„**love**“ („six love“, „love six“). Beispiel 36 zeigt eine Schwäche aus Abschnitt 8 (fehlendes Komma, V10)
und die Rückfälle in 10–15 und 37–40 (V15, V16).

---

## 5. Namens-Bausteine

**Regel** (`ConvertTo-NamensId` in `elevenlabs-stimme.ps1`, Entscheidung B20): Der Namens-Baustein heißt
`name_<id>.wav`, gesprochen wird der **Nachname** genau so, wie er in `spieler.csv` steht. Die `<id>` entsteht so:

1. deutsche Umlaute zuerst: ä → ae, ö → oe, ü → ue, ß → ss (auch groß);
2. Sonderbuchstaben: ø → o, æ → ae, œ → oe, ł → l, đ → d, þ → th, ı → i;
3. übrige Akzente entfernen (á → a, š → s, ñ → n …);
4. alles klein; jede Folge anderer Zeichen (Leerzeichen, Bindestrich, Apostroph) → ein `_`; `_` am Rand entfernen.

| Nachname | Datei | | Nachname | Datei |
|---|---|---|---|---|
| Adler | `name_adler.wav` | | Falk | `name_falk.wav` |
| Holloway | `name_holloway.wav` | | Moreau | `name_moreau.wav` |
| Varela | `name_varela.wav` | | Marchetti | `name_marchetti.wav` |
| Brooks | `name_brooks.wav` | | Thompson | `name_thompson.wav` |
| Hruška | `name_hruska.wav` | | Bláhová | `name_blahova.wav` |
| Arakawa | `name_arakawa.wav` | | Tanabe | `name_tanabe.wav` |
| Ferreira | `name_ferreira.wav` | | Zhou | `name_zhou.wav` |
| Lindqvist | `name_lindqvist.wav` | | Solberg | `name_solberg.wav` |
| Castellano | `name_castellano.wav` | | Ríos | `name_rios.wav` |
| Vermeulen | `name_vermeulen.wav` | | Bennett | `name_bennett.wav` |

Weitere Beispiele: „Müller“ → `name_mueller`, „Van der Berg“ → `name_van_der_berg`, „O'Neill“ → `name_o_neill`.

* **Eigene Namen** (Karriere-Figur, prozedurale Gegner): `elevenlabs-stimme.ps1 -Namen "Meier,Huber"` erzeugt nur
  die fehlenden; `-Liste` zeigt ohne Schlüssel, welche Namen noch keine Aufnahme haben (README 16.2: Anzeige in den Optionen).
* **Zwei Spieler mit gleichem Nachnamen** teilen sich eine Datei – im Kader kommt das nicht vor.
* **Wiedergabe im Spiel (M4):** Die Abbildung `@0`/`@1` → `name_<id>` gibt es im C++-Code noch nicht; sie muss
  **genau dieselbe Regel** benutzen wie das Skript (sonst findet das Spiel die Datei nicht und fällt auf
  „ohne Namen“ zurück). Vorschlag: die Regel einmal in `Core/Kern/` umsetzen und mit den Beispielen dieser Tabelle testen.

---

## 6. Rückfall ohne Namen

`FAnsage::OhneNamen()` macht aus einer Ansage mit Namen eine ohne (README 16.2: „sonst wird die kürzere Ansage
ohne Namen verwendet (‚Game‘, ‚Advantage server‘)“):

* `advantage @x` → `advantage server` bzw. `advantage receiver` (je nachdem, ob x gerade aufschlägt);
* `@x leads` → beides fällt weg (der Spielstand bleibt);
* jeder andere Name fällt ersatzlos weg; ein Komma am Ende wird entfernt.

| Mit Namen | Ohne Namen heute | Bewertung |
|---|---|---|
| Advantage Adler | Advantage server | ✅ wie README |
| Game Adler. Adler leads three games to two | Game. Three games to two | ⚠️ unklar, **wer** führt (V15) |
| Game Adler. Four games all | Game. Four games all | ✅ eindeutig |
| Three two, Adler | Three two | ⚠️ unklar, wer führt – im Tiebreak aber üblich kurz; die Einblendung zeigt den Stand |
| Game and first set Adler, six games to four | Game and first set, six games to four | ✅ verständlich |
| Game, set and match Adler, … | Game, set and match, … | ✅ verständlich |
| Adler is challenging the call on the left baseline | Is challenging the call on the left baseline | ❌ Satz ohne Subjekt (V16) |

---

## 7. VORSCHLAG: fehlende Ansagen und neue Bausteine

> **VORSCHLAG – noch nicht umgesetzt.** Keine dieser IDs steht in `ansagen.csv`. Wenn du zustimmst, kommen sie in
> die CSV (das Skript erzeugt dann nur diese neuen Bausteine), und der Code bekommt die passenden Ansagen.

### 7.1 Ruhe im Publikum, Seitenwechsel (README 12.8, 16.2 [NEU])

Die Bausteine `time`, `quiet_please`, `thank_you` und `ladies_and_gentlemen` gibt es schon; es fehlen nur die
Aufrufe im Code und ein paar Ergänzungen:

| Neue ID (VORSCHLAG) | Text | Beispiel-Ansage |
|---|---|---|
| `please` | please | „Ladies and gentlemen, please.“ (`ladies_and_gentlemen , please`) – Bitte um Ruhe |
| `please_take_your_seats` | please take your seats | „Ladies and gentlemen, please take your seats.“ (nach dem Seitenwechsel, wenn Zuschauer noch gehen) |
| `two_minutes` | two minutes | „Two minutes.“ – ersetzt `warm_up_time` (V11) |
| `one_minute` | one minute | „One minute.“ (Einspielzeit, optional) |

Ablauf-Vorschlag: Seitenwechsel mit Pause → nach Ablauf **„Time.“** (`time`); Unruhe vor dem Aufschlag →
**„Quiet please.“**, sobald es ruhig ist **„Thank you.“**; starke Unruhe → **„Ladies and gentlemen, please.“**

### 7.2 Vorstellung vor dem Match – „Ladies and gentlemen …“ (README 16.2 [NEU])

Nach dem Vorbild der Vorstellung durch den Schiedsrichter (ITF/USTA, Abschnitt 8): Format, Spieler links und
rechts vom Stuhl, Ergebnis der Platzwahl.

| Neue ID (VORSCHLAG) | Text |
|---|---|
| `this_match_is_the_best_of` | this match is the best of |
| `this_match_is_one_set` | this match is one set |
| `tiebreak_sets` | tie-break sets |
| `with_a_match_tiebreak` | with a match tie-break |
| `to_the_left_of_the_chair` | to the left of the chair |
| `to_the_right_of_the_chair` | to the right of the chair |
| `won_the_toss_and_elected_to_serve` | won the toss and elected to serve |
| `won_the_toss_and_elected_to_receive` | won the toss and elected to receive |
| `to_serve` | to serve |

Beispiel: „Ladies and gentlemen, this match is the best of three tie-break sets. To the left of the chair, Adler.
To the right of the chair, Holloway. Holloway won the toss and elected to serve.“ – vor dem ersten Punkt:
„Holloway to serve. Players ready. Play.“ (`@1 to_serve . players_ready . play`) **[PRÜFEN]** Wortlaut.

### 7.3 Challenge-Ergänzungen (README 7.2)

| Neue ID (VORSCHLAG) | Text | Wofür |
|---|---|---|
| `the_ball_was_called_out` | the ball was called out | ITF-Ansage beim Anfordern (V4) |
| `the_ball_was_called_in` | the ball was called in | ebenso, wenn „gut“ gegeben war (V4) |
| `has` | has | „Adler has two challenges remaining“ (V6) |
| `challenge_remaining` | challenge remaining | „… one challenge remaining“ |
| `challenges_remaining` | challenges remaining | „… two challenges remaining“ |
| `no_challenges_remaining` | no challenges remaining | „… no challenges remaining“ |
| `challenge_on_the` | challenge on the | Rückfall ohne Namen: „Challenge on the left baseline“ (V16) |

Beispiel: „Adler is challenging the call on the left baseline, the ball was called out.“ – nach dem Videobeweis
(erfolglos): „The ball was out. Adler has one challenge remaining.“

### 7.4 Satzstand nach einem Satz (V2)

| Neue ID (VORSCHLAG) | Text | Beispiel |
|---|---|---|
| `set_to` | set to | „Adler leads one set to love“ |
| `set_all` | set all | „One set all“ |
| `sets_all` | sets all | „Two sets all“ (3 Gewinnsätze) |

Beispiel: „Game and second set Holloway, six games to three. One set all.“

### 7.5 Regelverstöße, Fußfehler, Aufgabe

| Neue ID (VORSCHLAG) | Text | Wofür |
|---|---|---|
| `foot_fault` | foot fault | Fußfehler (README 5.5 [NEU, optional]) – Ruf des Linienrichters |
| `delay_of_game` | delay of game | „Code violation, delay of game, warning, Adler“ |
| `point_penalty` | point penalty | zweite Verwarnung (Zeitregel, README 5.5 [NEU]) **[PRÜFEN]** Stufen |
| `has_retired` | has retired | „Ladies and gentlemen, Holloway has retired. Adler wins by retirement.“ (Aufgeben, README 15.2) |

### 7.6 Regen und Dach (README 12.7 [NEU])

| Neue ID (VORSCHLAG) | Text |
|---|---|
| `play_is_suspended` | play is suspended |
| `due_to_rain` | due to rain |
| `the_roof_will_now_be_closed` | the roof will now be closed |
| `play_will_resume_shortly` | play will resume shortly |

Beispiel: „Ladies and gentlemen, play is suspended due to rain. The roof will now be closed. Play will resume shortly.“

### 7.7 Stadionsprecher für Einlauf und Siegerehrung (README 14.9, 16.2 [NEU])

Der Stadionsprecher ist eine **zweite Stimme** (README 16.2: „eigene ElevenLabs-Stimme“). Vorschlag:

* eigene Datei **`Unreal/Data/stadionsprecher.csv`** (gleiche Spalten `Name,Text`), IDs mit Präfix **`sp_`**;
* WAVs nach **`Unreal/SourceAssets/Audio/Stadionsprecher/`**, Import nach `/Game/Audio/Stadionsprecher/`
  (steht schon in Chunk 5 von `Data/chunks.json`);
* **Name in voller Länge** (`sp_name_<vorname>_<nachname>`, z. B. `sp_name_leon_adler` – „Leon Adler“) und Land
  (`sp_land_<iso3>`, z. B. `sp_land_aut` – „Austria“), Turniere aus den Turnier-Tabellen (`sp_turnier_<id>`).
  Echte Turniernamen nur für die private Nutzung (README 22.3).
* `elevenlabs-stimme.ps1` liest heute fest `ansagen.csv`; dafür bräuchte es einen Parameter für die CSV
  (z. B. `-Tabelle stadionsprecher.csv -Ziel …\Stadionsprecher -StimmeId <andere Stimme>`) – siehe Abschnitt 10.

| Neue ID (VORSCHLAG) | Text |
|---|---|
| `sp_ladies_and_gentlemen` | Ladies and gentlemen |
| `sp_welcome_to` | welcome to |
| `sp_please_welcome` | please welcome |
| `sp_to_the_court` | to the court |
| `sp_and` | and |
| `sp_from` | from |
| `sp_currently_ranked_number` | currently ranked number |
| `sp_in_the_world` | in the world |
| `sp_seeded_number` | seeded number |
| `sp_first_round` | the first round |
| `sp_second_round` | the second round |
| `sp_quarterfinal` | the quarter-final |
| `sp_semifinal` | the semi-final |
| `sp_final` | the final |
| `sp_the_runner_up` | the runner-up |
| `sp_your_champion` | your champion |
| `sp_a_big_round_of_applause_for` | please give a big round of applause for |
| `sp_congratulations` | congratulations |
| `sp_thank_you_and_see_you_next_year` | thank you, and see you next year |
| `sp_n1` … `sp_n100` | one … one hundred (Weltranglisten-Platz bzw. Setzung; über 100 wird der Platz nicht genannt) |

Beispiele:

* **Einlauf:** „Ladies and gentlemen, welcome to the final of *\<Turnier\>*. Please welcome to the court, from
  Great Britain, currently ranked number four in the world, Rhys Holloway! And from Austria, currently ranked
  number one in the world, Leon Adler!“
* **Siegerehrung:** „Ladies and gentlemen, the runner-up, Rhys Holloway! … And your champion, Leon Adler! Please give
  a big round of applause for Leon Adler. Congratulations. Thank you, and see you next year.“

---

## 8. Vergleich mit den offiziellen ITF-Ansagen – Verbesserungsvorschläge

Vorbild: **ITF Duties and Procedures for Officials** (Abschnitt „Announcing“, Ausgaben 2019 und 2026) und
**ITF Rules of Tennis**. Die PDF-Dateien ließen sich aus der Cloud nicht direkt öffnen (Netzsperre); die Aussagen
stützen sich auf Textauszüge aus der Websuche. Was dort nicht eindeutig bestätigt war, ist mit **[PRÜFEN]** markiert.

**Was bereits mit der ITF übereinstimmt:**

* Punktstand mit den Punkten des **Aufschlägers zuerst** – außer im Tiebreak (ITF: „The server’s score is always
  called first, except in the tie-break“).
* „Love“ für null Punkte, „all“ bei Gleichstand, „Deuce“, „Advantage <Name>“.
* **Tiebreak:** Punkte des **Führenden zuerst**, dann sein Name; „**zero**“ statt „love“; Gleichstand „one all“
  (ITF: „1-0 Jones“, „1-All“, „2-1 Smith“; „Use ‚Zero‘ rather than ‚Love‘ in the tie-break“).
* **Tiebreak-Beginn:** „Game Holloway. Six games all. Tie-break.“ (ITF: „Game Smith, 6 games all. Tie-break“).
* **Gleichstand der Spiele:** „Four games all“ (ITF: „Game Smith, 3 games all, first set“).
* **Satzende:** „Game and first set Adler, six games to four“ (ITF: „Game and third set Smith, 7 games to 5“).
* **Matchende:** „Game, set and match Adler, two sets to one, six four, three six, seven six“ (Aufbau wie in den
  ITF/USTA-Beispielen: Sätze, dann die Satzergebnisse aus Sicht des Siegers).
* **Challenge:** „… is challenging the call on the left baseline“ – die ITF verlangt ausdrücklich, die **genaue
  Linie** zu nennen.
* **Überstimmung:** „Correction, the ball was good“ ist der ITF-Wortlaut, wenn ein „Out“ falsch war.
* **Zeitverstoß:** „Time violation, warning, <Name>“; Verwarnung: „Code violation, <Verstoß>, warning, <Name>“ – die
  Bausteine dafür gibt es schon.

**Abweichungen und Verbesserungsvorschläge** (alle nur VORSCHLAG – CSV und Code bleiben unverändert):

| Nr | Situation | heute | ITF bzw. Tour-Praxis | Vorschlag | Aufwand |
|---|---|---|---|---|---|
| V1 | Spielstand nach jedem Spiel | „Game Adler. Adler leads three games to two“ | ITF nennt zusätzlich den Satz: „Game Smith, 3 games all, **first set**“ (ältere Ausgabe: entfällt, wenn eine Anzeigetafel sichtbar ist) | optional „…, first set“ anhängen (Bausteine `first_set` … `fifth_set` gibt es schon); da die TV-Einblendung den Satz zeigt, **Standard aus**, Option „ausführliche Ansagen“ | nur Code |
| V2 | Satzstand nach einem Satz | „Game and second set Holloway, six games to three“ | ITF: „Game and third set Smith, 7 games to 5. **Jones leads 2 sets to 1**“ | ab dem 2. Satz den Satzstand anhängen: „One set all“ / „Adler leads two sets to one“ – neue Bausteine `set_to`, `set_all`, `sets_all` (7.4); nicht nach dem Match | CSV + Code |
| V3 ✅ umgesetzt | 0 Spiele in den Satzergebnissen der Match-Ansage | „… six **zero**, seven five“ | gesprochen „six **love**“ (0 Spiele heißen „love“; „zero“ nur im Tiebreak) **[PRÜFEN]** | in der Satzliste der Match-Ansage `love` statt `n0` (im Match-Tiebreak bleibt „zero“) – gleiche Logik wie schon bei „two games to love“ | nur Code |
| V4 | Challenge anfordern | „Adler is challenging the call on the left baseline“ | ITF: „Mr/Ms … is challenging the call on the base line (give specific line), **the ball was called IN/OUT**“ | „…, the ball was called out“ anhängen – neue Bausteine `the_ball_was_called_out`, `the_ball_was_called_in` (7.3) | CSV + Code |
| V5 | Anrede | nur Nachname | ITF-Beispiele: „Mr/Ms <Name>“ | **beibehalten** (auf der Tour ist der Nachname allein üblich, spart Bausteine); höchstens als Option | – |
| V6 | verbleibende Challenges | wird nicht angesagt | ITF: verbleibende Challenges möglichst ansagen, wenn sich die Zahl ändert | nach einer erfolglosen Challenge: „Adler has one challenge remaining“ – neue Bausteine `has`, `challenge_remaining`, `challenges_remaining`, `no_challenges_remaining` (7.3) | CSV + Code |
| V7 | erfolgreiche Challenge, Ball war doch aus | „Correction, the ball was out“ | ITF-Überstimmung eines falschen „gut“: einfach „**Out**“ bzw. beim Aufschlag „**Fault**“ | „Out.“ bzw. „Fault. Second serve.“ statt „Correction, the ball was out“ **[PRÜFEN]** – im Fernsehen hört man nach Hawk-Eye beides | nur Code |
| V8 | Wiederholung nach Challenge | „Replay the point. First serve.“ | Rules of Tennis: „First **service**. The whole point must be replayed.“ | Wortlaut ist verständlich und auf der Tour verbreitet; wer es offizieller mag: Text von `first_serve`/`second_serve` auf „first service“/„second service“ ändern (nur CSV-Text, IDs bleiben) **[PRÜFEN]** | nur CSV |
| V9 | Let beim Aufschlag | „Let“ | in der Praxis oft „Let, first serve“ bzw. „Let, second serve“ **[PRÜFEN]** | beim Let die Aufschlagnummer anhängen (Bausteine vorhanden) | nur Code |
| V10 | No-Ad bei 40:40 | „Deuce deciding point“ | „Deuce, deciding point“ bzw. nur „Deciding point“ **[PRÜFEN]** | Komma-Pause einfügen: `deuce , deciding_point` | nur Code |
| V11 | Einspielzeit | ID `warm_up_time` mit Text „two minutes“ | ITF-Countdown: „Three minutes“, „Two minutes“, „One minute“, dann „Time“ | ID in `two_minutes` umbenennen (wird noch nirgends benutzt), optional `one_minute` (7.1) | nur CSV |
| V12 | Verwarnungen | nur `code_violation`, `time_violation`, `warning` | ITF: „Code violation, **delay of game**, warning, <Name>“; zweite Stufe Punktstrafe | `delay_of_game`, `point_penalty` ergänzen, wenn die 25-Sekunden-Uhr (README 5.5) kommt (7.5) | CSV + Code |
| V13 | Aufgabe | `retired`, `wins_by_retirement` (unbenutzt) | übliche Ansage: „<Name> has retired. <Name> wins by retirement“ **[PRÜFEN]** | `has_retired` ergänzen (7.5); bei „Aufgeben“ online (README 15.2) verwenden | CSV + Code |
| V14 | Vorstellung und Spielbeginn | `players_ready`, `play` (unbenutzt) | ITF: Vorstellung nach der „Two minutes“-Ansage: Spieler links/rechts vom Stuhl, Ergebnis der Platzwahl | Bausteine aus 7.2 | CSV + Code |
| V15 | Rückfall ohne Namen nach einem Spiel | „Game. Three games to two“ | README 16.2 nennt als Rückfall nur „Game“ | bei ungleichem Spielstand nur „Game“ sagen; bei Gleichstand „Game. Four games all“ behalten (eindeutig) | nur Code |
| V16 | Rückfall ohne Namen bei der Challenge | „Is challenging the call on the left baseline“ | – | „Challenge on the left baseline“ (`challenge_on_the`, 7.3) | CSV + Code |
| V17 | Linienbezeichnung | links/rechts aus Sicht des Stuhls, „near/far sideline“ | ITF verlangt nur die **genaue Linie**; „left/right baseline“ aus Sicht des Stuhls ist Tour-Praxis | so lassen – Entscheidung **B7** bitte bestätigen | – |
| V18 ✅ behoben | **Fehler:** Satz bzw. Match mit „Spiele pro Satz = 1“ (README 8) | „Game and first set Adler, **one games** to love“ · „Game, set and match Adler, one games to love“ | „one game to love“ | in der Satz- und der 1-Satz-Match-Ansage `game_to` statt `games_to`, wenn der Sieger genau 1 Spiel hat (wie schon beim Spielstand nach einem Spiel) | nur Code |

**Nicht übernommen** (bewusst): „Not up“, „Foul shot“, „Touch“, „Through“, „Hindrance“ – Situationen, die das
Spiel nicht simuliert. „Double fault“ ist keine ITF-Ansage (der Linienrichter ruft „Fault“, danach folgt der Punktstand) –
so ist es auch umgesetzt.

---

## 9. Aussprache und ElevenLabs

### 9.1 Texte so schreiben, dass die Stimme sie richtig spricht

* **Zahlen immer als Wörter** („fifteen“, „twenty-one“) – so steht es schon in der CSV. Ziffern würden je nach
  Modell als „one five“ oder „fifteen“ gelesen.
* **„tie-break“** mit Bindestrich (britische/ITF-Schreibweise) – wird als „tie break“ gesprochen, nicht „tiebreak“
  in einem Wort mit falscher Betonung. **„centre“** in britischer Schreibweise passt zur Standard-Stimme „Alice“ (britisch).
* **„deuce“** wird britisch /djuːs/ gesprochen – beim Anhören prüfen, ob es nicht wie „juice“ oder „deuks“ klingt.
* **Satzzeichen im Text** steuern die Satzmelodie: „game, set and match“ und „new balls, please“ enthalten das Komma
  bewusst. Keine Abkürzungen („Mr“), keine Großbuchstaben-Wörter (werden buchstabiert).
* Ein Baustein wird **einzeln** gesprochen und endet deshalb meist mit fallender Melodie. Beim Zusammensetzen klingt
  „Fifteen. Love.“ dann leicht abgehackt. Gegenmittel (**VORSCHLAG**, beim Anhören entscheiden **[PRÜFEN]**):
  1. Die **häufigsten Ansagen als Ganzes** erzeugen: die 14 Punktstände im Spiel (z. B. `fifteen_love`,
     `thirty_all`; dazu gibt es schon `deuce`) – das ist der größte Teil aller Ansagen, und sie klingen dann völlig natürlich.
  2. Die API kennt die Felder **`previous_text`** und **`next_text`** für einen Zusammenhang vor bzw. nach dem Text
     **[PRÜFEN]** – damit spricht die Stimme z. B. „fifteen“ so, als käme noch etwas.
  3. **Stille am Anfang und Ende** jeder WAV beim Import auf ca. 20 ms kürzen (ElevenLabs liefert oft ein paar
     Zehntelsekunden Stille), die Pausen kommen dann nur noch aus `,` und `.` (Abschnitt 1).

### 9.2 Einstellungen und Ablauf

* `elevenlabs-stimme.ps1` nutzt Stimme **„Alice“** (`Xb7hH8MSUJpSbSDYk0k2`) und Modell **`eleven_multilingual_v2`**,
  Ausgabe PCM 16 Bit, 24 kHz, mono.
* **Alle Bausteine mit derselben Stimme, demselben Modell und denselben Einstellungen** erzeugen. Wer Stimme oder
  Modell wechselt, löscht **alle** WAVs in `SourceAssets/Audio/Umpire/` (auch die Namen) und erzeugt neu – sonst
  passen die Teile nicht zusammen.
* Das Skript schickt keine `voice_settings`; die Stimme läuft mit ihren Standardwerten. **VORSCHLAG:** eine eher
  hohe Stabilität (z. B. `stability` 0,6–0,7) für einen gleichmäßigen, sachlichen Schiedsrichterton **[PRÜFEN]**.
* **Aussprache-Tags** (Phoneme) unterstützen nur einzelne englische Modelle, nicht `eleven_multilingual_v2` **[PRÜFEN]**.
  Einfacher: eine **Lautschrift-Schreibung** als Text (siehe 9.3).
* **Zeichenbedarf:** heute 921 Zeichen (Bausteine) + 136 Zeichen (20 Nachnamen) ≈ **1 060 Zeichen**; alle Vorschläge aus
  Abschnitt 7 (Schiedsrichter) kommen auf ca. 550 weitere Zeichen, die Stadionsprecher-Liste mit vollen Namen, Ländern,
  Zahlen und Turniernamen auf ca. 3 000. Das passt in das monatliche Kontingent des kostenlosen Tarifs **[PRÜFEN]** (Bedingungen:
  `CREDITS.md`, Entscheidung B13).
* **Ablauf:** `ElevenLabs-Stimme erzeugen.bat` → Schlüssel eingeben (wird nicht gespeichert) → das Skript erzeugt
  nur fehlende Bausteine → `Tools\assets.ps1 -NurImport`. Vorher `elevenlabs-stimme.ps1 -Liste` zeigt, was fehlt.

**Hörprobe nach dem Erzeugen** (einmal alle Bausteine anhören):

- [ ] Zahlen: „thirteen“ vs. „thirty“, „fourteen“ vs. „forty“ klar unterscheidbar
- [ ] „love“, „deuce“, „advantage“ britisch und nicht fragend
- [ ] „tie-break“ und „match tie-break“ gleich betont
- [ ] „game, set and match“ mit kleiner Pause nach „game“
- [ ] keine abgeschnittenen Anfänge/Enden, kein Atmen, kein Hall
- [ ] Namen richtig betont (9.3)

### 9.3 Namen richtig aussprechen

Ein englischer Schiedsrichter spricht fremde Namen „international“, meist nahe am Original. Vorschlag zur Aussprache
und eine **Schreibhilfe**, falls ElevenLabs den Namen falsch spricht:

| Nachname | gewünschte Aussprache (Betonung groß) | Schreibhilfe für ElevenLabs |
|---|---|---|
| Adler | AHD-ler (deutsch) | Ahdler |
| Holloway | HOL-uh-way | – |
| Varela | va-REH-la | Vareyla |
| Brooks | BROOKS | – |
| Hruška | HROOSH-ka | Hrooshka |
| Arakawa | ah-rah-KAH-wah | Ara-kawa |
| Ferreira | feh-RAY-ra | Ferrayra |
| Lindqvist | LIND-kvist | Lindkvist |
| Castellano | kas-teh-YAH-no | Castelyano |
| Vermeulen | ver-MUH-len | Vermurlen |
| Falk | FAHLK (kurzes a wie „Falke“) | Fahlk |
| Moreau | mo-ROH | Mohroh |
| Marchetti | mar-KET-tee | Marketti |
| Thompson | TOM-son | Tomson |
| Bláhová | BLAH-ho-vah | Blahhovah |
| Tanabe | tah-NAH-beh | Tanah-beh |
| Zhou | JOH | Joe |
| Solberg | SOOL-berg | Soolberg |
| Ríos | REE-os | Reeos |
| Bennett | BEN-it | – |

**So setzt du eine Schreibhilfe heute ein** (ohne Änderung am Skript): `elevenlabs-stimme.ps1 -Namen "Hrooshka"`
erzeugt `name_hrooshka.wav`; diese Datei (und die `.meta.json`) in `name_hruska.wav` umbenennen und die alte löschen.
**VORSCHLAG:** eine optionale Spalte `Aussprache` in `spieler.csv`, die das Skript als Text benutzt, während der
Dateiname weiter aus dem Nachnamen entsteht (Abschnitt 10).

---

## 10. Offene Punkte für dich

1. **B7 Linien:** „left/right baseline“ aus Sicht des Stuhls und „near/far sideline“ – so lassen?
2. **Vorschläge V1–V18** (Abschnitt 8): welche sollen umgesetzt werden? **V18** (Grammatikfehler) und **V3** („six love“)
   sind als Fehlerbehebung bereits umgesetzt und getestet. Empfehlung für die übrigen: **V2** (Satzstand),
   **V4** („the ball was called out“), **V10** (Komma), **V11** (ID), **V15/V16** (Rückfälle).
3. **Neue Ansagen** (Abschnitt 7): Ruhe/Seitenwechsel (7.1) und Challenge-Ergänzungen (7.3) mit M4; Vorstellung (7.2),
   Regen (7.6) und Stadionsprecher (7.7) mit M8.
4. **Stadionsprecher:** zweite ElevenLabs-Stimme – welche? Dafür braucht `elevenlabs-stimme.ps1` einen Parameter für
   die CSV-Datei.
5. **Aussprache:** Spalte `Aussprache` in `spieler.csv` einführen (9.3)?
6. **Satzmelodie:** ganze Punktstand-Ansagen als eigene Bausteine erzeugen (9.1)? Entscheidung nach der ersten Hörprobe.

---

## 11. Quellen

* ITF Duties and Procedures for Officials 2026 – <https://www.itftennis.com/media/2509/duties-procedures-for-officials-2026.pdf>
  (Challenge-Ansage „… is challenging the call on the …, the ball was called IN/OUT“, verbleibende Challenges ansagen,
  „Game Smith, 3 games all, first set“, Satzende mit Satzstand, Countdown der Einspielzeit, Vorstellung „to the left of the chair“)
* ITF Duties and Procedures for Officials 2019 (Kopie des Kroatischen Tennisverbands) –
  <https://hts.hr/wp-content/uploads/2019/07/ITF_2019-Duties_and_Procedures_for_Officials_v2.pdf>
  (Tiebreak: Führender zuerst, „zero“ statt „love“; „Game Smith, 6 games all. Tie-break“; Aufschläger zuerst;
  „Correction, the ball was good“; Verwarnungen)
* ITF Rules of Tennis 2026 – <https://www.itftennis.com/media/7221/2026-rules-of-tennis-english.pdf>
  (Fall „First service. The whole point must be replayed.“)
* USTA Officiating Procedures and Techniques – <https://www.usta.com/content/dam/usta/pdfs/USTA-Officiating-Procedures-and-Techniques.pdf>
  (Tiebreak-Ansage, „Game, set, match …; two sets to love, 7-6, 6-2“, Vorstellung der Spieler)
* ElevenLabs API-Dokumentation (Text-to-Speech: `voice_settings`, `previous_text`/`next_text`, Aussprache-Tags) –
  <https://elevenlabs.io/docs> **[PRÜFEN]** – aus der Cloud nicht abrufbar, Angaben aus dem Gedächtnis.
