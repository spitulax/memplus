#!/usr/bin/env bash

set -uo pipefail

# Env vars
QUIET=${QUIET:-}
WINDOWS=${WINDOWS:-}
COMPILER=${COMPILER:-clang}
SANITIZER=${SANITIZER:-1}
if [[ $WINDOWS -eq 1 ]]; then
    CSTD=${CSTD:-c11}
else
    CSTD=${CSTD:-c99}
fi
export WINEDEBUG=-all

CCARGS="-x c -std=$CSTD -Wconversion -Wsign-conversion -Wpedantic -Wall -Wextra -I. -I.. -ggdb -Og"
if [[ $SANITIZER -eq 1 ]]; then
    CCARGS+=" -fsanitize=address -fsanitize=undefined"
fi

# MSVC does not have c99 option
MSVCARGS="/nologo /std:$CSTD /I. /I.. /TC"

if [[ $QUIET -eq 1 ]]; then
    CCARGS+=" -DQUIET"
    MSVCARGS+=" /DQUIET"
fi

ACCENT="\033[35m"
RED="\033[31m"
GREEN="\033[32m"
YELLOW="\033[33m"
RESET="\033[0m"

FAILED=()

run () {
    local f
    for f in "$@"; do
        if [[ -d "$f" ]]; then
            echo "DIR: $f"
            echo -e "${YELLOW}Directory test not implemented${RESET}" >&2
            exit 1
        elif [[ -r "$f" ]]; then
            echo -e "${ACCENT}=== Running $f ===${RESET}"
            local name=".build/${f%.c}"
            if [[ $WINDOWS -eq 1 ]]; then
                cl $MSVCARGS /Fo:"$name.obj" /Fe:"$name.exe" "$f"
            else
                $COMPILER $CCARGS -o "$name" "$f"
            fi
            if [[ $? -eq 0 ]]; then
                local start=$(date +%s%N)
                if [[ $WINDOWS -eq 1 ]]; then
                    wine "$name.exe"
                else
                    "$name"
                fi
                local status=$?
                local done=$(date +%s%N)
                local dur_ns=$(($done - $start))
                local dur_us=$(($dur_ns / 1000))
                if [[ $status -eq 0 ]]; then
                    echo -e "${GREEN}>> $f successful in ${dur_us} microseconds${RESET}"
                else
                    echo -e "${RED}>> $f failed in ${dur_us} microseconds${RESET}"
                    FAILED+=("$f")
                fi
            else
                echo -e "${RED}>> failed to compile $f${RESET}"
                FAILED+=("$f")
            fi
        else
            echo -e "${YELLOW}warning: Unknown/unaccessible file \`$f\`${RESET}" >&2
        fi
        echo
    done
}

cd `dirname $0`

mkdir -p .build &>/dev/null

if [[ $# -gt 0 ]]; then
    TESTS=("$@")
else
    TESTS=($(find -maxdepth 1 -mindepth 1 '(' -type d -or -type f -name '*.c' ')' -and ! -name ".*" | sort -r))
fi

run "${TESTS[@]}"

if [[ ${#FAILED[@]} -eq 0 ]]; then
    COL="$GREEN"
else
    COL="$RED"
fi
echo -e "${COL}${#FAILED[@]}/${#TESTS[@]} test(s) failed${RESET}"
for failed in "${FAILED[@]}"; do
    echo -e "${RED}- $failed${RESET}"
done
