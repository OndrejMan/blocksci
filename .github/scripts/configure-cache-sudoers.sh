#!/usr/bin/env bash
set -euo pipefail

if [[ "${EUID}" -ne 0 ]]; then
  echo "Run this script as root on each self-hosted GitHub Actions runner." >&2
  exit 1
fi

RUNNER_USER="${1:-actions-runner}"
RUNNER_GROUP="${2:-${RUNNER_USER}}"
HELPER_PATH="/usr/local/sbin/gha-prepare-buildx-cache"
SUDOERS_PATH="/etc/sudoers.d/gha-buildx-cache"

cat > "${HELPER_PATH}" <<'SCRIPT'
#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "Usage: gha-prepare-buildx-cache <workflow-name> <amd64|arm64>" >&2
  exit 2
fi

WORKFLOW_NAME="$1"
ARCH="$2"

if [[ "${WORKFLOW_NAME}" == *"/"* || -z "${WORKFLOW_NAME}" ]]; then
  echo "Invalid workflow name" >&2
  exit 2
fi

case "${ARCH}" in
  amd64|arm64) ;;
  *)
    echo "Invalid architecture '${ARCH}'" >&2
    exit 2
    ;;
esac

RUN_AS_USER="${SUDO_USER:-${USER}}"
RUN_AS_GROUP="$(id -gn "${RUN_AS_USER}")"
CACHE_DIR="/cache/${WORKFLOW_NAME}/.buildx-cache-${ARCH}"

mkdir -p "${CACHE_DIR}"
chown -R "${RUN_AS_USER}:${RUN_AS_GROUP}" "/cache/${WORKFLOW_NAME}"
SCRIPT

chown root:root "${HELPER_PATH}"
chmod 0755 "${HELPER_PATH}"

cat > "${SUDOERS_PATH}" <<EOF
${RUNNER_USER} ALL=(root) NOPASSWD: ${HELPER_PATH}
EOF

chown root:root "${SUDOERS_PATH}"
chmod 0440 "${SUDOERS_PATH}"
visudo -cf "${SUDOERS_PATH}"

echo "Installed ${HELPER_PATH}"
echo "Installed ${SUDOERS_PATH} for user '${RUNNER_USER}'"
