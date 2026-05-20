/*
 * system_audit.cpp - System security audit implementation
 */

#include "system_audit.h"
#include <stdlib.h>
#include <tdelocale.h>
#include <tdeglobal.h>
#include <kstandarddirs.h>
#include <kprocess.h>
#include <krun.h>
#include <ntqfile.h>
#include <ntqtextstream.h>
#include <ntqfileinfo.h>
#include <tqtprocess.h>
#include <tqdatetime.h>
#include <tdeapplication.h>
#include "../libs/serializer/tqtstringserializer.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

static int runCommand(const TQString &cmd, TQString &output) {
    output = "";
    FILE *fp = popen(cmd.latin1(), "r");
    if (!fp) return -1;
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        output += TQString::fromLocal8Bit(buffer);
    }
    int status = pclose(fp);
    output = output.stripWhiteSpace();
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

static bool isServiceActive(const TQString &service) {
    TQString out;
    int rc = runCommand("systemctl is-active " + service, out);
    return rc == 0 && out == "active";
}

static bool isBinaryInstalled(const TQString &binary) {
    TQString out;
    if (runCommand("which " + binary, out) == 0) return true;
    if (TQFile::exists("/usr/sbin/" + binary)) return true;
    if (TQFile::exists("/sbin/" + binary)) return true;
    if (TQFile::exists("/usr/bin/" + binary)) return true;
    if (TQFile::exists("/bin/" + binary)) return true;
    return false;
}

SystemAudit::SystemAudit(TQObject *parent)
    : TQObject(parent)
{
}

SystemAudit::~SystemAudit()
{
}

bool SystemAudit::isBinaryAvailable(const TQString &binary) const
{
    return isBinaryInstalled(binary);
}

AuditSectionResult SystemAudit::checkClamAVHealth() const
{
    AuditSectionResult section;
    section.category = "clamav_health";
    section.title = i18n("ClamAV Health");
    section.icon_name = "security-high";
    
    // ClamAV Installation
    AuditCheckResult installCheck;
    installCheck.name = i18n("ClamAV Installation");
    TQString versionOut;
    if (runCommand("clamscan --version", versionOut) == 0) {
        installCheck.status = AuditPass;
        installCheck.detail = i18n("Installed: %1").arg(versionOut);
    } else {
        installCheck.status = AuditFail;
        installCheck.detail = i18n("ClamAV is not installed");
        installCheck.recommendation = i18n("Install ClamAV to enable virus scanning");
        installCheck.install_command = "sudo apt install clamav clamav-daemon";
        section.checks.append(installCheck);
        return section;
    }
    section.checks.append(installCheck);
    
    // Virus Database
    AuditCheckResult dbCheck;
    dbCheck.name = i18n("Virus Database");
    TQFileInfo dbInfo("/var/lib/clamav/daily.cvd");
    if (!dbInfo.exists()) dbInfo = TQFileInfo("/var/lib/clamav/daily.cld");
    
    if (dbInfo.exists()) {
        int daysOld = dbInfo.lastModified().daysTo(TQDateTime::currentDateTime());
        if (daysOld <= 3) {
            dbCheck.status = AuditPass;
            dbCheck.detail = i18n("Up to date (%1 days old)").arg(daysOld);
        } else if (daysOld <= 7) {
            dbCheck.status = AuditWarning;
            dbCheck.detail = i18n("Database is %1 days old").arg(daysOld);
            dbCheck.recommendation = i18n("Update virus database to ensure protection");
            dbCheck.install_command = "sudo freshclam";
        } else {
            dbCheck.status = AuditFail;
            dbCheck.detail = i18n("Database is %1 days old").arg(daysOld);
            dbCheck.recommendation = i18n("Database is critically outdated. Update immediately.");
            dbCheck.install_command = "sudo freshclam";
        }
    } else {
        dbCheck.status = AuditFail;
        dbCheck.detail = i18n("No virus database found");
        dbCheck.recommendation = i18n("Download virus database");
        dbCheck.install_command = "sudo freshclam";
    }
    section.checks.append(dbCheck);
    
    // ClamAV Daemon
    AuditCheckResult daemonCheck;
    daemonCheck.name = i18n("ClamAV Daemon");
    if (isServiceActive("clamav-daemon") || isServiceActive("clamd@scan") || isServiceActive("clamd")) {
        daemonCheck.status = AuditPass;
        daemonCheck.detail = i18n("Daemon is running");
    } else {
        daemonCheck.status = AuditWarning;
        daemonCheck.detail = i18n("ClamAV daemon is not running");
        daemonCheck.recommendation = i18n("Start the daemon for faster scanning");
        daemonCheck.install_command = "sudo systemctl start clamav-daemon";
    }
    section.checks.append(daemonCheck);
    
    // Automatic Updates
    AuditCheckResult autoDbCheck;
    autoDbCheck.name = i18n("Automatic Updates");
    if (isServiceActive("clamav-freshclam") || isServiceActive("clamav-freshclam.timer")) {
        autoDbCheck.status = AuditPass;
        autoDbCheck.detail = i18n("Freshclam service is active");
    } else {
        autoDbCheck.status = AuditWarning;
        autoDbCheck.detail = i18n("Freshclam automatic update service is not running");
        autoDbCheck.recommendation = i18n("Enable automatic database updates");
        autoDbCheck.install_command = "sudo systemctl enable --now clamav-freshclam";
    }
    section.checks.append(autoDbCheck);
    
    return section;
}

AuditSectionResult SystemAudit::checkFirewall() const
{
    AuditSectionResult section;
    section.category = "firewall";
    section.title = i18n("Firewall");
    section.icon_name = "security-medium";
    
    bool firewallFound = false;
    
    // Firewall Detection (check active status first)
    AuditCheckResult fwCheck;
    fwCheck.name = i18n("Firewall");
    
    TQString ufwOut;
    bool ufwActive = (isServiceActive("ufw") || isBinaryInstalled("ufw")) && (runCommand("ufw status", ufwOut) == 0 && ufwOut.contains("Status: active"));
    bool snitchActive = isServiceActive("opensnitch");
    bool firewalldActive = isServiceActive("firewalld");
    bool nftablesActive = isServiceActive("nftables");
    
    if (ufwActive) {
        fwCheck.status = AuditPass;
        fwCheck.detail = i18n("UFW is active and enabled");
        firewallFound = true;
    } else if (snitchActive) {
        fwCheck.status = AuditPass;
        fwCheck.detail = i18n("OpenSnitch application firewall is active");
        firewallFound = true;
    } else if (firewalldActive) {
        fwCheck.status = AuditPass;
        fwCheck.detail = i18n("Firewalld is running");
        firewallFound = true;
    } else if (nftablesActive) {
        fwCheck.status = AuditPass;
        fwCheck.detail = i18n("nftables service is active");
        firewallFound = true;
    } else if (isBinaryInstalled("ufw")) {
        fwCheck.status = AuditWarning;
        fwCheck.detail = i18n("UFW is installed but firewall may be disabled");
        fwCheck.recommendation = i18n("Enable the firewall");
        fwCheck.install_command = "sudo ufw enable";
        firewallFound = true;
    } else if (isBinaryInstalled("opensnitchd")) {
        fwCheck.status = AuditWarning;
        fwCheck.detail = i18n("OpenSnitch is installed but not running");
        fwCheck.recommendation = i18n("Start the OpenSnitch service");
        fwCheck.install_command = "sudo systemctl enable --now opensnitch";
        firewallFound = true;
    }
    
    if (!firewallFound) {
        fwCheck.status = AuditFail;
        fwCheck.detail = i18n("No active firewall detected");
        fwCheck.recommendation = i18n("Install and enable a firewall for network protection");
        fwCheck.install_command = "sudo apt install ufw opensnitch && sudo ufw enable && sudo systemctl enable --now opensnitch";
    }
    section.checks.append(fwCheck);
    
    // Open Ports
    AuditCheckResult portsCheck;
    portsCheck.name = i18n("Open Ports");
    TQString ssOut;
    if (runCommand("ss -tulnH", ssOut) == 0) {
        int count = ssOut.contains("\n") + 1;
        if (count > 0) {
            portsCheck.status = AuditWarning;
            portsCheck.detail = i18n("%1 ports listening").arg(count);
            portsCheck.recommendation = i18n("Review open ports and close unnecessary services");
        } else {
            portsCheck.status = AuditPass;
            portsCheck.detail = i18n("No open ports detected");
        }
    }
    section.checks.append(portsCheck);
    
    // Firewall GUI
    AuditCheckResult guiCheck;
    guiCheck.name = i18n("Firewall Manager");
    if (isBinaryInstalled("opensnitch-tde")) {
        guiCheck.status = AuditPass;
        guiCheck.detail = i18n("OpenSnitch TDE GUI (opensnitch-tde) is installed");
    } else if (isBinaryInstalled("opensnitch-ui")) {
        guiCheck.status = AuditPass;
        guiCheck.detail = i18n("OpenSnitch UI (opensnitch-ui) is installed");
    } else if (isBinaryInstalled("gufw")) {
        guiCheck.status = AuditPass;
        guiCheck.detail = i18n("Gufw is available");
        guiCheck.launch_command = "gufw";
        guiCheck.launch_label = i18n("Open Gufw");
    } else {
        guiCheck.status = AuditUnknown;
        guiCheck.detail = i18n("No graphical firewall manager detected");
        guiCheck.recommendation = i18n("Install a GUI for easier firewall management");
        guiCheck.install_command = "sudo apt install gufw";
    }
    section.checks.append(guiCheck);
    
    return section;
}

AuditSectionResult SystemAudit::checkMacFramework() const
{
    AuditSectionResult section;
    section.category = "mac_framework";
    section.title = i18n("Access Control");
    section.icon_name = "system-lock-screen";
    
    bool macFound = false;
    AuditCheckResult macCheck;
    macCheck.name = i18n("MAC Framework");
    
    if (TQFile::exists("/sys/module/apparmor/parameters/enabled")) {
        macCheck.name = i18n("AppArmor");
        TQString enabled;
        runCommand("cat /sys/module/apparmor/parameters/enabled", enabled);
        // Module loaded — count active profiles to determine if truly enforcing
        TQString profileCount;
        runCommand("cat /sys/kernel/security/apparmor/profiles 2>/dev/null | wc -l", profileCount);
        bool hasProfiles = profileCount.toInt() > 0;
        if (enabled == "Y" && hasProfiles) {
            macCheck.status = AuditPass;
            macCheck.detail = i18n("AppArmor is enabled and enforcing %1 profiles").arg(profileCount);
        } else if (enabled == "Y") {
            macCheck.status = AuditWarning;
            macCheck.detail = i18n("AppArmor module is loaded but no profiles are active");
            macCheck.recommendation = i18n("Load AppArmor profiles for application sandboxing");
            macCheck.install_command = "sudo aa-enforce /etc/apparmor.d/*";
        } else {
            macCheck.status = AuditWarning;
            macCheck.detail = i18n("AppArmor module is loaded but not enabled");
            macCheck.recommendation = i18n("Enable AppArmor for application sandboxing");
            macCheck.install_command = "sudo systemctl enable --now apparmor";
        }
        macFound = true;
    } else if (isBinaryInstalled("getenforce")) {
        macCheck.name = i18n("SELinux");
        TQString mode;
        runCommand("getenforce", mode);
        mode = mode.lower();
        if (mode == "enforcing") {
            macCheck.status = AuditPass;
            macCheck.detail = i18n("SELinux is enforcing");
        } else if (mode == "permissive") {
            macCheck.status = AuditWarning;
            macCheck.detail = i18n("SELinux is in permissive mode");
            macCheck.recommendation = i18n("Consider switching to enforcing mode for stronger protection");
        } else {
            macCheck.status = AuditFail;
            macCheck.detail = i18n("SELinux is disabled");
            macCheck.recommendation = i18n("Enable SELinux for mandatory access control");
        }
        macFound = true;
    }
    
    if (!macFound) {
        macCheck.status = AuditUnknown;
        macCheck.detail = i18n("No mandatory access control framework detected");
        macCheck.recommendation = i18n("Install and enable AppArmor");
        macCheck.install_command = "sudo apt install apparmor apparmor-profiles apparmor-utils && sudo systemctl enable --now apparmor";
    }
    section.checks.append(macCheck);
    return section;
}

AuditSectionResult SystemAudit::checkAutoUpdates() const
{
    AuditSectionResult section;
    section.category = "auto_updates";
    section.title = i18n("Automatic Updates");
    section.icon_name = "software-update-available";
    
    AuditCheckResult updateCheck;
    updateCheck.name = i18n("Unattended Upgrades");
    TQString aptOut;
    runCommand("apt-config dump APT::Periodic::Unattended-Upgrade", aptOut);
    if (aptOut.contains("\"1\"") || aptOut.lower().contains("\"true\"") || isServiceActive("unattended-upgrades")) {
        updateCheck.status = AuditPass;
        updateCheck.detail = i18n("Automatic security updates are enabled");
    } else if (aptOut.contains("\"0\"")) {
        updateCheck.status = AuditWarning;
        updateCheck.detail = i18n("Unattended upgrades is installed but disabled");
        updateCheck.recommendation = i18n("Enable automatic security updates");
        updateCheck.install_command = "sudo dpkg-reconfigure -plow unattended-upgrades";
    } else if (isServiceActive("dnf-automatic.timer")) {
        updateCheck.name = i18n("DNF Automatic");
        updateCheck.status = AuditPass;
        updateCheck.detail = i18n("DNF automatic updates are enabled");
    } else {
        updateCheck.status = AuditUnknown;
        updateCheck.detail = i18n("Could not determine automatic update status");
        updateCheck.recommendation = i18n("Configure automatic security updates");
        if (isBinaryInstalled("apt")) updateCheck.install_command = "sudo apt install unattended-upgrades";
    }
    section.checks.append(updateCheck);
    
    if (TQFile::exists("/var/run/reboot-required")) {
        AuditCheckResult rebootCheck;
        rebootCheck.name = i18n("Pending Reboot");
        rebootCheck.status = AuditWarning;
        rebootCheck.detail = i18n("System reboot required to apply updates");
        rebootCheck.recommendation = i18n("Reboot the system to apply pending security updates");
        section.checks.append(rebootCheck);
    }
    
    return section;
}

AuditSectionResult SystemAudit::checkIntrusionDetection() const
{
    AuditSectionResult section;
    section.category = "intrusion_detection";
    section.title = i18n("Intrusion Detection");
    section.icon_name = "dialog-warning";
    
    AuditCheckResult idsCheck;
    idsCheck.name = i18n("Intrusion Prevention");
    
    if (isServiceActive("fail2ban")) {
        idsCheck.name = i18n("fail2ban");
        idsCheck.status = AuditPass;
        idsCheck.detail = i18n("fail2ban is active and protecting services");
    } else if (isBinaryInstalled("fail2ban-client")) {
        idsCheck.name = i18n("fail2ban");
        idsCheck.status = AuditWarning;
        idsCheck.detail = i18n("fail2ban is installed but not running");
        idsCheck.recommendation = i18n("Start fail2ban to protect against brute force attacks");
        idsCheck.install_command = "sudo systemctl start fail2ban";
    } else if (isServiceActive("crowdsec")) {
        idsCheck.name = i18n("CrowdSec");
        idsCheck.status = AuditPass;
        idsCheck.detail = i18n("CrowdSec is active");
    } else {
        idsCheck.status = AuditWarning;
        idsCheck.detail = i18n("No intrusion detection system found");
        idsCheck.recommendation = i18n("Install fail2ban or CrowdSec to protect against attacks");
        if (isBinaryInstalled("apt")) idsCheck.install_command = "sudo apt install fail2ban";
    }
    section.checks.append(idsCheck);
    return section;
}

AuditSectionResult SystemAudit::checkSSHHardening() const
{
    AuditSectionResult section;
    section.category = "ssh_hardening";
    section.title = i18n("SSH Security");
    section.icon_name = "network-server";
    
    AuditCheckResult sshCheck;
    sshCheck.name = i18n("SSH Server");
    if (!isServiceActive("sshd") && !isServiceActive("ssh")) {
        sshCheck.status = AuditPass;
        sshCheck.detail = i18n("SSH server is not running (no remote attack surface)");
    } else {
        sshCheck.status = AuditPass;
        sshCheck.detail = i18n("SSH server is running");
    }
    section.checks.append(sshCheck);
    
    if (TQFile::exists("/etc/ssh/sshd_config")) {
        TQString configOut;
        runCommand("cat /etc/ssh/sshd_config", configOut);
        
        AuditCheckResult rootCheck;
        rootCheck.name = i18n("Root Login");
        if (configOut.contains("PermitRootLogin no") || configOut.contains("PermitRootLogin prohibit-password")) {
            rootCheck.status = AuditPass;
            rootCheck.detail = i18n("Root login: prohibit-password");
        } else {
            rootCheck.status = AuditFail;
            rootCheck.detail = i18n("Root login is allowed");
            rootCheck.recommendation = i18n("Disable root SSH login for security");
            rootCheck.install_command = "sudo sed -i 's/^PermitRootLogin.*/PermitRootLogin prohibit-password/' /etc/ssh/sshd_config && sudo systemctl restart ssh";
        }
        section.checks.append(rootCheck);
        
        AuditCheckResult passCheck;
        passCheck.name = i18n("Password Auth");
        if (configOut.contains("PasswordAuthentication no")) {
            passCheck.status = AuditPass;
            passCheck.detail = i18n("Password authentication is disabled (key-only)");
        } else {
            passCheck.status = AuditWarning;
            passCheck.detail = i18n("Password authentication is enabled");
            passCheck.recommendation = i18n("Consider disabling password auth in favor of SSH keys");
            passCheck.install_command = "sudo sed -i 's/^#\\?PasswordAuthentication.*/PasswordAuthentication no/' /etc/ssh/sshd_config && sudo systemctl restart ssh";
        }
        section.checks.append(passCheck);
        
        AuditCheckResult x11Check;
        x11Check.name = i18n("X11 Forwarding");
        // OpenSSH: explicit "X11Forwarding yes" means enabled (a risk). "no" or absent = safe.
        if (configOut.contains("X11Forwarding yes")) {
            x11Check.status = AuditWarning;
            x11Check.detail = i18n("X11 forwarding is enabled");
            x11Check.recommendation = i18n("Disable X11 forwarding to reduce attack surface");
            x11Check.install_command = "sudo sed -i 's/^X11Forwarding.*/X11Forwarding no/' /etc/ssh/sshd_config && sudo systemctl restart ssh";
        } else {
            x11Check.status = AuditPass;
            x11Check.detail = i18n("X11 forwarding is disabled");
        }
        section.checks.append(x11Check);
    }
    
    return section;
}

AuditSectionResult SystemAudit::checkHomePermissions() const
{
    AuditSectionResult section;
    section.category = "home_permissions";
    section.title = i18n("Home & Sensitive Files");
    section.icon_name = "folder-locked";
    
    TQString homeDir = TQString::fromLocal8Bit(getenv("HOME"));
    if (homeDir.isEmpty()) {
        AuditCheckResult chk;
        chk.name = i18n("Home Directory");
        chk.status = AuditUnknown;
        chk.detail = i18n("Could not determine home directory");
        section.checks.append(chk);
        return section;
    }
    
    // Check $HOME permissions
    {
        AuditCheckResult chk;
        chk.name = i18n("Home Directory Permissions");
        TQFileInfo homeInfo(homeDir);
        if (homeInfo.exists()) {
            TQString permOut;
            runCommand("stat -c '%a' " + homeDir, permOut);
            int perm = permOut.toInt();
            if (perm == 700) {
                chk.status = AuditPass;
                chk.detail = i18n("Home directory has secure permissions (%1)").arg(permOut);
            } else if (perm == 750) {
                chk.status = AuditPass;
                chk.detail = i18n("Home directory permissions are acceptable (%1)").arg(permOut);
            } else if (perm == 755) {
                chk.status = AuditWarning;
                chk.detail = i18n("Home directory is world-readable (%1)").arg(permOut);
                chk.recommendation = i18n("Restrict home directory permissions");
                chk.install_command = "chmod 750 " + homeDir;
            } else {
                chk.status = AuditFail;
                chk.detail = i18n("Home directory has overly permissive access (%1)").arg(permOut);
                chk.recommendation = i18n("Set home directory to 700 or 750");
                chk.install_command = "chmod 700 " + homeDir;
            }
        } else {
            chk.status = AuditUnknown;
            chk.detail = i18n("Cannot access home directory");
        }
        section.checks.append(chk);
    }
    
    // Check ~/.ssh/ directory
    {
        TQString sshDir = homeDir + "/.ssh";
        AuditCheckResult chk;
        chk.name = i18n("SSH Directory (~/.ssh/)");
        if (TQFile::exists(sshDir)) {
            TQString permOut;
            runCommand("stat -c '%a' " + sshDir, permOut);
            int perm = permOut.toInt();
            if (perm == 700) {
                chk.status = AuditPass;
                chk.detail = i18n("SSH directory has correct permissions (700)");
            } else {
                chk.status = AuditFail;
                chk.detail = i18n("SSH directory has incorrect permissions (%1)").arg(permOut);
                chk.recommendation = i18n("SSH directory must be 700");
                chk.install_command = "chmod 700 " + sshDir;
            }
            
            // Check private key permissions
            TQString keyOut;
            runCommand("find " + sshDir + " -name 'id_*' ! -name '*.pub' -printf '%f %m\\n' 2>/dev/null", keyOut);
            if (!keyOut.isEmpty()) {
                TQStringList keyLines = TQStringList::split('\n', keyOut);
                bool allSecure = true;
                TQStringList badKeys;
                for (TQStringList::ConstIterator it = keyLines.begin(); it != keyLines.end(); ++it) {
                    TQStringList parts = TQStringList::split(' ', *it);
                    if (parts.count() >= 2) {
                        int keyPerm = parts[1].toInt();
                        if (keyPerm != 600 && keyPerm != 400) {
                            allSecure = false;
                            badKeys.append(parts[0] + " (" + parts[1] + ")");
                        }
                    }
                }
                
                AuditCheckResult keyChk;
                keyChk.name = i18n("SSH Private Keys");
                if (allSecure) {
                    keyChk.status = AuditPass;
                    keyChk.detail = i18n("All private keys have secure permissions (600/400)");
                } else {
                    keyChk.status = AuditFail;
                    keyChk.detail = i18n("Insecure private key permissions: %1").arg(badKeys.join(", "));
                    keyChk.recommendation = i18n("Private keys must be readable only by owner");
                    keyChk.install_command = "chmod 600 " + sshDir + "/id_*";
                }
                section.checks.append(keyChk);
            }
        } else {
            chk.status = AuditPass;
            chk.detail = i18n("No SSH directory found (not applicable)");
        }
        section.checks.append(chk);
    }
    
    // Check ~/.gnupg/ directory
    {
        TQString gpgDir = homeDir + "/.gnupg";
        AuditCheckResult chk;
        chk.name = i18n("GnuPG Directory (~/.gnupg/)");
        if (TQFile::exists(gpgDir)) {
            TQString permOut;
            runCommand("stat -c '%a' " + gpgDir, permOut);
            int perm = permOut.toInt();
            if (perm == 700) {
                chk.status = AuditPass;
                chk.detail = i18n("GnuPG directory has correct permissions (700)");
            } else {
                chk.status = AuditWarning;
                chk.detail = i18n("GnuPG directory has permissions %1").arg(permOut);
                chk.recommendation = i18n("GnuPG directory should be 700");
                chk.install_command = "chmod 700 " + gpgDir;
            }
        } else {
            chk.status = AuditPass;
            chk.detail = i18n("No GnuPG directory found (not applicable)");
        }
        section.checks.append(chk);
    }
    
    // Check for plaintext credential files
    {
        AuditCheckResult chk;
        chk.name = i18n("Plaintext Credential Files");
        TQStringList credFiles;
        TQStringList foundBad;
        credFiles << "/.netrc" << "/.pgpass" << "/.my.cnf" << "/.docker/config.json";
        
        for (TQStringList::ConstIterator it = credFiles.begin(); it != credFiles.end(); ++it) {
            TQString filePath = homeDir + *it;
            if (TQFile::exists(filePath)) {
                TQString permOut;
                runCommand("stat -c '%a' " + filePath, permOut);
                int perm = permOut.toInt();
                if (perm != 600 && perm != 400) {
                    foundBad.append("~" + *it + " (" + permOut + ")");
                }
            }
        }
        
        if (foundBad.isEmpty()) {
            chk.status = AuditPass;
            chk.detail = i18n("No exposed credential files found");
        } else {
            chk.status = AuditFail;
            chk.detail = i18n("Credential files with insecure permissions: %1").arg(foundBad.join(", "));
            chk.recommendation = i18n("Restrict credential files to owner-only access (chmod 600)");
        }
        section.checks.append(chk);
    }
    
    return section;
}

AuditSectionResult SystemAudit::checkKernelHardening() const
{
    AuditSectionResult section;
    section.category = "kernel_hardening";
    section.title = i18n("Kernel Hardening");
    section.icon_name = "preferences-system";
    
    // ASLR (Address Space Layout Randomization)
    {
        AuditCheckResult chk;
        chk.name = i18n("ASLR");
        TQString val;
        if (TQFile::exists("/proc/sys/kernel/randomize_va_space")) {
            runCommand("cat /proc/sys/kernel/randomize_va_space", val);
            int level = val.toInt();
            if (level == 2) {
                chk.status = AuditPass;
                chk.detail = i18n("Full ASLR is enabled (level 2)");
            } else if (level == 1) {
                chk.status = AuditWarning;
                chk.detail = i18n("Partial ASLR is enabled (level 1)");
                chk.recommendation = i18n("Enable full ASLR for better memory protection");
                chk.install_command = "sudo sysctl -w kernel.randomize_va_space=2";
            } else {
                chk.status = AuditFail;
                chk.detail = i18n("ASLR is disabled");
                chk.recommendation = i18n("Enable ASLR to prevent memory exploitation attacks");
                chk.install_command = "sudo sysctl -w kernel.randomize_va_space=2";
            }
        } else {
            chk.status = AuditUnknown;
            chk.detail = i18n("Cannot read ASLR status");
        }
        section.checks.append(chk);
    }
    
    // Yama ptrace_scope
    {
        AuditCheckResult chk;
        chk.name = i18n("Ptrace Protection");
        TQString val;
        if (TQFile::exists("/proc/sys/kernel/yama/ptrace_scope")) {
            runCommand("cat /proc/sys/kernel/yama/ptrace_scope", val);
            int scope = val.toInt();
            if (scope >= 2) {
                chk.status = AuditPass;
                chk.detail = i18n("Ptrace is restricted to admin only (scope %1)").arg(scope);
            } else if (scope == 1) {
                chk.status = AuditPass;
                chk.detail = i18n("Ptrace is restricted to parent processes (scope 1)");
            } else {
                chk.status = AuditWarning;
                chk.detail = i18n("Ptrace is unrestricted (scope 0) — any process can debug others");
                chk.recommendation = i18n("Set ptrace_scope to at least 1 to prevent process snooping");
                chk.install_command = "sudo sysctl -w kernel.yama.ptrace_scope=1";
            }
        } else {
            chk.status = AuditUnknown;
            chk.detail = i18n("Yama LSM is not available on this kernel");
        }
        section.checks.append(chk);
    }
    
    // Core Dumps
    {
        AuditCheckResult chk;
        chk.name = i18n("Core Dump Handling");
        TQString val;
        if (TQFile::exists("/proc/sys/kernel/core_pattern")) {
            runCommand("cat /proc/sys/kernel/core_pattern", val);
            if (val.contains("systemd-coredump") || val.contains("apport")) {
                chk.status = AuditPass;
                chk.detail = i18n("Core dumps are handled securely by %1")
                    .arg(val.contains("systemd-coredump") ? "systemd-coredump" : "apport");
            } else if (val == "" || val == "|/bin/false") {
                chk.status = AuditPass;
                chk.detail = i18n("Core dumps are disabled");
            } else {
                chk.status = AuditWarning;
                chk.detail = i18n("Core dumps may be written to disk (%1)").arg(val.left(60));
                chk.recommendation = i18n("Core dumps can contain sensitive data (passwords, keys). Consider disabling or using systemd-coredump");
                chk.install_command = "echo 'kernel.core_pattern=|/bin/false' | sudo tee /etc/sysctl.d/50-coredump.conf && sudo sysctl -p /etc/sysctl.d/50-coredump.conf";
            }
        } else {
            chk.status = AuditUnknown;
            chk.detail = i18n("Cannot read core dump configuration");
        }
        section.checks.append(chk);
    }
    
    // Unprivileged BPF
    {
        AuditCheckResult chk;
        chk.name = i18n("Unprivileged BPF");
        TQString val;
        if (TQFile::exists("/proc/sys/kernel/unprivileged_bpf_disabled")) {
            runCommand("cat /proc/sys/kernel/unprivileged_bpf_disabled", val);
            int disabled = val.toInt();
            if (disabled >= 1) {
                chk.status = AuditPass;
                chk.detail = i18n("Unprivileged BPF is disabled");
            } else {
                chk.status = AuditWarning;
                chk.detail = i18n("Unprivileged users can load BPF programs");
                chk.recommendation = i18n("Disable unprivileged BPF to reduce kernel attack surface");
                chk.install_command = "sudo sysctl -w kernel.unprivileged_bpf_disabled=1";
            }
        } else {
            chk.status = AuditPass;
            chk.detail = i18n("BPF restriction not applicable (older kernel)");
        }
        section.checks.append(chk);
    }
    
    // Kernel lockdown mode
    {
        AuditCheckResult chk;
        chk.name = i18n("Kernel Lockdown");
        if (TQFile::exists("/sys/kernel/security/lockdown")) {
            TQString val;
            runCommand("cat /sys/kernel/security/lockdown", val);
            if (val.contains("[integrity]") || val.contains("[confidentiality]")) {
                chk.status = AuditPass;
                chk.detail = i18n("Kernel lockdown is active: %1").arg(val.stripWhiteSpace());
            } else if (val.contains("[none]")) {
                chk.status = AuditWarning;
                chk.detail = i18n("Kernel lockdown is available but not enforced");
                chk.recommendation = i18n("Consider enabling kernel lockdown via boot parameter");
            } else {
                chk.status = AuditUnknown;
                chk.detail = i18n("Lockdown status: %1").arg(val.stripWhiteSpace());
            }
        } else {
            chk.status = AuditUnknown;
            chk.detail = i18n("Kernel lockdown is not supported on this system");
        }
        section.checks.append(chk);
    }
    
    return section;
}

AuditSectionResult SystemAudit::checkPasswordPolicy() const
{
    AuditSectionResult section;
    section.category = "password_policy";
    section.title = i18n("Password & Authentication");
    section.icon_name = "dialog-password";
    
    // Password quality modules
    {
        AuditCheckResult chk;
        chk.name = i18n("Password Quality Policy");
        bool hasPwquality = isBinaryInstalled("pam_pwquality.so") || 
                            TQFile::exists("/lib/x86_64-linux-gnu/security/pam_pwquality.so") ||
                            TQFile::exists("/usr/lib/x86_64-linux-gnu/security/pam_pwquality.so") ||
                            TQFile::exists("/lib/security/pam_pwquality.so");
        bool hasCracklib = isBinaryInstalled("pam_cracklib.so") ||
                           TQFile::exists("/lib/x86_64-linux-gnu/security/pam_cracklib.so") ||
                           TQFile::exists("/usr/lib/x86_64-linux-gnu/security/pam_cracklib.so") ||
                           TQFile::exists("/lib/security/pam_cracklib.so");
        
        if (hasPwquality) {
            // Check if actually configured in PAM
            TQString pamOut;
            runCommand("grep -r 'pam_pwquality' /etc/pam.d/ 2>/dev/null | head -1", pamOut);
            if (!pamOut.isEmpty()) {
                chk.status = AuditPass;
                chk.detail = i18n("Password quality enforcement is active (pam_pwquality)");
            } else {
                chk.status = AuditWarning;
                chk.detail = i18n("pam_pwquality is installed but not configured in PAM");
                chk.recommendation = i18n("Configure password quality requirements in PAM");
            }
        } else if (hasCracklib) {
            chk.status = AuditPass;
            chk.detail = i18n("Password quality enforcement is active (pam_cracklib)");
        } else {
            chk.status = AuditWarning;
            chk.detail = i18n("No password quality policy enforced");
            chk.recommendation = i18n("Install libpam-pwquality to enforce strong passwords");
            chk.install_command = "sudo apt install libpam-pwquality";
        }
        section.checks.append(chk);
    }
    
    // Account lockout policy
    {
        AuditCheckResult chk;
        chk.name = i18n("Account Lockout Policy");
        TQString pamOut;
        runCommand("grep -r 'pam_faillock\\|pam_tally2' /etc/pam.d/ 2>/dev/null | head -1", pamOut);
        if (!pamOut.isEmpty()) {
            chk.status = AuditPass;
            if (pamOut.contains("pam_faillock")) {
                chk.detail = i18n("Account lockout is configured (pam_faillock)");
            } else {
                chk.detail = i18n("Account lockout is configured (pam_tally2)");
            }
        } else {
            chk.status = AuditWarning;
            chk.detail = i18n("No account lockout policy detected");
            chk.recommendation = i18n("Configure account lockout after failed login attempts to mitigate brute-force attacks");
        }
        section.checks.append(chk);
    }
    
    // Shell session timeout (TMOUT)
    {
        AuditCheckResult chk;
        chk.name = i18n("Shell Session Timeout");
        TQString tmoutVal;
        bool tmoutFound = false;
        
        // Check system-wide first
        TQString profileOut;
        runCommand("grep -h 'TMOUT' /etc/profile /etc/profile.d/*.sh 2>/dev/null | grep -v '^#' | head -1", profileOut);
        if (!profileOut.isEmpty()) {
            tmoutFound = true;
            tmoutVal = profileOut;
        }
        
        // Check user-level
        if (!tmoutFound) {
            TQString homeDir = TQString::fromLocal8Bit(getenv("HOME"));
            TQStringList rcFiles;
            rcFiles << "/.bashrc" << "/.profile" << "/.zshrc";
            for (TQStringList::ConstIterator it = rcFiles.begin(); it != rcFiles.end(); ++it) {
                TQString rcPath = homeDir + *it;
                if (TQFile::exists(rcPath)) {
                    TQString rcOut;
                    runCommand("grep 'TMOUT' " + rcPath + " 2>/dev/null | grep -v '^#' | head -1", rcOut);
                    if (!rcOut.isEmpty()) {
                        tmoutFound = true;
                        tmoutVal = rcOut;
                        break;
                    }
                }
            }
        }
        
        if (tmoutFound) {
            chk.status = AuditPass;
            chk.detail = i18n("Idle shell timeout is configured (%1)").arg(tmoutVal.stripWhiteSpace());
        } else {
            chk.status = AuditUnknown;
            chk.detail = i18n("No TMOUT set — idle terminal sessions will not auto-disconnect");
            chk.recommendation = i18n("Consider setting TMOUT in /etc/profile to auto-close idle sessions");
        }
        section.checks.append(chk);
    }
    
    // Password hash algorithm
    {
        AuditCheckResult chk;
        chk.name = i18n("Password Hash Algorithm");
        TQString pamOut;
        runCommand("grep -r 'pam_unix' /etc/pam.d/common-password 2>/dev/null | grep -v '^#'", pamOut);
        if (!pamOut.isEmpty()) {
            if (pamOut.contains("yescrypt") || pamOut.contains("sha512")) {
                chk.status = AuditPass;
                TQString algo = pamOut.contains("yescrypt") ? "yescrypt" : "SHA-512";
                chk.detail = i18n("Passwords are hashed with %1 (strong)").arg(algo);
            } else if (pamOut.contains("sha256")) {
                chk.status = AuditPass;
                chk.detail = i18n("Passwords are hashed with SHA-256");
            } else if (pamOut.contains("md5") || pamOut.contains("des")) {
                chk.status = AuditFail;
                chk.detail = i18n("Passwords use a weak hash algorithm");
                chk.recommendation = i18n("Switch to SHA-512 or yescrypt for password hashing");
            } else {
                chk.status = AuditPass;
                chk.detail = i18n("Password hashing is configured via pam_unix");
            }
        } else {
            chk.status = AuditUnknown;
            chk.detail = i18n("Could not determine password hash algorithm");
        }
        section.checks.append(chk);
    }
    
    return section;
}

AuditSectionResult SystemAudit::checkTDEHealth() const
{
    AuditSectionResult section;
    section.category = "tde_health";
    section.title = i18n("TDE Desktop Health");
    section.icon_name = "preferences-desktop";
    
    TQString homeDir = TQString::fromLocal8Bit(getenv("HOME"));
    TQString tdeHome = TQString::fromLocal8Bit(getenv("TDEHOME"));
    if (tdeHome.isEmpty()) {
        tdeHome = homeDir + "/.trinity";
    }
    TQString userName = TQString::fromLocal8Bit(getenv("USER"));
    
    // 1. DelayedCheck in kdedrc
    {
        AuditCheckResult chk;
        chk.name = i18n("System Configuration Check at Startup");
        TQString kdedrcPath = tdeHome + "/share/config/kdedrc";
        if (TQFile::exists(kdedrcPath)) {
            TQString contents;
            TQFile f(kdedrcPath);
            if (f.open(IO_ReadOnly)) {
                TQTextStream t(&f);
                contents = t.read();
                f.close();
            }
            
            // Look for DelayedCheck=true (disabled check)
            bool delayedCheckDisabled = false;
            TQStringList lines = TQStringList::split('\n', contents);
            for (TQStringList::ConstIterator it = lines.begin(); it != lines.end(); ++it) {
                TQString line = (*it).stripWhiteSpace();
                if (line.startsWith("DelayedCheck") && line.contains("=")) {
                    TQString val = line.mid(line.find('=') + 1).stripWhiteSpace().lower();
                    if (val == "true") {
                        delayedCheckDisabled = true;
                    }
                    break;
                }
            }
            
            if (delayedCheckDisabled) {
                chk.status = AuditWarning;
                chk.detail = i18n("System configuration check at startup is disabled (DelayedCheck=true)");
                chk.recommendation = i18n("It is recommended to enable this check to ensure TDE runs correctly after system changes");
            } else {
                chk.status = AuditPass;
                chk.detail = i18n("System configuration check at startup is enabled");
            }
        } else {
            chk.status = AuditPass;
            chk.detail = i18n("Default configuration (startup check is enabled)");
        }
        section.checks.append(chk);
    }
    
    // 2. DCOP Server
    {
        AuditCheckResult chk;
        chk.name = i18n("DCOP Server");
        TQString dcopOut;
        int rc = runCommand("dcopserver --serverid 2>/dev/null || dcop 2>/dev/null | head -1", dcopOut);
        
        // Alternative: check for the DCOP socket directly
        TQString dcopSocket;
        TQString hostname;
        runCommand("hostname", hostname);
        TQString dcopAuthFile = homeDir + "/.DCOPserver_" + hostname + "__0";
        
        if (TQFile::exists(dcopAuthFile)) {
            chk.status = AuditPass;
            chk.detail = i18n("DCOP server is running (IPC communication available)");
        } else {
            // Try the fallback with underscores (hostname may have dots replaced)
            TQString dcopOut2;
            runCommand("ls " + homeDir + "/.DCOPserver_* 2>/dev/null | head -1", dcopOut2);
            if (!dcopOut2.isEmpty()) {
                chk.status = AuditPass;
                chk.detail = i18n("DCOP server is running");
            } else {
                chk.status = AuditFail;
                chk.detail = i18n("DCOP server does not appear to be running");
                chk.recommendation = i18n("DCOP is essential for TDE inter-process communication. Try restarting your TDE session");
            }
        }
        section.checks.append(chk);
    }
    
    // 3. TDE temp directory permissions
    {
        AuditCheckResult chk;
        chk.name = i18n("TDE Temporary Directory");
        TQString tmpDir = "/tmp/tde-" + userName;
        if (TQFile::exists(tmpDir)) {
            TQString permOut;
            runCommand("stat -c '%a %U' " + tmpDir, permOut);
            TQStringList parts = TQStringList::split(' ', permOut);
            int perm = parts.count() > 0 ? parts[0].toInt() : 0;
            TQString owner = parts.count() > 1 ? parts[1] : "";
            
            if (perm == 700 && owner == userName) {
                chk.status = AuditPass;
                chk.detail = i18n("TDE temp directory has correct permissions (700, owner: %1)").arg(userName);
            } else if (owner != userName) {
                chk.status = AuditFail;
                chk.detail = i18n("TDE temp directory is owned by '%1' instead of '%2'").arg(owner).arg(userName);
                chk.recommendation = i18n("This is a security risk. Another user may be able to intercept TDE communications. Delete and recreate the directory");
                chk.install_command = "rm -rf " + tmpDir + " && mkdir -m 700 " + tmpDir;
            } else {
                chk.status = AuditWarning;
                chk.detail = i18n("TDE temp directory has permissions %1 (should be 700)").arg(parts[0]);
                chk.recommendation = i18n("Restrict permissions to prevent other users from accessing TDE sockets");
                chk.install_command = "chmod 700 " + tmpDir;
            }
        } else {
            chk.status = AuditWarning;
            chk.detail = i18n("TDE temp directory does not exist (will be created at next login)");
            chk.recommendation = i18n("Ensure the directory is created automatically on login or create it manually");
        }
        section.checks.append(chk);
    }
    
    // 4. ICEauthority file permissions
    {
        AuditCheckResult chk;
        chk.name = i18n("ICE Authority File");
        TQString iceFile = homeDir + "/.ICEauthority";
        if (TQFile::exists(iceFile)) {
            TQString permOut;
            runCommand("stat -c '%a %U' " + iceFile, permOut);
            TQStringList parts = TQStringList::split(' ', permOut);
            int perm = parts.count() > 0 ? parts[0].toInt() : 0;
            TQString owner = parts.count() > 1 ? parts[1] : "";
            
            if ((perm == 600 || perm == 400) && owner == userName) {
                chk.status = AuditPass;
                chk.detail = i18n("ICE authority file has secure permissions (%1)").arg(parts[0]);
            } else if (owner != userName) {
                chk.status = AuditFail;
                chk.detail = i18n("ICE authority file is owned by '%1' instead of '%2'").arg(owner).arg(userName);
                chk.recommendation = i18n("This file controls DCOP authentication. Wrong ownership is a security risk");
            } else {
                chk.status = AuditWarning;
                chk.detail = i18n("ICE authority file has permissions %1 (should be 600)").arg(parts[0]);
                chk.recommendation = i18n("Restrict permissions to prevent unauthorized access to TDE IPC");
                chk.install_command = "chmod 600 " + iceFile;
            }
        } else {
            chk.status = AuditWarning;
            chk.detail = i18n("ICE authority file not found (DCOP authentication may not be configured)");
            chk.recommendation = i18n("Ensure TDE session is active or recreate .ICEauthority if it was deleted");
        }
        section.checks.append(chk);
    }
    
    return section;
}

void SystemAudit::runLynisAudit()
{
    AuditSectionResult section;
    section.category = "deep_scan_lynis";
    section.title = i18n("Lynis Security Audit");
    section.icon_name = "system-run";
    
    if (!isBinaryInstalled("lynis")) {
        AuditCheckResult check;
        check.name = i18n("Lynis");
        check.status = AuditUnknown;
        check.detail = i18n("Lynis is not installed");
        check.recommendation = i18n("Install Lynis for comprehensive security auditing");
        check.install_command = "sudo apt install lynis";
        section.checks.append(check);
        emit sectionComplete(section);
        return;
    }
    
    // Launch detached terminal via TDEProcess
    TDEProcess proc;
    proc << "konsole" << "-T" << "ClamUI - Lynis Audit" << "--noclose" << "-e" << "tdesudo" << "lynis" << "audit" << "system";
    proc.start(TDEProcess::DontCare);
}

void SystemAudit::runRootkitCheck()
{
    AuditSectionResult section;
    section.category = "deep_scan_rootkit";
    section.title = i18n("Rootkit Detection");
    section.icon_name = "system-run";
    
    if (!isBinaryInstalled("chkrootkit")) {
        AuditCheckResult check;
        check.name = i18n("chkrootkit");
        check.status = AuditUnknown;
        check.detail = i18n("chkrootkit is not installed");
        check.recommendation = i18n("Install chkrootkit for rootkit detection");
        check.install_command = "sudo apt install chkrootkit";
        section.checks.append(check);
        emit sectionComplete(section);
        return;
    }
    
    TDEProcess proc;
    proc << "konsole" << "-T" << "ClamUI - Rootkit Scan" << "--noclose" << "-e" << "tdesudo" << "chkrootkit";
    proc.start(TDEProcess::DontCare);
}

void SystemAudit::saveAuditReport(const AuditReport &report)
{
    // Serialize to TQStringList
    TQStringList data;
    data.append(TQString::number(report.timestamp));
    
    for (TQValueList<AuditSectionResult>::ConstIterator sit = report.sections.begin(); sit != report.sections.end(); ++sit) {
        data.append("SEC|" + (*sit).category + "|" + (*sit).title + "|" + (*sit).icon_name);
        for (TQValueList<AuditCheckResult>::ConstIterator cit = (*sit).checks.begin(); cit != (*sit).checks.end(); ++cit) {
            data.append("CHK|" + (*cit).name + "|" + TQString::number((*cit).status) + "|" + 
                        (*cit).detail + "|" + (*cit).recommendation + "|" + (*cit).install_command + "|" + 
                        (*cit).info_url + "|" + (*cit).launch_command + "|" + (*cit).launch_label);
        }
    }
    
    TQtStringSerializer ser;
    TQString serialized = ser.serialize(TQVariant(data));
    
    TQFile f(locateLocal("appdata", "audit_cache.txt"));
    if (f.open(IO_WriteOnly)) {
        TQTextStream t(&f);
        t.setEncoding(TQTextStream::UnicodeUTF8);
        t << serialized;
        f.close();
    }
}

AuditReport SystemAudit::loadAuditReport() const
{
    AuditReport report;
    TQFile f(locateLocal("appdata", "audit_cache.txt"));
    if (!f.open(IO_ReadOnly)) return report;
    
    TQTextStream t(&f);
    t.setEncoding(TQTextStream::UnicodeUTF8);
    TQString serialized = t.read();
    f.close();
    
    TQtStringSerializer ser;
    TQVariant var = ser.deserialize(serialized, TQVariant::StringList);
    TQStringList data = var.toStringList();
    
    if (data.isEmpty()) return report;
    
    report.timestamp = data[0].toDouble();
    
    for (int i = 1; i < (int)data.count(); ++i) {
        TQStringList parts = TQStringList::split("|", data[i], true);
        if (parts.count() > 0) {
            if (parts[0] == "SEC" && parts.count() >= 4) {
                AuditSectionResult sec;
                sec.category = parts[1];
                sec.title = parts[2];
                sec.icon_name = parts[3];
                report.sections.append(sec);
            } else if (parts[0] == "CHK" && parts.count() >= 9 && report.sections.count() > 0) {
                AuditCheckResult chk;
                chk.name = parts[1];
                chk.status = (AuditStatus)parts[2].toInt();
                chk.detail = parts[3];
                chk.recommendation = parts[4];
                chk.install_command = parts[5];
                chk.info_url = parts[6];
                chk.launch_command = parts[7];
                chk.launch_label = parts[8];
                report.sections.last().checks.append(chk);
            }
        }
    }
    
    return report;
}

#include "system_audit.moc"
