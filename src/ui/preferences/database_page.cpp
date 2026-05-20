/*
 * database_page.cpp - Database Preferences Page
 */

#include "database_page.h"
#include "../../core/settings_manager.h"

#include <tqlayout.h>
#include <tqlabel.h>
#include <tqcombobox.h>
#include <tqspinbox.h>
#include <tqcheckbox.h>
#include <tdelocale.h>
#include <ntqfile.h>
#include <kstandarddirs.h>
#include <tdeglobal.h>
#include <stdlib.h>
#include <tdemessagebox.h>
#include <ntqregexp.h>
#include <ntqprocess.h>
#include <sys/stat.h>
#include <ntqapplication.h>
#include <unistd.h>

DatabasePage::DatabasePage(TQWidget *parent)
    : TQWidget(parent)
{
    TQVBoxLayout *mainLayout = new TQVBoxLayout(this, 11, 6);
    
    TQHBoxLayout *mirrorLayout = new TQHBoxLayout(mainLayout, 6);
    mirrorLayout->addWidget(new TQLabel(i18n("Database Mirror:"), this));
    
    m_mirrorCombo = new TQComboBox(this);
    m_mirrorCombo->setEditable(true);
    m_mirrorCombo->insertItem("database.clamav.net");
    m_mirrorCombo->insertItem("db.us.clamav.net");
    m_mirrorCombo->insertItem("db.local.clamav.net");
    m_mirrorCombo->insertItem("db.de.ipv6.clamav.net");
    m_mirrorCombo->insertItem("db.dk.ipv6.clamav.net");
    m_mirrorCombo->insertItem("db.pl.ipv6.clamav.net");
    m_mirrorCombo->insertItem("db.cz.ipv6.clamav.net");
    m_mirrorCombo->insertItem("db.at.ipv6.clamav.net");
    m_mirrorCombo->insertItem("db.ch.ipv6.clamav.net");
    m_mirrorCombo->insertItem("db.lu.ipv6.clamav.net");
    m_mirrorCombo->insertItem("db.be.ipv6.clamav.net");
    mirrorLayout->addWidget(m_mirrorCombo, 1);
    
    TQHBoxLayout *freqLayout = new TQHBoxLayout(mainLayout, 6);
    freqLayout->addWidget(new TQLabel(i18n("Checks per day:"), this));
    
    m_checkFreqSpin = new TQSpinBox(1, 50, 1, this);
    freqLayout->addWidget(m_checkFreqSpin);
    freqLayout->addStretch(1);
    
    mainLayout->addWidget(new TQLabel(i18n("Note: Changing these settings modifies the freshclam.conf file."), this));
    
    mainLayout->addSpacing(15);
    
    TQLabel *addSigsLabel = new TQLabel(i18n("<b>Additional Signatures:</b>"), this);
    mainLayout->addWidget(addSigsLabel);
    
    m_unofficialSigsCheck = new TQCheckBox(i18n("Enable clamav-unofficial-sigs support"), this);
    mainLayout->addWidget(m_unofficialSigsCheck);
    
    TQHBoxLayout *ssLayout = new TQHBoxLayout(mainLayout, 6);
    ssLayout->addSpacing(20);
    m_sanesecurityCheck = new TQCheckBox(i18n("Enable Sanesecurity signatures"), this);
    m_sanesecurityCheck->setEnabled(false);
    ssLayout->addWidget(m_sanesecurityCheck);

    TQHBoxLayout *urlhausLayout = new TQHBoxLayout(mainLayout, 6);
    urlhausLayout->addSpacing(20);
    m_urlhausCheck = new TQCheckBox(i18n("Enable URLhaus database (malicious URLs)"), this);
    m_urlhausCheck->setEnabled(false);
    urlhausLayout->addWidget(m_urlhausCheck);
    
    connect(m_unofficialSigsCheck, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(onUnofficialSigsToggled(bool)));
    
    mainLayout->addSpacing(10);
    TQLabel *pathLabel = new TQLabel(TQString(i18n("Signatures path: <i>%1</i>")).arg("/var/lib/clamav"), this);
    mainLayout->addWidget(pathLabel);
    
    mainLayout->addStretch(1);
}

DatabasePage::~DatabasePage()
{
}

void DatabasePage::onUnofficialSigsToggled(bool checked)
{
    m_sanesecurityCheck->setEnabled(checked);
    m_urlhausCheck->setEnabled(checked);
}

void DatabasePage::loadSettings(SettingsManager *settings)
{
    if (!settings) return;
    m_mirrorCombo->setCurrentText(settings->value("db_mirror", "database.clamav.net"));
    m_checkFreqSpin->setValue(settings->value("db_checks", "12").toInt());
    
    m_unofficialSigsCheck->setChecked(settings->valueBool("enable_unofficial_sigs", false));
    m_sanesecurityCheck->setChecked(settings->valueBool("enable_sanesecurity_sigs", false));
    m_sanesecurityCheck->setEnabled(m_unofficialSigsCheck->isChecked());
    
    m_urlhausCheck->setChecked(settings->valueBool("enable_urlhaus_sigs", false));
    m_urlhausCheck->setEnabled(m_unofficialSigsCheck->isChecked());
}

void DatabasePage::saveSettings(SettingsManager *settings)
{
    if (!settings) return;
    
    TQString newMirror = m_mirrorCombo->currentText().stripWhiteSpace();
    // Validate mirror name to prevent shell/config injection
    TQRegExp rx("^[a-zA-Z0-9.-]+$");
    if (!rx.exactMatch(newMirror)) {
        KMessageBox::error(this, i18n("Invalid Database Mirror hostname. It must only contain alphanumeric characters, dots, and hyphens."));
        return;
    }

    TQString oldMirror = settings->value("db_mirror", "database.clamav.net");
    TQString oldChecks = settings->value("db_checks", "12");
    bool oldUnofficial = settings->valueBool("enable_unofficial_sigs", false);
    bool oldSanesecurity = settings->valueBool("enable_sanesecurity_sigs", false);
    bool oldUrlhaus = settings->valueBool("enable_urlhaus_sigs", false);
    
    TQString newChecks = TQString::number(m_checkFreqSpin->value());
    bool newUnofficial = m_unofficialSigsCheck->isChecked();
    bool newSanesecurity = m_sanesecurityCheck->isChecked();
    bool newUrlhaus = m_urlhausCheck->isChecked();
    
    settings->setValue("db_mirror", newMirror);
    settings->setValue("db_checks", newChecks);
    settings->setValueBool("enable_unofficial_sigs", newUnofficial);
    settings->setValueBool("enable_sanesecurity_sigs", newSanesecurity);
    settings->setValueBool("enable_urlhaus_sigs", newUrlhaus);
    
    if (oldMirror != newMirror || oldChecks != newChecks || oldUnofficial != newUnofficial || oldSanesecurity != newSanesecurity || oldUrlhaus != newUrlhaus) {
        // Values changed! We need to update freshclam config, cron schedule and handle files
        TQString scriptPath = "/tmp/tdeclamui_freshclam_config.sh";
        TQFile scriptFile(scriptPath);
        if (scriptFile.open(IO_WriteOnly)) {
            TQString resetDatLine = "";
            if (oldMirror != newMirror) {
                resetDatLine = "    rm -f /var/lib/clamav/freshclam.dat\n";
            }
            TQString scriptContent = TQString(
                "#!/bin/sh\n"
                "CONF=\"/etc/clamav/freshclam.conf\"\n"
                "if [ -f \"$CONF\" ]; then\n"
                "    if grep -q \"^Checks \" \"$CONF\"; then\n"
                "        sed -i \"s/^Checks .*/Checks %1/\" \"$CONF\"\n"
                "    else\n"
                "        echo \"Checks %1\" >> \"$CONF\"\n"
                "    fi\n"
                "    sed -i '/^DatabaseMirror /d' \"$CONF\"\n"
                "    echo \"DatabaseMirror %2\" >> \"$CONF\"\n"
                "    if [ \"%2\" != \"database.clamav.net\" ]; then\n"
                "        echo \"DatabaseMirror database.clamav.net\" >> \"$CONF\"\n"
                "    fi\n"
                "%6"
                "    systemctl restart clamav-freshclam.service 2>/dev/null\n"
                "fi\n"
                "\n"
                "CRON_FILE=\"/etc/cron.d/tdeclamui-additional-sigs\"\n"
                "if [ \"%3\" = \"true\" ]; then\n"
                "    CHECKS=\"%1\"\n"
                "    INTERVAL=2\n"
                "    if [ \"$CHECKS\" -ge 24 ]; then INTERVAL=1;\n"
                "    elif [ \"$CHECKS\" -ge 12 ]; then INTERVAL=2;\n"
                "    elif [ \"$CHECKS\" -ge 8 ]; then INTERVAL=3;\n"
                "    elif [ \"$CHECKS\" -ge 6 ]; then INTERVAL=4;\n"
                "    elif [ \"$CHECKS\" -ge 4 ]; then INTERVAL=6;\n"
                "    elif [ \"$CHECKS\" -ge 2 ]; then INTERVAL=12;\n"
                "    else INTERVAL=24; fi\n"
                "    if [ \"$INTERVAL\" -eq 24 ]; then\n"
                "        SCHEDULE=\"0 0 * * *\"\n"
                "    else\n"
                "        SCHEDULE=\"0 */$INTERVAL * * *\"\n"
                "    fi\n"
                "    CMD=\"/usr/bin/tdeclamui --update-additional\"\n"
                "    if [ \"%4\" = \"true\" ]; then CMD=\"$CMD --sanesecurity\"; fi\n"
                "    if [ \"%5\" = \"true\" ]; then CMD=\"$CMD --urlhaus\"; fi\n"
                "    echo \"# TDeClamUI additional signatures update schedule\" > \"$CRON_FILE\"\n"
                "    echo \"$SCHEDULE root $CMD >/dev/null 2>&1\" >> \"$CRON_FILE\"\n"
                "    chmod 0644 \"$CRON_FILE\"\n"
                "else\n"
                "    rm -f \"$CRON_FILE\"\n"
                "fi\n"
                "\n"
                "# File management: deletions and backup copies\n"
                "if [ \"%3\" = \"false\" ]; then\n"
                "    # Unofficial signatures support is completely disabled: delete all unofficial files\n"
                "    rm -f /var/lib/clamav/junk.ndb /var/lib/clamav/jurlbl.ndb /var/lib/clamav/phish.ndb \\\n"
                "          /var/lib/clamav/rogue.hdb /var/lib/clamav/scam.ndb /var/lib/clamav/spamattach.hdb \\\n"
                "          /var/lib/clamav/spamimg.hdb /var/lib/clamav/blurl.ndb /var/lib/clamav/malwarehash.hsb \\\n"
                "          /var/lib/clamav/badmacro.ndb /var/lib/clamav/spam.ldb /var/lib/clamav/sanesecurity.ftm \\\n"
                "          /var/lib/clamav/sigwhitelist.ign2 /var/lib/clamav/urlhaus.ndb\n"
                "else\n"
                "    # Unofficial signatures support is enabled\n"
                "    # If Sanesecurity is disabled, delete Sanesecurity files\n"
                "    if [ \"%4\" = \"false\" ]; then\n"
                "        rm -f /var/lib/clamav/junk.ndb /var/lib/clamav/jurlbl.ndb /var/lib/clamav/phish.ndb \\\n"
                "              /var/lib/clamav/rogue.hdb /var/lib/clamav/scam.ndb /var/lib/clamav/spamattach.hdb \\\n"
                "              /var/lib/clamav/spamimg.hdb /var/lib/clamav/blurl.ndb /var/lib/clamav/malwarehash.hsb \\\n"
                "              /var/lib/clamav/badmacro.ndb /var/lib/clamav/spam.ldb\n"
                "    fi\n"
                "    \n"
                "    # If URLhaus is disabled, delete URLhaus file\n"
                "    if [ \"%5\" = \"false\" ]; then\n"
                "        rm -f /var/lib/clamav/urlhaus.ndb\n"
                "    fi\n"
                "    \n"
                "    # Recover files from backup directory if they exist\n"
                "    SRC_DIR=\"/var/lib/tdeclamui/unofficial-sigs\"\n"
                "    DST_DIR=\"/var/lib/clamav\"\n"
                "    if [ -d \"$SRC_DIR\" ]; then\n"
                "        # Always copy base files if they exist\n"
                "        for f in sanesecurity.ftm sigwhitelist.ign2; do\n"
                "            if [ -f \"$SRC_DIR/$f\" ]; then\n"
                "                cp -p \"$SRC_DIR/$f\" \"$DST_DIR/\"\n"
                "            fi\n"
                "        done\n"
                "        # Also copy Sanesecurity files if enabled\n"
                "        if [ \"%4\" = \"true\" ]; then\n"
                "            for f in junk.ndb jurlbl.ndb phish.ndb rogue.hdb scam.ndb spamattach.hdb spamimg.hdb blurl.ndb malwarehash.hsb badmacro.ndb spam.ldb; do\n"
                "                if [ -f \"$SRC_DIR/$f\" ]; then\n"
                "                    cp -p \"$SRC_DIR/$f\" \"$DST_DIR/\"\n"
                "                fi\n"
                "            done\n"
                "        fi\n"
                "        # Also copy URLhaus files if enabled\n"
                "        if [ \"%5\" = \"true\" ]; then\n"
                "            for f in urlhaus.ndb; do\n"
                "                if [ -f \"$SRC_DIR/$f\" ]; then\n"
                "                    cp -p \"$SRC_DIR/$f\" \"$DST_DIR/\"\n"
                "                fi\n"
                "            done\n"
                "        fi\n"
                "    fi\n"
                "fi\n"
                "rm -f \"$0\"\n"
            ).arg(newChecks, newMirror, TQString(newUnofficial ? "true" : "false"), TQString(newSanesecurity ? "true" : "false"))
             .arg(TQString(newUrlhaus ? "true" : "false"), resetDatLine);
            
            scriptFile.writeBlock(scriptContent.latin1(), scriptContent.length());
            scriptFile.close();
            chmod(scriptPath.local8Bit(), 0700);
            
            // Run it via tdesudo
            TQString tdesudo = TDEGlobal::dirs()->findExe("tdesudo");
            if (!tdesudo.isEmpty()) {
                TQProcess proc;
                proc.addArgument(tdesudo);
                proc.addArgument("-i");
                proc.addArgument("security-high");
                proc.addArgument("-d");
                proc.addArgument("--comment");
                proc.addArgument(i18n("ClamUI needs root access to update configuration"));
                proc.addArgument("--");
                proc.addArgument(scriptPath);
                
                if (proc.start()) {
                    while (proc.isRunning()) {
                        if (tqApp && tqApp->type() != TQApplication::Tty) {
                            tqApp->processEvents();
                        }
                        usleep(10000);
                    }
                    if (proc.normalExit() && proc.exitStatus() == 0) {
                        // Success
                    } else {
                        KMessageBox::error(this, i18n("Failed to apply configuration changes. Privilege elevation might have been cancelled."));
                    }
                } else {
                    KMessageBox::error(this, i18n("Failed to start privilege elevation utility."));
                }
            } else {
                TQProcess proc;
                proc.addArgument(scriptPath);
                if (proc.start()) {
                    while (proc.isRunning()) {
                        if (tqApp && tqApp->type() != TQApplication::Tty) {
                            tqApp->processEvents();
                        }
                        usleep(10000);
                    }
                }
            }
            TQFile::remove(scriptPath);
        }
    }
}

#include "database_page.moc"
