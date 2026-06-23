/*
 * clamui_app.cpp - KUniqueApplication subclass implementation
 */

#include "clamui_app.h"
#include "ui/mainwindow.h"
#include "tray.h"
#include "core/settings_manager.h"
#include "core/headless_scanner.h"

#include <tdecmdlineargs.h>
#include <tdelocale.h>

ClamUIApp::ClamUIApp()
    : KUniqueApplication(), m_mainWindow(0), m_tray(0), m_firstInstance(true)
{
}

ClamUIApp::~ClamUIApp()
{
}

int ClamUIApp::newInstance()
{
    TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();

    TQString scanProfileId = args->getOption("scan-profile");
    bool startMinimized = args->isSet("minimized");

    if (m_firstInstance) {
        m_firstInstance = false;

        /* Create main window */
        m_mainWindow = new ClamMainWindow();

        /* Create system tray */
        m_tray = new ClamTray(m_mainWindow);
        setMainWidget(m_mainWindow);
        m_tray->show();
        m_mainWindow->setTray(m_tray);

        if (!scanProfileId.isEmpty()) {
            /*
             * Launched by cron/systemd with --scan-profile:
             * - Force minimized (only tray visible)
             * - Start the scheduled scan
             * - Mark for auto-quit when scan finishes
             */
            m_mainWindow->hide();
            m_mainWindow->startScheduledScan(scanProfileId);
        } else if (startMinimized ||
                   (m_mainWindow->settingsManager() &&
                    m_mainWindow->settingsManager()->value("start_minimized", "false") == "true")) {
            m_mainWindow->hide();
        } else {
            m_mainWindow->show();
        }
    } else {
        /*
         * Second invocation — arguments forwarded via DCOP.
         * The running instance handles the request.
         */
        if (!scanProfileId.isEmpty()) {
            if (m_mainWindow) {
                /* App was already running — start scan but do NOT auto-quit */
                m_mainWindow->startScheduledScan(scanProfileId, false);
            }
        } else {
            /* No scan-profile: bring window to front */
            if (m_mainWindow) {
                m_mainWindow->show();
                m_mainWindow->raise();
            }
        }
    }

    args->clear();
    return 0;
}

#include "clamui_app.moc"
