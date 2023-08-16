#!/bin/sh -e
pattern="$1"
shift
exec v "\b$pattern\b" "$@"
