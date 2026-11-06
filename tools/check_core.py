#!/usr/bin/env python3
"""Require the tested installed Arduino core before building firmware."""
import argparse
import json
import os
import re
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('core', help='Required platform and version, e.g. esp8266:esp8266@3.1.2')
    parser.add_argument('--cli', default=os.environ.get('ARDUINO_CLI', 'arduino-cli'))
    args = parser.parse_args()
    if not re.fullmatch(r'[A-Za-z0-9_-]+:[A-Za-z0-9_-]+@[^\s@]+', args.core):
        parser.error('Expected a platform:architecture@version core identifier')
    platform, version = args.core.rsplit('@', 1)
    try:
        result = subprocess.run([args.cli, 'core', 'list', '--format', 'json'],
                                check=True, capture_output=True, text=True, timeout=30)
        listing = json.loads(result.stdout)
        platforms = listing.get('platforms') if isinstance(listing, dict) else None
        if not isinstance(platforms, list):
            raise ValueError('Arduino CLI returned an unsupported core list format')
        installed = next((item.get('installed_version') for item in platforms
                          if isinstance(item, dict) and item.get('id') == platform), None)
    except (OSError, subprocess.SubprocessError, ValueError) as error:
        parser.exit(1, f'Cannot verify the installed Arduino core: {error}\n')
    if installed != version:
        parser.exit(1, f'Build requires {args.core}; installed: {installed or "not installed"}.\n'
                       f'Install the tested core with Arduino CLI before building.\n')
    print('PASS: installed Arduino core', args.core)


if __name__ == '__main__':
    main()
