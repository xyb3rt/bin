#!/bin/sh -e
p="$1"
shift
[ $# -eq 0 ] && set .
find "$@" -name '.?*' -prune -o -print \
	| sed 's:^\./::' \
	| rg --smart-case "$p[^/]*$" \
	| sort
