/*
 * sidebar.h - Navigation sidebar
 *
 * Implements a TQListBox with custom styling to mimic the Adw.Leaflet
 * sidebar navigation.
 */

#ifndef CLAM_SIDEBAR_H
#define CLAM_SIDEBAR_H

#include <tqlistbox.h>

class ClamSidebar : public TQListBox
{
    TQ_OBJECT

public:
    ClamSidebar(TQWidget *parent = 0, const char *name = 0);
    ~ClamSidebar();

    int currentIndex() const;
    void setCurrentIndex(int index);
    
    void reloadIcons();

signals:
    void viewSelected(int index);

private slots:
    void slotSelectionChanged();

private:
    void setupItems();
    
    int m_currentIndex;
};

#endif /* CLAM_SIDEBAR_H */
