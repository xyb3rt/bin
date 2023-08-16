#!/bin/sh -e
pattern="$1"
shift
files="$(rg -l "$pattern" "$@")"
[ -n "$files" ]
files="$(echo "$files" | sort | ped)"
IFS='
'
export LESSEDIT='vim ?lj+%lj. "+normal! zt" %g'
exec less "+/*$pattern" $files
