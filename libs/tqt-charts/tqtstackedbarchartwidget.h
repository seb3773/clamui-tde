#ifndef TQTSTACKEDBARCHARTWIDGET_H
#define TQTSTACKEDBARCHARTWIDGET_H

#include "tqtabstractchartwidget.h"

#include <ntqstringlist.h>

class TQtStackedBarChartWidget : public TQtAbstractChartWidget {
    TQ_OBJECT
public:
    TQtStackedBarChartWidget(TQWidget* parent = 0);
    ~TQtStackedBarChartWidget();

    void setCategories(const TQStringList& cats);
    const TQStringList& categories() const;

    void setSeries(const TQtChartSeriesList& series);
    const TQtChartSeriesList& series() const;

    void clearSeries();

    void setOrientation(TQt::Orientation o);
    TQt::Orientation orientation() const;

    void setNormalizeTo100(int on);
    int normalizeTo100() const;

    void setShowValueLabels(int on);
    int showValueLabels() const;

    void setShowPercentLabels(int on);
    int showPercentLabels() const;

    void setCategorySpacing(int px);
    int categorySpacing() const;

    void setThickness(int px);
    int thickness() const;

protected:
    void paintEvent(TQPaintEvent* ev);

private:
    void paintLegend_(TQPainter& p, const TQRect& r);
    TQColor seriesColor_(int idx) const;

private:
    TQStringList m_categories;
    TQtChartSeriesList m_series;

    TQt::Orientation m_orient;
    int m_normalizeTo100;
    int m_showValueLabels;
    int m_showPercentLabels;

    int m_categorySpacing;
    int m_thickness;
};

#endif
