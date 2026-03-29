#!/bin/bash
set -e

cd "$(dirname "$0")/../base"
./make linux vulkan
