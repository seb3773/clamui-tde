/*
 * quarantine_view.cpp - Quarantine View
 */

#include "quarantine_view.h"
#include "../core/quarantine_manager.h"

#include <tqlayout.h>
#include <tqlabel.h>
#include <tqpushbutton.h>
#include <tqlistview.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <tqdatetime.h>
#include "icon_utils.h"

#include <tqpainter.h>
#include <tqfont.h>

class QuarantineListViewItem : public TQListViewItem {
public:
    QuarantineListViewItem(TQListView *parent, const TQString &path, const TQString &threat, const TQString &date, int id)
        : TQListViewItem(parent, path, threat, date), m_id(id) {}
    
    int entryId() const { return m_id; }
    
    void paintCell(TQPainter *p, const TQColorGroup &cg, int column, int width, int align) override {
        if (column == 1) { // Threat Detected column
            TQColorGroup customCg(cg);
            customCg.setColor(TQColorGroup::Text, TQt::red);
            
            TQFont f = p->font();
            f.setBold(true);
            p->setFont(f);
            
            TQListViewItem::paintCell(p, customCg, column, width, align);
        } else {
            TQListViewItem::paintCell(p, cg, column, width, align);
        }
    }
private:
    int m_id;
};

QuarantineView::QuarantineView(TQWidget *parent, TQWidgetStack *stack, QuarantineManager *quarantineManager)
    : TQWidget(parent), m_quarantine(quarantineManager)
{
    if (m_quarantine) {
        connect(m_quarantine, TQT_SIGNAL(quarantineUpdated()), this, TQT_SLOT(refreshList()));
        connect(m_quarantine, TQT_SIGNAL(error(const TQString &)), this, TQT_SLOT(refreshList())); // In a real app we'd show the error
    }
    
    setupUI();
    refreshList();
}

QuarantineView::~QuarantineView()
{
}

void QuarantineView::setupUI()
{
    TQVBoxLayout *mainLayout = new TQVBoxLayout(this, 11, 6);
    
    TQLabel *titleLabel = new TQLabel(i18n("<h2>Quarantine</h2>"), this);
    mainLayout->addWidget(titleLabel);
    
    TQLabel *descLabel = new TQLabel(i18n("Files safely isolated from the system. You can restore false positives or delete files permanently."), this);
    descLabel->setAlignment(TQt::WordBreak | TQt::AlignLeft);
    mainLayout->addWidget(descLabel);
    
    TQFrame *line = new TQFrame(this);
    line->setFrameStyle(TQFrame::HLine | TQFrame::Sunken);
    mainLayout->addWidget(line);
    
    TQHBoxLayout *statsLayout = new TQHBoxLayout(mainLayout, 6);
    m_lblTotalItems = new TQLabel(i18n("<b>Total Items:</b> 0"), this);
    
    TQHBoxLayout *sizeLayout = new TQHBoxLayout(statsLayout, 4);
    m_sizeIcon = new TQLabel(this);
    m_sizeIcon->setPixmap(IconUtils::load(drive_png, drive_png_len, 16, 16));
    m_lblTotalSize = new TQLabel(i18n("<b>Total Size:</b> 0 B"), this);
    
    statsLayout->addWidget(m_lblTotalItems);
    statsLayout->addSpacing(20);
    sizeLayout->addWidget(m_sizeIcon);
    sizeLayout->addWidget(m_lblTotalSize);
    
    statsLayout->addStretch(1);
    
    m_list = new TQListView(this);
    m_list->addColumn(i18n("Original Path"));
    m_list->addColumn(i18n("Threat Detected"));
    m_list->addColumn(i18n("Date"));
    m_list->setAllColumnsShowFocus(true);
    connect(m_list, TQT_SIGNAL(selectionChanged()), this, TQT_SLOT(selectionChanged()));
    mainLayout->addWidget(m_list);
    
    TQHBoxLayout *btnLayout = new TQHBoxLayout(mainLayout, 6);
    
    m_btnClearOld = new TQPushButton(i18n("Clear > 30 Days"), this);
    connect(m_btnClearOld, TQT_SIGNAL(clicked()), this, TQT_SLOT(clearOld()));
    btnLayout->addWidget(m_btnClearOld);
    
    btnLayout->addStretch(1);
    
    m_btnRestore = new TQPushButton(i18n("&Restore"), this);
    m_btnRestore->setEnabled(false);
    connect(m_btnRestore, TQT_SIGNAL(clicked()), this, TQT_SLOT(restoreSelected()));
    btnLayout->addWidget(m_btnRestore);
    
    m_btnDelete = new TQPushButton(i18n("&Delete Permanently"), this);
    m_btnDelete->setEnabled(false);
    connect(m_btnDelete, TQT_SIGNAL(clicked()), this, TQT_SLOT(deleteSelected()));
    btnLayout->addWidget(m_btnDelete);
}

void QuarantineView::reloadIcons()
{
    if (m_sizeIcon) m_sizeIcon->setPixmap(IconUtils::load(drive_png, drive_png_len, 16, 16));
}

void QuarantineView::refreshList()
{
    m_list->clear();
    
    TQValueList<QuarantineEntry> entries = m_quarantine->getEntries();
    TQ_LONG totalSizeBytes = 0;
    for (TQValueList<QuarantineEntry>::ConstIterator it = entries.begin(); it != entries.end(); ++it) {
        totalSizeBytes += (*it).fileSize;
        new QuarantineListViewItem(m_list,
            (*it).originalPath,
            (*it).threatName,
            (*it).detectionDate.toString(),
            (*it).id
        );
    }
    
    m_lblTotalItems->setText(i18n("<b>Total Items:</b> %1").arg(entries.count()));
    
    TQString sizeStr;
    if (totalSizeBytes < 1024) sizeStr = TQString("%1 B").arg(totalSizeBytes);
    else if (totalSizeBytes < 1024 * 1024) sizeStr = TQString("%1 KB").arg(totalSizeBytes / 1024.0, 0, 'f', 1);
    else if (totalSizeBytes < 1024 * 1024 * 1024) sizeStr = TQString("%1 MB").arg(totalSizeBytes / (1024.0 * 1024.0), 0, 'f', 1);
    else sizeStr = TQString("%1 GB").arg(totalSizeBytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    
    m_lblTotalSize->setText(i18n("<b>Total Size:</b> %1").arg(sizeStr));
    
    selectionChanged();
}

void QuarantineView::selectionChanged()
{
    bool hasSelection = m_list->selectedItem() != 0;
    m_btnRestore->setEnabled(hasSelection);
    m_btnDelete->setEnabled(hasSelection);
}

void QuarantineView::restoreSelected()
{
    QuarantineListViewItem *item = static_cast<QuarantineListViewItem*>(m_list->selectedItem());
    if (!item) return;
    
    int id = item->entryId();
    
    if (KMessageBox::warningContinueCancel(this,
        i18n("Are you sure you want to restore this file to its original location? If it is a real virus, it may infect your system."),
        i18n("Restore File"),
        KGuiItem(i18n("Restore"))) == KMessageBox::Continue)
    {
        if (!m_quarantine->restoreFile(id)) {
            KMessageBox::error(this, i18n("Failed to restore file."));
        }
    }
}

void QuarantineView::deleteSelected()
{
    QuarantineListViewItem *item = static_cast<QuarantineListViewItem*>(m_list->selectedItem());
    if (!item) return;
    
    int id = item->entryId();
    
    if (KMessageBox::warningContinueCancel(this,
        i18n("Are you sure you want to permanently delete this file? This cannot be undone."),
        i18n("Delete Permanently"),
        KStdGuiItem::del()) == KMessageBox::Continue)
    {
        if (!m_quarantine->deleteFile(id)) {
            KMessageBox::error(this, i18n("Failed to delete file."));
        }
    }
}

void QuarantineView::clearOld()
{
    if (KMessageBox::warningContinueCancel(this,
        i18n("Remove all quarantined files older than 30 days?"),
        i18n("Clear Old Files"),
        KStdGuiItem::del()) == KMessageBox::Continue)
    {
        m_quarantine->clearOldEntries(30);
    }
}

#include "quarantine_view.moc"
