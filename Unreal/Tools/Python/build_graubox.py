# -*- coding: utf-8 -*-
"""
build_graubox.py - erzeugt die Standard-Map /Game/Maps/M_Graubox (Meilenstein M1, README 3.2, 21).

Aufruf (lokal):
    UnrealEditor-Cmd.exe <Repo>\\Unreal\\MatchballTennis.uproject -run=pythonscript
        -script="<Repo>/Unreal/Tools/Python/build_graubox.py" -unattended -nosplash -nullrhi
    Optionen im -script-Wert:  --ev100 2.5   (feste Belichtung, siehe unten)
Direkt aufgerufen baut das Skript die Map immer neu; import_alles.py ruft es nur auf, wenn die Map fehlt
oder --neu angegeben ist.

Inhalt der Map - nur Licht, Himmel und Bildlook:
  * DirectionalLight "Sonne"        bewegliche Mobilitaet, Atmosphaeren-Sonne (Index 0), Neigung -50 Grad,
                                    Gierwinkel 35 Grad, 10 lx (Unreal-Standard, wie der Rueckfall in
                                    AMBTrainingGameMode::BeginPlay)
  * SkyLight "Himmelslicht"         beweglich, Echtzeit-Erfassung des Himmels (Real Time Capture)
  * SkyAtmosphere "Atmosphaere"     Standardwerte (Erdboden auf Hoehe 0)
  * ExponentialHeightFog "Dunst"    leichter Dunst
  * PostProcessVolume "Bildlook_TV" unbegrenzt (Unbound), FESTE Belichtung (Manual, ohne physikalische
                                    Kamerabelichtung), damit der Fernsehlook nicht pumpt, wenn die Kamera vom
                                    Platz zum Himmel schwenkt.
Platz, Netz, Ball, Ballmaschine, Spieler und Kamera setzt AMBTrainingGameMode zur Laufzeit selbst ein -
sie gehoeren NICHT in die Map. Der GameMode legt nur dann eigenes Licht an, wenn die Map keine Sonne hat.

Feste Belichtung (Startwert, lokal pruefen):
  Manual-Belichtung ohne physikalische Kamera rechnet mit EV100 = 0; die Belichtungskorrektur
  (Exposure Compensation) verschiebt davon in Blendenstufen. Fuer eine Belichtung wie EV100 = x setzt das
  Skript die Korrektur auf -x. Herleitung des Startwerts x = 2: Beleuchtungsstaerke am Boden
  E ~ 10 lx * sin(50 Grad) + Himmel ~ 9,5 lx; Belichtungsmesser-Gleichung EV100 = log2(E / 2,5) ~ 1,9.
  Zusaetzlich steht Min = Max = EV100 = x in den Auto-Belichtungsgrenzen - falls eine Engine-Version den
  Manual-Modus nicht annimmt, ist die Belichtung damit trotzdem fest.
  Zu dunkel/hell? Mit --ev100 neu bauen (kleiner = heller) und das Ergebnis in Dokumente/Werte.md eintragen.

Wege, die Map anzulegen (je nach Engine-Version und Commandlet, der erste funktionierende zaehlt):
  1. unreal.LevelEditorSubsystem.new_level(Pfad)            legt an, speichert leer, laedt sie
  2. unreal.EditorLoadingAndSavingUtils.new_blank_map(False) + save_map(Welt, Pfad)
  Vorhandene Map: loeschen und neu anlegen; geht das nicht (z. B. im Editor gerade geoeffnet), wird sie
  geladen und bis auf WorldSettings/Brush geleert.
Actors entstehen mit EditorActorSubsystem.spawn_actor_from_class; gespeichert wird mit
LevelEditorSubsystem.save_current_level (Rueckfall EditorLoadingAndSavingUtils.save_map).
"""

import os
import sys


def _skriptordner():
    try:
        return os.path.dirname(os.path.abspath(__file__))
    except NameError:
        import unreal
        return os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Tools", "Python")


if _skriptordner() not in sys.path:
    sys.path.insert(0, _skriptordner())

import unreal  # noqa: E402

import mb_unreal as mb  # noqa: E402

MAP_PFAD = "/Game/Maps/M_Graubox"
MARKE = "MB_Graubox"
ORDNER_IM_LEVEL = "Licht"

SONNE_ORT = (0.0, 0.0, 2000.0)
SONNE_NEIGUNG = -50.0       # Pitch in Grad (README-Vorgabe ca. -50)
SONNE_GIER = 35.0           # Yaw in Grad, wie der Licht-Rueckfall im GameMode
SONNE_LUX = 10.0            # Unreal-Standard fuer Richtungslicht (lx)
HIMMEL_ORT = (0.0, 0.0, 300.0)
HIMMEL_INTENSITAET = 1.0
NEBEL_DICHTE = 0.01         # Unreal-Standard 0.02 - etwas klarer fuer den TV-Look
NEBEL_ABFALL = 0.2
BELICHTUNG_EV100 = 2.0      # Startwert, siehe Kopfkommentar [lokal pruefen]

#: Klassen, die dieses Skript in die Map setzt (Klassenname, Label)
VERWALTET = (
    ("DirectionalLight", "Sonne"),
    ("SkyLight", "Himmelslicht"),
    ("SkyAtmosphere", "Atmosphaere"),
    ("ExponentialHeightFog", "Dunst"),
    ("PostProcessVolume", "Bildlook_TV"),
)


# ---------------------------------------------------------------------------------------------------------------------
# Map anlegen, laden, leeren, speichern
# ---------------------------------------------------------------------------------------------------------------------

def map_vorhanden():
    return mb.asset_existiert(MAP_PFAD)


def _aktuelle_map():
    welt = mb.editor_welt(pflicht=False)
    if welt is None:
        return None
    try:
        return mb.paket_pfad(welt.get_path_name())
    except Exception:
        return None


def _neue_map():
    """Legt die Map neu an und laedt sie. Rueckgabe: Name des benutzten Wegs."""
    level = mb.level_subsystem()
    if level is not None:
        try:
            if level.new_level(MAP_PFAD):
                return "new_level"
            mb.hinweis("LevelEditorSubsystem.new_level('%s') lieferte False - versuche new_blank_map." % MAP_PFAD)
        except Exception as ausnahme:
            mb.hinweis("LevelEditorSubsystem.new_level: %s - versuche new_blank_map." % mb.ausnahme_text(ausnahme))
    else:
        mb.hinweis("LevelEditorSubsystem nicht verfuegbar - versuche new_blank_map.")
    try:
        welt = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    except Exception as ausnahme:
        mb.abbrechen("Neue Map nicht anlegbar: %s" % mb.ausnahme_text(ausnahme))
    if welt is None:
        mb.abbrechen("Neue Map nicht anlegbar (new_blank_map lieferte None).")
    return "new_blank_map"


def _map_laden():
    level = mb.level_subsystem()
    if level is not None:
        try:
            if level.load_level(MAP_PFAD):
                return
        except Exception as ausnahme:
            mb.hinweis("LevelEditorSubsystem.load_level: %s" % mb.ausnahme_text(ausnahme))
    try:
        welt = unreal.EditorLoadingAndSavingUtils.load_map(MAP_PFAD)
    except Exception as ausnahme:
        mb.abbrechen("%s nicht ladbar: %s" % (MAP_PFAD, mb.ausnahme_text(ausnahme)))
    if welt is None:
        mb.abbrechen("%s nicht ladbar." % MAP_PFAD)


def _alle_actors():
    try:
        return list(mb.actor_subsystem().get_all_level_actors())
    except Exception as ausnahme:
        mb.abbrechen("Actors der Map nicht lesbar: %s" % ausnahme)


def _map_leeren():
    """Entfernt alle Actors ausser WorldSettings und Brush (die Map ist komplett skriptgeneriert)."""
    behalten = tuple(k for k in (getattr(unreal, "WorldSettings", None), getattr(unreal, "Brush", None)) if k)
    weg = [a for a in _alle_actors() if not isinstance(a, behalten)]
    if not weg:
        return 0
    subsystem = mb.actor_subsystem()
    try:
        subsystem.destroy_actors(weg)
    except Exception:
        for actor in weg:
            try:
                subsystem.destroy_actor(actor)
            except Exception as ausnahme:
                mb.hinweis("Actor %s nicht entfernbar: %s" % (actor.get_name(), ausnahme))
    rest = [a for a in _alle_actors() if not isinstance(a, behalten)]
    if rest:
        mb.abbrechen("Map liess sich nicht leeren, uebrig: %s" % ", ".join(a.get_name() for a in rest))
    return len(weg)


def _map_bereitstellen(neu):
    """Sorgt dafuer, dass MAP_PFAD als leere Map geladen ist. Rueckgabe: Weg (fuer Speichern und Meldungen)."""
    if map_vorhanden():
        if _aktuelle_map() != MAP_PFAD:
            try:
                if mb.asset_subsystem().delete_asset(MAP_PFAD):
                    mb.info("Alte Map geloescht: %s" % MAP_PFAD)
            except Exception as ausnahme:
                mb.hinweis("Alte Map nicht loeschbar (%s) - sie wird geleert." % ausnahme)
    if not map_vorhanden():
        return _neue_map()
    _map_laden()
    entfernt = _map_leeren()
    mb.info("%s geladen und geleert (%d Actors entfernt)." % (MAP_PFAD, entfernt))
    return "geleert"


def _speichern(weg):
    welt = mb.editor_welt()
    ok = False
    if weg != "new_blank_map":
        level = mb.level_subsystem()
        if level is not None:
            try:
                ok = bool(level.save_current_level())
            except Exception as ausnahme:
                mb.hinweis("save_current_level: %s" % mb.ausnahme_text(ausnahme))
    if not ok:
        try:
            ok = bool(unreal.EditorLoadingAndSavingUtils.save_map(welt, MAP_PFAD))
        except Exception as ausnahme:
            mb.abbrechen("Map nicht speicherbar: %s" % mb.ausnahme_text(ausnahme))
    if not ok:
        mb.abbrechen("Map %s konnte nicht gespeichert werden." % MAP_PFAD)


# ---------------------------------------------------------------------------------------------------------------------
# Actors
# ---------------------------------------------------------------------------------------------------------------------

def _vektor(xyz):
    return unreal.Vector(float(xyz[0]), float(xyz[1]), float(xyz[2]))


def _drehung(neigung=0.0, gier=0.0, rollen=0.0):
    # Achtung: unreal.Rotator(roll, pitch, yaw) - deshalb nur mit Schluesselwoertern
    return unreal.Rotator(roll=float(rollen), pitch=float(neigung), yaw=float(gier))


def _setzen(objekt, eigenschaft, wert, methode=None, pflicht=True):
    """Erst set_editor_property, sonst die Blueprint-Funktion (z. B. set_intensity)."""
    if mb.setze(objekt, eigenschaft, wert, pflicht=False):
        return True
    if methode and hasattr(objekt, methode):
        try:
            getattr(objekt, methode)(wert)
            mb.info("%s: %s ueber %s() gesetzt." % (mb.beschreibung(objekt), eigenschaft, methode))
            return True
        except Exception as ausnahme:
            text = "%s: %s() fehlgeschlagen: %s" % (mb.beschreibung(objekt), methode, ausnahme)
    else:
        text = "%s: %s nicht setzbar." % (mb.beschreibung(objekt), eigenschaft)
    if pflicht:
        mb.abbrechen(text)
    mb.hinweis(text)
    return False


def _spawnen(klassenname, label, ort=(0.0, 0.0, 0.0), drehung=None):
    klasse = getattr(unreal, klassenname, None)
    if klasse is None:
        mb.abbrechen("unreal.%s gibt es nicht." % klassenname)
    actor = mb.actor_subsystem().spawn_actor_from_class(klasse, _vektor(ort), drehung or _drehung())
    if actor is None:
        mb.abbrechen("%s konnte nicht in die Map gesetzt werden." % klassenname)
    try:
        actor.set_actor_label(label)
    except Exception as ausnahme:
        mb.hinweis("%s: Label nicht setzbar (%s)" % (klassenname, ausnahme))
    marke = unreal.Name(MARKE) if hasattr(unreal, "Name") else MARKE
    mb.setze(actor, "tags", [marke], pflicht=False, was=label)
    try:
        actor.set_folder_path(ORDNER_IM_LEVEL)
    except Exception:
        pass
    return actor


def _komponente(actor, klassenname):
    klasse = getattr(unreal, klassenname, None)
    komponente = actor.get_component_by_class(klasse) if klasse is not None else None
    if komponente is None:
        mb.abbrechen("%s hat keine %s." % (actor.get_name(), klassenname))
    return komponente


def _beweglich(komponente):
    beweglich = mb.enum_wert("ComponentMobility", "MOVABLE")
    try:
        komponente.set_mobility(beweglich)
    except Exception:
        mb.setze(komponente, "mobility", beweglich, was=mb.beschreibung(komponente))


def _sonne():
    actor = _spawnen("DirectionalLight", "Sonne", SONNE_ORT, _drehung(SONNE_NEIGUNG, SONNE_GIER))
    licht = _komponente(actor, "DirectionalLightComponent")
    _beweglich(licht)
    _setzen(licht, "intensity", SONNE_LUX, "set_intensity")
    _setzen(licht, "atmosphere_sun_light", True, "set_atmosphere_sun_light")
    _setzen(licht, "atmosphere_sun_light_index", 0, "set_atmosphere_sun_light_index", pflicht=False)
    mb.setze(licht, "cast_shadows", True, pflicht=False)
    return actor


def _himmelslicht():
    actor = _spawnen("SkyLight", "Himmelslicht", HIMMEL_ORT)
    licht = _komponente(actor, "SkyLightComponent")
    _beweglich(licht)
    quelle = mb.enum_wert("SkyLightSourceType", "SLS_CAPTURED_SCENE", pflicht=False)
    if quelle is not None:
        mb.setze(licht, "source_type", quelle, pflicht=False)
    _setzen(licht, "real_time_capture", True, "set_real_time_capture")
    _setzen(licht, "intensity", HIMMEL_INTENSITAET, "set_intensity", pflicht=False)
    return actor


def _atmosphaere():
    return _spawnen("SkyAtmosphere", "Atmosphaere")


def _nebel():
    actor = _spawnen("ExponentialHeightFog", "Dunst")
    nebel = _komponente(actor, "ExponentialHeightFogComponent")
    _setzen(nebel, "fog_density", NEBEL_DICHTE, "set_fog_density", pflicht=False)
    _setzen(nebel, "fog_height_falloff", NEBEL_ABFALL, "set_fog_height_falloff", pflicht=False)
    return actor


def _bildlook(ev100):
    actor = _spawnen("PostProcessVolume", "Bildlook_TV")
    mb.setze(actor, "unbound", True, was="PostProcessVolume")
    mb.setze(actor, "blend_weight", 1.0, pflicht=False, was="PostProcessVolume")
    werte = mb.lies(actor, "settings")
    if werte is None:
        mb.abbrechen("PostProcessVolume.settings nicht lesbar.")
    was = "PostProcessSettings"
    # Feste Belichtung: Manual ohne physikalische Kamera = EV100 0, Korrektur -ev100 (siehe Kopfkommentar)
    mb.setze_alle(werte, [("override_auto_exposure_method", True),
                          ("auto_exposure_method", mb.enum_wert("AutoExposureMethod", "AEM_MANUAL")),
                          ("override_auto_exposure_bias", True),
                          ("auto_exposure_bias", -float(ev100))], was=was)
    mb.setze_alle(werte, [("override_auto_exposure_apply_physical_camera_exposure", True),
                          ("auto_exposure_apply_physical_camera_exposure", False),
                          # Sicherung: auch die Auto-Grenzen fest auf denselben Wert (EV100)
                          ("override_auto_exposure_min_brightness", True),
                          ("auto_exposure_min_brightness", float(ev100)),
                          ("override_auto_exposure_max_brightness", True),
                          ("auto_exposure_max_brightness", float(ev100))], pflicht=False, was=was)
    mb.setze(actor, "settings", werte, was="PostProcessVolume")
    return actor


def _pruefen():
    """Jede verwaltete Klasse genau einmal in der Map; Map-Datei vorhanden."""
    zaehler = {}
    for actor in _alle_actors():
        for klassenname, _label in VERWALTET:
            klasse = getattr(unreal, klassenname, None)
            if klasse is not None and isinstance(actor, klasse):
                zaehler[klassenname] = zaehler.get(klassenname, 0) + 1
    falsch = ["%s x%d" % (k, zaehler.get(k, 0)) for k, _l in VERWALTET if zaehler.get(k, 0) != 1]
    if falsch:
        mb.abbrechen("Map-Inhalt unerwartet: %s" % ", ".join(falsch))
    if not map_vorhanden():
        mb.abbrechen("%s fehlt nach dem Speichern." % MAP_PFAD)
    datei = mb.pfade().content / "Maps" / "M_Graubox.umap"
    if not datei.is_file():
        mb.hinweis("Map-Datei %s nicht gefunden (anderer Content-Ordner?)." % datei)
    return sum(zaehler.values())


# ---------------------------------------------------------------------------------------------------------------------
# Ablauf
# ---------------------------------------------------------------------------------------------------------------------

def ausfuehren(neu=True, ev100=BELICHTUNG_EV100):
    """Baut M_Graubox. neu=False: eine vorhandene Map bleibt unangetastet. Rueckgabe: Kurztext."""
    if map_vorhanden() and not neu:
        mb.info("%s existiert schon - nichts zu tun (Neuaufbau: --neu)." % MAP_PFAD)
        return "vorhanden, unveraendert"
    mb.ordner_anlegen(mb.ordner_von(MAP_PFAD))
    weg = _map_bereitstellen(neu)
    mb.info("Map bereit (%s): %s" % (weg, _aktuelle_map()))
    if _aktuelle_map() != MAP_PFAD and weg != "new_blank_map":
        mb.abbrechen("Geladen ist %s statt %s." % (_aktuelle_map(), MAP_PFAD))

    _sonne()
    _himmelslicht()
    _atmosphaere()
    _nebel()
    _bildlook(ev100)
    _speichern(weg)
    anzahl = _pruefen()
    text = "%d Actors (Sonne, Himmelslicht, Atmosphaere, Dunst, Bildlook EV100 %.1f), Weg: %s" % (anzahl, ev100, weg)
    mb.info("%s gespeichert: %s" % (MAP_PFAD, text))
    return text


def main(argv=None):
    p = mb.parser("Map /Game/Maps/M_Graubox neu aufbauen")
    p.add_argument("--ev100", type=float, default=BELICHTUNG_EV100,
                   help="feste Belichtung in EV100 (Standard %.1f; kleiner = heller)" % BELICHTUNG_EV100)
    argumente = mb.argumente_lesen(p, argv)
    mb.umgebung_melden()
    return ausfuehren(neu=True, ev100=argumente.ev100)


if __name__ == "__main__":
    mb.als_skript_ausfuehren(main, "Map M_Graubox bauen (build_graubox.py)")
