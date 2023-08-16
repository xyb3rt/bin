#!/bin/sh
rg --files | rg -F "$1"
