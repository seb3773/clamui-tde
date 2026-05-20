# TDEClamUI

![Konqi clamui](about_github.png)

**TDEClamUI** is a lightweight, ultra-fast, native frontend for the **ClamAV** antivirus, specifically designed and optimized for the **Trinity Desktop Environment (TDE)**.

Originally inspired by ClamUI (written in Python/GTK4/libadwaita), this project is a complete clean-room rewrite in **C++** using the **TQt3 and TDE APIs**. It delivers a high-performance native desktop experience with near-zero startup latency, minimal memory usage, and advanced security features.

---

## Key Advantages Over the Original ClamUI

### 1. Low Resource Usage & Instant Startup
* **Zero Python Overhead**: By dropping the Python runtime, PyGObject, and GTK4 dependencies, TDEClamUI starts **instantly** (in less than 20 milliseconds) and consumes a fraction of the RAM.
* **Cached Versioning & Async Startup**: Rather than performing blocking, synchronous shell executions (`clamscan --version`, `systemctl`) on the GUI thread during window construction, TDEClamUI uses smart static caching and deferred single-shot asynchronous updates. The UI is completely responsive from the first millisecond.
* **Smart Tab Initialization**: Heavy tabs (like Components detection) defer system queries until they are actually shown to the user, ensuring the core scanner remains incredibly snappy.

### 2. Advanced Antivirus Databases & Unofficial Signatures
* **Out-of-the-Box Unofficial Bases**: Direct integration with high-quality third-party signature databases, including **SaneSecurity** (phishing, scam, spam, malware) and **URLhaus** (active malicious URLs).
* **Automatic Live Updates**: A secure updater pipeline that pulls daily signatures using `rsync` (with signature verification via `gpg` keyrings) and `curl`/`wget` with fallback mechanisms.
* **CDN Rate-Limit Auto-Reset**: When the user changes their database mirror inside the preferences, TDEClamUI automatically clears freshclam CDN rate-limit logs (`freshclam.dat`), bypassing CDN blocks immediately and elegantly.

### 3. System Security Audit tailored to TDE
* **Advanced Audit Engine**: Evaluates the system's security posture and generates rich structural cards detailing check results and mitigation actions.
* **TDE-Specific Integrity Checks**: Specifically monitors security flags and runtime constraints of the Trinity Desktop Environment (such as secure X11 authorizations, IPC permissions, active KRunner/TDEInit states, and correct user privileges).

### 4. Advanced Ergonomics
* **Consistent Layout System**: Structured using native TQt layouts where all view titles and descriptions utilize a unified HTML `<h2>` rendering format with correct word-wrapping policies, preventing broken layouts or truncated labels.
* **State Preservation & Profiles**: Remembers the exact last used scanning profile (e.g. Quick Scan, Home, Custom) and restores its selected targets and options seamlessly upon subsequent app launches.
* **Color-Coded Live Statuses**: The main status bar dynamically shows current engine statistics and service states with high-contrast, premium color indicators (e.g., active services in blue, stopped or warning states in orange).
* **Polished Signature Formatting**: Large numbers (e.g. database signature counts) are automatically formatted with groups of three digits (e.g. `1 275 321` instead of `1275321`) for superior readability.

### 5. Secure Scheduler
* **Secure CLI Orchestration**: Provides a solid, non-intrusive scanning scheduler utilizing either user systemd timers or cron tab engines.
* **Command Injection Protection**: Implements robust command building via strict argument escaping and quote validation, completely eliminating terminal hijack risks.

---

## Developer Scripts

To facilitate deployment, custom utility scripts to manage packaging and cleaning are included:

* **`clean.sh`**: Instantly wipes all out-of-tree CMake build directories, leaving the repository in a pristine, git-ready state.
* **`build_deb.sh`**: Compiles and packages TDEClamUI into a standard, lightweight, and installable Debian package (`.deb`).
  * **Minimal & Optimal Dependencies**: Features a clean control file without bloated or redundant library dependencies (such as `libtqt3-mt`, which is already pulled by TDE libraries), avoiding package manager conflicts:
    ```control
    Depends: tdelibs14-trinity (>= 4:14.1.1), clamav, clamav-freshclam, rsync, gnupg, curl
    ```
  * **Custom Branding**: Stages and registers 64x64 application icon under `/usr/share/icons/hicolor` and links all standard sizes (16x16 up to 48x48) automatically.

---

## Compilation & Packaging

### 1. Build from Source
To build the application locally:
```bash
./build.sh
```
The compiled executable will be located under `build/src/tdeclamui`.

### 2. Generate Debian Package
To compile and generate an installable `.deb` package:
```bash
./build_deb.sh
```

### 3. Clean Project Artifacts
To reset the repository:
```bash
./clean.sh
```

---

## 📄 License
This project is licensed under the MIT License - see the LICENSE file for details.
