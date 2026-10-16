#Requires -Version 5.1
<#
.SYNOPSIS
    Baut alle Blender-Assets neu und importiert sie nach Unreal (README 0.2, 3.2).

.DESCRIPTION
    1. Blender (headless) fuer jedes Skript aus Unreal/blender/skripte.json, in dieser Reihenfolge:
         blender -b --factory-startup --python-exit-code 1 -P Unreal/blender/scripts/<skript>.py -- <argumente>
       Format von skripte.json:  [ { "skript": "wuerfel_spieler.py", "argumente": [], "beschreibung": "..." }, ... ]
       Die Skripte schreiben nach Unreal/SourceAssets/_erzeugt/<Bereich>/ und Vorschaubilder nach
       Unreal/SourceAssets/_vorschau/.
    2. Unreal-Import (Editor ohne Grafik):
         UnrealEditor-Cmd.exe MatchballTennis.uproject -run=pythonscript
           -script="<Repo>/Unreal/Tools/Python/import_alles.py" -unattended -nosplash -nullrhi
       Voraussetzung: Tools\build.ps1 wurde ausgefuehrt (Editor-Ziel kompiliert).

    Schlaegt ein Blender-Skript fehl, laufen die uebrigen trotzdem; der Import wird dann ausgelassen.
    Logs: Unreal\Saved\Werkzeuge\assets-<Zeit>\

.PARAMETER NurBlender
    Nur die Blender-Skripte ausfuehren, nicht importieren.

.PARAMETER NurImport
    Nur den Unreal-Import ausfuehren (Blender-Ergebnisse muessen schon vorhanden sein).

.PARAMETER Skript
    Nur dieses eine Blender-Skript ausfuehren (z. B. schlaeger oder schlaeger.py); der Import folgt,
    ausser mit -NurBlender.

.EXAMPLE
    powershell -NoProfile -ExecutionPolicy Bypass -File Unreal\Tools\assets.ps1
.EXAMPLE
    powershell -NoProfile -ExecutionPolicy Bypass -File Unreal\Tools\assets.ps1 -Skript schlaeger -NurBlender
#>
[CmdletBinding()]
param(
    [switch] $NurBlender,
    [switch] $NurImport,
    [string] $Skript = ''
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'MBGemeinsam.psm1') -Force -DisableNameChecking

function Read-SkriptListe {
    param($Pfade)
    if (-not (Test-Path -LiteralPath $Pfade.BlenderSkriptListe -PathType Leaf)) {
        Stop-MBMitFehler ('Skriptliste fehlt: {0}' -f $Pfade.BlenderSkriptListe)
    }
    try {
        $roh = @(Read-MBJson $Pfade.BlenderSkriptListe)
    } catch {
        Stop-MBMitFehler ('Skriptliste ist kein gueltiges JSON: {0}' -f $Pfade.BlenderSkriptListe) -Hinweise @($_.Exception.Message)
    }
    $liste = @()
    $nummer = 0
    foreach ($eintrag in $roh) {
        $nummer++
        $name = [string](Get-MBEigenschaft $eintrag 'skript' '')
        if ($name -eq '' -or $name -notmatch '^[A-Za-z0-9_\-]+\.py$') {
            Stop-MBMitFehler ('Eintrag {0} in skripte.json hat keinen gueltigen Skriptnamen ("{1}").' -f $nummer, $name) -Hinweise @('Erwartet z. B.: { "skript": "ball.py", "argumente": [] }')
        }
        $argumente = @()
        foreach ($a in @(Get-MBEigenschaft $eintrag 'argumente' @())) { if ($null -ne $a) { $argumente += [string]$a } }
        $liste += [pscustomobject]@{
            Skript       = $name
            Argumente    = $argumente
            Beschreibung = [string](Get-MBEigenschaft $eintrag 'beschreibung' '')
            Pfad         = Join-Path $Pfade.BlenderSkripte $name
        }
    }
    return $liste
}

function Get-NeueDateien {
    # Dateien unter den Ausgabeordnern, die seit Start geschrieben wurden (zur Rueckmeldung)
    param($Pfade, [datetime] $Seit)
    $neu = @()
    foreach ($ordner in @($Pfade.Erzeugt, $Pfade.Vorschau)) {
        if (Test-Path -LiteralPath $ordner -PathType Container) {
            $neu += @(Get-ChildItem -LiteralPath $ordner -Recurse -File -ErrorAction SilentlyContinue | Where-Object { $_.LastWriteTime -ge $Seit })
        }
    }
    return $neu
}

$ergebnis = 0
try {
    $pfade = Get-MBPfade
    $gesamtStart = Get-Date
    Write-MBTitel 'Matchball Tennis - Assets bauen und importieren (assets.ps1)'
    if ($NurBlender -and $NurImport) { Stop-MBMitFehler '-NurBlender und -NurImport schliessen sich aus.' }
    $logOrdner = Join-Path $pfade.Werkzeuglogs ('assets-' + (Get-MBZeitstempel))
    [void](New-MBOrdner $logOrdner)
    $zusammenfassung = @()

    # --- 1. Blender ------------------------------------------------------------------------------
    if (-not $NurImport) {
        Write-MBSchritt 'Blender-Skripte (headless)'
        $liste = @(Read-SkriptListe $pfade)
        if ($Skript -ne '') {
            $gesucht = $Skript.Trim()
            if (-not $gesucht.EndsWith('.py')) { $gesucht += '.py' }
            $treffer = @($liste | Where-Object { $_.Skript -eq $gesucht })
            if ($treffer.Count -eq 0) {
                $pfad = Join-Path $pfade.BlenderSkripte $gesucht
                if (Test-Path -LiteralPath $pfad -PathType Leaf) {
                    Write-MBWarnung ('{0} steht nicht in skripte.json - laeuft ohne Argumente.' -f $gesucht)
                    $treffer = @([pscustomobject]@{ Skript = $gesucht; Argumente = @(); Beschreibung = ''; Pfad = $pfad })
                } else {
                    Stop-MBMitFehler ('Skript "{0}" gibt es nicht.' -f $Skript) -Hinweise @(('Vorhanden laut skripte.json: {0}' -f (($liste | ForEach-Object { $_.Skript }) -join ', ')))
                }
            }
            $liste = $treffer
        }
        if ($liste.Count -eq 0) { Stop-MBMitFehler ('Keine Skripte in {0}.' -f $pfade.BlenderSkriptListe) }

        $blender = Find-MBBlender -Pflicht
        $blenderVersion = Get-MBBlenderVersion -Pfad $blender.Pfad
        Write-MBInfo ('Blender: {0} ({1}, {2})' -f $blender.Pfad, $blenderVersion, $blender.Quelle)
        if ($blenderVersion -ne '' -and $blenderVersion -notmatch 'Blender\s+5\.2(\.|\s|$)') {
            Write-MBWarnung ('Das Projekt erwartet Blender 5.2 LTS, gefunden: {0}' -f $blenderVersion)
        }

        $fehler = @()
        $nummer = 0
        foreach ($eintrag in $liste) {
            $nummer++
            $titel = '[{0}/{1}] {2}' -f $nummer, $liste.Count, $eintrag.Skript
            if ($eintrag.Beschreibung) { $titel += ' - ' + $eintrag.Beschreibung }
            Write-MBSchritt $titel
            if (-not (Test-Path -LiteralPath $eintrag.Pfad -PathType Leaf)) {
                Write-MBFehler ('Skript fehlt: {0}' -f $eintrag.Pfad)
                $fehler += $eintrag.Skript
                $zusammenfassung += [pscustomobject]@{ Schritt = $eintrag.Skript; Status = 'FEHLER'; Details = 'Skriptdatei fehlt' }
                continue
            }
            $argumente = @('-b', '--factory-startup', '--python-exit-code', '1', '-P', $eintrag.Pfad, '--') + $eintrag.Argumente
            $log = Join-Path $logOrdner ('blender-{0}.log' -f [System.IO.Path]::GetFileNameWithoutExtension($eintrag.Skript))
            $start = Get-Date
            $code = Start-MBProzess -Datei $blender.Pfad -Argumente $argumente -LogDatei $log -Beschreibung ('Blender ' + $eintrag.Skript) -Arbeitsordner $pfade.Unreal -KeinAbbruch
            $dauer = Format-MBDauer ((Get-Date) - $start)
            if ($code -ne 0) {
                Write-MBFehler ('{0} ist fehlgeschlagen (Exit-Code {1}). Log: {2}' -f $eintrag.Skript, $code, $log)
                $fehler += $eintrag.Skript
                $zusammenfassung += [pscustomobject]@{ Schritt = $eintrag.Skript; Status = 'FEHLER'; Details = ('Exit-Code {0}, {1}' -f $code, $dauer) }
                continue
            }
            $neu = @(Get-NeueDateien -Pfade $pfade -Seit $start)
            foreach ($datei in ($neu | Select-Object -First 15)) {
                Write-MBInfo ('erzeugt: {0}' -f (ConvertTo-MBRelativerPfad -Basis $pfade.Unreal -Pfad $datei.FullName))
            }
            if ($neu.Count -gt 15) { Write-MBInfo ('... und {0} weitere Dateien' -f ($neu.Count - 15)) }
            if ($neu.Count -eq 0) { Write-MBWarnung ('{0} hat unter SourceAssets/_erzeugt und _vorschau nichts geschrieben.' -f $eintrag.Skript) }
            Write-MBOk ('{0} fertig ({1}).' -f $eintrag.Skript, $dauer)
            $zusammenfassung += [pscustomobject]@{ Schritt = $eintrag.Skript; Status = 'OK'; Details = ('{0} Dateien, {1}' -f $neu.Count, $dauer) }
        }
        if ($fehler.Count -gt 0) {
            foreach ($z in $zusammenfassung) { Write-MBInfo ('{0,-8} {1,-28} {2}' -f $z.Status, $z.Schritt, $z.Details) }
            $hinweise = @(('Logs: {0}' -f $logOrdner))
            if (-not $NurBlender) { $hinweise += 'Der Unreal-Import wurde deshalb nicht gestartet.' }
            Stop-MBMitFehler ('{0} von {1} Blender-Skripten sind fehlgeschlagen: {2}' -f $fehler.Count, $liste.Count, ($fehler -join ', ')) -Hinweise $hinweise
        }
    }

    # --- 2. Unreal-Import -------------------------------------------------------------------------
    if (-not $NurBlender) {
        Write-MBSchritt 'Unreal-Import (Tools/Python/import_alles.py)'
        $importSkript = Join-Path $pfade.ToolsPython 'import_alles.py'
        if (-not (Test-Path -LiteralPath $importSkript -PathType Leaf)) {
            Stop-MBMitFehler ('Import-Skript fehlt: {0}' -f $importSkript)
        }
        $ue = Find-MBUnreal -Pflicht
        Assert-MBEditorGebaut
        $unrealLog = Join-Path $logOrdner 'unreal-import.log'
        $konsolenLog = Join-Path $logOrdner 'unreal-import-konsole.log'
        $argumente = @(
            $pfade.UProject,
            '-run=pythonscript',
            (Format-MBSchalter 'script' $importSkript.Replace('\', '/')),
            '-unattended', '-nosplash', '-nullrhi',
            '-stdout', '-FullStdOutLogOutput', '-UTF8Output',
            (Format-MBSchalter 'abslog' $unrealLog))
        $start = Get-Date
        $code = Start-MBProzess -Datei $ue.EditorCmd -Argumente $argumente -LogDatei $konsolenLog -Beschreibung 'Unreal-Import' -Arbeitsordner $pfade.Unreal -KeinAbbruch
        $dauer = Format-MBDauer ((Get-Date) - $start)

        $logDatei = $unrealLog
        if (-not (Test-Path -LiteralPath $logDatei -PathType Leaf)) { $logDatei = $konsolenLog }
        $pythonFehler = @()
        $bilanz = ''
        $bilanzFehlschlag = $false
        if (Test-Path -LiteralPath $logDatei -PathType Leaf) {
            $pythonFehler = @(Select-String -LiteralPath $logDatei -Pattern 'LogPython: Error' -SimpleMatch | ForEach-Object { $_.Line.Trim() })
            $b = Select-String -LiteralPath $logDatei -Pattern '(Success|Failure) - \d+ error\(s\), \d+ warning\(s\)' | Select-Object -Last 1
            if ($null -ne $b) {
                $bilanz = $b.Matches[0].Value
                $bilanzFehlschlag = $bilanz.StartsWith('Failure')
            }
        }
        foreach ($zeile in ($pythonFehler | Select-Object -First 20)) { Write-Host ('          ' + $zeile) -ForegroundColor Red }
        if ($pythonFehler.Count -gt 20) { Write-MBInfo ('... und {0} weitere Python-Fehler' -f ($pythonFehler.Count - 20)) }
        if ($code -ne 0 -or $bilanzFehlschlag) {
            $zusatz = ''
            if ($bilanz) { $zusatz = ', ' + $bilanz }
            Stop-MBMitFehler ('Unreal-Import ist fehlgeschlagen (Exit-Code {0}{1}).' -f $code, $zusatz) -ExitCode $code -Hinweise @(
                ('Log: {0}' -f $logDatei),
                'Python-Fehler stehen im Log unter "LogPython: Error".')
        }
        if ($pythonFehler.Count -gt 0) {
            Write-MBWarnung ('Import beendet, aber das Log enthaelt {0} Python-Fehlermeldungen (siehe oben).' -f $pythonFehler.Count)
            $zusammenfassung += [pscustomobject]@{ Schritt = 'Unreal-Import'; Status = 'WARNUNG'; Details = ('{0} Python-Fehlermeldungen, {1}' -f $pythonFehler.Count, $dauer) }
        } else {
            Write-MBOk ('Unreal-Import fertig ({0}). {1}' -f $dauer, $bilanz)
            $zusammenfassung += [pscustomobject]@{ Schritt = 'Unreal-Import'; Status = 'OK'; Details = $dauer }
        }
    }

    Write-MBSchritt 'Zusammenfassung'
    foreach ($z in $zusammenfassung) {
        $farbe = 'Green'
        if ($z.Status -ne 'OK') { $farbe = 'Yellow' }
        Write-Host ('  {0,-8} {1,-28} {2}' -f $z.Status, $z.Schritt, $z.Details) -ForegroundColor $farbe
    }
    Write-MBInfo ('Logs: {0}' -f $logOrdner)
    Write-MBInfo ('Vorschaubilder: {0}' -f $pfade.Vorschau)
    Write-MBOk ('Fertig nach {0}.' -f (Format-MBDauer ((Get-Date) - $gesamtStart)))
} catch {
    $ergebnis = Write-MBAbbruch $_
}
exit $ergebnis
