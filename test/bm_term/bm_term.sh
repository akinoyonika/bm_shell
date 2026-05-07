#! /bin/bash

bm_term_path=/usr/local/libexec/bm_term
if [ "`basename $0`" == "bm_term.sh" ]; then bm_term_path=./bm_term; fi

mkfifo /tmp/$$_pipe0
mkfifo /tmp/$$_pipe1

read -ei "mono" -p "FONT: " FONT
read -ei 3      -p "MODE: " OPERATION_MODE
read -ei 16     -p "LHPX: " LINE_HEIGHT_PX
read -ei 8      -p "CWPX: " COLUMN_WIDTH_PX
read -ei 24     -p "ROWS: " LINES
read -ei 80     -p "COLS: " COLUMNS
read -e         -p "USER: " user
read -e        -sp "PASS: " pass

bm_term_opt="
	-f \"${FONT}\"
	-m ${OPERATION_MODE}
	-h ${LINE_HEIGHT_PX}
	-w ${COLUMN_WIDTH_PX}
	-r ${LINES}
	-c ${COLUMNS}
	-u ${user}
	-p ${pass}
"

echo ${bm_term_opt} > /tmp/$$_pipe1 &

es_ccat $* -s < /tmp/$$_pipe0 | ${bm_term_path} /tmp/$$_pipe1 > /tmp/$$_pipe0

rm /tmp/$$_pipe1
rm /tmp/$$_pipe0
