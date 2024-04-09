#!/bin/bash

# Kontroller, at antallet af argumenter er korrekt
if [ "$#" -ne 2 ]; then
    echo "Brug: $0 input.js output.h"
    exit 1
fi

# Gem indholdet af JavaScript-filen i en variabel
JS_CONTENT=$(cat "$1")

# Escape specielle tegn og tilføj citatmærker i starten og slutningen af hver linje
JS_CONTENT_ESCAPED=$(echo "$JS_CONTENT" | sed 's/\\/\\\\/g; s/"/\\"/g; s/^/"/; s/$/"/')

# Skriv C-string til outputfilen
echo "// Autogenereret af scriptet $0" > "$2"
echo "#ifndef JS_CONTENT_H" >> "$2"
echo "#define JS_CONTENT_H" >> "$2"
echo "" >> "$2"
echo "const char* js_content = {" >> "$2"
echo "$JS_CONTENT_ESCAPED" | sed 's/^/    /' >> "$2"
echo "};" >> "$2"
echo "" >> "$2"
echo "#endif" >> "$2"

echo "Filen $2 er blevet oprettet med JavaScript-indholdet fra $1."