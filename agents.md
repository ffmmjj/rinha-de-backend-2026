# agents

This file is consumed by **pi** (the coding agent) and documents the project
conventions, constraints, and file-level summaries so the agent can work
effectively across sessions.

## Project

**rinha-de-backend** – HTTP server in C23 for fraud-score evaluation.

## Dependencies

- **libmicrohttpd** (GNU) – lightweight HTTP/1.1 server library.
  Found via `pkg-config` at build time. No vendored source.
- **h2o** (libh2o-evloop) – event-loop HTTP/1.1 server library (alternative build).
  Also requires OpenSSL and libuv headers (brew-installed).
- **cmake** – Build system generator (bundled with CLion, or installed via
  Homebrew / apt). Not in default PATH on this machine — use the Makefile
  which points to the CLion-bundled cmake, or add it to PATH.
- All other deps: POSIX standard library only.

## Commands

| Action             | Command                                                      |
|--------------------|--------------------------------------------------------------|
| Configure & Build  | `make build` (or just `make`)                                |
| Run (MHD)          | `make run` / `make run-bg`                                   |
| Run (h2o)          | `make run-h2o` / `make run-h2o-bg`                           |
| Stop               | `make stop`                                                  |
| Test all (MHD)     | `make test`                                                  |
| Test all (h2o)     | `make test-h2o`                                              |
| Benchmark both     | `make benchmark`                                             |
| Clean              | `make clean`                                                 |

Manual equivalent (MHD):
```bash
PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig" cmake -B cmake-build-debug
cmake --build cmake-build-debug
./cmake-build-debug/rinha_de_backend
```

Manual equivalent (h2o):
```bash
PKG_CONFIG_PATH="/opt/homebrew/opt/h2o/lib/pkgconfig:/opt/homebrew/opt/openssl@3/lib/pkgconfig:/opt/homebrew/lib/pkgconfig" \
  cmake -B cmake-build-debug
cmake --build cmake-build-debug --target rinha_de_backend_h2o
./cmake-build-debug/rinha_de_backend_h2o
```

## Conventions

- Language: C23 (set via `CMAKE_C_STANDARD 23`).
- Style: K&R indentation, `snake_case` for names.
- MHD server: `MHD_USE_AUTO_INTERNAL_THREAD` + thread pool (4 workers).
- h2o server: single-threaded evloop (no libuv).
- No interactive input needed – runs until killed (SIGINT/SIGTERM).
- **After any structural change** (splitting files, refactoring modules,
  adding/removing endpoints), always run `make test` and `make test-h2o`
  to verify both servers build and all endpoints respond correctly.

## Endpoints

| Method | Path            | Status | Response                                    |
|--------|-----------------|--------|---------------------------------------------|
| GET    | `/ready`        | 200    | `{"status":"ok"}`                           |
| POST   | `/fraud-score`  | 200    | `{"approved":false,"fraud_score":1.0}`      |
| *      | *anything else* | 404    | `{"error":"not_found"}`                     |

## File map

### MHD server (original)
- `src/main.c` – Entry point: loads dataset, starts MHD daemon, sleep-loops.
- `src/server.h` / `src/server.c` – MHD server lifecycle and request dispatcher.
- `src/routes.h` / `src/routes.c` – Endpoint handlers + `respond_json` helper.

### h2o server (alternative)
- `src/main_h2o.c` – Entry point: loads dataset, starts h2o server.
- `src/server_h2o.h` / `src/server_h2o.c` – h2o server lifecycle (evloop, socket listen,
  accept callback) and global `g_dataset` definition.
- `src/routes_h2o.h` / `src/routes_h2o.c` – h2o endpoint handlers via `h2o_handler_t.on_req`.
  Uses `#define H2O_USE_LIBUV 0` before including `<h2o.h>`.

### Shared core
- `src/transaction.h` / `src/transaction.c` – Transaction struct + JSON parsing.
- `src/parse.h` / `src/parse.c` – Minimal JSON field extractor (string, number, bool, array).
- `src/features.h` / `src/features.c` – 14D feature extraction from a transaction.
- `src/fraud.h` / `src/fraud.c` – 5-NN fraud detection via cosine similarity (brute-force).
- `src/dataset.h` / `src/dataset.c` – Binary dataset load (mmap, 8-bit quantized vectors).

### Build
- `CMakeLists.txt` – CMake build: MHD and h2o targets.
- `Makefile` – Convenience targets: `build`, `run`, `run-h2o`, `test`, `test-h2o`,
  `benchmark`, `clean`, docker targets.
- `agents.md` – This file.
