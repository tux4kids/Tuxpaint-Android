# TODO

# later TODO (not for now)
- multitouch support when erasing
- ~~**Text tool (ABC) keyboard issue**~~ - **ROOT CAUSE FIXED!** ✅
  - TEXT/LABEL were disabled in Child Mode
  - Enabled in `apply_child_mode_tool_filter()` and `child_tools` arrays
  - Also implemented keyboard debounce (500ms grace period)
  - See: `dev/TEXT_TOOL_KEYBOARD_FIX.md`
  - **Needs testing on Android device** - should now work!
- check why stamp and magic is not working (disabled all the time)

- die haupt datei in mehrere sinnvoll splitten

- Add proper icon assets to APK for visual polish
- die app umbenennen in "TuxPaint Kids"
- fastlane beschriebungen anpassen
- Die Brush Kategorien auch im Export Mode umstritten. Global definieren
- Beim allerersten Mal die App im Kids Mode in Kategorie 3 starten und gesperrt (locked).
- Meldung beim Start, die das 3s Longpress erklärt, falls der in den preferences aktiv ist


