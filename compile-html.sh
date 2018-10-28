#!/bin/bash
minify -v -o index-min.html --mime text/html --html-keep-end-tags --html-keep-document-tags index.html
minify -v -o setup-min.html --mime text/html --html-keep-end-tags --html-keep-document-tags setup.html
minify -v -o index-min.js --mime text/javascript index.js
cat index-min.js | tr '\n' ' ' | sed -e 's/\\/\\\\/g;s/"/\\"/g;s/^/"/;s/$/\\n"/' > index-min2.js
cat index-min.html | tr '\n' ' ' | sed -e 's/\\/\\\\/g;s/"/\\"/g;s/^/"/;s/$/\\n"/' > index-min2.html
cat setup-min.html | tr '\n' ' ' | sed -e 's/\\/\\\\/g;s/"/\\"/g;s/^/"/;s/$/\\n"/' > setup-min2.html
rm index-min.html index-min.js setup-min.html
{
    echo
    echo //codeblock for files
    echo const char *INDEX_MIN_JS = $(cat index-min2.js)';'
    echo const char *INDEX_MIN_HTML = $(cat index-min2.html)';'
    echo const char *SETUP_MIN_HTML = $(cat setup-min2.html)';'
    echo
} > webfiles.h
rm index-min2.html index-min2.js setup-min2.html
