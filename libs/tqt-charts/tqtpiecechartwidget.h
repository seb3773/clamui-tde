#ifndef TQTPIECHARTWIDGET_H
#define TQTPIECHARTWIDGET_H

#include "tqtabstractchartwidget.h"

class TQtPieChartWidget : public TQtAbstractChartWidget {
    TQ_OBJECT
public:
    TQtPieChartWidget(TQWidget* parent = 0);
    ~TQtPieChartWidget();

    void setShowPercentLabels(int on);
    int showPercentLabels() const;

    void setShowValueLabels(int on);
    int showValueLabels() const;

    void setThickness(int px);
    int thickness() const;

    void setInnerRadiusPermille(int permille);
    int innerRadiusPermille() const;

protected:
    void paintEvent(TQPaintEvent* ev);

private:
    double total_() const;
    void paintLegend_(TQPainter& p, const TQRect& r);

private:
    int m_showPercentLabels;
    int m_showValueLabels;
    int m_thickness;
    int m_innerRadiusPermille;
};

#endif
