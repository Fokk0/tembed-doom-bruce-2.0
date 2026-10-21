# Bruce + Doom für den LilyGO T-Embed CC1101 (Plus) – per Webflash

Ein Flash-Vorgang im Browser installiert **alles**: Bootloader, Partitionstabelle, Bruce-Firmware,
Doom und das Doom-Shareware-WAD (Episode 1). Keine SD-Karte nötig.

## So funktioniert es
- Flash (16 MB): `ota_0` = Bruce (4,5 MB), `ota_1` = Doom (2 MB), `wad` = doom1.wad (roh, per mmap gelesen),
  `spiffs` = interner Bruce-Speicher (LittleFS, ~5 MB, ersetzt die SD-Karte).
- In Bruce: **Others → Doom** startet Doom (Neustart in `ota_1`).
- In Doom: **Menü → Quit Game** (oder Fehler) bootet zurück zu Bruce.
- Alle Bruce-Funktionen des T-Embed-CC1101-Builds bleiben unverändert erhalten (Sub-GHz, IR, WiFi, BLE, RFID/NFC,
  BadUSB, JS-Interpreter, …). Funktionen, die zwingend eine SD brauchen, nutzen den internen LittleFS-Speicher.

## Installation (einmalig, ohne lokale Tools)
1. Kostenloses GitHub-Konto anlegen, neues **öffentliches** Repository erstellen.
2. Den kompletten Inhalt dieses Ordners hochladen (Add file → Upload files; `.github` mit hochladen).
3. Repo → **Settings → Pages → Source: GitHub Actions**.
4. Repo → **Actions → build-and-publish-webflasher → Run workflow**. Dauer ca. 10–20 Minuten.
5. Nach Erfolg steht die Seite unter `https://<dein-name>.github.io/<repo>/`.
6. Seite in Chrome/Edge öffnen, T-Embed per USB anschließen, **Installieren** klicken.

## Steuerung (Drehregler)
**Im Spiel**
- Drehen = Umsehen links/rechts
- Klick = Schießen · Doppelklick = Benutzen (Türen/Schalter) · Dreifachklick = Menü
- Langer Druck (0,7 s) = Dauer-Vorwärtslauf an/aus
- Drücken + Drehen = Waffe wählen (1–7)

**Menüs/Titelbild**
- Drehen = hoch/runter · Klick = Enter · Doppelklick = „Y“ (Bestätigen) · Langer Druck = Esc/Zurück

## Wenn der erste CI-Lauf rot ist
Der Code konnte vor der Auslieferung nicht kompiliert werden. Der Build-Log zeigt die Stelle genau an:
- **`Anker 'Main Menu' … nicht gefunden`**: In `src/core/menu_items/OthersMenu.cpp` von Bruce eine Zeile
  `void launchDoom();` unter die Includes und `options.push_back({"Doom", launchDoom});` vor den „Main Menu“-Eintrag setzen
  (Patch dafür in `tools/patch_bruce.py` anpassen).
- **Unbekannte Struktur-Felder in `w_file.h`** oder **`DG_sound_module` undefined**: doomgeneric-Stand abweichend;
  Log in einen Issue/Chat kopieren, die Korrektur sind meist 1–3 Zeilen in `doom_app/`.
- **Bruce/Doom zu groß**: Größen in `partitions.csv` und den Prüfungen im Workflow anpassen (und Offsets in `web/manifest.json`).
- **Falsche Drehrichtung/Pins/Display gedreht**: `-DDOOM_ENC_DIR=-1`, `-DDOOM_ENC_A/B/KEY=…`, `-DDOOM_ROTATION=1`
  in `tools/doom_env.ini` unter `build_flags` ergänzen.
- Gibt es für den Plus einen eigenen Bruce-Build, `BRUCE_ENV` im Workflow ändern.

## Grenzen (v1)
Kein Ton, keine Savegames, nur Vorwärts-Lauf (kein Rückwärts/Strafen), nur Shareware-Episode 1.
Wer ein eigenes `doom1.wad` (≤ 4,25 MB) nutzen will, legt es nach `wad/doom1.wad`.

## Lizenzen / Hinweise
Bruce: AGPL-3.0 · doomgeneric/Doom-Quellcode: GPL-2.0 · doom1.wad (Shareware): frei weitergebbar, unverändert.
Bruce ist ein Security-Werkzeug – nur an eigenen Geräten bzw. mit Erlaubnis einsetzen; Sub-GHz-Senderegeln beachten.
