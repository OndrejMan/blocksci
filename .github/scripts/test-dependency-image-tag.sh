#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repository_root="$(cd "${script_dir}/../.." && pwd)"
tag_script="${script_dir}/dependency-image-tag.sh"
test_root="$(mktemp -d)"
trap 'rm -rf "$test_root"' EXIT

copy_inputs() {
    cp "${repository_root}/Dockerfile" "$test_root/Dockerfile"
    cp "${repository_root}/pip-all-requirements.txt" "$test_root/pip-all-requirements.txt"
    cp "${repository_root}/.dockerignore" "$test_root/.dockerignore"
}

compute_tag() {
    bash "$tag_script" "$test_root" "$@"
}

assert_equal() {
    if [ "$1" != "$2" ]; then
        echo "Expected equal values, got '$1' and '$2'." >&2
        exit 1
    fi
}

assert_not_equal() {
    if [ "$1" = "$2" ]; then
        echo "Expected different values, got '$1'." >&2
        exit 1
    fi
}

platforms="linux/amd64,linux/arm64"
copy_inputs
baseline="$(compute_tag "$platforms" false 1)"

printf '\n# simulated complete-stage change\n' >> "$test_root/Dockerfile"
assert_equal "$baseline" "$(compute_tag "$platforms" false 1)"

copy_inputs
awk '{ print } /^FROM ubuntu:20.04 AS dependencies$/ { print "# simulated dependency-stage change" }' \
    "$repository_root/Dockerfile" > "$test_root/Dockerfile"
assert_not_equal "$baseline" "$(compute_tag "$platforms" false 1)"

copy_inputs
printf '\n# simulated requirement change\n' >> "$test_root/pip-all-requirements.txt"
assert_not_equal "$baseline" "$(compute_tag "$platforms" false 1)"

copy_inputs
assert_not_equal "$baseline" "$(compute_tag 'linux/amd64' false 1)"
assert_equal "${baseline}-run-123-2" "$(compute_tag "$platforms" true 1 '123-2')"

grep -vF 'FROM ${DEPS_IMAGE} AS complete' "$repository_root/Dockerfile" > "$test_root/Dockerfile"
if compute_tag "$platforms" false 1 >/dev/null 2>&1; then
    echo "Expected a missing stage boundary to fail." >&2
    exit 1
fi

echo "dependency image tag tests passed"
