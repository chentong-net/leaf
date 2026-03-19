#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC_DIR="${LEAF_ASSETS_SRC:-${ROOT_DIR}/application/assets}"
DST_DIR="${LEAF_ASSETS_DST:-${ROOT_DIR}/ios/Leaf_iOS/Assets}"

if [[ ! -d "${SRC_DIR}" ]]; then
  echo "error: source assets directory not found: ${SRC_DIR}" >&2
  exit 1
fi

mkdir -p "${DST_DIR}"

if command -v rsync >/dev/null 2>&1; then
  rsync -a --delete \
    --exclude '.DS_Store' \
    "${SRC_DIR}/" "${DST_DIR}/"
else
  find "${DST_DIR}" -mindepth 1 -maxdepth 1 -exec rm -rf {} +
  cp -R "${SRC_DIR}/." "${DST_DIR}/"
  find "${DST_DIR}" -name '.DS_Store' -delete
fi

echo "synced iOS assets:"
echo "  source: ${SRC_DIR}"
echo "  target: ${DST_DIR}"