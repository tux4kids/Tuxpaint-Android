#!/bin/sh
#
# To add a new translation put its po here and add/modify the proper entry in ..res/values/arrays.xml
#
# Update .po files and convert to .xml translation files
#
# Note, 2023 december: If a2po doesn't work with python3, apply this pull request to android2po:
# https://github.com/miracle2k/android2po/pull/72/commits/144dc6186974df40cf79daa465d6c644b7671651

# Hack for Santali, as it doesn't work with the default generated name
mv ../res/values-b+sat+Olck ../res/values-sat-rOlck

# Create .po files for any languages that have
# an .xml file but no corresponding .po file
a2po init --groups strings

# Update the .po files and the .pot file
# with the latest changes in strings.xml
a2po export --groups strings --enable-fuzzy-matching --clear-obsolete

# (Re)generate .xml files based on the .po files
a2po import --groups strings --ignore-fuzzy

# Hack for Santali, as it doesn't work with the default generated name
mv ../res/values-sat-rOlck ../res/values-b+sat+Olck

