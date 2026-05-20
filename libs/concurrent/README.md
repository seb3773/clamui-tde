# concurrent

Files:

- `tqtconcurrent.h` / `tqtconcurrent.cpp`: minimal threadpool + future wrapper for TQt3.

API:

- `TQtConcurrent::run(TQtConcurrentRunnable*)`
- `TQtConcurrent::run(TQtConcurrentFunction, void* arg)`
- `TQtConcurrent::setMaxThreadCount(int)`
- `TQtConcurrent::shutdown()`
- `TQtFuture` emits `finished(const TQVariant&)` / `error(const TQString&)`.

Notes:

- Results are delivered to the UI thread using `TQApplication::postEvent()`.
- One global pool is created on first use. Use `TQtConcurrent::shutdown()` to stop/join threads.
