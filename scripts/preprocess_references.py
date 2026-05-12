#!/usr/bin/env python3
"""
Pre-process references.json.gz into a compact binary format for C.

Input:  resources/references.json.gz  (~48 MB gzipped, ~280 MB JSON)
Output: resources/references.bin       (~172 MB raw binary)

Binary layout (little-endian):
  [0..7]      uint64_t  N  (number of entries, 3_000_000)
  [8..]       repeated entries:
                [0..55]   float[14]  vector
                [56]      uint8_t    label  (0 = legit, 1 = fraud)
"""

import gzip
import json
import struct
import sys
import os

SRC = os.path.join(os.path.dirname(__file__), "..", "resources", "references.json.gz")
DST = os.path.join(os.path.dirname(__file__), "..", "resources", "references.bin")

VECTOR_LEN = 14

def main():
    print(f"Reading {SRC} ...")
    with gzip.open(SRC, "rt", encoding="utf-8") as f:
        data = json.load(f)

    n = len(data)
    print(f"Entries: {n}")
    print(f"Writing {DST} ...")

    with open(DST, "wb") as out:
        # header: number of entries
        out.write(struct.pack("<Q", n))

        for i, entry in enumerate(data):
            vec = entry["vector"]
            label = entry["label"]

            # validate
            if len(vec) != VECTOR_LEN:
                print(f"ERROR: entry {i} has vector length {len(vec)}, expected {VECTOR_LEN}")
                sys.exit(1)

            # float vector (14 × 4 = 56 bytes)
            out.write(struct.pack("<14f", *vec))

            # label: 0 = legit, 1 = fraud
            out.write(struct.pack("<B", 1 if label == "fraud" else 0))

            if (i + 1) % 500_000 == 0:
                print(f"  {i+1}/{n}")

    size_mb = os.path.getsize(DST) / (1024 * 1024)
    print(f"Done. {DST}  ({size_mb:.1f} MB)")


if __name__ == "__main__":
    main()
