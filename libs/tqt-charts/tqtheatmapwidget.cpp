#include "tqtheatmapwidget.h"

#include <ntqfontmetrics.h>
#include <ntqpainter.h>
#include <ntqpen.h>
#include <ntqbrush.h>

TQtHeatmapWidget::TQtHeatmapWidget(TQWidget* parent)
    : TQtAbstractChartWidget(parent),
      m_showCellValues(0)
{
}

TQtHeatmapWidget::~TQtHeatmapWidget() {}

void TQtHeatmapWidget::setData(const TQtHeatmapData& d)
{
    m_data = d;
    update();
}

const TQtHeatmapData& TQtHeatmapWidget::data() const
{
    return m_data;
}

void TQtHeatmapWidget::setShowCellValues(int on)
{
    m_showCellValues = on ? 1 : 0;
    update();
}

int TQtHeatmapWidget::showCellValues() const
{
    return m_showCellValues;
}

TQColor TQtHeatmapWidget::heatColor_(double t)
{
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;

    const int r0 = 255;
    const int g0 = 230;
    const int b0 = 120;

    const int r1 = 220;
    const int g1 = 68;
    const int b1 = 55;

    const double u = 1.0 - t;

    const int r = (int)(u * (double)r0 + t * (double)r1 + 0.5);
    const int g = (int)(u * (double)g0 + t * (double)g1 + 0.5);
    const int b = (int)(u * (double)b0 + t * (double)b1 + 0.5);

    return TQColor(r, g, b);
}

void TQtHeatmapWidget::paintEvent(TQPaintEvent* ev)
{
    (void)ev;

    TQPainter p(this);
    p.fillRect(rect(), colorGroup().base());

    const int margin = 8;

    TQRect plotR = rect();
    plotR.setLeft(plotR.left() + margin);
    plotR.setTop(plotR.top() + margin);
    plotR.setBottom(plotR.bottom() - margin);
    plotR.setRight(plotR.right() - margin);

    if (m_data.rows <= 0 || m_data.cols <= 0) return;

    const int nCells = m_data.rows * m_data.cols;
    if ((int)m_data.values.count() < nCells) return;

    double minV = 0.0;
    double maxV = 0.0;
    {
        TQValueList<double>::ConstIterator it = m_data.values.begin();
        int any = 0;
        for (int i = 0; it != m_data.values.end() && i < nCells; ++it, ++i) {
            const double v = *it;
            if (!any) { minV = maxV = v; any = 1; }
            else { if (v < minV) minV = v; if (v > maxV) maxV = v; }
        }
        if (!any) return;
    }
    if (minV == maxV) { maxV = minV + 1.0; }

    const int cellW = plotR.width() / m_data.cols;
    const int cellH = plotR.height() / m_data.rows;

    int cell = cellW;
    if (cellH < cell) cell = cellH;
    if (cell < 3) cell = 3;

    const int gridW = cell * m_data.cols;
    const int gridH = cell * m_data.rows;

    const int ox = plotR.left() + (plotR.width() - gridW) / 2;
    const int oy = plotR.top() + (plotR.height() - gridH) / 2;

    p.setPen(TQPen(TQt::NoPen));

    TQValueList<double>::ConstIterator it = m_data.values.begin();
    int idx = 0;

    for (int r = 0; r < m_data.rows; ++r) {
        for (int c = 0; c < m_data.cols; ++c, ++idx, ++it) {
            const double v = *it;
            const double t = (v - minV) / (maxV - minV);
            const TQColor col = heatColor_(t);

            const int x = ox + c * cell;
            const int y = oy + r * cell;

            p.setBrush(col);
            p.drawRect(x + 1, y + 1, cell - 2, cell - 2);

            if (m_showCellValues) {
                p.setPen(TQPen(black, 1));
                TQString s = TQString::number(v, 'f', 0);
                TQFontMetrics fm(font());
                const int tw = fm.width(s);
                const int th = fm.ascent();
                p.drawText(x + (cell - tw) / 2, y + (cell + th) / 2, s);
                p.setPen(TQPen(TQt::NoPen));
            }
        }
    }

    p.setPen(TQPen(TQColor(80, 80, 80), 1));
    p.setBrush(TQBrush(TQt::NoBrush));
    p.drawRect(TQRect(ox, oy, gridW, gridH));
}

#include "tqtheatmapwidget.moc"
