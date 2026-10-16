#Requires -Version 5.1
<#
.SYNOPSIS
    Kompiliert Matchball Tennis (README 2.4): Editor-Ziel, optional Spiel-Ziel.

.DESCRIPTION
    Ruft das Unreal Build Tool ueber Engine\Build\BatchFiles\Build.bat auf:
      Build.bat MatchballTennisEditor Win64 Development -Project="<uproject>" -WaitMutex
    Mit -Spiel zusaetzlich das Ziel MatchballTennis (Win64 Development).
    Der Exit-Code des Build Tools wird weitergegeben (0 = Erfolg).
    Logs: Unreal\Saved\Werkzeuge\build-*.log

    Nach einem Wechsel zwischen PCs (git pull) einfach dieses Skript ausfuehren - es erzeugt
    Binaries/ und Intermediate/ neu (README 0.0).

.PARAMETER Spiel
    Zusaetzlich das Spiel-Ziel MatchballTennis (Win64 Development) kompilieren.

.PARAMETER Projektdateien
    Vorher die Visual-Studio-Projektdateien erzeugen (MatchballTennis.sln, entspricht
    "Generate Visual Studio project files" im Explorer-Kontextmenue).

.PARAMETER Sauber
    Vorher die Ordner Binaries/ und Intermediate/ des Projekts loeschen (kompletter Neubau).

.EXAMPLE
    powershell -NoProfile -ExecutionPolicy Bypass -File Unreal\Tools\build.ps1
.EXAMPLE
    powershell -NoProfile -ExecutionPolicy Bypass -File Unreal\Tools\build.ps1 -Sauber -Projektdateien -Spiel
#>
[CmdletBinding()]
param(
    [switch] $Spiel,
    [switch] $Projektdateien,
    [switch] $Sauber
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'MBGemeinsam.psm1') -Force -DisableNameChecking

$ergebnis = 0
try {
    $pfade = Get-MBPfade
    $gesamtStart = Get-Date
    Write-MBTitel 'Matchball Tennis - Kompilieren (build.ps1)'

    if (-not (Test-MBWindows)) {
        Stop-MBMitFehler 'build.ps1 laeuft nur unter Windows (Unreal Build Tool mit Visual Studio).' -Hinweise @('In einer reinen Cloud-Sitzung wird nicht gebaut - lokal ausfuehren (README 0.0a).')
    }
    if (-not (Test-Path -LiteralPath $pfade.UProject -PathType Leaf)) {
        Stop-MBMitFehler ('Projektdatei nicht gefunden: {0}' -f $pfade.UProject)
    }
    $ue = Find-MBUnreal -Pflicht
    Write-MBInfo ('Projekt:        {0}' -f $pfade.UProject)
    Write-MBInfo ('Unreal Engine:  {0} (Version {1}, {2})' -f $ue.Wurzel, $ue.Version, $ue.Quelle)

    # --- Sauber: Binaries/ und Intermediate/ loeschen ---------------------------------------------
    if ($Sauber) {
        Write-MBSchritt 'Loesche Binaries/ und Intermediate/ (kompletter Neubau)'
        $ordnerListe = @($pfade.Binaries, $pfade.Intermediate)
        if (Test-Path -LiteralPath $pfade.Plugins -PathType Container) {
            foreach ($plugin in (Get-ChildItem -LiteralPath $pfade.Plugins -Directory)) {
                $ordnerListe += (Join-Path $plugin.FullName 'Binaries')
                $ordnerListe += (Join-Path $plugin.FullName 'Intermediate')
            }
        }
        foreach ($ordner in $ordnerListe) {
            if (-not (Test-Path -LiteralPath $ordner -PathType Container)) { continue }
            # Sicherheitsnetz: nur innerhalb von Unreal/ loeschen
            if (-not (Test-MBPfadUnter -Basis $pfade.Unreal -Pfad $ordner) -or $ordner.TrimEnd('\') -eq $pfade.Unreal.TrimEnd('\')) {
                Stop-MBMitFehler ('Ordner liegt nicht im Projekt, wird nicht geloescht: {0}' -f $ordner)
            }
            try {
                Remove-Item -LiteralPath $ordner -Recurse -Force
                Write-MBInfo ('geloescht: {0}' -f $ordner)
            } catch {
                Stop-MBMitFehler ('Ordner konnte nicht geloescht werden: {0}' -f $ordner) -Hinweise @(
                    $_.Exception.Message,
                    'Unreal Editor, Visual Studio und laufende Spiele schliessen und erneut versuchen.')
            }
        }
    }

    # --- Projektdateien erzeugen -----------------------------------------------------------------
    if ($Projektdateien) {
        Write-MBSchritt 'Erzeuge Visual-Studio-Projektdateien'
        $argumente = @('-ProjectFiles', (Format-MBSchalter 'Project' $pfade.UProject), '-Game', '-Progress')
        if ($ue.Installiert) { $argumente += '-Rocket' }
        [void](Start-MBProzess -Datei $ue.BuildBat -Argumente $argumente -LogDatei (Get-MBLogDatei 'build-projektdateien') -Beschreibung 'Erzeugen der Projektdateien' -Arbeitsordner $pfade.Unreal)
        Write-MBOk ('Projektdateien erzeugt: {0}' -f (Join-Path $pfade.Unreal 'MatchballTennis.sln'))
    }

    # --- Kompilieren -------------------------------------------------------------------------------
    $ziele = @('MatchballTennisEditor')
    if ($Spiel) { $ziele += 'MatchballTennis' }
    $fertig = @()
    foreach ($ziel in $ziele) {
        Write-MBSchritt ('Kompiliere {0} (Win64 Development)' -f $ziel)
        $argumente = @($ziel, 'Win64', 'Development', (Format-MBSchalter 'Project' $pfade.UProject), '-WaitMutex')
        $log = Get-MBLogDatei ('build-' + $ziel)
        $code = Start-MBProzess -Datei $ue.BuildBat -Argumente $argumente -LogDatei $log -Beschreibung ('Kompilieren von ' + $ziel) -Arbeitsordner $pfade.Unreal -KeinAbbruch
        if ($code -ne 0) {
            Stop-MBMitFehler ('Kompilieren von {0} ist fehlgeschlagen (Exit-Code {1}).' -f $ziel, $code) -ExitCode $code -Hinweise @(
                'Die erste Zeile mit "error" weiter oben nennt Datei und Zeile.',
                ('Vollstaendiges Log: {0}' -f $log),
                'Laeuft der Unreal Editor mit Live Coding? Dann Editor schliessen und erneut bauen.',
                'Nach groesseren Aenderungen hilft oft: Tools\build.ps1 -Sauber')
        }
        Write-MBOk ('{0} kompiliert.' -f $ziel)
        $fertig += $ziel
    }

    Write-Host ''
    Write-MBOk ('Fertig nach {0}: {1}' -f (Format-MBDauer ((Get-Date) - $gesamtStart)), ($fertig -join ', '))
    Write-MBInfo 'Naechste Schritte: Tools\test.ps1 (Tests), Tools\assets.ps1 (Assets), Editor mit MatchballTennis.uproject starten.'
} catch {
    $ergebnis = Write-MBAbbruch $_
}
exit $ergebnis
