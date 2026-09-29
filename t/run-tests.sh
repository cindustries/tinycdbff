#!/bin/sh
# Public domain. Part of the tinycdbff test tree.
#
# Behavioural test runner: builds the real cdb_*.c sources against the
# in-memory FatFs shim in t/mem (no real Chan FatFs, no hardware) and runs
# t/test_cdb.c. Exits non-zero on any failure.
#
# Usage: sh t/run-tests.sh   (override the compiler with CC=clang, etc.)
set -e
cd "$(dirname "$0")/.."
CC="${CC:-gcc}"
OUT="${TMPDIR:-/tmp}/tinycdbff-test.$$"
trap 'rm -f "$OUT"' EXIT
SRC="cdb_hash.c cdb_make_add.c cdb_make.c cdb_make_put.c cdb_seek.c cdb_unpack.c"
"$CC" -std=c99 -Wall -Wextra -g -I t/mem -I . -o "$OUT" t/test_cdb.c t/mem/ff.c $SRC
if "$OUT"; then
  echo "OK: behavioural tests passed"
else
  echo "FAILED: behavioural tests failed" >&2
  exit 1
fi
