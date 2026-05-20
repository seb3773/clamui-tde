#ifndef TQTNETWORK_H
#define TQTNETWORK_H

#include <ntqobject.h>
#include <ntqevent.h>
#include <ntqstring.h>
#include <ntqmap.h>
#include <ntqcstring.h>

class TQtNetworkReply;

class TQtNetworkRequest {
public:
    enum Method {
        Get,
        Post,
        Put,
        Delete,
        Head,
        Custom
    };

    TQtNetworkRequest();
    explicit TQtNetworkRequest(const TQString& url);

    void setUrl(const TQString& url);
    TQString url() const;

    void setRawHeader(const TQString& name, const TQString& value);
    TQMap<TQString, TQString> rawHeaders() const;

    void setMethod(Method m);
    Method method() const;
    void setCustomMethod(const TQString& m);
    TQString customMethod() const;

    void setBody(const TQByteArray& body);
    TQByteArray body() const;

    void setTimeoutMs(int timeoutMs);
    int timeoutMs() const;

    void setFollowRedirects(int enable);
    int followRedirects() const;
    void setMaxRedirects(int maxRedirects);
    int maxRedirects() const;

    void setBufferResponse(int enable);
    int bufferResponse() const;

private:
    TQString m_url;
    TQMap<TQString, TQString> m_headers;
    Method m_method;
    TQString m_customMethod;
    TQByteArray m_body;
    int m_timeoutMs;

    int m_followRedirects;
    int m_maxRedirects;
    int m_bufferResponse;
};

class TQtNetworkReply : public TQObject {
    TQ_OBJECT
public:
    TQtNetworkReply(TQObject* parent = 0);
    ~TQtNetworkReply();

    bool isFinished() const;
    bool hasError() const;

    int httpStatusCode() const;
    TQString errorString() const;
    TQByteArray data() const;

    int bytesAvailable() const;
    TQByteArray readAll();

    bool hasRawHeader(const TQString& name) const;
    TQString rawHeader(const TQString& name) const;
    TQMap<TQString, TQString> rawHeaders() const;

    void abort();

signals:
    void readyRead();
    void dataChunk(const TQByteArray& chunk);
    void downloadProgress(int received, int total);
    void finished();
    void error(const TQString& message);

protected:
    void customEvent(TQCustomEvent* e);

private:
    struct Private;
    Private* d;

    friend class TQtNetworkAccessManager;
};

class TQtNetworkAccessManager : public TQObject {
    TQ_OBJECT
public:
    TQtNetworkAccessManager(TQObject* parent = 0);
    ~TQtNetworkAccessManager();

    TQtNetworkReply* get(const TQtNetworkRequest& req);
};

#endif
