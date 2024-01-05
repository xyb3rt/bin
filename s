#!/bin/sh -e
pattern="$1"
sub="$2"
shift 2
files="$(rg -l "$pattern" "$@")"
[ -n "$files" ]
tmp="$(mktemp ".s.XXXXXX")"
cleanup() {
	rm -f "$tmp"
}
. trap.sh
files="$(echo "$files" | sort | ped "$tmp")"
echo "$files" | while IFS='' read -r file; do
	rg --passthru -r "$sub" "$pattern" "$file" | diff -u "$file" - || :
done >"$tmp"
[ -s "$tmp" ] && ${EDITOR:-ed} "$tmp" && patch -up0 <"$tmp"
