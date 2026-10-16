#Requires -Version 5.1
<#
.SYNOPSIS
    Erzeugt die Schiedsrichter-Stimme mit ElevenLabs (README 16.2) - nur fehlende Bausteine.

.DESCRIPTION
    Bausteine:
      - Satzteile aus Unreal/Data/ansagen.csv (Spalten Name,Text; Text darf in Anfuehrungszeichen Kommas enthalten)
        -> Unreal/SourceAssets/Audio/Umpire/<Name>.wav, z. B. fifteen.wav, game_set_and_match.wav
      - Spielernamen (Spalte Nachname aus Unreal/Data/spieler.csv) und optional -Namen "A,B"
        -> Unreal/SourceAssets/Audio/Umpire/name_<nachname>.wav, gesprochen wird der Nachname.
        Dateiname: Umlaute ae/oe/ue/ss, Akzente entfernt (Hruska), Kleinbuchstaben, andere Zeichen -> "_"
        (z. B. "Bl" + a-Akut + "hov" + a-Akut -> name_blahova, "Van der Berg" -> name_van_der_berg).
      Satzzeichen-Bausteine "," und "." sind Pausen im Spiel und werden nicht erzeugt.
    Es werden nur Bausteine erzeugt, deren WAV-Datei fehlt. Zum Neu-Erzeugen die WAV-Datei loeschen.
    Neben jede WAV kommt <id>.meta.json (Quelle, Stimme, Modell, Text, Datum).
    Danach importiert Tools\assets.ps1 die Dateien nach Unreal (Content/Audio/Umpire).

    Schluessel: wird abgefragt (Read-Host -AsSecureString), nur im Speicher gehalten, nie gespeichert und
    nie ausgegeben.

    API: POST <ApiAdresse>/v1/text-to-speech/<voice_id>?output_format=pcm_24000
         Header xi-api-key, JSON { "text": ..., "model_id": ... }
         Antwort: rohes PCM 16 Bit mono 24 kHz -> das Skript setzt einen WAV-Kopf (RIFF) davor.
    Fehler: 401 = Schluessel falsch (bzw. Kontingent aufgebraucht), 429 = zu viele Anfragen -> warten und erneut.

    Lizenz: Nutzung der erzeugten Stimme nach den ElevenLabs-Bedingungen des eigenen Tarifs [PRUEFEN, README 22.2].

.PARAMETER StimmeId
    ElevenLabs voice_id (Standard: "Alice", britisch, Xb7hH8MSUJpSbSDYk0k2).

.PARAMETER Modell
    ElevenLabs model_id (Standard eleven_multilingual_v2).

.PARAMETER Namen
    Zusaetzliche Namen, durch Komma getrennt, z. B. -Namen "Meier,Huber".

.PARAMETER Liste
    Nur anzeigen, welche Bausteine fehlen (kein Schluessel noetig).

.PARAMETER Ziel
    Anderer Ausgabeordner (Standard Unreal/SourceAssets/Audio/Umpire).

.PARAMETER ApiAdresse
    Basisadresse der API (nur fuer Tests aendern).

.EXAMPLE
    "ElevenLabs-Stimme erzeugen.bat" doppelklicken
.EXAMPLE
    powershell -NoProfile -ExecutionPolicy Bypass -File Unreal\Tools\elevenlabs-stimme.ps1 -Liste
.EXAMPLE
    powershell -NoProfile -ExecutionPolicy Bypass -File Unreal\Tools\elevenlabs-stimme.ps1 -Namen "Meier,Huber"
#>
[CmdletBinding()]
param(
    [string] $StimmeId = 'Xb7hH8MSUJpSbSDYk0k2',
    [string] $Modell = 'eleven_multilingual_v2',
    [string[]] $Namen = @(),
    [switch] $Liste,
    [string] $Ziel = '',
    [string] $ApiAdresse = 'https://api.elevenlabs.io'
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'MBGemeinsam.psm1') -Force -DisableNameChecking

$script:Abtastrate = 24000

function ConvertTo-NamensId {
    # "Hruska" mit Hacek -> "name_hruska"; liefert $null, wenn nichts Brauchbares uebrig bleibt
    param([string] $Name)
    $t = $Name.Trim()
    # Deutsche Umlaute zuerst (ae/oe/ue/ss statt a/o/u)
    $ersetzungen = @(
        @(0x00E4, 'ae'), @(0x00F6, 'oe'), @(0x00FC, 'ue'), @(0x00C4, 'Ae'), @(0x00D6, 'Oe'), @(0x00DC, 'Ue'), @(0x00DF, 'ss'),
        @(0x00F8, 'o'), @(0x00D8, 'O'), @(0x00E6, 'ae'), @(0x00C6, 'Ae'), @(0x0153, 'oe'), @(0x0152, 'Oe'),
        @(0x0142, 'l'), @(0x0141, 'L'), @(0x0111, 'd'), @(0x0110, 'D'), @(0x00FE, 'th'), @(0x00DE, 'Th'), @(0x0131, 'i'))
    foreach ($e in $ersetzungen) { $t = $t.Replace([string][char]$e[0], $e[1]) }
    # Akzente entfernen: zerlegen (FormD) und die kombinierenden Zeichen weglassen
    $zerlegt = $t.Normalize([System.Text.NormalizationForm]::FormD)
    $sb = New-Object System.Text.StringBuilder
    foreach ($zeichen in $zerlegt.ToCharArray()) {
        if ([System.Globalization.CharUnicodeInfo]::GetUnicodeCategory($zeichen) -ne [System.Globalization.UnicodeCategory]::NonSpacingMark) {
            [void]$sb.Append($zeichen)
        }
    }
    $ascii = ($sb.ToString().ToLowerInvariant() -replace '[^a-z0-9]+', '_').Trim('_')
    if ($ascii -eq '') { return $null }
    return ('name_' + $ascii)
}

function Get-Bausteine {
    param($Pfade, [string] $Ordner, [string[]] $ZusatzNamen)
    $liste = New-Object System.Collections.Generic.List[object]
    $ids = @{}

    $ansagenCsv = Join-Path $Pfade.Data 'ansagen.csv'
    if (-not (Test-Path -LiteralPath $ansagenCsv -PathType Leaf)) { Stop-MBMitFehler ('Datei fehlt: {0}' -f $ansagenCsv) }
    foreach ($zeile in (Import-Csv -LiteralPath $ansagenCsv -Encoding UTF8)) {
        $id = [string](Get-MBEigenschaft $zeile 'Name' '')
        $text = [string](Get-MBEigenschaft $zeile 'Text' '')
        $id = $id.Trim()
        $text = $text.Trim()
        if ($id -eq ',' -or $id -eq '.') { continue }
        if ($id -notmatch '^[A-Za-z0-9_]+$') { Write-MBWarnung ('ansagen.csv: ungueltige Baustein-ID "{0}" - uebersprungen.' -f $id); continue }
        if ($text -eq '') { Write-MBWarnung ('ansagen.csv: Baustein {0} hat keinen Text - uebersprungen.' -f $id); continue }
        if ($ids.ContainsKey($id)) { Write-MBWarnung ('ansagen.csv: Baustein {0} doppelt - erster Eintrag gilt.' -f $id); continue }
        $ids[$id] = $true
        $liste.Add([pscustomobject]@{ Id = $id; Text = $text; Art = 'Ansage'; Datei = (Join-Path $Ordner ($id + '.wav')) })
    }

    $namen = New-Object System.Collections.Generic.List[string]
    $spielerCsv = Join-Path $Pfade.Data 'spieler.csv'
    if (Test-Path -LiteralPath $spielerCsv -PathType Leaf) {
        foreach ($zeile in (Import-Csv -LiteralPath $spielerCsv -Encoding UTF8)) {
            $nachname = [string](Get-MBEigenschaft $zeile 'Nachname' '')
            if ($nachname.Trim() -ne '') { $namen.Add($nachname.Trim()) }
        }
    } else {
        Write-MBWarnung ('{0} fehlt - nur Namen aus -Namen.' -f $spielerCsv)
    }
    foreach ($eintrag in $ZusatzNamen) {
        foreach ($teil in ([string]$eintrag).Split(',')) {
            if ($teil.Trim() -ne '') { $namen.Add($teil.Trim()) }
        }
    }
    foreach ($name in $namen) {
        $id = ConvertTo-NamensId $name
        if ($null -eq $id) { Write-MBWarnung ('Name "{0}" ergibt keinen Dateinamen - uebersprungen.' -f $name); continue }
        if ($ids.ContainsKey($id)) { continue }
        $ids[$id] = $true
        $liste.Add([pscustomobject]@{ Id = $id; Text = $name; Art = 'Name'; Datei = (Join-Path $Ordner ($id + '.wav')) })
    }
    return $liste
}

function Test-BausteinVorhanden {
    param($Baustein)
    if (-not (Test-Path -LiteralPath $Baustein.Datei -PathType Leaf)) { return $false }
    return ((Get-Item -LiteralPath $Baustein.Datei).Length -gt 44)
}

function Write-Wav {
    # Rohes PCM (16 Bit, mono, little endian) mit RIFF/WAVE-Kopf speichern
    param([string] $Pfad, [byte[]] $Pcm, [int] $Abtastrate)
    $laenge = $Pcm.Length - ($Pcm.Length % 2)
    $ascii = [System.Text.Encoding]::ASCII
    $speicher = New-Object System.IO.MemoryStream
    $schreiber = New-Object System.IO.BinaryWriter -ArgumentList $speicher
    try {
        $schreiber.Write($ascii.GetBytes('RIFF'))
        $schreiber.Write([int](36 + $laenge))
        $schreiber.Write($ascii.GetBytes('WAVE'))
        $schreiber.Write($ascii.GetBytes('fmt '))
        $schreiber.Write([int]16)                 # Groesse des fmt-Blocks
        $schreiber.Write([int16]1)                # PCM
        $schreiber.Write([int16]1)                # mono
        $schreiber.Write([int]$Abtastrate)
        $schreiber.Write([int]($Abtastrate * 2))  # Bytes pro Sekunde
        $schreiber.Write([int16]2)                # Bytes pro Abtastwert
        $schreiber.Write([int16]16)               # Bit pro Abtastwert
        $schreiber.Write($ascii.GetBytes('data'))
        $schreiber.Write([int]$laenge)
        $schreiber.Write($Pcm, 0, $laenge)
        $schreiber.Flush()
        $temp = $Pfad + '.tmp'
        [System.IO.File]::WriteAllBytes($temp, $speicher.ToArray())
        Move-Item -LiteralPath $temp -Destination $Pfad -Force
    } finally {
        $schreiber.Dispose()
        $speicher.Dispose()
    }
}

function Get-FehlerText {
    # Fehlermeldung aus der API-Antwort (JSON "detail") - enthaelt nie den Schluessel
    param([byte[]] $Bytes)
    if ($null -eq $Bytes -or $Bytes.Length -eq 0) { return '' }
    $text = [System.Text.Encoding]::UTF8.GetString($Bytes)
    try {
        $json = $text | ConvertFrom-Json
        $detail = Get-MBEigenschaft $json 'detail' $null
        if ($detail -is [string]) { return $detail }
        $meldung = [string](Get-MBEigenschaft $detail 'message' '')
        $status = [string](Get-MBEigenschaft $detail 'status' '')
        $teile = @()
        if ($status) { $teile += $status }
        if ($meldung) { $teile += $meldung }
        if ($teile.Count -gt 0) { return ($teile -join ': ') }
    } catch {
        Write-Verbose 'Antwort ist kein JSON.'
    }
    if ($text.Length -gt 300) { $text = $text.Substring(0, 300) + ' ...' }
    return $text
}

function Invoke-Sprachausgabe {
    # Fragt ElevenLabs nach dem PCM-Audio fuer einen Text. Wiederholt bei 429/5xx/Netzfehlern.
    # Gibt die PCM-Bytes zurueck oder $null, wenn es nach allen Versuchen nicht geklappt hat.
    param($Client, [string] $Adresse, [string] $Schluessel, [string] $Text, [string] $Modell, [string] $Id)
    $koerper = ConvertTo-Json -InputObject ([ordered]@{ text = $Text; model_id = $Modell }) -Compress
    $maxVersuche = 6
    for ($versuch = 1; $versuch -le $maxVersuche; $versuch++) {
        $warten = [int][Math]::Min(60, 5 * [Math]::Pow(2, $versuch - 1))
        $anfrage = New-Object System.Net.Http.HttpRequestMessage -ArgumentList ([System.Net.Http.HttpMethod]::Post), $Adresse
        [void]$anfrage.Headers.TryAddWithoutValidation('xi-api-key', $Schluessel)
        [void]$anfrage.Headers.TryAddWithoutValidation('Accept', '*/*')
        $anfrage.Content = New-Object System.Net.Http.StringContent -ArgumentList $koerper, ([System.Text.Encoding]::UTF8), 'application/json'
        $antwort = $null
        try {
            $antwort = $Client.SendAsync($anfrage).GetAwaiter().GetResult()
        } catch {
            $grund = $_.Exception.Message
            if ($null -ne $_.Exception.InnerException) { $grund = $_.Exception.InnerException.Message }
            if ($versuch -lt $maxVersuche) {
                Write-MBWarnung ('{0}: Verbindung fehlgeschlagen ({1}) - neuer Versuch in {2} s.' -f $Id, $grund, $warten)
                Start-Sleep -Seconds $warten
                continue
            }
            Write-MBFehler ('{0}: Verbindung fehlgeschlagen ({1}).' -f $Id, $grund)
            return $null
        } finally {
            $anfrage.Dispose()
        }
        try {
            $status = [int]$antwort.StatusCode
            $bytes = $antwort.Content.ReadAsByteArrayAsync().GetAwaiter().GetResult()
            if ($status -eq 200) { return , $bytes }
            $detail = Get-FehlerText $bytes
            if ($status -eq 401) {
                if ($detail -match '(?i)quota') {
                    Stop-MBMitFehler 'ElevenLabs: Zeichen-Kontingent aufgebraucht (401).' -Hinweise @($detail, 'Im ElevenLabs-Konto das Kontingent pruefen; bereits erzeugte Bausteine bleiben erhalten.')
                }
                Stop-MBMitFehler 'ElevenLabs: Schluessel falsch oder abgelaufen (401).' -Hinweise @($detail, 'Schluessel unter elevenlabs.io -> Profil -> API Keys pruefen und das Skript neu starten.')
            }
            if ($status -eq 429 -or $status -ge 500) {
                $ra = $antwort.Headers.RetryAfter
                if ($null -ne $ra -and $null -ne $ra.Delta) { $warten = [int][Math]::Min(120, [Math]::Max(1, $ra.Delta.TotalSeconds)) }
                if ($versuch -lt $maxVersuche) {
                    $art = 'zu viele Anfragen'
                    if ($status -ge 500) { $art = 'Serverfehler' }
                    Write-MBWarnung ('{0}: {1} ({2}) - warte {3} s und versuche es erneut.' -f $Id, $art, $status, $warten)
                    Start-Sleep -Seconds $warten
                    continue
                }
                Write-MBFehler ('{0}: nach {1} Versuchen aufgegeben ({2}: {3}).' -f $Id, $maxVersuche, $status, $detail)
                return $null
            }
            if ($status -eq 404) {
                Stop-MBMitFehler ('ElevenLabs: Stimme "{0}" nicht gefunden (404).' -f $script:StimmeIdAktiv) -Hinweise @($detail, 'Die voice_id unter elevenlabs.io -> Voices nachsehen und mit -StimmeId angeben.')
            }
            if ($status -eq 400 -or $status -eq 422) {
                Stop-MBMitFehler ('ElevenLabs lehnt die Anfrage ab ({0}).' -f $status) -Hinweise @($detail, ('Modell "{0}" und Stimme pruefen (-Modell, -StimmeId).' -f $Modell))
            }
            Stop-MBMitFehler ('ElevenLabs: unerwartete Antwort {0}.' -f $status) -Hinweise @($detail)
        } finally {
            $antwort.Dispose()
        }
    }
    return $null
}

$ergebnis = 0
$sicher = $null
$schluessel = $null
$client = $null
try {
    $pfade = Get-MBPfade
    Write-MBTitel 'Matchball Tennis - Schiedsrichter-Stimme mit ElevenLabs (README 16.2)'
    $ordner = $Ziel
    if ($ordner -eq '') { $ordner = $pfade.Umpire }
    $ordner = [System.IO.Path]::GetFullPath($ordner)
    if ($StimmeId -notmatch '^[A-Za-z0-9]+$') { Stop-MBMitFehler ('Ungueltige Stimme-Id: {0}' -f $StimmeId) }
    if ($Modell -notmatch '^[A-Za-z0-9_\-.]+$') { Stop-MBMitFehler ('Ungueltiges Modell: {0}' -f $Modell) }
    $script:StimmeIdAktiv = $StimmeId

    $bausteine = @(Get-Bausteine -Pfade $pfade -Ordner $ordner -ZusatzNamen $Namen)
    $fehlend = @($bausteine | Where-Object { -not (Test-BausteinVorhanden $_) })
    $fehlendeNamen = @($fehlend | Where-Object { $_.Art -eq 'Name' })
    $zeichen = 0
    foreach ($b in $fehlend) { $zeichen += $b.Text.Length }
    Write-MBInfo ('Ordner:     {0}' -f $ordner)
    Write-MBInfo ('Bausteine:  {0} ({1} Satzteile, {2} Namen)' -f $bausteine.Count, @($bausteine | Where-Object { $_.Art -eq 'Ansage' }).Count, @($bausteine | Where-Object { $_.Art -eq 'Name' }).Count)
    Write-MBInfo ('Vorhanden:  {0}' -f ($bausteine.Count - $fehlend.Count))
    Write-MBInfo ('Fehlend:    {0} (ca. {1} Zeichen bei ElevenLabs)' -f $fehlend.Count, $zeichen)

    if ($Liste) {
        if ($fehlend.Count -gt 0) {
            Write-MBSchritt 'Fehlende Bausteine'
            foreach ($b in $fehlend) { Write-Host ('  {0,-36} "{1}"' -f ([System.IO.Path]::GetFileName($b.Datei)), $b.Text) }
        }
        if ($fehlendeNamen.Count -gt 0) {
            Write-MBInfo ('Namen ohne Aufnahme: {0}' -f (($fehlendeNamen | ForEach-Object { $_.Text }) -join ', '))
        }
        if ($fehlend.Count -eq 0) { Write-MBOk 'Alle Bausteine sind vorhanden.' }
    } elseif ($fehlend.Count -eq 0) {
        Write-MBOk 'Alle Bausteine sind vorhanden - nichts zu tun.'
    } else {
        Write-MBInfo ('Stimme:     {0}   Modell: {1}' -f $StimmeId, $Modell)
        Write-Host ''
        $umgeleitet = $false
        try { $umgeleitet = [System.Console]::IsInputRedirected } catch { $umgeleitet = $false }
        if ($umgeleitet) {
            # Eingabe kommt aus einer Umleitung (z. B. Passwort-Manager | Skript): erste Zeile ist der Schluessel.
            # Read-Host -AsSecureString funktioniert dann nicht. Auch hier wird nichts gespeichert oder angezeigt.
            Write-MBInfo 'Standardeingabe ist umgeleitet - lese den Schluessel aus der ersten Zeile.'
            $schluessel = [System.Console]::In.ReadLine()
        } else {
            $sicher = Read-Host -Prompt 'ElevenLabs-Schluessel eingeben (wird nicht angezeigt und nicht gespeichert)' -AsSecureString
            $schluessel = (New-Object System.Net.NetworkCredential -ArgumentList '', $sicher).Password
        }
        if ([string]::IsNullOrWhiteSpace($schluessel)) { Stop-MBMitFehler 'Kein Schluessel eingegeben - abgebrochen.' }
        $schluessel = $schluessel.Trim()

        if ($PSVersionTable.PSVersion.Major -lt 6) {
            # Windows PowerShell 5.1 (.NET Framework): TLS 1.2 sicher einschalten
            [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor [System.Net.SecurityProtocolType]::Tls12
        }
        Add-Type -AssemblyName System.Net.Http
        $client = New-Object System.Net.Http.HttpClient
        $client.Timeout = [TimeSpan]::FromSeconds(120)
        $adresse = '{0}/v1/text-to-speech/{1}?output_format=pcm_{2}' -f $ApiAdresse.TrimEnd('/'), $StimmeId, $script:Abtastrate
        [void](New-MBOrdner $ordner)

        Write-MBSchritt ('Erzeuge {0} Bausteine' -f $fehlend.Count)
        $erzeugt = 0
        $fehlgeschlagen = @()
        $nummer = 0
        foreach ($b in $fehlend) {
            $nummer++
            $pcm = Invoke-Sprachausgabe -Client $client -Adresse $adresse -Schluessel $schluessel -Text $b.Text -Modell $Modell -Id $b.Id
            if ($null -eq $pcm -or $pcm.Length -lt 2) {
                $fehlgeschlagen += $b.Id
                continue
            }
            Write-Wav -Pfad $b.Datei -Pcm $pcm -Abtastrate $script:Abtastrate
            $meta = [ordered]@{
                quelle      = 'ElevenLabs Text-to-Speech (Tools/elevenlabs-stimme.ps1, README 16.2)'
                lizenz      = 'ElevenLabs-Nutzungsbedingungen des eigenen Tarifs [PRUEFEN, README 22.2]'
                datum       = (Get-Date).ToString('yyyy-MM-dd')
                baustein    = $b.Id
                art         = $b.Art
                text        = $b.Text
                stimme_id   = $StimmeId
                modell      = $Modell
                format      = ('PCM 16 Bit mono {0} Hz' -f $script:Abtastrate)
                ziel_unreal = '/Game/Audio/Umpire/'
            }
            Write-MBTextDatei -Pfad ([System.IO.Path]::ChangeExtension($b.Datei, '.meta.json')) -Text (ConvertTo-MBJson $meta)
            $sekunden = ($pcm.Length / 2.0) / $script:Abtastrate
            if ($sekunden -lt 0.1) { Write-MBWarnung ('{0}: sehr kurz ({1:N2} s) - bitte anhoeren.' -f $b.Id, $sekunden) }
            Write-MBOk ('[{0}/{1}] {2}.wav  "{3}"  ({4:N2} s)' -f $nummer, $fehlend.Count, $b.Id, $b.Text, $sekunden)
            $erzeugt++
            Start-Sleep -Milliseconds 250
        }

        Write-MBSchritt 'Zusammenfassung'
        Write-MBInfo ('Erzeugt: {0}, fehlgeschlagen: {1}' -f $erzeugt, $fehlgeschlagen.Count)
        if ($erzeugt -gt 0) { Write-MBInfo 'Danach Tools\assets.ps1 -NurImport ausfuehren, damit Unreal die neuen WAV-Dateien uebernimmt.' }
        if ($fehlgeschlagen.Count -gt 0) {
            Stop-MBMitFehler ('{0} Bausteine konnten nicht erzeugt werden: {1}' -f $fehlgeschlagen.Count, ($fehlgeschlagen -join ', ')) -Hinweise @('Skript spaeter erneut starten - es erzeugt nur die fehlenden Bausteine.')
        }
        Write-MBOk 'Fertig.'
    }
} catch {
    $ergebnis = Write-MBAbbruch $_
} finally {
    # Schluessel aus dem Speicher entfernen
    $schluessel = $null
    if ($null -ne $sicher) { $sicher.Dispose() }
    if ($null -ne $client) { $client.Dispose() }
}
exit $ergebnis
