#!/bin/sh
# Update .po files and convert to .xml translation files

# Create .po files for any languages that have
# an .xml file but no corresponding .po file
a2po init --groups strings

# Update the .po files and the .pot file
# with the latest changes in strings.xml
a2po export --groups strings --enable-fuzzy-matching --clear-obsolete

# (Re)generate .xml files based on the .po files
a2po import --groups strings --ignore-fuzzy

