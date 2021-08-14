                                   Tux Paint
                                 version 0.9.26

  A simple drawing program for children

          Copyright © 2002-2021 by various contributors; see AUTHORS.
                            http://www.tuxpaint.org/

                                 June 28, 2021

     ----------------------------------------------------------------------

   +-------------------------------------------+
   |Table of Contents                          |
   |-------------------------------------------|
   |  * About Tux Paint                        |
   |  * Using Tux Paint                        |
   |       * Launching Tux Paint               |
   |       * Title Screen                      |
   |       * Main Screen                       |
   |       * Available Tools                   |
   |            * Drawing Tools                |
   |            * Other Controls               |
   |  * Loading Other Pictures into Tux Paint  |
   |  * Further Reading                        |
   |  * How to Get Help                        |
   |  * How to Participate                     |
   +-------------------------------------------+

     ----------------------------------------------------------------------

                                About Tux Paint

What Is "Tux Paint"?

       Tux Paint is a free drawing program designed for young children (kids
       ages 3 and up). It has a simple, easy-to-use interface, fun sound
       effects, and an encouraging cartoon mascot who helps guide children as
       they use the program. It provides a blank canvas and a variety of
       drawing tools to help your child be creative.

License:

       Tux Paint is an Open Source project, Free Software released under the
       GNU General Public License (GPL). It is free, and the 'source code'
       behind the program is available. (This allows others to add features,
       fix bugs, and use parts of the program in their own GPL'd software.)

       See COPYING.txt for the full text of the GPL license.

Objectives:

       Easy and Fun
               Tux Paint is meant to be a simple drawing program for young
               children. It is not meant as a general-purpose drawing tool.
               It is meant to be fun and easy to use. Sound effects and a
               cartoon character help let the user know what's going on, and
               keeps them entertained. There are also extra-large
               cartoon-style mouse pointer shapes.

       Extensibility
               Tux Paint is extensible. Brushes and 'rubber stamp' shapes can
               be dropped in and pulled out. For example, a teacher can drop
               in a collection of animal shapes and ask their students to
               draw an ecosystem. Each shape can have a sound which is
               played, and textual facts which are displayed, when the child
               selects the shape.

       Portability
               Tux Paint is portable among various computer platforms:
               Windows, Macintosh, Linux, etc. The interface looks the same
               among them all. Tux Paint runs suitably well on older systems,
               and can be built to run better on slow systems.

       Simplicity
               There is no direct access to the computer's underlying
               intricacies. The current image is kept when the program quits,
               and reappears when it is restarted. Saving images requires no
               need to create filenames or use the keyboard. Opening an image
               is done by selecting it from a collection of thumbnails.
               Access to other files on the computer is restricted.

     ----------------------------------------------------------------------

                                Using Tux Paint

Launching Tux Paint

  Linux/Unix Users

         Tux Paint should have placed a laucher icon in your KDE and/or GNOME
         menus, under 'Graphics.'

         Alternatively, you can run the following command at a shell prompt
         (e.g., "$"):

           $ tuxpaint

         If any errors occur, they will be displayed on the terminal (to
         STDERR).

     ----------------------------------------------------------------------

  Windows Users

                                                            [Tux Paint Icon]  
                                                               Tux Paint      

         If you installed Tux Paint on your computer using the 'Tux Paint
         Installer,' it will have asked you whether you wanted a 'Start' menu
         short-cut, and/or a desktop shortcut. If you agreed, you can simply
         run Tux Paint from the 'Tux Paint' section of your 'Start' menu
         (e.g., under 'All Programs'), or by double-clicking the 'Tux Paint'
         icon on your desktop, if you had the installer place one there.

         If you're using the 'portable' (ZIP-file) version of Tux Paint, or
         if you used the 'Tux Paint Installer,' but chose not to have
         shortcuts installed, you'll need to double-click the "tuxpaint.exe"
         icon in the "Tux Paint" folder on your computer.

         By default, the 'Tux Paint Installer' will put Tux Paint's folder in
         "C:\Program Files\", though you may have changed this when you ran
         the installer.

         If you used the 'ZIP-file' download, Tux Paint's folder will be
         wherever you extracted the contents of the ZIP file.

     ----------------------------------------------------------------------

  macOS Users

         Simply double-click the "Tux Paint" icon.

     ----------------------------------------------------------------------

Title Screen

       When Tux Paint first loads, a title/credits screen will appear.

                                 [Title screen]

       Once loading is complete, press a key or click or tap in the Tux Paint
       window to continue. (Or, after about 5 seconds, the title screen will
       go away automatically.)

     ----------------------------------------------------------------------

Main Screen

       The main screen is divided into the following sections:

       Left Side: Toolbar

               The toolbar contains the drawing and editing controls.

  [Tools: Paint, Stamp, Lines, Shapes, Text, Magic, Label, Undo, Redo, Eraser,
                         New, Open, Save, Print, Quit]

       Middle: Drawing Canvas

               The largest part of the screen, in the center, is the drawing
               canvas. This is, obviously, where you draw!

                                    [Canvas]

               Note: The size of the drawing canvas depends on the size of
               Tux Paint. You can change the size of Tux Paint using the Tux
               Paint Config. configuration tool, or by other means. See the
               Options documentation for more details.

       Right Side: Selector

               Depending on the current tool, the selector shows different
               things. e.g., when the Paint Brush or Line tool is selected,
               it shows the various brushes available. When the Rubber Stamp
               tool is selected, it shows the different shapes you can use.
               When the Text or Label tool is selected, it shows various
               fonts.

                 [Selectors - Brushes, Letters, Shapes, Stamps]

       Lower: Colors

               A palette of available colors are shown near the bottom of the
               screen.

 [Colors - Black, White, Red, Pink, Orange, Yellow, Green, Cyan, Blue, Purple,
                                  Brown, Grey]

               On the far right are two special color options, the "color
               picker", which has an outline of an eye-dropper, and allows
               you to pick a color found within your drawing, and the rainbow
               palette, which allows you to pick a color from within a box
               containing thousands of colors.

               (Note: You can define your own colors for Tux Paint. See the
               "Options" documentation.)

       Bottom: Help Area

               At the very bottom of the screen, Tux, the Linux Penguin,
               provides tips and other information while you use Tux Paint.

(Example tip: 'Pick a shape. Click to pick the center, drag, then let go when it
    is the size you want. Move around to rotate it, and click to draw it.')

     ----------------------------------------------------------------------

Available Tools

  Drawing Tools

         "Paint" Tool (Brush)

                 The Paint Brush tool lets you draw freehand, using various
                 brushes (chosen in the Selector on the right) and colors
                 (chosen in the Color palette towards the bottom).

                 If you hold the mouse button down, and move the mouse, it
                 will draw as you move.

                 As you draw, a sound is played. The bigger the brush, the
                 lower the pitch.

     ----------------------------------------------------------------------

         "Stamp" Tool (Rubber Stamps)

                 The Stamp tool is like a set of rubber stamps or stickers.
                 It lets you paste pre-drawn or photographic images (like a
                 picture of a horse, or a tree, or the moon) in your picture.

                 As you move the mouse around the canvas, an outline follows
                 the mouse, showing where the stamp will be placed, and how
                 big it will be. Click to place the stamp.

                 There can be numerous categories of stamps (e.g., animals,
                 plants, outer space, vehicles, people, etc.). Use the Left
                 and Right arrows near the bottom of the Selector to cycle
                 through the collections.

                 Prior to 'stamping' an image onto your drawing, various
                 effects can sometimes be applied (depending on the stamp):

                    * Some stamps can be colored or tinted. If the color
                      palette below the canvas is activated, you can click
                      the colors to change the tint or color of the stamp
                      before placing it in the picture.
                    * Stamps can be shrunk and expanded, by clicking within
                      the triangular-shaped series of bars at the bottom
                      right; the larger the bar, the larger the stamp will
                      appear in your picture.
                    * Many stamps may be flipped vertically, or displayed as
                      a mirror-image, using the control buttons at the bottom
                      right.

                 Different stamps can have different sound effects and/or
                 descriptive (spoken) sounds. Buttons in the Help Area at the
                 lower left (near Tux, the Linux penguin) allow you to
                 re-play the sound effects and descriptive sounds for the
                 currently-selected stamp.

                 (Note: If the "nostampcontrols" option is set, Tux Paint
                 won't display the Mirror, Flip, Shrink and Grow controls for
                 stamps. See the "Options" documentation.)

     ----------------------------------------------------------------------

         "Lines" Tool

                 This tool lets you draw straight lines using the various
                 brushes and colors you normally use with the Paint Brush.

                 Click the mouse and hold it to choose the starting point of
                 the line. As you move the mouse around, a thin 'rubber-band'
                 line will show where the line will be drawn.

                 Let go of the mouse to complete the line. A "sproing!" sound
                 will play.

     ----------------------------------------------------------------------

         "Shapes" Tool

                 This tool lets you draw some simple filled, and un-filled
                 shapes.

                 Select a shape from the selector on the right (circle,
                 square, oval, etc.).

                 Use the options at the bottom right to choose the shape
                 tool's behavior:

                      Shapes from center
                              The shape will expand from where you initially
                              clicked, and will be centered around that
                              position. (This was Tux Paint's only behavior
                              through version 0.9.24.)

                      Shapes from corner
                              The shape will extend with one corner starting
                              from where you initially clicked. This is the
                              default method of most other traditional
                              drawing software. (This option was added
                              starting with Tux Paint version 0.9.25.)

                 Note: If shape controls are disabled (e.g., with the
                 "noshapecontrols" option), the controls will not be
                 presented, and the "shapes from center" method will be used.

                 In the canvas, click the mouse and hold it to stretch the
                 shape out from where you clicked. Some shapes can change
                 proportion (e.g., rectangle and oval may be wider than tall,
                 or taller than wide), others cannot (e.g., square and
                 circle).

                 Let go of the mouse when you're done stretching.

                      Normal Shapes Mode

                              Now you can move the mouse around the canvas to
                              rotate the shape.

                              Click the mouse button again and the shape will
                              be drawn in the current color.

                      Simple Shapes Mode
                              If simple shapes are enabled (e.g., with the
                              "simpleshapes" option), the shape will be drawn
                              on the canvas when you let go of the mouse
                              button. (There's no rotation step.)

     ----------------------------------------------------------------------

         "Text" and "Label" Tools

                 Choose a font (from the 'Letters' available on the right)
                 and a color (from the color palette near the bottom). Click
                 on the screen and a cursor will appear. Type text and it
                 will show up on the screen.

                 Press [Enter] or [Return] and the text will be drawn onto
                 the picture and the cursor will move down one line.

                 Alternatively, press [Tab] and the text will be drawn onto
                 the picture, but the cursor will move to the right of the
                 text, rather than down a line, and to the left. (This can be
                 useful to create a line of text with mixed colors, fonts,
                 styles and sizes.)

                 Clicking elsewhere in the picture while the text entry is
                 still active causes the current line of text to move to that
                 location (where you can continue editing it).

                      "Text" versus "Label"

                              The Text tool is the original text-entry tool
                              in Tux Paint. Text entered using this tool
                              can't be modified or moved later, since it
                              becomes part of the drawing. However, because
                              the text becomes part of the picture, it can be
                              drawn over or modified using Magic tool effects
                              (e.g., smudged, tinted, embossed, etc.)

                              When using the Label tool (which was added to
                              Tux Paint in version 0.9.22), the text 'floats'
                              over the image, and the details of the label
                              (the text, the position of the label, the font
                              choice and the color) get stored separately.
                              This allows the label to be repositioned or
                              edited later.

                              The Label tool can be disabled (e.g., by
                              selecting "Disable 'Label' Tool" in Tux Paint
                              Config. or running Tux Paint with the "nolabel"
                              option).

                      International Character Input

                              Tux Paint allows inputting characters in
                              different languages. Most Latin characters
                              (A-Z, ñ, è, etc.) can by entered directly. Some
                              languages require that Tux Paint be switched
                              into an alternate input mode before entering,
                              and some characters must be composed using
                              numerous keypresses.

                              When Tux Paint's locale is set to one of the
                              languages that provide alternate input modes, a
                              key is used to cycle through normal (Latin
                              character) and locale-specific mode or modes.

                              Currently supported locales, the input methods
                              available, and the key to toggle or cycle
                              modes, are listed below. Note: Many fonts do
                              not include all characters for all languages,
                              so sometimes you'll need to change fonts to see
                              the characters you're trying to type.

                                 * Japanese — Romanized Hiragana and
                                   Romanized Katakana — right [Alt] key
                                 * Korean — Hangul 2-Bul — right [Alt] key or
                                   left [Alt] key
                                 * Traditional Chinese — right [Alt] key or
                                   left [Alt] key
                                 * Thai — right [Alt] key

                      On-screen Keyboard

                              An optional on-screen keyboard is available for
                              the Text and Label tools, which can provide a
                              variety of layouts and character composition
                              (e.g., composing "a" and "e" into "æ"). See the
                              "Options" and "Extending Tux Paint"
                              documentation for more information.

     ----------------------------------------------------------------------

         "Fill" Tool

                 The 'Fill' tool 'flood-fills' a contiguous area of your
                 drawing with a color of your choice. Three fill options are
                 offered:
                    * Solid — click once to fill an area with a solid color.
                    * Linear — click and then drag to fill the area with
                      color that fades away (a gradient) towards where you
                      drag the mouse.
                    * Radial — click once to fill an area with a color that
                      fades away (a gradient) radially, centered on where you
                      clicked.

                 Note: Prior to Tux Paint 0.9.24, this was a Magic tool (see
                 below). Note: Prior to Tux Paint 0.9.26, this tool only
                 offered the 'Solid' method of filling.

     ----------------------------------------------------------------------

         "Magic" Tool (Special Effects)

                 The Magic tool is actually a set of special tools. Select
                 one of the 'magic' effects from the selector on the right.
                 Then, depending on the tool, you can either click and drag
                 around the picture, and/or simply click the picture once, to
                 apply the effect.

                 If the tool can be used by clicking and dragging, a
                 'painting' button will be available on the left, below the
                 list of Magic tools on the right side of the screen. If the
                 tool can affect the entire picture at once, an 'entire
                 picture' button will be available on the right.

                 See the instructions for each Magic tool (in the
                 'magic-docs' folder).

     ----------------------------------------------------------------------

         "Eraser" Tool

                 This tool is similar to the Paint Brush. Wherever you click
                 (or click and drag), the picture will be erased. (This may
                 be white, some other color, or to a background picture,
                 depending on the picture.)

                 A number of eraser sizes are available, both round and
                 square.

                 As you move the mouse around, a square outline follows the
                 pointer, showing what part of the picture will be erased to
                 white.

                 As you erase, a 'squeaky clean' eraser wiping sound is
                 played.

     ----------------------------------------------------------------------

  Other Controls

         "Undo" Command

                 Clicking this tool will undo the last drawing action. You
                 can even undo more than once!

                 Note: You can also press [Control] + [Z] on the keyboard to
                 Undo.

     ----------------------------------------------------------------------

         "Redo" Command

                 Clicking this tool will redo the drawing action you just
                 un-did with the 'Undo' button.

                 As long as you don't draw again, you can redo as many times
                 as you had undone!

                 Note: You can also press [Control] + [R] on the keyboard to
                 Redo.

     ----------------------------------------------------------------------

         "New" Command

                 Clicking the 'New' button will start a new drawing. A dialog
                 will appear where you may choose to start a new picture
                 using a solid background color, or using a 'Starter' or
                 'Template' image (see below). You will first be asked
                 whether you really want to do this.

                 Note: You can also press [Control] + [N] on the keyboard to
                 start a new drawing.

                 'Starter' & Template Images

                   'Starters' can behave like a page from a coloring book — a
                   black-and-white outline of a picture, which you can then
                   color in, and the black outline remains intact — or like a
                   3D photograph, where you draw in between a foreground and
                   background layer.

                   'Templates' are similar, but simply provide a background
                   drawing to work off of. Unlike 'Starters', there is no
                   layer that remains in the foreground of anything you draw
                   in the picture.

                   When using the 'Eraser' tool, the original image from the
                   'Starter' or 'Template' will reappear. The 'Flip' and
                   'Mirror' Magic tools affect the orientation of the
                   'Starter' or 'Template', as well.

                   When you load a 'Starter' or 'Template', draw on it, and
                   then click 'Save,' it creates a new picture file — it
                   doesn't overwrite the original, so you can use it again
                   later (by accessing it from the 'New' dialog).

     ----------------------------------------------------------------------

         "Open" Command

                 This shows you a list of all of the pictures you've saved.
                 If there are more than can fit on the screen, use the up and
                 down arrows at the top and bottom of the list to scroll
                 through the list of pictures.

                 Click a picture to select it, and then...

                      * Click the green 'Open' button at the lower left of
                        the list to load the selected picture.

                        (Alternatively, you can double-click a picture's icon
                        to load it.)

                      * Click the brown 'Erase' (trash can) button at the
                        lower right of the list to erase the selected
                        picture. (You will be asked to confirm.)

                        Note: As of version 0.9.22, the picture will be
                        placed in your desktop's trash can, on Linux only.

                      * Click the 'Export' button near the lower right to
                        export the image to your export folder. (e.g.,
                        "~/Pictures/TuxPaint/")

                      * Click the blue 'Slides' (slide projector) button at
                        the lower left to go to slideshow mode. See "Slides",
                        below, for details.

                      * Click the red 'Back' arrow button at the lower right
                        of the list to cancel and return to the picture you
                        were drawing.

                 If choose to open a picture, and your current drawing hasn't
                 been saved, you will be prompted as to whether you want to
                 save it or not. (See "Save," below.)

                 Note: You can also press [Control] + [O] on the keyboard to
                 bring up the 'Open' dialog.

     ----------------------------------------------------------------------

         "Save" Command

                 This saves your current picture.

                 If you haven't saved it before, it will create a new entry
                 in the list of saved images. (i.e., it will create a new
                 file)

                 Note: It won't ask you anything (e.g., for a filename). It
                 will simply save the picture, and play a "camera shutter"
                 sound effect.

                 If you have saved the picture before, or this is a picture
                 you just loaded using the "Open" command, you will first be
                 asked whether you want to save over the old version, or
                 create a new entry (a new file).

                 Note: If either the "saveover" or "saveovernew" options are
                 set, it won't ask before saving over. See the "Options"
                 documentation.

                 Note: You can also press [Control] + [S] on the keyboard to
                 save.

     ----------------------------------------------------------------------

         "Print" Command

                 Click this button and your picture will be printed!

                 On most platforms, you can also hold the [Alt] key (called
                 [Option] on Macs) while clicking the 'Print' button to get a
                 printer dialog. Note that this may not work if you're
                 running Tux Paint in fullscreen mode. See below.

                      Disabling Printing

                              The "noprint" option can be set, which will
                              disable Tux Paint's 'Print' button.

                              See the "Options" documentation.

                      Restricting Printing

                              The "printdelay" option can be set, which will
                              only allow occasional printing — once every so
                              many seconds, as configured by you.

                              For example, with "printdelay=60" in Tux
                              Paint's configuration file, printing can only
                              occur once per minute (60 seconds).

                              See the "Options" documentation.

                      Printing Commands

                              (Linux and Unix only)

                              Tux Paint prints by generating a PostScript
                              representation of the drawing and sending it to
                              an external program. By default, the program
                              is:

                                lpr

                              This command can be changed by setting a
                              "printcommand" option in Tux Paint's
                              configuration file.

                              An alternative print command can be invoked by
                              holding the "[Alt]" key on the keyboard while
                              clicking clicking the 'Print' button, as long
                              as you're not in fullscreen mode, an
                              alternative program is run. By default, the
                              program is KDE's graphical print dialog:

                                kprinter

                              This command can be changed by setting a
                              "altprintcommand" option in Tux Paint's
                              configuration file.

                              See the "Options" documentation.

                      Printer Settings

                              (Windows and macOS)

                              By default, Tux Paint simply prints to the
                              default printer with default settings when the
                              'Print' button is pushed.

                              However, if you hold the [Alt] (or [Option])
                              key on the keyboard while clicking the 'Print'
                              button, as long as you're not in fullscreen
                              mode, your operating system's printer dialog
                              will appear, where you can change the settings.

                              You can have the printer configuration changes
                              stored between Tux Paint sessions by setting
                              the "printcfg" option.

                              If the "printcfg" option is used, printer
                              settings will be loaded from the file
                              "printcfg.cfg" in your personal folder (see
                              below). Any changes will be saved there as
                              well.

                              See the "Options" documentation.

                      Printer Dialog Options

                              By default, Tux Paint only shows the printer
                              dialog (or, on Linux/Unix, runs the
                              "altprintcommand"; e.g., "kprinter" instead of
                              "lpr") if the [Alt] (or [Option]) key is held
                              while clicking the 'Print' button.

                              However, this behavior can be changed. You can
                              have the printer dialog always appear by using
                              "--altprintalways" on the command-line, or
                              "altprint=always" in Tux Paint's configuration
                              file. Conversely, you can prevent the
                              [Alt]/[Option] key from having any effect by
                              using "--altprintnever", or "altprint=never".

                              See the "Options" documentation.

     ----------------------------------------------------------------------

         "Slides" Command (under "Open")

                 The 'Slides' button is available in the 'Open' dialog. It
                 can be used to play a simple animation within Tux Paint, or
                 a slideshow of pictures. It can also export an animated GIF
                 based on the chosen images.

                      Chosing pictures

                              When you enter the 'Slides' section of Tux
                              Paint, it displays a list of your saved files,
                              just like the 'Open' dialog.

                              Click each of the images you wish to display in
                              a slideshow-style presentation, one by one. A
                              digit will appear over each image, letting you
                              know in which order they will be displayed.

                              You can click a selected image to unselect it
                              (take it out of your slideshow). Click it again
                              if you wish to add it to the end of the list.

                      Set playback speed

                              A sliding scale at the lower left of the screen
                              (next to the 'Play' button) can be used to
                              adjust the speed of the slideshow or animated
                              GIF, from slowest to fastest. Choose the
                              leftmost setting to disable automatic
                              advancement during playback within Tux Paint —
                              you will need to press a key or click to go to
                              the next slide (see below).

                              Note: The slowest setting does not
                              automatically advance through the slides. Use
                              it for when you want to step through them
                              manually. (This does not apply to an exported
                              animated GIF.)

                      Playback in Tux Paint

                              To play a slideshow within Tux Paint, click the
                              'Play' button. (Note: If you hadn't selected
                              ANY images, then ALL of your saved images will
                              be played in the slideshow!)

                              During the slideshow, press [Space], [Enter] or
                              [Return], or the [Right arrow] — or click the
                              'Next' button at the lower left — to manually
                              advance to the next slide. Press [Left arrow]
                              to go back to the previous slide.

                              Press [Escape], or click the 'Back' button at
                              the lower right, to exit the slideshow and
                              return to the slideshow image selection screen.

                      Exporting an animated GIF

                              Click the 'GIF Export' button near the lower
                              right to have Tux Paint generate an animated
                              GIF file based on the selected images.

                              Note: At least two images must be selected. (To
                              export a single image, use the 'Export' option
                              from the main 'Open' dialog.) If no images are
                              selected, Tux Paint will NOT attempt to
                              generate a GIF based on all saved images.

                              Pressing [Escape] during the export process
                              will abort the process, and return you to the
                              'Slideshow' dialog.

                 Click 'Back' in the slideshow image selection screen to
                 return to the 'Open' dialog.

     ----------------------------------------------------------------------

         "Quit" Command

                 Clicking the 'Quit' button, closing the Tux Paint window, or
                 pushing the [Escape] key will quit Tux Paint.

                 You will first be prompted as to whether you really want to
                 quit.

                 If you choose to quit, and you haven't saved the current
                 picture, you will first be asked if wish to save it. If it's
                 not a new image, you will then be asked if you want to save
                 over the old version, or create a new entry. (See "Save"
                 above.)

                 Note: If the image is saved, it will be reloaded
                 automatically the next time you run Tux Paint -- unless the
                 "startblank" option is set.

                 Note: The 'Quit' button within Tux Paint, and quitting via
                 the [Escape] key, may be disabled, via the "noquit" option.

                 In that case, the "window close" button on Tux Paint's title
                 bar (if not in fullscreen mode) or the [Alt] + [F4] key
                 sequence may be used to quit.

                 If neither of those are possible, the key sequence of
                 [Shift] + [Control] + [Escape] may be used to quit.

                 See the "Options" documentation.

     ----------------------------------------------------------------------

         Sound Muting

                 There is no on-screen control button at this time, but by
                 using the [Alt] + [S] keyboard sequence, sound effects can
                 be disabled and re-enabled (muted and unmuted) while the
                 program is running.

                 Note that if sounds are completely disabled via the
                 "nosound" option, the [Alt] + [S] key combination has no
                 effect. (i.e., it cannot be used to turn on sounds when the
                 parent/teacher wants them disabled.)

     ----------------------------------------------------------------------

                     Loading Other Pictures into Tux Paint

     Tux Paint's 'Open' dialog only displays pictures you created with Tux
     Paint. So what do you do if you want to load some other drawinng or even
     a photograph into Tux Paint, so you can edit or draw on it?

     You can simply convert the picture to the format Tux Paint uses — PNG
     (Portable Network Graphic) — and place it in Tux Paint's "saved"
     directory/folder. Here is where to find it (by default):

     Windows 10, 8, 7, Vista
             Inside the user's "AppData" folder, e.g.:
             "C:\Users\username\AppData\Roaming\TuxPaint\saved\".

     Windows 2000, XP
             Inside the user's "Application Data" folder, e.g.: "C:\Documents
             and Settings\username\Application Data\TuxPaint\saved\".

     macOS
             Inside the user's "Library" folder, e.g.:
             "/Users/username/Library/Application Support/Tux Paint/saved/".

     Linux/Unix
             Inside a hidden ".tuxpaint" directory, in the user's home
             directory ("$HOME"), e.g. "/home/username/.tuxpaint/saved/".

     Note: It is also from this folder that you can copy or open pictures
     drawn in Tux Paint using other applications, though the 'Export' option
     from Tux Paint's 'Open' dialog can be used to copy them to a location
     that's easier and safer to access.

Using the import script, "tuxpaint-import"

       Linux and Unix users can use the "tuxpaint-import" shell script which
       gets installed when you install Tux Paint. It uses some NetPBM tools
       to convert the image ("anytopnm"), resize it so that it will fit in
       Tux Paint's canvas ("pnmscale"), and convert it to a PNG ("pnmtopng").

       It also uses the "date" command to get the current time and date,
       which is the file-naming convention Tux Paint uses for saved files.
       (Remember, you are never asked for a 'filename' when you go to save or
       open pictures!)

       To use this script, simply run it from a command-line prompt, and
       provide it the name(s) of the file(s) you wish to convert.

       They will be converted and placed in your Tux Paint "saved" directory.
       (Note: If you're doing this for a different user (e.g., your child)
       you'll need to make sure to run the command under their account.)

       Example:

         $ tuxpaint-import grandma.jpg
         grandma.jpg -> /home/username/.tuxpaint/saved/20211231012359.png
         jpegtopnm: WRITING A PPM FILE

       The first line ("tuxpaint-import grandma.jpg") is the command to run.
       The following two lines are output from the program while it's
       working.

       Now you can load Tux Paint, and a version of that original picture
       will be available under the 'Open' dialog. Just double-click its icon!

Importing Pictures Manually

       Windows, macOS, and Haiku users who wish to import arbitrary images
       into Tux Paint must do so via a manual process.

       Load a graphics program that is capable of both loading your picture
       and saving a PNG format file. (See the documentation file "PNG.html"
       for a list of suggested software, and other references.)

       When Tux Paint loads an image that's not the same size as its drawing
       canvas, it scales (and sometimes smears the edges of) the image so
       that it fits within the canvas.

       To avoid having the image stretched or smeared, you can resize it to
       Tux Paint's canvas size. This size depends on the size of the Tux
       Paint window, or resolution at which Tux Paint is run, if in
       fullscreen. (Note: The default resolution is 800x600.) See
       "Calculating Image Dimensions", below.

       Save the picture in PNG format. It is highly recommended that you name
       the filename using the current date and time, since that's the
       convention Tux Paint uses:

         YYYYMMDDhhmmss.png

         * YYYY = Year
         * MM = Month (two digits, "01"-"12")
         * DD = Day of month (two digits, "01"-"31")
         * HH = Hour (two digits, in 24-hour format, "00"-"23")
         * mm = Minute (two digits, "00"-"59")
         * ss = Seconds (two digits, "00"-"59")

       Example: "20210731110500.png", for July 31, 2021 at 11:05am.

       Place this PNG file in your Tux Paint "saved" directory/folder. (See
       above.)

  Calculating Image Dimensions

         This part of the documentation needs to be rewritten, since the new
         "buttonsize" option was added. For now, try drawing and saving an
         image within Tux Paint, then determine what size (pixel width and
         height) it came out to, and try to match that when scaling the
         picture(s) you're importing into Tux Paint.

     ----------------------------------------------------------------------

                                Further Reading

     Other documentation included with Tux Paint (found in the "docs"
     folder/directory) includes:
       * 'Magic' Tool Documentation ("magic-docs")
         Documentation for each of the currently-installed 'Magic' tools.
       * AUTHORS.txt
         List of authors and contributors.
       * CHANGES.txt
         Summary of what has changed between releases of Tux Paint.
       * COPYING.txt
         Tux Paint's software license, the GNU General Public License (GPL)
       * INSTALL.html
         Instructions for compiling and installing Tux Paint, when
         applicable.
       * EXTENDING.html
         Detailed instructions on extending Tux Paint: creating brushes,
         stamps, starters, and templates; adding fonts; and creating new
         on-screen keyboard layouts and input methods.
       * OPTIONS.html
         Detailed instructions on command-line and configuration-file
         options, for those who don't want to use the Tux Paint Config. tool
         to manage Tux Paint's configuration.
       * PNG.html
         Notes on creating PNG format bitmapped (raster) images for use in
         Tux Paint.
       * SVG.html
         Notes on creating SVG format vector images for use in Tux Paint.
       * SIGNALS.html
         Information about the POSIX signals that Tux Paint responds to.

     ----------------------------------------------------------------------

                                How to Get Help

     If you need help, there are numerous ways to interact with Tux Paint
     developers and other users.

       * Report bugs or request new features via the project's bug-tracking
         system
       * Participate in the various project mailing lists
       * Chat with developers and other users over IRC
       * Contact the developers directly

     To learn more, visit the "Contact" page of the official Tux Paint
     website: http://tuxpaint.org/contact/

     ----------------------------------------------------------------------

                               How to Participate

     Tux Paint is a volunteer-driven project, and we're happy to accept your
     help in a variety of ways.

       * Translate Tux Paint to another language
       * Improve existing translations
       * Create artwork (stamps, starters, templates, brushes)
       * Add or improve features or magic tools
       * Create classroom curriculum
       * Promote or help support others using Tux Paint

     To learn more, visit the "Help Us" page of the official Tux Paint
     website: http://tuxpaint.org/help/
