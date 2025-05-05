cd "$(dirname "$0")"
X="$PWD"
up_env() {
	while :; do
		[ -e env.sh ] && set -- "$PWD/env.sh" "$@"
		[ -d .git ] && break
		[ "$PWD" = / ] && { echo "$0: not a project" >&2; exit 1; }
		cd ..
	done
	for rc; do
		. "$rc"
	done
}
up_env
