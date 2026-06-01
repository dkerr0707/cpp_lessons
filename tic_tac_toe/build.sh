#!/usr/bin/env bash
set -euo pipefail

mkdir -p bin
g++ -std=c++17 -Wall -Wextra -o bin/tic_tac_toe tic_tac_toe.cpp
echo "Built ./bin/tic_tac_toe"
