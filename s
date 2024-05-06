#!/bin/sh -e
pat="$1"
sub="$2"
shift 2
[ "${0##*/}" = sw ] && w=-w || w=
files="$(rg -Fl $w "$pat" "$@")"
[ -n "$files" ]
tmp="$(mktemp ".s.XXXXXX")"
cleanup() {
	rm -f "$tmp"
}
. trap.sh
files="$(echo "$files" | sort | ped "$tmp")"
echo "$files" | while IFS='' read -r file; do
	rg -F --passthru -r "$sub" $w "$pat" "$file" | diff -u "$file" - || :
done >"$tmp"
[ -s "$tmp" ] && ${EDITOR:-ed} "$tmp" && patch -up0 <"$tmp"
