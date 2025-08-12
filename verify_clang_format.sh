#!/bin/bash
cd "$(dirname "$0")"

set -e

find client/ -iname *.hpp -o -iname *.cpp -o -iname *.inl |
    xargs clang-format --style=file --Werror --ferror-limit=1 --verbose -n

find server/ -iname *.hpp -o -iname *.cpp -o -iname *.inl |
    xargs clang-format --style=file --Werror --ferror-limit=1 --verbose -n

find share/ -iname *.hpp -o -iname *.cpp -o -iname *.inl |
    xargs clang-format --style=file --Werror --ferror-limit=1 --verbose -n
