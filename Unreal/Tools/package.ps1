#Requires -Version 5.1
<#
.SYNOPSIS
    Packt Matchball Tennis als Shipping-Build (README 2.4, 15.3, 15.4, 19.4.1).

.DESCRIPTION
    1. RunUAT.bat BuildCookRun -project=<uproject> -noP4 -platform=Win64 -clientconfig=Shipping
         -build -cook -stage -pak -iostore -prereqs -archive -archivedirectory=<Ziel>
       Ziel (Standard): C:\MatchballTennis-Build\<Version>\  (ausserhalb des Repositorys, nie einchecken)
       Version: ProjectVersion aus Config/DefaultGame.ini. Das Spiel liegt danach in <Ziel>\Windows\.
    2. Nachbearbeitung im Spielordner <Ziel>\Windows\:
       - "MatchballTennis.exe" wird in "Matchball Tennis.exe" umbenannt (Start-Programm, README 2.1).
       - Liegt "Matchball Tennis Updater.exe" unter Unreal\Updater\bin\, wird er mitkopiert (ab M10).
       - Build-Id (README 15.3): SHA-256 jeder .pak/.utoc/.ucas/.exe-Datei (ohne den Updater);
         je Datei eine Zeile "<pfad relativ, mit />, Kleinbuchstaben>|<sha256 klein>", Zeilen ordinal sortiert,
         jede Zeile mit "\n" abgeschlossen, davon SHA-256 (UTF-8) -> die ersten 12 Hex-Zeichen.
       - build.json:      { "version": "1.0.0", "build": "6e5689499cda", "datum": "2026-10-16" }
         (liest das Spiel beim Start, siehe Source/MatchballTennis/Core/MBGameInstance.cpp)
       - installiert.json: { "version", "build", "datum", "dateien": [ { "pfad", "groesse", "sha256" } ] }
         alle Dateien des Spielordners ausser installiert.json selbst (README 19.4.4, fuer den Updater).
    3. Mit -Zip: "Matchball Tennis <Version> (<Build>).zip" auf den Desktop (README 15.4) - enthaelt nur den
       Spielordner (Ordner "Matchball Tennis" in der ZIP), keine Quelldateien.

.PARAMETER Ziel
    Ausgabeordner (Standard C:\MatchballTennis-Build\<Version>). Muss ausserhalb des Repositorys liegen.

.PARAMETER Zip
    Zusaetzlich die ZIP fuer Freunde erzeugen (Desktop).

.PARAMETER ZipOrdner
    Anderer Ordner fuer die ZIP statt des Desktops.

.PARAMETER NurNachbearbeitung
    Nicht neu packen, nur einen vorhandenen Ausgabeordner nachbearbeiten (umbenennen, build.json,
    installiert.json, ZIP).

.EXAMPLE
    powershell -NoProfile -ExecutionPolicy Bypass -File Unreal\Tools\package.ps1 -Zip
#>
[CmdletBinding()]
param(
    [string] $Ziel = '',
    [switch] $Zip,
    [string] $ZipOrdner = '',
    [switch] $NurNachbearbeitung
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'MBGemeinsam.psm1') -Force -DisableNameChecking

$script:StartExeAlt = 'MatchballTennis.exe'
$script:StartExeNeu = 'Matchball Tennis.exe'
$script:UpdaterExe = 'Matchball Tennis Updater.exe'

function Get-HashListe {
    # SHA-256 aller Dateien des Spielordners (mit Fortschritt). Ergebnis: Liste mit Pfad (relativ), Voll, Groesse, Sha256
    param([string] $Ordner, [string[]] $Auslassen = @())
    $dateien = @(Get-ChildItem -LiteralPath $Ordner -Recurse -File -Force | Where-Object { $Auslassen -notcontains $_.Name })
    $gesamt = 0L
    foreach ($d in $dateien) { $gesamt += $d.Length }
    $erledigt = 0L
    $liste = New-Object System.Collections.Generic.List[object]
    $nummer = 0
    foreach ($d in $dateien) {
        $nummer++
        $prozent = 100
        if ($gesamt -gt 0) { $prozent = [int][Math]::Min(100, ($erledigt * 100 / $gesamt)) }
        Write-Progress -Activity 'Pruefsummen berechnen (SHA-256)' -Status ('{0} von {1}: {2}' -f $nummer, $dateien.Count, $d.Name) -PercentComplete $prozent
        $liste.Add([pscustomobject]@{
                Pfad    = ConvertTo-MBRelativerPfad -Basis $Ordner -Pfad $d.FullName
                Voll    = $d.FullName
                Groesse = [long]$d.Length
                Sha256  = Get-MBDateiHash $d.FullName
            })
        $erledigt += $d.Length
    }
    Write-Progress -Activity 'Pruefsummen berechnen (SHA-256)' -Completed
    return $liste
}

function Get-BuildId {
    # Build-Id (README 15.3) aus den Pruefsummen der .pak/.utoc/.ucas/.exe-Dateien, ohne den Updater
    param($HashListe)
    $zeilen = New-Object System.Collections.Generic.List[string]
    foreach ($e in $HashListe) {
        $endung = [System.IO.Path]::GetExtension($e.Pfad).ToLowerInvariant()
        if (@('.pak', '.utoc', '.ucas', '.exe') -notcontains $endung) { continue }
        if ([System.IO.Path]::GetFileName($e.Pfad) -eq $script:UpdaterExe) { continue }
        $zeilen.Add(('{0}|{1}' -f $e.Pfad.ToLowerInvariant(), $e.Sha256))
    }
    $zeilen.Sort([System.StringComparer]::Ordinal)
    $text = ''
    foreach ($z in $zeilen) { $text += $z + "`n" }
    return [pscustomobject]@{ Id = (Get-MBTextHash $text).Substring(0, 12); Anzahl = $zeilen.Count }
}

function New-SpielZip {
    param([string] $Spielordner, [string] $ZipPfad)
    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    if (Test-Path -LiteralPath $ZipPfad) { Remove-Item -LiteralPath $ZipPfad -Force }
    $temp = $ZipPfad + '.tmp'
    if (Test-Path -LiteralPath $temp) { Remove-Item -LiteralPath $temp -Force }
    $strom = [System.IO.File]::Open($temp, [System.IO.FileMode]::CreateNew)
    try {
        $archiv = New-Object System.IO.Compression.ZipArchive -ArgumentList $strom, ([System.IO.Compression.ZipArchiveMode]::Create)
        try {
            $nummer = 0
            $alle = @(Get-ChildItem -LiteralPath $Spielordner -Recurse -File -Force)
            foreach ($d in $alle) {
                $nummer++
                $relativ = ConvertTo-MBRelativerPfad -Basis $Spielordner -Pfad $d.FullName
                Write-Progress -Activity 'ZIP erzeugen' -Status ('{0} von {1}: {2}' -f $nummer, $alle.Count, $relativ) -PercentComplete ([int]($nummer * 100 / [Math]::Max(1, $alle.Count)))
                # .pak/.ucas sind schon komprimiert - nur speichern geht viel schneller
                $stufe = [System.IO.Compression.CompressionLevel]::Optimal
                $endung = $d.Extension.ToLowerInvariant()
                if ($endung -eq '.pak' -or $endung -eq '.ucas') { $stufe = [System.IO.Compression.CompressionLevel]::NoCompression }
                [void][System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archiv, $d.FullName, ('Matchball Tennis/' + $relativ), $stufe)
            }
        } finally {
            $archiv.Dispose()
        }
    } finally {
        $strom.Dispose()
        Write-Progress -Activity 'ZIP erzeugen' -Completed
    }
    Move-Item -LiteralPath $temp -Destination $ZipPfad -Force
}

$ergebnis = 0
try {
    $pfade = Get-MBPfade
    $gesamtStart = Get-Date
    Write-MBTitel 'Matchball Tennis - Spiel packen (package.ps1)'

    $version = Get-MBProjektVersion
    if ($Ziel -eq '') { $Ziel = 'C:\MatchballTennis-Build\' + $version }
    if (-not [System.IO.Path]::IsPathRooted($Ziel)) {
        Stop-MBMitFehler ('Ziel muss ein absoluter Pfad sein: {0}' -f $Ziel)
    }
    $Ziel = [System.IO.Path]::GetFullPath($Ziel).TrimEnd('\', '/')
    if ((Test-MBPfadUnter -Basis $pfade.Repo -Pfad $Ziel) -or (Test-MBPfadUnter -Basis $Ziel -Pfad $pfade.Repo)) {
        Stop-MBMitFehler ('Das Ziel darf nicht im Repository liegen: {0}' -f $Ziel) -Hinweise @('Gepackte Builds gehoeren nach C:\MatchballTennis-Build\ (README 0.0) und werden nie eingecheckt.')
    }
    $spielOrdner = Join-Path $Ziel 'Windows'
    Write-MBInfo ('Version:  {0}' -f $version)
    Write-MBInfo ('Ziel:     {0}' -f $Ziel)

    # --- 1. Bauen, kochen, packen ------------------------------------------------------------------
    if (-not $NurNachbearbeitung) {
        if (-not (Test-MBWindows)) {
            Stop-MBMitFehler 'package.ps1 laeuft nur unter Windows (Unreal Automation Tool).' -Hinweise @('Mit -NurNachbearbeitung laesst sich ein vorhandener Ausgabeordner nachbearbeiten.')
        }
        $ue = Find-MBUnreal -Pflicht
        Write-MBInfo ('Unreal:   {0} (Version {1})' -f $ue.Wurzel, $ue.Version)
        if (Test-Path -LiteralPath $spielOrdner) {
            Write-MBSchritt ('Loesche alten Spielordner {0}' -f $spielOrdner)
            try {
                Remove-Item -LiteralPath $spielOrdner -Recurse -Force
            } catch {
                Stop-MBMitFehler ('Alter Spielordner konnte nicht geloescht werden: {0}' -f $spielOrdner) -Hinweise @($_.Exception.Message, 'Laeuft das Spiel noch? Dann beenden und erneut versuchen.')
            }
        }
        Write-MBSchritt 'Bauen, Kochen und Packen (RunUAT BuildCookRun, Shipping) - das dauert beim ersten Mal lange'
        $argumente = @(
            'BuildCookRun',
            (Format-MBSchalter 'project' $pfade.UProject),
            '-noP4', '-platform=Win64', '-clientconfig=Shipping',
            '-build', '-cook', '-stage', '-pak', '-iostore', '-prereqs', '-archive',
            (Format-MBSchalter 'archivedirectory' $Ziel),
            '-nodebuginfo', '-utf8output', '-unattended')
        $log = Get-MBLogDatei 'package'
        $code = Start-MBProzess -Datei $ue.RunUAT -Argumente $argumente -LogDatei $log -Beschreibung 'Packen (BuildCookRun)' -Arbeitsordner $pfade.Unreal -KeinAbbruch
        if ($code -ne 0) {
            Stop-MBMitFehler ('Packen ist fehlgeschlagen (Exit-Code {0}).' -f $code) -ExitCode $code -Hinweise @(
                'Die erste Zeile mit "ERROR" weiter oben nennt die Ursache.',
                ('Vollstaendiges Log: {0}' -f $log),
                'Weitere Logs des Automation Tools: %AppData%\Unreal Engine\AutomationTool\Logs\',
                'Unreal Editor vorher schliessen; ist das Editor-Ziel aktuell (Tools\build.ps1)?')
        }
        Write-MBOk 'Packen erfolgreich.'
    }

    # --- 2. Nachbearbeitung ----------------------------------------------------------------------
    Write-MBSchritt ('Nachbearbeitung im Spielordner {0}' -f $spielOrdner)
    if (-not (Test-Path -LiteralPath $spielOrdner -PathType Container)) {
        Stop-MBMitFehler ('Kein gepacktes Spiel gefunden: {0}' -f $spielOrdner) -Hinweise @('Erst ohne -NurNachbearbeitung packen.')
    }
    $alt = Join-Path $spielOrdner $script:StartExeAlt
    $neu = Join-Path $spielOrdner $script:StartExeNeu
    if (Test-Path -LiteralPath $alt -PathType Leaf) {
        if (Test-Path -LiteralPath $neu) { Remove-Item -LiteralPath $neu -Force }
        Move-Item -LiteralPath $alt -Destination $neu
        Write-MBOk ('"{0}" in "{1}" umbenannt.' -f $script:StartExeAlt, $script:StartExeNeu)
    } elseif (Test-Path -LiteralPath $neu -PathType Leaf) {
        Write-MBInfo ('"{0}" ist schon umbenannt.' -f $script:StartExeNeu)
    } else {
        $exes = @(Get-ChildItem -LiteralPath $spielOrdner -Filter '*.exe' -File | ForEach-Object { $_.Name })
        Stop-MBMitFehler ('Start-Programm "{0}" fehlt im Spielordner.' -f $script:StartExeAlt) -Hinweise @(('Gefundene .exe: {0}' -f ($exes -join ', ')))
    }

    $updaterQuelle = Join-MBPfad $pfade.Updater 'bin' $script:UpdaterExe
    if (Test-Path -LiteralPath $updaterQuelle -PathType Leaf) {
        Copy-Item -LiteralPath $updaterQuelle -Destination (Join-Path $spielOrdner $script:UpdaterExe) -Force
        Write-MBOk ('Updater mitkopiert: {0}' -f $script:UpdaterExe)
    } else {
        Write-MBInfo ('Updater noch nicht vorhanden ({0}) - kommt mit Meilenstein M10.' -f $updaterQuelle)
    }

    # Alte Listen entfernen, damit sie nicht in die Pruefsummen eingehen
    foreach ($name in @('build.json', 'installiert.json')) {
        $p = Join-Path $spielOrdner $name
        if (Test-Path -LiteralPath $p) { Remove-Item -LiteralPath $p -Force }
    }

    Write-MBInfo 'Berechne SHA-256 aller Spieldateien ...'
    $hashListe = Get-HashListe -Ordner $spielOrdner
    $build = Get-BuildId -HashListe $hashListe
    $pakAnzahl = @($hashListe | Where-Object { $_.Pfad -match '\.(pak|utoc|ucas)$' }).Count
    if ($pakAnzahl -eq 0) { Write-MBWarnung 'Keine .pak/.utoc/.ucas-Dateien gefunden - wurde wirklich gepackt?' }
    Write-MBOk ('Build-Id: {0} (aus {1} Dateien)' -f $build.Id, $build.Anzahl)

    $datum = (Get-Date).ToString('yyyy-MM-dd')
    $buildJson = [ordered]@{ version = $version; build = $build.Id; datum = $datum }
    $buildDatei = Join-Path $spielOrdner 'build.json'
    Write-MBTextDatei -Pfad $buildDatei -Text (ConvertTo-MBJson $buildJson)
    Write-MBOk 'build.json geschrieben.'

    # installiert.json: alle Dateien inklusive build.json
    $eintraege = New-Object System.Collections.Generic.List[object]
    foreach ($e in $hashListe) { $eintraege.Add($e) }
    $eintraege.Add([pscustomobject]@{ Pfad = 'build.json'; Voll = $buildDatei; Groesse = [long](Get-Item -LiteralPath $buildDatei).Length; Sha256 = Get-MBDateiHash $buildDatei })
    $dateien = New-Object System.Collections.Generic.List[object]
    $gesamtGroesse = 0L
    foreach ($e in ($eintraege | Sort-Object -Property @{ Expression = { $_.Pfad.ToLowerInvariant() } })) {
        $dateien.Add([ordered]@{ pfad = $e.Pfad; groesse = $e.Groesse; sha256 = $e.Sha256 })
        $gesamtGroesse += $e.Groesse
    }
    $installiert = [ordered]@{ version = $version; build = $build.Id; datum = $datum; dateien = $dateien }
    Write-MBTextDatei -Pfad (Join-Path $spielOrdner 'installiert.json') -Text (ConvertTo-MBJson $installiert)
    Write-MBOk ('installiert.json geschrieben ({0} Dateien, {1}).' -f $dateien.Count, (Format-MBGroesse $gesamtGroesse))

    # --- 3. ZIP fuer Freunde -----------------------------------------------------------------------
    $zipPfad = ''
    if ($Zip) {
        Write-MBSchritt 'ZIP fuer Freunde erzeugen'
        if ($ZipOrdner -eq '') { $ZipOrdner = [System.Environment]::GetFolderPath('Desktop') }
        if (-not $ZipOrdner) { $ZipOrdner = $HOME }
        [void](New-MBOrdner $ZipOrdner)
        $zipPfad = Join-Path $ZipOrdner ('Matchball Tennis {0} ({1}).zip' -f $version, $build.Id)
        if (Test-MBPfadUnter -Basis $pfade.Repo -Pfad $zipPfad) {
            Stop-MBMitFehler ('Die ZIP darf nicht im Repository liegen: {0}' -f $zipPfad)
        }
        New-SpielZip -Spielordner $spielOrdner -ZipPfad $zipPfad
        Write-MBOk ('ZIP erzeugt: {0} ({1})' -f $zipPfad, (Format-MBGroesse (Get-Item -LiteralPath $zipPfad).Length))
    }

    Write-MBSchritt 'Zusammenfassung'
    Write-MBInfo ('Version {0} - Build {1}' -f $version, $build.Id)
    Write-MBInfo ('Spiel:   {0}' -f (Join-Path $spielOrdner $script:StartExeNeu))
    Write-MBInfo ('Groesse: {0} in {1} Dateien' -f (Format-MBGroesse $gesamtGroesse), $dateien.Count)
    if ($zipPfad) {
        Write-MBInfo ('ZIP:     {0}' -f $zipPfad)
        Write-MBInfo 'Weitergeben: ZIP entpacken, "Matchball Tennis.exe" starten. Windows warnt evtl. ("Unbekannter Herausgeber")'
        Write-MBInfo '             -> "Weitere Informationen" -> "Trotzdem ausfuehren" (README 15.4).'
    }
    Write-MBOk ('Fertig nach {0}.' -f (Format-MBDauer ((Get-Date) - $gesamtStart)))
} catch {
    $ergebnis = Write-MBAbbruch $_
}
exit $ergebnis
