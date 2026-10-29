#!/usr/bin/env python3
"""Measure the GP2 DCC prototype from a Saleae transition CSV (no dependencies)."""
import argparse
import collections
import csv
import json
import statistics
from pathlib import Path


def decode(path, channel=0):
    with Path(path).open() as source:
        reader = csv.DictReader(source)
        rows = [(float(row['Time [s]']), int(row[f'Channel {channel}'])) for row in reader]
    # First row represents capture start, not a measured edge. Last incomplete
    # half-bit is also excluded. Match high/low pairs on actual rising edges.
    bits, times, invalid = [], [], []
    widths = {0: [], 1: []}
    symmetry = []
    for index in range(1, len(rows) - 2):
        if [rows[index + j][1] for j in range(3)] != [1, 0, 1]:
            continue
        high = (rows[index + 1][0] - rows[index][0]) * 1e6
        low = (rows[index + 2][0] - rows[index + 1][0]) * 1e6
        # Transmitter one limits, and a tight nominal-zero window for this test.
        bit = (1 if all(55 <= value <= 61 for value in (high, low)) and abs(high-low) <= 3
               else 0 if all(95 <= value <= 105 for value in (high, low)) else None)
        bits.append(bit)
        times.append(rows[index][0])
        if bit is None:
            invalid.append({'time_s': rows[index][0], 'high_us': high, 'low_us': low})
        else:
            widths[bit].extend((high, low))
            symmetry.append(abs(high-low))
    packets, starts, preambles = [], [], []
    index = run = 0
    while index < len(bits):
        if bits[index] == 1:
            run += 1
            index += 1
            continue
        if bits[index] == 0 and run >= 14 and index + 27 < len(bits):
            cursor, data, valid = index + 1, [], True
            for byte in range(3):
                chunk = bits[cursor:cursor+8]
                valid &= None not in chunk
                value = 0
                for bit in chunk:
                    value = (value << 1) | (bit or 0)
                data.append(value)
                cursor += 8
                if byte < 2:
                    valid &= bits[cursor] == 0
                    cursor += 1
            valid &= bits[cursor] == 1
            if valid:
                packets.append(data)
                starts.append(times[index])
                preambles.append(run)
                index, run = cursor, 0
                continue
        index, run = index + 1, 0
    counter = collections.Counter(' '.join(f'{byte:02X}' for byte in packet) for packet in packets)
    stats = {}
    for bit, values in widths.items():
        if values:
            stats[str(bit)] = {key: round(value, 6) for key, value in
                               [('min_us', min(values)), ('median_us', statistics.median(values)), ('max_us', max(values))]}
    periods = [(b-a)*1e6 for a, b in zip(starts, starts[1:])]
    return {'channel': channel, 'duration_s': rows[-1][0]-rows[0][0] if rows else 0,
            'complete_packets': len(packets), 'packets': dict(counter),
            'checksum_errors': sum(a ^ b ^ c != 0 for a, b, c in packets),
            'invalid_bit_pairs': len(invalid), 'invalid_examples': invalid[:5],
            'half_bits': stats, 'max_half_bit_asymmetry_us': round(max(symmetry, default=0), 6),
            'preamble_lengths': dict(collections.Counter(preambles)),
            'median_packet_period_us': round(statistics.median(periods), 6) if periods else None}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('csv', type=Path)
    parser.add_argument('--channel', type=int, default=0)
    parser.add_argument('--expected', help='Expected three hex bytes, e.g. "03 80 83"')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    report = decode(args.csv, args.channel)
    report['passed'] = (report['complete_packets'] > 0 and not report['checksum_errors'] and
                        not report['invalid_bit_pairs'])
    if args.expected:
        expected = ' '.join(f'{int(byte,16):02X}' for byte in args.expected.split())
        report['passed'] &= set(report['packets']) == {expected}
    encoded = json.dumps(report, indent=2) + '\n'
    if args.output:
        args.output.write_text(encoded)
    print(encoded, end='')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
