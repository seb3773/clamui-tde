/*
 * tray.h - System tray integration
 *
 * Implements KSystemTray to provide an icon and context menu.
 */

#ifndef CLAM_TRAY_H
#define CLAM_TRAY_H

#include <ksystemtray.h>
#include <ntqpixmap.h>

class ClamMainWindow;
class TDEPopupMenu;
class TQPixmap;
class TQTimer;

class ClamTray : public KSystemTray
{
    TQ_OBJECT

public:
    ClamTray(ClamMainWindow *parent);
    ~ClamTray();

    void setStatus(int status); // 0=idle, 1=scanning, 2=threats, 3=updating
    void updateScanProgress(int percent); // Store scan % for tooltip
    void reloadIcons();

protected:
    void contextMenuAboutToShow(TDEPopupMenu *menu);
    void enterEvent(TQEvent *event);
    void paintEvent(TQPaintEvent *event);
    void resizeEvent(TQResizeEvent *event);

private slots:
    void slotShowHide();
    void slotQuickScan();
    void slotUpdateDatabase();
    void slotQuit();
    void slotUpdateAnim();

private:
    ClamMainWindow *m_mainWindow;
    
    TQPixmap m_idleIcon;
    TQPixmap m_threatIcon;
    TQPixmap m_updateIcon;
    
    TQPixmap m_animFrames[5];
    TQTimer *m_animTimer;
    int m_animFrame;
    int m_animDirection;
    
    int m_currentStatus;
    int m_scanPercent; // -1 = unknown, 0-100 = known
    TQPixmap m_currentPixmap; // For double-buffered painting
    int m_iconSize; // Current loaded icon size
    
    void loadIconsAtSize(int sz);
};

#endif /* CLAM_TRAY_H */
