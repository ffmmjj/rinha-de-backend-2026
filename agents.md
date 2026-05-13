# agents

This file is consumed by **pi** (the coding agent) and documents the project
conventions, constraints, and file-level summaries so the agent can work
effectively across sessions.

## Project

**rinha-de-backend** – HTTP server in C23 for fraud-score evaluation.

## Dependencies

- **h2o** (libh2o-evloop) – event-loop HTTP/1.1 server library.
  Also requires OpenSSL and libuv headers (brew-installed).
- **cmake** – Build system generator (bundled with CLion, or installed via
  Homebrew / apt). Not in default PATH on this machine — use the Makefile
  which points to the CLion-bundled cmake, or add it to PATH.
- All other deps: POSIX standard library only.

## Commands

| Action             | Command                                                      |
|--------------------|--------------------------------------------------------------|
| Configure & Build  | `make build` (or just `make`)                                |
| Run (foreground)   | `make run`                                                   |
| Run (background)   | `make run-bg`                                                |
| Stop               | `make stop`                                                  |
| Test all endpoints | `make test`                                                  |
| Benchmark          | `make benchmark`                                             |
| Clean              | `make clean`                                                 |

Manual equivalent:
```bash
PKG_CONFIG_PATH="/opt/homebrew/opt/h2o/lib/pkgconfig:/opt/homebrew/opt/openssl@3/lib/pkgconfig:/opt/homebrew/lib/pkgconfig" \
  cmake -B cmake-build-debug
cmake --build cmake-build-debug
./cmake-build-debug/rinha_de_backend
```

## Conventions

- Language: C23 (set via `CMAKE_C_STANDARD 23`).
- Style: K&R indentation, `snake_case` for names.
- Server: h2o with internal evloop (single-threaded, event-driven, no libuv).
- No interactive input needed – runs until killed (SIGINT/SIGTERM).
- **After any structural change** (splitting files, refactoring modules,
  adding/removing endpoints), always run `make test` to verify the server
  builds and all endpoints respond correctly.

## Endpoints

| Method | Path            | Status | Response                                    |
|--------|-----------------|--------|---------------------------------------------|
| GET    | `/ready`        | 200    | `{"status":"ok"}`                           |
| POST   | `/fraud-score`  | 200    | `{"approved":false,"fraud_score":1.0}`      |
| *      | *anything else* | 404    | `{"error":"not_found"}`                     |

## File map

- `src/main.c` – Entry point: loads dataset, starts h2o server.
- `src/server.h` / `src/server.c` – h2o server lifecycle (evloop, socket listen,
  accept callback) and global `g_dataset` definition.
  Uses `#define H2O_USE_LIBUV 0` before including `<h2o.h>`.
- `src/routes.h` / `src/routes.c` – h2o endpoint handlers (`handle_ready`,
  `handle_fraud_score`, `handle_not_found`) with shared `respond_json` helper.
  Uses `#define H2O_USE_LIBUV 0` before including `<h2o.h>`.
- `src/transaction.h` / `src/transaction.c` – Transaction struct + JSON parsing.
- `src/parse.h` / `src/parse.c` – Minimal JSON field extractor (string, number, bool, array).
- `src/features.h` / `src/features.c` – 14D feature extraction from a transaction.
- `src/fraud.h` / `src/fraud.c` – 5-NN fraud detection via cosine similarity (brute-force).
- `src/dataset.h` / `src/dataset.c` – Binary dataset load (mmap, 8-bit quantized vectors,
  pre-computed reference norms at startup).
- `CMakeLists.txt` – CMake build definition (finds h2o via pkg-config).
- `Makefile` – Convenience targets: `build`, `run`, `test`, `benchmark`, `clean`, docker.
- `agents.md` – This file.
