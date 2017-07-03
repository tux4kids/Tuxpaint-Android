README.txt for Tux Paint

Tux Paint - A simple drawing program for children.

Copyright 2002 by Bill Kendrick
bill@newbreedsoftware.com
http://www.newbreedsoftware.com/tuxpaint/

June 14, 2002 - September 25, 2002


About:
------
  Tux Paint is a drawing program for young children.  (Say, 3-10 years old.)
  It is mainly being developed to fill an educational/edutainment need
  for the Open Source "Linux" operating system, but is compatible with
  many other platforms, including Windows, MacOS, BeOS, other Unix variants,
  etc.


License:
--------
  Tux Paint an Open Source project, released under the
  GNU General Public License (GPL).  It is free, and the 'source code'
  behind the program is available.  (This allows others to add features,
  fix bugs, and use parts of the program in their own GPL'd Open Source
  software.)

  See COPYING.txt for the full text of the GPL license.


Objectives:
-----------
  Easy and Fun
  ------------
    Tux Paint is meant to be a simple drawing program for young children.
    It is not meant as a general-purpose drawing tool.  It IS meant to be
    fun and easy to use.  Sound effects and a cartoon character help let
    the user know what's going on, and keeps them entertained.
    There are also extra-large cartoon-style mouse pointer shapes.

  Extensibility
  -------------
    Tux Paint is extensible.  Brushes and "rubber stamp" shapes can be
    dropped in and pulled out.  For example, a teacher can drop in a
    collection of animal shapes and ask their students to draw an
    ecosystem.  Each shape can have a sound which is played, and
    textual facts which are displayed, when the child selects the shape.

  Portability
  -----------
    Tux Paint is portable among various computer platforms:
    Windows, Macintosh, Linux, etc.  The interface looks the same among
    them all.  Tux Paint runs suitably well on older systems (like a
    Pentium 133), and can be built to run better on slow systems.

  Simplicity
  ----------
    There is no direct access to the computer's underlying intricacies.
    The current image is kept when the program quits, and reappears when
    it is restarted.  Saving images requires no need to create filenames
    or use the keyboard.  Opening an image is done by selecting it from
    a collection of thumbnails.


Other Documentation
-------------------
  Other documentation included with Tux Paint (in the "docs" folder/directory)
  include:

    AUTHORS.txt     - List of authors and contributors
    CHANGES.txt     - Summary of changed between releases
    COPYING.txt     - Copying license (The GPL)
    INSTALL.txt     - Instructions for compiling/installing, when applicable
    PNG.txt         - Notes on creating PNG format images for use in Tux Paint
    README.txt      - (This file)
    TODO.txt        - A list of pending features or bugs needing fixed


Using Tux Paint
---------------
  Building Tux Paint
  ------------------
    To compile Tux Paint from source, please refer to INSTALL.txt.


  Loading Tux Paint
  -----------------
    Linux/Unix Users
    ----------------
      Run the following command at a shell prompt (e.g., "$"):

        $ tuxpaint

      It is also possible to make a launcher button or icon
      (e.g. in GNOME or KDE).  See your desktop environment's
      documentation for details...

      If any errors occur, they will be displayed on the terminal
      (to "stderr").


    Windows Users
    -------------
      Simply double-click the "tuxpaint.exe" icon in the Tux Paint
      folder.

      If any errors occur, they will be stored in a file named
      "stderr.txt" in the Tux Paint folder.

      See "INSTALL.txt" for details on making a 'Shortcut' icon to Tux Paint,
      which lets you easily set command-line options.

      To run Tux Paint and provide command-line options directly, you will
      need to run "tuxpaint.exe" from an MSDOS Prompt window.
      (See "INSTALL.txt" for details.)
      

    Macintosh Users
    ---------------
      Simply double-click the "Tux Paint" icon in the Tux Paint
      folder.

      [ how to issue comamnd-line options under MacOS?  Option-double-click? ]


  Options
  -------
    Configuration File
    ------------------
      You can create a simple configuration file for Tux Paint, which it
      will read each time you start it up.

      The file is simply a plain text file containing the options
      you want enabled:

        fullscreen=yes
	--------------
	  Run the program in full screen mode, rather than in a window.

	nosound=yes
	-----------
	  Disable sound effects.

	noquit=yes
	----------
	  Disable the on-screen "Quit" button.
	  (Pressing the "Escape" key or clicking the window close button
	  still works.
	
	noprint=yes
	-----------
	  Disable the printing feature.

	printdelay=SECONDS
	------------------
	  Restrict printing so that printing can occur only once every
	  SECONDS seconds.

	printcommand=COMMAND
	--------------------
	  Use the command COMMAND to print a PNG file.
	  If not set, the default command is:

	    pngtopnm | pnmtops | lpr

	  Which converts the PNG to a NetPBM 'portable anymap',
	  then converts that to a PostScript file, and finally
	  sends that to the printer, using the "lpr" command.
	
	simpleshapes=yes
	----------------
	  Disable rotation mode in shape tool.  Click, drag, release is
	  all that's needed to draw a shape.
	
	uppercase=yes
	-------------
	  All text will be rendered only in uppercase (e.g., "Brush" will
	  be "BRUSH").  Useful for children who can read, but who have only
	  learned uppercase letters so far.

	grab=yes
	--------
	  Tux Paint will attempt to 'grab' the mouse and keyboard, so that
	  the mouse is confined to Tux Paint's window, and nearly all keyboard
	  input is passed directly to it.  This is useful to disable
	  operating system actions that could get the user out of Tux Paint
	  [Alt]-[Tab] window cycling, [Ctrl]-[Escape], etc.  Especially
	  useful in fullscreen mode.
	
	nowheelmouse=yes
	----------------
	  This disables support for the wheel on mice that have it.
	  (Normally, the wheel will scroll the selector menu on the right.)
	
	saveover=yes
	------------
	  This disables the "Save over the old version...?" prompt when
	  saving an existing file.  With this option, the older version
	  will always be replaced by the new version, automatically.
	
	saveover=new
	------------
	  This also disables the "Save over the old version...?" prompt
	  when saving an existing file.  This option, however, will always
	  save a new file, rather than overwrite the older version.
	
	saveover=ask
	------------
	  (This option is redundant, since this is the default.)
	  When saving an existing drawing, you will be first asked whether
	  to save over the older version or not.
	  

      Linux Users
      -----------
        The file you should create is called ".tuxpaintrc" and it
	should be placed in your home directory.
	(a.k.a. "~/.tuxpaintrc" or "$HOME/.tuxpaintrc")

      Windows Users
      -------------
        The file you should create is called "tuxpaint.cfg" and it
	should be placed in Tux Paint's folder.
	

    Command-Line Options
    --------------------
    Options can also be issued on the command-line when you start Tux Paint.
    
      --fullscreen
      --nosound
      --noquit
      --noprint
      --printdelay=SECONDS
      --simpleshapes
      --uppercase
      --grab
      --nowheelmouse
      --saveover
      --saveovernew
      -----------
        These enable the options described above.

      --windowed
      --sound
      --quit
      --print
      --printdelay=0
      --complexshapes
      --mixedcase
      --dontgrab
      --wheelmouse
      --saveoverask
      -----------
        These options can be used to override any settings made in
	the configuration file.  (If the option isn't set in the
	configuration file, no overriding option is necessary.)

      --lang language
      ---------------
        Run Tux Paint in one of the supported languages.
	Choices available currently include:
	
	  english
	  bokmal
	  danish     dansk
	  dutch
	  finnish    suomi
	  french     francais
	  german     deutsch
	  italian    italiano
	  norwegian  nynorsk
	  spanish    espanol
	  swedish    svenska
	  turkish

      --locale locale
      ---------------
        Run Tux Paint in one of the support languages.
	See "Choosing a Different Language" below for the
	locale strings (e.g., "de_DE@euro" for German) to use.
	
	(If your locale is already set, e.g. with the
	"LANG" environment variable, this option is not necessary,
	since Tux Paint honors your environment's setting, if possible.)


  Command-Line Info. Options
  --------------------------
    The following options display some informative text on the screen.
    Tux Paint doesn't actually start up and run afterwards, however.

      --version
      ---------
        Display the version number and date of the copy of Tux Paint
	you are running.

      --copying
      ---------
        Show brief license information about copying Tux Paint.

      --usage
      -------
        Display the list of available command-line options.

      --help
      ------
        Display brief help on using Tux Paint.
  
  
  Choosing a Different Language
  -----------------------------
    Tux Paint has been translated into a number of languages.
    To access the translations, you can use the "--lang" option on
    the command-line to set the language (e.g. "--lang spanish").
    
    Tux Paint also honors your environment's current locale.
    (You can override it on the command-line using the "--locale" option
    (see above))
    
    The following are supported:

      da_DK      -                   Danish
      de_DE@euro - Deutsch         / German
      es_ES@euro - Espanol         / Spanish
      fi_FI@euro - Suomi           / Finnish
      fr_FR@euro - Francais        / French
      is_IS      - Islenska        / Icelandic
      it_IT@euro - Italiano        / Italian
      nb_NO      - Norsk (bokmal)  / Norwegian Bokmal
      nn_NO      - Norsk (nynorsk) / Norwegian Nynorsk
      nl_NL@euro -                   Dutch
      sv_SE@euro - Svenska         / Swedish
      tr_TR@euro -                   Turkish
  
  
    Setting Your Environment's Locale
    ---------------------------------
      Changing your locale will affect much of your environment.
      
      As stated above, along with letting you choose the language at
      runtime using command-line options ("--lang" and "--locale"),
      Tux Paint honors the global locale setting in your environment.

      If you haven't already set your environment's locale, the following
      will briefly explain how:
      
      Linux/Unix Users
      ----------------
        First, be sure the locale you want to use is enabled by
	editing the file "/etc/locale.gen" on your system and
	then running the program "locale-gen" as root.
	
	Note: Debian users may be able to simply run the command
	"dpkg-reconfigure locales".
	
        Then, before running Tux Paint, set your "LANG" environment
	variable to one of the locales listed above.  (If you want all
	programs that can be translated to be, you may wish to place
	the following in your login script; e.g. ~/.profile,
	~/.bashrc, ~/.cshrc, etc.)


        For example, in a Bourne Shell (like BASH):

          export LANG=es_ES@euro ; tuxpaint


        And in a C Shell (like TCSH):

          setenv LANG es_ES@euro ; tuxpaint


      Windows Users
      -------------
        TuxPaint will recoginse the current locale and use the appropriate
	files by default. So this section is only for people trying different
	languages.

	The simplest thing to do is to use the '--lang' switch in the
	shortcut (see "INSTALL.txt").  However, by using an MSDOS Prompt
	window, it is also possible to issue a command like this:

	  set LANG=es_ES@euro

	...which will set the language for the lifetime of that DOS window.

	For something more permanent, try editing your computer's
	'autoexec.bat' file using Windows' "sysedit" tool:

	Windows 95/98:
	--------------
	  1) Click on the 'Start' button, and select 'Run...'.
	  2) Type "sysedit" into the 'Open:' box (with or without quotes).
	  3) Click 'OK'.
	  4) Locate the AUTOEXEC.BAT window in the System Configuration Editor.
	  5) Add the following at the bottom of the file:
	     set LANG=es_ES@euro
	  6) Close the System Configuration Editor, answering yes to save
	     the changes.
	  7) Restart your machine.


	To affect the ENTIRE MACHINE, and ALL APPLICATIONS, it is possible to
	use the "Regional Settings" control panel:

	  1) Click on the 'Start' button, and select 'Settings|Control PAnel'.
	  2) Double click on the "Regional Settings" globe.
	  3) Select a language/region from the drop down list.
	  4) Click 'OK'.
	  5) Restart your machine when prompted.


  Title Screen
  ------------
    When Tux Paint first loads, a title/credits screen will appear.

    Once loading is complete, press a key or click on the mouse to continue.


  Main Screen
  -----------
    The main screen is divided into the following sections:

    Left Side: Toolbar
    ------------------
      The toolbar contains the drawing and editing controls.


    Middle: Drawing Canvas
    ----------------------
      The largest part of the screen, in the center, is the drawing
      canvas.  This is, obviously, where you draw!


    Right Side: Selector
    --------------------
      Depending on the current tool, the selector shows different
      things.  e.g., when the Paint Brush is selected, it shows
      the various brushes available.  When the Rubber Stamp is selected,
      it shows the different shapes you can use.


    Lower: Colors
    -------------
      A palette of available colors are shown near the bottom of the
      screen.


    Bottom: Help Area
    -----------------
      At the very bottom of the screen, Tux, the Linux Penguin,
      provides tips and other information while you draw.


  Available Tools
  ---------------
    Drawing Tools
    -------------
      Paint Brush
      -----------
        The Paint Brush tool lets you draw freehand, using various
        brushes (chosen in the Selector on the right) and colors
        (chosen in the Color palette towards the bottom).

        If you hold the mouse button down, and move the mouse, it will
        draw as you move.

        As you draw, a sound is played.  The bigger the brush, the
        lower the pitch.


      Stamp (Rubber Stamp)
      --------------------
        The Stamp tool is like a rubber stamp, or stickers.  It lets you
        paste pre-drawn images (like a picture of a horse, or a tree, or
        the moon) in your picture.

        As you move the mouse around, a rectangular outline follows the
        mouse, showing where the stamp will be placed.

        Different stamps can have different sound effects.


      Lines
      -----
        This tool lets you draw straight lines using the various
        brushes and colors you normally use with the Paint Brush.

        Click the mouse and hold it to choose the starting point of the
        line.  As you move the mouse around, a thin 'rubber-band' line
        will show where the line will be drawn.

        Let go of the mouse to complete the line.  A "sproing!" sound will
        play.


      Shapes
      ------
        This tool lets you draw some simple filled, and un-filled shapes.

	Select a shape from the selector on the right (circle, square,
	oval, etc.).

	In the canvas, click the mouse and hold it to stretch the shape
	out from where you clicked.  Some shapes can change proportion
	(e.g., rectangle and oval), others cannot (e.g., square and circle).

	Let go of the mouse when you're done stretching.

        Normal Mode
        -----------
          Now you can move the mouse around the canvas to rotate the shape.

	  Click the mouse button again and the shape will be drawn in the
	  current color.

        Simple Shapes Mode
        ------------------
          If simple shapes are enabled ("--simpleshapes" option),
          the shape will be drawn on the canvas when you let go of the
          mouse button.  (There's no rotation step.)


      Text
      ----
        Choose a font and a color.  Click on the screen and a cursor will
	appear.  Type text and it will show up on the screen.
	
	Push [Enter] or [Return] and the text will be drawn onto the picture
	and the cursor will move down one line.
	
	Click elsewhere in the picture and the text will move there.


      Magic (Special Effects)
      -----------------------
        The magic tool is actually a set of special tools.  Select one of
	the "magic" effects from the selector on the right, and then
	click and drag around the picture to apply the effect.

	Mirror
	------
	  When you click the mouse in your picture with the "Mirror"
	  magic effect selected, the entire image will be reversed,
	  turning it into a mirror image.
	  
	Flip
	----
	  Similar to "Mirror."  Click and the entire image will be turned
	  upside-down.
	  
	Blur
	----
	  This makes the picture fuzzy wherever you drag the mouse.

	Blocks
	------
	  This makes the picture blocky looking ("pixelated") wherever
	  you drag the mouse.

	Negative
	--------
	  This inverts the colors wherever you drag the mouse.
	  (e.g., white becomes black, and vice versa.)

	Fade
	----
	  This fades the colors wherever you drag the mouse.
	  (Do it to the same spot many times, and it will eventually become
	  white.)
	
	Rainbow
	-------
	  This is similar to the paint brush, but as you move the mouse
	  around, it goes through all of the colors in the rainbow.

	Sparkles
	--------
	  This draws glowing yellow sparkles on the picture.

	Chalk
	-----
	  This makes parts of the picture (where you move the mouse)
	  look like a chalk drawing.
	
	Drip
	----
	  This makes the paint "drip" wherever you move the mouse.
	 
        Thick
	-----
	  This makes the darker colors in the picture become thicker
	  wherever you drag the mouse.

	Thin
	----
	  Similar to "Thick," except dark colors become thinner
	  (light colors become thicker).
	
	Fill
	----
	  This floods the picture with a color.  It lets you quickly
	  fill parts of the picture, as if it were a coloring book.
	
	
      Eraser
      ------
        This tool is similar to the Paint Brush.  Wherever you click
        (or click and drag), the picture will be erased to white.

        As you move the mouse around, a very large square outline follows
        the pointer, showing what part of the picture will be erased to white.

        As you erase, a "squeaky clean" eraser/wiping sound is played.


    Other Controls
    --------------
      Undo
      ----
        Clicking this tool will undo the last drawing action.  You can
        undo more than once.
	
	Note: You can also press [Control]-[Z] on the keyboard.


      Redo
      ----
        Clicking this tool will redo the drawing action you just "undid."
        As long as you don't draw again, you can redo as many times as you
        had "undone."
	
	Note: You can also press [Control]-[R] on the keyboard.


      New
      ----
        Clicking the "New" button will start a new drawing.
        You will first be asked whether you really want to do this.

	Note: You can also press [Control]-[N] on the keyboard.


      Open
      ----
        This shows you a list of all of the pictures you've saved.
	If there are more than can fit on the screen, use the "Up"
	and "Down" arrows at the top and bottom of the list to scroll
	through the list of pictures.

	Click a picture to select it, then...

	  * Click the green "Open" button at the lower left of the list to
	    load the selected picture.

	    Alternatively, you can double-click the picture's icon
	    (within 1 second) to load it.

	  * Click the brown "Erase" (trash can) button at the lower right of the
	    list to erase the selected picture.  (You will be asked to confirm.)

	  * Or click the red "Back" arrow button at the lower right of the list
	    to cancel and return to the picture you were drawing.

	If choose to open a picture, and your current drawing hasn't been
	saved, you will be prompted as to whether you want to save it or not.
	(See "Save," below.)
	
	Note: You can also press [Control]-[O] on the keyboard to get the
	'Open' dialog.


      Save
      ----
        This saves your current picture.
	
	If you haven't saved it before, it will create a new entry in
	the list of saved images.  (i.e., it will create a new file)

	Note: It won't ask you anything (e.g., for a filename).
	It will simply save the picture, and play a "camera shutter" sound
	effect.

	If you HAVE saved the picture before, or this is a picture you
	just loaded using the "Open" command, you will first be asked
	whether you want to save over the old version, or create a new
	entry (a new file).
	
	Note: You can also press [Control]-[S] on the keyboard.


      Print
      -----
        [ Note: Printing only works under Linux and Unix at the moment,
	  and requires the NetPBM tools.  See docs/INSTALL.txt ]


        Click this button and your picture will be printed!


        Disabling Printing
	------------------
	  If the "noprint" option was set (either with "noprint=yes" in
	  Tux Paint's configuration file, or using "--noprint" on the
	  command-line), the "Print" button will be disabled.

	  See the "Options" section above.
	  

        Restricting Printing
	--------------------
	  If the "printdelay" option was used (either with
	  "printdelay=SECONDS" in the configuration file, or using
	  "--printdelay=SECONDS" on the command-line), you can only print
	  once every SECONDS seconds.

	  For example, if "printdelay=60", you can print only once a minute.

	  See the "Options" section above.

	
	Other Printing Options
	----------------------
	  The command used to print is actually a set of commands that
	  convert a PNG to a PostScript and send it to the printer:

	    pngtopnm | pnmtops | lpr

	  This command can be changed by setting the "printcommand" value
	  in Tux Paint's configuration file.

	  See the "Options" section above.


      Quit
      ----
        Clicking the "Quit" button, closing the Tux Paint window, or
        pushing the "Escape" key will quit Tux Paint.

	NOTE: The "Quit" button can be disabled (with the "--noquit"
	command-line option), but the "Escape" key will still work.
	
        You will first be prompted as to whether you really want to quit.

        If you choose to quit, and you haven't saved the current picture,
	you will first be asked if wish to save it.  If it's not a new image,
	you will then be asked if you want to save over the old version,
	or create a new entry.	(See "Save" above.)
        
	NOTE: If the image is saved, it will be reloaded automatically
	the next time you run Tux Paint!


Loading Other Pictures into Tux Paint
-------------------------------------
  Since Tux Paint's 'Open' dialog only displays pictures you created with
  Tux Paint, what if you want to load some other picture or photograph
  into Tux Paint to edit?

  To do so, you simply need to convert the picture into
  a PNG (Portable Network Graphic) image file, and place it in Tux Paint's
  "saved" directory.  ("~/.tuxpaint/saved/" under Linux and Unix,
  "userdata\saved\" under Windows.)
  

  Using 'tuxpaint-import'
  -----------------------
    Linux and Unix users can use the "tuxpaint-import" shell script which
    gets installed when you install Tux Paint.  It uses some NetPBM tools
    to convert the image ("anytopnm"), resize it so that it will fit in
    Tux Paint's canvas ("pnmscale"), and convert it to a PNG ("pnmtopng").

    It also uses "date" to get the current time and date, which is the
    filenaming convention Tux Paint uses for saved files.  (Remember, you
    are never asked for a 'filename' when you go to Save or Open pictures!)

    To use 'tuxpaint-import', simply run the command from a command-line prompt
    and provide it the name(s) of the file(s) you wish to convert.

    They will be converted and placed in your Tux Paint 'saved' directory.
    (Note: If you're doing this for a different user - e.g., your child,
    you'll need to make sure to run the command under their account.)

    Example:

      $ tuxpaint-import grandma.jpg
      grandma.jpg -> /home/username/.tuxpaint/saved/20020921123456.png
      jpegtopnm: WRITING A PPM FILE

    The first line ("tuxpaint-import grandma.jpg") is the command to run.
    The following two lines are output from the program while it's working.
    

    Now you can load Tux Paint, and a version of that original picture will
    be available under the 'Open' dialog.  Just double-click its icon!


  Doing it Manually
  -----------------
    Windows users must currently do the conversion manually.

    Load a graphics program that is capable of both loading your picture
    and saving a PNG format file.  (See "PNG.txt" for a list of suggested
    software.)

    Reduce the size of the image to no wider than 448 pixels across and
    no taller than 376 pixels tall.  (e.g., maximum size is 448 x 376 pixels)

    Save the picture in PNG format.  It is HIGHLY recommended that you
    name the filename using the current date and time, since that's
    the convention Tux Paint uses:

      YYYYMMDDhhmmss

    e.g.:

      20020921130500 - for September 21, 2002, 1:05:00pm

    Place this PNG file in your Tux Paint 'saved' directory.  (See above.)

    Under Windows, this is in the "userdata" folder.


Extending Tux Paint
-------------------
  If you wish to add or change things like Brushes and Rubber Stamps
  used by Tux Paint, you can do it fairly easily by simply putting
  or removing files on your hard disk.
  
  Note: You'll need to restart Tux Paint for the changes to take effect.

  
  Where Files Go
  --------------
    Standard Files
    --------------
      Tux Paint looks for its various data files in its data directory.

      Linux and Unix
      --------------
        Where this directory goes depends on what value was set for
        "DATA_PREFIX" when Tux Paint was built.  See INSTALL.txt for details.

        By default, though, the directory is:

          /usr/local/share/tuxpaint/

      Windows
      -------
        Where this directory goes depends on what folder you told the
        installer to put Tux Paint in.

        [ What's the default? ]


    Personal Files
    --------------
      You can also create brushes, stamps and fonts in your own directory
      for Tux Paint to find.

      Linux and Unix
      --------------
        Your personal Tux Paint directory is "~/.tuxpaint/".
	
	That is, if your home directory is "/home/karl", then
	your Tux Paint directory is "/home/karl/.tuxpaint/".
	
	Don't forget the period (".") before the word 'tuxpaint'!

      Windows
      -------
        Your personal Tux Paint directory is named "userdata".

	[ Where is it now? ]

    
      To add brushes, stamps and fonts, create subdirectories under
      your personal Tux Paint directory named "brushes", "stamps" and "fonts",
      respectively.

      (For example, if you created a brush named "flower.png", you
      would put it in "~/.tuxpaint/brushes/" under Linux or Unix.)


  Brushes
  -------
    The brushes used for drawing with the Brush and Lines tools in
    Tux Paint are simply greyscale PNG images.
    
    The alpha (transparency) of the PNG image is used to determine the shape
    of the brush, which means that the shape can be 'anti-aliased' and even
    partially-transparent!

    Brush images should be no wider than 40 pixels across and
    no taller than 40 pixels high.

    Just place them in the "brushes" directory.


  Stamps
  ------
    All stamp-related files go in the "stamps" directory.
    It's useful to create subdirectories and sub-subdirectories
    there to organize the stamps.  (For example, you can have a
    "holidays" folder with "halloween" and "christmas" subfolders.)
    
    Images
    ------
      Rubber Stamps in Tux Paint can be made up of a number of separate
      files.  The one file that is required is, of course, the picture itself.
   
      The Stamps used by Tux Paint are PNG pictures.  They can be full-color
      or greyscale.  The alpha (transparency) of the PNG is used to determine
      the actual shape of the picture (otherwise you'll stamp a large
      rectangle on your drawings).

      The PNGs can be any size, but in practice, a 100 pixels wide by
      100 pixels tall (100x100) is quite large for Tux Paint.


    Description Text
    ----------------
      Text (".TXT") files with the same name as the PNG.
      (e.g., "picture.png"'s description is stored in "picture.txt" in the
      same directory.)

      Lines beginning with "xx=" (where "xx" is one of the languages
      supported; e.g., "de" for German, "fr" for French, etc.) will be
      used under the various locales supported.

      If no translation is available for the user's locale, the default
      string (the first line, which should be in English) is used.


    Sound Effects
    -------------
      WAVE (".WAV") files with the same name as the PNG.
      (e.g., "picture.png"'s sound effect is the sound "picture.wav" in the
      same directory.)

      For sounds for different locales (e.g., if the sound is someone saying
      a word, and you want translated versions of the word said),
      also create WAV files with the locale's label in the filename, in
      the form: "STAMP_LOCALE.wav."

      "picture.png"'s sound effect, when Tux Paint is run in Spanish mode,
      would be "picture_es.wav".  In French mode, "picture_fr.wav".  And so on.

      If no localized sound effect can be loaded, Tux Paint will attempt to
      load the 'default' sound file.  (e.g., "picture.wav")


    Stamp Options
    -------------
      Aside from a graphical shape, a textual description, and a sound effect,
      stamps can also be given other attributes.  To do this, you need
      to create a 'data file' for the stamp.
      
      A stamp data file is simply a text file containing the options.
      
      The file has the same name as the PNG image, but a ".dat" extension.
      (e.g., "picture.png"'s data file is the text file "picture.dat" in the
      same directory.)

      Colored Stamps
      --------------
        Stamps can be made to be either "colorable" or "tintable."
      
        Colorable
        ---------
          "Colorable" stamps they work much like brushes - you pick the stamp
          to get the shape, and then pick the color you want it to be.
          (Symbol stamps, like the mathematical and musical ones, are an
	  example.)

          Nothing about the original image is used except the transparency
          ("alpha" channel).  The color of the stamp comes out solid.
        
	  Add the word "colorable" to the stamp's data file.

        Tinted
        ------
          "Tinted" stamps are similar to "colorable" ones, except the
	  details of the original image are kept.  (To put it techically,
	  the original image is used, but its hue is changed, based on the
	  currently-selected color.)

          Add the word "tintable" to the stamp's data file.


  Fonts
  -----
    The fonts used by Tux Paint are TrueType Fonts (TTF).

    Simply place them in the "fonts" directory.  Tux Paint will load the
    font and provide four different sizes in the 'Font Selector' when
    using the 'Text' tool.

