#!/bin/sh -e
p="$1"
shift
[ $# -eq 0 ] && set .
find "$@" | sed 's;^\./;;' | rg --smart-case "$p[^/]*$" | sort
