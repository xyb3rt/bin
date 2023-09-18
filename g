#!/bin/sh
rg -Hn --heading --smart-case --sort path "$@" | awk -F '^' '
	NR == 1 && ENVIRON["ACMEVIMID"] != "" {
		print ""
	}
	/^$/ {
		sep = NR
	}
	NR <= sep + 1
	NR >= sep + 2 {
		match($0, /^[0-9]+[-:]|--$/)
		pos = substr($0, 1, RLENGTH)
		txt = substr($0, RLENGTH + 1)
		printf("%7s %s\n", pos, txt)
	}'
