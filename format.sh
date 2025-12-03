#!/bin/env bash

FLAGS="-i"

if  [[ $1 = "-c" ]]; then
    echo "Checking style..."
    FLAGS="--dry-run --Werror"
fi

find ./src/ -iname '*.hpp' -o -iname '*.cpp' | xargs clang-format ${FLAGS}