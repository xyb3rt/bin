#!/bin/sh
rg --files -u | rg -F "$1"
