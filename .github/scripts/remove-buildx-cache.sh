#!/usr/bin/env bash
set -euo pipefail

CACHE_ROOT="${BUILDX_CACHE_ROOT:-/var/tmp/blocksci-buildx-cache}"

usage() {
  cat >&2 <<EOF
Usage:
  sudo bash $0 --all
  sudo bash $0 <workflow-name>
  sudo bash $0 <workflow-name> <amd64|arm64>

Removes Buildx local cache under:
  ${CACHE_ROOT}
EOF
}

if [[ $# -lt 1 || $# -gt 2 ]]; then
  usage
  exit 2
fi

if [[ "$1" == "--all" ]]; then
  if [[ $# -ne 1 ]]; then
    usage
    exit 2
  fi

  TARGET="${CACHE_ROOT}"
else
  WORKFLOW_NAME="$1"

  if [[ -z "${WORKFLOW_NAME}" || "${WORKFLOW_NAME}" == *"/"* ]]; then
    echo "Invalid workflow name" >&2
    exit 2
  fi

  TARGET="${CACHE_ROOT}/${WORKFLOW_NAME}"

  if [[ $# -eq 2 ]]; then
    ARCH="$2"
    case "${ARCH}" in
      amd64|arm64) ;;
      *)
        echo "Invalid architecture '${ARCH}'" >&2
        exit 2
        ;;
    esac

    TARGET="${TARGET}/.buildx-cache-${ARCH}"
  fi
fi

if [[ "${TARGET}" != "${CACHE_ROOT}" && "${TARGET}" != "${CACHE_ROOT}/"* ]]; then
  echo "Refusing to remove path outside cache root: ${TARGET}" >&2
  exit 1
fi

if [[ ! -e "${TARGET}" ]]; then
  echo "Nothing to remove: ${TARGET}"
  exit 0
fi

rm -rf "${TARGET}"
echo "Removed ${TARGET}"
