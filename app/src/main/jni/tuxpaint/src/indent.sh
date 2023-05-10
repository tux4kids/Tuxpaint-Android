#!/bin/bash

# Indent Tux Paint source files

echo "Indenting src/*.c"
find . -name "*.c" -exec indent -nbfda -npcs -npsl -bli0 --no-tabs {} \;

echo "Indenting src/*.h"
find . -name "*.h" -exec indent -nbfda -npcs -npsl -bli0 --no-tabs {} \;

echo "Indenting magic/src/*.c"
find ../magic/src/ -name "*.c" -exec indent -nbfda -npcs -npsl -bli0 --no-tabs {} \;

echo "Indenting magic/src/*.h"
find ../magic/src/ -name "*.h" -exec indent -nbfda -npcs -npsl -bli0 --no-tabs {} \;

