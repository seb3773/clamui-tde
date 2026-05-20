/*
 * clamui_app.h - KUniqueApplication subclass for single-instance support
 *
 * Ensures only one instance of TDEClamUI runs at a time.
 * When a second instance is launched with --scan-profile, the arguments
 * are forwarded to the running instance via DCOP automatically.
 */

#ifndef CLAM_APP_H
#define CLAM_APP_H

#include <kuniqueapplication.h>

class ClamMainWindow;
class ClamTray;

class ClamUIApp : public KUniqueApplication
{
    TQ_OBJECT

public:
    ClamUIApp();
    ~ClamUIApp();

    int newInstance();

private:
    ClamMainWindow *m_mainWindow;
    ClamTray *m_tray;
    bool m_firstInstance;
};

#endif /* CLAM_APP_H */
