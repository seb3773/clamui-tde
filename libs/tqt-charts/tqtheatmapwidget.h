#ifndef TQTHEATMAPWIDGET_H
#define TQTHEATMAPWIDGET_H

#include "tqtabstractchartwidget.h"

class TQtHeatmapWidget : public TQtAbstractChartWidget {
    TQ_OBJECT
public:
    TQtHeatmapWidget(TQWidget* parent = 0);
    ~TQtHeatmapWidget();

    void setData(const TQtHeatmapData& d);
    const TQtHeatmapData& data() const;

    void setShowCellValues(int on);
    int showCellValues() const;

protected:
    void paintEvent(TQPaintEvent* ev);

private:
    static TQColor heatColor_(double t);

private:
    TQtHeatmapData m_data;
    int m_showCellValues;
};

#endif
