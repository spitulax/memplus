#!/usr/bin/env bash

set -uo pipefail

CCARGS="-x c -std=c23 -Wconversion -Wsign-conversion -Wpedantic -Wall -Wextra -I. -I.. -ggdb"
if [[ "${QUIET:-}" -eq 1 ]]; then
    CCARGS+=" -DQUIET"
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
            cc $CCARGS -o ".build/${f%.c}" "$f"
            if [[ $? -eq 0 ]]; then
                local start=$(date +%s%N)
                "./.build/${f%.c}"
                local done=$(date +%s%N)
                local dur_ns=$(($done - $start))
                local dur_us=$(($dur_ns / 1000))
                if [[ $? -eq 0 ]]; then
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
    TESTS=($(find -maxdepth 1 -mindepth 1 '(' -type d -or -type f -name '*.c' ')' -and ! -name ".*"))
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
