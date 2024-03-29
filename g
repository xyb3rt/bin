#!/bin/sh
rg -Hn --heading --smart-case --sort path "$@" | awk -F '^' '
	/^$/ {
		sep = NR
	}
	NR == sep + 1 {
		if (!match($0, /^\.?\//)) {
			$0 = "./" $0
		}
		print
	}
	NR >= sep + 2 {
		match($0, /^[0-9]+[-:]|--$/)
		pos = substr($0, 1, RLENGTH)
		txt = substr($0, RLENGTH + 1)
		printf("%8s%s\n", pos, txt)
	}'
