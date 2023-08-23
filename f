#!/bin/sh
find . | sed 's;^\./;;' | rg "$1[^/]*$" | sort
