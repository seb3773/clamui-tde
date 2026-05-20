#ifndef TQTWAFFLECHARTWIDGET_H
#define TQTWAFFLECHARTWIDGET_H

#include "tqtabstractchartwidget.h"

class TQtWaffleChartWidget : public TQtAbstractChartWidget {
    TQ_OBJECT
public:
    TQtWaffleChartWidget(TQWidget* parent = 0);
    ~TQtWaffleChartWidget();

    void setGridSize(int cols, int rows);
    int gridCols() const;
    int gridRows() const;

protected:
    void paintEvent(TQPaintEvent* ev);

private:
    void paintLegend_(TQPainter& p, const TQRect& r);

private:
    int m_cols;
    int m_rows;
};

#endif
