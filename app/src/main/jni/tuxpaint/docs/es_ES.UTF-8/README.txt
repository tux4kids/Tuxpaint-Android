                                   Tux Paint
                                versión 0.9.31

                     A simple drawing program for children

        Copyright © 2002-2023 by various contributors; see AUTHORS.txt.
                             https://tuxpaint.org/

                                julio 20, 2023

+-------------------------------------------------+
|                Table of Contents                |
|-------------------------------------------------|
|  I. About Tux Paint                             |
| II. Using Tux Paint                             |
|      A. Launching Tux Paint                     |
|      B. Title Screen                            |
|      C. Main Screen                             |
|      D. Available Tools                         |
|          1. Drawing Tools                       |
|              a. "Paint" Tool (Brush)            |
|              b. "Stamp" Tool (Rubber Stamps)    |
|              c. "Lines" Tool                    |
|              d. "Shapes" Tool                   |
|              e. "Text" and "Label" Tools        |
|              f. "Fill" Tool                     |
|              g. "Magic" Tool (Special Effects)  |
|              h. "Eraser" Tool                   |
|          2. Other Controls                      |
|              a. "Undo" and "Redo" Commands      |
|              b. "New" Command                   |
|              c. "Open" Command                  |
|              d. "Save" Command                  |
|              e. "Print" Command                 |
|              f. "Slides" Command (under "Open") |
|              g. "Quit" Command                  |
|              h. Sound Muting                    |
|      E. Controlling Tux Paint                   |
| III. Loading Other Pictures into Tux Paint      |
| IV. Further Reading                             |
|  V. How to Get Help                             |
| VI. How to Participate                          |
+-------------------------------------------------+

I. About Tux Paint

A. What Is "Tux Paint"?

Tux Paint is a free drawing program designed for young children (kids ages 3
and up). It has a simple, easy-to-use interface, fun sound effects, and an
encouraging cartoon mascot who helps guide children as they use the program. It
provides a blank canvas and a variety of drawing tools to help your child be
creative.

B. Objectives

Easy and Fun
    Tux Paint is meant to be a simple drawing program for young children. It is
    not meant as a general-purpose drawing tool. It is meant to be fun and easy
    to use. Sound effects and a cartoon character help let the user know what's
    going on, and keeps them entertained. There are also extra-large
    cartoon-style mouse pointer shapes.

Extensibility
    Tux Paint is extensible. Brushes and 'rubber stamp' shapes can be dropped
    in and pulled out. For example, a teacher can drop in a collection of
    animal shapes and ask their students to draw an ecosystem. Each shape can
    have a sound which is played, and textual facts which are displayed, when
    the child selects the shape.

Portability
    Tux Paint is portable among various computer platforms: Windows, Macintosh,
    Linux, etc. The interface looks the same among them all. Tux Paint runs
    suitably well on older systems, and can be built to run better on slow
    systems.

Simplicity
    There is no direct access to the computer's underlying intricacies. The
    current image is kept when the program quits, and reappears when it is
    restarted. Saving images requires no need to create filenames or use the
    keyboard. Opening an image is done by selecting it from a collection of
    thumbnails. Access to other files on the computer is restricted.

Accessibility
    Tux Paint offers a number of accessibility options, including increasing
    the size of control buttons, changing the UI font, options to control the
    cursor (mouse pointer) using the keyboard or other input devices (joystick,
    gamepad, etc.), an on-screen keyboard, and "stick" mouse clicks.


C. License

Tux Paint is an Open Source project, Free Software released under the GNU
General Public License (GPL). It is free, and the 'source code' behind the
program is available. (This allows others to add features, fix bugs, and use
parts of the program in their own GPL'd software.)

See COPYING.txt for the full text of the GPL license.

D. What's New in Tux Paint version 0.9.31?

Fuzzy Erasers
    Fuzzy-edged round erasers have been added to the Eraser tool

New Magic tools
    Loops, Ribbon, Smooth, and Squiggles

Create templates from Open dialog
    Saved drawings can be turned into templates (used to make new drawings) via
    a new option in the Open dialog. (This feature may be deactivated using the
    "notemplateexport" simplification option.)

User interface font may be overridden
    The font used for the user interface (button labels, dialog pop-ups, and
    instructions) can be overridden from its default ("DejaVu Sans"). (See the
    "uifont" configuration option. Use "--listfonts" to see a list of available
    fonts.)

File Erase may be disabled

    Controls to erase saved work — saved drawings (via the Open dialog) and
    exported templates (via the New dialog) — can be disabled. (See the
    "noerase" configuration option.)

"Controlling Tux Paint" documentation

    A brief overview of how Tux Paint can be controlled — mouse, tablet,
    touchscreen, joystick, and keyboard — has been added to the README
    documentation.


See CHANGES.txt for the complete list of changes.

II. Using Tux Paint

A. Launching Tux Paint

1. Linux/Unix Users

Tux Paint should have placed a launcher icon in your KDE and/or GNOME menus,
under 'Graphics.'

Alternatively, you can run the following command at a shell prompt (e.g., "$"):

    $ tuxpaint

If any errors occur, they will be displayed on the terminal (to STDERR).

2. Windows Users

[Tux Paint Icon]
   Tux Paint

If you installed Tux Paint on your computer using the 'Tux Paint Installer,' it
will have asked you whether you wanted a 'Start' menu short-cut, and/or a
desktop shortcut. If you agreed, you can simply run Tux Paint from the 'Tux
Paint' section of your 'Start' menu (e.g., under 'All Programs'), or by
double-clicking the 'Tux Paint' icon on your desktop, if you had the installer
place one there.

If you're using the 'portable' (ZIP-file) version of Tux Paint, or if you used
the 'Tux Paint Installer,' but chose not to have shortcuts installed, you'll
need to double-click the "tuxpaint.exe" icon in the "Tux Paint" folder on your
computer.

By default, the 'Tux Paint Installer' will put Tux Paint's folder in "C:\
Program Files\", though you may have changed this when you ran the installer.

If you used the 'ZIP-file' download, Tux Paint's folder will be wherever you
extracted the contents of the ZIP file.

3. macOS Users

Simply double-click the "Tux Paint" icon.

[Title screen]

B. Title Screen

When Tux Paint first loads, a title/credits screen will appear.

Once loading is complete, press a key or click or tap in the Tux Paint window
to continue. (Or, after about 5 seconds, the title screen will go away
automatically.)

C. Main Screen

The main screen is divided into the following sections:

[Tools: Paint, Stamp, Lines, Shapes, Text, Magic, Label, Undo, Redo, Eraser,
    New, Open, Save, Print, Quit]
Left Side: Toolbar

    The toolbar contains the drawing and editing controls.



    [Canvas]
Middle: Drawing Canvas

    The largest part of the screen, in the center, is the drawing canvas. This
    is, obviously, where you draw!

    💡 Note: The size of the drawing canvas depends on the size of Tux Paint.
    You can change the size of Tux Paint using the Tux Paint Config.
    configuration tool, or by other means. See the Options documentation for
    more details.



    [Selectors - Brushes, Letters, Shapes, Stamps]
Right Side: Selector

    Depending on the current tool, the selector shows different things. e.g.,
    when the Paint Brush or Line tool is selected, it shows the various brushes
    available. When the Rubber Stamp tool is selected, it shows the different
    shapes you can use. When the Text or Label tool is selected, it shows
    various fonts.



    [Colors - Black, White, Red, Pink, Orange, Yellow, Green, Cyan, Blue,
    Purple, Brown, Grey]
Lower: Colors

    When the active tool supports colors, a palette of colors choices will be
    shown near the bottom of the screen. Click one to choose a color, and it
    will be used by the active tool. (For example, the "Paint" tool will use it
    as the color to draw with the chosen brush, and the "Fill" tool will use it
    as the color to use when flood-filling an area of the picture.)

    On the far right are three special color options:
      + Color Picker
        The "color picker" (which has an outline of an eye-dropper) allows you
        to pick a color found within your drawing.
        (A shortcut key is available to access this feature quickly; see
        below.)
      + Rainbow Palette
        The rainbow palette allows you to pick any color by choosing the hue,
        saturation, and value of the color you want. A box on the left displays
        hundreds of hues — from red at the top through to violet at the bottom
        — at hundreds of saturation/intensity levels — from pale & washed-out
        on the left through to pure on the right. A grey vertical bar provides
        access to hundreds of value levels — from lighest at the top through to
        darkest at the bottom.
        Click the green checkbox button to select the color, or the "Back"
        button to dismiss the pop-up without picking a new color.
        You may also set this tool's color to that of other color choices:
          o Whichever built-in color is selected, if any
          o The Color Picker's current color
          o The Color Mixer's current color
      + Color Mixer
        The "color mixer" (which has silhouette of a paint palette) allows you
        to create colors by blending primary additive colors — red, yellow, and
        blue — along with white (to "tint"), grey (to "tone"), and black (to
        "shade").
        You may click any button multiple times (for example, red + red +
        yellow results in a red-orange color). The ratios of colors added are
        shown at the bottom.
        You can start over (reset to no colors in your picture) by clicking the
        "Clear" button. You can also undo or redo multiple steps of mixing, in
        case you made a mistake (without having to start over).
        Click the green checkbox button to select the color, or the "Back"
        button to dismiss the pop-up without picking a new color.

    ⌨ When the active tool supports colors, a shortcut may be used for quick
    access to the "color picker" option. Hold the [Control] key while clicking,
    and the color under the mouse cursor will be shown at the bottom. You may
    drag around to canvas to find the color you want. When you release the
    mouse button, the color under the cursor will be selected. If you release
    the mouse outside of the canvas (e.g., over the "Tools" area), the color
    selection will be left unchanged. (This is similar to clicking the "Back"
    button that's available when bringing up the "color picker" option via its
    button the color palette.)

    ⚙ Note: You can define your own colors for Tux Paint. See the "Options"
    documentation.



    (Example tip: 'Pick a shape. Click to pick the center, drag, then let go
    when it is the size you want. Move around to rotate it, and click to draw
    it.')
Bottom: Help Area

    At the very bottom of the screen, Tux, the Linux Penguin, provides tips and
    other information while you use Tux Paint.




D. Available Tools

1. Drawing Tools

a. "Paint" Tool (Brush)

    The Paint Brush tool lets you draw freehand, using various brushes (chosen
    in the Selector on the right) and colors (chosen in the Color palette
    towards the bottom).

    If you hold the mouse button down, and move the mouse, it will draw as you
    move.

    Some brushes are animated — they change their shape as you draw them. A
    good example of this is the vines brush that ships with Tux Paint. These
    brushes will have a small "filmstrip" icon drawn on their Selector buttons.

    Other brushes are directional — they will draw a different shape depending
    on what direction you are painting with them. An example of this is the
    arrow brush that ships with Tux Paint. These brushes have a small 8-way
    arrow icon drawn on their Selector buttons.

    Finally, some brushes can be both direction and animated. Examples of this
    are the cat and squirrel brushes that ship with Tux Paint. These brushes
    will have both the "filmstrip" and 8-way arrow icons.

    As you draw, a sound is played. The bigger the brush, the lower the pitch.

    Brush Spacing


        The space between each position where a brush is applied to the canvas
        can vary. Some brushes (such as the footprints and flower) are spaced,
        by default, far enough apart that they don't overlap. Other brushes
        (such as the basic circular ones) are spaced closely, so they make a
        continuous stroke.

        The default spacing of brushes may be overridden using by clicking
        within the triangular-shaped series of bars at the bottom right; the
        larger the bar, the wider the spacing. Brush spacing affects both tools
        that use the brushes: the "Paint" tool and the "Lines" tool.

        ⚙ Note: If the "nobrushspacing" option is set, Tux Paint won't display
        the brush spacing controls. See the "Options" documentation.



b."Stamp" Tool (Rubber Stamps)

    The Stamp tool is like a set of rubber stamps or stickers. It lets you
    paste pre-drawn or photographic images (like a picture of a horse, or a
    tree, or the moon) in your picture.

    As you move the mouse around the canvas, an outline follows the mouse,
    showing where the stamp will be placed, and how big it will be. Click on
    the canvas where you wish to place the stamp.

    Stamp Categories
        There can be numerous categories of stamps (e.g., animals, plants,
        outer space, vehicles, people, etc.). Use the Left and Right arrows
        near the bottom of the Selector to cycle through the collections.

    Stamp Rotation

        Using the rotation toggle button near the bottom right, you can enable
        a rotation step when placing stamps. Once you've placed the stamp,
        choose the angle to rotate it by moving the mouse around the canvas.
        Click the mouse button again and the stamp will be added to the
        drawing.

        ⚙ Note: If "stamp rotation" option is disabled, the stamp will be drawn
        on the canvas when you let go of the mouse button. (There's no rotation
        step.) See the "Options" documentation to learn about the "stamp
        rotation" ("stamprotation") option.

        📜 Note: The stamp rotation feature was added to Tux Paint in version
        0.9.29.



    Stamp Controls

        Prior to 'stamping' an image onto your drawing, various effects can
        sometimes be applied (depending on the stamp):

          o Some stamps can be colored or tinted. If the color palette below
            the canvas is activated, you can click the colors to change the
            tint or color of the stamp before placing it in the picture.
          o Stamps can be shrunk and expanded, by clicking within the
            triangular-shaped series of bars at the bottom right; the larger
            the bar, the larger the stamp will appear in your picture.
          o Many stamps may be flipped vertically, or displayed as a
            mirror-image, using the control buttons at the bottom right.

        ⚙ Note: If the "nostampcontrols" option is set, Tux Paint won't display
        the Rotation, Mirror, Flip, or sizing controls for stamps. See the "
        Options" documentation.



    Stamp Sounds

        Different stamps can have different sound effects and/or descriptive
        (spoken) sounds. Buttons in the Help Area at the lower left (near Tux,
        the Linux penguin) allow you to re-play the sound effects and
        descriptive sounds for the currently-selected stamp.






c."Lines" Tool

    This tool lets you draw straight lines using the various brushes and colors
    you normally use with the Paint Brush.

    Click the mouse and hold it to choose the starting point of the line. As
    you move the mouse around, a thin 'rubber-band' line will show where the
    line will be drawn. At the bottom, you'll see the angle of your line, in
    degrees. A line going straight to the right is 0°, a line going straight up
    is 90°, a line going straight left is 180°, a line going straight down is
    270°, and so on.

    Let go of the mouse to complete the line. A "sproing!" sound will play.

    Some brushes are animated, and will show a pattern of shapes along the
    line. Others are directional, and will show a different shape depending on
    the angle of the brush. And finally some are both animated and directional.
    See "Paint", above, to learn more.

    Different brushes have different spacing, leaving either a series of
    individual shapes, or a continuous stroke of the brush shape. Brush spacing
    may be adjusted. See the brush spacing section of the "Paint" tool, above,
    to learn more.



d."Shapes" Tool

    This tool lets you draw some simple filled, and un-filled shapes.

    Select a shape from the selector on the right (circle, square, oval, etc.).

    Use the options at the bottom right to choose the shape tool's behavior:

    Shapes from center
        The shape will expand from where you initially clicked, and will be
        centered around that position.

        📜 This was Tux Paint's only behavior through version 0.9.24.)



    Shapes from corner
        The shape will extend with one corner starting from where you initially
        clicked. This is the default method of most other traditional drawing
        software.

        📜 This option was added starting with Tux Paint version 0.9.25.




    ⚙ Note: If shape controls are disabled (e.g., with the "noshapecontrols"
    option), the controls will not be presented, and the "shapes from center"
    method will be used.

    In the canvas, click the mouse and hold it to stretch the shape out from
    where you clicked. Some shapes can change proportion (e.g., rectangle and
    oval may be wider than tall, or taller than wide), others cannot (e.g.,
    square and circle).

    For shapes that can change proportion, the aspect ratio of the shape will
    be shown at the bottom. For example: "1:1" will be shown if it is "square"
    (as tall as it is wide); "2:1" if it is either twice as wide as it is tall,
    or twice as tall as it is wide; and so on.

    Let go of the mouse when you're done stretching.

    Normal Shapes Mode

        Now you can move the mouse around the canvas to rotate the shape. The
        angle your shape is rotated will be shown at the bottom, in degrees
        (similar to the "Lines" tool, described above).

        Click the mouse button again and the shape will be drawn in the current
        color.



    Simple Shapes Mode
        If the "simple shapes" option is enabled, the shape will be drawn on
        the canvas when you let go of the mouse button. (There's no rotation
        step.)

        ⚙ See the "Options" documentation to learn about the "simple shapes"
        ("simpleshapes") option.






e."Text" and "Label" Tools

    Choose a font (from the 'Letters' available on the right) and a color (from
    the color palette near the bottom). You may also apply a bold, and/or an
    italic styling effect to the text. Click on the screen and a cursor will
    appear. Type text and it will show up on the screen. (You can change the
    font, color, and styling while entering the text, before it is applied to
    the canvas.)

    Press [Enter] or [Return] and the text will be drawn onto the picture and
    the cursor will move down one line.

    Alternatively, press [Tab] and the text will be drawn onto the picture, but
    the cursor will move to the right of the text, rather than down a line, and
    to the left. (This can be useful to create a line of text with mixed
    colors, fonts, styles and sizes.)

    Clicking elsewhere in the picture while the text entry is still active
    causes the current line of text to move to that location (where you can
    continue editing it).

    "Text" versus "Label"

        The Text tool is the original text-entry tool in Tux Paint. Text
        entered using this tool can't be modified or moved later, since it
        becomes part of the drawing. However, because the text becomes part of
        the picture, it can be drawn over or modified using Magic tool effects
        (e.g., smudged, tinted, embossed, etc.)

        When using the Label tool (which was added to Tux Paint in version
        0.9.22), the text 'floats' over the image, and the details of the label
        (the text, the position of the label, the font choice and the color)
        get stored separately. This allows the label to be repositioned or
        edited later.

        To edit a label, click the label selection button. All labels in the
        drawing will appear highlighted. Click one — or use the [Tab] key to
        cycle through all the labels, and the [Enter] or [Return] key to select
        one — and you may then edit the label. (Use they [Backspace] key to
        erase characters, and other keys to add text to the label; click in the
        canvas to reposition the label; click in the palette to change the
        color of the text in the label; etc.)

        You may "apply" a label to the canvas, painting the text into the
        picture as if it had been added using the Text tool, by clicking the
        label application button. (This feature was added in Tux Paint version
        0.9.28.) All labels in the drawing will appear highlighted, and you
        select one just as you do when selecting a label to edit. The chosen
        label will be removed, and the text will be added directly to the
        canvas.

        ⚙ The Label tool can be disabled (e.g., by selecting "Disable 'Label'
        Tool" in Tux Paint Config. or running Tux Paint with the "nolabel"
        option).



    International Character Input

        Tux Paint allows inputting characters in different languages. Most
        Latin characters (A-Z, ñ, è, etc.) can by entered directly. Some
        languages require that Tux Paint be switched into an alternate input
        mode before entering, and some characters must be composed using
        numerous keypresses.

        When Tux Paint's locale is set to one of the languages that provide
        alternate input modes, a key is used to cycle through normal (Latin
        character) and locale-specific mode or modes.

        Currently supported locales, the input methods available, and the key
        to toggle or cycle modes, are listed below.

          o Japanese — Romanized Hiragana and Romanized Katakana — right [Alt]
            key or left [Alt] key
          o Korean — Hangul 2-Bul — right [Alt] key or left [Alt] key
          o Traditional Chinese — right [Alt] key or left [Alt] key
          o Thai — right [Alt] key

        💡 Note: Many fonts do not include all characters for all languages, so
        sometimes you'll need to change fonts to see the characters you're
        trying to type.



    On-screen Keyboard

        An optional on-screen keyboard is available for the Text and Label
        tools, which can provide a variety of layouts and character composition
        (e.g., composing "a" and "e" into "æ").

        ⚙ See the "Options" and "Extending Tux Paint" documentation for more
        information.






f."Fill" Tool

    The 'Fill' tool 'flood-fills' a contiguous area of your drawing with a
    color of your choice. Three fill options are offered:
      + Solid — click once to fill an area with a solid color.
      + Brush — click and drag to fill an area with a solid color using
        freehand painting.
      + Linear — click and then drag to fill the area with color that fades
        away (a gradient) towards where you drag the mouse.
      + Radial — click once to fill an area with a color that fades away (a
        gradient) radially, centered on where you clicked.
      + Shaped — click once to fill an area with a color that fades away (a
        gradient), following the contours of the shape you're filling.

    📜 Note: Prior to Tux Paint 0.9.24, "Fill" was a Magic tool (see below).
    Prior to Tux Paint 0.9.26, the "Fill" tool only offered the 'Solid' method
    of filling. 'Shaped' fill was introduced in Tux Paint 0.9.29.



g."Magic" Tool (Special Effects)

    The Magic tool is actually a set of special tools. Select one of the
    'magic' effects from the selector on the right. Then, depending on the
    tool, you can either click and drag around the picture, and/or simply click
    the picture once, to apply the effect.

    Magic Controls


        If the tool can be used by clicking and dragging, a 'painting' button
        will be available on the left, below the list of Magic tools on the
        right side of the screen. If the tool can affect the entire picture at
        once, an 'entire picture' button will be available on the right.

        ⚙ Note: If the "nomagiccontrols" option is set, Tux Paint won't display
        the painting or entire picture controls. See the "Options"
        documentation.

        💡 If the magic controls are disabled, the Magic plugin may make
        separate tools available, one for painting and one that affects the
        entire pictre.

    Magic Sizing


        Some tools offer different sizing options. If so, a slider will appear
        at the bottom right side of the screen. This may affect the radius of a
        special effect (e.g., Darken) or painted object (e.g., Patterns), or
        other attributes (e.g., large versus small Brick shapes).

        ⚙ Note: If the "nomagicsizes" option is set, Tux Paint won't display
        the sizing controls. See the "Options" documentation.

        💡 If the sizing option is disabled, the Magic plugin may simply offer a
        default size (e.g., Patterns), or it may make separate tools available
        with different pre-set sizes (e.g., Bricks and Googly Eyes).

        📜 This option was added starting with Tux Paint version 0.9.30.

    See the instructions for each Magic tool (in the 'magic-docs' folder).



h."Eraser" Tool

    This tool is similar to the Paint Brush. Wherever you click (or click and
    drag), the picture will be erased. (This may be white, some other color, or
    to a background picture, depending on the picture.)

    A number of eraser sizes are available, with both square and round shapes.
    Fuzzy-edged round shapes are also available.

    As you move the mouse around, a square outline follows the pointer, showing
    what part of the picture will be erased to white.

    As you erase, a 'squeaky clean' eraser wiping sound is played.

    ⌨ Hold the [X] key while clicking for quick access to a small sharp round
    eraser (not available when the Text or Label tools are selected, when
    you're in the process of rotating a stamp or shape, or when using an
    interactive magic tool). Release the mouse to return to your
    currently-selected tool.




2. Other Controls

a."Undo" and "Redo" Commands

    Clicking the "Undo" button will undo (revert) the last drawing action. You
    can even undo more than once!

    ⌨ Note: You can also press [Control / ⌘] + [Z] on the keyboard to Undo.

    Clicking the "Redo" button will redo the drawing action you just un-did via
    the "Undo" command.

    As long as you don't draw again, you can redo as many times as you had
    undone!

    ⌨ Note: You can also press [Control / ⌘] + [R] on the keyboard to Redo.



b."New" Command

    Clicking the 'New' button will start a new drawing. A dialog will appear
    where you may choose to start a new picture using a solid background color,
    or using a 'Starter' or 'Template' image (see below). You will first be
    asked whether you really want to do this.

    ⌨ Note: You can also press [Control / ⌘] + [N] on the keyboard to start a
    new drawing.

    Special Solid Background Color Choices
        Along with the preset solid colors, you can also choose colors using a
        rainbow palette or a "color mixer". These operate identically to the
        options found in the color palette shown below the canvas when drawing
        a picture. See Main Screen > Lower: Colors > Special color options for
        details.

    'Starter' & Template Images
          o 'Starters' can behave like a page from a coloring book — a
            black-and-white outline of a picture, which you can then color in,
            and the black outline remains intact — or like a 3D photograph,
            where you draw in between a foreground and background layer.
          o 'Templates' are similar, but simply provide a background drawing to
            work off of. Unlike 'Starters', there is no layer that remains in
            the foreground of anything you draw in the picture.

        When using the 'Eraser' tool, the original image from the 'Starter' or
        'Template' will reappear. The 'Flip' and 'Mirror' Magic tools affect
        the orientation of the 'Starter' or 'Template', as well.

        When you load a 'Starter' or 'Template', draw on it, and then click
        'Save,' it creates a new picture file — it doesn't overwrite the
        original, so you can use it again later (by accessing it from the 'New'
        dialog).

        ⚙ You can create your own 'Starter' and Template images. See the
        Extending Tux Paint documentation's sections on 'Starters' and
        Templates.

        💡 You can also convert your saved drawings into Templates directly
        within Tux Paint, from the 'Open' dialog. See "Open", below.



    Erasing Exported Template Images

        If you've selected a Template in your personal templates folder, and it
        was created from within Tux Paint (using the "Template" button in the
        "Open" dialog), you may remove it from within Tux Paint, too. An
        'Erase' (trash can) button will appear at the lower right of the list.
        Click it to erase the selected template. (You will be asked to
        confirm.)

        💡 Note: On Linux, Windows, and macOS, the picture will be placed in
        your desktop's trash can / recycle bin (where you may recover and
        restore it, if you change your mind).

        ⚙ Note: The 'Erase' button may be disabled, via the "noerase" option.




    ⚙ Note: The solid colors can be placed at the end of the 'New' dialog
    (below the Starters and Templates), via the "newcolorslast" option.



c."Open" Command

    This shows you a list of all of the pictures you've saved. If there are
    more than can fit on the screen, use the up and down arrows at the top and
    bottom of the list to scroll through the list of pictures.

    Click a picture to select it, and then...
      + Click the green 'Open' button at the lower left of the list to load the
        selected picture. You will then be able to edit it.

        (Alternatively, you can double-click a picture's icon to load it.)

        💡 If choose to open a picture, and your current drawing hasn't been
        saved, you will be prompted as to whether you want to save it or not.
        (See "Save," below.)

      + Click the brown 'Erase' (trash can) button at the lower right of the
        list to erase the selected picture. (You will be asked to confirm.)

        📜 Note: On Linux (as of version 0.9.22), Windows (as of version
        0.9.27), and macOS (as of version 0.9.29), the picture will be placed
        in your desktop's trash can / recycle bin (where you may recover and
        restore it, if you change your mind).

        ⚙ Note: The 'Erase' button may be disabled, via the "noerase" option.

      + Click the 'Export' button near the lower right to export the selected
        picture to your export folder. (e.g., "~/Pictures/TuxPaint/")

    From the "Open" screen you can also:
      + Click the blue 'Slides' (slide projector) button at the lower left to
        go to slideshow mode. See "Slides", below, for details.

      + Click the blue 'Template' button at the lower left to go to convert the
        selected picture into a new template, which can be used as the basis
        for new drawings.

        📜 Note: The Template creation feature was added to Tux Paint in version
        0.9.31. To learn how to create Templates outside of Tux Paint, see
        Extending Tux Paint

        ⚙ The Template creation feature can be disabled (e.g., by selecting
        "Disable 'Make Template'" in Tux Paint Config. or running Tux Paint
        with the "notemplateexport" option).

      + Click the red 'Back' arrow button at the lower right of the list to
        cancel and return to the picture you were drawing.

    ⌨ Note: You can also press [Control / ⌘] + [O] on the keyboard to bring up
    the 'Open' dialog.



d."Save" Command

    This saves your current picture.

    If you haven't saved it before, it will create a new entry in the list of
    saved images. (i.e., it will create a new file)

    💡 Note: It won't ask you anything (e.g., for a filename). It will simply
    save the picture, and play a "camera shutter" sound effect.

    If you have saved the picture before, or this is a picture you just loaded
    using the "Open" command, you will first be asked whether you want to save
    over the old version, or create a new entry (a new file).

    ⚙ Note: If either the "saveover" or "saveovernew" options are set, it won't
    ask before saving over. See the "Options" documentation.

    ⌨ Note: You can also press [Control / ⌘] + [S] on the keyboard to save.



e."Print" Command

    Click this button and your picture will be printed!

    On most platforms, you can also hold the [Alt] key (called [Option] on
    Macs) while clicking the 'Print' button to get a printer dialog. Note that
    this may not work if you're running Tux Paint in fullscreen mode. See
    below.

    Disabling Printing

        The "noprint" option can be set, which will disable Tux Paint's 'Print'
        button.

        ⚙ See the "Options" documentation.



    Restricting Printing

        The "printdelay" option can be set, which will only allow occasional
        printing — once every so many seconds, as configured by you.

        For example, with "printdelay=60" in Tux Paint's configuration file,
        printing can only occur once per minute (60 seconds).

        ⚙ See the "Options" documentation.



    Printing Commands

        (Linux and Unix only)

        Tux Paint prints by generating a PostScript representation of the
        drawing and sending it to an external program. By default, the program
        is:

            lpr

        This command can be changed by setting a "printcommand" option in Tux
        Paint's configuration file.

        An alternative print command can be invoked by holding the "[Alt]" key
        on the keyboard while clicking clicking the 'Print' button, as long as
        you're not in fullscreen mode, an alternative program is run. By
        default, the program is KDE's graphical print dialog:

            kprinter

        This command can be changed by setting a "altprintcommand" option in
        Tux Paint's configuration file.

        ⚙ See the "Options" documentation.



    Printer Settings

        (Windows and macOS)

        By default, Tux Paint simply prints to the default printer with default
        settings when the 'Print' button is pushed.

        However, if you hold the [Alt] (or [Option]) key on the keyboard while
        clicking the 'Print' button, as long as you're not in fullscreen mode,
        your operating system's printer dialog will appear, where you can
        change the settings.

        You can have the printer configuration changes stored between Tux Paint
        sessions by setting the "printcfg" option.

        If the "printcfg" option is used, printer settings will be loaded from
        the file "printcfg.cfg" in your personal folder (see below). Any
        changes will be saved there as well.

        ⚙ See the "Options" documentation.



    Printer Dialog Options

        By default, Tux Paint only shows the printer dialog (or, on Linux/Unix,
        runs the "altprintcommand"; e.g., "kprinter" instead of "lpr") if the
        [Alt] (or [Option]) key is held while clicking the 'Print' button.

        However, this behavior can be changed. You can have the printer dialog
        always appear by using "--altprintalways" on the command-line, or
        "altprint=always" in Tux Paint's configuration file. Conversely, you
        can prevent the [Alt]/[Option] key from having any effect by using
        "--altprintnever", or "altprint=never".

        ⚙ See the "Options" documentation.






f."Slides" Command (under "Open")

    The 'Slides' button is available in the 'Open' dialog. It can be used to
    play a simple animation within Tux Paint, or a slideshow of pictures. It
    can also export an animated GIF based on the chosen images.

    Chosing pictures

        When you enter the 'Slides' section of Tux Paint, it displays a list of
        your saved files, just like the 'Open' dialog.

        Click each of the images you wish to display in a slideshow-style
        presentation, one by one. A digit will appear over each image, letting
        you know in which order they will be displayed.

        You can click a selected image to unselect it (take it out of your
        slideshow). Click it again if you wish to add it to the end of the
        list.



    Set playback speed

        A sliding scale at the lower left of the screen (next to the 'Play'
        button) can be used to adjust the speed of the slideshow or animated
        GIF, from slowest to fastest. Choose the leftmost setting to disable
        automatic advancement during playback within Tux Paint — you will need
        to press a key or click to go to the next slide (see below).

        💡 Note: The slowest setting does not automatically advance through the
        slides. Use it for when you want to step through them manually. (This
        does not apply to an exported animated GIF.)



    Playback in Tux Paint

        To play a slideshow within Tux Paint, click the 'Play' button.

        💡 Note: If you hadn't selected any images, then all of your saved
        images will be played in the slideshow!

        During the slideshow, press [Space], [Enter] or [Return], or the [Right
        arrow] — or click the 'Next' button at the lower left — to manually
        advance to the next slide. Press [Left arrow] to go back to the
        previous slide.

        Press [Escape], or click the 'Back' button at the lower right, to exit
        the slideshow and return to the slideshow image selection screen.



    Exporting an animated GIF

        Click the 'GIF Export' button near the lower right to have Tux Paint
        generate an animated GIF file based on the selected images.

        💡 Note: At least two images must be selected. (To export a single
        image, use the 'Export' option from the main 'Open' dialog.) If no
        images are selected, Tux Paint will not attempt to generate a GIF based
        on all saved images.

        Pressing [Escape] during the export process will abort the process, and
        return you to the 'Slideshow' dialog.




    Click 'Back' in the slideshow image selection screen to return to the
    'Open' dialog.



g."Quit" Command

    Clicking the 'Quit' button, closing the Tux Paint window, or pushing the
    [Escape] key will quit Tux Paint.

    You will first be prompted as to whether you really want to quit.

    If you choose to quit, and you haven't saved the current picture, you will
    first be asked if wish to save it. If it's not a new image, you will then
    be asked if you want to save over the old version, or create a new entry.
    (See "Save" above.)

    ⚙ Note: If the image is saved, it will be reloaded automatically the next
    time you run Tux Paint -- unless the "startblank" option is set.

    ⚙ Note: The 'Quit' button within Tux Paint, and quitting via the [Escape]
    key, may be disabled, via the "noquit" option.

    In that case, the "window close" button on Tux Paint's title bar (if not in
    fullscreen mode) or the [Alt] + [F4] key sequence may be used to quit.

    If neither of those are possible, the key sequence of [Shift] + [Control /
    ⌘] + [Escape] may be used to quit.

    ⚙ See the "Options" documentation.



h.Sound Muting

    There is no on-screen control button at this time, but by using the [Alt] +
    [S] keyboard sequence, sound effects can be disabled and re-enabled (muted
    and unmuted) while the program is running.

    Note that if sounds are completely disabled via the "nosound" option, the
    [Alt] + [S] key combination has no effect. (i.e., it cannot be used to turn
    on sounds when the parent/teacher wants them disabled.)

    ⚙ See the "Options" documentation.




E. Controlling Tux Paint

1. Using a Mouse or Trackball

Tux Paint's main mode of operation is via any device that appears to your
operating system as a mouse, including standard mice, trackballs, and
trackpads, as well as drawing tablets (usually operated with a stylus) and
touch screens (operated with a finger and/or a stylus) (see "Using a Tablet or
Touchscreen" below for more information).

For drawing and controlling Tux Paint, only a single mouse button is used —
typically, on multi-button mice, this will the left mouse button, but this can
usually be configured at the operating system level. By default, Tux Paint will
ignore input from the other button(s). If a user attempts to use the other
button(s), a pop-up dialog will eventually appear reminding them that only one
button is recognized Tux Paint. However, you may configure Tux Paint to accept
any button as input (see the Options documentation).

a. Scrolling

Many input devices offer a way to quickly scroll within applications — many
mice have a scroll wheel, trackballs have scroll rings, and trackpads recognize
certain "scroll" gestures (e.g., two-finger vertical motion, or vertical motion
on the edge of the trackpad). Tux Paint supports scrolling input to allow quick
scrolling through certain lists (e.g., Stamps, Magic tools, and the New and
Open dialogs).

Tux Paint will also automatically scroll if you click and hold the mouse down
on an scroll button — the "up" and "down" arrow buttons that appear above and
below scrolling lists.

b. Mouse Accessibility

Other devices that appear as a mouse can be used to control Tux Paint. For
example:

  * Head pointing/tracking devices
  * Eye gaze trackers
  * Foot mice

Tux Paint offers a "sticky mouse click" accessibility setting, where a single
click begins a click-and-drag operation, and a subsequent click ends it. (See
the Options documentation.)

2. Using a Tablet or Touchscreen

As noted above, Tux Paint recognizes any device that appears as a mouse. This
means drawing tablets and touchscreens may be used. However, these devices
often support other features beyond X/Y motion, button clicks, and scroll-wheel
motion. Currently, those additional features are not supported by Tux Paint.
Some examples:

  * Pressure and angle
  * Eraser tip
  * Multi-touch gestures

3. Using a Joystick-like Device

Tux Paint may be configured to recognize input from any game controller that
appears to your operating system as a joystick. That even includes modern game
console controllers connected via USB or Bluetooth (e.g., Nintendo Switch or
Microsoft Xbox game pads)!

Numerous configuration options are available to best suit the device being
used, and the user's needs. Analog input will be used for coarse movement, and
digital "hat" input for fine movement. Buttons on the controller can be mapped
to different Tux Paint controls (e.g., acting as the [Escape] key, switching to
the Paint tool, invoking Undo and Redo operations, etc.). See the Options
documentation for more details.

4. Using the Keyboard

Tux Paint offers an option to allow the keyboard to be used to control the
mouse pointer. This includes motion and clicking, as well as shortcuts to
navigate between and within certain parts of the interface. See the Options
documentation for more details.

III. Loading Other Pictures into Tux Paint

A. Overview

Tux Paint's 'Open' dialog only displays pictures you created with Tux Paint. So
what do you do if you want to load some other drawinng or even a photograph
into Tux Paint, so you can edit or draw on it?

You can simply convert the picture to the format Tux Paint uses — PNG (Portable
Network Graphic) — and place it in Tux Paint's "saved" directory/folder. Here
is where to find it (by default):

Windows 10, 8, 7, Vista
    Inside the user's "AppData" folder, e.g.: "C:\Users\username\AppData\
    Roaming\TuxPaint\saved\".

Windows 2000, XP
    Inside the user's "Application Data" folder, e.g.: "C:\Documents and
    Settings\username\Application Data\TuxPaint\saved\".

macOS
    Inside the user's "Library" folder, e.g.: "/Users/username/Library/
    Application Support/Tux Paint/saved/".

Linux/Unix
    Inside a hidden ".tuxpaint" directory, in the user's home directory
    ("$HOME"), e.g. "/home/username/.tuxpaint/saved/".


💡 Note: It is also from this folder that you can copy or open pictures drawn in
Tux Paint using other applications, though the 'Export' option from Tux Paint's
'Open' dialog can be used to copy them to a location that's easier and safer to
access.

B. Using the import script, "tuxpaint-import"

Linux and Unix users can use the "tuxpaint-import" shell script which gets
installed when you install Tux Paint. It uses some NetPBM tools to convert the
image ("anytopnm"), resize it so that it will fit in Tux Paint's canvas
("pnmscale"), and convert it to a PNG ("pnmtopng").

It also uses the "date" command to get the current time and date, which is the
file-naming convention Tux Paint uses for saved files. (Remember, you are never
asked for a 'filename' when you go to save or open pictures!)

To use this script, simply run it from a command-line prompt, and provide it
the name(s) of the file(s) you wish to convert.

They will be converted and placed in your Tux Paint "saved" directory.

💡 Note: If you're doing this for a different user (e.g., your child) you'll
need to make sure to run the command under their account.)

Example:

    $ tuxpaint-import grandma.jpg
    grandma.jpg -> /home/username/.tuxpaint/saved/20211231012359.png
    jpegtopnm: WRITING A PPM FILE

The first line ("tuxpaint-import grandma.jpg") is the command to run. The
following two lines are output from the program while it's working.

Now you can load Tux Paint, and a version of that original picture will be
available under the 'Open' dialog. Just double-click its icon!

C. Importing Pictures Manually

Windows, macOS, and Haiku users who wish to import arbitrary images into Tux
Paint must do so via a manual process.

Load a graphics program that is capable of both loading your picture and saving
a PNG format file. (See the documentation file "PNG.html" for a list of
suggested software, and other references.)

When Tux Paint loads an image that's not the same size as its drawing canvas,
it scales (and sometimes smears the edges of) the image so that it fits within
the canvas.

To avoid having the image stretched or smeared, you can resize it to Tux
Paint's canvas size. This size depends on the size of the Tux Paint window, or
resolution at which Tux Paint is run, if in fullscreen. (Note: The default
resolution is 800x600.) See "Calculating Image Dimensions", below.

1. Naming the File

Save the picture in PNG format. It is highly recommended that you name the
filename using the current date and time, since that's the convention Tux Paint
uses:

    YYYYMMDDhhmmss.png

  * YYYY = Year
  * MM = Month (two digits, "01"-"12")
  * DD = Day of month (two digits, "01"-"31")
  * HH = Hour (two digits, in 24-hour format, "00"-"23")
  * mm = Minute (two digits, "00"-"59")
  * ss = Seconds (two digits, "00"-"59")

Example: "20210731110500.png", for July 31, 2021 at 11:05am.

Place this PNG file in your Tux Paint "saved" directory/folder. (See above.)

2. Calculating Image Dimensions

This part of the documentation needs to be rewritten, since the new
"buttonsize" option was added. For now, try drawing and saving an image within
Tux Paint, then determine what size (pixel width and height) it came out to,
and try to match that when scaling the picture(s) you're importing into Tux
Paint.

IV.Further Reading

Other documentation included with Tux Paint (found in the "docs" folder/
directory) includes:

Using Tux Paint:
      + OPTIONS.html
        Detailed instructions on command-line and configuration-file options,
        for those who don't want to use the Tux Paint Config. tool to manage
        Tux Paint's configuration.
      + 'Magic' Tool Documentation ("magic-docs")
        Documentation for each of the currently-installed 'Magic' tools.


How to extend Tux Paint:
      + EXTENDING.html
        Detailed instructions on extending Tux Paint: creating brushes, stamps,
        starters, and templates; adding fonts; and creating new on-screen
        keyboard layouts and input methods.
      + PNG.html
        Notes on creating PNG format bitmapped (raster) images for use in Tux
        Paint.
      + SVG.html
        Notes on creating SVG format vector images for use in Tux Paint.


Technical information:
      + INSTALL.html
        Instructions for compiling and installing Tux Paint, when applicable.
      + SIGNALS.html
        Information about the POSIX signals that Tux Paint responds to.
      + MAGIC-API.html
        Creating new Magic tools using Tux Paint's plugin API.


Development history and license:
      + AUTHORS.txt
        List of authors and contributors.
      + CHANGES.txt
        Summary of what has changed between releases of Tux Paint.
      + COPYING.txt
        Tux Paint's software license, the GNU General Public License (GPL)



V.How to Get Help

If you need help, there are numerous ways to interact with Tux Paint developers
and other users:

  * Report bugs or request new features via the project's bug-tracking system
  * Participate in the various project mailing lists
  * Contact the developers directly

To learn more, visit the "Contact" page of the official Tux Paint website:
https://tuxpaint.org/contact/

VI.How to Participate

Tux Paint is a volunteer-driven project, and we're happy to accept your help in
a variety of ways:

  * Translate Tux Paint to another language
  * Improve existing translations
  * Create artwork (stamps, starters, templates, brushes)
  * Add or improve features or magic tools
  * Create classroom curriculum
  * Promote or help support others using Tux Paint

To learn more, visit the "Help Us" page of the official Tux Paint website:
https://tuxpaint.org/help/

VII.Follow the Tux Paint project on social media

Tux Paint maintains a presence on a variety of social media networks, where we
post updates and artwork.

  * Join the Tux Paint page on Facebook
  * Follow @TuxPaintDevs on Instagram
  * Follow @tuxpaint@floss.social on Mastodon
  * Follow u/TuxPaintDevs on Reddit
  * Follow @TuxPaintDevs on Threads
  * Follow Tux Paint on Tumblr
  * Follow @TuxPaintTweets on Twitter

VIII. Trademark notices

  * "Linux" is a registered trademark of Linus Torvalds.
  * "Microsoft" and "Windows" are registered trademarks of Microsoft Corp.
  * "Apple" and "macOS" are registered trademarks of Apple Inc.
  * "Facebook", "Instagram", and "Threads" are registered trademarks of Meta
    Platforms, Inc.
  * "Mastodon" is a registered trademark of Mastodon gGmbH.
  * "Reddit" is a registered trademark of Reddit, Inc.
  * "Tumblr" is a registered trademark of Tumblr, Inc.
  * "Twitter" is a registered trademark of X Corp.

