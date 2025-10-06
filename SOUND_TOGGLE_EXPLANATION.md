# Sound Toggle - Variable Erklärung

## Die Sound Toggle Variable: `mute`

### Deklaration und Scope
Die Variable `mute` ist eine **globale Integer-Variable** die in zwei Dateien definiert ist:

**In `playsound.c` (Zeile 33):**
```c
int mute;
```

**In `playsound.h` (Zeile 39) als extern:**
```c
extern int mute, use_sound, use_stereo;
```

### Werte und Bedeutung
- **`mute = 0`**: Sound ist **AN** (unmuted)
- **`mute = 1`**: Sound ist **AUS** (muted)

### Wo wird die Variable verwendet?

#### 1. In `playsound()` Funktion (playsound.c, Zeile 57)
```c
if (!mute && use_sound && s != SND_NONE)
{
    Mix_PlayChannel(chan, sounds[s], 0);
    // ... Sound wird abgespielt
}
```
- Prüft ob `mute == 0` (NOT mute)
- Nur wenn `mute = 0` wird Sound abgespielt

#### 2. In Sound Button Handler (tuxpaint.c, Zeile 5410-5441)
```c
else if (HIT(r_sound_btn) && valid_click(event.button.button))
{
    __android_log_print(ANDROID_LOG_INFO, "TuxPaint", "SOUND_BTN: mute BEFORE = %d", mute);
    
    mute = !mute;  // Toggle: 0→1 oder 1→0
    
    __android_log_print(ANDROID_LOG_INFO, "TuxPaint", "SOUND_BTN: mute AFTER = %d", mute);
    
    Mix_HaltChannel(-1);  // Stoppe alle laufenden Sounds
    // ... Update UI
}
```

#### 3. In direkten Mix_PlayChannel() Aufrufen
Mehrere Stellen im Code rufen `Mix_PlayChannel()` direkt auf (z.B. Stamp-Sounds):
```c
if (!mute)
    Mix_PlayChannel(2, stamp_data[stamp_group][cur_thing]->ssnd, 0);
```

### Problem History

**Ursprüngliches Problem:** 
Der Sound Button wurde geklickt, aber Sound spielte weiter.

**Root Cause:** 
1. Canvas-Handler wurde VOR Sound-Button-Handler geprüft
2. Klicks landeten im Canvas statt im Button
3. Direkte `Mix_PlayChannel()` Aufrufe ignorierten `mute`

**Fix:**
1. Sound-Button-HandlerVOR Canvas-Handler verschoben (Zeile 5410)
2. Alle direkten `Mix_PlayChannel()` Aufrufe prüfen jetzt `mute`
3. Logging hinzugefügt um Toggle zu verifizieren

### Log Output Beispiel

Beim Klick auf Sound Button:
```
SOUND_BTN: Clicked at (40,921)
SOUND_BTN: mute BEFORE = 0
SOUND_BTN: mute AFTER = 1
```

Beim Versuch Sound abzuspielen (wenn muted):
```
Sound 8 blocked (muted)
```

### Testing

Der Unit Test `SoundToggleTest.java` verifiziert:
1. Button wird erkannt bei Klick
2. `mute` Variable toggled korrekt
3. Sounds werden geblockt wenn `mute = 1`
4. Sounds spielen wenn `mute = 0`

### Button Position
- **Rect:** `r_sound_btn` bei (0, y=tool_row_8, w=button_w, h=button_h)
- **Typische Position:** x=0-95, y=760-855 (abhängig von Bildschirmgröße)
- **Click Target:** Center des Buttons (~47, ~808)
