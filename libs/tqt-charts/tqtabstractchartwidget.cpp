#include "tqtabstractchartwidget.h"

#include <ntqapplication.h>

TQtAbstractChartWidget::TQtAbstractChartWidget(TQWidget* parent)
    : TQWidget(parent),
      m_showLegend(1),
      m_render2_5D(0)
{
    setMouseTracking(true);

    m_legendFont = font();
    m_legendFont.setPointSize(m_legendFont.pointSize() - 1);

    m_palette.clear();
    m_palette.append(TQColor(220, 68, 55));
    m_palette.append(TQColor(54, 162, 235));
    m_palette.append(TQColor(255, 206, 86));
    m_palette.append(TQColor(75, 192, 192));
    m_palette.append(TQColor(153, 102, 255));
    m_palette.append(TQColor(255, 159, 64));
    m_palette.append(TQColor(90, 90, 90));
    m_palette.append(TQColor(0, 160, 0));
}

TQtAbstractChartWidget::~TQtAbstractChartWidget() {}

void TQtAbstractChartWidget::setItems(const TQtChartItemList& items)
{
    m_items = items;
    update();
}

const TQtChartItemList& TQtAbstractChartWidget::items() const
{
    return m_items;
}

void TQtAbstractChartWidget::clear()
{
    m_items.clear();
    update();
}

void TQtAbstractChartWidget::setPaletteColors(const TQtChartPalette& pal)
{
    m_palette = pal;
    update();
}

const TQtChartPalette& TQtAbstractChartWidget::paletteColors() const
{
    return m_palette;
}

void TQtAbstractChartWidget::setShowLegend(int on)
{
    m_showLegend = on ? 1 : 0;
    update();
}

int TQtAbstractChartWidget::showLegend() const
{
    return m_showLegend;
}

void TQtAbstractChartWidget::setRender2_5D(int on)
{
    m_render2_5D = on ? 1 : 0;
    update();
}

int TQtAbstractChartWidget::render2_5D() const
{
    return m_render2_5D;
}

void TQtAbstractChartWidget::setLegendFont(const TQFont& f)
{
    m_legendFont = f;
    update();
}

TQFont TQtAbstractChartWidget::legendFont() const
{
    return m_legendFont;
}

TQColor TQtAbstractChartWidget::colorForIndex_(int idx) const
{
    if (idx < 0) return TQColor(128, 128, 128);
    if (m_palette.isEmpty()) return TQColor(128, 128, 128);

    int k = idx;
    const int n = (int)m_palette.count();
    if (n <= 0) return TQColor(128, 128, 128);

    k %= n;
    if (k < 0) k += n;

    TQtChartPalette::ConstIterator it = m_palette.begin();
    for (int i = 0; i < k; ++i) ++it;
    return *it;
}

#include "tqtabstractchartwidget.moc"
