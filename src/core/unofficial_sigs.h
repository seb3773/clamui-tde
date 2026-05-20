/*
 * unofficial_sigs.h - ClamAV unofficial signatures downloader and verifier
 */

#ifndef CLAM_UNOFFICIAL_SIGS_H
#define CLAM_UNOFFICIAL_SIGS_H

#include <ntqobject.h>
#include <ntqstring.h>
#include <ntqstringlist.h>

class IncrementalLogger
{
public:
    IncrementalLogger(TQStringList &list) : m_list(list) {}
    
    IncrementalLogger& operator<<(const TQString &str) {
        m_list.append(str);
        printf("%s\n", str.latin1());
        fflush(stdout);
        return *this;
    }
    
    IncrementalLogger& operator<<(const char *str) {
        m_list.append(str);
        printf("%s\n", str);
        fflush(stdout);
        return *this;
    }

private:
    TQStringList &m_list;
};

class UnofficialSigsUpdater : public TQObject
{
    TQ_OBJECT

public:
    UnofficialSigsUpdater(TQObject *parent = 0);
    ~UnofficialSigsUpdater();

    void setSanesecurityEnabled(bool enabled) { m_sanesecurityEnabled = enabled; }
    void setUrlhausEnabled(bool enabled) { m_urlhausEnabled = enabled; }

    // Runs the update sequence. Returns true on success.
    // Writes progress and log messages to logOutput.
    bool runUpdate(TQStringList &logOutput, bool force = false);
    
    // Runs the official freshclam update workflow in C++.
    bool runOfficialUpdate(TQStringList &logOutput, bool force = false, const TQString &mirror = TQString::null);

private:
    TQString m_workDir;
    TQString m_clamavDir;
    TQString m_gpgDir;
    bool m_sanesecurityEnabled;
    bool m_urlhausEnabled;

    bool downloadFile(const TQString &url, const TQString &outputPath, IncrementalLogger &logOutput);
    bool runCommand(const TQString &cmd, const TQStringList &args, IncrementalLogger &logOutput, int *exitCode = 0);
};

#endif // CLAM_UNOFFICIAL_SIGS_H
