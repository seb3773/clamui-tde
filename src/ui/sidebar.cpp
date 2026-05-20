/*
 * sidebar.cpp - Navigation sidebar implementation
 */

#include "sidebar.h"
#include <kiconloader.h>
#include <tdelocale.h>
#include <tdeglobal.h>
#include "icon_utils.h"
#include <tqpainter.h>

class ClamSidebarItem : public TQListBoxItem {
    TQPixmap m_pix;
    TQString m_text;
public:
    ClamSidebarItem(TQListBox *lb, const TQPixmap &pix, const TQString &text)
        : TQListBoxItem(lb), m_pix(pix), m_text(text) {}

    int height(const TQListBox *lb) const {
        return TQMAX(m_pix.height(), lb->fontMetrics().lineSpacing()) + 16; 
    }
    
    int width(const TQListBox *lb) const {
        return m_pix.width() + 10 + lb->fontMetrics().width(m_text) + 10;
    }
    
    void paint(TQPainter *p) {
        int h = height(listBox());
        p->drawPixmap(6, (h - m_pix.height()) / 2, m_pix);
        p->drawText(m_pix.width() + 14, 0, width(listBox()), h,
                    TQt::AlignVCenter | TQt::AlignLeft, m_text);
    }
    
    TQString text() const { return m_text; }
    const TQPixmap* pixmap() const { return &m_pix; }
};

ClamSidebar::ClamSidebar(TQWidget *parent, const char *name)
    : TQListBox(parent, name), m_currentIndex(-1)
{
    setFrameShape(TQFrame::NoFrame);
    setPaletteBackgroundColor(colorGroup().background());
    
    TQFont f = font();
    f.setBold(true);
    setFont(f);
    
    setupItems();

    connect(this, TQT_SIGNAL(selectionChanged()), 
            this, TQT_SLOT(slotSelectionChanged()));
}

ClamSidebar::~ClamSidebar()
{
}

void ClamSidebar::setupItems()
{
    new ClamSidebarItem(this, IconUtils::load(find_png, find_png_len, 32, 32), i18n("Scan"));
    new ClamSidebarItem(this, IconUtils::load(network_server_symbolic_png, network_server_symbolic_png_len, 32, 32), i18n("Database"));
    new ClamSidebarItem(this, IconUtils::load(recent_png, recent_png_len, 32, 32), i18n("Logs"));
    new ClamSidebarItem(this, IconUtils::load(components_png, components_png_len, 32, 32), i18n("Components"));
    new ClamSidebarItem(this, IconUtils::load(quarantine_png, quarantine_png_len, 32, 32), i18n("Quarantine"));
    new ClamSidebarItem(this, IconUtils::load(stats_png, stats_png_len, 32, 32), i18n("Statistics"));
    new ClamSidebarItem(this, IconUtils::load(audit_png, audit_png_len, 32, 32), i18n("System Audit"));
}

void ClamSidebar::reloadIcons()
{
    blockSignals(true);
    int sel = currentItem();
    
    clear();
    setupItems();
    
    if (sel != -1) {
        setCurrentItem(sel);
    }
    blockSignals(false);
}

int ClamSidebar::currentIndex() const
{
    return m_currentIndex;
}

void ClamSidebar::setCurrentIndex(int index)
{
    if (index >= 0 && index < (int)count()) {
        m_currentIndex = index;
        setSelected(index, true);
    }
}

void ClamSidebar::slotSelectionChanged()
{
    int index = currentItem();
    if (index != -1 && index != m_currentIndex) {
        m_currentIndex = index;
        emit viewSelected(m_currentIndex);
    }
}

#include "sidebar.moc"
