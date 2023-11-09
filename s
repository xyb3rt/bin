#!/bin/sh -e
pattern="$1"
sub="$2"
shift 2
files="$(rg -l "$pattern" "$@")"
[ -n "$files" ]
diff="/tmp/s.$$"
cleanup() {
	rm -f "$diff"
}
. trap.sh
echo "$files" | sort | ped | while IFS='' read -r file; do
	rg --passthru -r "$sub" "$pattern" "$file" | diff -u "$file" - || :
done >"$diff"
[ -s "$diff" ] && ${EDITOR:-ed} "$diff" && patch -up0 <"$diff"
