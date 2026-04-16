#!/usr/bin/env bash

CCARGS="-x c -std=c23 -Wconversion -Wsign-conversion -Wpedantic -Wall -Wextra -I. -I.."
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
            "./.build/${f%.c}"
            if [[ $? -eq 0 ]]; then
                echo -e "${GREEN}>> $f successful${RESET}"
            else
                echo -e "${RED}>> $f failed${RESET}"
                FAILED+=("$f")
            fi
        else
            echo -e "${YELLOW}WARNING: Unknown/unaccessible file \`$f\`${RESET}" >&2
        fi
        echo
    done
}

mkdir -p .build &>/dev/null

cd `dirname $0`

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
