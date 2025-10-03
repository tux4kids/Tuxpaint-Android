#TODO

- enable multitouch support when painting with single brush (multiple fingers can paint simultaneously)
  - See `MULTITOUCH_IMPLEMENTATION.md` for detailed implementation plan
  - Requires handling SDL_FINGERDOWN/UP/MOTION events
  - Track up to 10 simultaneous finger positions
  - Currently touch events are converted to mouse events (only single touch works)
  - Test created: `app/src/androidTest/java/org/tuxpaint/tests/MultitouchTest.java`

- remoove the label "Tools" on the left, instead add two more buttons:
    - 1. button to disable sound
    - 2. button to enable child mode:
        - hide all buttons on the left
        - hide the area below the color buttons, insead grow oll color buttons so they fill all the spaca all the the way down to the bottom of the screen
        - replace the right button row with only one big slider up and down for the brush-size
        - replace the left buttons with only some big buttons:
            - paint
            - fill
            - save
            - eraser
            - new
                - will autosave the current picture
            - undo
            - redo
            - exit child mode

- remove the label button "Colors" on the bottom left
