#!/usr/bin/php
<?php
/* nobr_forbidden.php

   A script to encase characters that are forbidden from
   appearing at the beginning of a line (e.g., the
   "。" full-stop), along with the previous character, inside
   a "<nobr>...</nobr>", to prevent `w3m`'s word-wrapping
   routine from doing that.

   Bill Kendrick
   2023-07-17 - 2023-07-17
*/

/* See https://en.wikipedia.org/wiki/Line_breaking_rules_in_East_Asian_languages */

/* Closing brackets (ignoring ' " ]) */
$forbidden_start = "）\)｝〕〉》」』】〙〗〟｠»";

/* Japanese characters: chiisai kana and special marks */
$forbidden_start .= "ヽヾーァィゥェォッャュョヮヵヶぁぃぅぇぉっゃゅょゎゕゖㇰㇱㇲㇳㇴㇵㇶㇷㇸㇹㇺㇻㇼㇽㇾㇿ々〻";

/* Hyphens */
$forbidden_start .= "‐゠–〜";

/* Delimiters */
$forbidden_start .= "？!‼⁇⁈⁉";

/* Mid-sentence punctuation */
$forbidden_start .= "・、:;,";

/* Sentence-ending punctuation */
$forbidden_start .= "。\.";

/* Opening brackets (ignoring ' " [) */
$forbidden_end = "（\(｛〔〈《「『【〘〖〝｟«";


/* FIXME: Would be better to use DOMDocument() and modify the
   the text in the nodeValues, but the tuxpaint-docs HTML is
   not currently XHTML compliant ;-( -bjk 2023.07.17

   Something like this:

     $dom = new DOMDocument();
     libxml_use_internal_errors(false);
     $dom->loadHTMLFile("php://stdin");
     
     $p = $dom->getElementsByTagName('p');
     foreach ($p as $pnode) {
       $nodeValue = $pnode->nodeValue;
     
       $nodeValue = preg_replace("/(.。)/", "<nobr>\\1<\/nobr>", $nodeValue);
       $newNode = $dom->createElement("p", $nodeValue);
       $pnode->parentNode->replaceChild($newNode, $pnode);
     }
     
     echo $dom->saveHTML();

   Instead, just reading the HTML file as a big text stream and
   doing our best to only modify things that are not within the
   HTML tags (esp. the <img> tags' "alt" attributes (aka "alt tags")).
*/

//setlocale(LC_ALL, "ja_JP.UTF-8");

$fi = fopen("php://stdin", "r");

$in_tag = false;

while (!feof($fi)) {
  $line = fgets($fi);

  if (!feof($fi)) {
    $newLine = "";
    $text = "";

    for ($i = 0; $i < strlen($line); $i++) {
      $c = substr($line, $i, 1);

      if ($c == "<") {
        $in_tag = true;
        $newLine .= replace_forbidden($text) . $c;
        $text = "";
      } else if ($c == ">") {
        $in_tag = false;
        $newLine .= $c;
        $text = "";
      } else if ($in_tag) {
        $newLine .= $c;
      } else {
        $text .= $c;
      }
    }

    $newLine .= replace_forbidden($text);
    $text = "";

    echo $newLine;
  }
}

function replace_forbidden($str) {
  global $forbidden_start, $forbidden_end;

  $japanese = "\p{Katakana}\p{Hiragana}\p{Han}";

  $str = preg_replace("/([$forbidden_end]+[$japanese][$forbidden_start]+)/u", "<nobr>\\1</nobr>", $str);
  $str = preg_replace("/([$japanese][$forbidden_start]+)/u", "<nobr>\\1</nobr>", $str);
  $str = preg_replace("/([$forbidden_end]+[$japanese])/u", "<nobr>\\1</nobr>", $str);
  return $str;
}

