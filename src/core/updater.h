/*
 * updater.h - Database updater using tdesudo and TQProcess
 */

#ifndef CLAM_UPDATER_H
#define CLAM_UPDATER_H

#include <ntqobject.h>
#include <ntqstring.h>
#include <ntqprocess.h>

class Updater : public TQObject
{
    TQ_OBJECT

public:
    enum UpdateStatus {
        SUCCESS,
        ERROR,
        CANCELLED,
        SERVICE_RUNNING
    };

    Updater(TQObject *parent = 0);
    ~Updater();

    void startUpdate(bool force = false);
    void cancelUpdate();
    bool isRunning() const;

    /* Apply settings before calling startUpdate() */
    void setDatabaseMirror(const TQString &mirror) {
        TQString temp = mirror;
        if (temp.startsWith("https://")) {
            temp = temp.mid(8);
        } else if (temp.startsWith("http://")) {
            temp = temp.mid(7);
        }
        m_mirror = temp;
    }
    void setChecksPerDay(int checks) { m_checksPerDay = checks; }
    bool isServiceRunning() const;

signals:
    void updateProgress(const TQString &message);
    void updateFinished(int status);

private slots:
    void processOutput();
    void processError();
    void processExited();
    void slotTimeout();

private:
    TQProcess *m_process;
    bool m_cancelled;
    bool m_runningUnofficial;
    bool m_officialFailed;
    TQString m_mirror;
    int m_checksPerDay;
    class TQTimer *m_timeoutTimer;
    
    TQString findFreshclam() const;
    void sendSignalToService();
};

#endif /* CLAM_UPDATER_H */
