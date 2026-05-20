#ifndef ABOUT_DIALOG_H
#define ABOUT_DIALOG_H

#include <tqdialog.h>
#include <tqpixmap.h>

class AboutDialog : public TQDialog {
    TQ_OBJECT

public:
    AboutDialog(TQWidget *parent = 0);
    ~AboutDialog();

private:
    TQPixmap m_dragonPixmap;
};

#endif // ABOUT_DIALOG_H
