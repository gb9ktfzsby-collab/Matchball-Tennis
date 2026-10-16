# Matchball Tennis - gemeinsame Funktionen fuer die Werkzeugskripte (README 0.0a, 2.4, 20)
#
# Einbinden (in einem Skript unter Unreal/Tools):
#   Import-Module (Join-Path $PSScriptRoot 'MBGemeinsam.psm1') -Force -DisableNameChecking
#
# Regeln fuer alle Werkzeugskripte:
#   - laufen unter Windows PowerShell 5.1 UND PowerShell 7
#     (keine Operatoren ?? und ?., kein ternaerer Operator, kein Join-Path mit mehr als zwei Teilen)
#   - Dateiinhalt nur ASCII (PowerShell 5.1 liest Dateien ohne BOM als ANSI) - Umlaute als ae/oe/ue/ss
#   - Zugangsdaten werden nie gespeichert und nie ausgegeben
#
# Muster fuer ein Skript:
#   $ErrorActionPreference = 'Stop'
#   Import-Module (Join-Path $PSScriptRoot 'MBGemeinsam.psm1') -Force -DisableNameChecking
#   $ergebnis = 0
#   try {
#       ...
#       Stop-MBMitFehler 'Klare Meldung' -Hinweise @('Was zu tun ist') -ExitCode 1
#   } catch {
#       $ergebnis = Write-MBAbbruch $_
#   }
#   exit $ergebnis
#
# Suchreihenfolge Unreal Engine 5.8:
#   1. Umgebungsvariable MB_UE_PFAD (Ordner UE_5.8 oder dessen Unterordner Engine)
#   2. Registry HKLM:\SOFTWARE\EpicGames\Unreal Engine\5.8 -> InstalledDirectory (Epic Games Launcher)
#   3. C:\Program Files\Epic Games\UE_5.8
# Suchreihenfolge Blender 5.2:
#   1. Umgebungsvariable MB_BLENDER (Pfad zu blender.exe oder dessen Ordner)
#   2. C:\Program Files\Blender Foundation\Blender 5.2\blender.exe
#   3. blender.exe im PATH
#   4. andere Versionen unter C:\Program Files\Blender Foundation\ (mit Warnung)

$script:MBToolsOrdner = $PSScriptRoot
$script:MBUnrealVersion = '5.8'
$script:MBBlenderVersion = '5.2'

# ---------------------------------------------------------------------------------------------
# Allgemeines
# ---------------------------------------------------------------------------------------------

function Test-MBWindows {
    # $IsWindows gibt es erst ab PowerShell 6
    return ([System.Environment]::OSVersion.Platform -eq [System.PlatformID]::Win32NT)
}

function Join-MBPfad {
    # Join-Path mit beliebig vielen Teilen (Join-Path kann in PowerShell 5.1 nur zwei)
    param([Parameter(Mandatory = $true, ValueFromRemainingArguments = $true)] [string[]] $Teile)
    $ergebnis = $Teile[0]
    for ($i = 1; $i -lt $Teile.Count; $i++) {
        $ergebnis = Join-Path -Path $ergebnis -ChildPath $Teile[$i]
    }
    return $ergebnis
}

function Get-MBPfade {
    # Alle wichtigen Projektpfade, ermittelt aus dem Ort dieses Moduls (Unreal/Tools)
    $tools = [System.IO.Path]::GetFullPath($script:MBToolsOrdner)
    $unreal = Split-Path -Parent $tools
    $repo = Split-Path -Parent $unreal
    $sourceAssets = Join-Path $unreal 'SourceAssets'
    $blender = Join-Path $unreal 'blender'
    return [pscustomobject]@{
        Repo               = $repo
        Unreal             = $unreal
        UProject           = Join-Path $unreal 'MatchballTennis.uproject'
        Config             = Join-Path $unreal 'Config'
        DefaultGameIni     = Join-MBPfad $unreal 'Config' 'DefaultGame.ini'
        Data               = Join-Path $unreal 'Data'
        Content            = Join-Path $unreal 'Content'
        Saved              = Join-Path $unreal 'Saved'
        Binaries           = Join-Path $unreal 'Binaries'
        Intermediate       = Join-Path $unreal 'Intermediate'
        Plugins            = Join-Path $unreal 'Plugins'
        SourceAssets       = $sourceAssets
        Erzeugt            = Join-Path $sourceAssets '_erzeugt'
        Vorschau           = Join-Path $sourceAssets '_vorschau'
        Umpire             = Join-MBPfad $sourceAssets 'Audio' 'Umpire'
        Blender            = $blender
        BlenderSkripte     = Join-Path $blender 'scripts'
        BlenderSkriptListe = Join-Path $blender 'skripte.json'
        Tools              = $tools
        ToolsPython        = Join-Path $tools 'Python'
        KernTests          = Join-Path $tools 'KernTests'
        Updater            = Join-Path $unreal 'Updater'
        Werkzeuglogs       = Join-MBPfad $unreal 'Saved' 'Werkzeuge'
    }
}

function Get-MBZeitstempel {
    return (Get-Date).ToString('yyyyMMdd-HHmmss')
}

function New-MBOrdner {
    param([Parameter(Mandatory = $true)] [string] $Pfad)
    if (-not (Test-Path -LiteralPath $Pfad -PathType Container)) {
        [void](New-Item -ItemType Directory -Path $Pfad -Force)
    }
    return $Pfad
}

function Get-MBLogDatei {
    # Log-Datei unter Unreal/Saved/Werkzeuge/ (Saved ist per .gitignore ausgeschlossen)
    param(
        [Parameter(Mandatory = $true)] [string] $Name,
        [string] $Ordner = ''
    )
    if ($Ordner -eq '') { $Ordner = (Get-MBPfade).Werkzeuglogs }
    [void](New-MBOrdner $Ordner)
    return (Join-Path $Ordner ('{0}-{1}.log' -f $Name, (Get-MBZeitstempel)))
}

function Get-MBEigenschaft {
    # Liest eine Eigenschaft tolerant (fehlt sie, kommt der Standardwert zurueck)
    param($Objekt, [string] $Name, $Standard = $null)
    if ($null -eq $Objekt) { return $Standard }
    if ($Objekt -is [System.Collections.IDictionary]) {
        if ($Objekt.Contains($Name)) { return $Objekt[$Name] }
        return $Standard
    }
    $eigenschaft = $Objekt.PSObject.Properties[$Name]
    if ($null -eq $eigenschaft) { return $Standard }
    return $eigenschaft.Value
}

function Format-MBGroesse {
    param([double] $Bytes)
    if ($Bytes -ge 1GB) { return ('{0:N2} GB' -f ($Bytes / 1GB)) }
    if ($Bytes -ge 1MB) { return ('{0:N1} MB' -f ($Bytes / 1MB)) }
    if ($Bytes -ge 1KB) { return ('{0:N0} KB' -f ($Bytes / 1KB)) }
    return ('{0:N0} Bytes' -f $Bytes)
}

function Format-MBDauer {
    param([TimeSpan] $Dauer)
    if ($Dauer.TotalHours -ge 1) { return ('{0} h {1:D2} min' -f [int][Math]::Floor($Dauer.TotalHours), $Dauer.Minutes) }
    if ($Dauer.TotalMinutes -ge 1) { return ('{0} min {1:D2} s' -f [int][Math]::Floor($Dauer.TotalMinutes), $Dauer.Seconds) }
    return ('{0:N1} s' -f $Dauer.TotalSeconds)
}

# ---------------------------------------------------------------------------------------------
# Meldungen (farbig) und Abbruch
# ---------------------------------------------------------------------------------------------

function Write-MBTitel {
    param([string] $Text)
    $linie = '=' * [Math]::Max(20, $Text.Length + 4)
    Write-Host ''
    Write-Host $linie -ForegroundColor Cyan
    Write-Host ('  ' + $Text) -ForegroundColor Cyan
    Write-Host $linie -ForegroundColor Cyan
}

function Write-MBSchritt {
    param([string] $Text)
    Write-Host ''
    Write-Host ('==> ' + $Text) -ForegroundColor Cyan
}

function Write-MBInfo {
    param([string] $Text)
    Write-Host ('    ' + $Text) -ForegroundColor Gray
}

function Write-MBOk {
    param([string] $Text)
    Write-Host ('[OK] ' + $Text) -ForegroundColor Green
}

function Write-MBWarnung {
    param([string] $Text)
    Write-Host ('[WARNUNG] ' + $Text) -ForegroundColor Yellow
}

function Write-MBFehler {
    param([string] $Text)
    Write-Host ('[FEHLER] ' + $Text) -ForegroundColor Red
}

function Stop-MBMitFehler {
    # Bricht das laufende Skript mit einer klaren deutschen Meldung ab.
    # Die Meldung zeigt Write-MBAbbruch im catch-Block des Skripts an und liefert den Exit-Code.
    param(
        [Parameter(Mandatory = $true)] [string] $Meldung,
        [string[]] $Hinweise = @(),
        [int] $ExitCode = 1
    )
    if ($ExitCode -eq 0) { $ExitCode = 1 }
    $ausnahme = New-Object System.Exception -ArgumentList $Meldung
    $ausnahme.Data['MBAbbruch'] = $true
    $ausnahme.Data['MBExitCode'] = $ExitCode
    $ausnahme.Data['MBHinweise'] = @($Hinweise)
    throw $ausnahme
}

function Write-MBAbbruch {
    # Zeigt einen Abbruch (Stop-MBMitFehler) oder einen unerwarteten Fehler an und gibt den Exit-Code zurueck
    param([Parameter(Mandatory = $true)] $Fehler)
    $ausnahme = $null
    if ($Fehler -is [System.Management.Automation.ErrorRecord]) { $ausnahme = $Fehler.Exception }
    elseif ($Fehler -is [System.Exception]) { $ausnahme = $Fehler }

    # Eigene Abbrueche koennen in andere Ausnahmen eingepackt sein (z. B. MethodInvocationException)
    $eigene = $ausnahme
    while ($null -ne $eigene -and -not $eigene.Data.Contains('MBAbbruch')) { $eigene = $eigene.InnerException }

    Write-Host ''
    if ($null -ne $eigene) {
        Write-MBFehler $eigene.Message
        foreach ($hinweis in @($eigene.Data['MBHinweise'])) {
            if ($hinweis) { Write-Host ('          ' + $hinweis) -ForegroundColor Yellow }
        }
        Write-Host ''
        return [int]$eigene.Data['MBExitCode']
    }

    $text = 'Unbekannter Fehler'
    if ($null -ne $ausnahme) { $text = $ausnahme.Message }
    Write-MBFehler ('Unerwarteter Fehler: ' + $text)
    if ($Fehler -is [System.Management.Automation.ErrorRecord] -and $null -ne $Fehler.InvocationInfo) {
        $stelle = $Fehler.InvocationInfo.PositionMessage
        if ($stelle) { Write-Host $stelle -ForegroundColor DarkGray }
    }
    Write-Host '          Das ist vermutlich ein Fehler im Skript. Bitte die ganze Ausgabe an Claude Code geben.' -ForegroundColor Yellow
    Write-Host ''
    return 1
}

# ---------------------------------------------------------------------------------------------
# Befehlszeilen und Prozesse
# ---------------------------------------------------------------------------------------------

function ConvertTo-MBArgument {
    # Setzt ein Argument nach den Windows-Regeln (CommandLineToArgvW / C-Laufzeit) in Anfuehrungszeichen.
    # Bereits fertig formatierte Unreal-Schalter der Form -Name="Wert" bzw. "Wert" bleiben unveraendert,
    # weil Unreal seine Befehlszeile selbst auswertet und genau diese Form erwartet.
    param([AllowEmptyString()] [string] $Wert, [switch] $Immer)
    if ($Wert -eq '') { return '""' }
    if (-not $Immer) {
        if ($Wert -notmatch '[\s"]') { return $Wert }
        if ($Wert -match '^[^\s"]*="[^"]*"$') { return $Wert }
        if ($Wert -match '^"[^"]*"$') { return $Wert }
    }
    $sb = New-Object System.Text.StringBuilder
    [void]$sb.Append('"')
    $rueckstriche = 0
    foreach ($zeichen in $Wert.ToCharArray()) {
        if ($zeichen -eq [char]'\') { $rueckstriche++; continue }
        if ($zeichen -eq [char]'"') {
            [void]$sb.Append(('\' * ($rueckstriche * 2 + 1)))
            [void]$sb.Append('"')
            $rueckstriche = 0
            continue
        }
        if ($rueckstriche -gt 0) { [void]$sb.Append(('\' * $rueckstriche)); $rueckstriche = 0 }
        [void]$sb.Append($zeichen)
    }
    if ($rueckstriche -gt 0) { [void]$sb.Append(('\' * ($rueckstriche * 2))) }
    [void]$sb.Append('"')
    return $sb.ToString()
}

function Format-MBSchalter {
    # Unreal-/UAT-Schalter mit Wert: -Name="Wert" (abschliessende Backslashes entfernt, sonst wuerde \" als
    # maskiertes Anfuehrungszeichen gelesen)
    param([Parameter(Mandatory = $true)] [string] $Name, [Parameter(Mandatory = $true)] [string] $Wert)
    $wert = $Wert.TrimEnd('\')
    if ($wert.Contains('"')) { Stop-MBMitFehler ('Ungueltiges Anfuehrungszeichen im Wert fuer -{0}: {1}' -f $Name, $Wert) }
    return ('-{0}="{1}"' -f $Name, $wert)
}

function ConvertTo-MBBefehlszeile {
    param([string[]] $Argumente = @())
    $teile = @()
    foreach ($argument in $Argumente) {
        if ($null -eq $argument) { continue }
        $teile += (ConvertTo-MBArgument $argument)
    }
    return ($teile -join ' ')
}

function Resolve-MBProgramm {
    # Vollstaendiger Pfad einer Programmdatei (Pfad oder Name im PATH), sonst $null
    param([Parameter(Mandatory = $true)] [string] $Datei)
    if (Test-Path -LiteralPath $Datei -PathType Leaf) {
        return [System.IO.Path]::GetFullPath((Resolve-Path -LiteralPath $Datei).ProviderPath)
    }
    if ([System.IO.Path]::IsPathRooted($Datei)) { return $null }
    $befehl = Get-Command -Name $Datei -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -ne $befehl) { return $befehl.Path }
    return $null
}

function New-MBStartInfo {
    param(
        [Parameter(Mandatory = $true)] [string] $Programm,
        [string[]] $Argumente = @(),
        [string] $Arbeitsordner = ''
    )
    $info = New-Object System.Diagnostics.ProcessStartInfo
    $argumentText = ConvertTo-MBBefehlszeile $Argumente
    $endung = [System.IO.Path]::GetExtension($Programm).ToLowerInvariant()
    if ($endung -eq '.bat' -or $endung -eq '.cmd') {
        if (-not (Test-MBWindows)) { Stop-MBMitFehler ('Batch-Dateien laufen nur unter Windows: {0}' -f $Programm) }
        # cmd.exe /s /c ""C:\Pfad mit Leerzeichen\x.bat" argumente" - /s entfernt nur das aeussere Anfuehrungszeichenpaar
        $cmd = $env:ComSpec
        if (-not $cmd) { $cmd = 'cmd.exe' }
        $info.FileName = $cmd
        $info.Arguments = '/d /s /c "' + (ConvertTo-MBArgument $Programm -Immer) + ' ' + $argumentText + '"'
    } else {
        $info.FileName = $Programm
        $info.Arguments = $argumentText
    }
    $info.UseShellExecute = $false
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $info.RedirectStandardInput = $false
    $info.CreateNoWindow = $false
    $utf8 = New-Object System.Text.UTF8Encoding -ArgumentList $false
    $info.StandardOutputEncoding = $utf8
    $info.StandardErrorEncoding = $utf8
    if ($Arbeitsordner -ne '') { $info.WorkingDirectory = $Arbeitsordner }
    return $info
}

function Stop-MBProzessBaum {
    param([Parameter(Mandatory = $true)] [System.Diagnostics.Process] $Prozess)
    try {
        if ($Prozess.HasExited) { return }
        if (Test-MBWindows) {
            $taskkill = Join-MBPfad $env:SystemRoot 'System32' 'taskkill.exe'
            & $taskkill /PID $Prozess.Id /T /F 2>&1 | Out-Null
        } else {
            $Prozess.Kill()
        }
    } catch {
        Write-MBWarnung ('Prozess {0} konnte nicht beendet werden: {1}' -f $Prozess.Id, $_.Exception.Message)
    }
}

function Write-MBProzessZeile {
    param([string] $Zeile, $Schreiber, [bool] $Still, [string] $Kanal)
    if ($null -ne $Schreiber) {
        if ($Kanal -eq 'err') { $Schreiber.WriteLine('[stderr] ' + $Zeile) } else { $Schreiber.WriteLine($Zeile) }
    }
    if ($Still) { return }
    $farbe = 'Gray'
    if ($Zeile -match '(?i)(: error |: fatal error |\berror:|LogPython: Error|Result=\{Fail|Traceback \(most recent call last\)|\bFEHLER\b)') { $farbe = 'Red' }
    elseif ($Zeile -match '(?i)(: warning |\bwarning:|\bWARNUNG\b)') { $farbe = 'Yellow' }
    Write-Host $Zeile -ForegroundColor $farbe
}

function Start-MBProzess {
    # Startet ein Programm, zeigt seine Ausgabe laufend an, schreibt sie in eine Log-Datei
    # und prueft den Exit-Code. Gibt den Exit-Code zurueck.
    #   -KeinAbbruch   bei Exit-Code ungleich 0 nicht abbrechen, nur den Code zurueckgeben
    #   -Still         Ausgabe nur ins Log schreiben
    #   -TimeoutMinuten  Prozess(baum) nach dieser Zeit beenden (0 = kein Zeitlimit)
    # .bat/.cmd werden ueber cmd.exe gestartet.
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)] [string] $Datei,
        [string[]] $Argumente = @(),
        [string] $LogDatei = '',
        [string] $Arbeitsordner = '',
        [string] $Beschreibung = '',
        [int] $TimeoutMinuten = 0,
        [switch] $KeinAbbruch,
        [switch] $Still
    )
    if ($Beschreibung -eq '') { $Beschreibung = [System.IO.Path]::GetFileName($Datei) }
    $programm = Resolve-MBProgramm $Datei
    if ($null -eq $programm) {
        Stop-MBMitFehler ('Programm nicht gefunden: {0}' -f $Datei) -Hinweise @(('Benoetigt fuer: {0}' -f $Beschreibung))
    }
    $info = New-MBStartInfo -Programm $programm -Argumente $Argumente -Arbeitsordner $Arbeitsordner

    $schreiber = $null
    if ($LogDatei -ne '') {
        [void](New-MBOrdner (Split-Path -Parent $LogDatei))
        $schreiber = New-Object System.IO.StreamWriter -ArgumentList $LogDatei, $true, (New-Object System.Text.UTF8Encoding -ArgumentList $false)
        $schreiber.AutoFlush = $true
        $schreiber.WriteLine(('# {0}' -f $Beschreibung))
        $schreiber.WriteLine(('# Befehl: {0} {1}' -f $info.FileName, $info.Arguments))
        if ($Arbeitsordner -ne '') { $schreiber.WriteLine(('# Arbeitsordner: {0}' -f $Arbeitsordner)) }
        $schreiber.WriteLine(('# Start: {0}' -f (Get-Date).ToString('yyyy-MM-dd HH:mm:ss')))
    }
    Write-MBInfo ('Befehl: {0} {1}' -f $programm, (ConvertTo-MBBefehlszeile $Argumente))
    if ($LogDatei -ne '') { Write-MBInfo ('Log:    {0}' -f $LogDatei) }

    $prozess = New-Object System.Diagnostics.Process
    $prozess.StartInfo = $info
    $start = Get-Date
    try {
        [void]$prozess.Start()
    } catch {
        if ($null -ne $schreiber) { $schreiber.WriteLine('# Start fehlgeschlagen: ' + $_.Exception.Message); $schreiber.Dispose() }
        Stop-MBMitFehler ('{0} konnte nicht gestartet werden: {1}' -f $Beschreibung, $_.Exception.Message)
    }

    $zeitlimitErreicht = $false
    $abgekoppelt = $false
    try {
        $ausgabe = $prozess.StandardOutput
        $fehlerAusgabe = $prozess.StandardError
        $aufgabeAus = $ausgabe.ReadLineAsync()
        $aufgabeErr = $fehlerAusgabe.ReadLineAsync()
        $ausOffen = $true
        $errOffen = $true
        $letzteAktivitaet = Get-Date
        while ($ausOffen -or $errOffen) {
            $offen = @()
            if ($ausOffen) { $offen += $aufgabeAus }
            if ($errOffen) { $offen += $aufgabeErr }
            [void][System.Threading.Tasks.Task]::WaitAny([System.Threading.Tasks.Task[]]$offen, 250)

            if ($ausOffen -and $aufgabeAus.IsCompleted) {
                $zeile = $null
                try { $zeile = $aufgabeAus.Result } catch { $zeile = $null }
                if ($null -eq $zeile) { $ausOffen = $false }
                else {
                    Write-MBProzessZeile -Zeile $zeile -Schreiber $schreiber -Still $Still.IsPresent -Kanal 'out'
                    $aufgabeAus = $ausgabe.ReadLineAsync()
                    $letzteAktivitaet = Get-Date
                }
            }
            if ($errOffen -and $aufgabeErr.IsCompleted) {
                $zeile = $null
                try { $zeile = $aufgabeErr.Result } catch { $zeile = $null }
                if ($null -eq $zeile) { $errOffen = $false }
                else {
                    Write-MBProzessZeile -Zeile $zeile -Schreiber $schreiber -Still $Still.IsPresent -Kanal 'err'
                    $aufgabeErr = $fehlerAusgabe.ReadLineAsync()
                    $letzteAktivitaet = Get-Date
                }
            }

            if ($TimeoutMinuten -gt 0 -and -not $zeitlimitErreicht -and -not $prozess.HasExited) {
                if (((Get-Date) - $start).TotalMinutes -ge $TimeoutMinuten) {
                    Write-MBWarnung ('{0} dauert laenger als {1} Minuten und wird beendet.' -f $Beschreibung, $TimeoutMinuten)
                    Stop-MBProzessBaum $prozess
                    $zeitlimitErreicht = $true
                }
            }
            # Ist der Prozess beendet, haelt aber ein Unterprozess die Ausgabe offen (z. B. ein
            # weiterlaufender Hilfsprozess), nicht ewig warten.
            if ($prozess.HasExited -and ($ausOffen -or $errOffen)) {
                if (((Get-Date) - $letzteAktivitaet).TotalSeconds -ge 15) {
                    $abgekoppelt = $true
                    break
                }
            }
        }
        $prozess.WaitForExit()
        $exitCode = $prozess.ExitCode
    } finally {
        $dauer = (Get-Date) - $start
        if ($null -ne $schreiber) {
            if ($abgekoppelt) { $schreiber.WriteLine('# Hinweis: Ausgabe eines weiterlaufenden Unterprozesses wurde nicht mehr abgewartet.') }
            if ($prozess.HasExited) { $schreiber.WriteLine(('# Ende: {0}, Exit-Code {1}, Dauer {2}' -f (Get-Date).ToString('yyyy-MM-dd HH:mm:ss'), $prozess.ExitCode, (Format-MBDauer $dauer))) }
            $schreiber.Dispose()
        }
    }
    if ($abgekoppelt) { Write-MBInfo 'Hinweis: Ausgabe eines weiterlaufenden Unterprozesses wird nicht mehr abgewartet.' }
    if ($zeitlimitErreicht) {
        if ($exitCode -eq 0) { $exitCode = 1 }
        if (-not $KeinAbbruch) {
            Stop-MBMitFehler ('{0} wurde nach {1} Minuten abgebrochen (Zeitlimit).' -f $Beschreibung, $TimeoutMinuten) -ExitCode $exitCode -Hinweise @(('Log: {0}' -f $LogDatei))
        }
        return $exitCode
    }
    if ($exitCode -ne 0 -and -not $KeinAbbruch) {
        $hinweise = @()
        if ($LogDatei -ne '') { $hinweise += ('Vollstaendige Ausgabe: {0}' -f $LogDatei) }
        Stop-MBMitFehler ('{0} ist fehlgeschlagen (Exit-Code {1}).' -f $Beschreibung, $exitCode) -ExitCode $exitCode -Hinweise $hinweise
    }
    Write-MBInfo ('Fertig nach {0} (Exit-Code {1}).' -f (Format-MBDauer $dauer), $exitCode)
    return $exitCode
}

function Invoke-MBBefehlAusgabe {
    # Fuehrt ein Programm kurz aus und gibt Ausgabe und Exit-Code zurueck (ohne Anzeige, ohne Abbruch).
    # Ergebnis: Gefunden, ExitCode, Ausgabe (stdout + stderr), Standard, Fehler, Zeitlimit
    param(
        [Parameter(Mandatory = $true)] [string] $Datei,
        [string[]] $Argumente = @(),
        [int] $TimeoutSekunden = 60,
        [string] $Arbeitsordner = ''
    )
    $ergebnis = [pscustomobject]@{ Gefunden = $false; ExitCode = -1; Ausgabe = ''; Standard = ''; Fehler = ''; Zeitlimit = $false; Programm = '' }
    $programm = Resolve-MBProgramm $Datei
    if ($null -eq $programm) { return $ergebnis }
    $ergebnis.Gefunden = $true
    $ergebnis.Programm = $programm
    try {
        $info = New-MBStartInfo -Programm $programm -Argumente $Argumente -Arbeitsordner $Arbeitsordner
        $info.CreateNoWindow = $true
        $prozess = New-Object System.Diagnostics.Process
        $prozess.StartInfo = $info
        [void]$prozess.Start()
    } catch {
        $ergebnis.Fehler = $_.Exception.Message
        $ergebnis.Ausgabe = $ergebnis.Fehler
        return $ergebnis
    }
    $aufgabeAus = $prozess.StandardOutput.ReadToEndAsync()
    $aufgabeErr = $prozess.StandardError.ReadToEndAsync()
    if (-not $prozess.WaitForExit($TimeoutSekunden * 1000)) {
        Stop-MBProzessBaum $prozess
        $ergebnis.Zeitlimit = $true
    }
    [void][System.Threading.Tasks.Task]::WaitAll([System.Threading.Tasks.Task[]]@($aufgabeAus, $aufgabeErr), 5000)
    if ($aufgabeAus.IsCompleted -and -not $aufgabeAus.IsFaulted) { $ergebnis.Standard = $aufgabeAus.Result }
    if ($aufgabeErr.IsCompleted -and -not $aufgabeErr.IsFaulted) { $ergebnis.Fehler = $aufgabeErr.Result }
    $ergebnis.Ausgabe = ($ergebnis.Standard + "`n" + $ergebnis.Fehler).Trim()
    if ($prozess.HasExited) { $ergebnis.ExitCode = $prozess.ExitCode }
    return $ergebnis
}

# ---------------------------------------------------------------------------------------------
# Unreal Engine finden
# ---------------------------------------------------------------------------------------------

function Read-MBJson {
    param([Parameter(Mandatory = $true)] [string] $Pfad)
    $text = [System.IO.File]::ReadAllText($Pfad)
    if ($text.Length -gt 0 -and $text[0] -eq [char]0xFEFF) { $text = $text.Substring(1) }
    return ($text | ConvertFrom-Json)
}

function New-MBUnrealKandidat {
    param([string] $Pfad, [string] $Quelle)
    $wurzel = $Pfad.Trim().Trim('"').TrimEnd('\', '/')
    if ((Split-Path -Leaf $wurzel) -eq 'Engine') { $wurzel = Split-Path -Parent $wurzel }
    $engine = Join-Path $wurzel 'Engine'
    $kandidat = [pscustomobject]@{
        Wurzel      = $wurzel
        Quelle      = $Quelle
        Gueltig     = $false
        Version     = ''
        BuildBat    = Join-MBPfad $engine 'Build' 'BatchFiles' 'Build.bat'
        RunUAT      = Join-MBPfad $engine 'Build' 'BatchFiles' 'RunUAT.bat'
        EditorCmd   = Join-MBPfad $engine 'Binaries' 'Win64' 'UnrealEditor-Cmd.exe'
        Editor      = Join-MBPfad $engine 'Binaries' 'Win64' 'UnrealEditor.exe'
        Installiert = $false
        WindowsSdkJson = Join-MBPfad $engine 'Config' 'Windows' 'Windows_SDK.json'
    }
    if ($wurzel -eq '') { return $kandidat }
    $kandidat.Gueltig = (Test-Path -LiteralPath $kandidat.EditorCmd -PathType Leaf) -and (Test-Path -LiteralPath $kandidat.BuildBat -PathType Leaf)
    $kandidat.Installiert = Test-Path -LiteralPath (Join-MBPfad $engine 'Build' 'InstalledBuild.txt') -PathType Leaf
    $versionsDatei = Join-MBPfad $engine 'Build' 'Build.version'
    if (Test-Path -LiteralPath $versionsDatei -PathType Leaf) {
        try {
            $v = Read-MBJson $versionsDatei
            $kandidat.Version = '{0}.{1}.{2}' -f (Get-MBEigenschaft $v 'MajorVersion' '?'), (Get-MBEigenschaft $v 'MinorVersion' '?'), (Get-MBEigenschaft $v 'PatchVersion' '?')
        } catch {
            $kandidat.Version = ''
        }
    }
    return $kandidat
}

function Get-MBUnrealKandidaten {
    # Alle Orte, an denen Unreal Engine 5.8 gesucht wird, in Suchreihenfolge (mit Gueltig = $true/$false)
    $liste = @()
    $gesehen = @{}
    $orte = @()
    if ($env:MB_UE_PFAD) { $orte += , @($env:MB_UE_PFAD, 'Umgebungsvariable MB_UE_PFAD') }
    if (Test-MBWindows) {
        try {
            $schluessel = 'HKLM:\SOFTWARE\EpicGames\Unreal Engine\' + $script:MBUnrealVersion
            if (Test-Path -LiteralPath $schluessel) {
                $eintrag = Get-ItemProperty -LiteralPath $schluessel -ErrorAction Stop
                $wert = Get-MBEigenschaft $eintrag 'InstalledDirectory' ''
                if ($wert) { $orte += , @($wert, 'Registry (Epic Games Launcher)') }
            }
        } catch {
            Write-Verbose ('Registry nicht lesbar: ' + $_.Exception.Message)
        }
        $orte += , @(('C:\Program Files\Epic Games\UE_' + $script:MBUnrealVersion), 'Standardpfad')
        if ($env:ProgramFiles) { $orte += , @((Join-MBPfad $env:ProgramFiles 'Epic Games' ('UE_' + $script:MBUnrealVersion)), 'Standardpfad') }
    }
    foreach ($ort in $orte) {
        $kandidat = New-MBUnrealKandidat -Pfad $ort[0] -Quelle $ort[1]
        $schluesselName = $kandidat.Wurzel.ToLowerInvariant()
        if ($gesehen.ContainsKey($schluesselName)) { continue }
        $gesehen[$schluesselName] = $true
        $liste += $kandidat
    }
    return $liste
}

function Find-MBUnreal {
    # Erste gueltige Unreal-Installation. -Pflicht: Abbruch mit Erklaerung, wenn keine gefunden wird.
    param([switch] $Pflicht, [switch] $Leise)
    $kandidaten = @(Get-MBUnrealKandidaten)
    foreach ($kandidat in $kandidaten) {
        if (-not $kandidat.Gueltig) { continue }
        if (-not $Leise -and $kandidat.Version -ne '' -and -not $kandidat.Version.StartsWith($script:MBUnrealVersion + '.')) {
            Write-MBWarnung ('Gefundene Unreal Engine hat Version {0}, das Projekt erwartet {1} ({2}).' -f $kandidat.Version, $script:MBUnrealVersion, $kandidat.Wurzel)
        }
        return $kandidat
    }
    if ($Pflicht) {
        $hinweise = @()
        if (-not (Test-MBWindows)) { $hinweise += 'Unreal Engine laeuft nur unter Windows (lokaler PC, nicht in der Cloud).' }
        foreach ($kandidat in $kandidaten) { $hinweise += ('Geprueft: {0} ({1}) - keine UnrealEditor-Cmd.exe/Build.bat' -f $kandidat.Wurzel, $kandidat.Quelle) }
        if ($env:MB_UE_PFAD) { $hinweise += 'Die Umgebungsvariable MB_UE_PFAD zeigt auf keinen gueltigen Unreal-Ordner.' }
        $hinweise += 'Unreal Engine 5.8 im Epic Games Launcher installieren, oder den Ordner angeben:'
        $hinweise += '  setx MB_UE_PFAD "D:\Epic Games\UE_5.8"   (danach neues Fenster oeffnen)'
        Stop-MBMitFehler ('Unreal Engine {0} wurde nicht gefunden.' -f $script:MBUnrealVersion) -Hinweise $hinweise
    }
    return $null
}

function Test-MBEditorGebaut {
    # $true, wenn das Editor-Ziel des Projekts kompiliert ist (sonst fragt Unreal beim Start nach)
    $pfade = Get-MBPfade
    $win64 = Join-Path $pfade.Binaries 'Win64'
    if (Test-Path -LiteralPath (Join-Path $win64 'UnrealEditor.modules') -PathType Leaf) { return $true }
    if (Test-Path -LiteralPath (Join-Path $win64 'UnrealEditor-MatchballTennis.dll') -PathType Leaf) { return $true }
    return $false
}

function Assert-MBEditorGebaut {
    if (-not (Test-MBEditorGebaut)) {
        Stop-MBMitFehler 'Das Editor-Ziel MatchballTennisEditor ist noch nicht kompiliert.' -Hinweise @(
            'Zuerst ausfuehren:  Tools\build.ps1',
            '(ohne kompilierte Module fragt Unreal nach einem Neubau und bricht im unbeaufsichtigten Modus ab)')
    }
}

# ---------------------------------------------------------------------------------------------
# Blender, Visual Studio, CMake finden
# ---------------------------------------------------------------------------------------------

function Get-MBBlenderKandidaten {
    $liste = @()
    $exe = 'blender'
    if (Test-MBWindows) { $exe = 'blender.exe' }
    if ($env:MB_BLENDER) {
        $pfad = $env:MB_BLENDER.Trim().Trim('"')
        if (Test-Path -LiteralPath $pfad -PathType Container) { $pfad = Join-Path $pfad $exe }
        $liste += [pscustomobject]@{ Pfad = $pfad; Quelle = 'Umgebungsvariable MB_BLENDER'; Gueltig = (Test-Path -LiteralPath $pfad -PathType Leaf); AndereVersion = $false }
    }
    if (Test-MBWindows) {
        $standard = 'C:\Program Files\Blender Foundation\Blender ' + $script:MBBlenderVersion + '\blender.exe'
        $liste += [pscustomobject]@{ Pfad = $standard; Quelle = 'Standardpfad'; Gueltig = (Test-Path -LiteralPath $standard -PathType Leaf); AndereVersion = $false }
    }
    $befehl = Get-Command -Name $exe -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -ne $befehl) {
        $liste += [pscustomobject]@{ Pfad = $befehl.Path; Quelle = 'PATH'; Gueltig = $true; AndereVersion = $false }
    }
    if (Test-MBWindows) {
        $basis = 'C:\Program Files\Blender Foundation'
        if (Test-Path -LiteralPath $basis -PathType Container) {
            $ordner = Get-ChildItem -LiteralPath $basis -Directory -ErrorAction SilentlyContinue | Where-Object { $_.Name -like 'Blender *' }
            $sortiert = $ordner | Sort-Object -Descending -Property @{ Expression = {
                    $v = $null
                    if ([System.Version]::TryParse(($_.Name -replace '^Blender\s+', ''), [ref]$v)) { $v } else { New-Object System.Version -ArgumentList 0, 0 }
                } }
            foreach ($o in $sortiert) {
                $pfad = Join-Path $o.FullName 'blender.exe'
                if (Test-Path -LiteralPath $pfad -PathType Leaf) {
                    $liste += [pscustomobject]@{ Pfad = $pfad; Quelle = ('andere Version ({0})' -f $o.Name); Gueltig = $true; AndereVersion = $true }
                }
            }
        }
    }
    return $liste
}

function Find-MBBlender {
    param([switch] $Pflicht)
    $kandidaten = @(Get-MBBlenderKandidaten)
    if ($env:MB_BLENDER -and $kandidaten.Count -gt 0 -and -not $kandidaten[0].Gueltig) {
        if ($Pflicht) {
            Stop-MBMitFehler ('MB_BLENDER zeigt auf keine Blender-Datei: {0}' -f $env:MB_BLENDER) -Hinweise @('MB_BLENDER auf den Pfad zu blender.exe setzen, z. B.  setx MB_BLENDER "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe"')
        }
        return $null
    }
    foreach ($kandidat in $kandidaten) {
        if ($kandidat.Gueltig) {
            if ($kandidat.AndereVersion) {
                Write-MBWarnung ('Blender {0} nicht gefunden, nehme {1} ({2}).' -f $script:MBBlenderVersion, $kandidat.Pfad, $kandidat.Quelle)
            }
            return $kandidat
        }
    }
    if ($Pflicht) {
        $hinweise = @()
        foreach ($kandidat in $kandidaten) { $hinweise += ('Geprueft: {0} ({1})' -f $kandidat.Pfad, $kandidat.Quelle) }
        $hinweise += ('Blender {0} LTS installieren (blender.org) oder den Pfad angeben:' -f $script:MBBlenderVersion)
        $hinweise += '  setx MB_BLENDER "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe"   (danach neues Fenster oeffnen)'
        Stop-MBMitFehler ('Blender {0} wurde nicht gefunden.' -f $script:MBBlenderVersion) -Hinweise $hinweise
    }
    return $null
}

function Get-MBBlenderVersion {
    # Liefert z. B. "Blender 5.2.0 LTS" (erste Zeile von blender --version) oder '' bei Fehler
    param([Parameter(Mandatory = $true)] [string] $Pfad)
    $ergebnis = Invoke-MBBefehlAusgabe -Datei $Pfad -Argumente @('--factory-startup', '--version') -TimeoutSekunden 90
    if (-not $ergebnis.Gefunden -or $ergebnis.ExitCode -ne 0) { return '' }
    foreach ($zeile in ($ergebnis.Standard -split "`r?`n")) {
        if ($zeile -match '^\s*Blender\s+\d') { return $zeile.Trim() }
    }
    return ''
}

function Find-MBVsWhere {
    $kandidaten = @()
    if (${env:ProgramFiles(x86)}) { $kandidaten += Join-MBPfad ${env:ProgramFiles(x86)} 'Microsoft Visual Studio' 'Installer' 'vswhere.exe' }
    $kandidaten += 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
    foreach ($pfad in $kandidaten) {
        if (Test-Path -LiteralPath $pfad -PathType Leaf) { return $pfad }
    }
    $befehl = Get-Command -Name 'vswhere.exe' -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -ne $befehl) { return $befehl.Path }
    return $null
}

function Find-MBCMake {
    # cmake aus dem PATH, sonst das von Visual Studio mitgebrachte, sonst C:\Program Files\CMake
    $exeName = 'cmake'
    $ctestName = 'ctest'
    if (Test-MBWindows) { $exeName = 'cmake.exe'; $ctestName = 'ctest.exe' }
    $treffer = @()
    $befehl = Get-Command -Name $exeName -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -ne $befehl) { $treffer += , @($befehl.Path, 'PATH') }
    if (Test-MBWindows) {
        $vswhere = Find-MBVsWhere
        if ($null -ne $vswhere) {
            $vs = Invoke-MBBefehlAusgabe -Datei $vswhere -Argumente @('-latest', '-products', '*', '-utf8', '-find', 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe')
            if ($vs.ExitCode -eq 0) {
                foreach ($zeile in ($vs.Standard -split "`r?`n")) {
                    if ($zeile.Trim() -ne '') { $treffer += , @($zeile.Trim(), 'Visual Studio'); break }
                }
            }
        }
        if ($env:ProgramFiles) { $treffer += , @((Join-MBPfad $env:ProgramFiles 'CMake' 'bin' 'cmake.exe'), 'C:\Program Files\CMake') }
    }
    foreach ($t in $treffer) {
        if (Test-Path -LiteralPath $t[0] -PathType Leaf) {
            $ctest = Join-Path (Split-Path -Parent $t[0]) $ctestName
            if (-not (Test-Path -LiteralPath $ctest -PathType Leaf)) {
                $c = Get-Command -Name $ctestName -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
                if ($null -ne $c) { $ctest = $c.Path } else { $ctest = $null }
            }
            return [pscustomobject]@{ CMake = $t[0]; CTest = $ctest; Quelle = $t[1] }
        }
    }
    return $null
}

# ---------------------------------------------------------------------------------------------
# Projektdaten, Dateien, Pruefsummen
# ---------------------------------------------------------------------------------------------

function Get-MBIniWert {
    # Liest einen Wert aus einer Unreal-.ini (Abschnitt [..], Schluessel=Wert). Letzter Treffer gewinnt.
    param(
        [Parameter(Mandatory = $true)] [string] $Datei,
        [Parameter(Mandatory = $true)] [string] $Abschnitt,
        [Parameter(Mandatory = $true)] [string] $Schluessel
    )
    if (-not (Test-Path -LiteralPath $Datei -PathType Leaf)) { return $null }
    $aktuell = ''
    $wert = $null
    foreach ($zeile in [System.IO.File]::ReadAllLines($Datei)) {
        $z = $zeile.Trim()
        if ($z -eq '' -or $z.StartsWith(';') -or $z.StartsWith('#')) { continue }
        if ($z.StartsWith('[') -and $z.EndsWith(']')) { $aktuell = $z.Substring(1, $z.Length - 2); continue }
        if ($aktuell -ne $Abschnitt) { continue }
        $gleich = $z.IndexOf('=')
        if ($gleich -le 0) { continue }
        if ($z.Substring(0, $gleich).Trim() -eq $Schluessel) { $wert = $z.Substring($gleich + 1).Trim() }
    }
    return $wert
}

function Get-MBProjektVersion {
    # ProjectVersion aus Config/DefaultGame.ini, z. B. "1.0.0"
    $pfade = Get-MBPfade
    $version = Get-MBIniWert -Datei $pfade.DefaultGameIni -Abschnitt '/Script/EngineSettings.GeneralProjectSettings' -Schluessel 'ProjectVersion'
    if (-not $version) {
        Stop-MBMitFehler ('ProjectVersion fehlt in {0}' -f $pfade.DefaultGameIni) -Hinweise @('Abschnitt [/Script/EngineSettings.GeneralProjectSettings], Zeile ProjectVersion=1.0.0')
    }
    if ($version -notmatch '^[0-9A-Za-z][0-9A-Za-z.\-]*$') {
        Stop-MBMitFehler ('ProjectVersion "{0}" enthaelt ungueltige Zeichen (erlaubt: Ziffern, Buchstaben, Punkt, Minus).' -f $version)
    }
    return $version
}

function Write-MBTextDatei {
    # Schreibt Text als UTF-8 ohne BOM (Set-Content -Encoding UTF8 schreibt in PowerShell 5.1 ein BOM)
    param([Parameter(Mandatory = $true)] [string] $Pfad, [Parameter(Mandatory = $true)] [AllowEmptyString()] [string] $Text)
    $ordner = Split-Path -Parent $Pfad
    if ($ordner) { [void](New-MBOrdner $ordner) }
    [System.IO.File]::WriteAllText($Pfad, $Text, (New-Object System.Text.UTF8Encoding -ArgumentList $false))
}

function ConvertTo-MBJson {
    param([Parameter(Mandatory = $true)] $Objekt)
    return (ConvertTo-Json -InputObject $Objekt -Depth 10)
}

function Get-MBDateiHash {
    # SHA-256 einer Datei, Kleinbuchstaben
    param([Parameter(Mandatory = $true)] [string] $Pfad)
    return (Get-FileHash -LiteralPath $Pfad -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Get-MBTextHash {
    # SHA-256 eines Textes (UTF-8 ohne BOM), Kleinbuchstaben
    param([Parameter(Mandatory = $true)] [AllowEmptyString()] [string] $Text)
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        $bytes = (New-Object System.Text.UTF8Encoding -ArgumentList $false).GetBytes($Text)
        $hash = $sha.ComputeHash($bytes)
    } finally {
        $sha.Dispose()
    }
    return (($hash | ForEach-Object { $_.ToString('x2') }) -join '')
}

function ConvertTo-MBRelativerPfad {
    # Pfad relativ zu Basis, mit "/" getrennt ([System.IO.Path]::GetRelativePath gibt es in .NET Framework nicht)
    param([Parameter(Mandatory = $true)] [string] $Basis, [Parameter(Mandatory = $true)] [string] $Pfad)
    $b = [System.IO.Path]::GetFullPath($Basis).TrimEnd('\', '/')
    $p = [System.IO.Path]::GetFullPath($Pfad)
    $vergleich = [System.StringComparison]::Ordinal
    if (Test-MBWindows) { $vergleich = [System.StringComparison]::OrdinalIgnoreCase }
    if (-not $p.StartsWith($b + [System.IO.Path]::DirectorySeparatorChar, $vergleich)) {
        Stop-MBMitFehler ('Pfad liegt nicht unter {0}: {1}' -f $b, $p)
    }
    return $p.Substring($b.Length + 1).Replace('\', '/')
}

function Test-MBPfadUnter {
    # $true, wenn Pfad gleich Basis ist oder darunter liegt
    param([Parameter(Mandatory = $true)] [string] $Basis, [Parameter(Mandatory = $true)] [string] $Pfad)
    $b = [System.IO.Path]::GetFullPath($Basis).TrimEnd('\', '/')
    $p = [System.IO.Path]::GetFullPath($Pfad).TrimEnd('\', '/')
    $vergleich = [System.StringComparison]::Ordinal
    if (Test-MBWindows) { $vergleich = [System.StringComparison]::OrdinalIgnoreCase }
    if ($p.Equals($b, $vergleich)) { return $true }
    return $p.StartsWith($b + [System.IO.Path]::DirectorySeparatorChar, $vergleich)
}

Export-ModuleMember -Function @(
    'Test-MBWindows', 'Join-MBPfad', 'Get-MBPfade', 'Get-MBZeitstempel', 'New-MBOrdner', 'Get-MBLogDatei',
    'Get-MBEigenschaft', 'Format-MBGroesse', 'Format-MBDauer',
    'Write-MBTitel', 'Write-MBSchritt', 'Write-MBInfo', 'Write-MBOk', 'Write-MBWarnung', 'Write-MBFehler',
    'Stop-MBMitFehler', 'Write-MBAbbruch',
    'ConvertTo-MBArgument', 'Format-MBSchalter', 'ConvertTo-MBBefehlszeile', 'Resolve-MBProgramm',
    'Start-MBProzess', 'Invoke-MBBefehlAusgabe', 'Stop-MBProzessBaum',
    'Read-MBJson', 'Get-MBUnrealKandidaten', 'Find-MBUnreal', 'Test-MBEditorGebaut', 'Assert-MBEditorGebaut',
    'Get-MBBlenderKandidaten', 'Find-MBBlender', 'Get-MBBlenderVersion', 'Find-MBVsWhere', 'Find-MBCMake',
    'Get-MBIniWert', 'Get-MBProjektVersion', 'Write-MBTextDatei', 'ConvertTo-MBJson',
    'Get-MBDateiHash', 'Get-MBTextHash', 'ConvertTo-MBRelativerPfad', 'Test-MBPfadUnter'
)
