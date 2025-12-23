#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Building tests..."
mkdir -p build
cd build
cmake ..
make

echo ""
echo "Running tests..."
./test_led_controller
