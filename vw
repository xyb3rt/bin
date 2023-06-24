#!/bin/sh -e
pattern="$1"; shift
files="$(rg -lw "$pattern" "$@" | ped)"
IFS='
'
export LESSEDIT='%E ?lj+%lj. %g'
exec less -j.5 "+/*\b$pattern\b" $files
