# process

Files:

- `tqtprocess.h` / `tqtprocess.cpp`: `TQtProcess`.
- `example/main.cpp`: demonstrates running `/bin/echo`, running `/bin/cat` with stdin write+close, working directory + environment, and a wait timeout case.

Purpose:

A minimal `QProcess`-like helper for TQt3 that runs a child process asynchronously, captures stdout/stderr, optionally writes to stdin, and notifies completion via TQt events/signals.

Public API:

- Configuration (must be done before `start()`)
  - `setWorkingDirectory(dir)` (ignored if already running)
  - `setEnvironment(env)` where `env` is a list of `"KEY=VALUE"` strings (ignored if already running)

- Execution
  - `start(program, arguments)`
  - `bool waitForFinished(int timeoutMs = -1)`

- State
  - `bool isRunning() const`
  - `bool isFinished() const`
  - `int exitCode() const`
  - `TQString errorString() const`

- I/O
  - `TQByteArray readAllStandardOutput()` (returns and clears internal buffer)
  - `TQByteArray readAllStandardError()` (returns and clears internal buffer)
  - `int write(const TQByteArray& data)` writes to child stdin (non-blocking fd)
  - `closeWriteChannel()` closes stdin pipe

- Termination
  - `terminate()` sends `SIGTERM`
  - `kill()` sets abort flag and sends `SIGKILL`

Signals:

- `finished(int exitCode)`
- `error(const TQString& message)`

Implementation model (key points):

- `start()`:
  - creates 3 pipes (stdin/stdout/stderr)
  - `fork()` + in child:
    - `dup2()` pipes to fd 0/1/2
    - optional `chdir(workingDir)`
    - if environment list is non-empty: `clearenv()` then `putenv()` entries (each duplicated and leaked intentionally as required by `putenv()` semantics)
    - `execvp(program, argv)`
  - parent keeps non-blocking pipe ends and spawns a detached worker via `TQtConcurrent::runDetached()`.

- Worker thread:
  - loops with `select()` on stdout/stderr and `waitpid(WNOHANG)` until both pipes are closed and the child status is known
  - reads into internal `TQByteArray` buffers (stdout/stderr)
  - posts a `TQCustomEvent` back to the `TQtProcess` object containing exit code and captured output.

- Main thread completion:
  - `customEvent()` stores `exitCode/out/err`, closes stdin, releases shared state, wakes `waitForFinished()`, emits `error()` (if any) and `finished()`.

- `waitForFinished(timeoutMs)`:
  - processes Qt events (`tqApp->processEvents()`) while waiting.
  - On timeout, sets error string `"timed out"`, sends SIGTERM, then escalates to SIGKILL if still not finished.

Usage:

See `process/example/main.cpp` for complete usage patterns.

Build:

- Example target: `tqtprocess_example`

```sh
cmake -S . -B build
cmake --build build -j --target tqtprocess_example
./build/tqtprocess_example
```

Limitations / notes:

- Captures stdout/stderr entirely in memory; no streaming callbacks.
- Only supports a full environment override (if you call `setEnvironment()`, it clears the environment in the child).
- `start()` silently does nothing if already running.
- Requires `concurrent/` (`TQtConcurrent::runDetached()`), pthreads, and Unix `fork/exec`.
