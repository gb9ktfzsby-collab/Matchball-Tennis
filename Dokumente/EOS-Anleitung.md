# Anleitung: Epic Online Services (EOS) für das Online-Spiel anlegen

Wird für **Meilenstein M9** (Online gegen Freunde, README 15) gebraucht – nicht vorher.
EOS ist kostenlos. Spieler brauchen **kein** Epic-Konto: das Spiel meldet sich mit einer anonymen
Geräte-Id an (EOS „Connect"-Schnittstelle, Anmeldeart *Device ID*). Nur du als Entwickler brauchst ein
Epic-Konto für das Entwicklerportal.

> Hinweis: Die Oberfläche des Portals ändert sich gelegentlich. Wenn ein Menüpunkt anders heißt,
> schick mir einen Screenshot – ich passe die Anleitung an. **[PRÜFEN]**

## 1. Im Entwicklerportal anmelden
1. https://dev.epicgames.com/portal öffnen und mit deinem Epic-Konto anmelden
   (bei Bedarf Konto anlegen und die Zwei-Faktor-Anmeldung einschalten – das Portal verlangt sie).
2. Beim ersten Mal eine **Organisation** anlegen, z. B. „Matchball Tennis (privat)".
   Den Bedingungen für Epic Online Services zustimmen.

## 2. Produkt anlegen
1. Links **Produkt erstellen** (Create Product) → Name **Matchball Tennis**.
2. Das Portal legt automatisch **Sandboxes** (Live, Stage, Dev) und je eine **Deployment** an.
3. Unter **Produkteinstellungen** (Product Settings) findest du:
   * **Product ID**
   * **Sandbox ID** und **Deployment ID** – wir nehmen zum Entwickeln die *Dev*-Sandbox
     (später für die Freunde die *Live*-Sandbox).

## 3. Client anlegen (Zugangsdaten für das Spiel)
1. **Produkteinstellungen → Clients** (bzw. „Client credentials").
2. Zuerst eine **Client Policy** anlegen: Typ **GameClient** (erlaubt Lobbys und P2P für Spieler ohne
   eigenen Server). Name z. B. „Matchball Spiel".
3. Dann einen **Client** mit dieser Policy anlegen → du erhältst **Client ID** und **Client Secret**.
   (Das „Secret" eines Spiel-Clients steckt später im gepackten Spiel – das ist bei EOS so vorgesehen
   und kein Problem, solange die Policy nur Spieler-Rechte hat.)

## 4. Dienste prüfen
* **Lobbies** und **P2P (NAT-Durchdringung, Relay)** sind für GameClient-Policies freigeschaltet –
  falls das Portal einen Schalter dafür zeigt, beide aktivieren.
* **Epic Account Services** werden **nicht** gebraucht (keine Anmeldung mit Epic-Konto).
* Identity Provider: für *Device ID* ist nichts einzurichten.

## 5. Schlüssel für verschlüsselte Daten
Das Unreal-Plugin verlangt einen **Encryption Key** aus 64 Hex-Zeichen. Den erzeugst du selbst, z. B. in
PowerShell:
```powershell
-join ((1..32) | ForEach-Object { '{0:x2}' -f (Get-Random -Maximum 256) })
```

## 6. Werte eintragen (nur lokal, nie committen)
1. `Unreal/Config/Windows/WindowsEngine.ini.vorlage` kopieren nach `Unreal/Config/Windows/WindowsEngine.ini`.
2. Die Platzhalter `<CLIENT_ID>`, `<CLIENT_SECRET>`, `<PRODUCT_ID>`, `<SANDBOX_ID>`, `<DEPLOYMENT_ID>` und
   `<64_HEX_ZEICHEN>` ersetzen.
3. Die Datei steht in `.gitignore` – sie bleibt auf deinem PC. Auf dem zweiten PC dasselbe noch einmal.

## 7. Was ich dann in M9 umsetze
* Plugin *Online Subsystem EOS* aktivieren, Anmeldung per Geräte-Id (EOS Connect).
* Lobby mit vierstelligem Code als Suchattribut (README 15.1), P2P-Verbindung mit Relay-Rückfall,
  Ping-Anzeige, Versionsvergleich beim Beitreten (README 15.3).
* **[PRÜFEN]** Ob die Geräte-Id-Anmeldung direkt über das Unreal-Subsystem geht oder über die
  EOS-Schnittstelle selbst (EOS_Connect_CreateDeviceId + Anmeldung mit DeviceID-Token), kläre ich in M9
  im ersten Test – das ändert an deinen Schritten oben nichts.
