#!/bin/sh
# Standalone compile-check for tinycdbff.
#
# tinycdbff is a source-drop module that needs Chan FatFs (ff.h) at build time.
# To catch breakage without vendoring all of FatFs, this script syntax-checks
# every source against the minimal FatFs stub in t/ff.h. It proves the sources
# compile; it does NOT exercise cdb behaviour (that needs real FatFs + storage).
#
# Usage: sh t/compile-check.sh   (override the compiler with CC=clang, etc.)
set -e
cd "$(dirname "$0")/.."
CC="${CC:-gcc}"
SRC="cdb_hash.c cdb_make_add.c cdb_make.c cdb_make_put.c cdb_seek.c cdb_unpack.c"
for f in $SRC; do
  echo "checking $f"
  "$CC" -std=c99 -Wall -Wextra -fsyntax-only -I t -I . "$f"
done
echo "OK: all sources compile-check clean"
