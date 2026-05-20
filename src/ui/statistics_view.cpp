/*
 * statistics_view.cpp - Statistics View
 */

#include "statistics_view.h"
#include "../core/log_manager.h"
#include "../core/settings_manager.h"
#include "mainwindow.h"

#include <kiconloader.h>
#include <ntqscrollview.h>
#include <ntqwidgetstack.h>
#include <tdeglobal.h>
#include <tdelocale.h>
#include <tqcolor.h>
#include <tqcombobox.h>
#include <tqfont.h>
#include <tqframe.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqpushbutton.h>
#include <tqtstackedbarchartwidget.h>
#include "icon_utils.h"

StatisticsView::StatisticsView(TQWidget *parent, TQWidgetStack *stack)
    : TQWidget(parent) {
  m_logManager = new LogManager(this);
  m_stats = new Statistics(m_logManager, this);

  setupUI();
  refreshStats();
  applyTheme();
}

StatisticsView::~StatisticsView() {}

TQWidget *StatisticsView::createRow(TQWidget *parent, const TQPixmap &iconPixmap,
                                    const TQString &title,
                                    const TQString &subtitle,
                                    TQLabel **valueLabel,
                                    TQLabel **iconLabelOut) {
  TQWidget *row = new TQWidget(parent);
  TQHBoxLayout *layout = new TQHBoxLayout(row, 10, 5);

  TQLabel *iconLabel = new TQLabel(row);
  iconLabel->setPixmap(iconPixmap);
  iconLabel->setFixedSize(24, 24);
  iconLabel->setAlignment(TQt::AlignCenter);
  if (iconLabelOut) *iconLabelOut = iconLabel;
  layout->addWidget(iconLabel);

  TQVBoxLayout *textLayout = new TQVBoxLayout(layout, 0);
  TQLabel *titleLbl = new TQLabel("<b>" + title + "</b>", row);
  textLayout->addWidget(titleLbl);

  TQLabel *subLbl = new TQLabel(subtitle, row);
  subLbl->setPaletteForegroundColor(TQColor(136, 136, 136));
  m_subtitleLabels.append(subLbl);
  TQFont subFont = subLbl->font();
  subFont.setPointSize(subFont.pointSize() - 1);
  subLbl->setFont(subFont);
  textLayout->addWidget(subLbl);

  layout->addStretch(1);

  *valueLabel = new TQLabel(row);
  TQFont valFont = (*valueLabel)->font();
  valFont.setPointSize(valFont.pointSize() + 4);
  valFont.setBold(true);
  (*valueLabel)->setFont(valFont);
  (*valueLabel)->setAlignment(TQt::AlignRight | TQt::AlignVCenter);
  layout->addWidget(*valueLabel);

  return row;
}

void StatisticsView::setupUI() {
  m_subtitleLabels.setAutoDelete(false);
  TQVBoxLayout *outerLayout = new TQVBoxLayout(this, 11, 6);

  // Header
  TQLabel *titleLabel = new TQLabel(i18n("<h2>System Statistics</h2>"), this);
  outerLayout->addWidget(titleLabel);

  TQLabel *descLabel = new TQLabel(i18n("View system protection status and scan trends."), this);
  descLabel->setAlignment(TQt::WordBreak | TQt::AlignLeft);
  outerLayout->addWidget(descLabel);

  TQFrame *line = new TQFrame(this);
  line->setFrameStyle(TQFrame::HLine | TQFrame::Sunken);
  outerLayout->addWidget(line);

  outerLayout->addSpacing(10);

  TQScrollView *scrollView = new TQScrollView(this);
  scrollView->setFrameStyle(TQFrame::NoFrame);
  scrollView->setResizePolicy(TQScrollView::AutoOneFit);
  scrollView->setHScrollBarMode(TQScrollView::AlwaysOff);
  outerLayout->addWidget(scrollView, 1);

  TQWidget *content = new TQWidget(scrollView->viewport());
  scrollView->addChild(content);
  TQVBoxLayout *mainLayout = new TQVBoxLayout(content, 20, 15);

  // --- Protection Status Section ---
  TQVBoxLayout *protLayout = new TQVBoxLayout(mainLayout, 5);
  TQLabel *protTitle =
      new TQLabel("<b>" + i18n("Protection Status") + "</b>", content);
  protLayout->addWidget(protTitle);
  TQLabel *protSub = new TQLabel(i18n("Current system security posture"), content);
  protSub->setPaletteForegroundColor(TQColor(136, 136, 136));
  protLayout->addWidget(protSub);

  // Protection Status Frame
  m_protFrame = new TQFrame(content);
  m_protFrame->setFrameStyle(TQFrame::Box | TQFrame::Plain);
  m_protFrame->setLineWidth(1);
  TQVBoxLayout *protFrameLayout = new TQVBoxLayout(m_protFrame, 5, 0);

  TQWidget *sysStatRow = new TQWidget(m_protFrame);
  TQHBoxLayout *sysStatLayout = new TQHBoxLayout(sysStatRow, 10, 0);
  m_lblSystemProtectedIcon = new TQLabel(sysStatRow);
  m_lblSystemProtectedIcon->setFixedSize(24, 24);
  sysStatLayout->addWidget(m_lblSystemProtectedIcon);
  TQVBoxLayout *sysStatText = new TQVBoxLayout(sysStatLayout, 0);
  sysStatText->addWidget(
      new TQLabel("<b>" + i18n("System Status") + "</b>", sysStatRow));
  m_lblSystemProtectedSub = new TQLabel(sysStatRow);
  sysStatText->addWidget(m_lblSystemProtectedSub);
  sysStatLayout->addStretch(1);
  m_lblSystemProtected = new TQLabel(sysStatRow);
  sysStatLayout->addWidget(m_lblSystemProtected);
  protFrameLayout->addWidget(sysStatRow);

  TQFrame *line1 = new TQFrame(m_protFrame);
  line1->setFrameStyle(TQFrame::HLine | TQFrame::Sunken);
  protFrameLayout->addWidget(line1);

  TQWidget *lastScanRow = new TQWidget(m_protFrame);
  TQHBoxLayout *lastScanLayout = new TQHBoxLayout(lastScanRow, 10, 0);
  m_clockIcon = new TQLabel(lastScanRow);
  m_clockIcon->setPixmap(IconUtils::load(recent_png, recent_png_len, 24, 24));
  m_clockIcon->setFixedSize(24, 24);
  lastScanLayout->addWidget(m_clockIcon);
  TQVBoxLayout *lastScanText = new TQVBoxLayout(lastScanLayout, 0);
  lastScanText->addWidget(
      new TQLabel("<b>" + i18n("Last Scan") + "</b>", lastScanRow));
  m_lblLastScan = new TQLabel(lastScanRow);
  lastScanText->addWidget(m_lblLastScan);
  lastScanLayout->addStretch(1);
  protFrameLayout->addWidget(lastScanRow);

  mainLayout->addWidget(m_protFrame);

  // --- Timeframe Section ---
  TQHBoxLayout *timeLayout = new TQHBoxLayout(mainLayout, 10);
  TQVBoxLayout *timeTextLayout = new TQVBoxLayout(timeLayout, 0);
  timeTextLayout->addWidget(
      new TQLabel("<b>" + i18n("Timeframe") + "</b>", content));
  TQLabel *timeSub = new TQLabel(i18n("Select the time period for statistics"), content);
  timeSub->setPaletteForegroundColor(TQColor(136, 136, 136));
  timeTextLayout->addWidget(timeSub);

  timeLayout->addStretch(1);

  m_timeFrameCombo = new TQComboBox(content);
  m_timeFrameCombo->insertItem(i18n("Day"));
  m_timeFrameCombo->insertItem(i18n("Week"));
  m_timeFrameCombo->insertItem(i18n("Month"));
  m_timeFrameCombo->insertItem(i18n("All Time"));
  m_timeFrameCombo->setCurrentItem(3);
  connect(m_timeFrameCombo, TQT_SIGNAL(activated(int)), this,
          TQT_SLOT(timeFrameChanged(int)));
  timeLayout->addWidget(m_timeFrameCombo);

  // --- Scan Statistics Section ---
  TQVBoxLayout *statLayout = new TQVBoxLayout(mainLayout, 5);
  statLayout->addWidget(
      new TQLabel("<b>" + i18n("Scan Statistics") + "</b>", content));

  m_statFrame = new TQFrame(content);
  m_statFrame->setFrameStyle(TQFrame::Box | TQFrame::Plain);
  m_statFrame->setLineWidth(1);
  TQVBoxLayout *statFrameLayout = new TQVBoxLayout(m_statFrame, 5, 0);

  statFrameLayout->addWidget(createRow(m_statFrame, IconUtils::load(find_png, find_png_len, 24, 24), i18n("Total Scans"),
                                       i18n("Files and directories checked"), &m_lblTotalScans, &m_statIcon1));
  statFrameLayout->addWidget(createRow(m_statFrame, IconUtils::load(folder_png, folder_png_len, 24, 24), i18n("Files Scanned"),
                                       i18n("Individual items analyzed"), &m_lblFilesScanned, &m_statIcon2));
  statFrameLayout->addWidget(createRow(m_statFrame, IconUtils::load(virus_png, virus_png_len, 24, 24), i18n("Threats Found"),
                                       i18n("Malware and viruses detected"), &m_lblThreatsFound, &m_statIcon3));
  statFrameLayout->addWidget(createRow(m_statFrame, IconUtils::colorize(check_png, check_png_len, TQt::green, 24, 24), i18n("Clean Scans"),
                                       i18n("Scans with no threats found"), &m_lblCleanScans, &m_statIcon4));
  statFrameLayout->addWidget(createRow(m_statFrame, IconUtils::load(duration_png, duration_png_len, 24, 24), i18n("Average Duration"),
                                       i18n("Average time per scan"), &m_lblAvgDuration, &m_statIcon5));

  mainLayout->addWidget(m_statFrame);

  // --- Scan Activity (Chart) Section ---
  TQVBoxLayout *chartLayout = new TQVBoxLayout(mainLayout, 5);
  chartLayout->addWidget(
      new TQLabel("<b>" + i18n("Scan Activity") + "</b>", content));
  TQLabel *trendSub = new TQLabel(i18n("Scan trends over the selected timeframe"), content);
  trendSub->setPaletteForegroundColor(TQColor(136, 136, 136));
  chartLayout->addWidget(trendSub);

  m_chartFrame = new TQFrame(content);
  m_chartFrame->setFrameStyle(TQFrame::Box | TQFrame::Plain);
  m_chartFrame->setLineWidth(1);
  m_chartFrame->setMinimumHeight(200);
  TQVBoxLayout *chartFrameLayout = new TQVBoxLayout(m_chartFrame, 10, 10);

  m_chartWidget = new TQtStackedBarChartWidget(m_chartFrame);
  m_chartWidget->setCategorySpacing(15);

  chartFrameLayout->addWidget(m_chartWidget);

  chartLayout->addWidget(m_chartFrame);

  // -------------------------------------------------------------
  // Quick Actions Group
  // -------------------------------------------------------------
  TQLabel *lblActions = new TQLabel(i18n("<b>Quick Actions</b>"), content);
  mainLayout->addWidget(lblActions);

  m_actionsFrame = new TQFrame(content);
  m_actionsFrame->setFrameStyle(TQFrame::Box | TQFrame::Plain);
  m_actionsFrame->setLineWidth(1);
  TQVBoxLayout *actionsFrameLayout = new TQVBoxLayout(m_actionsFrame, 5, 0);

  // System Scan Row
  TQWidget *quickScanRow = new TQWidget(m_actionsFrame);
  TQHBoxLayout *quickScanLayout = new TQHBoxLayout(quickScanRow, 5, 6);
  m_icnQuickScan = new TQLabel(quickScanRow);
  m_icnQuickScan->setPixmap(IconUtils::load(start_png, start_png_len, 24, 24));
  quickScanLayout->addWidget(m_icnQuickScan);

  TQVBoxLayout *quickScanTextLayout = new TQVBoxLayout(quickScanLayout, 0, 0);
  quickScanTextLayout->addWidget(new TQLabel(i18n("<b>System Scan</b>"), quickScanRow));
  TQLabel *quickScanSub = new TQLabel(i18n("Scan critical system areas"), quickScanRow);
  quickScanSub->setPaletteForegroundColor(TQt::darkGray);
  m_subtitleLabels.append(quickScanSub);
  quickScanTextLayout->addWidget(quickScanSub);

  quickScanLayout->addStretch(1);
  m_btnQuickScan = new TQPushButton(IconUtils::load(right_chevron_png, right_chevron_png_len, 16, 16), "", quickScanRow);
  m_btnQuickScan->setFlat(true);
  connect(m_btnQuickScan, TQT_SIGNAL(clicked()), this, TQT_SLOT(quickScanClicked()));
  quickScanLayout->addWidget(m_btnQuickScan);

  actionsFrameLayout->addWidget(quickScanRow);

  // Divider
  TQFrame *divider2 = new TQFrame(m_actionsFrame);
  divider2->setFrameStyle(TQFrame::HLine | TQFrame::Sunken);
  actionsFrameLayout->addWidget(divider2);

  // View Logs Row
  TQWidget *viewLogsRow = new TQWidget(m_actionsFrame);
  TQHBoxLayout *viewLogsLayout = new TQHBoxLayout(viewLogsRow, 5, 6);
  m_icnViewLogs = new TQLabel(viewLogsRow);
  m_icnViewLogs->setPixmap(IconUtils::load(focus_png, focus_png_len, 24, 24));
  viewLogsLayout->addWidget(m_icnViewLogs);

  TQVBoxLayout *viewLogsTextLayout = new TQVBoxLayout(viewLogsLayout, 0, 0);
  viewLogsTextLayout->addWidget(new TQLabel(i18n("<b>View Scan Logs</b>"), viewLogsRow));
  TQLabel *viewLogsSub = new TQLabel(i18n("Review past scan history"), viewLogsRow);
  viewLogsSub->setPaletteForegroundColor(TQt::darkGray);
  m_subtitleLabels.append(viewLogsSub);
  viewLogsTextLayout->addWidget(viewLogsSub);

  viewLogsLayout->addStretch(1);
  m_btnViewLogs = new TQPushButton(IconUtils::load(right_chevron_png, right_chevron_png_len, 16, 16), "", viewLogsRow);
  m_btnViewLogs->setFlat(true);
  connect(m_btnViewLogs, TQT_SIGNAL(clicked()), this, TQT_SLOT(viewLogsClicked()));
  viewLogsLayout->addWidget(m_btnViewLogs);

  actionsFrameLayout->addWidget(viewLogsRow);

  mainLayout->addWidget(m_actionsFrame);

  mainLayout->addStretch(1);
}

void StatisticsView::applyTheme() {
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
  
  m_protFrame->setPalette(pal);
  m_protFrame->setPaletteBackgroundColor(bgColor);
  
  m_statFrame->setPalette(pal);
  m_statFrame->setPaletteBackgroundColor(bgColor);
  
  m_chartFrame->setPalette(pal);
  m_chartFrame->setPaletteBackgroundColor(bgColor);
  
  m_actionsFrame->setPalette(pal);
  m_actionsFrame->setPaletteBackgroundColor(bgColor);
  
  // Explicitly apply background color to detached labels
  for (TQLabel *lbl = m_subtitleLabels.first(); lbl; lbl = m_subtitleLabels.next()) {
      lbl->setPaletteBackgroundColor(bgColor);
  }
  
  if (m_lblThreatsFound) m_lblThreatsFound->setPaletteBackgroundColor(bgColor);
  if (m_lblSystemProtected) m_lblSystemProtected->setPaletteBackgroundColor(bgColor);
  if (m_lblSystemProtectedSub) m_lblSystemProtectedSub->setPaletteBackgroundColor(bgColor);
  if (m_lblLastScan) m_lblLastScan->setPaletteBackgroundColor(bgColor);
}

void StatisticsView::reloadIcons()
{
  if (m_clockIcon) m_clockIcon->setPixmap(IconUtils::load(recent_png, recent_png_len, 24, 24));
  if (m_icnQuickScan) m_icnQuickScan->setPixmap(IconUtils::load(start_png, start_png_len, 24, 24));
  if (m_icnViewLogs) m_icnViewLogs->setPixmap(IconUtils::load(focus_png, focus_png_len, 24, 24));
  if (m_btnQuickScan) m_btnQuickScan->setIconSet(TQIconSet(IconUtils::load(right_chevron_png, right_chevron_png_len, 16, 16)));
  if (m_btnViewLogs) m_btnViewLogs->setIconSet(TQIconSet(IconUtils::load(right_chevron_png, right_chevron_png_len, 16, 16)));
  
  if (m_statIcon1) m_statIcon1->setPixmap(IconUtils::load(find_png, find_png_len, 24, 24));
  if (m_statIcon2) m_statIcon2->setPixmap(IconUtils::load(folder_png, folder_png_len, 24, 24));
  if (m_statIcon3) m_statIcon3->setPixmap(IconUtils::load(virus_png, virus_png_len, 24, 24));
  if (m_statIcon4) m_statIcon4->setPixmap(IconUtils::colorize(check_png, check_png_len, TQt::green, 24, 24));
  if (m_statIcon5) m_statIcon5->setPixmap(IconUtils::load(duration_png, duration_png_len, 24, 24));
}

void StatisticsView::onSettingsChanged() {
  applyTheme();
  reloadIcons();
}

void StatisticsView::setScanRunningState(bool running) {
  if (m_btnQuickScan) m_btnQuickScan->setEnabled(!running);
}

void StatisticsView::showEvent(TQShowEvent *e) {
  TQWidget::showEvent(e);
  // Reload logs from disk and refresh stats when page becomes visible
  delete m_stats;
  delete m_logManager;
  m_logManager = new LogManager(this);
  m_stats = new Statistics(m_logManager, this);
  refreshStats();
}

void StatisticsView::quickScanClicked() {
  ClamMainWindow *mainWindow = dynamic_cast<ClamMainWindow *>(topLevelWidget());
  if (mainWindow) {
    mainWindow->slotQuickScan();
  }
}

void StatisticsView::viewLogsClicked() {
  ClamMainWindow *mainWindow = dynamic_cast<ClamMainWindow *>(topLevelWidget());
  if (mainWindow) {
    mainWindow->switchToView(2); // 2 is VIEW_LOGS
  }
}

void StatisticsView::timeFrameChanged(int) { refreshStats(); }

void StatisticsView::refreshStats() {
  Statistics::TimeFrame tf = Statistics::ALL_TIME;
  int id = m_timeFrameCombo->currentItem();
  if (id == 0)
    tf = Statistics::TODAY;
  else if (id == 1)
    tf = Statistics::WEEK;
  else if (id == 2)
    tf = Statistics::MONTH;
  else if (id == 3)
    tf = Statistics::ALL_TIME;

  StatsData data = m_stats->getStats(tf);

  m_lblTotalScans->setText(TQString::number(data.totalScans));
  m_lblFilesScanned->setText(TQString::number(data.filesScanned));
  m_lblThreatsFound->setText(TQString::number(data.threatsFound));

  if (data.threatsFound > 0) {
    m_lblThreatsFound->setPaletteForegroundColor(TQt::red);
  } else {
    m_lblThreatsFound->setPaletteForegroundColor(colorGroup().text());
  }

  m_lblCleanScans->setText(TQString::number(data.cleanScans));
  m_lblAvgDuration->setText(TQString::number(data.avgDurationSeconds, 'f', 1) +
                            "s");

  if (m_stats->isSystemProtected()) {
    m_lblSystemProtectedIcon->setPixmap(IconUtils::colorize(check_png, check_png_len, TQt::green, 24, 24));
    m_lblSystemProtected->setText(i18n("Protected"));
    m_lblSystemProtected->setPaletteForegroundColor(TQt::green);
    m_lblSystemProtectedSub->setText(i18n("System is protected"));
    m_lblSystemProtectedSub->setPaletteForegroundColor(TQColor(136, 136, 136));
  } else {
    m_lblSystemProtectedIcon->setPixmap(IconUtils::colorize(warning_png, warning_png_len, TQColor(255, 165, 0), 24, 24));
    m_lblSystemProtected->setText(i18n("Needs Attention"));
    m_lblSystemProtected->setPaletteForegroundColor(TQColor(255, 165, 0));
    m_lblSystemProtectedSub->setText(i18n("Run a scan or database update soon."));
    m_lblSystemProtectedSub->setPaletteForegroundColor(TQColor(136, 136, 136));
  }

  m_lblLastScan->setText(m_stats->getLastScanTime());
  m_lblLastScan->setPaletteForegroundColor(TQColor(136, 136, 136));

  // Update Chart
  TQValueList<TrendDataPoint> trend = m_stats->getTrendData(tf);
  TQStringList categories;
  TQtChartSeries scansSeries;
  scansSeries.label = i18n("Scans");
  scansSeries.color = TQColor(100, 150, 255);
  scansSeries.hasColor = 1;

  TQtChartSeries threatsSeries;
  threatsSeries.label = i18n("Threats");
  threatsSeries.color = TQColor(255, 100, 100);
  threatsSeries.hasColor = 1;

  for (TQValueList<TrendDataPoint>::ConstIterator it = trend.begin();
       it != trend.end(); ++it) {
    categories.append((*it).label);
    scansSeries.values.append((*it).scans);
    threatsSeries.values.append((*it).threats);
  }

  m_chartWidget->setCategories(categories);
  TQtChartSeriesList seriesList;
  seriesList.append(scansSeries);
  seriesList.append(threatsSeries);
  m_chartWidget->setSeries(seriesList);
  m_chartWidget->update();
}

#include "statistics_view.moc"
