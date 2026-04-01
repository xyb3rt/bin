#!/bin/sh -e
cmd="$1"
shift
cmd="$(realpath "$cmd")"
dir="${cmd%/*}"
case "$dir" in
*/x) ;;
*) echo "Not an x script: $cmd" >&2; exit 1;;
esac
cd "$dir/.."
[ -r x/env.sh ] && . x/env.sh
exec /bin/sh -e "$cmd" "$@"
