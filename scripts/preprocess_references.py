#!/usr/bin/env python3
"""
Pre-process references.json.gz into an 8-bit quantized binary format.

Input:  resources/references.json.gz  (~48 MB gzipped, ~280 MB JSON)
Output: resources/references.bin       (~45 MB binary)

Binary layout (little-endian):
  [0..7]       uint64_t  N  (number of entries, 3_000_000)
  [8..63]      float[14] dim_mins    (per-dimension minimum values)
  [64..119]    float[14] dim_ranges  (per-dimension max - min)
  [120..]      repeated entries:
                 [0..13]   uint8_t[14]  quantized vector
                 [14]      uint8_t      label (0 = legit, 1 = fraud)

Decoding (in C):
    recovered = quantized / 255.0 * range + min
"""

import gzip
import json
import struct
import sys
import os
import numpy as np

SRC = os.path.join(os.path.dirname(__file__), "..", "resources", "references.json.gz")
DST = os.path.join(os.path.dirname(__file__), "..", "resources", "references.bin")

VECTOR_LEN = 14
QMAX = 255.0

def main():
    print(f"Reading {SRC} ...")
    with gzip.open(SRC, "rt", encoding="utf-8") as f:
        data = json.load(f)

    n = len(data)
    print(f"Entries: {n}")

    # Extract vectors as numpy array for easy min/max computation
    vectors = np.array([entry["vector"] for entry in data], dtype=np.float64)
    labels = np.array([1 if entry["label"] == "fraud" else 0 for entry in data], dtype=np.uint8)

    # Per-dimension min and range
    dim_mins = np.min(vectors, axis=0).astype(np.float32)
    dim_maxs = np.max(vectors, axis=0).astype(np.float32)
    dim_ranges = dim_maxs - dim_mins
    dim_ranges[dim_ranges == 0] = 1.0  # avoid division by zero

    print(f"Dim mins:  {list(dim_mins)}")
    print(f"Dim maxs:  {list(dim_maxs)}")

    # Quantize
    quantized = np.round((vectors - dim_mins) / dim_ranges * QMAX).astype(np.uint8)

    print(f"Writing {DST} ...")
    with open(DST, "wb") as out:
        # Header: count
        out.write(struct.pack("<Q", n))
        # Per-dimension mins and ranges
        out.write(struct.pack("<14f", *dim_mins))
        out.write(struct.pack("<14f", *dim_ranges))
        # Entries
        for i in range(n):
            out.write(struct.pack("<14BB", *quantized[i], labels[i]))
            if (i + 1) % 500_000 == 0:
                print(f"  {i+1}/{n}")

    size_mb = os.path.getsize(DST) / (1024 * 1024)
    print(f"Done. {DST}  ({size_mb:.1f} MB)")

    # Quick error estimate
    recovered = quantized.astype(np.float64) / QMAX * dim_ranges + dim_mins
    abs_err = np.abs(vectors - recovered)
    print(f"\nQuantization error (8-bit):")
    print(f"  MAE:  {abs_err.mean():.6f}")
    print(f"  RMSE: {np.sqrt((abs_err**2).mean()):.6f}")
    print(f"  Max:  {abs_err.max():.6f}")


if __name__ == "__main__":
    main()
