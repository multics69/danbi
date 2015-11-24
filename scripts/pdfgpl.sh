#!/bin/bash
# ./pdfgpl.sh filename.gpl
args=("$@")
gplfile=${args[0]}

gnuplot ${gplfile} > ${gplfile}.pdf

