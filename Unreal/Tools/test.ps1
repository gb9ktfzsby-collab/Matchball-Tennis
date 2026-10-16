#Requires -Version 5.1
<#
.SYNOPSIS
    Fuehrt die Tests von Matchball Tennis aus (README 0.2): Kern-Tests per CMake und Unreal-Automation-Tests.

.DESCRIPTION
    (a) Kern-Tests ohne Unreal (reines C++ aus Source/MatchballTennis/*/Kern und Source/MatchballTennisTests/Kern):
          cmake -S Unreal/Tools/KernTests -B Unreal/Tools/KernTests/build-win
          cmake --build Unreal/Tools/KernTests/build-win --config Release
          ctest -C Release --output-on-failure
        Fehlt cmake, gibt es einen Hinweis; dieselben Kern-Tests laufen auch in Unreal (MatchballTennis.Kern.*).
    (b) Unreal-Automation-Tests im Editor ohne Grafik:
          UnrealEditor-Cmd.exe MatchballTennis.uproject -ExecCmds="Automation RunTests MatchballTennis; Quit"
            -unattended -nullrhi -nosplash -nosound -NoPause -log -ReportExportPath="Unreal/Saved/Testberichte/<Zeit>/Unreal"
        Das Ergebnis wird aus dem Bericht (index.json) gelesen, ersatzweise aus dem Log.
        Voraussetzung: Tools\build.ps1 wurde ausgefuehrt (Editor-Ziel kompiliert).

    Exit-Code 0 nur, wenn alle ausgefuehrten Tests gruen sind.
    Berichte und Logs: Unreal\Saved\Testberichte\<Zeit>\

.PARAMETER NurKern
    Nur die Kern-Tests per CMake (kein Unreal noetig, laeuft auch ohne Visual-Studio-Projekt).

.PARAMETER NurUnreal
    Nur die Unreal-Automation-Tests.

.PARAMETER Filter
    Filter fuer die Unreal-Tests (Standard "MatchballTennis", z. B. "MatchballTennis.Kern.Zaehlweise").

.PARAMETER TimeoutMinuten
    Unreal-Testlauf nach dieser Zeit abbrechen (Standard 60).

.EXAMPLE
    powershell -NoProfile -ExecutionPolicy Bypass -File Unreal\Tools\test.ps1
.EXAMPLE
    powershell -NoProfile -ExecutionPolicy Bypass -File Unreal\Tools\test.ps1 -NurUnreal -Filter MatchballTennis.Kern.Zaehlweise
#>
[CmdletBinding()]
param(
    [switch] $NurKern,
    [switch] $NurUnreal,
    [string] $Filter = 'MatchballTennis',
    [int] $TimeoutMinuten = 60
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'MBGemeinsam.psm1') -Force -DisableNameChecking

$script:Teile = New-Object System.Collections.Generic.List[object]

function Add-Teilergebnis {
    # Status: OK, FEHLER, UEBERSPRUNGEN
    param([string] $Teil, [string] $Status, [string] $Details)
    $script:Teile.Add([pscustomobject]@{ Teil = $Teil; Status = $Status; Details = $Details })
}

function Invoke-KernTests {
    param($Pfade, [string] $BerichtOrdner, [bool] $Pflicht)
    Write-MBSchritt 'Kern-Tests (CMake, ohne Unreal)'
    $cmake = Find-MBCMake
    if ($null -eq $cmake) {
        $hinweise = @(
            'CMake installieren: Visual Studio Installer -> Einzelne Komponenten -> "C++-CMake-Tools fuer Windows",',
            'oder von cmake.org. Die Kern-Tests laufen zusaetzlich in Unreal (MatchballTennis.Kern.*).')
        if ($Pflicht) { Stop-MBMitFehler 'cmake wurde nicht gefunden - Kern-Tests nicht moeglich.' -Hinweise $hinweise }
        Write-MBWarnung 'cmake wurde nicht gefunden - Kern-Tests per CMake werden uebersprungen.'
        foreach ($h in $hinweise) { Write-MBInfo $h }
        Add-Teilergebnis 'Kern-Tests (CMake)' 'UEBERSPRUNGEN' 'cmake nicht gefunden'
        return
    }
    if ($null -eq $cmake.CTest) {
        if ($Pflicht) { Stop-MBMitFehler ('ctest wurde neben {0} nicht gefunden.' -f $cmake.CMake) }
        Write-MBWarnung 'ctest wurde nicht gefunden - Kern-Tests werden uebersprungen.'
        Add-Teilergebnis 'Kern-Tests (CMake)' 'UEBERSPRUNGEN' 'ctest nicht gefunden'
        return
    }
    Write-MBInfo ('CMake: {0} ({1})' -f $cmake.CMake, $cmake.Quelle)
    $quelle = $Pfade.KernTests
    $bauOrdner = Join-Path $Pfade.KernTests 'build-win'

    $code = Start-MBProzess -Datei $cmake.CMake -Argumente @('-S', $quelle, '-B', $bauOrdner, '-DCMAKE_BUILD_TYPE=Release') `
        -LogDatei (Join-Path $BerichtOrdner 'kern-1-konfigurieren.log') -Beschreibung 'CMake konfigurieren' -KeinAbbruch
    if ($code -ne 0) {
        Write-MBFehler ('CMake-Konfiguration fehlgeschlagen (Exit-Code {0}).' -f $code)
        Write-MBInfo ('Tipp: Ordner {0} loeschen und erneut versuchen (z. B. nach einem Wechsel des Compilers).' -f $bauOrdner)
        Add-Teilergebnis 'Kern-Tests (CMake)' 'FEHLER' ('Konfiguration fehlgeschlagen (Exit-Code {0})' -f $code)
        return
    }
    $code = Start-MBProzess -Datei $cmake.CMake -Argumente @('--build', $bauOrdner, '--config', 'Release', '--parallel') `
        -LogDatei (Join-Path $BerichtOrdner 'kern-2-kompilieren.log') -Beschreibung 'Kern-Tests kompilieren' -KeinAbbruch
    if ($code -ne 0) {
        Write-MBFehler ('Kompilieren der Kern-Tests fehlgeschlagen (Exit-Code {0}).' -f $code)
        Add-Teilergebnis 'Kern-Tests (CMake)' 'FEHLER' ('Kompilieren fehlgeschlagen (Exit-Code {0})' -f $code)
        return
    }
    $ctestLog = Join-Path $BerichtOrdner 'kern-3-ctest.log'
    $code = Start-MBProzess -Datei $cmake.CTest -Argumente @('-C', 'Release', '--output-on-failure') `
        -Arbeitsordner $bauOrdner -LogDatei $ctestLog -Beschreibung 'Kern-Tests ausfuehren (ctest)' -KeinAbbruch
    # Zusammenfassung des Testprogramms ("N Tests, M Pruefungen, K fehlgeschlagen") aus dem Log holen
    $zusammenfassung = ''
    if (Test-Path -LiteralPath $ctestLog -PathType Leaf) {
        $treffer = Select-String -LiteralPath $ctestLog -Pattern '\d+ Tests, \d+ Pruefungen, \d+ fehlgeschlagen' | Select-Object -Last 1
        if ($null -ne $treffer) { $zusammenfassung = $treffer.Matches[0].Value }
    }
    if ($code -eq 0) {
        Write-MBOk ('Kern-Tests gruen. {0}' -f $zusammenfassung)
        Add-Teilergebnis 'Kern-Tests (CMake)' 'OK' $zusammenfassung
    } else {
        Write-MBFehler ('Kern-Tests fehlgeschlagen (Exit-Code {0}). {1}' -f $code, $zusammenfassung)
        Add-Teilergebnis 'Kern-Tests (CMake)' 'FEHLER' (('ctest Exit-Code {0}. {1}' -f $code, $zusammenfassung).Trim())
    }
}

function Read-UnrealBericht {
    # Liest index.json des Automation-Berichts. Ergebnis: Erfolgreich, Fehlgeschlagen, NichtGelaufen, Fehlerliste
    param([string] $Datei)
    $bericht = Read-MBJson $Datei
    $tests = @(Get-MBEigenschaft $bericht 'tests' @())
    $fehlerliste = @()
    $erfolgreich = 0
    $fehlgeschlagen = 0
    $nichtGelaufen = 0
    foreach ($test in $tests) {
        $zustand = [string](Get-MBEigenschaft $test 'state' '')
        $pfad = [string](Get-MBEigenschaft $test 'fullTestPath' (Get-MBEigenschaft $test 'testDisplayName' '?'))
        if ($zustand -match '^(Success|Succeeded)$') { $erfolgreich++; continue }
        if ($zustand -match '^(Skipped)$') { continue }
        if ($zustand -match '^(NotRun|InProcess)$') { $nichtGelaufen++ } else { $fehlgeschlagen++ }
        $meldungen = @()
        foreach ($eintrag in @(Get-MBEigenschaft $test 'entries' @())) {
            $ereignis = Get-MBEigenschaft $eintrag 'event' $null
            $typ = [string](Get-MBEigenschaft $ereignis 'type' '')
            if ($typ -eq 'Error') { $meldungen += [string](Get-MBEigenschaft $ereignis 'message' '') }
        }
        $fehlerliste += [pscustomobject]@{ Test = $pfad; Zustand = $zustand; Meldungen = $meldungen }
    }
    if ($tests.Count -eq 0) {
        # Ohne Testliste die Zaehler des Berichts nehmen
        $erfolgreich = [int](Get-MBEigenschaft $bericht 'succeeded' 0) + [int](Get-MBEigenschaft $bericht 'succeededWithWarnings' 0)
        $fehlgeschlagen = [int](Get-MBEigenschaft $bericht 'failed' 0)
        $nichtGelaufen = [int](Get-MBEigenschaft $bericht 'notRun' 0) + [int](Get-MBEigenschaft $bericht 'inProcess' 0)
    }
    return [pscustomobject]@{ Quelle = $Datei; Erfolgreich = $erfolgreich; Fehlgeschlagen = $fehlgeschlagen; NichtGelaufen = $nichtGelaufen; Fehlerliste = $fehlerliste }
}

function Read-UnrealLog {
    # Ersatz, falls kein Bericht entstanden ist: Zeilen "Test Completed. Result={Success} Name={..} Path={..}"
    param([string] $Datei)
    $erfolgreich = 0
    $fehlgeschlagen = 0
    $fehlerliste = @()
    foreach ($treffer in (Select-String -LiteralPath $Datei -Pattern 'Result=\{(\w+)\}\s+Name=\{([^}]*)\}\s+Path=\{([^}]*)\}')) {
        $zustand = $treffer.Matches[0].Groups[1].Value
        $pfad = $treffer.Matches[0].Groups[3].Value
        if ($zustand -match '^(Success|Succeeded|Passed)$') { $erfolgreich++ }
        elseif ($zustand -eq 'Skipped') { continue }
        else {
            $fehlgeschlagen++
            $fehlerliste += [pscustomobject]@{ Test = $pfad; Zustand = $zustand; Meldungen = @() }
        }
    }
    return [pscustomobject]@{ Quelle = $Datei; Erfolgreich = $erfolgreich; Fehlgeschlagen = $fehlgeschlagen; NichtGelaufen = 0; Fehlerliste = $fehlerliste }
}

function Invoke-UnrealTests {
    param($Pfade, [string] $BerichtOrdner, [string] $Filter, [int] $TimeoutMinuten)
    Write-MBSchritt ('Unreal-Automation-Tests ({0})' -f $Filter)
    if ($Filter -notmatch '^[A-Za-z0-9_.+]+$') {
        Stop-MBMitFehler ('Ungueltiger Filter "{0}" (erlaubt: Buchstaben, Ziffern, Punkt, Unterstrich, Plus).' -f $Filter)
    }
    $ue = Find-MBUnreal -Pflicht
    Assert-MBEditorGebaut
    $berichtUnreal = Join-Path $BerichtOrdner 'Unreal'
    [void](New-MBOrdner $berichtUnreal)
    $unrealLog = Join-Path $BerichtOrdner 'unreal.log'
    $konsolenLog = Join-Path $BerichtOrdner 'unreal-konsole.log'
    $argumente = @(
        $Pfade.UProject,
        ('-ExecCmds="Automation RunTests {0}; Quit"' -f $Filter),
        '-unattended', '-nullrhi', '-nosplash', '-nosound', '-NoPause', '-log',
        '-stdout', '-FullStdOutLogOutput', '-UTF8Output',
        (Format-MBSchalter 'ReportExportPath' $berichtUnreal),
        (Format-MBSchalter 'abslog' $unrealLog))
    $code = Start-MBProzess -Datei $ue.EditorCmd -Argumente $argumente -LogDatei $konsolenLog `
        -Beschreibung 'Unreal-Automation-Tests' -Arbeitsordner $Pfade.Unreal -TimeoutMinuten $TimeoutMinuten -KeinAbbruch

    # Auswertung: Bericht bevorzugt, sonst Log
    $auswertung = $null
    $index = Get-ChildItem -LiteralPath $berichtUnreal -Recurse -Filter 'index.json' -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if ($null -ne $index) {
        try { $auswertung = Read-UnrealBericht $index.FullName } catch { Write-MBWarnung ('Bericht nicht lesbar: {0}' -f $_.Exception.Message) }
    }
    $logDatei = $unrealLog
    if (-not (Test-Path -LiteralPath $logDatei -PathType Leaf)) { $logDatei = $konsolenLog }
    if ($null -eq $auswertung -and (Test-Path -LiteralPath $logDatei -PathType Leaf)) {
        Write-MBWarnung 'Kein Testbericht (index.json) gefunden - werte das Log aus.'
        $auswertung = Read-UnrealLog $logDatei
    }
    $keineTests = $false
    if (Test-Path -LiteralPath $logDatei -PathType Leaf) {
        $keineTests = $null -ne (Select-String -LiteralPath $logDatei -Pattern 'No automation tests matched' -SimpleMatch | Select-Object -First 1)
    }

    Write-MBInfo ('Berichte: {0}' -f $berichtUnreal)
    Write-MBInfo ('Log:      {0}' -f $logDatei)
    if ($null -eq $auswertung) {
        Write-MBFehler ('Kein Ergebnis gefunden (Unreal Exit-Code {0}).' -f $code)
        Add-Teilergebnis 'Unreal-Tests' 'FEHLER' ('kein Bericht und kein Log (Exit-Code {0})' -f $code)
        return
    }
    foreach ($f in $auswertung.Fehlerliste) {
        Write-MBFehler ('{0} ({1})' -f $f.Test, $f.Zustand)
        $anzahl = 0
        foreach ($m in $f.Meldungen) {
            if ($anzahl -ge 10) { Write-MBInfo '  ...'; break }
            Write-Host ('          ' + $m) -ForegroundColor Red
            $anzahl++
        }
    }
    $details = '{0} erfolgreich, {1} fehlgeschlagen, {2} nicht gelaufen' -f $auswertung.Erfolgreich, $auswertung.Fehlgeschlagen, $auswertung.NichtGelaufen
    if ($keineTests -or ($auswertung.Erfolgreich + $auswertung.Fehlgeschlagen + $auswertung.NichtGelaufen) -eq 0) {
        Write-MBFehler ('Keine Tests zum Filter "{0}" gelaufen.' -f $Filter)
        Write-MBInfo 'Ist das Editor-Ziel aktuell (Tools\build.ps1)? Stimmt der Filter (Testnamen beginnen mit MatchballTennis.)?'
        Add-Teilergebnis 'Unreal-Tests' 'FEHLER' ('keine Tests gelaufen (Exit-Code {0})' -f $code)
        return
    }
    if ($auswertung.Fehlgeschlagen -gt 0 -or $auswertung.NichtGelaufen -gt 0) {
        Write-MBFehler ('Unreal-Tests: {0}' -f $details)
        Add-Teilergebnis 'Unreal-Tests' 'FEHLER' $details
        return
    }
    if ($code -ne 0) {
        Write-MBFehler ('Alle Tests gruen, aber Unreal endete mit Exit-Code {0} (Absturz beim Beenden?).' -f $code)
        Add-Teilergebnis 'Unreal-Tests' 'FEHLER' ($details + (', Unreal Exit-Code {0}' -f $code))
        return
    }
    Write-MBOk ('Unreal-Tests gruen: {0}' -f $details)
    Add-Teilergebnis 'Unreal-Tests' 'OK' $details
}

$ergebnis = 0
try {
    $pfade = Get-MBPfade
    Write-MBTitel 'Matchball Tennis - Tests (test.ps1)'
    if ($NurKern -and $NurUnreal) { Stop-MBMitFehler '-NurKern und -NurUnreal schliessen sich aus.' }
    $berichtOrdner = Join-MBPfad $pfade.Saved 'Testberichte' (Get-MBZeitstempel)
    [void](New-MBOrdner $berichtOrdner)

    if (-not $NurUnreal) { Invoke-KernTests -Pfade $pfade -BerichtOrdner $berichtOrdner -Pflicht $NurKern.IsPresent }
    if (-not $NurKern) { Invoke-UnrealTests -Pfade $pfade -BerichtOrdner $berichtOrdner -Filter $Filter -TimeoutMinuten $TimeoutMinuten }

    Write-MBSchritt 'Zusammenfassung'
    $alleGruen = $true
    foreach ($t in $script:Teile) {
        $farbe = 'Green'
        if ($t.Status -eq 'FEHLER') { $farbe = 'Red'; $alleGruen = $false }
        elseif ($t.Status -eq 'UEBERSPRUNGEN') { $farbe = 'Yellow' }
        Write-Host ('  {0,-15} {1,-22} {2}' -f $t.Status, $t.Teil, $t.Details) -ForegroundColor $farbe
    }
    $gelaufen = @($script:Teile | Where-Object { $_.Status -ne 'UEBERSPRUNGEN' })
    Write-MBInfo ('Berichte und Logs: {0}' -f $berichtOrdner)
    if ($gelaufen.Count -eq 0) {
        Stop-MBMitFehler 'Es wurden keine Tests ausgefuehrt.'
    }
    if (-not $alleGruen) {
        Stop-MBMitFehler 'Nicht alle Tests sind gruen.' -Hinweise @(('Details: {0}' -f $berichtOrdner))
    }
    Write-MBOk 'Alle Tests gruen.'
} catch {
    $ergebnis = Write-MBAbbruch $_
}
exit $ergebnis
