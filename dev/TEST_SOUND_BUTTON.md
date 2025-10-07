# Sound Button Test - Anleitung für echtes Handy

## Build und Installation

### 1. APK bauen
```bash
./gradlew :app:assemblePlayStoreDebug
```

### 2. Auf Handy installieren
```bash
# Nur ein Gerät anschließen, dann:
adb install -r app/build/outputs/apk/playStore/debug/app-playStore-debug.apk
```

Oder manuell:
- APK kopieren nach: `app/build/outputs/apk/playStore/debug/app-playStore-debug.apk`
- Auf Handy übertragen und installieren

## Test durchführen

### 3. Logcat vorbereiten
```bash
# Logs löschen
adb logcat -c

# Logcat in separatem Terminal starten
adb logcat -s TuxPaint:I TuxPaint:D
```

### 4. App starten
- Tuxpaint auf dem Handy öffnen
- Warten bis App vollständig geladen ist

### 5. Sound Button Position prüfen
**Im Logcat suchen nach:**
```
TuxPaint: r_sound_btn: x=0 y=??? w=??? h=???
```

Diese Werte zeigen die exakte Position des Sound Buttons.

### 6. Auf Sound Button klicken

**Der Sound Button ist:**
- Ganz unten links im Tool-Bereich
- Unter dem "Quit" Button
- Icon zeigt Lautsprecher/Sound-Symbol

**Beim Klick sollte im Logcat erscheinen:**
```
Click at (40,889) - HIT(r_sound_btn)=1 rect(0,???,???,???)
SOUND_BTN: Clicked at (40,889)
SOUND_BTN: mute BEFORE = 0
SOUND_BTN: mute AFTER = 1
```

### 7. Testen ob Sound wirklich stumm ist

**Nach dem ersten Klick (mute=1):**
1. Male mit dem Pinsel
2. Es sollte **KEIN Sound** zu hören sein
3. Im Logcat sollte stehen (nur bei DEBUG build):
   ```
   Sound 8 blocked (muted)
   ```

**Nach dem zweiten Klick (mute=0):**
1. Male wieder mit dem Pinsel
2. Es sollte **Sound zu hören** sein
3. Im Logcat:
   ```
   SOUND_BTN: mute BEFORE = 1
   SOUND_BTN: mute AFTER = 0
   ```

## Was zu prüfen ist

### ✓ Erfolgskriterien:
1. **Button wird erkannt:** `HIT(r_sound_btn)=1` beim Klick
2. **Mute toggled:** BEFORE=0→AFTER=1, dann BEFORE=1→AFTER=0
3. **Sound stoppt:** Kein Pinsel-Sound wenn muted
4. **Sound spielt:** Pinsel-Sound wenn unmuted
5. **Keine Canvas-Clicks:** Beim Klick auf Button sollte NICHT "Canvas click detected!" erscheinen

### ✗ Fehler-Anzeichen:
1. `HIT(r_sound_btn)=0` → Button-Rect ist falsch positioniert
2. `Canvas click detected!` beim Button-Klick → Event-Reihenfolge falsch
3. Sound spielt trotz mute=1 → Mix_PlayChannel() wird nicht geprüft
4. Keine "SOUND_BTN:" Logs → Button-Handler wird nicht erreicht

## Bekannte Probleme aus vorherigen Tests

### Problem 1: Button zu weit oben
**User Log:** Click at (40, 889)
**Button Rect:** y=760-855

→ **Fix:** Event-Handler-Reihenfolge korrigiert (Sound Button VOR Canvas)

### Problem 2: Sound spielt trotz Button-Klick
**Ursache:** Direct `Mix_PlayChannel()` calls bypassed mute check

→ **Fix:** Alle Calls prüfen jetzt `if (!mute)`

## Debug-Informationen sammeln

Falls der Test fehlschlägt, bitte folgende Logs sammeln:

```bash
# Komplette Logs vom App-Start
adb logcat -d -s TuxPaint:I TuxPaint:D > tuxpaint_test.log

# Spezifische Suche
grep "r_sound_btn" tuxpaint_test.log
grep "Click at" tuxpaint_test.log
grep "SOUND_BTN" tuxpaint_test.log
grep "mute" tuxpaint_test.log
```

## Erwartete Log-Sequenz

```
# Bei App-Start:
TuxPaint: === TUXPAINT MAIN STARTED ===
TuxPaint: r_sound_btn: x=0 y=856 w=107 h=107

# Erster Button-Klick:
TuxPaint: Click at (53,910) - HIT(r_sound_btn)=1 rect(0,856,107,107)
TuxPaint: SOUND_BTN: Clicked at (53,910)
TuxPaint: SOUND_BTN: mute BEFORE = 0
TuxPaint: SOUND_BTN: mute AFTER = 1

# Pinsel-Klick wenn muted:
TuxPaint: Canvas click detected! cur_tool=0
TuxPaint: BRUSH DOWN: new stroke at (400,500)
# KEIN Sound sollte spielen

# Zweiter Button-Klick:
TuxPaint: SOUND_BTN: mute BEFORE = 1
TuxPaint: SOUND_BTN: mute AFTER = 0

# Pinsel-Klick wenn unmuted:
TuxPaint: Canvas click detected! cur_tool=0
TuxPaint: BRUSH DOWN: new stroke at (400,500)
# Sound sollte jetzt spielen
```
