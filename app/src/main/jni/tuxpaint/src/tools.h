/*
  tools.h

  For Tux Paint
  List of available tools.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  (See COPYING.txt)

  Copyright (c) 2002-2020 by Bill Kendrick
  bill@newbreedsoftware.com
  http://www.tuxpaint.org/

  June 14, 2002 - August 15, 2020
  $Id$
*/


#include "tip_tux.h"


/* What tools are available: */

enum
{
  TOOL_BRUSH,
  TOOL_STAMP,
  TOOL_LINES,
  TOOL_SHAPES,
  TOOL_TEXT,
  TOOL_LABEL,
  TOOL_FILL,
  TOOL_MAGIC,
  TOOL_UNDO,
  TOOL_REDO,
  TOOL_ERASER,
  TOOL_NEW,
  TOOL_OPEN,
  TOOL_SAVE,
  TOOL_PRINT,
  TOOL_QUIT,
  NUM_TOOLS
};


/* Tool names: */

const char *const tool_names[NUM_TOOLS] = {
  // Freehand painting tool
  gettext_noop("Paint"),

  // Stamp tool (aka Rubber Stamps)
  gettext_noop("Stamp"),

  // Line drawing tool
  gettext_noop("Lines"),

  // Shape creation tool (square, circle, etc.)
  gettext_noop("Shapes"),

  // Text tool
  gettext_noop("Text"),

  // Label tool
  gettext_noop("Label"),

  // Fill tool
  gettext_noop("Fill"),

  // "Magic" effects tools (blur, flip image, etc.)
  gettext_noop("Magic"),

  // Undo last action
  gettext_noop("Undo"),

  // Redo undone action
  gettext_noop("Redo"),

  // Eraser tool
  gettext_noop("Eraser"),

  // Start a new picture
  gettext_noop("New"),

  // Open a saved picture
  gettext_noop("Open"),

  // Save the current picture
  gettext_noop("Save"),

  // Print the current picture
  gettext_noop("Print"),

  // Quit/exit Tux Paint application
  gettext_noop("Quit")
};


/* Some text to write when each tool is selected: */

const char *const tool_tips[NUM_TOOLS] = {
  // Paint tool instructions
  gettext_noop("Pick a color and a brush shape to draw with."),

  // Stamp tool instructions
  gettext_noop("Pick a picture to stamp around your drawing."),

  // Line tool instructions
  gettext_noop("Click to start drawing a line. Let go to complete it."),

  // Shape tool instructions
  gettext_noop
    ("Pick a shape. Click to start drawing, drag, and let go when it is the size you want. Move around to rotate it, and click to draw it."),

  // Text tool instructions
  gettext_noop
    ("Choose a style of text. Click on your drawing and you can start typing. Press [Enter] or [Tab] to complete the text."),

  // Label tool instructions
  gettext_noop
    ("Choose a style of text. Click on your drawing and you can start typing. Press [Enter] or [Tab] to complete the text. By using the selector button and clicking an existing label, you can move it, edit it and change its text style."),

  // Fill tool instructions
  gettext_noop("Click in the picture to fill that area with color."),

  // Magic tool instruction
  gettext_noop("Pick a magical effect to use on your drawing!"),

  // Response to 'undo' action
  gettext_noop("Undo!"),

  // Response to 'redo' action
  gettext_noop("Redo!"),

  // Eraser tool
  gettext_noop("Eraser!"),

  // Response to 'start a new image' action
  gettext_noop("Pick a color or picture with which to start a new drawing."),

  // Response to 'open' action (while file dialog is being constructed)
  gettext_noop("Open…"),

  // Response to 'save' action
  gettext_noop("Your image has been saved!"),

  // Response to 'print' action (while printing, or print dialog is being used)
  gettext_noop("Printing…"),

  // Response to 'quit' (exit) action
  gettext_noop("Bye bye!")
};

// Instruction while using Line tool (after click, before release)
#define TIP_LINE_START gettext_noop("Let go of the button to complete the line.")

// Instruction while using Shape tool (after first click, before release)
#define TIP_SHAPE_START gettext_noop("Hold the button to stretch the shape.")

// Instruction while finishing Shape tool (after release, during rotation step before second click)
#define TIP_SHAPE_NEXT gettext_noop("Move the mouse to rotate the shape. Click to draw it.")

// Notification that 'New' action was aborted (current image would have been lost)
#define TIP_NEW_ABORT gettext_noop("OK then… Let’s keep drawing this one!")


/* Tool icon filenames: */

const char *const tool_img_fnames[NUM_TOOLS] = {
  DATA_PREFIX "images/tools/brush.png",
  DATA_PREFIX "images/tools/stamp.png",
  DATA_PREFIX "images/tools/lines.png",
  DATA_PREFIX "images/tools/shapes.png",
  DATA_PREFIX "images/tools/text.png",
  DATA_PREFIX "images/tools/label.png",
  DATA_PREFIX "images/tools/fill.png",
  DATA_PREFIX "images/tools/magic.png",
  DATA_PREFIX "images/tools/undo.png",
  DATA_PREFIX "images/tools/redo.png",
  DATA_PREFIX "images/tools/eraser.png",
  DATA_PREFIX "images/tools/new.png",
  DATA_PREFIX "images/tools/open.png",
  DATA_PREFIX "images/tools/save.png",
  DATA_PREFIX "images/tools/print.png",
  DATA_PREFIX "images/tools/quit.png"
};


/* Tux icons to use: */

const int tool_tux[NUM_TOOLS] = {
  TUX_DEFAULT,
  TUX_DEFAULT,
  TUX_DEFAULT,
  TUX_DEFAULT,
  TUX_DEFAULT,
  TUX_DEFAULT,
  TUX_DEFAULT,
  TUX_DEFAULT,
  TUX_OOPS,
  TUX_WAIT,
  TUX_DEFAULT,
  TUX_DEFAULT,
  TUX_DEFAULT,
  TUX_GREAT,
  TUX_GREAT,
  TUX_DEFAULT
};
