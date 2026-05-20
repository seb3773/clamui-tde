/*
 * main.cpp - TDEClamUI application entry point
 *
 * ClamAV antivirus GUI for the Trinity Desktop Environment.
 * Port of ClamUI (GTK4/libadwaita) to native TQt3/TDE.
 *
 * Uses KUniqueApplication to ensure single-instance operation.
 * When a second instance is launched with --scan-profile, the
 * arguments are forwarded to the running instance via DCOP.
 * Falls back to HeadlessScanner if no display is available.
 */

#include <tdeapplication.h>
#include <tdeaboutdata.h>
#include <tdecmdlineargs.h>
#include <tdelocale.h>
#include <kuniqueapplication.h>

#include "clamui_app.h"
#include "core/headless_scanner.h"
#include "core/unofficial_sigs.h"
#include <ntqapplication.h>

#include <stdlib.h>
#include <ntqdir.h>
#include <ntqfile.h>
#include <ntqtextstream.h>

static const char description[] =
    I18N_NOOP("ClamAV Antivirus GUI for Trinity Desktop");

static const char version[] = "1.0.0";

static TDECmdLineOptions options[] =
{
    { "minimized", I18N_NOOP("Start minimized to system tray"), 0 },
    { "scan-profile <id>", I18N_NOOP("Scan using the specified profile ID"), 0 },
    { "update-additional", I18N_NOOP("Update databases (official and/or additional)"), 0 },
    { "sanesecurity", I18N_NOOP("Enable Sanesecurity signatures"), 0 },
    { "urlhaus", I18N_NOOP("Enable URLhaus database"), 0 },
    { "official", I18N_NOOP("Update official databases"), 0 },
    { "force", I18N_NOOP("Force official update"), 0 },
    { "mirror <url>", I18N_NOOP("Use specified database mirror"), 0 },
    TDECmdLineLastOption
};

#include <dirent.h>

static void injectTDEEnvironment()
{
    /* If DISPLAY is already set, we might be running normally from the terminal.
       However, systemd sometimes sets DISPLAY but misses the rest.
       If SESSION_MANAGER is set, we are definitely fully integrated. */
    if (getenv("SESSION_MANAGER") && getenv("DCOPSERVER")) return;

    /* Find tdeinit process to steal its environment */
    DIR *dir = opendir("/proc");
    if (!dir) return;

    int tdeinitPid = -1;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] >= '1' && ent->d_name[0] <= '9') {
            TQString cmdPath = TQString("/proc/%1/cmdline").arg(ent->d_name);
            TQFile cmdFile(cmdPath);
            if (cmdFile.open(IO_ReadOnly)) {
                TQByteArray cmd = cmdFile.readAll();
                TQString cmdStr(cmd.data());
                if (cmdStr.startsWith("tdeinit") || cmdStr.startsWith("/opt/trinity/bin/tdeinit")) {
                    tdeinitPid = atoi(ent->d_name);
                    cmdFile.close();
                    break;
                }
                cmdFile.close();
            }
        }
    }
    closedir(dir);

    if (tdeinitPid > 0) {
        TQString envPath = TQString("/proc/%1/environ").arg(tdeinitPid);
        TQFile envFile(envPath);
        if (envFile.open(IO_ReadOnly)) {
            TQByteArray envData = envFile.readAll();
            int i = 0;
            while (i < (int)envData.size()) {
                TQString envStr(envData.data() + i);
                if (envStr.isEmpty()) break;
                
                int eqIdx = envStr.find('=');
                if (eqIdx > 0) {
                    TQString key = envStr.left(eqIdx);
                    TQString val = envStr.mid(eqIdx + 1);
                    /* Inject critical session variables */
                    if (key == "DISPLAY" || key == "XAUTHORITY" || key == "ICEAUTHORITY" ||
                        key == "TDEHOME" || key == "TDEDIR" || key == "SESSION_MANAGER" ||
                        key == "DCOPSERVER" || key == "DBUS_SESSION_BUS_ADDRESS" || key == "XDG_RUNTIME_DIR") {
                        setenv(key.latin1(), val.latin1(), 0);
                    } else if (key == "PATH") {
                        /* Prepend to PATH if needed */
                        TQString currentPath = getenv("PATH");
                        if (!currentPath.contains("/opt/trinity/bin")) {
                            TQString newPath = "/opt/trinity/bin:" + val;
                            setenv(key.latin1(), newPath.latin1(), 1);
                        } else {
                            setenv(key.latin1(), val.latin1(), 0);
                        }
                    }
                }
                i += envStr.length() + 1; /* Skip null terminator */
            }
            envFile.close();
        }
    }
}

int main(int argc, char **argv)
{
    // Check for --update-additional early to bypass TDE/DCOP initialization
    bool isUpdateAdditional = false;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--update-additional") == 0) {
            isUpdateAdditional = true;
            break;
        }
    }

    if (isUpdateAdditional) {
        TDEAboutData about(
            "tdeclamui",                          /* appName */
            I18N_NOOP("TDE ClamUI"),              /* programName */
            version,                               /* version */
            description,                           /* shortDescription */
            TDEAboutData::License_MIT,            /* licenseType */
            "(C) 2026, TDE ClamUI Contributors"   /* copyrightStatement */
        );
        TDECmdLineArgs::init(argc, argv, &about);
        TDECmdLineArgs::addCmdLineOptions(options);
        TDEApplication app(false /* no GUI */);
        TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();
        bool enableSanesecurity = args->isSet("sanesecurity");
        bool enableUrlhaus = args->isSet("urlhaus");
        bool official = args->isSet("official");
        bool force = args->isSet("force");
        TQString mirror = args->getOption("mirror");

        UnofficialSigsUpdater updater;
        updater.setSanesecurityEnabled(enableSanesecurity);
        updater.setUrlhausEnabled(enableUrlhaus);
        TQStringList logOutput;

        bool officialOk = true;
        if (official) {
            officialOk = updater.runOfficialUpdate(logOutput, force, mirror);
        }
        bool additionalOk = true;
        if (enableSanesecurity || enableUrlhaus) {
            additionalOk = updater.runUpdate(logOutput, force);
        }
        return (officialOk && additionalOk) ? 0 : 1;
    }

    injectTDEEnvironment();
    
    TDEAboutData about(
        "tdeclamui",                          /* appName */
        I18N_NOOP("TDE ClamUI"),              /* programName */
        version,                               /* version */
        description,                           /* shortDescription */
        TDEAboutData::License_MIT,            /* licenseType */
        "(C) 2026, TDE ClamUI Contributors",  /* copyrightStatement */
        0,                                     /* text */
        "https://github.com/seb3773",         /* homePageAddress */
        "bugs@tdeclamui.org"                   /* bugsEmailAddress */
    );

    about.addAuthor("seb3773", 0, 0);

    TDECmdLineArgs::init(argc, argv, &about);
    TDECmdLineArgs::addCmdLineOptions(options);
    KUniqueApplication::addCmdLineOptions();

    /*
     * Check if DCOP/display is available for single-instance mode.
     * If not (headless cron/systemd without session), fall back
     * to HeadlessScanner for non-interactive scan.
     */
    TDECmdLineArgs *preArgs = TDECmdLineArgs::parsedArgs();
    
    TQString earlyProfileId = preArgs->getOption("scan-profile");

    if (!KUniqueApplication::start()) {
        /*
         * Another instance is already running.
         * KUniqueApplication has forwarded our args via DCOP.
         * The running instance's newInstance() will handle it.
         */
        return 0;
    }

    /*
     * If we reach here with --scan-profile but no DISPLAY,
     * fall back to headless mode.
     */
    if (!earlyProfileId.isEmpty() && !getenv("DISPLAY")) {
        TDEApplication app(false /* no GUI */);
        HeadlessScanner scanner(earlyProfileId, &app);
        scanner.start();
        return app.exec();
    }

    ClamUIApp app;
    int ret = app.exec();
    return ret;
}
