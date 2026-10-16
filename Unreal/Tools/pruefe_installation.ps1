#Requires -Version 5.1
<#
.SYNOPSIS
    Prueft, ob alles fuer Matchball Tennis installiert ist (README 0.1).

.DESCRIPTION
    Prueft Unreal Engine 5.8, Blender 5.2 LTS, Visual Studio 2022 mit "Spieleentwicklung mit C++" und MSVC,
    Windows SDK, Git, Git LFS, GitHub-Kommandozeile gh (angemeldet), Windows "LongPathsEnabled",
    ob die Arbeitskopie unter OneDrive liegt, die Pfadlaenge, freien Speicher und optional CMake.

    Das Ergebnis erscheint als Markdown-Block (zum Einfuegen in STAND.md) und wird nach
    Unreal/Saved/installation.md geschrieben.

    Exit-Code: 0 = alles Noetige vorhanden (Warnungen erlaubt), 1 = etwas Noetiges fehlt.

.PARAMETER OhneDatei
    Nur anzeigen, Unreal/Saved/installation.md nicht schreiben.

.EXAMPLE
    powershell -NoProfile -ExecutionPolicy Bypass -File Unreal\Tools\pruefe_installation.ps1
#>
[CmdletBinding()]
param(
    [switch] $OhneDatei
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'MBGemeinsam.psm1') -Force -DisableNameChecking

$script:Ergebnisse = New-Object System.Collections.Generic.List[object]
$script:Aufgaben = New-Object System.Collections.Generic.List[string]

function Add-Ergebnis {
    # Status: OK, WARNUNG, FEHLT, FEHLER (beides zaehlt als "Noetiges fehlt"), INFO, OPTIONAL
    param([string] $Pruefung, [string] $Status, [string] $Details, [string[]] $Aufgaben = @())
    $script:Ergebnisse.Add([pscustomobject]@{ Pruefung = $Pruefung; Status = $Status; Details = $Details })
    foreach ($a in $Aufgaben) { if ($a) { $script:Aufgaben.Add($a) } }
    $farbe = 'Gray'
    switch ($Status) {
        'OK' { $farbe = 'Green' }
        'WARNUNG' { $farbe = 'Yellow' }
        'FEHLT' { $farbe = 'Red' }
        'FEHLER' { $farbe = 'Red' }
        'OPTIONAL' { $farbe = 'DarkYellow' }
        default { $farbe = 'Gray' }
    }
    Write-Host ('{0,-10} {1,-32} {2}' -f ('[' + $Status + ']'), $Pruefung, ($Details -replace '`', '')) -ForegroundColor $farbe
}

function Format-MdZelle {
    param([string] $Text)
    return ($Text -replace '\|', '\|' -replace "`r?`n", ' ')
}

function Get-ErsteZeile {
    param([string] $Text)
    foreach ($z in ($Text -split "`r?`n")) { if ($z.Trim() -ne '') { return $z.Trim() } }
    return ''
}

function ConvertTo-Version {
    # "14.38.33130" -> [System.Version], sonst $null
    param([string] $Text)
    $v = $null
    if ([System.Version]::TryParse($Text.Trim(), [ref]$v)) { return $v }
    return $null
}

function Test-VersionImBereich {
    # Bereich wie "14.38.33130-14.38.99999" (Unreal Windows_SDK.json) oder eine einzelne Version
    param([System.Version] $Version, [string] $Bereich)
    $teile = $Bereich.Split('-')
    $von = ConvertTo-Version $teile[0]
    $bis = $von
    if ($teile.Count -gt 1) { $bis = ConvertTo-Version $teile[1] }
    if ($null -eq $von -or $null -eq $bis) { return $false }
    return ($Version -ge $von -and $Version -le $bis)
}

$ergebnis = 0
try {
    $pfade = Get-MBPfade
    $istWindows = Test-MBWindows
    Write-MBTitel 'Matchball Tennis - Installation pruefen (README 0.1)'
    Write-Host ''

    # --- System --------------------------------------------------------------------------------
    $systemText = [System.Environment]::OSVersion.VersionString
    $build = 0
    if ($istWindows) {
        try {
            $os = Get-CimInstance -ClassName Win32_OperatingSystem -ErrorAction Stop
            $systemText = '{0} ({1})' -f $os.Caption, $os.Version
            $build = [int]$os.BuildNumber
        } catch {
            $build = [System.Environment]::OSVersion.Version.Build
        }
        if ($build -ge 22000) {
            Add-Ergebnis 'Windows' 'OK' $systemText
        } else {
            Add-Ergebnis 'Windows' 'WARNUNG' ($systemText + ' - das Spiel ist fuer Windows 11 ausgelegt') @('Auf Windows 11 aktualisieren (README 2.1).')
        }
    } else {
        Add-Ergebnis 'Windows' 'FEHLER' ($systemText + ' - kein Windows; Unreal, Blender-Pipeline und Builds laufen nur auf dem Windows-PC') @('Dieses Skript auf dem Windows-PC ausfuehren (README 0.0a).')
    }
    $psText = 'PowerShell {0} ({1})' -f $PSVersionTable.PSVersion.ToString(), (Get-MBEigenschaft $PSVersionTable 'PSEdition' 'Desktop')
    Add-Ergebnis 'PowerShell' 'INFO' $psText
    if ($istWindows) {
        try {
            $richtlinie = [string](Get-ExecutionPolicy -Scope CurrentUser)
            $wirksam = [string](Get-ExecutionPolicy)
            if ($wirksam -eq 'Restricted' -or $wirksam -eq 'AllSigned') {
                Add-Ergebnis 'Skriptausfuehrung' 'WARNUNG' ('ExecutionPolicy {0} - die .bat-Dateien funktionieren trotzdem, direkte Aufrufe von Tools\*.ps1 nicht' -f $wirksam) @('Fuer direkte Aufrufe: Set-ExecutionPolicy -Scope CurrentUser RemoteSigned')
            } else {
                Add-Ergebnis 'Skriptausfuehrung' 'OK' ('ExecutionPolicy {0} (CurrentUser: {1})' -f $wirksam, $richtlinie)
            }
        } catch {
            Add-Ergebnis 'Skriptausfuehrung' 'INFO' 'ExecutionPolicy nicht lesbar'
        }
    }

    # --- Unreal Engine 5.8 -----------------------------------------------------------------------
    $kandidaten = @(Get-MBUnrealKandidaten)
    $ue = $null
    foreach ($k in $kandidaten) { if ($k.Gueltig) { $ue = $k; break } }
    if ($null -ne $ue) {
        $art = 'Launcher-Installation'
        if (-not $ue.Installiert) { $art = 'aus dem Quellcode gebaut' }
        $details = '`{0}` - Version {1}, {2}, gefunden ueber {3}' -f $ue.Wurzel, $ue.Version, $art, $ue.Quelle
        if ($ue.Version.StartsWith('5.8.')) {
            Add-Ergebnis 'Unreal Engine 5.8' 'OK' $details
        } elseif ($ue.Version -eq '') {
            Add-Ergebnis 'Unreal Engine 5.8' 'WARNUNG' ($details + ' (Version nicht lesbar: Engine\Build\Build.version)')
        } else {
            Add-Ergebnis 'Unreal Engine 5.8' 'FEHLT' $details @(('Unreal Engine 5.8 installieren - gefunden wurde {0}.' -f $ue.Version))
        }
    } else {
        $geprueft = @()
        foreach ($k in $kandidaten) { $geprueft += ('`{0}` ({1})' -f $k.Wurzel, $k.Quelle) }
        $text = 'nicht gefunden'
        if ($geprueft.Count -gt 0) { $text += ' - geprueft: ' + ($geprueft -join ', ') }
        Add-Ergebnis 'Unreal Engine 5.8' 'FEHLT' $text @('Unreal Engine 5.8 im Epic Games Launcher installieren (oder MB_UE_PFAD auf den Ordner UE_5.8 setzen).')
    }
    if ($istWindows) {
        try {
            $basis = 'HKLM:\SOFTWARE\EpicGames\Unreal Engine'
            if (Test-Path -LiteralPath $basis) {
                $versionen = @(Get-ChildItem -LiteralPath $basis -ErrorAction Stop | ForEach-Object { $_.PSChildName } | Where-Object { $_ -ne '5.8' })
                if ($versionen.Count -gt 0) { Add-Ergebnis 'Weitere Unreal-Versionen' 'INFO' ($versionen -join ', ') }
            }
        } catch {
            Write-Verbose $_.Exception.Message
        }
    }

    # --- Blender 5.2 LTS -------------------------------------------------------------------------
    $blenderKandidaten = @(Get-MBBlenderKandidaten)
    $blender = $null
    foreach ($k in $blenderKandidaten) { if ($k.Gueltig) { $blender = $k; break } }
    if ($env:MB_BLENDER -and $blenderKandidaten.Count -gt 0 -and -not $blenderKandidaten[0].Gueltig) {
        Add-Ergebnis 'MB_BLENDER' 'WARNUNG' ('zeigt auf keine Datei: `{0}`' -f $env:MB_BLENDER) @('Umgebungsvariable MB_BLENDER korrigieren oder loeschen.')
    }
    if ($null -ne $blender) {
        $versionText = Get-MBBlenderVersion -Pfad $blender.Pfad
        $anzeige = $versionText
        if ($anzeige -eq '') { $anzeige = 'Version nicht ermittelbar (blender --version)' }
        $details = '`{0}` - {1}, gefunden ueber {2}' -f $blender.Pfad, $anzeige, $blender.Quelle
        if ($versionText -match 'Blender\s+5\.2(\.|\s|$)') {
            Add-Ergebnis 'Blender 5.2 LTS' 'OK' $details
        } elseif ($versionText -eq '') {
            Add-Ergebnis 'Blender 5.2 LTS' 'WARNUNG' $details @('Pruefen, ob Blender startet: blender --version')
        } else {
            Add-Ergebnis 'Blender 5.2 LTS' 'FEHLT' $details @('Blender 5.2 LTS installieren (blender.org); andere Versionen koennen andere Ergebnisse liefern.')
        }
    } else {
        Add-Ergebnis 'Blender 5.2 LTS' 'FEHLT' 'nicht gefunden' @('Blender 5.2 LTS installieren (blender.org) oder MB_BLENDER auf blender.exe setzen.')
    }

    # --- Visual Studio 2022, MSVC, Windows SDK ---------------------------------------------------
    $sdkVorgaben = $null
    if ($null -ne $ue -and (Test-Path -LiteralPath $ue.WindowsSdkJson -PathType Leaf)) {
        try { $sdkVorgaben = Read-MBJson $ue.WindowsSdkJson } catch { $sdkVorgaben = $null }
    }
    $vswhere = $null
    if ($istWindows) { $vswhere = Find-MBVsWhere }
    $vsGewaehlt = $null
    if ($null -eq $vswhere) {
        Add-Ergebnis 'Visual Studio 2022' 'FEHLT' 'Visual Studio Installer (vswhere.exe) nicht gefunden' @('Visual Studio 2022 Community installieren mit Workload "Spieleentwicklung mit C++".')
    } else {
        $alle = Invoke-MBBefehlAusgabe -Datei $vswhere -Argumente @('-all', '-products', '*', '-format', 'json', '-utf8')
        $instanzen = @()
        if ($alle.ExitCode -eq 0 -and $alle.Standard.Trim() -ne '') {
            # Erst einer Variablen zuweisen: Windows PowerShell 5.1 gibt ein JSON-Array als EIN Objekt aus,
            # @(... | ConvertFrom-Json) waere dort ein Array mit dem ganzen Array als einzigem Element.
            try { $roh = $alle.Standard | ConvertFrom-Json; $instanzen = @($roh) } catch { $instanzen = @() }
        }
        $mitSpiel = @{}
        $mitMsvc = @{}
        $abfrage = Invoke-MBBefehlAusgabe -Datei $vswhere -Argumente @('-all', '-products', '*', '-requires', 'Microsoft.VisualStudio.Workload.NativeGame', '-property', 'instanceId')
        foreach ($z in ($abfrage.Standard -split "`r?`n")) { if ($z.Trim()) { $mitSpiel[$z.Trim()] = $true } }
        $abfrage = Invoke-MBBefehlAusgabe -Datei $vswhere -Argumente @('-all', '-products', '*', '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'instanceId')
        foreach ($z in ($abfrage.Standard -split "`r?`n")) { if ($z.Trim()) { $mitMsvc[$z.Trim()] = $true } }

        $beschreibungen = @()
        $bester = $null
        $besterRang = -1
        foreach ($inst in $instanzen) {
            $id = [string](Get-MBEigenschaft $inst 'instanceId' '')
            $version = [string](Get-MBEigenschaft $inst 'installationVersion' '')
            $name = [string](Get-MBEigenschaft $inst 'displayName' 'Visual Studio')
            $beschreibungen += ('{0} {1}' -f $name, $version)
            $rang = 0
            if ($version.StartsWith('17.')) { $rang += 4 }
            if ($mitSpiel.ContainsKey($id)) { $rang += 2 }
            if ($mitMsvc.ContainsKey($id)) { $rang += 1 }
            if ($rang -gt $besterRang) { $besterRang = $rang; $bester = $inst }
        }
        if ($null -eq $bester) {
            Add-Ergebnis 'Visual Studio 2022' 'FEHLT' 'keine Visual-Studio-Installation gefunden' @('Visual Studio 2022 Community installieren mit Workload "Spieleentwicklung mit C++".')
        } else {
            $vsGewaehlt = $bester
            $id = [string](Get-MBEigenschaft $bester 'instanceId' '')
            $version = [string](Get-MBEigenschaft $bester 'installationVersion' '')
            $name = [string](Get-MBEigenschaft $bester 'displayName' 'Visual Studio')
            $details = '{0} {1}' -f $name, $version
            if ($beschreibungen.Count -gt 1) { $details += ' (installiert: ' + ($beschreibungen -join ', ') + ')' }
            $fehlt = @()
            if (-not $mitSpiel.ContainsKey($id)) { $fehlt += 'Workload "Spieleentwicklung mit C++" fehlt' }
            if (-not $mitMsvc.ContainsKey($id)) { $fehlt += 'MSVC-Compiler (C++ x64/x86-Buildtools) fehlt' }
            if ($fehlt.Count -gt 0) {
                Add-Ergebnis 'Visual Studio 2022' 'FEHLT' ($details + ' - ' + ($fehlt -join ', ')) @('Visual Studio Installer -> Aendern -> Workload "Spieleentwicklung mit C++" (mit "Unreal Engine-Installationsprogramm" und MSVC) auswaehlen.')
            } elseif (-not $version.StartsWith('17.')) {
                Add-Ergebnis 'Visual Studio 2022' 'WARNUNG' ($details + ' - keine 2022er Version (17.x); README verlangt Visual Studio 2022') @('Pruefen, ob Unreal Engine 5.8 diese Visual-Studio-Version unterstuetzt, sonst Visual Studio 2022 installieren [PRUEFEN].')
            } else {
                Add-Ergebnis 'Visual Studio 2022' 'OK' ($details + ' - Spieleentwicklung mit C++ und MSVC vorhanden')
            }
        }

        # MSVC-Versionen der gewaehlten Installation gegen die Vorgaben der Engine (Engine/Config/Windows/Windows_SDK.json)
        if ($null -ne $vsGewaehlt) {
            $vsPfad = [string](Get-MBEigenschaft $vsGewaehlt 'installationPath' '')
            $msvcOrdner = Join-MBPfad $vsPfad 'VC' 'Tools' 'MSVC'
            $msvcVersionen = @()
            if ($vsPfad -and (Test-Path -LiteralPath $msvcOrdner -PathType Container)) {
                $msvcVersionen = @(Get-ChildItem -LiteralPath $msvcOrdner -Directory | ForEach-Object { $_.Name } | Where-Object { $null -ne (ConvertTo-Version $_) })
            }
            $minimum = [string](Get-MBEigenschaft $sdkVorgaben 'MinimumVisualCppVersion' '')
            $bevorzugt = @(Get-MBEigenschaft $sdkVorgaben 'PreferredVisualCppVersions' @())
            if ($msvcVersionen.Count -eq 0) {
                Add-Ergebnis 'MSVC' 'FEHLT' 'kein MSVC-Toolset gefunden' @('Visual Studio Installer -> Einzelne Komponenten -> "MSVC v143 - VS 2022 C++ x64/x86-Buildtools" installieren.')
            } else {
                $details = 'installiert: ' + ($msvcVersionen -join ', ')
                $status = 'OK'
                $aufgabe = @()
                $minVersion = ConvertTo-Version $minimum
                if ($null -ne $minVersion) {
                    $details += ('; Unreal verlangt mindestens {0}' -f $minimum)
                    $genug = @($msvcVersionen | Where-Object { (ConvertTo-Version $_) -ge $minVersion })
                    if ($genug.Count -eq 0) {
                        $status = 'FEHLT'
                        $aufgabe += ('MSVC {0} oder neuer installieren (Visual Studio Installer -> Einzelne Komponenten).' -f $minimum)
                    }
                } else {
                    $details += '; Mindestversion der Engine unbekannt (Release Notes von UE 5.8 pruefen)'
                }
                if ($status -eq 'OK' -and $bevorzugt.Count -gt 0) {
                    $passend = @()
                    foreach ($m in $msvcVersionen) {
                        foreach ($b in $bevorzugt) { if (Test-VersionImBereich (ConvertTo-Version $m) ([string]$b)) { $passend += $m } }
                    }
                    if ($passend.Count -eq 0) {
                        $status = 'WARNUNG'
                        $details += ('; von Unreal bevorzugt: {0}' -f ($bevorzugt -join ', '))
                        $aufgabe += ('Fuer weniger Kompilierprobleme die von Unreal bevorzugte MSVC-Version installieren: {0}' -f ($bevorzugt -join ', '))
                    }
                }
                Add-Ergebnis 'MSVC' $status $details $aufgabe
            }
        }
    }
    if ($istWindows) {
        $kits = $null
        if (${env:ProgramFiles(x86)}) { $kits = Join-MBPfad ${env:ProgramFiles(x86)} 'Windows Kits' '10' 'Include' }
        $sdks = @()
        if ($kits -and (Test-Path -LiteralPath $kits -PathType Container)) {
            $sdks = @(Get-ChildItem -LiteralPath $kits -Directory | ForEach-Object { $_.Name } | Where-Object { $_ -like '10.*' } | Sort-Object { ConvertTo-Version $_ })
        }
        if ($sdks.Count -eq 0) {
            Add-Ergebnis 'Windows SDK' 'FEHLT' 'kein Windows 10/11 SDK gefunden' @('Visual Studio Installer -> Einzelne Komponenten -> aktuelles "Windows 11 SDK" installieren.')
        } else {
            $details = 'installiert: ' + ($sdks -join ', ')
            $status = 'OK'
            $aufgabe = @()
            $minSdk = ConvertTo-Version ([string](Get-MBEigenschaft $sdkVorgaben 'MinVersion' ''))
            $hauptSdk = [string](Get-MBEigenschaft $sdkVorgaben 'MainVersion' '')
            if ($hauptSdk) { $details += ('; Unreal empfiehlt {0}' -f $hauptSdk) }
            if ($null -ne $minSdk) {
                $genug = @($sdks | Where-Object { (ConvertTo-Version $_) -ge $minSdk })
                if ($genug.Count -eq 0) {
                    $status = 'FEHLT'
                    $aufgabe += ('Windows SDK {0} oder neuer installieren.' -f $minSdk)
                }
            }
            Add-Ergebnis 'Windows SDK' $status $details $aufgabe
        }
    }

    # --- Git, Git LFS, gh --------------------------------------------------------------------------
    $git = Invoke-MBBefehlAusgabe -Datei 'git' -Argumente @('--version')
    if (-not $git.Gefunden -or $git.ExitCode -ne 0) {
        Add-Ergebnis 'Git' 'FEHLT' 'git nicht gefunden' @('Git fuer Windows installieren (git-scm.com, enthaelt Git LFS).')
    } else {
        Add-Ergebnis 'Git' 'OK' (Get-ErsteZeile $git.Standard)
        $lfs = Invoke-MBBefehlAusgabe -Datei 'git' -Argumente @('lfs', 'version')
        if ($lfs.ExitCode -ne 0) {
            Add-Ergebnis 'Git LFS' 'FEHLT' 'git lfs nicht verfuegbar' @('Git LFS installieren (git-lfs.com) und einmal "git lfs install" ausfuehren.')
        } else {
            $filter = Invoke-MBBefehlAusgabe -Datei 'git' -Argumente @('config', '--get', 'filter.lfs.process') -Arbeitsordner $pfade.Repo
            if ($filter.ExitCode -eq 0 -and $filter.Standard.Trim() -ne '') {
                Add-Ergebnis 'Git LFS' 'OK' ((Get-ErsteZeile $lfs.Standard) + ', eingerichtet')
            } else {
                Add-Ergebnis 'Git LFS' 'WARNUNG' ((Get-ErsteZeile $lfs.Standard) + ', aber nicht eingerichtet') @('Einmal ausfuehren: git lfs install   (danach im Repository: git lfs pull)')
            }
        }
        $stand = Invoke-MBBefehlAusgabe -Datei 'git' -Argumente @('log', '-1', '--format=%h %cd', '--date=short') -Arbeitsordner $pfade.Repo
        $zweig = Invoke-MBBefehlAusgabe -Datei 'git' -Argumente @('rev-parse', '--abbrev-ref', 'HEAD') -Arbeitsordner $pfade.Repo
        if ($stand.ExitCode -eq 0 -and $zweig.ExitCode -eq 0) {
            Add-Ergebnis 'Repository-Stand' 'INFO' ('Branch {0}, Commit {1}' -f (Get-ErsteZeile $zweig.Standard), (Get-ErsteZeile $stand.Standard))
        } else {
            Add-Ergebnis 'Repository-Stand' 'WARNUNG' 'kein Git-Repository (Arbeitskopie mit git clone anlegen)' @('Repository mit git clone nach C:\Dev\MatchballTennis\ holen (README 0.0).')
        }
        if ($istWindows) {
            $lang = Invoke-MBBefehlAusgabe -Datei 'git' -Argumente @('config', '--get', 'core.longpaths') -Arbeitsordner $pfade.Repo
            if ($lang.Standard.Trim() -eq 'true') {
                Add-Ergebnis 'Git lange Pfade' 'OK' 'core.longpaths = true'
            } else {
                Add-Ergebnis 'Git lange Pfade' 'WARNUNG' 'core.longpaths ist nicht gesetzt' @('git config --global core.longpaths true')
            }
        }
    }
    $gh = Invoke-MBBefehlAusgabe -Datei 'gh' -Argumente @('--version')
    if (-not $gh.Gefunden -or $gh.ExitCode -ne 0) {
        Add-Ergebnis 'GitHub-Kommandozeile gh' 'FEHLT' 'gh nicht gefunden' @('GitHub CLI installieren: winget install --id GitHub.cli   danach: gh auth login')
    } else {
        $ghVersion = Get-ErsteZeile $gh.Standard
        $anmeldung = Invoke-MBBefehlAusgabe -Datei 'gh' -Argumente @('auth', 'status') -TimeoutSekunden 30
        if ($anmeldung.ExitCode -eq 0) {
            Add-Ergebnis 'GitHub-Kommandozeile gh' 'OK' ($ghVersion + ', angemeldet')
        } else {
            Add-Ergebnis 'GitHub-Kommandozeile gh' 'FEHLT' ($ghVersion + ', nicht angemeldet') @('gh auth login   (GitHub.com, HTTPS, im Browser anmelden)')
        }
    }

    # --- Windows: lange Pfade, OneDrive, Pfadlaenge, Speicher ---------------------------------------
    if ($istWindows) {
        $wert = $null
        try {
            $eintrag = Get-ItemProperty -LiteralPath 'HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem' -ErrorAction Stop
            $wert = Get-MBEigenschaft $eintrag 'LongPathsEnabled' $null
        } catch {
            $wert = $null
        }
        if ($null -ne $wert -and [int]$wert -eq 1) {
            Add-Ergebnis 'Lange Pfade (LongPathsEnabled)' 'OK' 'aktiviert'
        } else {
            Add-Ergebnis 'Lange Pfade (LongPathsEnabled)' 'WARNUNG' 'nicht aktiviert (Windows-Grenze 260 Zeichen)' @("PowerShell als Administrator: New-ItemProperty -Path 'HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem' -Name LongPathsEnabled -Value 1 -PropertyType DWORD -Force   (danach neu anmelden)")
        }
    }

    $repo = $pfade.Repo
    $oneDrive = $null
    foreach ($name in @('OneDrive', 'OneDriveConsumer', 'OneDriveCommercial')) {
        $wert = [System.Environment]::GetEnvironmentVariable($name)
        if ($wert -and (Test-MBPfadUnter -Basis $wert -Pfad $repo)) { $oneDrive = $wert }
    }
    if ($null -eq $oneDrive -and $repo -match '[\\/]OneDrive( - [^\\/]+)?([\\/]|$)') { $oneDrive = 'OneDrive' }
    $laenge = $repo.Length
    $details = '`{0}` ({1} Zeichen)' -f $repo, $laenge
    if ($null -ne $oneDrive) {
        Add-Ergebnis 'Arbeitskopie' 'FEHLER' ($details + ' - liegt in OneDrive') @('Repository ausserhalb von OneDrive neu klonen, z. B. nach C:\Dev\MatchballTennis\ (README 0.0).')
    } elseif ($laenge -gt 80) {
        Add-Ergebnis 'Arbeitskopie' 'FEHLER' ($details + ' - Pfad viel zu lang fuer Unreal') @('Repository mit kurzem Pfad klonen, z. B. C:\Dev\MatchballTennis\ (README 0.0).')
    } elseif ($laenge -gt 40) {
        Add-Ergebnis 'Arbeitskopie' 'WARNUNG' ($details + ' - Pfad recht lang, Unreal erzeugt tiefe Unterordner') @('Besser mit kurzem Pfad klonen, z. B. C:\Dev\MatchballTennis\ (README 0.0).')
    } else {
        Add-Ergebnis 'Arbeitskopie' 'OK' ($details + ', nicht in OneDrive')
    }

    # Laengster vorhandener Pfad (ohne von Unreal erzeugte Ordner)
    $auslassen = @('.git', 'Binaries', 'Intermediate', 'Saved', 'DerivedDataCache', 'build-win', 'build')
    $laengster = ''
    $stapel = New-Object System.Collections.Generic.Stack[string]
    $stapel.Push($repo)
    while ($stapel.Count -gt 0) {
        $ordner = $stapel.Pop()
        try {
            foreach ($eintrag in [System.IO.Directory]::GetFileSystemEntries($ordner)) {
                if ($eintrag.Length -gt $laengster.Length) { $laengster = $eintrag }
                if ([System.IO.Directory]::Exists($eintrag)) {
                    $name = [System.IO.Path]::GetFileName($eintrag)
                    if ($auslassen -notcontains $name -and $name -notlike 'build-*') { $stapel.Push($eintrag) }
                }
            }
        } catch {
            Write-Verbose $_.Exception.Message
        }
    }
    if ($laengster.Length -ge 240) {
        Add-Ergebnis 'Laengster Pfad' 'WARNUNG' ('{0} Zeichen: `{1}`' -f $laengster.Length, $laengster) @('Arbeitskopie naeher an die Laufwerkswurzel legen oder LongPathsEnabled aktivieren.')
    } else {
        Add-Ergebnis 'Laengster Pfad' 'INFO' ('{0} Zeichen im Repository' -f $laengster.Length)
    }

    try {
        $wurzel = [System.IO.Path]::GetPathRoot($repo)
        $laufwerk = New-Object System.IO.DriveInfo -ArgumentList $wurzel
        $frei = $laufwerk.AvailableFreeSpace
        $details = '{0} frei auf {1}' -f (Format-MBGroesse $frei), $wurzel
        if ($frei -lt 50GB) {
            Add-Ergebnis 'Freier Speicher' 'WARNUNG' ($details + ' - Unreal braucht fuer Zwischendateien und Builds viel Platz (50 GB+)') @('Speicherplatz freigeben (Unreal-Zwischendateien, gepackte Builds unter C:\MatchballTennis-Build\).')
        } else {
            Add-Ergebnis 'Freier Speicher' 'OK' $details
        }
    } catch {
        Add-Ergebnis 'Freier Speicher' 'INFO' 'nicht ermittelbar'
    }

    # --- Optional: CMake fuer die Kern-Tests ohne Unreal --------------------------------------------
    $cmake = Find-MBCMake
    if ($null -ne $cmake) {
        $cv = Invoke-MBBefehlAusgabe -Datei $cmake.CMake -Argumente @('--version')
        Add-Ergebnis 'CMake (optional)' 'OK' ('{0} (`{1}`, {2})' -f (Get-ErsteZeile $cv.Standard), $cmake.CMake, $cmake.Quelle)
    } else {
        Add-Ergebnis 'CMake (optional)' 'OPTIONAL' 'nicht gefunden - die Kern-Tests laufen dann nur ueber Unreal (test.ps1)' @('Optional: im Visual Studio Installer "C++-CMake-Tools fuer Windows" hinzufuegen (fuer Tools\test.ps1 -NurKern).')
    }

    # --- Markdown --------------------------------------------------------------------------------
    $fehlend = @($script:Ergebnisse | Where-Object { $_.Status -eq 'FEHLT' -or $_.Status -eq 'FEHLER' })
    $warnungen = @($script:Ergebnisse | Where-Object { $_.Status -eq 'WARNUNG' })
    $rechner = [System.Environment]::MachineName
    $md = New-Object System.Text.StringBuilder
    [void]$md.AppendLine('## Installation')
    [void]$md.AppendLine('')
    [void]$md.AppendLine(('Geprueft mit `Unreal/Tools/pruefe_installation.ps1` am {0} auf `{1}`.' -f (Get-Date).ToString('yyyy-MM-dd HH:mm'), $rechner))
    [void]$md.AppendLine('')
    [void]$md.AppendLine('| Pruefung | Status | Details |')
    [void]$md.AppendLine('|---|---|---|')
    foreach ($e in $script:Ergebnisse) {
        [void]$md.AppendLine(('| {0} | {1} | {2} |' -f (Format-MdZelle $e.Pruefung), $e.Status, (Format-MdZelle $e.Details)))
    }
    [void]$md.AppendLine('')
    if ($script:Aufgaben.Count -gt 0) {
        [void]$md.AppendLine('**Zu erledigen:**')
        [void]$md.AppendLine('')
        foreach ($a in $script:Aufgaben) { [void]$md.AppendLine(('- {0}' -f $a)) }
    } else {
        [void]$md.AppendLine('Alles Noetige ist installiert.')
    }
    $markdown = $md.ToString()

    Write-Host ''
    Write-Host '----- Markdown fuer STAND.md (Anfang) -----' -ForegroundColor Cyan
    Write-Host $markdown
    Write-Host '----- Markdown fuer STAND.md (Ende) -----' -ForegroundColor Cyan

    if (-not $OhneDatei) {
        $datei = Join-Path $pfade.Saved 'installation.md'
        Write-MBTextDatei -Pfad $datei -Text $markdown
        Write-MBInfo ('Gespeichert: {0}' -f $datei)
    }

    Write-Host ''
    if ($fehlend.Count -gt 0) {
        Write-MBFehler ('{0} noetige Punkte fehlen, {1} Warnungen. Siehe "Zu erledigen".' -f $fehlend.Count, $warnungen.Count)
        $ergebnis = 1
    } elseif ($warnungen.Count -gt 0) {
        Write-MBWarnung ('Alles Noetige ist da, aber {0} Warnungen. Siehe "Zu erledigen".' -f $warnungen.Count)
    } else {
        Write-MBOk 'Alles Noetige ist installiert.'
    }
} catch {
    $ergebnis = Write-MBAbbruch $_
}
exit $ergebnis
