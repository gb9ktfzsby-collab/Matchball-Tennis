#!/usr/bin/env python3
"""Matchball Tennis - Pruefung der Karriere-CSV-Dateien (README 14.2-14.7, 12.4-12.6).

Prueft punkte_atp.csv, punkte_wta.csv, preisgeld.csv, turniere_atp.csv, turniere_wta.csv,
turniere_niedrig.csv und anlagen.csv in Unreal/Data/:
  * UTF-8 ohne BOM, Spaltenzahl je Zeile, eindeutige Zeilennamen, keine leeren Felder,
  * Wochen 1-47, Datum passt zur Kalenderwoche, Belaege gueltig, Anlagen-Bezug vorhanden,
  * Punkte und Preisgeld-Anteile monoton fallend, Anteile summieren sich je Feld zu 1,
  * Feld/Qualifikation eines Turniers passen zur Wertungszeile, Saison-Abschnitte (Rasen, Halle).

Aufruf (Repository-Wurzel oder beliebig):  python Unreal/Tools/pruefe_karriere_daten.py [<Pfad zu Unreal/Data>]
Laeuft ohne Zusatzpakete, auch in der CI (.github/workflows/kern-tests.yml).
Exit-Code 0 = alles in Ordnung.
"""

import csv
import datetime
import math
import pathlib
import re
import sys

DATEN = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else pathlib.Path(__file__).resolve().parents[1] / "Data"

BELAEGE = {"Hart", "Sand", "Rasen", "Halle"}
BOOL = {"True", "False"}
RUNDEN = ["S", "F", "HF", "VF", "R16", "R32", "R64", "R128"]
QUALI = ["Q", "Q3", "Q2", "Q1"]
STUFEN = {
    "ATP": {"GrandSlam": 2000, "Masters1000": 1000, "ATP500": 500, "ATP250": 250, "Challenger175": 175,
            "Challenger125": 125, "Challenger100": 100, "Challenger75": 75, "Challenger50": 50,
            "ITF_M25": 25, "ITF_M15": 15, "Finals": 1500},
    "WTA": {"GrandSlam": 2000, "WTA1000": 1000, "WTA500": 500, "WTA250": 250, "WTA125": 125, "ITF_W100": 100,
            "ITF_W75": 75, "ITF_W50": 50, "ITF_W35": 35, "ITF_W15": 15, "Finals": 1500},
}
NIEDRIG = {"Challenger175", "Challenger125", "Challenger100", "Challenger75", "Challenger50", "ITF_M25", "ITF_M15",
           "WTA125", "ITF_W100", "ITF_W75", "ITF_W50", "ITF_W35", "ITF_W15"}
KONTINENTE = {"Europa", "Nordamerika", "Suedamerika", "Asien", "Ozeanien", "Afrika"}
TRIBUENEN = {"Keine", "Holzbaenke", "Stahlrohr", "Klapptribuene", "Steinstufen", "Schalensitze"}
DAECHER = {"Keins", "Teil", "Voll", "Halle"}
TAFELN = {"Keine", "Kipptafel", "Elektronisch", "Klassisch", "Videowand", "Videowuerfel"}
CHUNKS = {"Arena": 10, "CourtCentral": 11, "CentreCourt": 12, "Training": 13, "ArenaIndoor": 14, "Finals": 15,
          "Turnierhalle": 16, "Trainingshalle": 17}
PUBLIKUM = {"ITF": (0, 200), "Challenger": (500, 3000), "Tour": (5000, 15000), "GrandSlam": (15000, 10 ** 6),
            "Standard": (0, 0)}
# ISO-3166-Codes, die NICHT zu den Tour-/IOC-Codes in spieler.csv passen wuerden
NUR_ISO = {"DEU", "NLD", "CHE", "CHL", "HRV", "PRT", "ARE", "MCO", "GRC", "SAU", "IDN", "TWN", "ZAF", "DNK", "BGR"}
HEX = re.compile(r"^#[0-9A-F]{6}$")
NAME = re.compile(r"^[A-Za-z0-9_]+$")
CPR = {"Sand": (15, 30), "Hart": (30, 48), "Halle": (40, 52), "Rasen": (45, 56)}

fehler = []
hinweise = []


def melde(text):
    fehler.append(text)


def lies(datei):
    pfad = DATEN / datei
    roh = pfad.read_bytes()
    if roh.startswith(b"\xef\xbb\xbf"):
        melde(f"{datei}: BOM vorhanden")
    if b"\r" in roh:
        melde(f"{datei}: CR-Zeilenenden")
    text = roh.decode("utf-8")
    if '"' in text:
        melde(f"{datei}: enthaelt Anfuehrungszeichen (Quoting vermeiden)")
    zeilen = list(csv.reader(text.splitlines()))
    kopf, daten = zeilen[0], zeilen[1:]
    namen = set()
    ergebnis = []
    for nummer, zeile in enumerate(daten, start=2):
        if len(zeile) != len(kopf):
            melde(f"{datei}:{nummer}: {len(zeile)} Felder, Kopf hat {len(kopf)}")
            continue
        if not NAME.match(zeile[0]):
            melde(f"{datei}:{nummer}: ungueltiger Zeilenname {zeile[0]!r}")
        if zeile[0].lower() in namen:
            melde(f"{datei}:{nummer}: Zeilenname doppelt {zeile[0]!r} (Unreal ignoriert Gross-/Kleinschreibung)")
        namen.add(zeile[0].lower())
        for spalte, wert in zip(kopf, zeile):
            if wert == "" or wert != wert.strip():
                melde(f"{datei}:{nummer}: Feld {spalte} leer oder mit Leerzeichen am Rand")
        eintrag = dict(zip(kopf, zeile))
        eintrag["_zeile"] = nummer
        ergebnis.append(eintrag)
    if kopf[0] != "Name":
        melde(f"{datei}: erste Spalte muss 'Name' heissen")
    return kopf, ergebnis


def ganz(eintrag, spalte, datei):
    try:
        return int(eintrag[spalte])
    except ValueError:
        melde(f"{datei}:{eintrag['_zeile']}: {spalte}={eintrag[spalte]!r} ist keine ganze Zahl")
        return 0


def zahl(eintrag, spalte, datei):
    try:
        return float(eintrag[spalte])
    except ValueError:
        melde(f"{datei}:{eintrag['_zeile']}: {spalte}={eintrag[spalte]!r} ist keine Zahl")
        return 0.0


def pruefe_bool(eintrag, spalte, datei):
    if eintrag[spalte] not in BOOL:
        melde(f"{datei}:{eintrag['_zeile']}: {spalte}={eintrag[spalte]!r} ist nicht True/False")
    return eintrag[spalte] == "True"


def pruefe_hex(eintrag, spalte, datei):
    if not HEX.match(eintrag[spalte]):
        melde(f"{datei}:{eintrag['_zeile']}: {spalte}={eintrag[spalte]!r} ist keine Farbe #RRGGBB")


def ko_runden(feld):
    """Liefert {Runde: Anzahl Verlierer} fuer ein K.-o.-Feld (S = Sieger)."""
    b = 1 << math.ceil(math.log2(feld))
    r_max = int(math.log2(b))
    namen = {r_max: "F", r_max - 1: "HF", r_max - 2: "VF", r_max - 3: "R16", r_max - 4: "R32", r_max - 5: "R64", r_max - 6: "R128"}
    anzahl = {"S": 1}
    for r in range(1, r_max + 1):
        anzahl[namen[r]] = feld - b // 2 if r == 1 else b // (2 ** r)
    return anzahl


def quali_runden(qfeld, qr):
    return {f"Q{k}": qfeld // (2 ** k) for k in range(1, qr + 1)}


def pruefe_monoton(werte, datei, zeile, art, streng_bis=0):
    for i in range(len(werte) - 1):
        (n1, w1), (n2, w2) = werte[i], werte[i + 1]
        if w2 > w1 or (i < streng_bis and w2 >= w1):
            melde(f"{datei}:{zeile}: {art} nicht monoton fallend ({n1}={w1}, {n2}={w2})")


# --- Punkte ------------------------------------------------------------------------------------

def pruefe_punkte(datei, tour):
    _, zeilen = lies(datei)
    tabelle = {}
    for z in zeilen:
        nr = z["_zeile"]
        if z["Tour"] != tour:
            melde(f"{datei}:{nr}: Tour {z['Tour']} statt {tour}")
        if z["Stufe"] not in STUFEN[tour]:
            melde(f"{datei}:{nr}: unbekannte Stufe {z['Stufe']}")
        feld, qfeld, qr = ganz(z, "Feld", datei), ganz(z, "Quali_Feld", datei), ganz(z, "Quali_Runden", datei)
        p = {s: ganz(z, s, datei) for s in RUNDEN + QUALI + ["GruppenSieg"]}
        if z["Stufe"] == "Finals":
            if feld != 8 or qfeld or qr:
                melde(f"{datei}:{nr}: Finals brauchen Feld 8 ohne Qualifikation")
            if any(p[s] for s in RUNDEN[3:] + QUALI) or not (p["S"] > p["F"] > p["HF"] == 0) or p["GruppenSieg"] <= 0:
                melde(f"{datei}:{nr}: Finals-Punkte unplausibel")
            maximal = p["S"] + 3 * p["GruppenSieg"]
            if maximal != STUFEN[tour]["Finals"]:
                melde(f"{datei}:{nr}: Finals ungeschlagen = {maximal}, erwartet 1500")
        else:
            if (qfeld == 0) != (qr == 0) or (qfeld and qfeld % (2 ** qr)):
                melde(f"{datei}:{nr}: Quali_Feld {qfeld} passt nicht zu {qr} Runden")
            vorhanden = ko_runden(feld)
            for s in RUNDEN:
                if s not in vorhanden and p[s]:
                    melde(f"{datei}:{nr}: Runde {s} gibt es bei Feld {feld} nicht, Punkte muessen 0 sein")
            pruefe_monoton([(s, p[s]) for s in RUNDEN if s in vorhanden], datei, nr, "Punkte", streng_bis=3)
            qvorh = ["Q"] + [f"Q{k}" for k in range(qr, 0, -1)] if qr else []
            for s in QUALI:
                if s not in qvorh and p[s]:
                    melde(f"{datei}:{nr}: Quali-Runde {s} gibt es nicht, Punkte muessen 0 sein")
            pruefe_monoton([(s, p[s]) for s in qvorh], datei, nr, "Quali-Punkte")
            if p["GruppenSieg"]:
                melde(f"{datei}:{nr}: GruppenSieg nur bei Finals")
            soll = STUFEN[tour].get(z["Stufe"])
            if soll and p["S"] != soll:
                melde(f"{datei}:{nr}: Siegerpunkte {p['S']} passen nicht zur Stufe {z['Stufe']} ({soll})")
        tabelle[z["Name"]] = dict(z, Feld=feld, Quali_Feld=qfeld, Quali_Runden=qr)
    return tabelle


# --- Preisgeld ---------------------------------------------------------------------------------

def pruefe_preisgeld(punkte):
    datei = "preisgeld.csv"
    _, zeilen = lies(datei)
    tabelle = {}
    abweichung_max = 0.0
    for z in zeilen:
        nr = z["_zeile"]
        name = z["Name"]
        feld, qfeld, qr = ganz(z, "Feld", datei), ganz(z, "Quali_Feld", datei), ganz(z, "Quali_Runden", datei)
        a = {s: zahl(z, s, datei) for s in ["Teilnahme", "GruppenSieg"] + RUNDEN + ["Q3", "Q2", "Q1"]}
        gesamt = ganz(z, "Gesamt_USD", datei)
        if gesamt <= 0:
            melde(f"{datei}:{nr}: Gesamt_USD muss > 0 sein")
        p = punkte.get(z["Tour"], {}).get(name)
        if p is None:
            melde(f"{datei}:{nr}: keine Punkte-Zeile {name} in punkte_{z['Tour'].lower()}.csv")
        elif (p["Stufe"], p["Feld"], p["Quali_Feld"], p["Quali_Runden"]) != (z["Stufe"], feld, qfeld, qr):
            melde(f"{datei}:{nr}: Stufe/Feld/Quali weichen von der Punkte-Zeile ab")
        if z["Stufe"] == "Finals":
            summe = 8 * a["Teilnahme"] + 12 * a["GruppenSieg"] + a["S"] + a["F"] + 2 * a["HF"]
            if not (a["S"] > a["F"] > a["HF"] >= 0):
                melde(f"{datei}:{nr}: Finals-Anteile nicht monoton")
        else:
            anzahl = ko_runden(feld)
            anzahl.update(quali_runden(qfeld, qr))
            for s, w in a.items():
                if s not in anzahl and w:
                    melde(f"{datei}:{nr}: Anteil {s} muss 0 sein (Runde gibt es nicht)")
            summe = sum(a[s] * n for s, n in anzahl.items())
            pruefe_monoton([(s, a[s]) for s in RUNDEN if s in anzahl], datei, nr, "Preisgeld-Anteile", streng_bis=4)
            quali = [(f"Q{k}", a[f"Q{k}"]) for k in range(qr, 0, -1)]
            pruefe_monoton(quali, datei, nr, "Quali-Anteile")
            erste = [s for s in RUNDEN if s in anzahl][-1]
            if quali and quali[0][1] > a[erste]:
                melde(f"{datei}:{nr}: letzte Quali-Runde zahlt mehr als die erste Hauptfeldrunde")
        abweichung_max = max(abweichung_max, abs(summe - 1.0))
        if abs(summe - 1.0) > 1e-4:
            melde(f"{datei}:{nr}: Anteile ergeben {summe:.6f} statt 1")
        tabelle[name] = dict(z, Gesamt=gesamt, Feld=feld, Quali_Feld=qfeld)
    for tour, t in punkte.items():
        for name in t:
            if name not in tabelle:
                melde(f"preisgeld.csv: Zeile {name} fehlt")
    hinweise.append(f"preisgeld.csv: groesste Abweichung der Anteilssumme von 1: {abweichung_max:.2e}")
    return tabelle


# --- Anlagen -----------------------------------------------------------------------------------

def pruefe_anlagen():
    datei = "anlagen.csv"
    _, zeilen = lies(datei)
    anlagen = {}
    for z in zeilen:
        nr = z["_zeile"]
        if z["Name"] != f"{z['Anlage']}_{z['Variante']}":
            melde(f"{datei}:{nr}: Name muss Anlage_Variante sein")
        if z["Belag"] not in BELAEGE:
            melde(f"{datei}:{nr}: Belag {z['Belag']} ungueltig")
        for s in ("Belagfarbe", "Auslauffarbe", "Tribuenenfarbe", "Bandenfarbe"):
            pruefe_hex(z, s, datei)
        if z["Tribuenenart"] not in TRIBUENEN:
            melde(f"{datei}:{nr}: Tribuenenart {z['Tribuenenart']} ungueltig")
        reihen, seiten = ganz(z, "Reihen", datei), ganz(z, "Seiten", datei)
        if not 0 <= seiten <= 4 or ((reihen == 0) != (seiten == 0)) or ((reihen == 0) != (z["Tribuenenart"] == "Keine")):
            melde(f"{datei}:{nr}: Reihen/Seiten/Tribuenenart widerspruechlich")
        if z["Dach"] not in DAECHER:
            melde(f"{datei}:{nr}: Dach {z['Dach']} ungueltig")
        if (z["Dach"] == "Halle") != (z["Belag"] == "Halle"):
            melde(f"{datei}:{nr}: Hallen-Anlagen haben Standardbelag Halle, Freiluft-Anlagen nicht")
        pmin, pmax = ganz(z, "Publikum_Min", datei), ganz(z, "Publikum_Max", datei)
        lo, hi = PUBLIKUM.get(z["Variante"], (None, None))
        if lo is None:
            melde(f"{datei}:{nr}: Variante {z['Variante']} unbekannt")
        elif not (pmin <= pmax and lo <= pmin and pmax <= hi and (z["Variante"] != "GrandSlam" or pmin >= 15000)):
            melde(f"{datei}:{nr}: Publikum {pmin}-{pmax} passt nicht zu README 12.6 ({z['Variante']})")
        if z["Anzeigetafel"] not in TAFELN:
            melde(f"{datei}:{nr}: Anzeigetafel {z['Anzeigetafel']} ungueltig")
        for s in ("bBaeume", "bFahnen", "bEfeu", "bFlutlicht", "bZaun"):
            pruefe_bool(z, s, datei)
        if zahl(z, "Auslauf_Hinten", datei) < 6.40 or zahl(z, "Auslauf_Seite", datei) < 3.66:
            melde(f"{datei}:{nr}: Auslauf unter ITF-Minimum (6,40 / 3,66 m)")
        if ganz(z, "Chunk", datei) != CHUNKS.get(z["Anlage"]):
            melde(f"{datei}:{nr}: Chunk passt nicht zu README 19.4.1")
        cpr = zahl(z, "CourtPace", datei)
        if cpr and not CPR[z["Belag"]][0] <= cpr <= CPR[z["Belag"]][1]:
            melde(f"{datei}:{nr}: CourtPace {cpr} unplausibel fuer {z['Belag']}")
        anlagen.setdefault(z["Anlage"], []).append(z)
    for name in CHUNKS:
        if name not in anlagen:
            melde(f"{datei}: Anlage {name} fehlt (README 12.4/12.5)")
    for name in ("Arena", "CourtCentral", "CentreCourt", "ArenaIndoor"):
        varianten = {z["Variante"] for z in anlagen.get(name, [])}
        if varianten != {"ITF", "Challenger", "Tour", "GrandSlam"}:
            melde(f"{datei}: {name} braucht alle 4 Varianten aus README 12.6, hat {sorted(varianten)}")
    return anlagen


# --- Turniere ----------------------------------------------------------------------------------

def standardbelag(anlagen, name):
    return anlagen[name][0]["Belag"]


def ist_halle(anlagen, name):
    return anlagen[name][0]["Dach"] == "Halle"


def pruefe_ort(z, datei, spalte="Land"):
    code = z[spalte]
    if not re.match(r"^[A-Z]{3}$", code):
        melde(f"{datei}:{z['_zeile']}: Laendercode {code!r} ungueltig")
    if code in NUR_ISO:
        melde(f"{datei}:{z['_zeile']}: {code} ist ein ISO-Code; spieler.csv nutzt Tour-/IOC-Codes")


def pruefe_turniere(datei, tour, punkte, preise, anlagen):
    _, zeilen = lies(datei)
    belegung = {}
    zaehler = {}
    for z in zeilen:
        nr = z["_zeile"]
        if not z["Name"].startswith(tour + "_"):
            melde(f"{datei}:{nr}: Name soll mit {tour}_ beginnen")
        pruefe_ort(z, datei)
        woche, wochen = ganz(z, "Woche", datei), ganz(z, "Wochen", datei)
        if not (1 <= woche <= 47 and wochen in (1, 2) and woche + wochen - 1 <= 47):
            melde(f"{datei}:{nr}: Woche {woche}/{wochen} ausserhalb 1-47")
        try:
            datum = datetime.date.fromisoformat(z["Datum"])
            kw = datum.isocalendar()[1]
            if datum.year != 2026 or not (kw == woche or (datum.weekday() >= 5 and kw + 1 == woche)):
                melde(f"{datei}:{nr}: Datum {datum} liegt in KW {kw}, nicht in KW {woche}")
        except ValueError:
            melde(f"{datei}:{nr}: Datum {z['Datum']!r} ungueltig")
        stufe = z["Stufe"]
        if stufe not in STUFEN[tour] or stufe in NIEDRIG:
            melde(f"{datei}:{nr}: Stufe {stufe} gehoert nicht in den Tour-Kalender")
        wertung = punkte[tour].get(z["Wertung"])
        preis = preise.get(z["Wertung"])
        feld, quali, gesetzte = ganz(z, "Feld", datei), ganz(z, "Qualifikation", datei), ganz(z, "Gesetzte", datei)
        if wertung is None or preis is None:
            melde(f"{datei}:{nr}: Wertung {z['Wertung']} fehlt in Punkte- oder Preisgeld-Tabelle")
        else:
            if wertung["Stufe"] != stufe or wertung["Feld"] != feld or wertung["Quali_Feld"] != quali:
                melde(f"{datei}:{nr}: Stufe/Feld/Qualifikation passen nicht zu {z['Wertung']}")
            verhaeltnis = ganz(z, "Preisgeld_USD", datei) / preis["Gesamt"]
            if not 0.4 <= verhaeltnis <= 2.5:
                melde(f"{datei}:{nr}: Preisgeld {z['Preisgeld_USD']} weit weg vom Richtwert {preis['Gesamt']}")
        freilose = (1 << math.ceil(math.log2(feld))) - feld
        if gesetzte not in (8, 16, 32) or gesetzte > max(feld // 2, 8) or freilose > gesetzte:
            melde(f"{datei}:{nr}: Gesetzte {gesetzte} passen nicht zu Feld {feld}")
        belag = z["Belag"]
        halle = pruefe_bool(z, "bHalle", datei)
        nacht = pruefe_bool(z, "bNachtsession", datei)
        if belag not in BELAEGE:
            melde(f"{datei}:{nr}: Belag {belag} ungueltig")
        anlage = z["Anlage"]
        if anlage not in anlagen:
            melde(f"{datei}:{nr}: Anlage {anlage} fehlt in anlagen.csv")
        else:
            if halle != ist_halle(anlagen, anlage):
                melde(f"{datei}:{nr}: Halle={halle}, aber Anlage {anlage} ist {'eine' if ist_halle(anlagen, anlage) else 'keine'} Halle")
            if not halle and standardbelag(anlagen, anlage) != belag:
                melde(f"{datei}:{nr}: Freiluft-Belag {belag} passt nicht zur Anlage {anlage}")
            if anlage in ("Training", "Trainingshalle"):
                melde(f"{datei}:{nr}: Trainingsanlagen sind keine Turnieranlagen")
            if (stufe == "Finals") != (anlage == "Finals"):
                melde(f"{datei}:{nr}: Finals gehoeren in die Finals-Halle")
        if belag == "Halle" and not halle:
            melde(f"{datei}:{nr}: Belag Halle nur in der Halle")
        if halle and belag == "Rasen":
            melde(f"{datei}:{nr}: Rasen in der Halle gibt es nicht")
        if halle and nacht:
            melde(f"{datei}:{nr}: Nachtsession nur draussen sinnvoll")
        cpr = zahl(z, "CourtPace", datei)
        if belag in CPR and not CPR[belag][0] <= cpr <= CPR[belag][1]:
            melde(f"{datei}:{nr}: CourtPace {cpr} unplausibel fuer {belag}")
        for s in ("Belagfarbe", "Auslauffarbe", "Akzentfarbe"):
            pruefe_hex(z, s, datei)
        # Saison-Abschnitte (README 14.3)
        if belag == "Rasen" and not 24 <= woche <= 28:
            melde(f"{datei}:{nr}: Rasen ausserhalb der Rasensaison (KW 24-28)")
        if belag == "Halle" and 10 <= woche <= 38:
            melde(f"{datei}:{nr}: Hallen-Hartplatz mitten in der Freiluftsaison (KW {woche})")
        if belag == "Sand" and not halle and not (6 <= woche <= 23 or 29 <= woche <= 31):
            melde(f"{datei}:{nr}: Sandplatz ausserhalb Februar-Suedamerika/Sandsaison/Juli (KW {woche})")
        if stufe == "Finals" and not 45 <= woche <= 47:
            melde(f"{datei}:{nr}: Finals nicht im November")
        for w in range(woche, woche + wochen):
            belegung.setdefault(w, []).append((stufe, z["Name"]))
        zaehler[stufe] = zaehler.get(stufe, 0) + 1
    gross = {"GrandSlam", "Masters1000", "WTA1000", "Finals"}
    for w, liste in sorted(belegung.items()):
        grosse = [n for s, n in liste if s in gross]
        if len(grosse) > 1:
            melde(f"{datei}: KW {w} hat mehrere Grossturniere: {grosse}")
        if any(s == "GrandSlam" for s, _ in liste) and len(liste) > 1:
            melde(f"{datei}: KW {w}: neben dem Grand Slam laeuft {liste}")
    leer = [w for w in range(1, 48) if w not in belegung]
    hinweise.append(f"{datei}: {len(zeilen)} Turniere {dict(sorted(zaehler.items()))}; Wochen ohne Tour-Turnier: {leer}")
    return zeilen


def pruefe_niedrig(punkte, preise, anlagen):
    datei = "turniere_niedrig.csv"
    _, zeilen = lies(datei)
    proWoche = {("ATP", w): 0 for w in range(1, 48)}
    proWoche.update({("WTA", w): 0 for w in range(1, 48)})
    for z in zeilen:
        nr = z["_zeile"]
        tour = z["Tour"]
        if tour not in STUFEN:
            melde(f"{datei}:{nr}: Tour {tour} ungueltig")
            continue
        if z["Stufe"] not in NIEDRIG or z["Stufe"] not in STUFEN[tour]:
            melde(f"{datei}:{nr}: Stufe {z['Stufe']} ist keine niedrige Stufe der {tour}")
        wertung = punkte[tour].get(z["Wertung"])
        feld, quali = ganz(z, "Feld", datei), ganz(z, "Qualifikation", datei)
        if wertung is None or z["Wertung"] not in preise:
            melde(f"{datei}:{nr}: Wertung {z['Wertung']} fehlt")
        elif (wertung["Stufe"], wertung["Feld"], wertung["Quali_Feld"]) != (z["Stufe"], feld, quali):
            melde(f"{datei}:{nr}: Stufe/Feld/Qualifikation passen nicht zu {z['Wertung']}")
        von, bis = ganz(z, "Woche_Von", datei), ganz(z, "Woche_Bis", datei)
        if not 1 <= von <= bis <= 47:
            melde(f"{datei}:{nr}: Wochen {von}-{bis} ausserhalb 1-47")
        belag = z["Belag"]
        halle = pruefe_bool(z, "bHalle", datei)
        pruefe_bool(z, "bNachtsession", datei)
        if belag not in BELAEGE:
            melde(f"{datei}:{nr}: Belag {belag} ungueltig")
        if belag == "Rasen" and not (23 <= von and bis <= 28):
            melde(f"{datei}:{nr}: Rasen ausserhalb KW 23-28")
        anlage = z["Anlage"]
        if anlage not in anlagen:
            melde(f"{datei}:{nr}: Anlage {anlage} fehlt in anlagen.csv")
        else:
            if halle != ist_halle(anlagen, anlage):
                melde(f"{datei}:{nr}: Halle={halle} passt nicht zur Anlage {anlage}")
            if not halle and standardbelag(anlagen, anlage) != belag:
                melde(f"{datei}:{nr}: Belag {belag} passt nicht zur Anlage {anlage}")
            varianten = {a["Variante"] for a in anlagen[anlage]}
            if not {"ITF", "Challenger"} <= varianten:
                melde(f"{datei}:{nr}: Anlage {anlage} hat keine ITF-/Challenger-Variante")
        if belag == "Halle" and not halle:
            melde(f"{datei}:{nr}: Belag Halle nur in der Halle")
        if z["Kontinent"] not in KONTINENTE:
            melde(f"{datei}:{nr}: Kontinent {z['Kontinent']} ungueltig")
        anzahl = ganz(z, "AnzahlProWoche", datei)
        staedte = z["Staedte"].split(";")
        gesehen = set()
        for eintrag in staedte:
            teile = eintrag.split(":")
            if len(teile) != 2 or not teile[0]:
                melde(f"{datei}:{nr}: Stadt-Eintrag {eintrag!r} muss Stadt:LAND sein")
                continue
            pruefe_ort({"Land": teile[1], "_zeile": nr}, datei)
            if teile[0] in gesehen:
                melde(f"{datei}:{nr}: Stadt {teile[0]} doppelt")
            gesehen.add(teile[0])
        if not 1 <= anzahl <= len(staedte):
            melde(f"{datei}:{nr}: AnzahlProWoche {anzahl} groesser als die Staedteliste")
        for w in range(von, bis + 1):
            proWoche[(tour, w)] += anzahl
    for tour in ("ATP", "WTA"):
        werte = [proWoche[(tour, w)] for w in range(1, 48)]
        luecken = [w for w in range(1, 48) if proWoche[(tour, w)] == 0]
        if luecken:
            melde(f"{datei}: {tour} hat Wochen ohne niedrige Turniere: {luecken}")
        hinweise.append(f"{datei}: {tour} je Woche {min(werte)}-{max(werte)} Turniere (Mittel {sum(werte) / 47:.1f})")
    hinweise.append(f"{datei}: {len(zeilen)} Vorlagen")


def main():
    punkte = {"ATP": pruefe_punkte("punkte_atp.csv", "ATP"), "WTA": pruefe_punkte("punkte_wta.csv", "WTA")}
    preise = pruefe_preisgeld(punkte)
    anlagen = pruefe_anlagen()
    atp = pruefe_turniere("turniere_atp.csv", "ATP", punkte, preise, anlagen)
    wta = pruefe_turniere("turniere_wta.csv", "WTA", punkte, preise, anlagen)
    pruefe_niedrig(punkte, preise, anlagen)
    benutzt = {z["Anlage"] for z in atp + wta}
    hinweise.append(f"Anlagen im Tour-Kalender: {sorted(benutzt)}")
    for h in hinweise:
        print(h)
    if fehler:
        print(f"\nFEHLER ({len(fehler)}):")
        for f in fehler:
            print("  - " + f)
        return 1
    print("\nAlle Karriere-CSV-Dateien sind konsistent.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
