#!/usr/bin/env bash

TESTS=(allocs string vector)

cd `dirname $0`

WHITE="\033[1;38m"
RESET="\033[0m"

run () {
    if [[ -r ${1}.c ]]; then
        echo -ne $WHITE
        echo "|=> $1"
        echo -ne $RESET
        cc -ggdb -o $1 ${1}.c
        ./$1
        echo -ne $WHITE
        echo "####################"
        echo -ne $RESET
    else
        echo "No such file ${1}.c"
        exit 1
    fi
}

if [[ $# -gt 0 ]]; then
    run $1
else
    for test in ${TESTS[@]}; do
        run $test
    done
fi
