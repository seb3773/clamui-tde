#ifndef TQTSCATTERPLOTWIDGET_H
#define TQTSCATTERPLOTWIDGET_H

#include "tqtabstractchartwidget.h"

class TQtScatterPlotWidget : public TQtAbstractChartWidget {
    TQ_OBJECT
public:
    TQtScatterPlotWidget(TQWidget* parent = 0);
    ~TQtScatterPlotWidget();

    void setSeries(const TQtChartPointSeriesList& series);
    const TQtChartPointSeriesList& series() const;

    void setPointRadius(int px);
    int pointRadius() const;

protected:
    void paintEvent(TQPaintEvent* ev);

private:
    void paintLegend_(TQPainter& p, const TQRect& r);
    void computeBounds_(double& minX, double& maxX, double& minY, double& maxY) const;

private:
    TQtChartPointSeriesList m_series;
    int m_pointRadius;
};

#endif
