#include "about_dialog.h"
#include "../embedded_icons.h"
#include "icon_utils.h"

#include <tqlayout.h>
#include <tqlabel.h>
#include <tqpushbutton.h>
#include <tdelocale.h>

AboutDialog::AboutDialog(TQWidget *parent)
    : TQDialog(parent, "about_dialog", true)
{
    setCaption(i18n("About tdeClamUI"));
    setFixedSize(600, 420);
    
    // Set background to black
    setPaletteBackgroundColor(TQt::black);
    
    m_dragonPixmap = IconUtils::load(about_clamuitde_png, about_clamuitde_png_len);
    
    // Create outer layout
    TQVBoxLayout *outerLayout = new TQVBoxLayout(this, 15, 10);
    
    // Top layout for image and texts
    TQHBoxLayout *topLayout = new TQHBoxLayout(outerLayout);
    
    // Left side: image
    TQLabel *imageLabel = new TQLabel(this);
    imageLabel->setPixmap(m_dragonPixmap);
    imageLabel->setAlignment(TQt::AlignLeft | TQt::AlignVCenter);
    imageLabel->setFixedSize(250, 350);
    topLayout->addWidget(imageLabel);
    
    // Right side: texts
    TQVBoxLayout *rightLayout = new TQVBoxLayout(topLayout);
    rightLayout->addStretch(1);
    
    TQLabel *titleLabel = new TQLabel("tdeClamUI", this);
    TQFont titleFont = titleLabel->font();
    titleFont.setPointSize(32);
    titleFont.setBold(true);
    titleFont.setItalic(true);
    titleLabel->setFont(titleFont);
    titleLabel->setPaletteForegroundColor(TQt::white);
    titleLabel->setAlignment(TQt::AlignHCenter | TQt::AlignVCenter);
    rightLayout->addWidget(titleLabel);
    
    rightLayout->addSpacing(25);
    
    TQLabel *descLabel = new TQLabel("A trinity DE graphical interface\nfor clamAV", this);
    TQFont descFont = descLabel->font();
    descFont.setPointSize(18);
    descFont.setBold(true);
    descLabel->setFont(descFont);
    descLabel->setPaletteForegroundColor(TQt::white);
    descLabel->setAlignment(TQt::AlignHCenter | TQt::AlignVCenter);
    rightLayout->addWidget(descLabel);
    
    rightLayout->addSpacing(25);
    
    TQLabel *basedLabel = new TQLabel("based on ClamUI ( https://github.com/linx-systems/clamui )", this);
    TQFont basedFont = basedLabel->font();
    basedFont.setPointSize(10);
    basedFont.setItalic(true);
    basedLabel->setFont(basedFont);
    basedLabel->setPaletteForegroundColor(TQt::white);
    basedLabel->setAlignment(TQt::AlignRight | TQt::AlignVCenter);
    rightLayout->addWidget(basedLabel);
    
    rightLayout->addStretch(1);
    
    TQLabel *authorLabel = new TQLabel("by seb3773", this);
    TQFont authorFont = authorLabel->font();
    authorFont.setPointSize(14);
    authorFont.setBold(true);
    authorFont.setItalic(true);
    authorLabel->setFont(authorFont);
    authorLabel->setPaletteForegroundColor(TQt::white);
    authorLabel->setAlignment(TQt::AlignRight | TQt::AlignVCenter);
    rightLayout->addWidget(authorLabel);
    
    outerLayout->addSpacing(15);
    
    TQHBoxLayout *bottomLayout = new TQHBoxLayout(outerLayout);
    bottomLayout->addStretch(1);
    TQPushButton *closeBtn = new TQPushButton(i18n("Close"), this);
    connect(closeBtn, TQT_SIGNAL(clicked()), this, TQT_SLOT(accept()));
    bottomLayout->addWidget(closeBtn);
    bottomLayout->addStretch(1);
}

AboutDialog::~AboutDialog() {}
#include "about_dialog.moc"
