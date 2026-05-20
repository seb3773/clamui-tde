/*
 * tray.cpp - System tray integration implementation
 *
 * Flicker-free animation via paintEvent double-buffering.
 * Tooltip updated lazily on enterEvent (not on every progress callback).
 */

#include "tray.h"
#include "ui/mainwindow.h"

#include <kiconloader.h>
#include <tdepopupmenu.h>
#include <tdeaction.h>
#include <tdelocale.h>
#include <tdeapplication.h>
#include <tdeglobal.h>
#include <ntqtooltip.h>
#include <ntqtimer.h>
#include <ntqpainter.h>
#include "ui/icon_utils.h"

ClamTray::ClamTray(ClamMainWindow *parent)
    : KSystemTray(parent, "ClamTray"), m_mainWindow(parent),
      m_currentStatus(0), m_scanPercent(-1), m_iconSize(0)
{
    m_animTimer = new TQTimer(this);
    connect(m_animTimer, TQT_SIGNAL(timeout()), this, TQT_SLOT(slotUpdateAnim()));
    m_animFrame = 0;
    m_animDirection = 1;
    
    // Load at default small size; resizeEvent will reload at correct size
    loadIconsAtSize(22);
    
    // Set initial icon
    m_currentPixmap = m_idleIcon;
    setPixmap(m_currentPixmap);
    TQToolTip::add(this, i18n("TDE ClamUI - Ready"));
    
    // Default action (left click)
    connect(this, TQT_SIGNAL(quitSelected()), this, TQT_SLOT(slotQuit()));
}

ClamTray::~ClamTray()
{
}

void ClamTray::loadIconsAtSize(int sz)
{
    if (sz == m_iconSize) return; // Already loaded at this size
    m_iconSize = sz;
    
    m_idleIcon = IconUtils::load(main_icon_png, main_icon_png_len, sz, sz);
    m_threatIcon = IconUtils::colorize(warning_png, warning_png_len, TQt::red, sz, sz);
    m_updateIcon = IconUtils::load(refresh_png, refresh_png_len, sz, sz);
    
    m_animFrames[0] = IconUtils::load(scan_icon1_png, scan_icon1_png_len, sz, sz);
    m_animFrames[1] = IconUtils::load(scan_icon2_png, scan_icon2_png_len, sz, sz);
    m_animFrames[2] = IconUtils::load(scan_icon3_png, scan_icon3_png_len, sz, sz);
    m_animFrames[3] = IconUtils::load(scan_icon4_png, scan_icon4_png_len, sz, sz);
    m_animFrames[4] = IconUtils::load(scan_icon5_png, scan_icon5_png_len, sz, sz);
    
    // Update the current displayed pixmap
    switch (m_currentStatus) {
        case 0: m_currentPixmap = m_idleIcon; break;
        case 1: m_currentPixmap = m_animFrames[m_animFrame]; break;
        case 2: m_currentPixmap = m_threatIcon; break;
        case 3: m_currentPixmap = m_updateIcon; break;
    }
}

void ClamTray::reloadIcons()
{
    m_iconSize = 0; // Force reload
    int sz = TQMIN(width(), height());
    if (sz < 8) sz = 22;
    loadIconsAtSize(sz);
    repaint(false);
}

void ClamTray::resizeEvent(TQResizeEvent *event)
{
    KSystemTray::resizeEvent(event);
    int sz = TQMIN(event->size().width(), event->size().height());
    if (sz > 0 && sz != m_iconSize) {
        loadIconsAtSize(sz);
        repaint(false);
    }
}

void ClamTray::setStatus(int status)
{
    m_currentStatus = status;
    
    if (status != 1 && m_animTimer->isActive()) {
        m_animTimer->stop();
    }

    switch(status) {
        case 0: 
            m_currentPixmap = m_idleIcon;
            m_scanPercent = -1;
            break;
        case 1: 
            m_scanPercent = -1;
            if (!m_animTimer->isActive()) {
                m_animFrame = 0;
                m_animDirection = 1;
                m_currentPixmap = m_animFrames[0];
                m_animTimer->start(400);
            }
            break;
        case 2: 
            m_currentPixmap = m_threatIcon;
            m_scanPercent = -1;
            break;
        case 3: 
            m_currentPixmap = m_updateIcon;
            m_scanPercent = -1;
            break;
    }
    
    // Update pixmap via parent class for XEMBED, then schedule
    // a flicker-free repaint
    setPixmap(m_currentPixmap);
}

void ClamTray::slotUpdateAnim()
{
    m_animFrame += m_animDirection;
    
    if (m_animFrame >= 4) {
        m_animFrame = 4;
        m_animDirection = -1;
    } else if (m_animFrame <= 0) {
        m_animFrame = 0;
        m_animDirection = 1;
    }
    
    m_currentPixmap = m_animFrames[m_animFrame];
    // Use repaint(false) to avoid erase-then-paint flicker
    repaint(false);
}

void ClamTray::paintEvent(TQPaintEvent *)
{
    // Double-buffered paint: draw pixmap scaled to widget size, no erase
    if (!m_currentPixmap.isNull()) {
        TQPainter p(this);
        int w = width();
        int h = height();
        if (m_currentPixmap.width() == w && m_currentPixmap.height() == h) {
            p.drawPixmap(0, 0, m_currentPixmap);
        } else {
            // Scale to fill (e.g. icons loaded before resize)
            TQImage scaled = m_currentPixmap.convertToImage().smoothScale(w, h);
            p.drawPixmap(0, 0, TQPixmap(scaled));
        }
    }
}

void ClamTray::updateScanProgress(int percent)
{
    // Just store the value — tooltip is updated lazily in enterEvent
    m_scanPercent = percent;
}

void ClamTray::enterEvent(TQEvent *event)
{
    // Update tooltip text on mouse enter (lazy, avoids flicker)
    TQToolTip::remove(this);
    
    switch (m_currentStatus) {
        case 0:
            TQToolTip::add(this, i18n("TDE ClamUI - Ready"));
            break;
        case 1:
            if (m_scanPercent >= 0) {
                TQToolTip::add(this, i18n("TDE ClamUI - Scanning... (%1%)").arg(m_scanPercent));
            } else {
                TQToolTip::add(this, i18n("TDE ClamUI - Scanning..."));
            }
            break;
        case 2:
            TQToolTip::add(this, i18n("TDE ClamUI - Threats Found!"));
            break;
        case 3:
            TQToolTip::add(this, i18n("TDE ClamUI - Updating Database..."));
            break;
    }
    
    KSystemTray::enterEvent(event);
}

void ClamTray::contextMenuAboutToShow(TDEPopupMenu *menu)
{
    // Clear default items added by KSystemTray except quit
    menu->clear();
    
    TQString toggleText = m_mainWindow->isVisible() ? i18n("Hide") : i18n("Show");
    menu->insertItem(toggleText, this, TQT_SLOT(slotShowHide()));
    
    menu->insertSeparator();
    
    int idQuickScan = menu->insertItem(IconUtils::load(start_png, start_png_len, 16, 16), i18n("System Scan"), 
                     this, TQT_SLOT(slotQuickScan()));
    int idUpdate = menu->insertItem(IconUtils::load(refresh_png, refresh_png_len, 16, 16), i18n("Update Database"), 
                     this, TQT_SLOT(slotUpdateDatabase()));
                     
    if (m_mainWindow->isScanRunning()) {
        menu->setItemEnabled(idQuickScan, false);
        menu->setItemEnabled(idUpdate, false);
    }
                     
    menu->insertSeparator();
    
    menu->insertItem(IconUtils::load(quit_png, quit_png_len, 16, 16), i18n("Quit"), 
                     this, TQT_SLOT(slotQuit()));
}

void ClamTray::slotShowHide()
{
    if (m_mainWindow->isVisible()) {
        m_mainWindow->hide();
    } else {
        m_mainWindow->userShowedWindow();
        m_mainWindow->show();
    }
}

void ClamTray::slotQuickScan()
{
    m_mainWindow->userShowedWindow();
    m_mainWindow->show();
    m_mainWindow->slotQuickScan();
}

void ClamTray::slotUpdateDatabase()
{
    m_mainWindow->userShowedWindow();
    m_mainWindow->show();
    m_mainWindow->slotUpdateDatabase();
}

void ClamTray::slotQuit()
{
    m_mainWindow->slotQuitApp();
}

#include "tray.moc"
