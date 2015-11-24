#!/bin/bash
# ./pdfdot.sh filename.dot
args=("$@")
dotfile=${args[0]}

dot -T pdf -o ${dotfile}.pdf ${dotfile}

