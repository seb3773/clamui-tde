#include "components_view.h"
#include "../core/clamav_detection.h"
#include "../core/settings_manager.h"
#include "mainwindow.h"

#include <kiconloader.h>
#include <ntqfile.h>
#include <ntqscrollview.h>
#include <ntqtextstream.h>
#include <ntqwidgetstack.h>
#include <tdeglobal.h>
#include <tdelocale.h>
#include <tqcolor.h>
#include <tqcursor.h>
#include <tqfont.h>
#include <tqframe.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqpushbutton.h>
#include "icon_utils.h"

// --- Distribution Detection ---
enum DistroFamily { DistroUnknown, DistroDebian, DistroFedora, DistroArch };

static DistroFamily detectDistro() {
  static bool checked = false;
  static DistroFamily family = DistroUnknown;

  if (checked)
    return family;
  checked = true;

  TQFile file("/etc/os-release");
  if (file.open(IO_ReadOnly)) {
    TQTextStream stream(&file);
    while (!stream.atEnd()) {
      TQString line = stream.readLine().lower();
      if (line.startsWith("id=") || line.startsWith("id_like=")) {
        if (line.contains("debian") || line.contains("ubuntu")) {
          family = DistroDebian;
          break;
        }
        if (line.contains("fedora") || line.contains("rhel") ||
            line.contains("centos") || line.contains("suse")) {
          family = DistroFedora;
          break;
        }
        if (line.contains("arch") || line.contains("manjaro")) {
          family = DistroArch;
          break;
        }
      }
    }
    file.close();
  }
  return family;
}

// --- ComponentRow Implementation ---

ComponentRow::ComponentRow(const TQString &title, const TQPixmap &iconPixmap,
                           TQWidget *parent)
    : TQWidget(parent), m_isExpanded(false), m_hasInstructions(false) {
  TQVBoxLayout *mainLayout = new TQVBoxLayout(this, 0, 0);

  // Header Widget
  TQWidget *headerWidget = new TQWidget(this);
  TQHBoxLayout *headerLayout = new TQHBoxLayout(headerWidget, 10, 10);

  m_mainIcon = new TQLabel(headerWidget);
  m_mainIcon->setPixmap(iconPixmap);
  m_mainIcon->setFixedSize(32, 32);
  headerLayout->addWidget(m_mainIcon);

  TQVBoxLayout *titleLayout = new TQVBoxLayout(headerLayout, 2);
  m_titleLabel = new TQLabel("<b>" + title + "</b>", headerWidget);
  titleLayout->addWidget(m_titleLabel);

  m_subtitleLabel = new TQLabel(headerWidget);
  TQFont subtitleFont = m_subtitleLabel->font();
  subtitleFont.setPointSize(subtitleFont.pointSize() - 1);
  m_subtitleLabel->setFont(subtitleFont);
  m_subtitleLabel->setPaletteForegroundColor(TQt::gray);
  titleLayout->addWidget(m_subtitleLabel);

  headerLayout->addStretch(1);

  m_statusIcon = new TQLabel(headerWidget);
  m_statusIcon->setFixedSize(16, 16);
  headerLayout->addWidget(m_statusIcon);

  m_statusLabel = new TQLabel(headerWidget);
  headerLayout->addWidget(m_statusLabel);

  m_expandIcon = new TQLabel(headerWidget);
  m_expandIcon->setPixmap(IconUtils::load(right_chevron_png, right_chevron_png_len, 24, 24));
  m_expandIcon->setFixedSize(24, 24);
  m_expandIcon->setAlignment(TQt::AlignCenter);
  m_expandIcon->hide(); // Hidden by default unless instructions exist
  headerLayout->addWidget(m_expandIcon);

  mainLayout->addWidget(headerWidget);

  // Body Widget (Instructions)
  m_bodyWidget = new TQWidget(this);
  m_bodyLayout = new TQVBoxLayout(m_bodyWidget, 10, 5);
  m_bodyWidget->hide();

  mainLayout->addWidget(m_bodyWidget);
}

void ComponentRow::mousePressEvent(TQMouseEvent *e) {
  if (e->button() == TQt::LeftButton && m_hasInstructions &&
      m_expandIcon->isVisible()) {
    toggleExpanded();
  }
  TQWidget::mousePressEvent(e);
}

void ComponentRow::setInstructions(const TQString &ubuntuCmd,
                                   const TQString &fedoraCmd,
                                   const TQString &archCmd,
                                   const TQString &notes) {
  auto addCmd = [&](const TQString &distro, const TQString &cmd) {
    TQString formattedCmd = cmd;
    formattedCmd.replace("\n", "<br>");
    TQLabel *lbl = new TQLabel("<b>" + distro + ":</b><br><code>" +
                                   formattedCmd + "</code><br>",
                               m_bodyWidget);
    lbl->setTextFormat(TQt::RichText);
    m_bodyLayout->addWidget(lbl);
  };

  DistroFamily distro = detectDistro();

  if (distro == DistroDebian || distro == DistroUnknown)
    addCmd("Ubuntu/Debian", ubuntuCmd);
  if (distro == DistroFedora || distro == DistroUnknown)
    addCmd("Fedora/SUSE", fedoraCmd);
  if (distro == DistroArch || distro == DistroUnknown)
    addCmd("Arch Linux", archCmd);

  if (!notes.isEmpty()) {
    TQLabel *noteLbl = new TQLabel("<i>" + notes + "</i>", m_bodyWidget);
    noteLbl->setTextFormat(TQt::RichText);
    noteLbl->setPaletteForegroundColor(TQt::gray);
    m_bodyLayout->addWidget(noteLbl);
  }

  m_hasInstructions = true;
}

void ComponentRow::setStatus(bool installed, const TQString &versionStr,
                             int daemonStatus) {
  if (daemonStatus != -1) {
    if (daemonStatus == 0) { // DaemonNotInstalled
      m_statusIcon->setPixmap(IconUtils::colorize(warning_png, warning_png_len, TQColor(212, 160, 23), 16, 16));
      m_statusLabel->setText(i18n("Not installed"));
      m_statusLabel->setPaletteForegroundColor(TQColor(212, 160, 23));
      m_subtitleLabel->setText(
          i18n("Not installed - expand for setup instructions"));
      m_expandIcon->show();
    } else if (daemonStatus == 1) { // DaemonStopped
      m_statusIcon->setPixmap(IconUtils::colorize(stop_png, stop_png_len, TQColor(255, 140, 0), 16, 16));
      m_statusLabel->setText(i18n("Stopped"));
      m_statusLabel->setPaletteForegroundColor(TQColor(255, 140, 0));
      m_subtitleLabel->setText(i18n("Daemon is installed but not running"));
      m_expandIcon->show();         // Show guide for stopped daemon too
    } else if (daemonStatus == 2) { // DaemonRunning
      m_statusIcon->setPixmap(IconUtils::colorize(check_png, check_png_len, TQt::green, 16, 16));
      m_statusLabel->setText(i18n("Running"));
      m_statusLabel->setPaletteForegroundColor(TQt::green);
      m_subtitleLabel->setText(i18n("Daemon is running"));
      m_expandIcon->hide();
      m_bodyWidget->hide();
      m_isExpanded = false;
      m_expandIcon->setPixmap(IconUtils::load(right_chevron_png, right_chevron_png_len, 24, 24));
    }
  } else {
    if (installed) {
      m_statusIcon->setPixmap(IconUtils::colorize(check_png, check_png_len, TQt::green, 16, 16));
      m_statusLabel->setText(i18n("Installed"));
      m_statusLabel->setPaletteForegroundColor(TQt::green);
      if (!versionStr.isEmpty() && versionStr != i18n("Unknown") &&
          versionStr != i18n("Not installed")) {
        m_subtitleLabel->setText(versionStr);
      } else {
        m_subtitleLabel->setText(i18n("Installed"));
      }
      m_expandIcon->hide();
      m_bodyWidget->hide();
      m_isExpanded = false;
      m_expandIcon->setPixmap(IconUtils::load(right_chevron_png, right_chevron_png_len, 24, 24));
    } else {
      m_statusIcon->setPixmap(IconUtils::colorize(warning_png, warning_png_len, TQColor(212, 160, 23), 16, 16));
      m_statusLabel->setText(i18n("Not installed"));
      m_statusLabel->setPaletteForegroundColor(TQColor(212, 160, 23));
      m_subtitleLabel->setText(
          i18n("Not installed - expand for setup instructions"));
      m_expandIcon->show();
    }
  }

  // Ensure button icon matches state if it's shown
  if (m_expandIcon->isVisible()) {
    m_expandIcon->setPixmap(IconUtils::load(m_isExpanded ? down_chevron_png : right_chevron_png, m_isExpanded ? down_chevron_png_len : right_chevron_png_len, 24, 24));
    this->setCursor(TQCursor(TQt::PointingHandCursor));
  } else {
    this->setCursor(TQCursor(TQt::ArrowCursor));
  }
}

void ComponentRow::applyTheme(bool isDark) {
  TQColor bgColor = isDark ? TQColor(0, 0, 0) : TQColor(255, 255, 255);
  if (m_statusLabel) m_statusLabel->setPaletteBackgroundColor(bgColor);
  if (m_subtitleLabel) m_subtitleLabel->setPaletteBackgroundColor(bgColor);
}

void ComponentRow::toggleExpanded() {
  if (!m_hasInstructions)
    return;

  m_isExpanded = !m_isExpanded;
  m_bodyWidget->setShown(m_isExpanded);
  m_expandIcon->setPixmap(IconUtils::load(m_isExpanded ? down_chevron_png : right_chevron_png, m_isExpanded ? down_chevron_png_len : right_chevron_png_len, 24, 24));
}

void ComponentRow::reloadIcon(const TQPixmap &iconPixmap) {
  m_mainIcon->setPixmap(iconPixmap);
  if (m_expandIcon->isVisible()) {
    m_expandIcon->setPixmap(IconUtils::load(m_isExpanded ? down_chevron_png : right_chevron_png, m_isExpanded ? down_chevron_png_len : right_chevron_png_len, 24, 24));
  }
}

// --- ComponentsView Implementation ---

#include <ntqevent.h>

ComponentsView::ComponentsView(TQWidget *parent, TQWidgetStack *stack)
    : TQWidget(parent), m_firstShow(true) {
  setupUI();
  applyTheme();
}

ComponentsView::~ComponentsView() {}

void ComponentsView::showEvent(TQShowEvent *event) {
  if (m_firstShow) {
    refreshStatus();
    m_firstShow = false;
  }
  TQWidget::showEvent(event);
}

void ComponentsView::setupUI() {
  TQVBoxLayout *mainLayout = new TQVBoxLayout(this, 20, 15);

  TQHBoxLayout *headerLayout = new TQHBoxLayout(mainLayout, 10);
  TQLabel *titleLabel = new TQLabel(i18n("<h2>ClamAV Components</h2>"), this);
  headerLayout->addWidget(titleLabel, 1);

  m_refreshBtn =
      new TQPushButton(SmallIcon("view-refresh"), i18n("Refresh"), this);
  connect(m_refreshBtn, TQT_SIGNAL(clicked()), this, TQT_SLOT(refreshStatus()));
  headerLayout->addWidget(m_refreshBtn);

  TQLabel *descLabel =
      new TQLabel(i18n("Check the installation status of required ClamAV tools "
                       "on your system."),
                  this);
  descLabel->setAlignment(TQt::WordBreak | TQt::AlignLeft);
  mainLayout->addWidget(descLabel);

  TQFrame *line = new TQFrame(this);
  line->setFrameStyle(TQFrame::HLine | TQFrame::Sunken);
  mainLayout->addWidget(line);

  TQScrollView *scrollView = new TQScrollView(this);
  scrollView->setFrameStyle(TQScrollView::NoFrame);
  scrollView->setVScrollBarMode(TQScrollView::Auto);
  scrollView->setHScrollBarMode(TQScrollView::AlwaysOff);
  scrollView->setResizePolicy(TQScrollView::AutoOneFit);
  mainLayout->addWidget(scrollView, 1);

  TQWidget *scrollWidget = new TQWidget(scrollView->viewport());
  scrollView->addChild(scrollWidget);

  TQVBoxLayout *scrollLayout = new TQVBoxLayout(scrollWidget, 0, 0);

  // Flat frame to contain the list
  m_listFrame = new TQFrame(scrollWidget);
  m_listFrame->setFrameStyle(TQFrame::StyledPanel | TQFrame::Sunken);

  scrollLayout->addWidget(m_listFrame);
  scrollLayout->addStretch(1);

  TQVBoxLayout *listLayout = new TQVBoxLayout(m_listFrame, 5, 0);

  // Clamscan
  m_rowClamscan =
      new ComponentRow(i18n("Virus Scanner"), IconUtils::load(find_png, find_png_len, 32, 32), m_listFrame);
  m_rowClamscan->setInstructions("sudo apt install clamav",
                                 "sudo dnf install clamav",
                                 "sudo pacman -S clamav", "");
  listLayout->addWidget(m_rowClamscan);

  // Separator line
  TQFrame *sep1 = new TQFrame(m_listFrame);
  sep1->setFrameStyle(TQFrame::HLine | TQFrame::Sunken);
  listLayout->addWidget(sep1);

  // Freshclam
  m_rowFreshclam = new ComponentRow(i18n("Database Updater"),
                                    IconUtils::load(network_server_symbolic_png, network_server_symbolic_png_len, 32, 32), m_listFrame);
  m_rowFreshclam->setInstructions("sudo apt install clamav-freshclam",
                                  "sudo dnf install clamav-update",
                                  "sudo pacman -S clamav",
                                  "Note: freshclam updates the virus database. "
                                  "Enable the service for automatic updates.");
  listLayout->addWidget(m_rowFreshclam);

  // Separator line
  TQFrame *sep2 = new TQFrame(m_listFrame);
  sep2->setFrameStyle(TQFrame::HLine | TQFrame::Sunken);
  listLayout->addWidget(sep2);

  // Clamdscan
  m_rowClamdscan = new ComponentRow(i18n("Daemon Scanner Client"),
                                    IconUtils::load(view_png, view_png_len, 32, 32), m_listFrame);
  m_rowClamdscan->setInstructions("sudo apt install clamav-daemon",
                                  "sudo dnf install clamd",
                                  "sudo pacman -S clamav", "");
  listLayout->addWidget(m_rowClamdscan);

  // Separator line
  TQFrame *sep3 = new TQFrame(m_listFrame);
  sep3->setFrameStyle(TQFrame::HLine | TQFrame::Sunken);
  listLayout->addWidget(sep3);

  // Clamd
  m_rowClamd =
      new ComponentRow(i18n("Scanner Daemon"), IconUtils::load(focus_png, focus_png_len, 32, 32), m_listFrame);
  m_rowClamd->setInstructions(
      "sudo apt install clamav-daemon\nsudo systemctl enable "
      "clamav-daemon\nsudo systemctl start clamav-daemon",
      "sudo dnf install clamd clamav-update\nsudo freshclam\nsudo systemctl "
      "enable clamd@scan\nsudo systemctl start clamd@scan",
      "sudo pacman -S clamav\nsudo systemctl enable clamav-daemon\nsudo "
      "systemctl start clamav-daemon",
      "");
  listLayout->addWidget(m_rowClamd);
}

void ComponentsView::refreshStatus() {
  bool isClamscanInstalled = ClamAVDetection::isClamscanInstalled();
  bool isFreshclamInstalled = ClamAVDetection::isFreshclamInstalled();
  bool isClamdscanInstalled = ClamAVDetection::isClamdscanInstalled();
  int daemonStatus = (int)ClamAVDetection::getDaemonStatus();

  m_rowClamscan->setStatus(
      isClamscanInstalled,
      isClamscanInstalled ? ClamAVDetection::getClamscanVersion() : "");

  TQString freshclamSubtitle =
      isFreshclamInstalled ? ClamAVDetection::getDatabaseDate() : "";
  m_rowFreshclam->setStatus(isFreshclamInstalled, freshclamSubtitle);

  m_rowClamdscan->setStatus(
      isClamdscanInstalled,
      isClamdscanInstalled ? ClamAVDetection::getClamscanVersion() : "");
  m_rowClamd->setStatus(daemonStatus > 0, "", daemonStatus);
}

void ComponentsView::applyTheme() {
  ClamMainWindow *mainWindow = dynamic_cast<ClamMainWindow *>(topLevelWidget());
  bool isDark = false;
  if (mainWindow && mainWindow->settingsManager()) {
    isDark = (mainWindow->settingsManager()->value("dark_mode", "false") == "true");
  }
  
  TQColor bgColor = isDark ? TQColor(0, 0, 0) : TQColor(255, 255, 255);
  TQColor fgColor = isDark ? TQColor(200, 200, 200) : TQColor(0, 0, 0);
  
  TQPalette pal = palette();
  pal.setColor(TQColorGroup::Background, bgColor);
  pal.setColor(TQColorGroup::Foreground, fgColor);
  pal.setColor(TQColorGroup::Text, fgColor);
  
  m_listFrame->setPalette(pal);
  m_listFrame->setPaletteBackgroundColor(bgColor);
  
  if (m_rowClamscan) m_rowClamscan->applyTheme(isDark);
  if (m_rowFreshclam) m_rowFreshclam->applyTheme(isDark);
  if (m_rowClamd) m_rowClamd->applyTheme(isDark);
  if (m_rowClamdscan) m_rowClamdscan->applyTheme(isDark);
}

void ComponentsView::reloadIcons() {
  if (m_rowClamscan) m_rowClamscan->reloadIcon(IconUtils::load(find_png, find_png_len, 32, 32));
  if (m_rowFreshclam) m_rowFreshclam->reloadIcon(IconUtils::load(network_server_symbolic_png, network_server_symbolic_png_len, 32, 32));
  if (m_rowClamdscan) m_rowClamdscan->reloadIcon(IconUtils::load(view_png, view_png_len, 32, 32));
  if (m_rowClamd) m_rowClamd->reloadIcon(IconUtils::load(focus_png, focus_png_len, 32, 32));
  if (m_refreshBtn) m_refreshBtn->setIconSet(TQIconSet(SmallIcon("view-refresh")));
}

void ComponentsView::onSettingsChanged() {
  applyTheme();
  reloadIcons();
  refreshStatus();
}

#include "components_view.moc"
