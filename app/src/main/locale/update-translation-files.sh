#!/bin/sh
# Update .po files and convert to .xml translation files


# Hack for Santali, as it doesn't work with the default generated name
mv ../res/values-b+sat+Olck ../res/values-sat@olchiki

# Create .po files for any languages that have
# an .xml file but no corresponding .po file
a2po init --groups strings

# Update the .po files and the .pot file
# with the latest changes in strings.xml
a2po export --groups strings --enable-fuzzy-matching --clear-obsolete

# (Re)generate .xml files based on the .po files
a2po import --groups strings --ignore-fuzzy

# Hack for Santali, as it doesn't work with the default generated name
mv ../res/values-sat@olchiki ../res/values-b+sat+Olck

