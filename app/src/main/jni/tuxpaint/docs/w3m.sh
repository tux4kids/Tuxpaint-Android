#!/bin/bash

infile=${@: -1}
args=${@:1:${#}-1}

# Add some visual indicators around some text
# to make their context -- heading ("<h2>", "<h3", etc.),
# description term ("<dt>"), etc -- more obvious in
# the plain text version of the documentation:

H2_OPEN_MUNGE="s/<h2\([^>]*\)>/<h2\1><center>### /"
H2_CLOSE_MUNGE="s/<\/h2>/ ###<\/center><\/h2>/"
H3_OPEN_MUNGE="s/<h3\([^>]*\)>/<h3\1>## /"
H3_CLOSE_MUNGE="s/<\/h3>/ ##<\/h3>/"
H4_OPEN_MUNGE="s/<h4\([^>]*\)>/<h4\1># /"
H4_CLOSE_MUNGE="s/<\/h4>/ #<\/h4>/"
DT_OPEN_MUNGE="s/<dt>/<dt>\&rarr; /"
DT_CLOSE_MUNGE="s/<\/dt>/ \&larr;<\/dt>/"
DD_CLOSE_MUNGE="s/<\/dd>/<br\/>\&nbsp;<br\/><\/dd>/"

# FIXME: https://en.wikipedia.org/wiki/Line_breaking_rules_in_East_Asian_languages
# | sed -e "s/\(.[、。）]\)/<nobr>\\1<\\/nobr>/g" \
# This needs expanding & to not mess with <img> `alt` tag attributes or anything inside HTML tags (e.g., " quotes)!
# -bjk 2023.07.17

if [[ "$infile" =~ ja_JP ]]; then
  sed $infile \
    -e "$H2_OPEN_MUNGE" \
    -e "$H2_CLOSE_MUNGE" \
    -e "$H3_OPEN_MUNGE" \
    -e "$H3_CLOSE_MUNGE" \
    -e "$H4_OPEN_MUNGE" \
    -e "$H4_CLOSE_MUNGE" \
    -e "$DT_OPEN_MUNGE" \
    -e "$DT_CLOSE_MUNGE" \
    -e "$DD_CLOSE_MUNGE" \
    | php ./nobr_forbidden.php \
    | w3m $args
else
  sed $infile \
    -e "$H2_OPEN_MUNGE" \
    -e "$H2_CLOSE_MUNGE" \
    -e "$H3_OPEN_MUNGE" \
    -e "$H3_CLOSE_MUNGE" \
    -e "$H4_OPEN_MUNGE" \
    -e "$H4_CLOSE_MUNGE" \
    -e "$DT_OPEN_MUNGE" \
    -e "$DT_CLOSE_MUNGE" \
    -e "$DD_CLOSE_MUNGE" \
    | w3m $args
fi
