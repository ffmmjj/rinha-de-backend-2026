#!/usr/bin/env python3
"""
Analyze quantization error for the references dataset.

Loads references.bin and computes per-dimension min/max, then
simulates uint8 and uint16 quantization and reports error stats.
"""

import struct
import numpy as np
import os

SRC = os.path.join(os.path.dirname(__file__), "..", "resources", "references.bin")

VECTOR_LEN = 14
ENTRY_BYTES = 60  # 56 bytes float[14] + 1 byte label + 3 padding

def load_vectors():
    with open(SRC, "rb") as f:
        n = struct.unpack("<Q", f.read(8))[0]
        vectors = np.zeros((n, VECTOR_LEN), dtype=np.float32)
        labels = np.zeros(n, dtype=np.uint8)
        for i in range(n):
            vec = struct.unpack("<14f", f.read(14 * 4))
            label = struct.unpack("<B", f.read(1))[0]
            f.read(3)  # padding
            vectors[i] = vec
            labels[i] = label
    return vectors, labels

def quantize_uniform(data, bits):
    """Uniform quantization to given bits, returns quantized ints + scale/offset."""
    dims = data.shape[1]
    qmin = 0
    qmax = (1 << bits) - 1

    mins = np.min(data, axis=0)
    maxs = np.max(data, axis=0)

    # Avoid division by zero for constant dimensions
    ranges = maxs - mins
    ranges[ranges == 0] = 1.0

    quantized = np.round((data - mins) / ranges * qmax).astype(np.uint16 if bits > 8 else np.uint8)
    quantized = np.clip(quantized, qmin, qmax)

    recovered = quantized.astype(np.float32) / qmax * ranges + mins

    return quantized, recovered, mins, maxs

def main():
    print("Loading vectors...")
    vectors, labels = load_vectors()
    n = len(vectors)
    print(f"Loaded {n} vectors")

    # Stats per dimension
    print("\n=== Per-dimension statistics ===")
    for d in range(VECTOR_LEN):
        vals = vectors[:, d]
        print(f"  dim {d:2d}: min={vals.min():10.6f}  max={vals.max():10.6f}  "
              f"mean={vals.mean():10.6f}  std={vals.std():10.6f}")

    print("\n=== Quantization analysis ===")
    for bits in [8, 16]:
        q, recovered, mins, maxs = quantize_uniform(vectors, bits)

        abs_err = np.abs(vectors - recovered)
        rel_err = abs_err / (np.abs(vectors) + 1e-10)

        print(f"\n--- {bits}-bit quantization ---")
        print(f"  Per-dimension MAE:")
        for d in range(VECTOR_LEN):
            print(f"    dim {d:2d}: MAE={abs_err[:, d].mean():.6f}  "
                  f"max_abs_err={abs_err[:, d].max():.6f}  "
                  f"mean_rel_err={rel_err[:, d].mean()*100:.3f}%")

        print(f"\n  Global:")
        print(f"    MAE:         {abs_err.mean():.6f}")
        print(f"    RMSE:        {np.sqrt((abs_err**2).mean()):.6f}")
        print(f"    Max abs err: {abs_err.max():.6f}")
        print(f"    Mean rel err:{rel_err.mean()*100:.3f}%")
        print(f"    Storage per vector: {VECTOR_LEN * (bits // 8)} bytes  "
              f"(was {VECTOR_LEN * 4} bytes)")

        # Show worst-case dimension
        dim_mae = abs_err.mean(axis=0)
        worst_dim = np.argmax(dim_mae)
        print(f"    Worst dim: {worst_dim} (MAE={dim_mae[worst_dim]:.6f})")

    # Show a few sample recoveries for the worst dimension
    d_worst = np.argmax(np.abs(vectors - recovered).mean(axis=0))
    print(f"\n=== Sample recoveries (dim {d_worst}, 8-bit) ===")
    q8, r8, _, _ = quantize_uniform(vectors, 8)
    for i in range(5):
        orig = vectors[i, d_worst]
        recovered = r8[i, d_worst]
        print(f"  orig={orig:.6f}  recovered={recovered:.6f}  "
              f"error={abs(orig-recovered):.6f}")


if __name__ == "__main__":
    main()
