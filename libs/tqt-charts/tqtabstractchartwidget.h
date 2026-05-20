#ifndef TQTABSTRACTCHARTWIDGET_H
#define TQTABSTRACTCHARTWIDGET_H

#include "tqtcharts_common.h"

#include <ntqfont.h>
#include <ntqwidget.h>

class TQtAbstractChartWidget : public TQWidget {
    TQ_OBJECT
public:
    TQtAbstractChartWidget(TQWidget* parent = 0);
    ~TQtAbstractChartWidget();

    void setItems(const TQtChartItemList& items);
    const TQtChartItemList& items() const;

    void clear();

    void setPaletteColors(const TQtChartPalette& pal);
    const TQtChartPalette& paletteColors() const;

    void setShowLegend(int on);
    int showLegend() const;

    void setRender2_5D(int on);
    int render2_5D() const;

    void setLegendFont(const TQFont& f);
    TQFont legendFont() const;

protected:
    TQColor colorForIndex_(int idx) const;

protected:
    TQtChartItemList m_items;
    TQtChartPalette m_palette;

    int m_showLegend;
    int m_render2_5D;

    TQFont m_legendFont;
};

#endif
