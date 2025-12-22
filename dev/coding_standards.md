# Coding Standards & Debug Notes

## Android Emulator Touch Input Coordinates

The Android emulator has a coordinate transformation issue where touch input coordinates are mapped differently than expected:

- Physical screen size: 1344x2992 pixels
- SDL receives coordinates scaled by approximately 0.5x
- Example: Touch at screen position (310, 100) → SDL receives (159, 100)

### Child Mode Button Position
- SDL coordinates: x=107-214, y=0-107
- Screen tap position: x=310, y=100 (approximately center of button)

### Sound Button Position  
- SDL coordinates: x=0-107, y=0-107
- Screen tap position: x=50-100, y=100

**Note:** Always use scaled coordinates when testing button clicks via `adb shell input tap`.
