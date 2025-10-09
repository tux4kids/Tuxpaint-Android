# Brush Thumbnails Implementation

**Datum:** 9.10.2025, 06:25

## Problem

Brushes mit `frames` und/oder `directional` Properties zeigten im Child Mode Slider nur ein Quadrat statt des korrekten Icons. Das liegt daran, dass diese Brushes Multi-Frame-Texturen oder 3x3-Directional-Grids verwenden.

## Lösung

### 1. Analyse der Brush-Properties

23 Brushes benötigen spezielle Thumbnail-Extraktion:

#### Brushes mit FRAMES (Multi-Frame-Texturen)
- 9=acrylic (frames=4)
- 25=fluff_gradient (frames=4)
- 26=fluff (frames=4)
- 30=graphite (frames=7)
- 33=impasto (frames=10)
- 34=inksplat (frames=5)
- 38=paint_splats (frames=4)
- 39=pencil (frames=4)
- 48=smoke (frames=3)
- 49=sparkles (frames=4)
- 52=spiral (frames=4)
- 54=spray (frames=6)
- 65=vine (frames=6)
- 66=watercolor-texture (frames=4)
- 67=water_still (frames=3)

#### Brushes mit DIRECTIONAL (3x3-Grid)
- 11=arrow_triangles (directional)
- 35=lines-angled (directional)

#### Brushes mit BOTH (Frames + Directional)
- 14=critter_dog (frames=2, directional)
- 15=critter_kuroneko (frames=2, directional)
- 16=critter_squirrel (frames=3, directional)
- 27=footprints-human (frames=2, directional)
- 28=footprints-human-shoes (frames=2, directional)
- 29=footprints-paws (frames=2, directional)

### 2. Thumbnail-Extraktion

Script: `/tmp/create_brush_thumbnails.py`

#### Extraktions-Logik:

**Frames-Only:**
- Teile Bildbreite durch Anzahl Frames
- Schneide linken Frame (ersten Frame) aus
- Speichere als `{brush_name}_thumb.png`

**Directional-Only:**
- Teile Bild in 3x3 Grid
- Schneide mittlere Zelle (Center) aus
- Speichere als `{brush_name}_thumb.png`

**Both (Frames + Directional):**
- Schritt 1: Extrahiere ersten Frame
- Schritt 2: Extrahiere Center aus dem Frame
- Speichere als `{brush_name}_thumb.png`

#### Ergebnisse:

Alle 23 Thumbnails erfolgreich erstellt in:
`/var/www/Tuxpaint-Android/app/src/main/assets/data/brushes/thumbs/`

### 3. Code-Änderungen in tuxpaint.c

#### Neue Variable (Zeile 2016)
```c
static SDL_Surface **img_brushes_special_thumbs = NULL;
```

#### Realloc-Anpassung (Zeile 9130)
```c
img_brushes_special_thumbs = realloc(img_brushes_special_thumbs, 
                                      num_brushes_max * sizeof *img_brushes_special_thumbs);
```

#### Thumbnail-Loading (Zeile 9229-9268)
```c
/* Try to load special thumbnail for brushes with frames/directional */
img_brushes_special_thumbs[num_brushes] = NULL;
if (brushes_frames[num_brushes] != 1 || brushes_directional[num_brushes]) {
  char thumb_fname[512];
  char *base_name = strdup(files[i].str);
  char *dot = strrchr(base_name, '.');
  if (dot) *dot = '\0';
  
  safe_snprintf(thumb_fname, sizeof thumb_fname, "%s/thumbs/%s_thumb.png", dir, base_name);
  
  SDL_Surface *special_thumb = loadimage(thumb_fname);
  if (special_thumb != NULL) {
    // Scale and store thumbnail
  }
  free(base_name);
}
```

#### Child Mode Slider Icon (Zeile 11772-11778)
```c
/* Use special thumbnail if available, otherwise use regular thumbnail */
SDL_Surface *icon = (img_brushes_special_thumbs[cur_brush] != NULL) ? 
                    img_brushes_special_thumbs[cur_brush] : 
                    img_brushes_thumbs[cur_brush];

if (icon == NULL) return;
```

#### Cleanup (Zeile 16681)
```c
free_surface_array(img_brushes_special_thumbs, num_brushes);
```

### 4. Tests

#### Unit-Tests
- `/tmp/test_brush_thumbnails.py` - Verifiziert Thumbnail-Erstellung
- `/tmp/test_brush_integration.py` - Verifiziert Integration

**Alle Tests bestanden:**
- ✓ 23 Thumbnails erfolgreich erstellt
- ✓ Alle Thumbnails sind valide PNG-Dateien
- ✓ Alle haben korrekte Dimensionen (28x29 bis 140x128 px)
- ✓ 58 Brushes in Icon-Kategorien haben korrekte Thumbnails

### 5. Kategorie-Zuordnung

Brushes mit speziellen Thumbnails werden in folgenden Kategorien verwendet:

- **Kategorie 3** (Mixed Icon Brushes): 30, 33, 34, 35, 38, 39, 49, 52
- **Kategorie 6** (Animals & Nature): 14, 15, 16, 27, 28, 29, 67
- **Kategorie 9** (Texture Brushes): 9, 25, 26, 48, 54, 65, 66
- **Kategorie 10** (Effect Brushes): 11

## Ergebnis

✅ Alle Brushes zeigen jetzt korrekte Icons im Child Mode Slider
✅ Keine Quadrate mehr bei frames/directional Brushes
✅ Icons werden korrekt skaliert und zentriert im Slider-Handle
✅ Automatisches Fallback auf normale Thumbnails wenn spezielle fehlen

## Dateien geändert

1. `/var/www/Tuxpaint-Android/app/src/main/jni/tuxpaint/src/tuxpaint.c`
   - Zeile 2016: Neue Variable
   - Zeile 9130: Realloc
   - Zeile 9229-9268: Thumbnail-Loading
   - Zeile 11772-11778: Icon-Display
   - Zeile 16681: Cleanup

2. `/var/www/Tuxpaint-Android/IMPLEMENTATION_PLAN.md`
   - Aktualisierung der Brush-Liste mit properties

3. Neue Assets (23 Dateien):
   - `/app/src/main/assets/data/brushes/thumbs/*_thumb.png`

## Test-Ausführung

```bash
# Thumbnails erstellen
python3 /tmp/create_brush_thumbnails.py

# Unit-Tests
python3 /tmp/test_brush_thumbnails.py

# Integration-Tests
python3 /tmp/test_brush_integration.py

# Kompilierung
./gradlew assembleDebug
```

Alle Tests erfolgreich ✓
