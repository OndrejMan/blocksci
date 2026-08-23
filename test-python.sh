#!/bin/bash

set -e

cd /mnt/blocksci/test/blockscipy
uv run --project /blocksci --locked pytest
