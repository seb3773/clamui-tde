#include "../tqtprocess.h"

#include "tqtconcurrent.h"

#include <ntqapplication.h>
#include <ntqobject.h>

#include <stdio.h>
#include <string.h>

class Runner : public TQObject {
    TQ_OBJECT
public:
    Runner(TQtProcess* p) : proc(p) {
        connect(proc, SIGNAL(finished(int)), this, SLOT(onFinished(int)));
        connect(proc, SIGNAL(error(const TQString&)), this, SLOT(onError(const TQString&)));
    }

public slots:
    void onFinished(int code) {
        printf("exit=%d\n", code);
        const TQByteArray out = proc->readAllStandardOutput();
        const TQByteArray err = proc->readAllStandardError();
        if (!out.isEmpty()) {
            fwrite(out.data(), 1, (size_t)out.size(), stdout);
            fputc('\n', stdout);
        }
        if (!err.isEmpty()) {
            fprintf(stderr, "stderr: ");
            fwrite(err.data(), 1, (size_t)err.size(), stderr);
            fputc('\n', stderr);
        }
        tqApp->quit();
    }

    void onError(const TQString& msg) {
        fprintf(stderr, "error: %s\n", msg.utf8().data());
    }

private:
    TQtProcess* proc;
};

int main(int argc, char** argv) {
    TQApplication app(argc, argv);

    TQtProcess p;
    Runner r(&p);

    if (argc > 1 && strcmp(argv[1], "--wait-timeout") == 0) {
        TQString prog = "/bin/sh";
        TQStringList args;
        args << "-c" << "sleep 5";
        p.start(prog, args);
        const bool ok = p.waitForFinished(200);
        printf("waitOk=%d\n", ok ? 1 : 0);
        if (!ok) fprintf(stderr, "error: %s\n", p.errorString().utf8().data());
        TQtConcurrent::shutdown();
        return ok ? 0 : 3;
    }

    if (argc > 1 && strcmp(argv[1], "--envwd") == 0) {
        p.setWorkingDirectory("/");
        TQStringList env;
        env << "TQT_PROCESS_EXAMPLE=1";
        p.setEnvironment(env);

        TQString prog = "/bin/sh";
        TQStringList args;
        args << "-c" << "pwd; echo $TQT_PROCESS_EXAMPLE";
        p.start(prog, args);
        p.waitForFinished(3000);
        const TQByteArray out = p.readAllStandardOutput();
        if (!out.isEmpty()) fwrite(out.data(), 1, (size_t)out.size(), stdout);
        const TQByteArray err = p.readAllStandardError();
        if (!err.isEmpty()) fwrite(err.data(), 1, (size_t)err.size(), stderr);
        TQtConcurrent::shutdown();
        return 0;
    }

    TQString prog = "/bin/echo";
    TQStringList args;
    args << "hello" << "from" << "tqtprocess";

    if (argc > 1) {
        prog = TQString::fromLocal8Bit(argv[1]);
        args.clear();
        for (int i = 2; i < argc; ++i) args << TQString::fromLocal8Bit(argv[i]);
    }

    if (prog == "/bin/cat" || prog.endsWith("/cat")) {
        p.start(prog, args);
        const char* msg = "hello from stdin\n";
        const int n = (int)strlen(msg);
        TQByteArray in;
        in.resize(n);
        memcpy(in.data(), msg, (size_t)n);
        p.write(in);
        p.closeWriteChannel();
    } else {
        p.start(prog, args);
    }

    int rc = app.exec();
    TQtConcurrent::shutdown();
    return rc;
}

#include "process_main.moc"
