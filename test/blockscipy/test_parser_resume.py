"""Disk parser cursor regression; run against a rebuilt blocksci_parser."""
import json
from pathlib import Path
import shutil
import subprocess

import pytest


@pytest.mark.btc
def test_resume_cursor_survives_an_empty_preallocated_tail(tmp_path):
    parser = shutil.which("blocksci_parser")
    if parser is None:
        pytest.skip("requires the rebuilt parser")
    fixture = Path(__file__).resolve().parents[1] / "files/btc/regtest/blocks/blk00000.dat"
    source = tmp_path / "source"
    (source / "blocks").mkdir(parents=True)
    shutil.copyfile(fixture, source / "blocks/blk00000.dat")
    (source / "blocks/blk00001.dat").write_bytes(bytes(4096))
    index = tmp_path / "index"
    index.mkdir()
    config = index / "config.json"
    subprocess.run([parser, str(config), "generate-config", "bitcoin_regtest", str(index / "parsed"),
                    "--disk", str(source)], check=True, capture_output=True)
    subprocess.run([parser, str(config), "update"], check=True, capture_output=True)
    cursor_path = index / "parsed/parser/blockList.dat.resume.json"
    cursor = json.loads(cursor_path.read_text())
    assert cursor["file_number"] == 0
    assert cursor["data_offset"] + cursor["block_size"] <= fixture.stat().st_size
    # No new data: preserve the same cursor despite the empty next file.
    subprocess.run([parser, str(config), "update"], check=True, capture_output=True)
    assert json.loads(cursor_path.read_text()) == cursor
    # Missing the real resume file is an error, never a successful no-op update.
    (source / "blocks/blk00000.dat").unlink()
    failed = subprocess.run([parser, str(config), "update"], capture_output=True, text=True)
    assert failed.returncode != 0
    assert "resume block file is missing" in failed.stdout + failed.stderr
