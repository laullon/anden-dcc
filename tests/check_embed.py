#!/usr/bin/env python3
"""Verify asset generation rejects missing boundaries and preserves literal SVG text."""
from pathlib import Path
import gzip
import re
import shutil
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='anden-embed-') as directory:
    repo = Path(directory)
    firmware = repo / 'firmware/esp13-ota'
    firmware.mkdir(parents=True)
    artwork = repo / 'web/locos'
    artwork.mkdir(parents=True)
    for kind in ['electric','diesel','steam']:
        # Literal \1 and \g are legal SVG text, not regex replacement sequences.
        (artwork / (kind+'.svg')).write_text('<svg><text>\\1 \\g '+kind+'</text></svg>')
    shutil.copyfile(root/'firmware/esp13-ota/embed.py',firmware/'embed.py')
    page = firmware / 'web/index.html'
    page.parent.mkdir()
    original = '<html><!-- LOCO_ART_START -->old<!-- LOCO_ART_END --></html>'
    def run(*args):
        return subprocess.run([sys.executable,str(firmware/'embed.py'),*args],
                              capture_output=True,text=True)
    page.write_text(original)
    result=run()
    assert result.returncode == 0,result.stderr
    generated=page.read_text()
    assert '\\1 \\g electric' in generated
    assert run('--check').returncode == 0
    stored=(firmware/'SetupPage.cpp').read_text().split('{',1)[1].rsplit('}',1)[0]
    assert gzip.decompress(bytes(map(int,re.findall(r'\d+',stored)))).decode() == generated
    for invalid in ['<html></html>',original+original,
                    '<!-- LOCO_ART_END --><!-- LOCO_ART_START -->']:
        page.write_text(invalid)
        result=run()
        assert result.returncode != 0 and 'exactly one ordered' in result.stderr
        assert page.read_text() == invalid
    page.write_text(generated)
    (firmware/'SetupPage.h').unlink()
    result=run('--check')
    assert result.returncode != 0 and 'assets stale' in result.stderr and 'Traceback' not in result.stderr
    assert run().returncode == 0
    (firmware/'SetupPage.cpp').write_text('const uint8_t SETUP_PAGE[]={31,139,8,0,255};')
    result=run('--check')
    assert result.returncode != 0 and 'assets stale' in result.stderr and 'Traceback' not in result.stderr
print('PASS: asset markers, literal SVG text, gzip payload, missing interface and malformed data checks.')
