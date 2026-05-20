#ifndef TQTBARCHARTWIDGET_H
#define TQTBARCHARTWIDGET_H

#include "tqtabstractchartwidget.h"

class TQtBarChartWidget : public TQtAbstractChartWidget {
    TQ_OBJECT
public:
    TQtBarChartWidget(TQWidget* parent = 0);
    ~TQtBarChartWidget();

    void setShowValueLabels(int on);
    int showValueLabels() const;

    void setBarSpacing(int px);
    int barSpacing() const;

    void setThickness(int px);
    int thickness() const;

protected:
    void paintEvent(TQPaintEvent* ev);

private:
    void paintLegend_(TQPainter& p, const TQRect& r);

private:
    int m_showValueLabels;
    int m_barSpacing;
    int m_thickness;
};

#endif
