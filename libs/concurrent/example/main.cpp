#include "../tqtconcurrent.h"

#include <ntqapplication.h>
#include <ntqobject.h>

#include <stdio.h>

static TQVariant work_fn(void* arg) {
    int n = arg ? *(int*)arg : 0;
    long long acc = 0;
    for (int i = 0; i < n; ++i) acc += i;
    return TQVariant((double)acc);
}

class Runner : public TQObject {
    TQ_OBJECT
public:
    Runner(TQtFuture* f) : future(f) {
        connect(future, SIGNAL(finished(const TQVariant&)), this, SLOT(onFinished(const TQVariant&)));
        connect(future, SIGNAL(error(const TQString&)), this, SLOT(onError(const TQString&)));
    }

public slots:
    void onFinished(const TQVariant& v) {
        printf("result=%f\n", v.toDouble());
        tqApp->quit();
    }

    void onError(const TQString& s) {
        fprintf(stderr, "error: %s\n", s.utf8().data());
    }

private:
    TQtFuture* future;
};

int main(int argc, char** argv) {
    TQApplication app(argc, argv);

    int n = 1000000;
    TQtFuture* f = TQtConcurrent::run(work_fn, &n);
    Runner r(f);

    int rc = app.exec();
    TQtConcurrent::shutdown();
    return rc;
}

#include "concurrent_main.moc"
