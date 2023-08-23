#!/bin/sh -e
pattern="$1"
shift
exec s "\b$pattern\b" "$@"
