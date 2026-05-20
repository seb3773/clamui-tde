#include <ntqapplication.h>
#include <ntqlayout.h>
#include <ntqsplitter.h>
#include <ntqtabwidget.h>
#include <ntqwidget.h>

#include "../tqtbarchartwidget.h"
#include "../tqtpiecechartwidget.h"
#include "../tqtstackedbarchartwidget.h"
#include "../tqtmosaicplotwidget.h"
#include "../tqtwafflechartwidget.h"
#include "../tqtscatterplotwidget.h"
#include "../tqtheatmapwidget.h"

static TQtChartItem makeItem(const char* label, double value)
{
    TQtChartItem it;
    it.label = label;
    it.value = value;
    it.hasColor = 0;
    return it;
}

static TQtChartPointSeries makePointSeries(const char* label, int r, int g, int b)
{
    TQtChartPointSeries s;
    s.label = label;
    s.hasColor = 1;
    s.color = TQColor(r, g, b);
    return s;
}

static TQtChartSeries makeSeries(const char* label, double v0, double v1, double v2, double v3)
{
    TQtChartSeries s;
    s.label = label;
    s.hasColor = 0;
    s.values.append(v0);
    s.values.append(v1);
    s.values.append(v2);
    s.values.append(v3);
    return s;
}

static TQtChartItem makeItemC(const char* label, double value, int r, int g, int b)
{
    TQtChartItem it;
    it.label = label;
    it.value = value;
    it.hasColor = 1;
    it.color = TQColor(r, g, b);
    return it;
}

int main(int argc, char** argv)
{
    TQApplication app(argc, argv);

    TQWidget* w = new TQWidget(0);
    TQVBoxLayout* lay = new TQVBoxLayout(w);
    lay->setMargin(8);
    lay->setSpacing(8);

    TQTabWidget* tabs = new TQTabWidget(w);
    lay->addWidget(tabs, 1);

    TQtChartItemList items;
    items.append(makeItem("A", 12.0));
    items.append(makeItem("B", 25.0));
    items.append(makeItem("C", 7.0));
    items.append(makeItem("D", 18.0));

    TQStringList cats;
    cats.append("A");
    cats.append("B");
    cats.append("C");
    cats.append("D");

    TQtChartSeriesList series;
    {
        TQtChartSeries s0 = makeSeries("Red", 6.0, 15.0, 4.0, 10.0);
        s0.hasColor = 1;
        s0.color = TQColor(220, 68, 55);
        series.append(s0);

        TQtChartSeries s1 = makeSeries("Blue", 6.0, 10.0, 3.0, 8.0);
        s1.hasColor = 1;
        s1.color = TQColor(54, 162, 235);
        series.append(s1);
    }

    {
        TQWidget* page = new TQWidget(tabs);
        TQVBoxLayout* v = new TQVBoxLayout(page);
        v->setMargin(0);
        v->setSpacing(8);

        TQSplitter* splitV = new TQSplitter(TQt::Vertical, page);
        TQSplitter* splitTop = new TQSplitter(TQt::Horizontal, splitV);
        TQSplitter* splitBottom = new TQSplitter(TQt::Horizontal, splitV);

        TQtPieChartWidget* pie2d = new TQtPieChartWidget(splitTop);
        pie2d->setItems(items);
        pie2d->setRender2_5D(0);
        pie2d->setThickness(12);
        pie2d->setShowLegend(1);
        pie2d->setShowPercentLabels(1);

        TQtPieChartWidget* pie25d = new TQtPieChartWidget(splitTop);
        pie25d->setItems(items);
        pie25d->setRender2_5D(1);
        pie25d->setThickness(12);
        pie25d->setShowLegend(1);
        pie25d->setShowPercentLabels(1);

        TQtBarChartWidget* bar2d = new TQtBarChartWidget(splitBottom);
        bar2d->setItems(items);
        bar2d->setRender2_5D(0);
        bar2d->setThickness(10);
        bar2d->setShowLegend(1);
        bar2d->setShowValueLabels(1);

        TQtBarChartWidget* bar25d = new TQtBarChartWidget(splitBottom);
        bar25d->setItems(items);
        bar25d->setRender2_5D(1);
        bar25d->setThickness(10);
        bar25d->setShowLegend(1);
        bar25d->setShowValueLabels(1);

        v->addWidget(splitV, 1);
        tabs->addTab(page, "Basic");
    }

    {
        TQWidget* page = new TQWidget(tabs);
        TQVBoxLayout* v = new TQVBoxLayout(page);
        v->setMargin(0);
        v->setSpacing(8);

        TQSplitter* splitV = new TQSplitter(TQt::Vertical, page);
        TQSplitter* splitTop = new TQSplitter(TQt::Horizontal, splitV);
        TQSplitter* splitBottom = new TQSplitter(TQt::Horizontal, splitV);

        TQtPieChartWidget* donut2d = new TQtPieChartWidget(splitTop);
        donut2d->setItems(items);
        donut2d->setRender2_5D(0);
        donut2d->setThickness(12);
        donut2d->setShowLegend(1);
        donut2d->setShowPercentLabels(1);
        donut2d->setInnerRadiusPermille(550);

        TQtPieChartWidget* donut25d = new TQtPieChartWidget(splitTop);
        donut25d->setItems(items);
        donut25d->setRender2_5D(1);
        donut25d->setThickness(12);
        donut25d->setShowLegend(1);
        donut25d->setShowPercentLabels(1);
        donut25d->setInnerRadiusPermille(550);

        TQSplitter* splitBL = new TQSplitter(TQt::Vertical, splitBottom);
        TQSplitter* splitBR = new TQSplitter(TQt::Vertical, splitBottom);

        TQtStackedBarChartWidget* v2d = new TQtStackedBarChartWidget(splitBL);
        v2d->setCategories(cats);
        v2d->setSeries(series);
        v2d->setOrientation(TQt::Vertical);
        v2d->setRender2_5D(0);
        v2d->setNormalizeTo100(0);
        v2d->setShowLegend(1);

        TQtStackedBarChartWidget* v25d = new TQtStackedBarChartWidget(splitBL);
        v25d->setCategories(cats);
        v25d->setSeries(series);
        v25d->setOrientation(TQt::Vertical);
        v25d->setRender2_5D(1);
        v25d->setThickness(10);
        v25d->setNormalizeTo100(1);
        v25d->setShowLegend(1);

        TQtStackedBarChartWidget* h2d = new TQtStackedBarChartWidget(splitBR);
        h2d->setCategories(cats);
        h2d->setSeries(series);
        h2d->setOrientation(TQt::Horizontal);
        h2d->setRender2_5D(0);
        h2d->setNormalizeTo100(0);
        h2d->setShowLegend(1);

        TQtStackedBarChartWidget* h25d = new TQtStackedBarChartWidget(splitBR);
        h25d->setCategories(cats);
        h25d->setSeries(series);
        h25d->setOrientation(TQt::Horizontal);
        h25d->setRender2_5D(1);
        h25d->setThickness(10);
        h25d->setNormalizeTo100(1);
        h25d->setShowLegend(1);

        v->addWidget(splitV, 1);
        tabs->addTab(page, "Parts-to-Whole");
    }

    {
        TQWidget* page = new TQWidget(tabs);
        TQVBoxLayout* v = new TQVBoxLayout(page);
        v->setMargin(0);
        v->setSpacing(8);

        TQSplitter* split = new TQSplitter(TQt::Horizontal, page);

        TQtChartGroupList groups;
        {
            TQtChartGroup g0;
            g0.label = "Group 1";
            g0.items.append(makeItemC("Alpha", 30.0, 220, 68, 55));
            g0.items.append(makeItemC("Beta", 15.0, 54, 162, 235));
            g0.items.append(makeItemC("Gamma", 10.0, 255, 206, 86));
            groups.append(g0);

            TQtChartGroup g1;
            g1.label = "Group 2";
            g1.items.append(makeItemC("Alpha", 10.0, 220, 68, 55));
            g1.items.append(makeItemC("Beta", 30.0, 54, 162, 235));
            g1.items.append(makeItemC("Gamma", 20.0, 255, 206, 86));
            groups.append(g1);
        }

        TQtMosaicPlotWidget* mosaic = new TQtMosaicPlotWidget(split);
        mosaic->setGroups(groups);
        mosaic->setShowLegend(1);

        TQtWaffleChartWidget* waffle = new TQtWaffleChartWidget(split);
        waffle->setItems(items);
        waffle->setGridSize(10, 10);
        waffle->setShowLegend(1);

        v->addWidget(split, 1);
        tabs->addTab(page, "Mosaic / Waffle");
    }

    {
        TQWidget* page = new TQWidget(tabs);
        TQVBoxLayout* v = new TQVBoxLayout(page);
        v->setMargin(0);
        v->setSpacing(8);

        TQSplitter* split = new TQSplitter(TQt::Horizontal, page);

        TQtChartPointSeriesList pts;
        {
            TQtChartPointSeries s0 = makePointSeries("A", 220, 68, 55);
            s0.points.append(TQtChartPoint(1.0, 2.0));
            s0.points.append(TQtChartPoint(2.0, 3.5));
            s0.points.append(TQtChartPoint(3.0, 2.2));
            s0.points.append(TQtChartPoint(4.0, 4.0));
            pts.append(s0);

            TQtChartPointSeries s1 = makePointSeries("B", 54, 162, 235);
            s1.points.append(TQtChartPoint(1.2, 1.0));
            s1.points.append(TQtChartPoint(2.3, 1.8));
            s1.points.append(TQtChartPoint(3.8, 1.6));
            s1.points.append(TQtChartPoint(4.5, 2.0));
            pts.append(s1);

            TQtChartPointSeries s2 = makePointSeries("C", 255, 206, 86);
            s2.points.append(TQtChartPoint(0.8, 3.2));
            s2.points.append(TQtChartPoint(1.6, 4.2));
            s2.points.append(TQtChartPoint(2.6, 3.9));
            s2.points.append(TQtChartPoint(3.4, 4.6));
            pts.append(s2);
        }

        TQtScatterPlotWidget* scatter = new TQtScatterPlotWidget(split);
        scatter->setSeries(pts);
        scatter->setShowLegend(1);
        scatter->setPointRadius(5);

        TQtHeatmapData hm;
        hm.rows = 6;
        hm.cols = 8;
        {
            for (int r = 0; r < hm.rows; ++r) {
                for (int c = 0; c < hm.cols; ++c) {
                    const double v0 = (double)(r * (hm.cols - 1) + c);
                    const double v1 = (double)((r + 1) * (c + 2));
                    hm.values.append(v0 + 0.3 * v1);
                }
            }
        }

        TQtHeatmapWidget* heat = new TQtHeatmapWidget(split);
        heat->setData(hm);
        heat->setShowCellValues(0);

        v->addWidget(split, 1);
        tabs->addTab(page, "Scatter / Heatmap");
    }

    w->resize(1024, 740);
    w->show();

    app.setMainWidget(w);

    return app.exec();
}
