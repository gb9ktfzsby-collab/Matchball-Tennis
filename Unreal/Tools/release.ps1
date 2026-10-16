#Requires -Version 5.1
<#
.SYNOPSIS
    Update veroeffentlichen (README 19.4.6) - NOCH NICHT UMGESETZT (Meilenstein M10).

.DESCRIPTION
    Aufruf spaeter ueber "Update veroeffentlichen.bat" (Ordner Unreal\). Dieses Geruest meldet nur, dass die
    Funktion noch fehlt, und endet mit Exit-Code 1.

    Geplanter Ablauf (README 19.4.6):
      1. Nach der neuen Versionsnummer fragen (Vorschlag: letzte + 1) und nach den Neuigkeiten
         (eine Zeile pro Punkt). Die Version wird in Config/DefaultGame.ini (ProjectVersion) eingetragen.
      2. Spiel bauen und packen: Tools\package.ps1 (feste Chunk-Aufteilung aus Data/chunks.json, README 19.4.1).
      3. Jeden Chunk ueber die Pruefsumme seiner Quell-Assets mit der letzten Version vergleichen; unveraenderte
         Pakete (.pak/.utoc/.ucas) werden unveraendert aus der letzten Version uebernommen.
      4. SHA-256 aller Dateien berechnen, manifest.json schreiben (version, build, datum, speicherstand_version,
         updater_mindestversion, neuigkeiten, dateien mit pfad/groesse/sha256/url, loeschen) und signieren.
      5. Ueber die GitHub-Kommandozeile (gh release create) nur neue/geaenderte Dateien plus manifest.json als
         neues Release im Update-Repository hochladen (README 19.4.2; unveraenderte Dateien zeigen weiter auf
         das Release, in dem sie zuletzt geaendert wurden).
      6. Zusaetzlich die Komplett-ZIP fuer neue Freunde erzeugen (Tools\package.ps1 -Zip, README 19.3).
      7. Zusammenfassung anzeigen, z. B. "Update 2.1.0 veroeffentlicht - Download fuer bestehende Spieler:
         412 MB von 6,8 GB".

    Signatur (Idee, README 19.4.3):
      - Ed25519-Schluesselpaar mit dem in Windows enthaltenen OpenSSH (ssh-keygen.exe):
          ssh-keygen -t ed25519 -f "%USERPROFILE%\MatchballTennis-Signatur\update_signatur" -C "Matchball Tennis Updates"
        Der private Schluessel liegt NUR ausserhalb des Repositorys (Ordner oben, nie committen, nie in die Cloud);
        release.ps1 erzeugt das Paar beim ersten Mal und erklaert, wo die Sicherungskopie hin muss
        (geht der Schluessel verloren, kann kein Update mehr veroeffentlicht werden).
      - Signieren:
          ssh-keygen -Y sign -f <privater Schluessel> -n matchball-tennis-update manifest.json
        erzeugt manifest.json.sig (SSHSIG-Format). Signiert wird das Manifest ohne das Feld "signatur";
        die Signatur kommt danach in "signatur" (bzw. als manifest.json.sig daneben).
      - Pruefen vor dem Hochladen:
          ssh-keygen -Y verify -f allowed_signers -I matchball-tennis -n matchball-tennis-update -s manifest.json.sig < manifest.json
      - Der oeffentliche Schluessel (update_signatur.pub) wird in Spiel und Updater eingebaut; beide verwerfen ein
        Manifest ohne gueltige Signatur (Ed25519-Pruefung ueber die SSHSIG-Struktur: "SSHSIG", Namensraum,
        Hash-Algorithmus, Hash des Manifests).

.EXAMPLE
    "Update veroeffentlichen.bat" doppelklicken
#>
param(
    [Parameter(ValueFromRemainingArguments = $true)] [string[]] $Rest
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'MBGemeinsam.psm1') -Force -DisableNameChecking

$ergebnis = 0
try {
    Write-MBTitel 'Matchball Tennis - Update veroeffentlichen (release.ps1)'
    if ($null -ne $Rest -and $Rest.Count -gt 0) { Write-MBInfo ('Parameter werden noch nicht ausgewertet: {0}' -f ($Rest -join ' ')) }
    Stop-MBMitFehler 'Noch nicht umgesetzt (Meilenstein M10, README 19.4.6).' -Hinweise @(
        'Fuer die Erstinstallation bei Freunden: "Spiel fuer Freunde packen.bat" (Tools\package.ps1 -Zip).')
} catch {
    $ergebnis = Write-MBAbbruch $_
}
exit $ergebnis
