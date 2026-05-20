#ifndef TQTCHARTS_COMMON_H
#define TQTCHARTS_COMMON_H

#include <ntqcolor.h>
#include <ntqstring.h>
#include <ntqstringlist.h>
#include <ntqvaluelist.h>

struct TQtChartItem {
    TQtChartItem() : value(0.0), hasColor(0) {}

    TQString label;
    double value;
    TQColor color;
    int hasColor;
};

typedef TQValueList<TQtChartItem> TQtChartItemList;

struct TQtChartSeries {
    TQtChartSeries() : hasColor(0) {}

    TQString label;
    TQColor color;
    int hasColor;

    TQValueList<double> values;
};

typedef TQValueList<TQtChartSeries> TQtChartSeriesList;

struct TQtChartGroup {
    TQtChartGroup() {}

    TQString label;
    TQtChartItemList items;
};

typedef TQValueList<TQtChartGroup> TQtChartGroupList;

struct TQtChartPoint {
    TQtChartPoint() : x(0.0), y(0.0) {}
    TQtChartPoint(double xx, double yy) : x(xx), y(yy) {}
    double x;
    double y;
};

typedef TQValueList<TQtChartPoint> TQtChartPointList;

struct TQtChartPointSeries {
    TQtChartPointSeries() : hasColor(0) {}

    TQString label;
    TQColor color;
    int hasColor;

    TQtChartPointList points;
};

typedef TQValueList<TQtChartPointSeries> TQtChartPointSeriesList;

struct TQtHeatmapData {
    TQtHeatmapData() : rows(0), cols(0) {}

    int rows;
    int cols;
    TQValueList<double> values;

    TQStringList rowLabels;
    TQStringList colLabels;
};

typedef TQValueList<TQColor> TQtChartPalette;

#endif
