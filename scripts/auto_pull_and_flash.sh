#!/usr/bin/env bash

set -e

# Thư mục gốc repo (cha của scripts)
REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

cd "$REPO_DIR" || {
  exit 1
}

echo "=== Bước 1: git pull ==="
if ! git pull --rebase; then
  exit 1
fi

pio run -e esp32-s3-devkit -t upload
pio run -e esp32-s3-devkit -t uploadfs
pio device monitor -b 115200

