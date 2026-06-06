#!/usr/bin/env bash
set -euo pipefail

if [[ "${EUID}" -ne 0 ]]; then
  echo "Run this script as root on each self-hosted GitHub Actions runner." >&2
  exit 1
fi

if [[ $# -ge 1 ]]; then
  RUNNER_USER="$1"
elif [[ -n "${SUDO_USER:-}" && "${SUDO_USER}" != "root" ]]; then
  RUNNER_USER="${SUDO_USER}"
else
  RUNNER_USER="actions-runner"
fi

if ! id -u "${RUNNER_USER}" >/dev/null 2>&1; then
  echo "Runner user '${RUNNER_USER}' does not exist." >&2
  echo "Usage: sudo bash $0 <runner-user>" >&2
  exit 1
fi

ROOT_GROUP="$(id -gn 0)"
HELPER_DIR="/usr/local/sbin"
HELPER_PATH="${HELPER_DIR}/gha-prepare-buildx-cache"
SUDOERS_PATH="/etc/sudoers.d/gha-buildx-cache"

install -d -o root -g "${ROOT_GROUP}" -m 0755 "${HELPER_DIR}"
install -d -o root -g "${ROOT_GROUP}" -m 0755 "$(dirname "${SUDOERS_PATH}")"

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
CACHE_ROOT="/var/tmp/blocksci-buildx-cache"
CACHE_DIR="${CACHE_ROOT}/${WORKFLOW_NAME}/.buildx-cache-${ARCH}"

mkdir -p "${CACHE_DIR}"
chown -R "${RUN_AS_USER}:${RUN_AS_GROUP}" "${CACHE_ROOT}/${WORKFLOW_NAME}"
SCRIPT

chown root:"${ROOT_GROUP}" "${HELPER_PATH}"
chmod 0755 "${HELPER_PATH}"

cat > "${SUDOERS_PATH}" <<EOF
${RUNNER_USER} ALL=(root) NOPASSWD: ${HELPER_PATH}
EOF

chown root:"${ROOT_GROUP}" "${SUDOERS_PATH}"
chmod 0440 "${SUDOERS_PATH}"
visudo -cf "${SUDOERS_PATH}"

echo "Installed ${HELPER_PATH}"
echo "Installed ${SUDOERS_PATH} for user '${RUNNER_USER}'"
echo "Granted: ${RUNNER_USER} ALL=(root) NOPASSWD: ${HELPER_PATH}"
