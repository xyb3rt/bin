for sig in HUP INT QUIT ABRT TERM USR1 USR2; do
	trap "cleanup; trap - $sig EXIT; kill -s $sig $$" $sig || :
done
trap cleanup EXIT
