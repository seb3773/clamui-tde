# http

Files:

- `tqtnetwork.h` / `tqtnetwork.cpp`: minimal async HTTP wrapper for TQt3.
- Backend: libcurl.

API:

- `TQtNetworkAccessManager` with `get(const TQtNetworkRequest&)`.
- `TQtNetworkRequest`: URL + raw headers + timeout.
- `TQtNetworkReply`: emits `finished()` and `error(const TQString&)`.

Notes:

- Requests are executed in a detached pthread.
- Results are delivered to the UI thread using `TQApplication::postEvent()`.

Limitations:

- One thread per request (no pooling).
- Response body is buffered in memory (no streaming / `readyRead()` yet).
- `abort()` is best-effort: it sets a flag checked via libcurl progress callback.
- If a `TQtNetworkReply` is destroyed while the request is running, the request is aborted and the result event is dropped.
