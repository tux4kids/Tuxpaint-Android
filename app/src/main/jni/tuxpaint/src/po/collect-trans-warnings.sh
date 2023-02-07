#!/bin/bash

echo
echo Collecting warnings from \"check_translations.sh\" into \"check_translations_warnings.txt\"
echo
rm -f check_translations_warnings.txt
date --rfc-3339=s > check_translations_warnings.txt
./check_translations.sh | grep --line-buffered _WARNING_ | tee -a check_translations_warnings.txt
echo
echo DONE

