#!/usr/bin/env bash

set -euo pipefail

repository_root="${1:?repository root is required}"
platforms="${2:?target platforms are required}"
force_rebuild="${3:-false}"
key_version="${4:-1}"
run_id="${5:-}"

dockerfile="${repository_root}/Dockerfile"
requirements="${repository_root}/pip-all-requirements.txt"
dockerignore="${repository_root}/.dockerignore"
# Single quotes: the boundary is a literal Dockerfile line, not something
# the shell should expand.
stage_boundary='FROM ${DEPS_IMAGE} AS complete'

boundary_count="$(awk -v boundary="$stage_boundary" '$0 == boundary { count++ } END { print count + 0 }' "$dockerfile")"
if [ "$boundary_count" -ne 1 ]; then
    echo "Expected exactly one '$stage_boundary' boundary in $dockerfile; found $boundary_count." >&2
    exit 1
fi

for dependency_input in "$requirements" "$dockerignore"; do
    if [ ! -f "$dependency_input" ]; then
        echo "Dependency image input is missing: $dependency_input" >&2
        exit 1
    fi
done

dependency_hash="$(
    {
        printf 'dependency-key-version\0%s\0' "$key_version"
        printf 'Dockerfile:dependencies\0'
        # Exact-line match rather than a regex: the boundary now contains ${...},
        # which no regex dialect should have to interpret.
        awk -v boundary="$stage_boundary" '$0 == boundary { exit } { print }' "$dockerfile"
        printf '\0pip-all-requirements.txt\0'
        cat "$requirements"
        printf '\0.dockerignore\0'
        cat "$dockerignore"
        printf '\0platforms\0%s\0' "$platforms"
    } | sha256sum | awk '{ print $1 }'
)"

dependency_tag="deps-${dependency_hash}"
case "$force_rebuild" in
    true)
        if [ -z "$run_id" ]; then
            echo "A run ID is required for a forced dependency rebuild." >&2
            exit 1
        fi
        dependency_tag="${dependency_tag}-run-${run_id}"
        ;;
    false|"")
        ;;
    *)
        echo "force_rebuild must be true or false, got: $force_rebuild" >&2
        exit 1
        ;;
esac

printf '%s\n' "$dependency_tag"
