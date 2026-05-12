# agents

This file is consumed by **pi** (the coding agent) and documents the project
conventions, constraints, and file-level summaries so the agent can work
effectively across sessions.

## Project

**rinha-de-backend** – HTTP server in C23 for fraud-score evaluation.

## Dependencies

- **libmicrohttpd** (GNU) – lightweight HTTP/1.1 server library.
  Found via `pkg-config` at build time. No vendored source.
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
| Clean              | `make clean`                                                 |

Manual equivalent:

```bash
PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig" cmake -B cmake-build-debug
cmake --build cmake-build-debug
./cmake-build-debug/rinha_de_backend
```

## Conventions

- Language: C23 (set via `CMAKE_C_STANDARD 23`).
- Style: K&R indentation, `snake_case` for names.
- libmicrohttpd is configured with `MHD_USE_AUTO_INTERNAL_THREAD`
  + thread pool (4 workers) for concurrent request handling.
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

- `CMakeLists.txt` – CMake build definition (finds libmicrohttpd via pkg-config).
- `Makefile` – Convenience targets: `build`, `run`, `run-bg`, `stop`, `test`, `clean`.
- `main.c` – Entry point: starts the server and sleep-loops until killed.
- `server.h` / `server.c` – Server lifecycle (start/stop) and request dispatcher (routes method+URL to handlers).
- `routes.h` / `routes.c` – Individual endpoint handlers (`handle_ready`, `handle_fraud_score`, `handle_not_found`) with a shared `respond_json` helper.
- `agents.md` – This file.
