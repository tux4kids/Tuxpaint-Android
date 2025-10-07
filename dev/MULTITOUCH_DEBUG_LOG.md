# Multitouch Drawing Bug - Debug Log

## Problem
Finger 0 zeichnet eine durchgezogene Linie, aber Finger 1+ zeichnen nur gepunktete Linien (dots statt continuous strokes).

## Root Cause Analysis

### Kernproblem
Der SDL Mainloop läuft zu langsam (~1000ms pro Iteration statt 10ms), weil `SDL_PollEvent()` blockiert wenn keine Events vorhanden sind.

**Beweis aus Logs:**
- Touch Events: alle ~17ms (60 Hz) ✅
- Mainloop: nur alle ~1000ms ❌
- Resultat: `android_multitouch_paint_now()` wird quasi nie aufgerufen

```
19:46:17.853  Touch: action=0
19:46:17.914  Touch: action=2
19:46:17.931  Touch: action=2
... (40+ Touch Events in 1.5 Sekunden)
19:46:19.660  Mainloop end: button_down=0  <- NUR 1x!
19:46:20.763  Mainloop end: button_down=0  <- 1100ms später!
```

---

## Versuchte Lösungen (alle fehlgeschlagen)

### Versuch 1: Callback von JNI Thread (19:02)
**Ansatz:** `android_multitouch_paint_now()` direkt aus `nativeOnTouch()` aufrufen

**Code:**
```c
// In nativeOnTouch():
if (g_paint_callback) {
    g_paint_callback();  // Direct call from JNI thread
}
```

**Ergebnis:** ❌ **EGL_BAD_ACCESS Error**
```
eglMakeCurrentImpl:1100 error 3002 (EGL_BAD_ACCESS)
```

**Grund:** OpenGL/EGL kann nur von einem Thread verwendet werden. JNI läuft im Java UI Thread, SDL/OpenGL im Render Thread.

**Status:** Sofort verworfen wegen Threading-Konflikt

---

### Versuch 2: Aufruf am Ende des Mainloops (19:16)
**Ansatz:** `android_multitouch_paint_now()` am Ende jeder Mainloop-Iteration aufrufen

**Code:**
```c
// Am Ende des Mainloops:
if (button_down && cur_tool == TOOL_BRUSH) {
    android_multitouch_paint_now();
}
SDL_Delay(10);
```

**Ergebnis:** ❌ **Beide Striche unterbrochen**

**Logs zeigen:**
- Nur 1x "REALTIME DRAW Finger 1" in 1.5 Sekunden
- Mainloop läuft nur alle ~1000ms
- Finger 0 funktionierte auch nicht mehr!

**Grund:** Mainloop läuft zu selten, weil `SDL_PollEvent()` blockiert. `SDL_Delay(10)` wird nie erreicht wenn keine Events kommen.

**Status:** Verschlechtert - auch Finger 0 jetzt kaputt

---

### Versuch 3: Aufruf im MOUSEMOTION Handler (19:25)
**Ansatz:** `android_multitouch_paint_now()` bei jedem MOUSEMOTION Event aufrufen

**Code:**
```c
if (cur_tool == TOOL_BRUSH) {
    brush_draw(old_x, old_y, new_x, new_y, 1);  // Finger 0
    #ifdef ENABLE_MULTITOUCH
    android_multitouch_paint_now();  // Finger 1+
    #endif
}
```

**Ergebnis:** ❌ **Finger 1 immer noch gepunktet**

**Logs zeigen:**
- Touch Events: alle ~17ms
- MOUSEMOTION Events: nur 2x in 700ms!
- REALTIME DRAW: nur 2x in 700ms

**Grund:** MOUSEMOTION Events sind zu selten. SDL generiert nicht für jeden Android Touch ein MOUSEMOTION Event.

**Status:** Finger 0 wieder OK, aber Finger 1+ immer noch kaputt

---

### Versuch 4: SDL User Event Push (19:47) - AKTUELL
**Ansatz:** SDL User Event pushen bei jedem Touch, um Mainloop aufzuwecken

**Code:**
```c
// In nativeOnTouch():
if (pointerCount > 1) {
    SDL_Event wake_event;
    wake_event.type = SDL_USEREVENT;
    wake_event.user.code = 1;
    SDL_PushEvent(&wake_event);
}
```

**Ergebnis:** ❌ **Immer noch keine Verbesserung**

**Status:** Unverändert - immer noch gepunktete Linie für Finger 1+

---

## Aktuelle Code-Struktur

### Finger 0 (funktioniert)
```c
// In MOUSEMOTION Handler:
brush_draw(old_x, old_y, new_x, new_y, 1);
```
- Läuft bei jedem MOUSEMOTION Event
- MOUSEMOTION kommt häufig genug für Finger 0
- Durchgezogene Linie ✅

### Finger 1+ (funktioniert NICHT)
```c
// Am Ende des Mainloops:
if (button_down && cur_tool == TOOL_BRUSH) {
    android_multitouch_paint_now();  // Zeichnet Finger 1+
}
```
- Sollte alle 10ms laufen
- Läuft aber nur alle ~1000ms
- Gepunktete Linie ❌

---

## Offene Fragen

1. **Warum blockiert SDL_PollEvent() so lange?**
   - Sollte bei Touch Events sofort Events liefern
   - Tut es aber nicht

2. **Warum kommen MOUSEMOTION Events so selten?**
   - Touch Events: 60 Hz
   - MOUSEMOTION: ~2 Hz
   - SDL konvertiert nicht alle Touches zu MOUSEMOTION?

3. **Warum weckt SDL_PushEvent() den Mainloop nicht auf?**
   - SDL_USEREVENT sollte SDL_PollEvent() aufwecken
   - Scheint nicht zu funktionieren

---

## Nächste Schritte (TODO)

### Option A: SDL Event Loop Debug
- Log hinzufügen: Wie viele Events gibt SDL_PollEvent() zurück?
- Prüfen: Wird SDL_USEREVENT überhaupt verarbeitet?

### Option B: Threading-Architektur ändern
- Separater Thread für Multitouch-Zeichnung?
- Mutex-geschützter Zugriff auf Canvas?
- VORSICHT: OpenGL/EGL Threading!

### Option C: Anderer Event-Typ
- Statt SDL_USEREVENT einen anderen Event-Typ pushen?
- SDL_FINGERMOTION?
- Eigener Custom Event?

### Option D: Mainloop umbauen
- `SDL_PollEvent()` durch `SDL_WaitEventTimeout(1)` ersetzen?
- Timeout von 1ms statt blockierendes Wait?

---

## Logs-Analyse

### Touch Events (60 Hz) - FUNKTIONIERT
```
19:46:17.853  Touch: action=0 pointers=1
19:46:17.914  Touch: action=2 pointers=2  <- +61ms
19:46:17.931  Touch: action=2 pointers=2  <- +17ms
19:46:17.948  Touch: action=2 pointers=2  <- +17ms
```

### Mainloop (0.9 Hz) - KAPUTT
```
19:46:19.660  Mainloop end: fingers=1
19:46:20.763  Mainloop end: fingers=1  <- +1103ms !!!
```

### REALTIME DRAW (selten) - KAPUTT
```
19:46:17.957  REALTIME DRAW Finger 1: (426,630)->(446,635)
... (1.5 Sekunden Pause - keine weiteren Draws!)
```

---

## Fazit

**Hauptproblem:** SDL Mainloop läuft zu langsam
**Grund:** `SDL_PollEvent()` blockiert zu lange
**Konsequenz:** `android_multitouch_paint_now()` wird quasi nie aufgerufen

**Status:** Alle bisherigen Lösungsversuche fehlgeschlagen
