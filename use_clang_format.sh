#!/bin/bash
cd "$(dirname "$0")"

find client/ -iname *.hpp -o -iname *.cpp -o -iname *.inl |
    xargs clang-format --style=file --verbose -i

find server/ -iname *.hpp -o -iname *.cpp -o -iname *.inl |
    xargs clang-format --style=file --verbose -i

find share/ -iname *.hpp -o -iname *.cpp -o -iname *.inl |
    xargs clang-format --style=file --verbose -i
