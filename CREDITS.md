# Credits – fremde Quellen und Lizenzen

README 0.2: Jede Datei aus fremder Quelle wird hier sofort eingetragen (Quelle, Urheber, Lizenz, Änderungen).
Stand: **keine fremden Assets** – alles wird selbst erstellt (README 22.1).

## Übernommene Dateien

| Datei | Quelle / Urheber | Lizenz | Änderungen |
|---|---|---|---|
| `Vorgaben/racket.js` (Schlägermodell) | eigenes Modell des Projektinhabers | private Nutzung; Schlägerlogo vor einer Veröffentlichung ersetzen (README 22.3) | wird per `blender/scripts/schlaeger.py` skaliert (68,58 cm), Ursprung ans Griffende gelegt, LODs erzeugt |

## Algorithmen und Daten (kein Code übernommen, nur Verfahren/Werte)

| Was | Quelle | Hinweis |
|---|---|---|
| Zufallsgenerator PCG32 | M. E. O'Neill, pcg-random.org (Referenzimplementierung Apache 2.0) | eigene Umsetzung des Verfahrens in `Core/Kern/MBZufall.cpp`; Referenzwerte im Test |
| Startwert-Mischung SplitMix64 | S. Vigna (gemeinfrei) | `MischeStartwert` |
| Ballphysik-Kennwerte | Cross & Lindsey, Štěpánek, Goodwill et al., Brody/Cross/Lindsey | siehe `Dokumente/Werte.md` |
| Platzmaße, Ball-Spezifikation, Court Pace Rating | ITF Rules of Tennis, ITF Court Pace Classification | |
| Ranglistenpunkte, Turnierkalender (echte Namen) | ATP, WTA, ITF | nur private Nutzung; vor einer Veröffentlichung durch erfundene Namen ersetzen (README 22.3) |
| Körperproportionen | Drillis & Contini (1966) | Startwerte Figuren-Generator |

## Werkzeuge und Dienste

| Was | Lizenz / Bedingungen |
|---|---|
| Unreal Engine 5.8 | Unreal Engine EULA (Epic Games) |
| Blender 5.2 LTS | GPL – gilt nicht für die damit erzeugten Dateien |
| ElevenLabs (Schiedsrichter-Stimme, optional) | kostenloser Tarif: nur nicht-kommerziell, Nennung **„elevenlabs.io"** erforderlich; bezahlte Tarife: kommerziell erlaubt |
| Epic Online Services (M9) | kostenlos, EOS-Nutzungsbedingungen |

## Für den Credits-Bildschirm (M11)

* Stimme des Schiedsrichters: erzeugt mit elevenlabs.io (falls genutzt)
* Engine: Unreal® Engine – Unreal and its logo are Epic's trademarks or registered trademarks
