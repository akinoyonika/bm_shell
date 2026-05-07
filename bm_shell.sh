#! /bin/bash

bm_shell_path=/usr/local/libexec/bm_shell
if [ "`basename $0`" == "bm_shell.sh" ]; then bm_shell_path=./bm_shell; fi

es_gw $* ${bm_shell_path}
