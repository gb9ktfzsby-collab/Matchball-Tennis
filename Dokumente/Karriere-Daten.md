# Karriere-Daten: Punkte, Preisgeld, Turnierkalender, Anlagen

Herleitung und Quellen aller Werte für den Karrieremodus (README 14.2–14.7, 12.4–12.6, 24.3).
**Stand:** 16.10.2026 · **Status:** Startwerte – in der Cloud geschrieben und mit einem Prüfskript geprüft,
im Spiel noch nicht ausprobiert (README 0.0a). Markierung **[PRÜFEN]** = Wert, der unsicher ist oder mit dir
abgestimmt werden muss; gesammelt in Abschnitt 9.

> **Vor einer Veröffentlichung ersetzen (README 22.3):** Die Daten enthalten **echte Turniernamen** und reale
> Preisgeld- und Punktetabellen. Das ist für dieses **private Projekt** erlaubt. Bevor das Spiel jemals
> veröffentlicht wird, müssen alle echten Turniernamen durch erfundene ersetzt werden – Einzelheiten in Abschnitt 10.

---

## 1. Überblick

| Datei (`Unreal/Data/`) | Inhalt | Zeilen | spätere Data Table (Vorschlag) |
|---|---|---|---|
| `punkte_atp.csv` | Ranglistenpunkte je Stufe und Feldgröße (Herren) | 16 | `FMBPunkteZeile` → `DT_PunkteAtp` |
| `punkte_wta.csv` | Ranglistenpunkte je Stufe und Feldgröße (Damen) | 14 | `FMBPunkteZeile` → `DT_PunkteWta` |
| `preisgeld.csv` | Gesamt-Richtwert (USD) und Anteile je Runde, beide Touren | 30 | `FMBPreisgeldZeile` → `DT_Preisgeld` |
| `turniere_atp.csv` | ATP-Kalender (Grand Slams, Masters 1000, 500, 250, Finals) | 60 | `FMBTurnierZeile` → `DT_TurniereAtp` |
| `turniere_wta.csv` | WTA-Kalender (Grand Slams, WTA 1000, 500, 250, Finals) | 53 | `FMBTurnierZeile` → `DT_TurniereWta` |
| `turniere_niedrig.csv` | Vorlagen für Challenger / WTA 125 / ITF (prozedural je Woche) | 74 | `FMBTurnierVorlageZeile` → `DT_TurniereNiedrig` |
| `anlagen.csv` | Anlagen und Varianten je Turnierstufe | 21 | `FMBAnlageZeile` → `DT_Anlagen` |

Die Zeilenstrukturen gibt es **noch nicht** in `Core/MBDatenTabellen.h`; sie kommen mit M7 (Karriere). Bis dahin
überspringt `Tools/Python/import_daten.py` diese Dateien mit einem Hinweis, und `Tools/pruefe_daten.py` prüft sie
nicht. Die Spaltennamen sind so gewählt, dass sie 1:1 als `UPROPERTY`-Namen taugen (wie bei den bestehenden
Tabellen, die `pruefe_daten.py` exakt gegen die Strukturen vergleicht).

**Zusammenhang der Tabellen:**

```
turniere_atp/wta.csv ── Wertung ──► punkte_atp/wta.csv (gleicher Zeilenname)
        │                   └─────► preisgeld.csv     (gleicher Zeilenname)
        └────────────── Anlage ───► anlagen.csv (Spalte Anlage; Variante je Runde, Abschnitt 7)
turniere_niedrig.csv ── Wertung / Anlage ── wie oben
```

**Gemeinsame Regeln aller Dateien**

* UTF-8 ohne BOM, LF, Komma als Trenner, **Dezimalpunkt**, keine Anführungszeichen. Erste Spalte `Name` =
  eindeutiger Zeilenname (nur `A–Z a–z 0–9 _`; Unreal unterscheidet keine Groß-/Kleinschreibung).
* Wahrheitswerte `True`/`False`, Spalten mit **`b`-Präfix** wie in `belaege.csv` (`bHalle`) – so heißen die
  `bool`-Felder später auch in C++ (Unreal-Stil, `Dokumente/Architektur.md`): `bHalle` (Halle ja/nein),
  `bNachtsession` (Tageszeit Nacht), `bBaeume`, `bFahnen` usw.
* Farben als Text `#RRGGBB` (in C++ als `FString`, Umwandlung mit `FColor::FromHex` – der Unreal-CSV-Import
  erwartet für `FColor`-Felder sonst die Form `(R=…,G=…,B=…)`).
* **Ländercodes:** dreibuchstabige **Tour-Codes (IOC)** wie auf Anzeigetafeln und wie in `spieler.csv`
  (`GER`, `NED`, `SUI` …), **nicht ISO-3166** – sonst würde ein Heimturnier (Wildcard, README 14.4) für deutsche
  Spieler nicht erkannt (`spieler.csv`: `GER`, ISO wäre `DEU`). Abweichungen zu ISO, die hier vorkommen:
  GER/DEU, NED/NLD, SUI/CHE, CHI/CHL, CRO/HRV, POR/PRT, UAE/ARE, MON/MCO, GRE/GRC, KSA/SAU, INA/IDN, TPE/TWN.
  **[PRÜFEN]** Der Auftrag nannte ISO3 – bitte bestätigen, dass die Tour-Codes gewünscht sind.
* Listen in einem Feld: Einträge mit `;` getrennt, Stadt und Land mit `:` (`Bergamo:ITA;Brest:FRA`).
* Echte Turniernamen **ohne Sponsorennamen** („Washington Open“ statt Sponsortitel), Städte deutsch, wo üblich
  (München, Rom, Peking, Tokio).

---

## 2. Ranglistenpunkte (`punkte_atp.csv`, `punkte_wta.csv`)

### 2.1 Spalten

| Spalte | Bedeutung |
|---|---|
| `Stufe` | `GrandSlam`, `Masters1000`/`WTA1000`, `ATP500`/`WTA500`, `ATP250`/`WTA250`, `Challenger175…50`, `WTA125`, `ITF_M25`/`ITF_M15`, `ITF_W100…W15`, `Finals` |
| `Feld` | Teilnehmer im Hauptfeld (28, 32, 48, 56, 96, 128, Finals 8). Das Tableau hat die nächste Zweierpotenz; die Lücke sind Freilose für die besten Gesetzten. |
| `Quali_Feld`, `Quali_Runden` | Größe und Rundenzahl der Qualifikation; Qualifikanten = `Quali_Feld / 2^Quali_Runden` |
| `S` | Turniersieg |
| `F`, `HF`, `VF`, `R16`, `R32`, `R64`, `R128` | **in dieser Runde ausgeschieden** (F = Finale verloren). Runden, die es bei der Feldgröße nicht gibt, sind 0. |
| `Q` | Qualifikation überstanden – **zusätzlich** zu den Hauptfeldpunkten |
| `Q3`, `Q2`, `Q1` | in Qualifikationsrunde 3/2/1 ausgeschieden (`Q3` nur bei 3 Runden) |
| `GruppenSieg` | nur Finals: Punkte je Sieg in der Gruppenphase |

### 2.2 Werte ATP (ab 2025/2026)

| Stufe / Feld | S | F | HF | VF | R16 | R32 | R64 | R128 | Q | Q3 | Q2 | Q1 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Grand Slam 128 | 2000 | 1300 | 800 | 400 | 200 | 100 | 50 | 10 | 30 | 16 | 8 | 0 |
| Masters 1000 – 96 | 1000 | 650 | 400 | 200 | 100 | 50 | 30 | 10 | 20 | – | 10 | 0 |
| Masters 1000 – 56 | 1000 | 650 | 400 | 200 | 100 | 50 | 10 | – | 30 | – | 16 | 0 |
| ATP 500 – 48 | 500 | 330 | 200 | 100 | 50 | 25 | 0 | – | 16 | – | 8 | 0 |
| ATP 500 – 32 | 500 | 330 | 200 | 100 | 50 | 0 | – | – | 25 | – | 13 | 0 |
| ATP 250 – 48 **[PRÜFEN]** | 250 | 165 | 100 | 50 | 25 | 13 | 0 | – | 13 | – | 6 | 0 |
| ATP 250 – 28/32 | 250 | 165 | 100 | 50 | 25 | 0 | – | – | 13 | – | 6 | 0 |
| Challenger 175 | 175 | 100 | 60 | 32 | 15 | 0 | – | – | 6 | – | 3 | 0 |
| Challenger 125 | 125 | 75 | 45 | 25 | 13 | 0 | – | – | 5 | – | 3 | 0 |
| Challenger 100 | 100 | 60 | 35 | 18 | 8 | 0 | – | – | 5 | – | 2 | 0 |
| Challenger 75 | 75 | 44 | 26 | 14 | 6 | 0 | – | – | 4 | – | 2 | 0 |
| Challenger 50 | 50 | 30 | 17 | 9 | 4 | 0 | – | – | 3 | – | 1 | 0 |
| ITF M25 | 25 | 14 | 7 | 3 | 1 | 0 | – | – | 0 | – | 0 | 0 |
| ITF M15 | 15 | 8 | 4 | 2 | 1 | 0 | – | – | 0 | – | 0 | 0 |

* ATP 500 mit 48er-Feld: wer die zweite Runde (R32) erreicht und verliert, bekommt 25; beim 32er-Feld ist die
  erste Runde (R32) punktlos.
* **ATP 250 mit 48er-Feld** (Winston-Salem): R32 = 13 ist **abgeleitet** (wie beim 500er: halbe R16-Punkte),
  nicht aus der Tabelle **[PRÜFEN]**.
* Die ITF-Qualifikation der Herren bringt keine ATP-Punkte.

### 2.3 Werte WTA (2025/2026)

| Stufe / Feld | S | F | HF | VF | R16 | R32 | R64 | R128 | Q | Q3 | Q2 | Q1 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Grand Slam 128 | 2000 | 1300 | 780 | 430 | 240 | 130 | 70 | 10 | 40 | 30 | 20 | 2 |
| WTA 1000 – 96 | 1000 | 650 | 390 | 215 | 120 | 65 | 35 | 10 | 30 | – | 20 | 2 |
| WTA 1000 – 56 | 1000 | 650 | 390 | 215 | 120 | 65 | 10 | – | 30 | – | 20 | 2 |
| WTA 500 – 48 | 500 | 325 | 195 | 108 | 60 | 32 | 1 | – | 25 | – | 13 | 1 |
| WTA 500 – 28/32 **[PRÜFEN]** | 500 | 325 | 195 | 108 | 60 | 1 | – | – | 25 | – | 13 | 1 |
| WTA 250 | 250 | 163 | 98 | 54 | 30 | 1 | – | – | 18 | – | 12 | 1 |
| WTA 125 | 125 | 81 | 49 | 27 | 15 | 1 | – | – | 6 | – | 4 | 1 |
| ITF W100 | 100 | 65 | 39 | 21 | 12 | 1 | – | – | 5 | – | 3 | 0 |
| ITF W75 | 75 | 49 | 29 | 16 | 9 | 1 | – | – | 3 | – | 2 | 0 |
| ITF W50 | 50 | 33 | 20 | 11 | 6 | 1 | – | – | 2 | – | 1 | 0 |
| ITF W35 **[PRÜFEN]** | 35 | 23 | 14 | 8 | 4 | 0 | – | – | 1 | – | 0 | 0 |
| ITF W15 | 15 | 10 | 6 | 3 | 1 | 0 | – | – | 0 | – | 0 | 0 |

* WTA 1000 mit 56er-Feld: erste Runde (R64) = 10 statt 35. Die Quali-Punkte des 56er-Felds sind wie beim
  96er-Feld angesetzt **[PRÜFEN]**.
* WTA 500 mit 28/30/32er-Feld: Die Recherche nannte nur das 48er-Feld; die erste Runde (R32) ist hier mit
  1 Punkt angesetzt (wie bei WTA 250/125) **[PRÜFEN]**.
* ITF W35: Die Tabelle nennt nur fünf Werte (35/23/14/8/4) und Q 1 – die erste Runde ist mit 0 angesetzt
  **[PRÜFEN]**.

### 2.4 Sonderregeln

* **Qualifikanten** bekommen die Quali-Punkte `Q` **plus** die Punkte der erreichten Hauptfeldrunde
  (Grand Slam: Quali + erste Runde verloren = 30 + 10 = 40 ATP-Punkte).
* **Freilos:** Wer ein Freilos hat und sein erstes Match verliert, bekommt die Punkte der **ersten Runde**
  (Masters 96: 10 statt 30) – Regel aus den Regelwerken, wie sie üblich ist **[PRÜFEN]**.
* **Lucky Loser** (README 14.4): Quali-Punkte der verlorenen Runde plus Hauptfeldpunkte.
* **Finals** (ATP und WTA gleich): je Gruppensieg 200, Halbfinalsieg 400, Finalsieg 500, ungeschlagen 1500.
  Abgebildet als `GruppenSieg = 200`, `F = 400` (Halbfinale gewonnen), `S = 900` (Halbfinale + Finale gewonnen),
  `HF = 0`. **Gesamt = Anzahl Gruppensiege × 200 + K.-o.-Wert.** Beispiel: 2 Gruppensiege, Finale verloren
  = 2 × 200 + 400 = 800.

---

## 3. Weltrangliste – wie sie rechnet (README 14.6)

Umgesetzt im Kern (`Career/Kern/MBKarriereKern.h`, `FRanglistenRegeln`: `BesteErgebnisse = 18`, `Wochen = 52`,
`bFinalsZusaetzlich = true`; Tests `Karriere.*`).

1. **Wertungswoche** eines Turniers = `Woche + Wochen − 1` (Woche des Turnierendes; die Rangliste erscheint am
   Montag danach). Im Kern ist das `FTurnierErgebnis::Woche` (fortlaufend seit Karrierebeginn:
   `(Saison − 1) × 52 + Wertungswoche`).
2. **52 Wochen rollierend:** In Woche W zählen alle Ergebnisse der Wochen W−51 … W. Ein Ergebnis fällt also genau
   ein Jahr später heraus – in der Praxis in der Woche, in der dasselbe Turnier wieder stattfindet.
3. **Beste 18 Ergebnisse** (ATP ab der Saison 2026, vorher 19; WTA seit 2024: 18) aus allen Stufen – auch
   Challenger, WTA 125 und ITF. Zählt ein Spieler mehr als 18 Turniere, fallen die schwächsten heraus.
4. **Finals zusätzlich** (als 19. Ergebnis), auch sie zählen 52 Wochen.
5. Punkte eines Turniers = Hauptfeld-Runde (Abschnitt 2) **+** Quali-Punkte, falls qualifiziert.
6. **Gleichstand:** höheres bestes Einzelergebnis, dann kleinere Spieler-Id.
7. **Race zu den Finals [NEU]:** gleiche Zählung, aber nur Ergebnisse des laufenden Kalenderjahrs (ab KW 1).
   Die besten 8 nach der letzten Turnierwoche vor den Finals (ATP: nach KW 46, WTA: nach KW 45) spielen die Finals
   (2 Gruppen à 4, README 14.6).

**Vereinfachung gegenüber der echten Tour:** Bei der ATP zählen in Wirklichkeit die 4 Grand Slams und die
Pflicht-Masters immer (verpasste Pflichtturniere als 0-Ergebnis), dazu die besten übrigen Ergebnisse; die WTA hat
ähnliche Pflichtturnier-Regeln. Im Spiel gilt standardmäßig „beste 18“; **Pflichtturniere für Top-Spieler** sind
laut README 14.6 eine **[NEU]-Option** und wären eine Erweiterung von `FRanglistenRegeln`.

**Beispiel:** Spielerin mit 22 Ergebnissen in den letzten 52 Wochen (u. a. WTA-250-Titel 250, zwei W75-Titel je 75,
viele Erstrundenniederlagen à 1) → es zählen die 18 besten; vier Ergebnisse à 1 Punkt fallen weg.

**Startpunkt einer neuen Karriere:** ohne Ranglistenpunkte → Zulassung nur über ITF-Qualifikationen
(bzw. Wildcards bei Heimturnieren, README 14.4).

**Typische Zulassungsgrenzen der echten Tour** (Kontrollwerte für die Hintergrund-Simulation, keine Daten in den
CSVs, **[PRÜFEN]**, nur Größenordnung):

| Stufe | Hauptfeld bis etwa Rang | Qualifikation bis etwa Rang |
|---|---|---|
| Grand Slam | 100–105 | 230–250 |
| Masters/WTA 1000 (96) | 70–80 | 150–200 |
| 500 | 30–60 | 60–120 |
| 250 | 50–100 | 100–250 |
| Challenger 125–175 / WTA 125 | 100–200 | 200–350 |
| Challenger 50–100 | 200–400 | 350–700 |
| ITF M25 / W35–W100 | 300–700 | offen |
| ITF M15 / W15 | 500–1500 | offen |

Mit nur ca. 500 simulierten Spielern je Tour (README 14.6) werden die unteren Felder vom Generator mit
prozeduralen Spielern ohne Rangliste aufgefüllt (README 9.1).

---

## 4. Preisgeld (`preisgeld.csv`)

### 4.1 Bedeutung

* `Gesamt_USD` = **Richtwert** für das **Einzel-Preisgeld eines Feldes** (Hauptfeld + Qualifikation) in US-Dollar.
  Das Doppel ist nicht enthalten (gibt es im Spiel nicht, README 23.8). Echte Turniere geben meist das
  Gesamtpreisgeld inkl. Doppel an; das Einzel macht davon ca. **78–80 %** aus (Monte-Carlo 2026: 80 %,
  Hongkong 2026: 78 %).
* Spalten `S` … `R128`, `Q3` … `Q1` = **Anteil am Einzel-Preisgeld je Spieler**, der in dieser Runde ausscheidet
  (bzw. gewinnt). Betrag = `Preisgeld_USD des Turniers × Anteil`.
* **Nicht kumulativ:** Qualifikanten bekommen nur den Betrag der erreichten Hauptfeldrunde (wie auf der Tour).
* **Finals:** Betrag = `Teilnahme + Gruppensiege × GruppenSieg + K.-o.-Anteil` (`S` = Halbfinal- und Finalsieg,
  `F` = Halbfinalsieg, `HF` = 0) – jeweils × Gesamt.
* **Summenprobe** (im Prüfskript): Σ Anteil × Anzahl Spieler, die in der Runde ausscheiden = 1.
  Anzahl Verlierer der ersten Runde = `Feld − Raster/2` (Freilose!), in jeder weiteren Runde `Raster/2^r`.
  Größte Abweichung von 1: 2,7 · 10⁻⁶ (Rundung auf 7 Stellen).
* `Preisgeld_USD` im Kalender ist je Turnier eingetragen; bei unbekannten Turnieren steht der Richtwert der Stufe.

**Umrechnung** (gerundete Kurse 2026, **[PRÜFEN]**): 1 AUD = 0,66 USD · 1 EUR = 1,16 USD · 1 GBP = 1,34 USD.

### 4.2 Herleitung je Zeile

Die Anteile sind **Referenzbetrag der Runde / Summe aller Beträge des Feldes**. Referenzbeträge (je Spieler):

| Zeile | Vorbild | S | F | HF | VF | R16 | R32 | R64 | R128 | Q3 | Q2 | Q1 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `*_GS_128` | Australian Open 2026 (AUD) | 4 150 000 | 2 150 000 | 1 250 000 | 750 000 | 480 000 | 327 750 | 225 000 | 150 000 | 83 500 | 57 000 | 40 500 |
| `ATP_M1000_96` | Cincinnati 2026 Herren (USD) | 1 151 380 | 612 340 | 340 190 | 193 645 | 105 720 | 61 865 | 36 110 | 24 335 | – | 12 470¹ | 6 530¹ |
| `ATP_M1000_56` | Monte-Carlo 2026 (EUR) | 974 370 | 532 120 | 290 960 | 158 700 | 84 890 | 45 520 | 25 220 | – | – | 12 920 | 6 765 |
| `ATP_500_32` | Peking 2026 (USD) | 764 750 | 411 480 | 219 925 | 112 035 | 59 805 | 31 895 | – | – | – | 16 330¹ | 8 550¹ |
| `ATP_500_48` | Verhältnis zum Sieger² | 1 | 0,538 | 0,288 | 0,1465 | 0,0782 | 0,0429 | 0,0233 | – | – | 0,0120 | 0,0063 |
| `ATP_250_28/32/48`, `ATP_CH*` | Hongkong 2026 (USD) | 106 460 | 62 115 | 36 520 | 21 155 | 12 285 | 7 510 | 4 130³ | – | – | 3 755 | 2 050 |
| `ITF_*` | ITF-Standard ($15 000) | 2 160 | 1 272 | 753 | 438 | 258 | 151 | – | – | – | 0 | 0 |
| `WTA_1000_96` | Cincinnati 2026 Damen (USD) | 1 085 220 | 564 920 | 297 315 | 154 210 | 81 752 | 47 375 | 26 255 | 16 260 | – | 8 330¹ | 4 360¹ |
| `WTA_1000_56` | aus Cincinnati Damen⁴ | 1 085 220 | 564 920 | 297 315 | 154 210 | 81 752 | 47 375 | 16 260 | – | – | 8 330¹ | 4 360¹ |
| `WTA_500_*` | Verhältnis geschätzt **[PRÜFEN]** | 1 | 0,59 | 0,33 | 0,165 | 0,09 | 0,055 | 0,035 | – | – | 0,017 | 0,010 |
| `WTA_250_32` | Verhältnis geschätzt **[PRÜFEN]** | 1 | 0,596 | 0,356 | 0,201 | 0,122 | 0,086 | – | – | – | 0,050 | 0,030 |
| `WTA_125_32` | Verhältnis wie Challenger | 1 | 0,58 | 0,34 | 0,20 | 0,115 | 0,07 | – | – | – | 0,035 | 0,019 |
| `*_FINALS_8` | ATP Finals 2025 (USD)⁵ | 3 550 500 | 1 183 500 | 0 | Teilnahme 331 000, je Gruppensieg 396 500 | | | | | | | |

¹ Quali-Beträge nicht veröffentlicht gefunden → wie bei Monte-Carlo: letzte Quali-Runde = 0,512 × erste
Hauptrunde, erste Quali-Runde = 0,268 × erste Hauptrunde **[PRÜFEN]**.
² Washington 2026 nannte Sieger 375 500 und Finale 202 000 USD (Quellen widersprüchlich); übrige Runden wie beim
32er-Feld, erste Runde (R64) ≈ 0,54 × R32 **[PRÜFEN]**.
³ R64 nur beim 48er-Feld (Winston-Salem), geschätzt als 0,55 × R32.
⁴ Erste Runde des 56er-Felds bekommt den Betrag der ersten Runde des 96er-Felds (so ist es auch bei Monte-Carlo
gegenüber Cincinnati).
⁵ ATP Finals 2025: Teilnahme 331 000, Gruppensieg 396 500, Halbfinalsieg 1 183 500, Finalsieg 2 367 000 USD,
ungeschlagen 5 071 000 USD – die Aufteilung ist aus dem bekannten Höchstbetrag zurückgerechnet **[PRÜFEN]**.
Die WTA Finals bekommen dieselbe Struktur und dieselbe Summe **[PRÜFEN]**.

### 4.3 Gesamt-Richtwerte (Einzel je Feld, USD)

| Stufe | Richtwert | Herleitung |
|---|---|---|
| Grand Slam | 30 000 000 | AO 2026: 43,44 Mio. AUD Einzel je Geschlecht (Haupt 37,68 + Quali 5,75) ≈ **28,7 Mio. USD**; RG 2025 ≈ 23,8 Mio. EUR ≈ 26,9 Mio. USD; Wimbledon 2025 ≈ 22,0 Mio. GBP ≈ 29,7 Mio. USD; US Open 2025 ≈ 35,6 Mio. USD (RG/Wimbledon/USO aus Rundenbeträgen 2025 geschätzt **[PRÜFEN]**). Gesamtpreisgeld AO 2026: 111,5 Mio. AUD. Die in der Vorab-Recherche genannte Summe „49,2 Mio. AUD“ lässt sich aus den Rundenbeträgen nicht nachrechnen **[PRÜFEN]**. |
| Masters 1000 (96) | 7 300 000 | Cincinnati 2026: 9 415 725 USD gesamt, Einzel = 6 988 520 Hauptfeld + ca. 306 000 Quali = 7,29 Mio. |
| Masters 1000 (56) | 5 700 000 | Monte-Carlo 2026: 4 921 080 EUR Einzel (von 6 309 095) × 1,16 |
| ATP 500 (32) | 2 300 000 | typischer 500er ca. 2,9 Mio. gesamt × 0,78; Spanne Rio 1,95 – Peking 3,19 Mio. |
| ATP 500 (48) | 2 400 000 | Barcelona/Washington |
| ATP 250 (28/32/48) | 546 000 / 576 000 / 642 000 | Hongkong/Delray 2026: 700 045 USD gesamt → 546 055 Einzel (28er); 32er + 4 × R32; 48er zusätzlich R64 |
| Challenger 175/125/100/75/50 | 234 000 / 175 500 / 138 000 / 83 500 / 49 100 | Kategorie-Summen 2026: ca. 300 000 **[PRÜFEN]** / 225 000 / 177 000 / 107 000 / 63 000 USD × 0,78 |
| ITF M25 / M15 | 22 350 / 11 170 | 30 000 / 15 000 USD × 0,745 (Einzelanteil der ITF-Standardverteilung) **[PRÜFEN: M25 = 30 000?]** |
| WTA 1000 (96) | 6 000 000 | Cincinnati 2026 Damen: 5 634 106 Hauptfeld + ca. 204 000 Quali = 5,84 Mio.; Indian Wells, Miami, Madrid zahlen gleich wie die Herren (7,3 Mio. im Kalender) |
| WTA 1000 (56) | 2 900 000 | Doha/Dubai/Wuhan ca. 3,65 Mio. gesamt × 0,78 |
| WTA 500 (48/32/28) | 900 000 / 770 000 / 750 000 | WTA-500-Mindestpreisgeld 2025: 1 064 510 USD; Einzelanteil ca. 0,7 **[PRÜFEN]** |
| WTA 250 | 221 000 | WTA-250-Mindestpreisgeld 2025: 275 094 USD (2026 ca. 283 000) × 0,78 |
| WTA 125 | 97 500 | ca. 125 000 USD × 0,78 **[PRÜFEN]** |
| ITF W100/W75/W50/W35/W15 | 74 450 / 44 700 / 29 800 / 22 350 / 11 170 | 100 000 / 60 000 / 40 000 / 30 000 / 15 000 USD × 0,745 **[PRÜFEN]** (die W-Stufen sind nach Punkten benannt, nicht nach Preisgeld) |
| Finals | 12 140 000 | 8 × 331 000 + 12 × 396 500 + 2 × 1 183 500 + 2 367 000 |

### 4.4 Ergebnis (Beispielbeträge mit dem Richtwert)

| Zeile | Gesamt USD | Sieger | Finale | erste Runde |
|---|---|---|---|---|
| `ATP_GS_128` | 30.000.000 | 2.866.287 | 1.484.943 | 103.602 (R128) |
| `ATP_M1000_96` | 7.300.000 | 1.152.188 | 612.770 | 24.352 (R128) |
| `ATP_M1000_56` | 5.700.000 | 1.128.595 | 616.345 | 29.212 (R64) |
| `ATP_500_48` | 2.400.000 | 529.521 | 284.882 | 12.338 (R64) |
| `ATP_500_32` | 2.300.000 | 551.958 | 296.986 | 23.020 (R32) |
| `ATP_250_28` | 546.000 | 106.449 | 62.109 | 7.509 (R32) |
| `ATP_CH125_32` | 175.500 | 31.571 | 18.420 | 2.227 (R32) |
| `ATP_CH50_32` | 49.100 | 8.833 | 5.153 | 623 (R32) |
| `ITF_M15_32` | 11.170 | 2.160 | 1.272 | 151 (R32) |
| `ATP_FINALS_8` | 12.140.000 | 3.550.500 + Gruppe | 1.183.499 + Gruppe | 331.000 (Teilnahme) |
| `WTA_1000_96` | 6.000.000 | 1.115.199 | 580.526 | 16.709 (R128) |
| `WTA_1000_56` | 2.900.000 | 655.618 | 341.287 | 9.823 (R64) |
| `WTA_500_28` | 750.000 | 166.223 | 98.072 | 9.142 (R32) |
| `WTA_250_32` | 221.000 | 36.088 | 21.508 | 3.104 (R32) |
| `WTA_125_32` | 97.500 | 18.082 | 10.488 | 1.266 (R32) |
| `ITF_W100_32` | 74.450 | 14.397 | 8.478 | 1.006 (R32) |

Zum Vergleich: Ausgaben (Reisekosten, Trainer, README 14.7) müssen so gewählt werden, dass ITF-Turniere
(Sieg 2 000–4 000 USD) knapp kostendeckend sind – sonst ist die ITF-Phase kein Planungsspiel.

---

## 5. Turnierkalender (`turniere_atp.csv`, `turniere_wta.csv`)

### 5.1 Spalten

| Spalte | Bedeutung |
|---|---|
| `Name` | Zeilenname, beginnt mit `ATP_`/`WTA_` |
| `Turnier`, `Stadt`, `Land` | Anzeigename (ohne Sponsor), Stadt, Tour-/IOC-Code |
| `Woche`, `Wochen` | Startwoche (Kalenderwoche 1–47) und Dauer im Spiel (1 oder 2) |
| `Datum` | erster Turniertag des Vorbilds 2026 (Anzeige, Saisonplanung) |
| `Stufe`, `Wertung` | Turnierstufe; Zeilenname in `punkte_*.csv` und `preisgeld.csv` |
| `Belag`, `bHalle` | `Hart`/`Sand`/`Rasen`/`Halle` (Halle = Hallen-Hartplatz, eigene Physik in `belaege.csv`); Hallen-Sand (Linz, Stuttgart, Rouen): `Sand` + `bHalle = True` |
| `Feld`, `Qualifikation`, `Gesetzte` | Teilnehmer Hauptfeld, Quali-Feld (0 = keine), Anzahl Gesetzte |
| `Anlage` | Anlage aus `anlagen.csv` (Variante je Runde → Abschnitt 7) |
| `Preisgeld_USD` | Einzel-Preisgeld des Turniers (Abschnitt 4) |
| `bNachtsession` | Abendsessions unter Flutlicht (README 12.7); in Hallen immer `False` (Licht ist gleich) |
| `CourtPace` | Ziel-Court-Pace-Rating für `FBelagParameter::SetzeCourtPace` (README 12.2, `Werte.md` 2.1) |
| `Belagfarbe`, `Auslauffarbe`, `Akzentfarbe` | eigenes Aussehen je Turnier (README 12.6), überschreiben die Anlagen-Farben; Akzent = Banden, Fahnen, Anzeigetafel-Rahmen |

### 5.2 Wochenraster

* **Woche = ISO-Kalenderwoche 2026** (KW 1 = 29.12.2025–4.1.2026). So liegt die Saison genau in KW 1–47:
  ATP Finals 15.–22.11. = KW 47.
* Woche = Kalenderwoche des ersten Hauptturniertags. Beginnt ein Turnier am **Wochenende davor** (Australian Open
  So 18.1., Monte-Carlo So 5.4., WTA Finals Sa 7.11.), zählt die folgende Woche.
* `Wochen = 2` für Grand Slams und die 12-Tage-Turniere (Indian Wells, Miami, Madrid, Rom, Cincinnati, Shanghai,
  Peking WTA). Monte-Carlo, Paris, Doha, Dubai und Wuhan dauern eine Woche.
* Pro Woche spielt ein Spieler höchstens ein Turnier (README 14.3). Das Prüfskript stellt sicher, dass sich keine
  zwei Großturniere einer Tour überschneiden und neben einem Grand Slam kein anderes Tour-Turnier läuft.
* Wochen ohne Tour-Turnier: ATP KW 1 (United Cup) und KW 38 (Davis Cup); WTA KW 1 (United Cup) und KW 47
  (Saisonende). Dort gibt es Challenger/ITF bzw. Training/Pause.

### 5.3 Saison-Abschnitte (README 14.3)

| KW | Abschnitt | ATP (Beispiele) | WTA (Beispiele) |
|---|---|---|---|
| 2–5 | Hartplatz-Start Australien | Brisbane, Hongkong, Adelaide, Auckland, **Australian Open** (4–5) | Brisbane, Auckland, Adelaide, Hobart, **Australian Open** |
| 6–9 | Hallen im Februar, Südamerika-Sand, Golf | Montpellier, Dallas, Rotterdam (Halle); Buenos Aires, Rio, Santiago (Sand); Doha, Dubai, Acapulco, Delray Beach | Ostrava (Halle), Abu Dhabi, **Doha**, **Dubai**, Mérida, Austin |
| 10–13 | Hartplatz-Frühjahr USA | **Indian Wells** (10–11), **Miami** (12–13) | **Indian Wells**, **Miami** |
| 14–23 | Sandsaison | Houston, Marrakesch, Bukarest, **Monte-Carlo**, Barcelona, München, **Madrid** (17–18), **Rom** (19–20), Hamburg, Genf, **Roland Garros** (22–23) | Charleston, Bogotá, Linz und Stuttgart (Halle, Sand), Rouen, **Madrid**, **Rom**, Straßburg, Rabat, **Roland Garros** |
| 24–28 | kurze Rasensaison | Stuttgart, ’s-Hertogenbosch, Halle, Queen’s, Mallorca, Eastbourne, **Wimbledon** (27–28) | Queen’s, ’s-Hertogenbosch, Berlin, Nottingham, Bad Homburg, Eastbourne, **Wimbledon** |
| 29–31 | Sand-Juli Europa, Beginn US-Sommer | Båstad, Gstaad, Umag, Kitzbühel, Estoril; Washington, Los Cabos | Iași, Hamburg; Prag, Washington |
| 32–37 | Hartplatz-Sommer Nordamerika | **Kanada** (32), **Cincinnati** (33–34), Winston-Salem, **US Open** (36–37) | **Kanada**, **Cincinnati**, Monterrey, Cleveland, **US Open** |
| 38–42 | Asien-Herbst | Chengdu, Hangzhou, Tokio, Peking, **Shanghai** (41–42) | Guadalajara, São Paulo, Singapur, Seoul, **Peking** (40–41), **Wuhan** (42) |
| 43–46 | Hallensaison Europa / Asien-Ende | Almaty, Brüssel, Lyon, Basel, Wien, **Paris** (45), Stockholm, Athen | Ningbo, Osaka, Tokio, Guangzhou, Chennai, Hongkong, Jiujiang |
| 46–47 | Finals | **ATP Finals Turin** (47) | **WTA Finals** (46) |

Anzahl: ATP 4 Grand Slams, 9 Masters 1000, 16 ATP 500, 30 ATP 250 (davon 1 Platzhalter), Finals.
WTA 4 Grand Slams, 10 WTA 1000, 17 WTA 500, 21 WTA 250, Finals. Kombinierte Turniere (z. B. Indian Wells,
Grand Slams) stehen in beiden Dateien mit eigener Zeile.

### 5.4 Unsichere Termine und Annahmen [PRÜFEN]

Quellen: ATP-Kalender 2026 (Ankündigung und PDF, Stand Dezember 2025), WTA-Ankündigung 2026 und Kalenderlisten
(Abschnitt 11). Die Seiten waren aus der Cloud nur über die Websuche auswertbar.

1. **Kanada (Montréal Herren / Toronto Damen) 2.–12.8.2026**: 12 Tage, endet am Mittwoch der KW 33, Cincinnati
   beginnt am 13.8. Im Spiel auf **KW 32 verdichtet (`Wochen = 1`)**, damit Cincinnati (KW 33–34) ohne
   Überschneidung folgt.
2. **ATP Athen (KW 46)**: Der ATP-Kalender 2026 nennt für 8.–14.11. nur einen unbenannten 250er-Platz neben
   Stockholm; Athen (2025 im November) ist ein Platzhalter.
3. **ATP Lyon (KW 43)**: Die Quellen sind widersprüchlich (Marseille wandert von Februar in den Oktober bzw. Lyon
   ersetzt Marseille). Metz findet 2026 nicht mehr statt; Córdoba und Atlanta stehen in keiner 2026er-Liste und
   fehlen deshalb.
4. **Dallas** ist ab 2026 ATP 500 (Halle); Feld 32 angenommen.
5. **Chengdu/Hangzhou** laut ATP-Kalender 23.–29.9. (Mittwoch bis Dienstag) – im Spiel KW 39.
6. **WTA Straßburg**: Quellen nennen 500 und 250; mit 500 ergibt sich die offizielle Zahl von 17 WTA-500-Turnieren.
7. **WTA Seoul** 2026 als 250 (2025: 500), **Singapur** als 500 in der Halle angenommen.
8. **WTA Hamburg** 20.–26.7. (eine Quelle: 27.7.–3.8.); **WTA Cleveland** (KW 35) für 2026 nicht bestätigt.
9. Offiziell gibt es 22 WTA-250-Turniere; in den Quellen fanden sich 21 – eines fehlt.
10. **WTA Finals 2026**: ursprünglich Riad 7.–14.11.; laut mehreren Berichten nach Indian Wells (8.–15.11.)
    verlegt. Im Spiel steht Riad (Stand 2025) in der **Finals-Halle** (README 12.5 verlangt Finals in der Halle).
11. **Feldgrößen** einzelner 250/500-Turniere (28 oder 32, WTA 28/30/32) und **Quali-Felder** (16/24/32/48/128)
    sind typische Werte, nicht je Turnier recherchiert.
12. **Preisgelder einzelner Turniere** sind Richtwerte (Abschnitt 4); bekannt sind AO 2026, Cincinnati 2026,
    Monte-Carlo 2026, Peking 2026, Dubai/Doha 2026 (Sieger), die ATP-250-Standardsumme 2026.
13. **`Datum`**: erster Turniertag laut Kalendern; ±1 Tag möglich.
14. **`CourtPace`**: Startwerte nach dem Ruf der Plätze (Indian Wells langsam 33, Cincinnati 44, Shanghai 45,
    Paris 47, Halle 53; Sand 19–27, die höher gelegenen Orte Madrid/Gstaad/Kitzbühel/Bogotá etwas schneller).
    Eigentlich wirkt die **Höhenlage** über die Luftdichte auf den Ballflug – **[NEU-Vorschlag]** eine Spalte
    `Hoehe_m` je Turnier und Luftdichte `1,204 · e^(−h/8400)` in der Physik.
15. **Linienrichter:** Auf der echten Tour 2026 entscheiden fast überall elektronische Linienrichter (Ausnahme
    Roland Garros mit Ballmarken). Im Spiel bleibt laut README 7.1/7.2 der Linienrichter mit Challenge Standard;
    eine Spalte je Turnier („Elektronisch“) wäre leicht nachzurüsten.

### 5.5 Weitere Festlegungen

* **Gewinnsätze** stehen nicht in der Tabelle: Grand Slams Herren 3, sonst 2 (README 14.5).
* **Gesetzte/Freilose:** 28er-Feld 8 Gesetzte, 4 Freilose; 48er 16/16; 56er 16/8; 96er 32/32; 128er 32/0.
  Freilose bekommen die besten Gesetzten (`MB::Auslosung`).
* **Anlagen im Tour-Kalender:** Hartplatz draußen → `Arena`, Sand → `CourtCentral`, Rasen → `CentreCourt`,
  Halle (auch Hallen-Sand) → `ArenaIndoor`, Finals → `Finals`.

---

## 6. Prozedurale Turniere (`turniere_niedrig.csv`)

Challenger, WTA 125 und ITF-Turniere werden **nicht einzeln** gelistet, sondern je Woche aus Vorlagen erzeugt.

| Spalte | Bedeutung |
|---|---|
| `Tour`, `Stufe`, `Wertung` | wie im Kalender |
| `Abschnitt`, `Woche_Von`, `Woche_Bis` | Saison-Abschnitt und Wochenbereich, in dem die Vorlage gilt |
| `Belag`, `bHalle`, `Kontinent` | Belag und Ort (Kontinent für Reisekosten, README 14.7: Europa, Nordamerika, Suedamerika, Asien, Ozeanien, Afrika) |
| `Feld`, `Qualifikation`, `Gesetzte` | Challenger 32/24/8, WTA 125 32/16/8, ITF 32/32/8 |
| `AnzahlProWoche` | so viele Turniere erzeugt die Vorlage in jeder Woche ihres Bereichs |
| `Anlage`, `bNachtsession` | Anlage (Variante nach Stufe, Abschnitt 7); keine Nachtsessions |
| `Staedte` | mögliche Austragungsorte `Stadt:LAND;…` |

**Erzeugungsregel (Vorschlag für M7):** Für jede Woche w und jede Vorlage mit `Woche_Von ≤ w ≤ Woche_Bis`
entstehen `AnzahlProWoche` Turniere. Die Stadt wird mit dem Zufallsgenerator der Karriere (`MB::FZufall`,
Startwert der Karriere) aus `Staedte` gezogen – in derselben Woche ohne Wiederholung; in aufeinanderfolgenden
Wochen darf eine Stadt wiederkommen (wie die echten ITF-Serien in Monastir oder Antalya). Name =
„Stadt + Stufe“, z. B. „Bergamo Challenger 125“, „Monastir M15“, „Saint-Malo WTA 125“, „Wiesbaden W50“.
Preisgeld = `Gesamt_USD` der Wertungszeile. Belagfarbe: Standardfarbe der Anlagen-Variante; spielt eine Halle auf
Sand, gilt die Farbe von `CourtCentral` derselben Variante.

**Menge:** ATP 8–14 Turniere je Woche (Mittel 11,0), WTA 8–13 (Mittel 10,7). Die echte Tour hat je Woche etwa
3–6 Challenger und 10–15 ITF-Turniere der Herren. Für die Auswahl im Menü zeigt das Spiel nur Turniere, für die
der Spieler zugelassen ist, nahe Kontinente zuerst. Die Mengen sind Startwerte für die Bots-Simulation
(README 24.3) **[PRÜFEN]**.

**Belag-Abschnitte der Vorlagen:** Januar Hartplatz Ozeanien/Asien und Hallen Europa/Nordamerika; Februar–März
Sand Südamerika/Afrika, Hallen Europa; KW 14–23 Sand Europa/Südamerika, grüner Sand USA, Hartplatz Asien;
KW 23–26 Rasen England/Italien; Sommer Sand Europa, Hartplatz Nordamerika/Asien; Herbst Sand Europa/Südamerika,
Hartplatz Asien, Hallen Frankreich; KW 41–47 Hallen Europa/Nordamerika, Hartplatz Japan/Australien. Ganzjährig:
ITF in Monastir/Sharm El-Sheikh, Nordamerika, Südamerika, Asien.

---

## 7. Anlagen (`anlagen.csv`, README 12.4–12.6)

### 7.1 Anlagen und Varianten

| Anlage | Chunk / Ordner (`chunks.json`) | Varianten | Standardbelag | Merkmale (README) |
|---|---|---|---|---|
| `Arena` | 10 · `/Game/Venues/HartplatzArena` | ITF, Challenger, Tour, GrandSlam | Hart | blaue Schalensitze, Dachring, Flutlicht, Videowände |
| `CourtCentral` | 11 · `/Game/Venues/CourtCentral` | ITF, Challenger, Tour, GrandSlam | Sand | grüne Banden, helle Steintribünen, Fahnen, Bäume |
| `CentreCourt` | 12 · `/Game/Venues/CentreCourt` | ITF, Challenger, Tour, GrandSlam | Rasen | Efeu, Dach über den oberen Reihen, klassische Anzeigetafel |
| `Training` | 13 · `/Game/Venues/Training` | Standard | Hart (wählbar) | ohne Publikum, Zaun mit Windschutz, Flutlicht |
| `ArenaIndoor` | 14 · `/Game/Venues/ArenaIndoor` | ITF, Challenger, Tour, GrandSlam | Halle | dunkle Tribünen, Videowürfel |
| `Finals` | 15 · `/Game/Venues/Finals` | GrandSlam | Halle | dunkler Belag, verdunkelte Ränge |
| `Turnierhalle` | 16 · `/Game/Venues/Turnierhalle` | ITF, Challenger | Halle (auch Sand) | Klapptribünen, Holzwände |
| `Trainingshalle` | 17 · `/Game/Venues/Trainingshalle` | Standard | Halle | Traglufthalle, ohne Publikum |

Varianten nach README 12.6: **ITF** = Außenplatz, 2–5 Reihen auf einer Seite, Zaun, 0–200 Zuschauer;
**Challenger** = Nebenplatz, Tribüne auf 1–2 Seiten, 500–3 000; **Tour** = Hauptplatz, 4 Seiten, 5 000–15 000;
**GrandSlam** = große Arena/Centre Court, volle Ränge, Dach, ab 15 000.

### 7.2 Welche Variante bei welchem Match (Vorschlag) [PRÜFEN]

| Turnierstufe | Qualifikation | frühe Runden | späte Runden |
|---|---|---|---|
| ITF | ITF | ITF | ITF |
| Challenger / WTA 125 | ITF | Challenger | Challenger |
| 250 / 500 | ITF | Challenger (bis Achtelfinale) | Tour (ab Viertelfinale) |
| Masters 1000 / WTA 1000 | ITF | Challenger (bis R32), Tour (Achtelfinale) | GrandSlam (ab Viertelfinale) |
| Grand Slam | ITF | Tour (Runde 1–3) | GrandSlam (ab Achtelfinale) |
| Finals | – | GrandSlam (`Finals`) | GrandSlam (`Finals`) |

README 12.6 nennt „Grand Slam / Finals“ für die große Arena; dass auch die späten Runden der 1000er dort spielen,
ist ein Vorschlag (die Stadien in Indian Wells, Madrid oder Shanghai fassen 12 000–16 000). **[NEU, optional]**
Hoch gesetzte Spieler spielen schon früher auf dem Hauptplatz. Die Zuschauerzahl im Match ergibt sich als
`Publikum_Min + (Publikum_Max − Publikum_Min) × Rundenfaktor` (Qualifikation fast leer, Finale voll, README 3.7).

### 7.3 Spalten

| Spalte | Werte |
|---|---|
| `Einsatz` | wofür die Variante gedacht ist (`ITF;Quali`, `Challenger;WTA125;TourFrueh`, `TourSpaet`, `GrandSlam;Finals`, `Training`) |
| `Belag`, `Belagfarbe`, `Auslauffarbe` | Standardbelag und -farben (Schnelles Match, Training, niedrige Turniere). Im Turnier gelten Belag und Farben aus dem Kalender. |
| `Tribuenenart` | `Keine`, `Holzbaenke`, `Stahlrohr`, `Klapptribuene`, `Steinstufen`, `Schalensitze` (Bausteine README 3.5) |
| `Reihen`, `Seiten` | Sitzreihen je Tribüne, Platzseiten mit Tribüne (1–4; **0 = keine Tribüne**, nur Training) |
| `Dach` | `Keins`, `Teil` (Dachring bzw. Dach über den oberen Reihen), `Voll` (schließbares Dach – bei Regen wird geschlossen, README 12.7), `Halle` |
| `Publikum_Min`, `Publikum_Max` | Zuschauerzahl nach README 12.6 |
| `Anzeigetafel` | `Keine`, `Kipptafel`, `Elektronisch`, `Klassisch`, `Videowand`, `Videowuerfel` |
| `bBaeume`, `bFahnen`, `bEfeu`, `bFlutlicht`, `bZaun` | Ausstattung |
| `Bandenfarbe` | Grundfarbe der Banden (Turnier-Akzentfarbe kommt dazu) |
| `Auslauf_Hinten`, `Auslauf_Seite` | Meter; ITF-Minimum 6,40/3,66 (README 24.1), Haupt- und Stadionplätze 8,23/4,57 (übliche Empfehlung für große Turniere **[PRÜFEN]**) |
| `Chunk` | Paket für Updates (README 19.4.1, `Data/chunks.json`) |
| `CourtPace` | Standard-Court-Pace der Anlage (0 = Belag-Standard aus `belaege.csv`) |

---

## 8. Prüfung

Ein Prüfskript (in der Cloud-Sitzung ausgeführt, noch nicht im Repository) hat alle sieben Dateien geprüft:

* UTF-8 ohne BOM, LF, keine Anführungszeichen, Spaltenzahl je Zeile, keine leeren Felder, eindeutige Zeilennamen
  (auch ohne Groß-/Kleinschreibung);
* Punkte: Runden passend zur Feldgröße, **monoton fallend** (S > F > HF > VF streng), Quali monoton,
  Siegerpunkte = Stufe (README 14.2), Finals ungeschlagen = 1500;
* Preisgeld: zu jeder Punkte-Zeile eine Preisgeld-Zeile mit gleichem Feld, Anteile monoton, **Summe = 1**;
* Kalender: Wochen 1–47, Datum passt zur Kalenderwoche, Belag gültig, Anlage vorhanden und passend
  (Halle ↔ Hallen-Anlage, Freiluft-Belag = Standardbelag der Anlage, Finals in der Finals-Halle), Feld/Quali passen
  zur Wertung, Gesetzte ≥ Freilose, Preisgeld 0,4–2,5 × Richtwert, keine Überschneidung von Großturnieren,
  nichts neben Grand Slams, Rasen nur KW 24–28, Hallen-Hartplatz nicht in KW 10–38, Finals im November,
  Farben `#RRGGBB`, Court Pace passend zum Belag, keine ISO-Ländercodes, die von `spieler.csv` abweichen;
* Vorlagen: Wochenbereich, Belag/Anlage/Halle, Kontinent, Städte `Stadt:LAND` ohne Doppelte, jede Woche hat
  niedrige Turniere;
* Anlagen: Varianten vollständig, Publikum nach README 12.6, Chunks nach README 19.4.1, Auslauf ≥ ITF-Minimum.

Ergebnis: **keine Fehler.** Gegenprobe mit absichtlich kaputten Kopien (Woche 48, Belag „Gras“, fehlende Anlage,
steigende Punkte, ISO-Code, falsche Spaltenzahl, Turnier neben dem Grand Slam): alle erkannt.

**Vorschlag:** Die Prüfungen mit M7 in `Tools/pruefe_daten.py` übernehmen (sobald die Zeilenstrukturen existieren)
und dort in der CI laufen lassen.

---

## 9. Offene [PRÜFEN]-Punkte

1. Ländercodes: Tour-/IOC-Codes (wie `spieler.csv`) statt ISO3 – bitte bestätigen (Abschnitt 1).
2. Punkte: ATP 250 mit 48er-Feld (R32 = 13), WTA 500 mit 28/32er-Feld (R32 = 1), WTA 1000 mit 56er-Feld (Quali),
   ITF W35 (erste Runde 0), Freilos-Regel (Abschnitt 2).
3. Pflichtturniere (echte ATP/WTA-Regel) als Option – oder bleibt es bei „beste 18“? (Abschnitt 3)
4. Preisgeld: Quali-Beträge der 1000er/500er, ATP 500 mit 48er-Feld, alle WTA-500/250/125-Verhältnisse,
   Challenger-175-Summe, ITF-Summen, Finals-Aufteilung, WTA-Finals-Summe, Wechselkurse, die AO-Summe
   „49,2 Mio. AUD“ aus der Vorab-Recherche (Abschnitt 4).
5. Termine: Kanada verdichtet, Athen, Lyon, Straßburg, Seoul/Singapur, WTA Hamburg, Cleveland, fehlendes
   WTA-250, WTA-Finals-Ort, Feld- und Quali-Größen (Abschnitt 5.4).
6. Court Pace je Turnier und die Idee „Höhenlage → Luftdichte“ (Abschnitt 5.4, Punkt 14).
7. Variante je Runde (Abschnitt 7.2), Auslauf großer Plätze 8,23/4,57 m.
8. Anzahl niedriger Turniere je Woche (Abschnitt 6) – nach der Bots-Simulation nachstellen.
9. Ausgaben (Reisekosten, Trainer) gegen die ITF-Preisgelder abstimmen (Abschnitt 4.4).

---

## 10. Vor einer Veröffentlichung ersetzen (README 22.3)

Für die private Nutzung bleibt alles, wie es ist. Vor einer Veröffentlichung:

* **`turniere_atp.csv`, `turniere_wta.csv`, Spalte `Turnier`:** alle echten Turniernamen (z. B. „Australian Open“,
  „Roland Garros“, „Wimbledon“, „US Open“, „Monte-Carlo Masters“, „Queen’s Club Championships“, „Swiss Indoors“,
  „ATP Finals“, „WTA Finals“) durch erfundene Namen ersetzen. Städte und Länder sind Ortsangaben und dürfen bleiben;
  ein eigener, erfundener Name je Turnier reicht.
* **Anzeige der Stufen:** „ATP“, „WTA“, „ITF“, „Masters 1000“, „Challenger“ sind Namen bzw. Marken der Verbände.
  Die internen Schlüssel (`ATP500`, `ATP_GS_128` …) dürfen bleiben, in Menüs und Einblendungen aber eigene
  Bezeichnungen zeigen (z. B. „Tour 500“, „Major“, „Profi-Serie 25“).
* **Akzentfarben**, die an Erkennungsfarben einzelner Turniere erinnern (z. B. Violett/Grün beim Rasen-Grand-Slam),
  neutraler wählen.
* Echte Punkte- und Preisgeldtabellen sind Zahlen/Fakten; sie können bleiben, sollten aber mit erfundenen
  Turniernamen nicht mehr 1:1 einem echten Turnier zuzuordnen sein.

---

## 11. Quellen

Vorab-Recherche (übernommen): ATP Media Guide 2026 (Punkte, beste 18), ITF „WTA 2026 points table“,
openerarankings.com (Punkte und Ranglistenregeln), Punto de Break (ATP ab 2026 beste 18), GitHub Docs.

Websuche in dieser Sitzung (Direktabruf aus der Cloud gesperrt, ausgewertet über Suchergebnisse):

* ATP: [What is the 2026 ATP Tour calendar?](https://www.atptour.com/en/news/what-is-the-2026-atp-tour-calendar),
  [2026 ATP Tour calendar (PDF, Dezember 2025)](https://www.atptour.com/-/media/files/calendar-pdfs/2025/2026-atp-tour-calendar-december-2025.pdf),
  [2026 ATP Tour calendar announced (PDF)](https://www.atptour.com/-/media/sites/atp-tour/press/press-releases/2026-atp-tour-calendar-announced.pdf),
  [Tennis365: 2026 calendar changes](https://www.tennis365.com/tennis-features/2026-atp-tour-calendar-changes-longer-break-wimbledon-canada-monday-finish-cincy),
  [Punto de Break: ATP calendar 2026](https://www.puntodebreak.com/en/2025/02/03/this-will-be-the-atp-calendar-for-2026),
  [Tennisnerd: Highlights 2026](https://www.tennisnerd.net/news/highlights-changes-and-important-dates-of-the-2026-atp-tour-season/62166),
  [Tennishead: Full ATP Tour 2026 schedule](https://tennishead.net/full-atp-tour-2026-schedule-tournaments-dates-reigning-champions-and-more/),
  [TicketSwap: ATP schedule 2026](https://www.ticketswap.com/magazine/guides/atp-schedule).
* WTA: [WTA announces 2026 Tour calendar](https://www.wtatennis.com/news/4377748/wta-announces-2026-tour-calendar),
  [WTA-Kalender (PDF)](https://wtafiles.wtatennis.com/pdf/calendar/calendar.pdf),
  [Too Many Rackets: 2026 WTA schedule](https://toomanyrackets.com/2026-wta-tour-schedule/),
  [Serve and Volley: WTA calendar 2026](https://serveandvolleyart.com/wta-calendar/),
  [ESPN: WTA schedule 2026](https://www.espn.co.uk/tennis/schedule/_/type/wta),
  [Wikipedia: 2026 WTA Finals](https://en.wikipedia.org/wiki/2026_WTA_Finals).
* Preisgeld: [ATP: Australian Open 2026 prize money](https://www.atptour.com/en/news/australian-open-2026-prize-money),
  [NBC: prize money per round AO 2026](https://www.nbcnewyork.com/news/sports/tennis/prize-money-per-round-2026-australian-open),
  [ATP: Cincinnati 2026 prize money](https://www.atptour.com/en/news/cincinnati-2026-prize-money),
  [WTA: Cincinnati 2026 what to know](https://www.wtatennis.com/news/4556759/cincinnati-open-what-to-know-dates-draws-schedule-prize-money-and-more),
  [Monte-Carlo Masters (offiziell)](https://montecarlotennismasters.com/en/?p=94408),
  [Tennis Tour Calendar: Monte-Carlo 2026](https://tennistourcalendar.com/2026-rolex-monte-carlo-masters-prize-money/),
  [ATP: Beijing 2026 prize money](https://www.atptour.com/en/news/beijing-2026-prize-money),
  [Tennis Companion: Hong Kong](https://tenniscompanion.org/prize-money/hong-kong/),
  [Tennis Companion: Delray Beach](https://tenniscompanion.org/prize-money/delray-beach/),
  [Tennis Companion: Dubai](https://tenniscompanion.org/prize-money/dubai/),
  [Tennis Companion: Doha](https://tenniscompanion.org/prize-money/doha/),
  [Punto de Break: Challenger-Preisgeld 2026](https://www.puntodebreak.com/en/2025/11/24/the-challenger-circuit-will-have-significant-increase-in-prize-money-in-2026),
  [Tennis Tour Talk: Challenger 2026 $32.4 Mio.](https://www.tennistourtalk.com/127155/atp-challenger-tour-prize-money-rises-to-record-32-4-million-for-2026).
* Aus eigenem Wissen (Stand 2025, nicht neu geprüft, deshalb **[PRÜFEN]**): Rundenbeträge RG/Wimbledon/US Open
  2025, ATP-Finals-Aufteilung 2025, WTA-Mindestpreisgelder 2025, ITF-Standardverteilung, Platzauslauf-Empfehlungen.
