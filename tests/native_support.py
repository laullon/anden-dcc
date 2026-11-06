"""Production translation units shared by native firmware checks."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def library_sources():
    return [str(path) for path in sorted((ROOT / 'libraries/AndenStackLink/src').glob('*.cpp'))]
