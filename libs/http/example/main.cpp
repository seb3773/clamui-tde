#include "../tqtnetwork.h"

#include "tqtconcurrent.h"

#include <ntqapplication.h>
#include <ntqobject.h>

#include <stdio.h>

class Runner : public TQObject {
    TQ_OBJECT
public:
    Runner(TQtNetworkReply* r) : reply(r) {
        connect(reply, SIGNAL(finished()), this, SLOT(onFinished()));
        connect(reply, SIGNAL(error(const TQString&)), this, SLOT(onError(const TQString&)));
    }

public slots:
    void onFinished() {
        printf("status=%d\n", reply->httpStatusCode());
        if (reply->hasRawHeader("content-type")) {
            printf("content-type: %s\n", reply->rawHeader("content-type").utf8().data());
        }
        const TQByteArray b = reply->data();
        fwrite(b.data(), 1, (size_t)b.size(), stdout);
        fputc('\n', stdout);
        tqApp->quit();
    }

    void onError(const TQString& msg) {
        fprintf(stderr, "error: %s\n", msg.utf8().data());
    }

private:
    TQtNetworkReply* reply;
};

int main(int argc, char** argv) {
    TQApplication app(argc, argv);

    const TQString url = (argc > 1) ? TQString::fromLocal8Bit(argv[1]) : TQString("https://example.com/");

    TQtNetworkAccessManager nam;
    TQtNetworkRequest req(url);
    req.setTimeoutMs(30000);
    req.setRawHeader("User-Agent", "tqt3-http-test/1.0");

    TQtNetworkReply* reply = nam.get(req);
    Runner runner(reply);

    int rc = app.exec();
    TQtConcurrent::shutdown();
    return rc;
}

#include "main.moc"
