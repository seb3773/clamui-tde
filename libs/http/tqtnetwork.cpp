#include "tqtnetwork.h"

#include <ntqapplication.h>
#include <ntqdatetime.h>
#include <ntqmutex.h>

#include <pthread.h>

#include <string.h>

#include <curl/curl.h>

#include "tqtconcurrent.h"

static size_t curl_header_cb(char* buffer, size_t size, size_t nitems, void* userdata);
static int curl_xferinfo_cb(void* clientp, curl_off_t, curl_off_t, curl_off_t, curl_off_t);
static size_t curl_write_stream_cb(char* ptr, size_t size, size_t nmemb, void* userdata);

static int g_curl_global_ref = 0;

class TQtNetworkResultEvent : public TQCustomEvent {
public:
    enum { TypeId = TQEvent::User + 127 };

    TQtNetworkResultEvent() : TQCustomEvent((TQEvent::Type)TypeId) {
        httpStatus = 0;
        curlCode = 0;
    }
    int httpStatus;
    int curlCode;
    TQString error;
    TQByteArray data;
    TQMap<TQString, TQString> headers;
};

class TQtNetworkChunkEvent : public TQCustomEvent {
public:
    enum { TypeId = TQEvent::User + 126 };

    TQtNetworkChunkEvent() : TQCustomEvent((TQEvent::Type)TypeId), chunk() {}
    TQByteArray chunk;
};

class TQtNetworkProgressEvent : public TQCustomEvent {
public:
    enum { TypeId = TQEvent::User + 125 };

    TQtNetworkProgressEvent() : TQCustomEvent((TQEvent::Type)TypeId) {
        received = 0;
        total = 0;
    }

    int received;
    int total;
};

struct TQtNetworkShared {
    pthread_mutex_t lock;
    int ref;
    TQtNetworkReply* reply;

    TQtNetworkRequest req;
    int abort;

    int lastProgress;
    int lastTotal;

    TQtNetworkShared() : ref(1), reply(0), req(), abort(0) {
        pthread_mutex_init(&lock, 0);
        lastProgress = -1;
        lastTotal = -1;
    }

    ~TQtNetworkShared() {
        pthread_mutex_destroy(&lock);
    }
};

struct TQtNetworkReply::Private {
    TQMutex lock;

    int finished;
    int httpStatus;
    int curlCode;

    TQString error;
    TQByteArray data;
    TQMap<TQString, TQString> headers;

    TQtNetworkShared* shared;
};

static inline void shared_ref(TQtNetworkShared* s) {
    __sync_add_and_fetch(&s->ref, 1);
}

static inline void shared_unref(TQtNetworkShared* s) {
    if (__sync_sub_and_fetch(&s->ref, 1) == 0) {
        delete s;
    }
}

static size_t curl_write_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    TQByteArray* out = (TQByteArray*)userdata;
    const size_t n = size * nmemb;
    if (!out || n == 0) return 0;

    const int old = out->size();
    out->resize(old + (int)n);
    memcpy(out->data() + old, ptr, n);
    return n;
}

static inline void post_reply_event(TQtNetworkShared* s, TQCustomEvent* ev) {
    if (!s || !ev) return;

    pthread_mutex_lock(&s->lock);
    TQtNetworkReply* r = s->reply;
    pthread_mutex_unlock(&s->lock);

    if (!r) {
        delete ev;
        return;
    }

    TQApplication::postEvent(r, ev);
}

static size_t curl_write_stream_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    TQtNetworkShared* s = (TQtNetworkShared*)userdata;
    const size_t n = size * nmemb;
    if (!s || !ptr || n == 0) return 0;

    if (__sync_add_and_fetch(&s->abort, 0)) return 0;

    TQtNetworkChunkEvent* ev = new TQtNetworkChunkEvent();
    ev->chunk.resize((int)n);
    memcpy(ev->chunk.data(), ptr, n);
    post_reply_event(s, ev);
    return n;
}

static size_t curl_header_cb(char* buffer, size_t size, size_t nitems, void* userdata) {
    TQMap<TQString, TQString>* hdrs = (TQMap<TQString, TQString>*)userdata;
    size_t n = size * nitems;
    if (!hdrs || n == 0) return 0;

    while (n > 0 && (buffer[n - 1] == '\n' || buffer[n - 1] == '\r')) {
        --n;
    }
    if (n == 0) return size * nitems;

    const char* colon = (const char*)memchr(buffer, ':', n);
    if (!colon) return size * nitems;

    size_t kn = (size_t)(colon - buffer);
    size_t vn = n - kn - 1;
    const char* vp = colon + 1;

    while (vn > 0 && (*vp == ' ' || *vp == '\t')) {
        ++vp;
        --vn;
    }

    TQString k = TQString::fromUtf8(buffer, (int)kn).lower();
    TQString v = TQString::fromUtf8(vp, (int)vn);

    if (k == "set-cookie" && hdrs->contains(k)) {
        (*hdrs)[k] = (*hdrs)[k] + "\n" + v;
    } else {
        (*hdrs)[k] = v;
    }
    return size * nitems;
}

static int curl_xferinfo_cb(void* clientp, curl_off_t, curl_off_t dlnow, curl_off_t dltotal, curl_off_t) {
    TQtNetworkShared* s = (TQtNetworkShared*)clientp;
    if (__builtin_expect(!s, 0)) return 0;

    if (__sync_add_and_fetch(&s->abort, 0)) return 1;

    int now = 0;
    int total = 0;
    if (dlnow > 0) now = (dlnow > 0x7fffffff) ? 0x7fffffff : (int)dlnow;
    if (dltotal > 0) total = (dltotal > 0x7fffffff) ? 0x7fffffff : (int)dltotal;

    const int lastNow = __sync_add_and_fetch(&s->lastProgress, 0);
    const int lastTotal = __sync_add_and_fetch(&s->lastTotal, 0);

    int shouldPost = 0;
    if (total != lastTotal) {
        shouldPost = 1;
    } else if (now != lastNow) {
        if (now == 0 || total == 0) {
            shouldPost = ((now - lastNow) >= (64 * 1024)) ? 1 : 0;
        } else {
            const int p0 = (lastNow * 100) / total;
            const int p1 = (now * 100) / total;
            shouldPost = (p1 != p0) ? 1 : 0;
        }
    }

    if (shouldPost) {
        __sync_lock_test_and_set(&s->lastProgress, now);
        __sync_lock_test_and_set(&s->lastTotal, total);

        TQtNetworkProgressEvent* ev = new TQtNetworkProgressEvent();
        ev->received = now;
        ev->total = total;
        post_reply_event(s, ev);
    }

    return 0;
}

static void post_result_event(TQtNetworkShared* s, TQtNetworkResultEvent* ev) {
    pthread_mutex_lock(&s->lock);
    TQtNetworkReply* r = s->reply;
    pthread_mutex_unlock(&s->lock);

    if (!r) {
        delete ev;
        return;
    }

    TQApplication::postEvent(r, ev);
}

static void* worker_thread(void* arg) {
    TQtNetworkShared* s = (TQtNetworkShared*)arg;

    CURL* curl = curl_easy_init();
    if (!curl) {
        TQtNetworkResultEvent* ev = new TQtNetworkResultEvent();
        ev->curlCode = -1;
        ev->error = "curl init failed";
        post_result_event(s, ev);
        shared_unref(s);
        return 0;
    }

    const TQCString url = s->req.url().utf8();

    curl_easy_setopt(curl, CURLOPT_URL, url.data());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, s->req.followRedirects() ? 1L : 0L);
    if (s->req.followRedirects()) {
        const int maxr = s->req.maxRedirects();
        if (maxr > 0) curl_easy_setopt(curl, CURLOPT_MAXREDIRS, (long)maxr);
    }
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, curl_xferinfo_cb);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, s);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

    const int timeoutMs = s->req.timeoutMs();
    if (timeoutMs > 0) {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, (long)timeoutMs);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, (long)timeoutMs);
    }

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_stream_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, s);

    TQMap<TQString, TQString> resp_headers;
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, curl_header_cb);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &resp_headers);

    struct curl_slist* headers = 0;
    {
        const TQMap<TQString, TQString> h = s->req.rawHeaders();
        for (TQMap<TQString, TQString>::ConstIterator it = h.begin(); it != h.end(); ++it) {
            const TQCString k = it.key().utf8();
            const TQCString v = it.data().utf8();
            TQCString line = k + ": " + v;
            headers = curl_slist_append(headers, line.data());
        }
    }

    if (headers) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    const TQtNetworkRequest::Method m = s->req.method();
    const TQByteArray body = s->req.body();
    const TQCString custom = s->req.customMethod().utf8();

    if (m == TQtNetworkRequest::Post) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        if (!body.isEmpty()) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.data());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body.size());
        } else {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);
        }
    } else if (m == TQtNetworkRequest::Put) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        if (!body.isEmpty()) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.data());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body.size());
        } else {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);
        }
    } else if (m == TQtNetworkRequest::Delete) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    } else if (m == TQtNetworkRequest::Head) {
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    } else if (m == TQtNetworkRequest::Custom) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, custom.data());
        if (!body.isEmpty()) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.data());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body.size());
        }
    }

    CURLcode code = curl_easy_perform(curl);

    long httpStatus = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatus);

    if (headers) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    TQtNetworkResultEvent* ev = new TQtNetworkResultEvent();
    ev->curlCode = (int)code;
    ev->httpStatus = (int)httpStatus;
    ev->headers = resp_headers;

    if (code != CURLE_OK) {
        if (__sync_add_and_fetch(&s->abort, 0)) {
            ev->error = "aborted";
        } else {
            ev->error = curl_easy_strerror(code);
        }
    } else if (httpStatus >= 400) {
        ev->error = TQString("HTTP %1").arg((int)httpStatus);
    }

    post_result_event(s, ev);

    shared_unref(s);
    return 0;
}

static TQVariant http_worker_detached(void* arg) {
    worker_thread(arg);
    return TQVariant();
}

TQtNetworkRequest::TQtNetworkRequest()
    : m_url(),
      m_headers(),
      m_method(Get),
      m_customMethod(),
      m_body(),
      m_timeoutMs(30000),
      m_followRedirects(1),
      m_maxRedirects(10),
      m_bufferResponse(1) {}

TQtNetworkRequest::TQtNetworkRequest(const TQString& url)
    : m_url(url),
      m_headers(),
      m_method(Get),
      m_customMethod(),
      m_body(),
      m_timeoutMs(30000),
      m_followRedirects(1),
      m_maxRedirects(10),
      m_bufferResponse(1) {}

void TQtNetworkRequest::setUrl(const TQString& url) { m_url = url; }

TQString TQtNetworkRequest::url() const { return m_url; }

void TQtNetworkRequest::setRawHeader(const TQString& name, const TQString& value) {
    m_headers[name] = value;
}

TQMap<TQString, TQString> TQtNetworkRequest::rawHeaders() const { return m_headers; }

void TQtNetworkRequest::setMethod(Method m) { m_method = m; }

TQtNetworkRequest::Method TQtNetworkRequest::method() const { return m_method; }

void TQtNetworkRequest::setCustomMethod(const TQString& m) {
    m_customMethod = m;
    m_method = Custom;
}

TQString TQtNetworkRequest::customMethod() const { return m_customMethod; }

void TQtNetworkRequest::setBody(const TQByteArray& body) { m_body = body; }

TQByteArray TQtNetworkRequest::body() const { return m_body; }

void TQtNetworkRequest::setTimeoutMs(int timeoutMs) { m_timeoutMs = timeoutMs; }

int TQtNetworkRequest::timeoutMs() const { return m_timeoutMs; }

void TQtNetworkRequest::setFollowRedirects(int enable) { m_followRedirects = enable ? 1 : 0; }

int TQtNetworkRequest::followRedirects() const { return m_followRedirects; }

void TQtNetworkRequest::setMaxRedirects(int maxRedirects) { m_maxRedirects = maxRedirects; }

int TQtNetworkRequest::maxRedirects() const { return m_maxRedirects; }

void TQtNetworkRequest::setBufferResponse(int enable) { m_bufferResponse = enable ? 1 : 0; }

int TQtNetworkRequest::bufferResponse() const { return m_bufferResponse; }

TQtNetworkReply::TQtNetworkReply(TQObject* parent) : TQObject(parent) {
    d = new Private;
    d->finished = 0;
    d->httpStatus = 0;
    d->curlCode = 0;
    d->shared = 0;
}

TQtNetworkReply::~TQtNetworkReply() {
    if (d->shared) {
        __sync_lock_test_and_set(&d->shared->abort, 1);
        pthread_mutex_lock(&d->shared->lock);
        d->shared->reply = 0;
        pthread_mutex_unlock(&d->shared->lock);
        shared_unref(d->shared);
        d->shared = 0;
    }

    delete d;
    d = 0;
}

bool TQtNetworkReply::isFinished() const {
    return d->finished != 0;
}

bool TQtNetworkReply::hasError() const {
    return !d->error.isEmpty();
}

int TQtNetworkReply::httpStatusCode() const {
    return d->httpStatus;
}

TQString TQtNetworkReply::errorString() const {
    return d->error;
}

TQByteArray TQtNetworkReply::data() const {
    return d->data;
}

int TQtNetworkReply::bytesAvailable() const {
    return d->data.size();
}

TQByteArray TQtNetworkReply::readAll() {
    TQByteArray out = d->data;
    d->data.resize(0);
    return out;
}

bool TQtNetworkReply::hasRawHeader(const TQString& name) const {
    return d->headers.contains(name.lower());
}

TQString TQtNetworkReply::rawHeader(const TQString& name) const {
    TQMap<TQString, TQString>::ConstIterator it = d->headers.find(name.lower());
    if (it == d->headers.end()) return TQString();
    return it.data();
}

TQMap<TQString, TQString> TQtNetworkReply::rawHeaders() const {
    return d->headers;
}

void TQtNetworkReply::abort() {
    if (!d->shared) return;
    __sync_lock_test_and_set(&d->shared->abort, 1);
}

void TQtNetworkReply::customEvent(TQCustomEvent* e) {
    if (!e) return;

    if (e->type() == (TQEvent::Type)TQtNetworkChunkEvent::TypeId) {
        if (d->finished) return;
        TQtNetworkChunkEvent* ev = (TQtNetworkChunkEvent*)e;
        if (!ev->chunk.isEmpty()) {
            if (!d->shared || d->shared->req.bufferResponse()) {
                const int old = d->data.size();
                d->data.resize(old + ev->chunk.size());
                memcpy(d->data.data() + old, ev->chunk.data(), (size_t)ev->chunk.size());
            }
            emit readyRead();
            emit dataChunk(ev->chunk);
        }
        return;
    }

    if (e->type() == (TQEvent::Type)TQtNetworkProgressEvent::TypeId) {
        if (d->finished) return;
        TQtNetworkProgressEvent* ev = (TQtNetworkProgressEvent*)e;
        emit downloadProgress(ev->received, ev->total);
        return;
    }

    if (e->type() != (TQEvent::Type)TQtNetworkResultEvent::TypeId) return;

    TQtNetworkResultEvent* ev = (TQtNetworkResultEvent*)e;

    d->httpStatus = ev->httpStatus;
    d->curlCode = ev->curlCode;
    if (!ev->data.isEmpty()) d->data = ev->data;
    d->error = ev->error;
    d->headers = ev->headers;
    d->finished = 1;

    if (!d->error.isEmpty()) emit error(d->error);
    emit finished();
}

TQtNetworkAccessManager::TQtNetworkAccessManager(TQObject* parent) : TQObject(parent) {
    if (__sync_add_and_fetch(&g_curl_global_ref, 1) == 1) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }
}

TQtNetworkAccessManager::~TQtNetworkAccessManager() {
    if (__sync_sub_and_fetch(&g_curl_global_ref, 1) == 0) {
        curl_global_cleanup();
    }
}

TQtNetworkReply* TQtNetworkAccessManager::get(const TQtNetworkRequest& req) {
    TQtNetworkReply* r = new TQtNetworkReply(this);

    TQtNetworkShared* s = new TQtNetworkShared();
    s->req = req;
    pthread_mutex_lock(&s->lock);
    s->reply = r;
    pthread_mutex_unlock(&s->lock);

    r->d->shared = s;
    shared_ref(s);

    TQtConcurrent::runDetached(http_worker_detached, s);
    return r;
}

#include "tqtnetwork.moc"
