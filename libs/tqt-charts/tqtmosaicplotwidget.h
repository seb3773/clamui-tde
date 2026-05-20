#ifndef TQTMOSAICPLOTWIDGET_H
#define TQTMOSAICPLOTWIDGET_H

#include "tqtabstractchartwidget.h"

class TQtMosaicPlotWidget : public TQtAbstractChartWidget {
    TQ_OBJECT
public:
    TQtMosaicPlotWidget(TQWidget* parent = 0);
    ~TQtMosaicPlotWidget();

    void setGroups(const TQtChartGroupList& groups);
    const TQtChartGroupList& groups() const;

protected:
    void paintEvent(TQPaintEvent* ev);

private:
    void paintLegend_(TQPainter& p, const TQRect& r);

private:
    TQtChartGroupList m_groups;
};

#endif
