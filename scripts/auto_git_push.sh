#!/usr/bin/env bash

# Script tự động:
# - git add .
# - git commit với message có timestamp
# - git push origin master
# Chỉ chạy 1 lần mỗi khi bạn gọi script.

BRANCH="${1:-master}"
MESSAGE_PREFIX="auto-commit"

# Lấy đường dẫn gốc repo (thư mục cha của scripts)
REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

cd "$REPO_DIR" || {
  echo "Không cd được vào repo: $REPO_DIR"
  exit 1
}

# Kiểm tra có gì thay đổi không
if ! git status --porcelain | grep -q .; then
  echo "[auto-git] Không có thay đổi nào để commit."
  exit 0
fi

echo "[auto-git] Đang add, commit và push lên origin/$BRANCH..."

git add .

TIMESTAMP="$(date '+%Y-%m-%d %H:%M:%S')"
COMMIT_MSG="$MESSAGE_PREFIX $TIMESTAMP"

if ! git commit -m "$COMMIT_MSG"; then
  echo "[auto-git] Commit thất bại."
  exit 1
fi

if ! git push origin "$BRANCH"; then
  echo "[auto-git] Push thất bại."
  exit 1
fi

echo "[auto-git] Hoàn tất: đã push lên origin/$BRANCH với message: $COMMIT_MSG"

