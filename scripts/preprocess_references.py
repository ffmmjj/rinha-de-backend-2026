#!/usr/bin/env python3
"""
Pre-process references.json.gz into an 8-bit quantized binary format.

Streams the JSON file using ijson to keep memory usage low.
If ijson is not available, falls back to a simple bracket-counting parser.

Input:  resources/references.json.gz  (~48 MB gzipped, ~280 MB JSON)
Output: resources/references.bin       (~43 MB binary)

Binary layout (little-endian):
  [0..7]       uint64_t  N
  [8..63]      float[14] dim_mins
  [64..119]    float[14] dim_ranges
  [120..]      struct reference[N]  (each: uint8_t[14] + uint8_t label)
"""

import gzip
import json
import struct
import os
import sys

SRC = os.path.join(os.path.dirname(__file__), "..", "resources", "references.json.gz")
DST = os.path.join(os.path.dirname(__file__), "..", "resources", "references.bin")

VECTOR_LEN = 14
QMAX = 255.0

try:
    import ijson
    HAVE_IJSON = True
except ImportError:
    HAVE_IJSON = False


def iter_entries_ijson(f):
    """Stream entries using ijson."""
    for obj in ijson.items(f, "item"):
        yield obj["vector"], 1 if obj["label"] == "fraud" else 0


def iter_entries_manual(f):
    """Simple streaming parser that counts braces to find object boundaries."""
    decoder = json.JSONDecoder()
    # We'll read the file in helper_text_chunks
    buf = ""
    # Skip initial whitespace and '['
    while True:
        ch = f.read(1)
        if not ch:
            return
        if ch == '[':
            break

    depth = 0
    obj_start = None
    while True:
        ch = f.read(1)
        if not ch:
            break
        if ch == '{':
            if depth == 0:
                obj_start = len(buf)
            depth += 1
        elif ch == '}':
            depth -= 1
            if depth == 0 and obj_start is not None:
                # We have a complete object in the buffer
                obj_str = buf[obj_start:] + '}'
                buf = ""
                obj_start = None
                try:
                    obj = decoder.decode(obj_str)
                    yield obj["vector"], 1 if obj["label"] == "fraud" else 0
                except json.JSONDecodeError:
                    pass
                continue
        if depth > 0 or ch not in ' \t\n\r,':
            buf += ch


def stream_entries():
    """Yield (vector, label) tuples from the JSON file."""
    f = gzip.open(SRC, "rt", encoding="utf-8")
    try:
        if HAVE_IJSON:
            yield from iter_entries_ijson(f)
        else:
            yield from iter_entries_manual(f)
    finally:
        f.close()


def first_pass():
    """Read the JSON, compute per-dimension min/max and count."""
    print("First pass: computing min/max ...")
    mins = [float('inf')] * VECTOR_LEN
    maxs = [float('-inf')] * VECTOR_LEN
    count = 0

    for vec, _ in stream_entries():
        for i, v in enumerate(vec):
            if v < mins[i]:
                mins[i] = v
            if v > maxs[i]:
                maxs[i] = v
        count += 1
        if count % 500_000 == 0:
            print(f"  scanned {count}")

    if count == 0:
        print("ERROR: no entries found!")
        sys.exit(1)

    print(f"  total: {count}")
    print(f"  mins:  {[round(m, 4) for m in mins]}")
    print(f"  maxs:  {[round(m, 4) for m in maxs]}")
    return count, mins, maxs


def second_pass(count, mins, maxs):
    """Second pass: quantize and write binary."""
    ranges = [maxs[i] - mins[i] if maxs[i] != mins[i] else 1.0
              for i in range(VECTOR_LEN)]

    print(f"\nSecond pass: quantizing and writing ...")
    print(f"  ranges: {[round(r, 4) for r in ranges]}")

    with open(DST, "wb") as out:
        # Header
        out.write(struct.pack("<Q", count))
        out.write(struct.pack("<14f", *mins))
        out.write(struct.pack("<14f", *ranges))

        # Entries
        written = 0
        for vec, label in stream_entries():
            qvec = [round((v - mins[i]) / ranges[i] * QMAX) for i, v in enumerate(vec)]
            qvec = [max(0, min(255, q)) for q in qvec]
            out.write(struct.pack("<14BB", *qvec, label))
            written += 1
            if written % 500_000 == 0:
                print(f"  wrote {written}")

    size_mb = os.path.getsize(DST) / (1024 * 1024)
    print(f"\nDone. {DST}  ({size_mb:.1f} MB)")


def main():
    count, mins, maxs = first_pass()
    second_pass(count, mins, maxs)


if __name__ == "__main__":
    main()
