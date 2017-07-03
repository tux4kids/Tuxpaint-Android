                     Creating Tux Paint Magic Tool Plugins

                Copyright 2007-2008 by Bill Kendrick and others
                               New Breed Software

                           bill@newbreedsoftware.com
                            http://www.tuxpaint.org/

                          July 5, 2007 - July 19, 2008

     ----------------------------------------------------------------------

Overview

     Beginning with version 0.9.18, Tux Paint's 'Magic' tools were converted
     from routines that lived within the application itself, to a set of
     'plugins' that are loaded when Tux Paint starts up.

     This division allows more rapid development of 'Magic' tools, and allows
     programmers to create and test new tools without needing to integrate
     them within the main Tux Paint source code. (Users of more professional
     graphics tools, such as The GIMP, should be familiar with this plugin
     concept.)

     ----------------------------------------------------------------------

Table of Contents

     * Prequisites
     * Interfaces
          * 'Magic' tool plugin functions
               * Common arguments to plugin functions
               * Required Plugin Functions
                    * Plugin "housekeeping" functions
                    * Plugin event functions
          * Tux Paint Functions and Data
               * Pixel Manipulations
               * Helper Functions
               * Informational
               * Tux Paint System Calls
               * Color Conversions
          * Helper Macros in "tp_magic_api.h"
          * Constant Definitions in "tp_magic_api.h"
     * Compiling
          * Linux and other Unix-like Platforms
          * Windows
          * Mac OS X
     * Installing
          * Linux and other Unix-like Platforms
          * Windows
          * Mac OS X
     * Creating plugins with multiple effects
     * Example Code
     * Getting Help
     * Glossary

     ----------------------------------------------------------------------

Prerequisites

     Tux Paint is written in the C programming language, and uses the
     Simple DirectMedia Layer library ('libSDL', or simply 'SDL'; available
     from http://www.libsdl.org/). Therefore, for the moment at least, one
     must understand the C language and how to compile C-based programs.
     Familiarity with the SDL API is highly recommended, but some basic SDL
     concepts will be covered in this document.

     ----------------------------------------------------------------------

Interfaces

     Those who create 'Magic' tool plugins for Tux Paint must provide some
     interfaces (C functions) that Tux Paint may invoke.

     Tux Paint utilizes SDL's "SDL_LoadObject()" and "SDL_LoadFunction()"
     routines to load plugins (shared objects files; e.g., ".so" files on
     Linux or ".dll" files on Windows) and find the functions within.

     In turn, Tux Paint provides a number of helper functions that the plugin
     may (or sometimes is required to) use. This is exposed as a C structure
     (or "struct") which contains pointers to functions and other data inside
     Tux Paint. A pointer to this structure gets passed along to the plugin's
     functions as an argument when Tux Paint invokes them.

     Plugins should #include the C header file "tp_magic_api.h", which
     exposes the 'Magic' tool plugin API. Also, when you run the C compiler
     to build a plugin, you should use the command-line tool
     "tp-magic-config" to get the appropriate compiler flags (such as where
     the compiler can find the Tux Paint plugin header file, as well as SDL's
     header files) for building a plugin. (See "Compiling", below.)

     The C header file and command-line tool mentioned above are included
     with Tux Paint - or in some cases, as part of a "Tux Paint 'Magic' Tool
     Plugin Development package".

  'Magic' tool plugin functions

       'Magic' tool plugins must contain the functions listed below. Note: To
       avoid 'namespace' collisions, each function's name must start with the
       shared object's filename (e.g., "blur.so" or "blur.dll" would have
       functions whose names begin with "blur_"). This includes private
       functions (ones not used by Tux Paint directly), unless you declare
       those as 'static'.

    Common arguments to plugin functions:

       Here is a description of arguments that many of your plugin's
       functions will need to accept.
         * magic_api * api
           Pointer to a C structure containing pointers to Tux Paint
           functions and other data that the plugin can (and sometimes
           should) use. The contents of this struct are described below.

           Note: The magic_api struct is defined in the C header file
           "tp_magic_api.h", which you should include at the top of your
           plugin's C source file:

             #include "tp_magic_api.h"

         * int which
           An index the plugin should use to differentiate different 'Magic'
           tools, if the plugin provides more than one. (If not, "which" will
           always be 0.) See "Creating plugins with multiple effects", below.

         * SDL_Surface * snapshot
           A snapshot of the previous Tux Paint canvas, taken when the the
           mouse was first clicked to activate the current magic tool. If you
           don't continuously affect the image during one hold of the mouse
           button, you should base your effects off the contents of this
           canvas. (That is, read from "snapshot" and write to "canvas",
           below.)

         * SDL_Surface * canvas
           The current Tux Paint drawing canvas. Your magical effects should
           end up here!

         * SDL_Rect * update_rect
           A pointer to an SDL 'rectangle' structure that you use to tell
           Tux Paint what part of the canvas has been updated. If your effect
           affects a 32x32 area centered around the mouse pointer, you would
           fill the SDL_Rect as follows:

             update_rect->x = x - 16;
             update_rect->y = y - 16;
             update_rect->w = 32;
             update_rect->h = 32;

           Or, if your effect changes the entire canvas (e.g., flips it
           upside-down), you'd fill it as follows:

             update_rect->x = 0;
             update_rect->y = 0;
             update_rect->w = canvas->w;
             update_rect->h = canvas->h;

           Note: "update_rect" is a C pointer (an "SDL_Rect *" rather than
           just an "SDL_Rect") because you need to fill in its contents.
           Because it is a pointer, you access its elements via "->" (arrow)
           rather than "." (dot).

    Required Plugin Functions:

         Your plugin is required to contain, at the least, all of the
         following functions.

         Note: Remember, your plugin's function names must be preceded by
         your plugin's filename. That is, if your plugin is called "zoom.so"
         (on Linux) or "zoom.dll" (on Windows), then the names of your
         functions must begin with "zoom_" (e.g., "zoom_get_name(...)").

      Plugin "housekeeping" functions:

           * Uint32 api_version(void)
             The plugin should return an integer value representing the
             version of the Tux Paint 'Magic' tool plugin API the plugin was
             built against. The safest thing to do is return the value of
             TP_MAGIC_API_VERSION, which is defined in "tp_magic_api.h". If
             Tux Paint deems your plugin to be compatible, it will go ahead
             and use it.

             Note: Called once by Tux Paint, at startup. It is called first.

           * int init(magic_api * api)
             The plugin should do any initialization here. Return '1' if
             initialization was successful, or '0' if not (and Tux Paint will
             not present any 'Magic' tools from the plugin).

             Note: Called once by Tux Paint, at startup. It is called first.
             It is called after "api_version()", if Tux Paint believes your
             plugin to be compatible.

           * int get_tool_count(magic_api * api)
             This should return the number of Magic tools this plugin
             provides to Tux Paint.

             Note: Called once by Tux Paint, at startup. It is called after
             your "init()", if it succeeded.

           * char * get_name(magic_api * api, int which)
             This should return a string containing the name of a magic tool.
             This will appear on the button in the 'Magic' selector within
             Tux Paint.

             Tux Paint will free() the string upon exit, so you should wrap
             it in a C strdup() call.

             Note: Called once for each Magic tool your plugin claims to
             contain (by your "get_tool_count()").

           * SDL_Surface * get_icon(magic_api * api, int which)
             This should return an SDL_Surface containing the icon
             representing the tool. (A greyscale image with alpha, no larger
             than 40x40.) This will appear on the button in the 'Magic'
             selector within Tux Paint.

             Tux Paint will free ("SDL_FreeSurface()") the surface upon exit.

             Note: Called once for each Magic tool your plugin claims to
             contain (by your "get_tool_count()").

           * char * get_description(magic_api * api, int which, int mode)
             This should return a string containing the description of how to
             use a particular magic tool. This will appear as a help tip,
             explained by Tux the Penguin, within Tux Paint.

             Tux Paint will free() the string upon exit, so you should wrap
             it in a C strdup() call.

             Note: For each Magic tool your plugin claims to contain
             (reported by your "get_tool_count()" function), this function
             will be called for each mode the tool claims to support
             (reported by your "modes()" function).

             In other words, if your plugin contains two tools, one which
             works in paint mode only, and the other that works in both paint
             mode and full-image mode, your plugin's "get_description()" will
             be called three times.

           * int requires_colors(magic_api * api, int which)
             Return a '1' if the 'Magic' tool accepts colors (the 'Colors'
             palette in Tux Paint will be available), or '0' if not.

             Note: Called once for each Magic tool your plugin claims to
             contain (by your "get_tool_count()").

           * int modes(magic_api * api, int which)
             This lets you tell Tux Paint what modes your tool can be used in
             (either as a tool the user can paint with, or a tool that
             affects the entire drawing at once)

             You must return a value that's some combination of one or more
             of available modes:
                * MODE_PAINT
                * MODE_FULLSCREEN
             e.g., if your tool is only one that the user can paint with,
             return "MODE_PAINT". If the user can do both, return
             "MODE_PAINT | MODE_FULLSCREEN" to tell Tux Paint it can do both.

             Note: Called once for each Magic tool your plugin claims to
             contain (by your "get_tool_count()").

             Note: Added to Tux Paint 0.9.21; Magic API version '0x00000002'

           * void shutdown(magic_api * api)
             The plugin should do any cleanup here. If you allocated any
             memory or used SDL_Mixer to load any sounds during init(), for
             example, you should free() the allocated memory and
             Mix_FreeChunk() the sounds here.

             Note: This function is called once, when Tux Paint exits.

      Plugin event functions:

           * void switchin(magic_api * api, int which, int mode,
             SDL_Surface * snapshot, SDL_Surface * canvas)
             void switchout(magic_api * api, int which, int mode,
             SDL_Surface * snapshot, SDL_Surface * canvas)
             switchin() is called whenever one of the plugin's Magic tools
             becomes active, and switchout() is called whenever one becomes
             inactive. This can be because the user just clicked a specific
             Magic tool (the current one is switched-out, and a new one is
             switched-in).

             It can also happen when user leaves/returns from the selection
             of "Magic" tools when doing some other activity (i.e., using a
             different tool, such as "Text" or "Brush", activating a
             momentary tool, such as "Undo" and "Redo", or returning from a
             dialog - possibly with a new picture when it switches back -
             such as "Open", "New" or "Quit"). In this case, the same Magic
             tool is first 'switched-out', and then 'switched-back-in',
             usually moments later.

             Finally, it can also happen when the user changes the 'mode' of
             a tool (i.e., from paint mode to full-image mode). First
             switchout() is called for the old mode, then switchin() is
             called for the new mode.

             These functions allow users to interact in complicated ways with
             Magic tools (for example, a tool that lets the user draw
             multiple freehand strokes, and then uses that as input such as
             handwriting - normally, the user could click somewhere in the
             canvas to tell the Magic tool they are 'finished', but if they
             switch to another tool, the Magic tool may want to undo any
             temporary changes to the canvas).

             These functions could also be used to streamline certain
             effects; a behind-the-scenes copy of the entire canvas could be
             altered in some way when the user first switches to the canvas,
             and then pieces of that copy could be drawn on the canvas when
             they draw with the Magic tool.
             Note: Added to Tux Paint 0.9.21; Magic API version '0x00000002'

           * void set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 g)
             Tux Paint will call this function to inform the plugin of the
             RGB values of the currently-selected color in Tux Paint's
             'Colors' palette. (It will be called whenever one of the
             plugin's Magic tools that accept colors becomes active, and
             whenever the user picks a new color while such a tool is
             currently active.)

           * void click(magic_api * api, int which, int mode,
             SDL_Surface * snapshot, SDL_Surface * canvas, int x, int y,
             SDL_Rect * update_rect)
             The plugin should apply the appropriate 'Magic' tool on the
             'canvas' surface. The (x,y) coordinates are where the mouse was
             (within the canvas) when the mouse button was clicked, and you
             are told which 'mode' your tool is in (i.e., 'MODE_PAINT' or
             'MODE_FULLSCREEN).

             The plugin should report back what part of the canvas was
             affected, by filling in the (x,y) and (w,h) elements of
             'update_rect'.

             The contents of the drawing canvas immediately prior to the
             mouse button click is stored within the 'snapshot' canvas.

           * void drag(magic_api * api, int which, SDL_Surface * snapshot,
             SDL_Surface * canvas, int ox, int oy, int x, int y,
             SDL_Rect * update_rect)
             The plugin should apply the appropriate 'Magic' tool on the
             'canvas' surface. The (ox,oy) and (x,y) coordinates are the
             location of the mouse at the beginning and end of the stroke.

             Typically, plugins that let the user "draw" effects onto the
             canvas utilize Tux Paint's "line()" 'Magic' tool plugin helper
             function to calculate the points of the line between (ox,oy) and
             (x,y), and call another function within the plugin to apply the
             effect at each point. (See "Tux Paint Functions and Data,"
             below).

             The plugin should report back what part of the canvas was
             affected, by filling in the (x,y) and (w,h) elements of
             'update_rect'.

             Note: The contents of the drawing canvas immediately prior to
             the mouse button click remains as it was (when the plugin's
             "click()" function was called), and is still available in the
             'snapshot' canvas.

           * void release(magic_api * api, int which, SDL_Surface * snapshot,
             SDL_Surface * canvas, int x, int y, SDL_Rect * update_rect)
             The plugin should apply the appropriate 'Magic' tool on the
             'canvas' surface. The (x,y) coordinates are where the mouse was
             (within the canvas) when the mouse button was released.

             The plugin should report back what part of the canvas was
             affected, by filling in the (x,y) and (w,h) elements of
             'update_rect'.

             Note: The contents of the drawing canvas immediately prior to
             the mouse button click remains as it was (when the plugin's
             "click()" function was called), and is still available in the
             'snapshot' canvas.

  Tux Paint Functions and Data

       Tux Paint provides a number of helper functions that plugins may
       access via the "magic_api" structure, sent to all of the plugin's
       functions. (See "Required Plugin Functions," above.)

    Pixel Manipulations

           * Uint32 getpixel(SDL_Surface * surf, int x, int y)
             Retreives the pixel value from the (x,y) coordinates of an
             SDL_Surface. (You can use SDL's "SDL_GetRGB()" function to
             convert the Uint32 'pixel' to a set of Uint8 RGB values.)

           * void putpixel(SDL_Surface * surf, int x, int y, Uint32 pixel)
             Sets the pixel value at position (x,y) of an SDL_Surface. (You
             can use SDL's "SDL_MapRGB()" function to convert a set of Uint8
             RGB values to a Uint32 'pixel' value appropriate to the
             destination surface.)

           * SDL_Surface * scale(SDL_Surface * surf, int w, int h,
             int keep_aspect)
             This accepts an existing SDL surface and creates a new one
             scaled to an arbitrary size. (The original surface remains
             untouched.)

             The "keep_aspect" flag can be set to '1' to force the new
             surface to stay the same shape (aspect ratio) as the original,
             meaning it may not be the same width and height you requested.
             (Check the "->w" and "->h" elements of the output
             "SDL_Surface *" to determine the actual size.)

    Helper Functions

           * int in_circle(int x, int y, int radius)
             Returns '1' if the (x,y) location is within a circle of a
             particular radius (centered around the origin: (0,0)). Returns
             '0' otherwise. Useful to create 'Magic' tools that affect the
             canvas with a circular brush shape.

           * void line(void * api, int which, SDL_Surface * canvas,
             SDL_Surface * snapshot, int x1, int y1, int x2, int y2,
             int step, FUNC callback)
             This function calculates all points on a line between the
             coordinates (x1,y1) and (x2,y2). Every 'step' iterations, it
             calls the 'callback' function.

             It sends the 'callback' function the (x,y) coordinates on the
             line, Tux Paint's "magic_api" struct (as a "void *" pointer
             which you need to send to it), a 'which' value, represening
             which of the plugin's 'Magic' tool is being used, and the
             current and snapshot canvases.

             Example prototype of a callback function that may be sent to
             Tux Paint's "line()" 'Magic' tool plugin helper function:

               void exampleCallBack(void * ptr_to_api, int which_tool,
               SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y);

             Example use of the "line()" helper (e.g., within a plugin's
             draw() function):

               api->line((void *) api, which, canvas, snapshot, ox, oy, x, y,
               1, exampleCallBack);

           * Uint8 touched(int x, int y)
             This function allows you to avoid re-processing the same pixels
             multiple times when the user drags the mouse across an area of
             the canvas, thus increasing Tux Paint's response time,
             especially with math-heavy effects.

             If your effect's "click()", "drag()" and/or "release()"
             functions take the contents of the source surface ("snapshot")
             and always create the same results in the desintation surface
             ("canvas"), you should wrap the effect in a call to
             "api->touched()".

             This function simply returns whether or not it had already been
             called for the same (x,y) coordinates, since the user first
             clicked the mouse. In other words, the first time you call it
             for a particular (x,y) coordinate, it returns '0'. Future calls
             will return '1' until the user releases the mouse button.

             Note: Magic effects that continuously affect the destination
             surface ("canvas") (ignoring the "snapshot surface) have no
             reason to use this function. The "Blur" and "Smudge" tools that
             ship with Tux Paint are examples of such effects.

    Informational

           * char * tp_version
             A string containing the version of Tux Paint that's running
             (e.g., "0.9.18").

           * int canvas_w Returns the width of the drawing canvas.

           * int canvas_h Returns the height of the drawing canvas.

           * int button_down(void)
             A '1' is returned if the mouse button is down; '0' otherwise.

           * char * data_directory
             This string contains the directory where Tux Paint's data files
             are stored. For example, on Linux, this may be
             "/usr/share/tuxpaint/".

             Magic tools should include an icon (see "get_icon()", above) and
             are encouraged to include sound effects, it's useful for plugins
             to know where such things are located.

             When compiling and installing a plugin, the "tp-magic-config"
             command-line tool should be used to determine where such data
             should be placed for the installed version of Tux Paint to find
             them. (See "Installing," below.)

             Note: If your plugin is installed locally (e.g., in your
             "~/.tuxpaint/plugins/" directory), rather than globally
             (system-wide), the "data_directory" value will be different.
             (e.g., "/home/username/.tuxpaint/plugins/data/").

    Tux Paint System Calls

           * void update_progress_bar(void)
             Asks Tux Paint to animate and draw one frame of its progress bar
             (at the bottom of the screen). Useful for routines that may take
             a long time, to provide feedback to the user that Tux Paint has
             not crashed or frozen.

           * void playsound(Mix_Chunk * snd, int pan, int dist)
             This function plays a sound (one loaded by the SDL helper
             library "SDL_mixer"). It uses SDL_mixer's "Mix_SetPanning()" to
             set the volume of the sound on the left and right speakers,
             based on the 'pan' and 'dist' values sent to it.

             A 'pan' of 128 causes the sound to be played at equal volume on
             the left and right speakers. A 'pan' of 0 causes it to be played
             completely on the left, and 255 completely on the right.

             The 'dist' value affects overall volume. 255 is loudest, and 0
             is silent.

             The 'pan' and 'dist' values can be used to simulate location and
             distance of the 'Magic' tool effect.

           * void stopsound(void)
             This function stops playing a sound played by playsound(). It is
             useful to silence effects when the user stops using the tool (in
             your 'release' function).
           * void special_notify(int flag)
             This function notifies Tux Paint of special events. Various
             values defined in "tp_magic_api.h" can be 'or'ed together (using
             C's boolean 'or': "|") and sent to this function.
                * SPECIAL_FLIP - The contents of the canvas has been flipped
                  vertically.

                  If a 'Starter' image was used as the basis of this image,
                  it should be flipped too, and a record of the flip should
                  be stored as part of Tux Paint's undo buffer stack.
                  Additionally, the fact that the starter has been flipped
                  (or unflipped) should be recorded on disk when the current
                  drawing is saved.

                * SPECIAL_MIRROR - Similar to SPECIAL_FLIP, but for magic
                  tools that mirror the contents of the canvas horizontally.

    Color Conversions

           * float sRGB_to_linear(Uint8 srbg)
             Converts an 8-bit sRGB value (one between 0 and 255) to a linear
             floating point value (between 0.0 and 1.0).

             See also: sRGB article at Wikipedia.

           * uint8 linear_to_sRGB(float linear)
             Converts a linear floating point value (one between 0.0 and 1.0)
             to an 8-bit sRGB value (between 0 and 255).

           * void rgbtohsv(Uint8 r, Uint8 g, Uint8 b, float * h, float * s,
             float * v)
             Converts 8-bit sRGB values (between 0 and 255) to floating-point
             HSV (Hue, Saturation and Value) values (Hue between 0.0 and
             360.0, and Saturation and Value between 0.0 and 1.0).

             See also: HSV Color Space article at Wikipedia.

           * void hsvtorgb(float h, float s, float v, Uint8 * r, Uint8 * g,
             Uint8 * b)
             Converts floating-point HSV (Hue, Saturation and Value) values
             (Hue between 0.0 and 360.0, and Saturation and Value between 0.0
             and 1.0) to 8-bit sRGB values (between 0 and 255).

  Helper Macros in "tp_magic_api.h":

       Along with the "magic_api" C structure containing functions and data
       described above, the tp_magic_api.h C header file also contains some
       helper macros that you may use.

         * min(x, y)
           The minimum of 'x' and 'y'. (That is, if 'x' is less than or equal
           to 'y', then the value of 'x' will be used. If 'y' is less than
           'x', it will be used.)

         * max(x, y)
           The maximum of 'x' and 'y'. The opposite of min().

         * clamp(lo, value, hi)
           A value, clamped to be no smaller than 'lo', and no higher than
           'hi'. (That is, if 'value' is less than 'lo', then 'lo' will be
           used; if 'value' is greater than 'hi', then 'hi' will be used;
           otherwise, 'value' will be used.)

           Example: red = clamp(0, n, 255);
           Tries to set 'red' to be the value of 'n', but without allowing it
           to become less than 0 or greater than 255.

           Note: This macro is simply a #define of:
           "(min(max(value,lo),hi))".

  Constant Defintions in "tp_magic_api.h":

       The following is a summary of constant values that are set
       (via "#define") within the 'Magic' tool API header file.

         * TP_MAGIC_API_VERSION
           This integer value represents which version of the Tux Paint
           'Magic' tool API the header corresponds to.

           It should be referenced by your magic tool's "api_version()"
           function, to inform the running copy of Tux Paint whether or not
           your plugin is compatible.

           Note: This version number does not correspond to Tux Paint's own
           release number (e.g., "0.9.18"). The API will not change every
           time a new version of Tux Paint is released, which means plugins
           compiled for earlier versions of Tux Paint will often run under
           newer versions.

         * SPECIAL_MIRROR
           SPECIAL_FLIP
           These are flags for Tux Paint's "special_notify()" helper
           function. They are described above.

     ----------------------------------------------------------------------

Compiling

  Linux and other Unix-like Platforms

       Use the C compiler's "-shared" command-line option to generate a
       shared object file (".so") based on your 'Magic' tool plugin's C
       source code.

       Use the "tp-magic-config --cflags" command, supplied as part of
       Tux Paint - or in some cases, as part of a "Tux Paint 'Magic' Tool
       Plugin Development package" - to provide additional command-line flags
       to your C compiler that will help it build your plugin.

    Command-Line Example

         As a stand-alone command, using the GNU C Compiler and BASH shell,
         for example:

           $ gcc -shared `tp-magic-config --cflags` my_plugin.c -o
           my_plugin.so

         Note: The characters around the "tp-magic-config" command are a
         grave/backtick/backquote ("`"), and not an apostrophe/single-quote
         ("'"). They tell the shell to execute the command within (in this
         case, "tp-magic-config ..."), and use its output as an argument to
         the command being executed (in this case, "gcc ...").

    Makefile Example

         A snippet from a Makefile to compile a Tux Paint "Magic" tool plugin
         might look like this:

           +------------------------------------------------------+
           | CFLAGS=-Wall -O2 $(shell tp-magic-config --cflags)   |
           |                                                      |
           | my_plugin.so: my_plugin.c                            |
           |    gcc -shared $(CFLAGS) -o my_plugin.so my_plugin.c |
           +------------------------------------------------------+

         The first line sets up Makefile variable ("CFLAGS") that contains
         flags for the compiler. "-Wall" asks for all compiler warnings to be
         shown. "-O2" asks for level 2 optimization.
         "($shell tp-magic-config --cflags)" runs "tp-magic-config" to
         retrieve additional compiler flags that "Magic" tool plugins
         require. (The "$(shell ...)" directive is similar to the ` ("grave")
         character in the BASH shell examples, above.)

         The next line defines a Makefile target, "my_plugin.so", and states
         that it depends on the C source file "my_plugin.c". (Any time the C
         file changes, "make" will know to recompile it and produce an
         updated ".so" file. If the C file hadn't changed, it won't bother
         recompiling.)

         The last line defines the command "make" should run when it
         determines that it needs to (re)compile the ".so" file. Here, we're
         using "gcc", with "-shared and "$(CFLAGS)" command-line arguments,
         like above. "-o my_plugin.so" tells the C compiler that the output
         file should be "my_plugin.so". The last argument is the C file to
         compile, in this case "my_plugin.c".

         Note: Commands listed below a Makefile target should be intented
         using a single tab character.

    Advanced Makefile

         An even more generalized Makefile might look like this:

           +----------------------------------------------------+
           | CFLAGS=-Wall -O2 $(shell tp-magic-config --cflags) |
           |                                                    |
           | my_plugin_1.so: my_plugin_1.c                      |
           |    $(CC) -shared $(CFLAGS) -o $@ $<                |
           |                                                    |
           | my_plugin_2.so: my_plugin_2.c                      |
           |    $(CC) -shared $(CFLAGS) -o $@ $<                |
           +----------------------------------------------------+

         As before, there are lines that define the command "make" should run
         when it determines that it needs to (re)compile the ".so" file(s).
         However, more general terms are used...

         "$(CC)" gets expanded to your default C compiler (e.g., "gcc").
         "-shared and "$(CFLAGS)" are command-line arguments to the compiler,
         like above. "-o $@" tells the C compiler what the output file should
         be; "make" replaces "$@" with the name of the target, in this case
         "my_plugin_1.so" or "my_plugin_2.so". And finally, the last argument
         is the C file to compile; "make" replaces it with the target's
         dependency, in this case "my_plugin_1.c" or "my_plugin_2.c".

  Windows

       TBD

  Mac OS X

       TBD

     ----------------------------------------------------------------------

Installing

  Linux and other Unix-like Platforms

       Use the "tp-magic-config" command-line tool, supplied as part of
       Tux Paint - or in some cases, as part of a "Tux Paint 'Magic' Tool
       Plugin Development package" - to determine where your plugins' files
       should go.

    Shared Object

         Use "tp-magic-config --pluginprefix" to determine where the plugin
         shared object (".so") files should be installed. The value returned
         by this command will be the global location where the installed copy
         of Tux Paint looks for plugins (e.g., "/usr/lib/tuxpaint/plugins").

         Alternatively, you may use "tp-magic-config --localpluginprefix" to
         find out where Tux Paint expects to find local plugins for the
         current user (e.g., "/home/username/.tuxpaint/plugins").

         As stand-alone commands, using the BASH shell, for example:

           # cp my_plugin.so `tp-magic-config --pluginprefix`
           # chmod 644 `tp-magic-config --pluginprefix`/my_plugin.so

         Note: See the note above regarding the "`" (grave) character.

    Documentation

         Use the "tp-magic-config --plugindocprefix" command to determine
         where documentation for your "Magic" tools should go. The value
         returned by this command will be the location where the
         documentation to the installed copy of Tux Paint is stored. The main
         documentation includes a link to a folder where "Magic" tools'
         documentation is expected to be installed

         (e.g., "/usr/share/doc/tuxpaint/magic-docs").

         Note: It's best to include both HTML and plain-text versions of your
         documentation. An "html" subdirectory exists within the "magic-docs"
         directory, and is where the HTML versions should go.

         As stand-alone commands, using the BASH shell, for example:

           # cp my_plugin.html `tp-magic-config --plugindocprefix`/html
           # cp my_plugin.txt `tp-magic-config --plugindocprefix`

         Note: See the note above regarding the "`" (grave) character.

         Note: Currently, there is no "--localplugindocprefix" option.

    Icons, Sounds and other Data Files

         Use the "tp-magic-config --dataprefix" command, supplied as part of
         Tux Paint, to determine where data files (PNG icon, Ogg Vorbis sound
         effects, etc.) should be installed. The value returned by this
         command will be the same as the value of the "data_directory" string
         stored within the "magic_api" structure that your plugin's functions
         receive (e.g., "/usr/share/tuxpaint/").

         For locally-installed plugins (for the current user only), use
         "tp-magic-config --localdataprefix". It will return the value of
         "data_directory" string that locally-installed plugins will see
         within their "magic_api" structure (e.g.,
         "/home/username/.tuxpaint/plugins/data/").

         Note: Tux Paint's default Magic tool plugins install their data
         within "magic" subdirectories of Tux Paint's "images" and "sounds"
         data directories (e.g., "/usr/share/tuxpaint/images/magic/"). You
         are encouraged to do the same.

         As stand-alone commands, using the BASH shell, for example:

           # cp my_plugin_icon.png `tp-magic-config
           --dataprefix`/images/magic/
           # chmod 644 `tp-magic-config
           --dataprefix`/images/magic/my_plugin_icon.png

         Note: See the note above regarding the "`" (grave) character.

    Putting it Together in a Makefile

         A snippet from a more generalized Makefile might look like this:

           +------------------------------------------------------------+
           | PLUGINPREFIX=$(shell tp-magic-config --pluginprefix)       |
           | PLUGINDOCPREFIX=$(shell tp-magic-config --plugindocprefix) |
           | DATAPREFIX=$(shell tp-magic-config --dataprefix)           |
           |                                                            |
           | install:                                                   |
           |    #                                                       |
           |    # Install plugin                                        |
           |    mkdir -p $(PLUGINPREFIX)                                |
           |    cp *.so $(PLUGINPREFIX)/                                |
           |    chmod 644 $(PLUGINPREFIX)/*.so                          |
           |    #                                                       |
           |    # Install icons                                         |
           |    mkdir -p $(DATAPREFIX)/images/magic                     |
           |    cp icons/*.png $(DATAPREFIX)/images/magic/              |
           |    chmod 644 $(DATAPREFIX)/images/magic/*.png              |
           |    #                                                       |
           |    # Install sound effects                                 |
           |    mkdir -p $(DATAPREFIX)/sounds/magic                     |
           |    cp sounds/*.ogg $(DATAPREFIX)/sounds/magic/             |
           |    chmod 644 $(DATAPREFIX)/sounds/magic/*.ogg              |
           |    #                                                       |
           |    # Install docs                                          |
           |    mkdir -p $(PLUGINDOCPREFIX)/html                        |
           |    cp docs/*.html $(PLUGINDOCPREFIX)/html/                 |
           |    cp docs/*.txt $(PLUGINDOCPREFIX)/                       |
           |    chmod 644 $(PLUGINDOCPREFIX)/html/*.html                |
           |    chmod 644 $(PLUGINDOCPREFIX)/*.txt                      |
           +------------------------------------------------------------+

         The first three lines set up Makefile variables that contain the
         paths returned by the "tp-magic-config" command-line tool. (The
         "$(shell ...)" directive is similar to the ` ("grave") character in
         the BASH shell examples, above.)

         Below that is an "install" target in the Makefile. (Invoked by, for
         example, "$ sudo make install" or "# make install".)

         The "install" target uses "mkdir -p" to make sure that the plugin
         directory exists, then uses "cp" to copy all plugin (".so") files
         into it, and invokes "chmod" to make sure they are readable.

         It then does a similar series of commands to install icon files
         (".png" images) and sound effects (".ogg" files) into subdirectories
         within Tux Paint's data directory, and to install documentation
         (".html" and ".txt" files) within Tux Paint's documentation
         directory.

         Note: The above Makefile example assumes the user will have
         priveleges to install Tux Paint plugins system-wide.

  Windows

       TBD

  Mac OS X

       TBD

     ----------------------------------------------------------------------

Creating plugins with multiple effects

     Plugins for Tux Paint may contain more than one effect. If you have
     multiple effects that are similar, it may make sense to place them in
     one plugin file, to reduce overhead and share code.

     These following suggestions can help you create plugins that contain
     multiple effects:

       * Use a C "enum" to enumerate the effects, and count them.

           enum {
             ONE_TOOL,
             ANOTHER_TOOL,
             AND_YET_ANOTHER_TOOL,
             NUM_TOOLS };

       * Return the value of "NUM_TOOLS" when "get_tool_count()" is called,
         and compare "which" values sent to other functions with the other
         enumerated values.

       * Create arrays of "NUM_TOOLS" length to contain effect-specific data.

           char * my_plugin_snd_filenames[NUM_TOOLS] = {
             "one.ogg", "another.ogg", "yet_another.ogg" };
           Mix_Chunk * my_plugin_snds[NUM_TOOLS];

       * Use a C "for"-loop to load or create the effect-specific data (such
         as loading sound effects during your "init()").

           int i;
           char fname[1024];

           for (i = 0; i < NUM_TOOLS; i++)
           {
             /* Becomes, for example,
           "/usr/share/tuxpaint/sounds/magic/one.ogg" */
             
             snprintf(fname, sizeof(fname), "%s/sounds/magic/%s",
                 api->data_prefix, my_plugin_snd_filenames[i];

             my_plugin_snds[i] = Mix_LoadWAV(fname);
           }

       * Similarly, do the same to free them later (such as freeing sound
         effects during your "shutdown()").

           int i;

           for (i = 0; i < NUM_TOOLS; i++)
             Mix_FreeChunk(my_plugin_snds[i]);

       * Use "which" values sent to your functions as an index into those
         arrays (e.g., for playing the appropriate sound effect for a tool).

     Note: Even if your plugin currently contains only one effect, it may be
     useful to follow the steps above so that you can add a new variation of
     an effect with little effort. ("NUM_TOOLS" will simply be '1', your
     arrays will be of length '1', etc.)

     ----------------------------------------------------------------------

Example Code

     The C source file "tp_magic_example.c" contains a complete example of a
     plugin with multiple simple effects.

     ----------------------------------------------------------------------

Getting Help

     For more information, check the Tux Paint website:
     http://www.tuxpaint.org/, and the Simple DirectMedia Layer library
     website: http://www.libsdl.org/.

     Additionally, other Tux Paint developers and users can be found on the
     "tuxpaint-devel" and "tuxpaint-users" mailing lists:
     http://www.tuxpaint.org/lists/.

     ----------------------------------------------------------------------

Glossary

     * alpha: See "RGBA"
     * &: See "ampersand"
     * ampersand: "&". A symbol in C that allows you to refer to the memory
       address of a variable; that is, a pointer. (For example, consider
       "int i;". Later, "&i" refers to the memory where "i" is stored, not
       the value of "i" itself; it is a 'pointer to "i"'.)
     * API: Application Programming Interface. TBD
     * argument: A value sent to a function.
     * arrow: "->". A symbol in C that references an element within a pointer
       to a struct.
     * backquote: See "grave."
     * backtick: See "grave."
     * bit: "Binary digit." Bits are the basic storage unit in a computer's
       memory, disk, networking, etc. They represent either 0 or 1. (Compared
       to a decimal digit, which can be anything between 0 and 9.) Just as a
       series of decimal digits can represent a larger number (e.g., "1" and
       "5" is fifteen (15)), so can bits (e.g., "1" and "0", is two). In
       decimal, we go from right to left: ones place, tens place, hundreds
       place, thousands place, etc. In binary, it is: ones place, twos place,
       fours place, eights place, etc. (See also "byte.")
     * blue: See "RGBA"
     * boolean 'or': A mathematical operation that results in a true value if
       either operand is true. ("1 | 0", "0 | 1" and "1 | 1" all result in
       "1". "0 | 0" results in "0".)
     * |: See "boolean 'or'"
     * .: See "dot"
     * `: See "grave."
     * *: See "star"
     * byte: A unit of memory made up of 8 bits. As a signed value, it can
       represent -128 through 127. As an unsigned value, it can represent 0
       through 255. As a series of bits, for example, the byte "00001100"
       represents the decimal value 12.
     * callback: TBD
     * C enumeration: A construct in C that allows you to label numeric
       values (usually starting at 0 and incrementing by one). (e.g., "enum {
       ONE, TWO, THREE };"
     * C function: TBD
     * C header file: TBD
     * channel: TBD
     * click: The action of pressing a button on a mouse.
     * coordinates: A set of numbers corresponding to a physical position;
       for example, in a two-dimensional (2D) image, "X" and "Y" coordinates
       specify the position across (left-to-right) and down the image,
       respectively. In SDL, the coordinates (0,0) is the top-leftmost pixel
       of a surface.
     * C pointer: A variable that contains the location of a piece of memory;
       usually used to 'point' to another variable. Since C functions can
       only return one value as a result, pointers are often sent to
       functions to allow the function to change the values of multiple
       variables. (For example, Tux Paint's "rgbtohsv()" and "hsvtorgb()".)
     * C structure: A construct in C that allows you to declare a new
       variable 'type' which may contain other types within. For example,
       SDL's "SDL_Rect" contains four integer values, the coordinates of the
       rectangle (X,Y), and its dimensions (width and height).
     * #define: A C statement that defines a substitution that can occur
       later in the code. Generally used for constant values (e.g.,
       "#define RADIUS 16"; all instances of "RADIUS" will be replaced with
       "16"), but can also be used to create macros. Typically placed within
       C header files.
     * dimensions: The size of an object, in terms of its width (left to
       right) and height (top to bottom).
     * .dll: See "Shared Object"
     * dot: ".". A symbol in C that references an element within a struct.
     * drag: The action of moving a mouse while the button remains held.
     * element: A variable stored within a C structure. (Example: "w" and "h"
       elements of SDL_Surface store the surface's width and height,
       respectively.)
     * enum: See "C enumeration"
     * float: See "floating point"
     * floating point: TBD
     * format: An SDL_Surface element (a pointer to an SDL_PixelFormat
       structure) that contains information about a surface; for example, the
       number of bits used to represent each pixel). (See also the
       "SDL_PixelFormat(3) man page)
     * free(): A C function that frees (deallocates) memory allocated by
       other C functions (such as "strdup()").
     * function: See "C function"
     * gcc: The GNU C compiler, a portable Open Source compiler. (See also
       the "gcc(1)" man page)
     * GNU C Compiler: See "gcc"
     * grave: The "`" character; used by the BASH shell to use the output of
       a command as the command-line arguments to another.
     * green: See "RGBA"
     * ->: See "arrow"
     * .h: See "C header file"
     * header: See "C header file"
     * header file: See "C header file"
     * HSV: Hue, Saturation and Value. TBD
     * hue: See "HSV"
     * IMG_Load(): An SDL_image function that loads an image file (e.g., a
       PNG) and returns it as an "SDL_Surface *".
     * #include: A C statement that asks the compiler to read the contents of
       another file (usually a header file).
     * int: See "integer"
     * integer: TBD
     * libSDL: See "Simple DirectMedia Layer"
     * linear: TBD
     * macro: A C construct that looks similar to a C function, but is simply
       a #define that is expanded 'inline'. For example, if you declared the
       macro "#define ADD(A,B) ((A)+(B))", and then used it with "c =
       ADD(1,2);", that line of code would literally expand to "c = ((1) +
       (2));", or more simply, "c = 1 + 2;".
     * magic_api: A C structure that is passed along to a plugin's functions
       that exposes data and functions within the running copy of Tux Paint.
     * make: A utility that automatically determines which pieces of a larger
       program need to be recompiled, and issues the commands to recompile
       them. (See also "Makefile")
     * Makefile: A text file used by the "make" utility; it describes the
       relationships among files in your program, and the commands for
       updating each file. (For example, to compile a human-readable
       source-code file into a computer-readable executable program file.)
     * Magic tool: One of a number of effects or drawing tools in Tux Paint,
       made available via the "Magic" tool button.
     * Mix_Chunk *: (A pointer to) a C structure defined by SDL_mixer that
       contains a sound.
     * Mix_FreeChunk(): An SDL_mixer function that frees (deallocates) memory
       allocated for an SDL_mixer sound 'chunk' ("Mix_Chunk *").
     * Mix_LoadWAV(): An SDL_mixer function that loads a sound file (WAV,
       Ogg Vorbis, etc.) and returns it as a "Mix_Chunk *".
     * namespace: TBD
     * OGG: See "Ogg Vorbis"
     * Ogg Vorbis: TBD (See also: "WAV")
     * Plugin: TBD
     * PNG: Portable Network Graphics. An extensible file format for the
       lossless, portable, well-compressed storage of raster images. It's the
       file format Tux Paint uses to save images, and for its brushes and
       stamps. It's an easy way to store 32bpp RGBA images (24bpp true color
       with full 8bpp alpha transparency), excellent for use in graphics
       programs like Tux Paint. (See also the "png(5) man page)
     * pointer: See "C pointer"
     * red: See "RGBA"
     * release: The action of releasing a button on a mouse.
     * RGBA: "Red, Green, Blue, Alpha." TBD
     * RGB: See "RBGA"
     * saturation: See "HSV"
     * SDL: See "Simple DirectMedia Layer"
     * SDL_FreeSurface(): An libSDL function that frees (deallocates) memory
       allocated for an SDL surface ("SDL_Surface *"). (See also the
       "SDL_FreeSurface(3)" man page)
     * SDL_GetRGB(): A libSDL function that, given a Uint32 pixel value
       (e.g., one returned from the Tux Paint's Magic tool API helper
       function "getpixel()"), the format of the surface the pixel was taken
       from, and pointers to three Uint8 variables, will place the Red, Green
       and Blue (RGB) values of the pixel into the three Uint8 variables.
       (Example: "SDL_GetRGB(getpixel(surf, x, y), surf->format, &r, &g,
       &b);".) (See also the "SDL_GetRGB(3)" man page)
     * SDL_MapRGB(): A libSDL function that, given the format of a surface
       and Uint8 values representing Red, Green and Blue values for a pixel,
       returns a Uint32 pixel value that can be placed in the surface (e.g.,
       using Tux Paint's Magic tool API helper function "putpixel()").
       (Example: "putpixel(surf, x, y, SDL_MapRGB(surf->format, r, g, b));".)
       (See also the "SDL_MapRGB(3)" man page)
     * SDL_image: A library on top of libSDL that can load various kinds of
       image files (e.g., PNG) and return them as an "SDL_Surface *".
     * SDL_mixer: A library on top of libSDL that can load various kinds of
       sound files (WAV, Ogg Vorbis, etc.) and play back multiple sounds at
       once (mix them).
     * SDL_Rect: A C structure defined by libSDL that represents a
       rectangular area. It contains elements representing the coordinates of
       the top left corner of the rectange (x,y) and the dimensions of the
       rectangle (w,h). (See also the "SDL_Rect(3)" man page)
     * SDL_Surface *: (A pointer to) a C structure defined by libSDL that
       contains a drawing surface. (See also the "SDL_Surface(3)" man page)
     * Shared Object: A piece of code that's compiled separately from the
       main application, and loaded dynamically, at runtime.
     * Simple DirectMedia Layer: A programming library that allows programs
       portable low level access to a video framebuffer, audio output, mouse,
       and keyboard. (See also: http://www.libsdl.org/)
     * snprintf(): A C function, related to "printf()", which takes a
       'format' string and one or more additional arguments, and puts them
       together. "snprintf()" takes the resulting output and stores it into a
       string, making sure not to go beyond the string's buffer size (which
       must also be supplied). For example, assume a string "char str[20];"
       has been declared; "snprintf(str, 20, "Name: %s, Age: %d", "Bill",
       "32");" will store "Name: Bill, Age: 32" into 'str'. (See also the
       "snprintf(3)" man page)
     * .so: See "Shared Object"
     * sRBG: See "RGBA"
     * star: "*". A symbol in C that, when used in the declaration of
       variables (e.g., arguments to a function), denotes that the variable
       is a pointer. (For example, "int * p;" means that "p" is a pointer to
       an integer.) When used next to a pointer, it 'dereferences' the
       variable. (For example, later "*p = 50;" assigns the value of 50 to
       the memory that "p" points to; it does not change the value of "p",
       which is still a pointer to an integer. In essence, it changed the
       integer that's being pointed to.)
     * strdup(): A C function that allocates enough memory to store a copy of
       a string, copies the string to it, and returns a "char *" pointer to
       the new copy. (See also the "strdup(3)" man page)
     * struct: See "C structure"
     * The GIMP: An Open Source image manipulation and paint program.
     * tp_magic_api.h: A header file that defines Tux Paint's Magic tool API.
       Plugins must '#include' it.
     * tp-magic-config: A command-line program that provides information
       about the installed version of Tux Paint to plugin developers (such as
       what C compiler flags they should compile with, and where plugin
       shared objects and data files should be installed). (See also the
       "tp-magic-config(3)" man page.)
     * Uint32: A 32-bit, unsigned integer (defined by libSDL). In other
       words, four bytes that can represent 0 through 4294967295. (Typically
       used to hold enough information to store three or four bytes
       representing a pixel's color; i.e., RBGA value).
     * Uint8: An 8-bit, unsigned integer (defined by libSDL). In other words,
       a byte that can represent 0 through 255.
     * unsigned: In C, a variable that can store a numeric value can be
       declared as either "signed" (the default), or "unsigned". In the
       former case, one bit of the value is used to denote the sign of the
       value (either positive or negative). In the latter case, the value can
       only be positive, but benefits from one extra bit of storage for the
       number. A signed byte (8 bits), for example, can represent any number
       between -128 and 127. An unsigned byte can go up to 255, but it cannot
       go below 0. For the purposes of graphics in SDL, unsigned values
       should be used for RGB values, since each channel (red, green and
       blue) may be between 0 (off) and 255 (brightest).
     * value: See "HSV"
     * variable: A construct in computer programming that contains a value
       which can be referenced again later by referring to the variable's
       name, and typically changed later. For example, a variable to hold
       someone's age could be declared as an integer: "int a;". It can be
       examined later: "if (a >= 18) { /* they are an adult */ } else { /*
       they are not an adult */ }".
     * WAV: TBD (See also "Ogg Vorbis")
     * (w,h): See "Dimensions"
     * (x,y): See "Coordinates"
