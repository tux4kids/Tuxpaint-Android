<?
/* This PHP file contains all the documentation for the various Magic Tools
that are shipped with Tux Paint, as well as code and templates to generate
individual HTML files for each of them, and an index.html that links to
them all. */

/* Bill Kendrick <bill@newbreedsoftware.com> */
/* 2009.10.08 */


/* Authors of the Magic tools: */

$AUTHOR_KENDRICK = "Bill Kendrick|bill@newbreedsoftware.com";
$AUTHOR_ALBERT = "Albert Cahalan|albert@users.sf.net";
$AUTHOR_ANDREWC = "Andrew Corcoran|akanewbie@gmail.com";
$AUTHOR_ADAMR = "Adam Rakowski|foo-script@o2.pl";
$AUTHOR_PERE = "Pere Pujal i Carabantes|pere@fornol.no-ip.org";


/* Information about each of the tools:

   'name' is the name of the tool; the name for the HTML file is based on
   this (all lowercase, with spaces stripped)

   e.g.: "My Magic Tool"   (and the file will be "mymagictool.html")


   'author' is the author's name and email, separated by a '|' character;
   it may be an array.  Try to add authors as constant vars above, so they
   can be accurately reused or updated.

   e.g. "Joe Schmoe|joe@sch.org"
   or array("Joe Schmoe|joe@sch.org", "Another Guy|a.guy@inter.net")


   'desc' is the description, in HTML. (It will be wrapped in <p>...</p>).


   'see' is optional.  It should be the name of another tool to link to
   (same format as 'name'; it will be converted for use as a link).

   e.g. "Related Magic Tool"  (will link to "relatedmagictool.html")
   or array("Related One", "Related Two")


   NOTE: If an image "ex_shortname.png" exists in html/images/,
   it will be referred to in an <img> tag in the output.
*/


$tools = array(

  array('name'=>'Blinds',
   'desc'=>'Click towards the edge of your picture to pull window blinds over it. Move perpendicularly to open or close the blinds.',
   'author'=>$AUTHOR_PERE),

  array('name'=>'Blocks',
   'desc'=>'This makes the picture blocky looking ("pixelated") wherever you drag the mouse.',
   'author'=>array($AUTHOR_KENDRICK,$AUTHOR_ALBERT)),

  array('name'=>'Blur',
   'desc'=>'This makes the picture fuzzy wherever you drag the mouse.',
   'author'=>array($AUTHOR_KENDRICK,$AUTHOR_ALBERT),
   'see'=>array('Sharpen', 'Smudge')),

  array('name'=>'Bricks',
   'desc'=>'These two tools intelligently paint large and small brick patterns on the canvas.  The bricks can be tinted various redish hues by selecting different colors in the color palette.',
   'author'=>$AUTHOR_ALBERT),

  array('name'=>'Calligraphy',
   'desc'=>'This paints on the canvas with a calligraphy pen. The quicker you move, the thinner the lines.',
   'author'=>$AUTHOR_KENDRICK),

  array('name'=>'Cartoon',
   'desc'=>'This makes the picture look like a cartoon &mdash; with thick outlines and bright, solid colors &mdash; wherever you move the mouse.',
   'author'=>$AUTHOR_KENDRICK),

  array('name'=>'Chalk',
   'desc'=>'This makes parts of the picture (where you move the mouse) look like a chalk drawing.',
   'author'=>$AUTHOR_KENDRICK),

  array('name'=>'Color and White',
   'desc'=>'This makes parts of your picture two colors: white, and the color chosen in the palette. (i.e., if you choose black, you\'ll get a black and white picture).',
   'author'=>$AUTHOR_ANDREWC),

  array('name'=>'Color Shift',
   'desc'=>'This shifts the colors in your picture.', /* What? */
   'author'=>$AUTHOR_ANDREWC),

  array('name'=>'Confetti',
   'desc'=>'Throw confetti around your picture!',
   'author'=>$AUTHOR_ADAMR),

  array('name'=>'Darken',
   'desc'=>'This dakrens the colors wherever you drag the mouse.  (Do it to the same spot many times, and it will eventually become black.)',
   'author'=>$AUTHOR_KENDRICK,
   'see'=>array('Lighten', 'Tint')),

  array('name'=>'Distortion',
   'desc'=>'This slightly distorts the picture wherever you move the mouse.',
   'author'=>$AUTHOR_KENDRICK),

  array('name'=>'Drip',
   'desc'=>'This makes the paint "drip" wherever you move the mouse.',
   'author'=>$AUTHOR_KENDRICK),

  array('name'=>'Edges',
   'desc'=>'Trace the edges in your picture, over a white background.',
   'author'=>$AUTHOR_ANDREWC,
   'see'=>array('Emboss', 'Silhouette')),

  array('name'=>'Emboss',
   'desc'=>'This makes parts of your picture look "embossed."  Wherever there are sharp edges in your picture, the picture will look raised like it was stamped in metal.',
   'author'=>$AUTHOR_KENDRICK,
   'see'=>array('Edges', 'Silhouette')),

  array('name'=>'Fill',
   'desc'=>'This floods the picture with a color.  It lets you quickly fill parts of the picture, as if it were a coloring book.',
   'author'=>$AUTHOR_KENDRICK),

  array('name'=>'Fisheye',
   'desc'=>'Warp parts of your picture like it\'s being seen through a fisheye lens.',
   'author'=>$AUTHOR_ADAMR),

  array('name'=>'Flip',
   'desc'=>'Similar to "Mirror."  Click and the entire image will be turned upside-down.',
   'author'=>$AUTHOR_KENDRICK,
   'see'=>'Mirror'),

  array('name'=>'Flower',
   'desc'=>'This tool draws small flowers, with leafy bases and stalks.  Click to set the base, then drag the mouse upwards to drawe the stalk, and finally release the mouse button to finish the flower.  It will be drawn in the currently-selected color.  The shape and length of the stalk depends on how you move the mouse while you drag.',
   'author'=>$AUTHOR_KENDRICK,
   'see'=>'Grass'),

  array('name'=>'Foam',
   'desc'=>'Click and drag the mouse to draw foamy bubbles.  The more you drag the mouse in a particular spot, the more likely small bubbles will combine to form bigger bubbles.',
   'author'=>$AUTHOR_KENDRICK),

  array('name'=>'Fold',
   'desc'=>'Click a corner of your picture and drag towards the center to fold it up like a piece of paper.',
   'author'=>array($AUTHOR_ADAMR, $AUTHOR_KENDRICK, $AUTHOR_PERE)),

  array('name'=>'Glass Tile',
   'desc'=>'Click and drag over your picture to make it look like it\'s being seen through glass tiles.',
   'author'=>$AUTHOR_KENDRICK),

  array('name'=>'Grass',
   'desc'=>'This paints grass on the image. The higher up the canvas, the smaller the grass is drawn, giving an illusion of perspective.  The grass can be tinted various greenish hues by selecting different colors in the color palette.',
   'author'=>$AUTHOR_ALBERT,
   'see'=>'Flower'),

  array('name'=>'Kaleidoscope',
   'desc'=>'This paint brush draws in four places at the same time, mirroring symmetrically, both horizontally and vertically.  It uses the currently selected color.',
   'author'=>$AUTHOR_KENDRICK,
   'see'=>'Picasso', 'Rosette'),

  array('name'=>'Light',
   'desc'=>'This draws a glowing beam on the canvas, in the currently-selected color.  The more you use it on one spot, the more white it becomes.',
   'author'=>$AUTHOR_KENDRICK),

  array('name'=>'Lighten',
   'desc'=>'This fades the colors wherever you drag the mouse.  (Do it to the same spot many times, and it will eventually become white.)',
   'author'=>$AUTHOR_KENDRICK,
   'see'=>array('Darken', 'Tint')),

  array('name'=>'Metal Paint',
   'desc'=>'Click and drag to draw shiny metal using the current color.',
   'author'=>$AUTHOR_KENDRICK),

  array('name'=>'Mirror',
   'desc'=>'When you click the mouse in your picture with the "Mirror" magic effect selected, the entire image will be reversed, turning it into a mirror image.',
   'author'=>$AUTHOR_KENDRICK,
   'see'=>'Flip'),

  array('name'=>'Mosaic',
   'desc'=>'Adds a glass mosaic effect to your picture.',
   'author'=>array($AUTHOR_ADAMR, $AUTHOR_PERE),
   'see'=>array(
                'Hexagon Mosaic',
                'Irregular Mosaic',
                'Square Mosaic',
               )),

  array('name'=>'Hexagon Mosaic',
   'desc'=>'Converts parts of your picture into a mosaic of hexagon cells.',
   'author'=>$AUTHOR_PERE,
   'see'=>array(
                'Irregular Mosaic',
                'Square Mosaic',
                'Mosaic',
               )),

  array('name'=>'Irregular Mosaic',
   'desc'=>'Converts parts of your picture into a mosaic of irregularly-shaped cells.',
   'author'=>$AUTHOR_PERE,
   'see'=>array(
                'Hexagon Mosaic',
                'Square Mosaic',
                'Mosaic',
               )),

  array('name'=>'Square Mosaic',
   'desc'=>'Converts parts of your picture into a mosaic of square cells.',
   'author'=>$AUTHOR_PERE,
   'see'=>array(
                'Hexagon Mosaic',
                'Irregular Mosaic',
                'Mosaic',
               )),

  array('name'=>'Negative',
   'desc'=>'This inverts the colors wherever you drag the mouse.  (e.g., white becomes black, and vice versa.)',
   'author'=>$AUTHOR_KENDRICK),

  array('name'=>'Noise',
   'desc'=>'Add random noise and static to your picture.',
   'author'=>$AUTHOR_ANDREWC),

  array('name'=>'Perspective',
   'desc'=>'Click and drag from the corners to change the perspective of your picture.',
   'author'=>$AUTHOR_PERE),

  array('name'=>'Picasso',
   'desc'=>'Draw three swirling brushes at once, in a Picasso style.',
   'author'=>$AUTHOR_ADAMR,
   'see'=>'Rosette', 'Kaleidoscope'),

  array('name'=>'Rails',
   'desc'=>'Draw connecting locomotive train rails on your picture.',
   'author'=>array($AUTHOR_ADAMR, $AUTHOR_PERE, $AUTHOR_KENDRICK)),

  array('name'=>'Rain',
   'desc'=>'Paint raindrops on your picture.',
   'author'=>$AUTHOR_ANDREWC,
   'see'=>array('Snow Ball', 'Snow Flake')),

  array('name'=>'Rainbow',
   'desc'=>'This is similar to the paint brush, but as you move the mouse around, it cycles through a spectrum of bright colors.',
   'author'=>$AUTHOR_KENDRICK),

  array('name'=>'Real Rainbow',
   'desc'=>'Draw a transparent arc that looks like a real rainbow.',
   'author'=>$AUTHOR_KENDRICK),

  array('name'=>'ROYGBIV Rainbow',
   'desc'=>'Draw a rainbow arc of red, orange, yellow, green, blue, indigo and violet.',
   'author'=>$AUTHOR_KENDRICK),

  array('name'=>'Ripples',
   'desc'=>'Click in your picture to make water ripple distortions appear over it.',
   'author'=>$AUTHOR_KENDRICK),

  array('name'=>'Rosette',
   'desc'=>'Draw three brushes at once, in a rosette shape.',
   'author'=>$AUTHOR_ADAMR,
   'see'=>array('Kaleidoscope', 'Picasso')),

  array('name'=>'Sharpen',
   'desc'=>'Sharpen the focus of the picture.',
   'author'=>$AUTHOR_ANDREWC,
   'see'=>'Blur'),

  array('name'=>'Shift',
   'desc'=>'This shifts your picture around the canvas.  Anything that gets shifts off an edge reappears on the opposite edge.',
   'author'=>$AUTHOR_KENDRICK),

  array('name'=>'Silhouette',
   'desc'=>'Trace the edges in your picture, over a black background.',
   'author'=>$AUTHOR_ANDREWC,
   'see'=>array('Edges', 'Emboss')),

  array('name'=>'Smudge',
   'desc'=>'This pushes the colors around under the mouse, like finger painting with wet paint.',
   'author'=>$AUTHOR_ALBERT,
   'see'=>array('Blur', 'Wet Paint')),

  array('name'=>'Snow Ball',
   'desc'=>'Fill the picture with snowballs.',
   'author'=>$AUTHOR_ANDREWC,
   'see'=>array('Rain', 'Snow Flake')),

  array('name'=>'Snow Flake',
   'desc'=>'Fill the picture with snowflakes.',
   'author'=>$AUTHOR_ANDREWC,
   'see'=>array('Rain', 'Snow Ball')),

  array('name'=>'String V',
   'desc'=>'Draw V-shaped string art at any angle.',
   'author'=>$AUTHOR_PERE,
   'see'=>array('String Corner', 'String Edges')),

  array('name'=>'String Corner',
   'desc'=>'Draw V-shaped string art at right angles.',
   'author'=>$AUTHOR_PERE,
   'see'=>array('String V', 'String Edges')),

  array('name'=>'String Edges',
   'desc'=>'Draw string art around the edges of your picture.',
   'author'=>$AUTHOR_PERE,
   'see'=>array('String V', 'String Corner')),

  array('name'=>'TV',
   'desc'=>'Distort your picture so it looks like it\'s on a television (TV).',
   'author'=>$AUTHOR_ADAMR),

  array('name'=>'Tint',
   'desc'=>'This changes the color (or hue) of the parts of the picture to the selected color.',
   'author'=>$AUTHOR_KENDRICK,
   'see'=>array('Lighten', 'Darken')),

  array('name'=>'Toothpaste',
   'desc'=>'Paint thick blobs of color on your picture that look like toothpaste.',
   'author'=>$AUTHOR_ANDREWC),

/* FIXME: Tornado */

  array('name'=>'Waves',
   'desc'=>'Click to make the entire picture wavy, side-to-side.  Drag the mouse up and down to change the height of the ripples, and left and right to change the width.  Release the mouse button when it looks the way you like it.', /* FIXME: Dragging went away! */
   'author'=>$AUTHOR_KENDRICK,
   'see'=>'Wavelets'),

  array('name'=>'Wavelets',
   'desc'=>'Click to make the entire picture wavy, up-and-down.  Drag the mouse up and down to change the height of the ripples, and left and right to change the width.  Release the mouse button when it looks the way you like it.',
   'author'=>array($AUTHOR_KENDRICK, $AUTHOR_ADAMR),
   'see'=>'Waves'),

  array('name'=>'Wet Paint',
   'desc'=>'This draws a light, smudgy coat of paint on the picture.',
   'author'=>array($AUTHOR_ALBERT, $AUTHOR_KENDRICK),
   'see'=>'Smudge'),

  array('name'=>'Zoom',
   'desc'=>'Click and drag up to zoom in, or down to zoom out.',
   'author'=>$AUTHOR_PERE),
);

$fiidx = fopen("../html/index.html", "w");

fwrite($fiidx, page_header("List of Magic Tools"));

foreach ($tools as $t) {

  $shortname = str_replace(' ','', strtolower($t['name']));

  $out = page_header($t['name']);
  $out .= "<h2 align=\"center\">By ";
  if (is_array($t['author'])) {
    foreach ($t['author'] as $a) {
      list($authname, $authemail) = split('\|', $a);
      $out .= $authname." &lt;<a href=\"mailto:".$authemail."\">".$authemail."</a>&gt;<br>\n";
    }
  } else {
    list($authname, $authemail) = split('\|', $t['author']);
    $out .= $authname." &lt;<a href=\"mailto:".$authemail."\">".$authemail."</a>&gt;";
  }

  $out .= "</h2>\n";

  $out .= "<p>".$t['desc']."</p>\n";

  if (!empty($t['see'])) {
    $out .= "<p>See also: ";
    if (is_array($t['see'])) {
      foreach ($t['see'] as $s) {
        $out .= "<a href=\"".str_replace(' ', '', strtolower($s)).".html\">".$s."</a> ";
      }
    } else {
      $out .= "<a href=\"".str_replace(' ', '', strtolower($t['see'])).".html\">".$t['see']."</a>";
    }
    $out .= "</p>\n";
  }

  if (file_exists("../html/images/ex_".$shortname.".png")) {
    $out .= "<p align=center><img src=\"images/ex_".$shortname.".png\"></p>\n";
  }

  $out .= page_footer();

  $fi = fopen("../html/".$shortname.".html", "w");
  fwrite($fi, $out);
  fclose($fi);

  $link = "<li><a href=\"".$shortname.".html\">".$t['name']."</a></li>\n";

  fwrite($fiidx, $link);
}

fwrite($fiidx, page_footer());

fclose($fiidx);

function page_header($title)
{
  return "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n".
    "<body><html><head><title>Tux Paint \"Magic\" Tool: ".$title."</title>\n".
    "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=us-ascii\">\n".
    "</head>\n".
    "<body bgcolor=\"#FFFFFF\" text=\"#000000\" link=\"#0000FF\" vlink=\"#FF0000\" alink=\"#FF00FF\">\n".
    "<h1 align=\"center\">Tux Paint \"Magic\" Tool: ".$title."</h1>\n";
}

function page_footer()
{
  return "</body></html>";
}

?>
