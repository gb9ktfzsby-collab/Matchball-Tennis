# Werte und ihre Herleitung

Dieses Dokument hält fest, **woher jeder Zahlenwert kommt** (README 0.1 Punkt 2 und 24.3).
Es gibt kein Altprojekt als Vorlage – alle Werte sind neu hergeleitet aus

1. den Vorgaben der README,
2. realen Tennisdaten (Regeln der ITF, Messungen aus der Fachliteratur, Tour-Statistiken),
3. Tests, die die Werte gegen Richtwerte prüfen (`Unreal/Source/MatchballTennisTests/Kern/`),
4. und später dem Probespielen bzw. der Bots-gegen-Bots-Simulation (README 24.3).

Die Werte selbst stehen **nicht im Code**, sondern in `Unreal/Data/*.csv` (in Unreal als Data Tables
importiert). Die Tests lesen genau dieselben CSV-Dateien – wird ein Wert geändert, prüfen die Tests
sofort, ob das Verhalten noch in den Richtwerten liegt.

Markierungen: **[PRÜFEN]** = Startwert, der lokal im Spiel bzw. mit dir abgestimmt werden muss.

---

## 1. Ball (`Data/ball.csv`)

| Größe | Wert | Herleitung |
|---|---|---|
| Masse | 57,7 g | Mitte der ITF-Vorgabe 56,0–59,4 g |
| Radius | 3,35 cm (Ø 6,70 cm) | Mitte der ITF-Vorgabe 6,54–6,86 cm |
| Luftdichte | 1,204 kg/m³ | Luft bei 20 °C, Meereshöhe |
| Widerstandsbeiwert c_W | 0,507 | Messung an neuen Bällen **im freien Flug** (Cross & Lindsey): 0,507 ± 0,024, unabhängig von Drall und Tempo. Windkanalwerte liegen 15–20 % höher (≈ 0,55–0,6), passen aber schlechter zum echten Flug. |
| Auftrieb (Magnus) | c_A = S / (1 + 2S), S = r·ω / v | Formel nach Štěpánek (1988) für Tennisbälle. Rechnerisch ohne Division durch ω umgeformt (deterministisch, siehe 6.). Bei S = 0,26 (Topspin 2500 U/min, 120 km/h) ist c_A ≈ 0,17. |
| Drall-Abnahme | 0,4 % pro geflogenem Meter | **Schätzung [PRÜFEN].** Aus der Literatur nicht sicher bekannt; mit einem Drehmomentbeiwert von ≈ 0,02 ergäben sich bei 30 m/s ca. 10–25 % pro Sekunde. Gewählt: ≈ 12 %/s bei 30 m/s, d. h. ein Grundschlag verliert über 20 m ca. 8 % Drall. |
| Trägheitsfaktor | I = 0,55 · m · r² | dünnwandige Hohlkugel (2/3) mit Filz und dicker Gummiwand – in der Literatur üblicher Wert 0,55 |
| Ballsteifigkeit | 30 kN/m (dynamisch) | so gewählt, dass die Kontaktzeit am Boden π·√(m/k) = **4,36 ms** beträgt – Messwerte für Tennisbälle liegen bei 4–5 ms |
| Schwerkraft | 9,81 m/s² | – |

**Prüfung (Test `Ballphysik.ItfFallversuch`):** ITF-Ballprüfung – aus 254 cm (Unterkante) auf Beton fallen
gelassen muss der Ball 135–147 cm hoch springen. Mit Luftwiderstand trifft der Ball mit 6,90 m/s auf
(ohne Luft 7,06 m/s); mit der Stoßzahl 0,77 für einen senkrechten Aufprall springt er **1,40 m** hoch. ✔

## 2. Beläge (`Data/belaege.csv`)

### 2.1 Grundlage: ITF Court Pace Rating

Die ITF klassifiziert Beläge über das **Court Pace Rating** CPR = 100·(1 − μ) + 150·(0,81 − e),
mit μ = Gleitreibungszahl und e = senkrechte Stoßzahl beim Prüfschuss (30 m/s unter 16°, also
8,27 m/s senkrecht). Kategorien: langsam ≤ 29, mittel-langsam 30–34, mittel 35–39, mittel-schnell 40–44, schnell ≥ 45.
Bänder der ITF: Stoßzahl hoch > 0,85, mittel 0,79–0,84, niedrig ≤ 0,78; Reibung hoch > 0,71, mittel 0,56–0,70, niedrig ≤ 0,55.

| Belag | μ | e (bei 8,27 m/s) | CPR | Kategorie | README 12.2 |
|---|---|---|---|---|---|
| Hart | 0,58 | 0,82 | 40,5 | mittel-schnell | „mittel–schnell, Absprung mittel–hoch" ✔ |
| Sand | 0,74 | 0,85 | 20,0 | langsam | „langsam, hoch" ✔ |
| Rasen | 0,56 | 0,76 | 51,5 | schnell | „schnell, tief" ✔ |
| Halle (Hart) | 0,55 | 0,80 | 46,5 | schnell | „schnell, mittel" ✔ |

**Stoßzahl abhängig vom Tempo:** e sinkt bei härterem Aufprall: e(v) = e_ref − 0,008·(v − 8,27 m/s),
begrenzt auf 0,30–0,95 **[PRÜFEN]** (übliche Größenordnung; ein Kick-Aufschlag mit 12 m/s senkrecht
springt dadurch etwas flacher ab).

**Rollen:** Rollreibung als konstante Verzögerung (Hart 0,6, Sand 1,2, Rasen 0,9, Halle 0,5 m/s²) –
nur für das Ausrollen nach dem Punkt **[PRÜFEN]**.

**Feineinstellung je Turnier [NEU]:** `FBelagParameter::SetzeCourtPace(Ziel)` stellt die Reibung so ein,
dass ein gewünschter CPR-Wert entsteht (z. B. „schneller Hartplatz" CPR 44, „langsamer Hartplatz" CPR 33).
Die Spalte `CourtPace` in `Data/anlagen.csv` nutzt das.

### 2.2 Absprung-Modell

Gleiten mit Reibung bis zum Übergang ins Rollen (Brody/Cross-Modell): Der Reibungsstoß ist höchstens
μ·(1 + e)·v_n; reicht er, um den Kontaktpunkt zu stoppen, rollt der Ball ab (dann hängt das Ergebnis
nicht mehr von μ ab). Drall verändert den Absprung so wie im echten Tennis:

* **Topspin** gleitet kürzer → behält mehr Vorwärtstempo, springt durch den steileren Einfall höher ab.
* **Slice** gleitet länger → wird stärker gebremst, kommt flacher an und bleibt tief.
* **Stopp** mit viel Rückwärtsdrall verliert fast das ganze Vorwärtstempo – ab ca. 4000 U/min springt er
  sogar zurück (Test `Ballphysik.StoppSpringtZurueck`: 3000 U/min → 6,6 auf 0,6 m/s; 4500 U/min → −1,2 m/s).

## 3. Netz (`Data/netz.csv`)

* Netzhöhe 0,914 m in der Mitte, 1,07 m an den Einzelstützen (0,914 m außerhalb der Einzelseitenlinie);
  dazwischen gerade (Mittelband zieht das Netz in der Mitte herunter), außen bis zu den Pfosten 1,07 m.
* **Netzband** als Zylinder mit 1 cm Radius (Drahtseil + umgeschlagenes Band, ITF: Band 5–6,35 cm
  je Seite); Stoßzahl 0,30 senkrecht zur Oberfläche, Tangentialtempo × 0,75, Drall × 0,7 →
  Netzroller fallen je nach Treffpunkt über das Netz oder zurück **[PRÜFEN]**.
* **Netzgeflecht:** weich – Ball verliert fast alles Tempo (Rückprall 10 %, seitlich/vertikal × 0,25)
  und fällt auf der eigenen Seite herunter.
* Die Ebene wird zeitgenau innerhalb des Schritts getroffen – auch ein 250-km/h-Ball tunnelt nicht durch
  (Test `Ballphysik.Netz`).

## 4. Simulation

* Fester Zeitschritt **240 Hz**, Runge-Kutta 4. Ordnung (README 6.1 „z. B. 240 Hz").
* Bodenkontakt, Netzband und Netzgeflecht werden **innerhalb** eines Schritts linear auf den genauen
  Zeitpunkt interpoliert (Fehler < 0,1 mm bei 240 Hz), dann geht der Schritt mit dem Rest weiter.
* **Determinismus** (README 6.1, wichtig für Online und Videobeweis): im Simulationspfad nur +, −, ×, ÷
  und √ (IEEE-754 exakt gerundet), keine sin/cos/exp/pow (die Windows-CRT wählt je nach CPU andere
  Rechenwege); Zufall nur über einen eigenen Ganzzahl-Generator (PCG32); unter MSVC
  `#pragma float_control(precise, on)` für die Simulationsdatei.
  **Geprüft:** Der Fingerabdruck einer Flugbahn ist mit GCC, Clang und einem Debug-Build mit
  Sanitizern bitgleich (`599a2d964081de7e`). Zwei PCs müssen außerdem dieselbe .exe haben – das stellt
  der Build-Vergleich beim Beitreten sicher (README 15.3).

## 5. Prüfung gegen Richtwerte (Meilenstein M1, „Ball fliegt/springt plausibel")

| Test | Ergebnis | Richtwert |
|---|---|---|
| ITF-Fallversuch | Rücksprung 1,40 m | 1,35–1,47 m |
| Aufschlag 200 km/h, flach | 144 km/h vor, 111 km/h nach dem Aufsprung; Aufsprung nach 0,39 s; Grundlinie nach 0,60 s | ca. 30 % Tempoverlust im Flug; Rückschläger hat ca. 0,5–0,7 s |
| Topspin-Grundschlag 120 km/h, 2500 U/min | Flugzeit 0,81 s, Netz 1,68 m; Absprunghöhe Sand 1,09 m > Hart 1,02 m > Rasen 0,88 m | Sand hoch, Rasen tief (README 12.2) |
| Flacher Ball 140 km/h nach Aufsprung | Sand 68 km/h < Rasen 75 km/h | Sand langsam, Rasen schnell |
| Drall bei gleichem Abschuss | Topspin landet bei 6,6 m, ohne Drall 11,2 m, Slice 17,1 m | Topspin taucht ab, Slice trägt |
| Seitendrall 2000 U/min | Ball zieht seitlich > 0,3 m | Slice-Aufschlag zieht nach außen |
| Ballmaschinen-/Schlagarten-Daten | alle 12 Schlagarten erreichen ihr Ziel bei Spielerwert 50, 99 und mit Power | Test `Schlag.SchlagartenAusDatenErreichbar` |

## 6. Schlagarten (`Data/schlagarten.csv`)

README 24.3: Aufschläge ca. 150–230 km/h, Grundschläge ca. 80–150 km/h. Drallwerte aus der Literatur:
Topspin-Grundschläge der Tour 2000–3500 U/min, Slice 1000–2000 U/min, Kick-Aufschlag 3000–4500 U/min.

| Schlag | Tempo bei Wert 50 → 99 (km/h) | Drall (U/min) | Ziel / Besonderheit |
|---|---|---|---|
| Flach | 100 → 145 | +800 | 9,5 m hinter dem Netz, Netzabstand ≥ 0,10 m |
| Topspin | 90 → 130 | +2600 | Netzabstand ≥ 0,40 m (sicherer) |
| Slice | 70 → 100 | −1600 | flach, schwebend |
| Lob (defensiv) | 66 → 70 | −900 | hoher Bogen (obere Lösung), Scheitel ca. 6–12 m |
| Lob offensiv (Lob + Power) | 70 → 90 | +3000 | untere Lösung, kreuzt das Netz ≥ 2,8 m darüber (über den Netzspieler) |
| Stopp | 40 → 48 | −3000 | 2 m hinter dem Netz. Unter 40 km/h erreicht ein Stopp aus 8 m Entfernung das Ziel physikalisch nicht (Test-Befund). |
| Volley | 70 → 110 | +300 | |
| Halbvolley | 60 → 90 | +800 | kleines Timing-Fenster |
| Schmetterball | 140 → 190 | +500 | immer flach und mit voller Wucht (README 5.7) |
| Aufschlag flach | 170 → 225 | +800 | Netzabstand ≥ 0,03 m |
| Aufschlag Kick | 140 → 185 | +3500 | |
| Aufschlag Slice | 150 → 195 | +500, seitlich +2000 | zieht nach links (Rechtshänder) |

Power: Tempo × 1,05–1,12; Präzision: × 0,90–0,95. Ist der Ball für das Netz zu flach, senkt die
Schlagberechnung das Tempo in 7-%-Schritten (höherer Bogen), statt das Ziel zu verfehlen.
**Alle Werte [PRÜFEN] durch Probespielen.**

## 7. Timing und Streuung (`Data/timing.csv`)

Startwerte **[PRÜFEN]** (README 24.3: „durch Probespielen"):

* Grünes Fenster ± 30 ms, gelbes ± 75 ms um den idealen Loslasszeitpunkt.
* Grundstreuung seitlich 0,20 m (auch bei perfektem Timing), dazu **22 m pro Sekunde Timing-Fehler**
  außerhalb von Grün (40 ms zu spät → ca. 0,2 + 0,2 m), Längenstreuung 0,30 m + 6 m/s.
* Die **Seite** der Abweichung ist zufällig (README 5.3: beide Seiten gleich riskant) – Test: 50,3 % links.
* Power: Fenster × 0,70, Streuung × 1,45; Präzision: Fenster × 1,35, Streuung × 0,65.
* Spielerwert 50 → 99: Fenster × 0,85 → 1,20, Streuung × 1,40 → 0,70.
* Volley: Fenster × 1,30 (README 5.6), Halbvolley × 0,70.
* Die Match-Einstellung „Timing-Fenster" multipliziert alle Fenster für beide Spieler.

## 8. Kontaktfläche beim Aufsprung (Hawk-Eye, README 6.2 / 7.3)

Eigene Formel:

1. Der Ball wird als Feder mit der Steifigkeit k betrachtet: maximale Stauchung δ_max = v_n·√(m/k),
   Kontaktzeit t_c = π·√(m/k) = 4,36 ms. δ_max ist auf 45 % des Radius begrenzt.
2. Während des Kontakts folgt die Stauchung einer Halbwelle; gerechnet wird mit der Parabel
   δ(t) = δ_max · 4u(1 − u), u = t/t_c (deterministisch, weicht < 6 % vom Sinus ab).
3. Der Radius der Berührfläche ist der Kugelabschnitt a = √(2rδ − δ²).
4. Der Ballmittelpunkt wandert während des Kontakts mit der mittleren Horizontalgeschwindigkeit weiter.
   Die Kontaktfläche ist die Vereinigung aller Berührkreise (33 Stützstellen).

Damit ist die **Breite** = 2·a_max (hängt nur vom senkrechten Tempo ab) und die **Länge** = Breite +
Wanderweg (hängt vom Tempo ab – schnelle Bälle „rutschen länger").
Beispiele (Test `HawkEye.Kontaktflaeche`): steil und langsam 5,5 × 5,0 cm; schnell (35 m/s) 14,0 × 5,0 cm;
schnell und flach 13,3 × 3,2 cm **[PRÜFEN]** gegen echte Hawk-Eye-Bilder.

**„In", sobald die Fläche das Feld inklusive Linie berührt.** Alle Platzmaße werden – wie in den
ITF-Regeln – bis zur **Außenkante** der Linien gemessen; die Linien liegen innerhalb und gehören zum Feld.
Die Anzeige nennt den Abstand zur Entscheidungsgrenze in mm (Test: 1 mm Lücke → „AUS 1 mm").

## 9. Linienrichter (README 7.1)

* Fehlerwahrscheinlichkeit p(d) = 0,2 · (1 − d / 15 cm) für d ≤ 15 cm, sonst 0 (Vorschlag der README).
* **d** ist der Abstand der Kontaktfläche zur Entscheidungsgrenze (Außenkante der Linie), auf beiden
  Seiten – also wie knapp die Entscheidung ist. Ein Ball, der die Linie gerade noch streift, ist
  besonders knapp; ein Ball mitten auf der Linie ist es nicht.
* **Prüfung per Simulation (README: [PRÜFEN])**, Test `Linienrichter.FehlerKurveUndMittelwert`:
  200 000 knappe Bälle, d gleichverteilt bis 15 cm → **10,02 % Fehler** (Ziel 10 %); bei d < 3 cm 18,3 %. ✔
  Ist d im Spiel nicht gleichverteilt (z. B. mehr Bälle sehr knapp), kann p_max angepasst werden – die
  Bots-gegen-Bots-Simulation (M3) misst das nach.
* „Elektronische Linienrichter": keine Fehler, keine Challenges.

## 10. Challenge-Verhalten der KI (`FKIChallengeParameter`)

Startwerte **[PRÜFEN]**: challengt nur bei Abstand ≤ 8 cm; Grundchance 55 % bei 0 cm, linear fallend;
erkennt Fehlentscheidungen besser (Faktor 1,0 statt 0,3 bei richtigen Entscheidungen) – Ziel ist eine
Erfolgsquote der KI-Challenges ähnlich der Tour (ca. 25–30 %); mit der letzten Challenge vorsichtiger
(× 0,5); bei Break-/Satz-/Matchbällen zusätzlich 4 % „strategisch" (bis 20 cm).

## 11. Spielerwerte und Gesamtwert (README 9.2)

Gesamtwert = gewichteter Durchschnitt (gerundet, 0–99):

| Aufschlag | Vorhand | Rückhand | Volley | Schnelligkeit | Ausdauer | Nerven | Spin | Return |
|---|---|---|---|---|---|---|---|---|
| 0,15 | 0,15 | 0,15 | 0,08 | 0,12 | 0,07 | 0,08 | 0,08 | 0,12 |

Begründung: Aufschlag, Grundschläge und Return entscheiden die meisten Punkte; Schnelligkeit ist für
Erreichbarkeit zentral; Volley, Spin, Nerven und Ausdauer wirken seltener oder indirekt **[PRÜFEN]**.
Der Test `Karriere.Gesamtwert` rechnet den Gesamtwert jedes Kader-Spielers aus `spieler.csv` nach.

## 12. Hintergrund-Simulation der Karriere (README 14.5)

Gewinnwahrscheinlichkeit eines Aufschlagpunkts:
p = 0,635 + 0,004·(Aufschlag A − Return B) + 0,0035·(Gesamt A − Gesamt B) + 0,015·(Form A − Form B)
+ Belagbonus, begrenzt auf 0,40–0,85; bei Break-, Satz- und Matchbällen zusätzlich 0,0006 je Punkt
Nerven-Unterschied. 0,635 entspricht dem Tour-Durchschnitt gewonnener Aufschlagpunkte (ca. 62–65 %).

Ergebnisse (Test `Karriere.MatchSimulation`, je 1000–3000 Matches über zwei Gewinnsätze):
gleich starke Spieler 50,4 % / 49,6 %, im Mittel 164 Punkte; 95 gegen 70: 100 %; 80 gegen 75: 82 %
**[PRÜFEN]** – evtl. etwas zu deutlich; nach dem Ranglisten-Probelauf (M7) nachstellen.

## 13. Weltrangliste (README 14.6)

Rollierend über 52 Wochen; es zählen die **besten 18 Ergebnisse** (ATP seit der Saison 2026 – vorher 19;
WTA seit 2024), die Finals zählen zusätzlich. Gleichstand: höheres Einzelergebnis, dann Spieler-Id.
Punkte je Runde und Preisgelder: `Data/punkte_atp.csv`, `Data/punkte_wta.csv`, `Data/preisgeld.csv`
(Herleitung in `Dokumente/Karriere-Daten.md`).

## 14. Platz (README 12.1, 24.1)

Alle Maße aus ITF-Regel 1, gemessen bis zur Außenkante der Linien: 23,77 × 8,23 m (Einzel),
10,97 m (Doppel), Aufschlaglinie 6,40 m vom Netz, Netz 0,914 m Mitte / 1,07 m an den Pfosten,
Pfosten 0,914 m außerhalb der Doppellinie, Einzelstützen 0,914 m außerhalb der Einzellinie.
**Linienbreiten:** Grundlinie 10 cm, alle anderen 5 cm, Mittellinie und Mittelmarke 5 cm (ITF: 2,5–5 cm,
Grundlinie bis 10 cm; 12.1 [PRÜFEN] → so umgesetzt, je Anlage änderbar). Die Mittellinie liegt mittig
und gehört zu beiden Aufschlagfeldern. Auslauf in der Graubox: 6,40 m hinten, 3,66 m seitlich (ITF-Minimum).

## 15. Proportionen der Figuren (README 3.3, [PRÜFEN])

Startwerte für den Figuren-Generator nach Drillis & Contini (1966), angegeben als Anteil der Körpergröße H:

| Punkt / Strecke | Anteil von H | Punkt / Strecke | Anteil von H |
|---|---|---|---|
| Augenhöhe | 0,936 | Schulterbreite | 0,259 |
| Kinn | 0,870 (Kopfhöhe 0,130 → ca. 7,7 Köpfe) | Hüftbreite | 0,191 |
| Schulter (Akromion) | 0,818 | Oberarm | 0,186 |
| Ellbogen | 0,630 | Unterarm | 0,146 |
| Handgelenk | 0,485 | Hand | 0,108 |
| Hüfte (Trochanter) | 0,530 | Oberschenkel | 0,245 |
| Knie | 0,285 | Unterschenkel | 0,246 |
| Knöchel | 0,039 | Fußlänge | 0,152 |

Sportliche Figuren: 7,5–8 Kopfhöhen; Schultern bei Männern etwas breiter (bis 0,27 H).

## 16. Biomechanik der Schläge (README 4.1, [PRÜFEN] – Grundlage für M2/M3)

Richtwerte aus der Tennis-Biomechanik (Elliott, Reid & Crespo; Kovacs & Ellenbecker – 8-Phasen-Modell
des Aufschlags) als Startpunkt für die Keyframe-Animationen:

* **Kinetische Kette** Beine → Hüfte → Rumpf → Schulter → Ellbogen → Handgelenk, jeweils zeitversetzt
  (Spitzen der Winkelgeschwindigkeiten nacheinander, ca. 20–50 ms Abstand).
* **Aufschlag:** Kniebeugung vorne in der Ladephase ca. 60–70°; Treffpunkt ca. 1,5 × Körpergröße
  (≈ 2,7–2,8 m bei 1,85 m); Schläger-Kopfgeschwindigkeit bei Profis 45–55 m/s; Innenrotation der
  Schulter bis ca. 2000–2500 °/s; Ballwurf ca. 0,3–0,6 m über den Treffpunkt.
* **Vorhand:** Ausholen 0,4–0,6 s, Vorwärtsschwung 0,15–0,20 s, Treffpunkt 0,2–0,4 m vor der vorderen
  Hüfte; Schläger-Kopfgeschwindigkeit 30–38 m/s; Rumpfrotation 700–900 °/s; offener oder neutraler Stand.
* **Beidhändige Rückhand:** kürzeres Ausholen, Treffpunkt etwas weiter hinten als bei der Vorhand.
* **Split-Step:** im Moment des gegnerischen Treffpunkts, Landung 0,1–0,2 s danach.

## 17. Was noch durch Probespielen bestimmt wird

Timing-Fenster und Streuung, KI-Profile je Schwierigkeit, Laufgeschwindigkeit/Reichweite
(Startwerte im Testspieler: 6 m/s, 1,6 m; am Netz × 1,2), Level-Kurve und Fähigkeitspunkte,
Ausdauer-Abnahme – sobald M2 (Figur, Timing-Meter, KI) läuft. Danach Bots-gegen-Bots (README 24.3):
tausende Matches headless, Abgleich von Aufschlagquote, Asse, Ballwechsellänge und Break-Rate mit der Tour.

## Quellen

* ITF Rules of Tennis, Regel 1 (Platz) und Anhang I (Ball: Masse, Größe, Rücksprung).
* ITF Court Pace Classification / Court Surface Assessment (CPR-Formel, Stoßzahl- und Reibungsbänder).
* R. Cross, C. Lindsey: „Measurements of drag and lift on tennis balls in flight" (Sports Engineering);
  Zusammenfassung bei Tennis Warehouse University („aerodynamics2").
* J. Štěpánek: „The aerodynamics of tennis balls – the topspin lob", American Journal of Physics 56 (1988).
* S. Goodwill, S. Chin, S. Haake: Aerodynamics of spinning and non-spinning tennis balls (2004).
* H. Brody, R. Cross, C. Lindsey: The Physics and Technology of Tennis (Absprungmodell, Kontaktzeiten).
* W. Drillis, R. Contini (1966) – Segmentlängen; zitiert u. a. in D. A. Winter: Biomechanics and Motor Control of Human Movement.
* B. Elliott, M. Reid, M. Crespo: Biomechanics of Advanced Tennis; M. Kovacs, T. Ellenbecker (2011): An 8-stage model for evaluating the tennis serve.
* ATP Media Guide 2026, ITF „WTA 2026 points table", openerarankings.com (Punkte/Rangliste), Punto de Break (ATP ab 2026 beste 18).
* GitHub Docs: Git LFS – 10 GiB Speicher und 10 GiB Bandbreite pro Monat im kostenlosen Konto.

Hinweis: Einige Quellen (ITF-PDFs, openerarankings.com) waren aus der Cloud-Sitzung nicht direkt
abrufbar; die Werte stammen aus Suchergebnis-Auszügen und sind mit [PRÜFEN] markiert, wo sie die
Spiellogik direkt betreffen.
