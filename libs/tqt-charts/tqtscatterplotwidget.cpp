#include "tqtscatterplotwidget.h"

#include <ntqfontmetrics.h>
#include <ntqpainter.h>
#include <ntqpen.h>
#include <ntqbrush.h>

TQtScatterPlotWidget::TQtScatterPlotWidget(TQWidget* parent)
    : TQtAbstractChartWidget(parent),
      m_pointRadius(4)
{
}

TQtScatterPlotWidget::~TQtScatterPlotWidget() {}

void TQtScatterPlotWidget::setSeries(const TQtChartPointSeriesList& series)
{
    m_series = series;
    update();
}

const TQtChartPointSeriesList& TQtScatterPlotWidget::series() const
{
    return m_series;
}

void TQtScatterPlotWidget::setPointRadius(int px)
{
    if (px < 1) px = 1;
    if (px > 32) px = 32;
    m_pointRadius = px;
    update();
}

int TQtScatterPlotWidget::pointRadius() const
{
    return m_pointRadius;
}

void TQtScatterPlotWidget::paintLegend_(TQPainter& p, const TQRect& r)
{
    if (!m_showLegend) return;

    p.save();
    p.setFont(m_legendFont);

    const int box = 10;
    const int pad = 6;

    int x = r.x();
    int y = r.y();

    TQtChartPointSeriesList::ConstIterator it = m_series.begin();
    int idx = 0;
    for (; it != m_series.end(); ++it, ++idx) {
        const TQtChartPointSeries& s = *it;
        const TQColor col = s.hasColor ? s.color : colorForIndex_(idx);

        p.fillRect(x, y + 2, box, box, col);
        p.setPen(TQPen(black, 1));
        p.drawRect(x, y + 2, box, box);

        p.setPen(black);
        p.drawText(x + box + pad, y + box + 2, s.label);

        y += box + pad;
        if (y + box + pad > r.bottom()) break;
    }

    p.restore();
}

void TQtScatterPlotWidget::computeBounds_(double& minX, double& maxX, double& minY, double& maxY) const
{
    minX = 0.0;
    maxX = 0.0;
    minY = 0.0;
    maxY = 0.0;

    int any = 0;

    TQtChartPointSeriesList::ConstIterator sit = m_series.begin();
    for (; sit != m_series.end(); ++sit) {
        const TQtChartPointList& pts = (*sit).points;
        TQtChartPointList::ConstIterator it = pts.begin();
        for (; it != pts.end(); ++it) {
            const double x = (*it).x;
            const double y = (*it).y;
            if (!any) {
                minX = maxX = x;
                minY = maxY = y;
                any = 1;
            } else {
                if (x < minX) minX = x;
                if (x > maxX) maxX = x;
                if (y < minY) minY = y;
                if (y > maxY) maxY = y;
            }
        }
    }

    if (!any) {
        minX = 0.0;
        maxX = 1.0;
        minY = 0.0;
        maxY = 1.0;
        return;
    }

    if (minX == maxX) { minX -= 1.0; maxX += 1.0; }
    if (minY == maxY) { minY -= 1.0; maxY += 1.0; }

    const double padX = (maxX - minX) * 0.05;
    const double padY = (maxY - minY) * 0.05;
    minX -= padX;
    maxX += padX;
    minY -= padY;
    maxY += padY;
}

void TQtScatterPlotWidget::paintEvent(TQPaintEvent* ev)
{
    (void)ev;

    TQPainter p(this);
    p.fillRect(rect(), colorGroup().base());

    const int margin = 8;
    const int legendW = (m_showLegend ? 180 : 0);

    TQRect plotR = rect();
    plotR.setLeft(plotR.left() + margin);
    plotR.setTop(plotR.top() + margin);
    plotR.setBottom(plotR.bottom() - margin);
    plotR.setRight(plotR.right() - margin - legendW);

    TQRect legendR;
    if (m_showLegend) {
        legendR = rect();
        legendR.setLeft(rect().right() - legendW + 1);
        legendR.setTop(rect().top() + margin);
        legendR.setRight(rect().right() - margin);
        legendR.setBottom(rect().bottom() - margin);
    }

    if (m_series.isEmpty() || plotR.width() <= 8 || plotR.height() <= 8) {
        paintLegend_(p, legendR);
        return;
    }

    double minX, maxX, minY, maxY;
    computeBounds_(minX, maxX, minY, maxY);

    const int leftPad = 28;
    const int bottomPad = 22;
    const int topPad = 8;
    const int rightPad = 8;

    TQRect pr = plotR;
    pr.setLeft(pr.left() + leftPad);
    pr.setRight(pr.right() - rightPad);
    pr.setTop(pr.top() + topPad);
    pr.setBottom(pr.bottom() - bottomPad);

    if (pr.width() <= 4 || pr.height() <= 4) {
        paintLegend_(p, legendR);
        return;
    }

    const int ox = pr.left();
    const int oy = pr.bottom();

    p.setPen(TQPen(TQColor(120, 120, 120), 1));
    p.drawLine(ox, pr.top(), ox, pr.bottom());
    p.drawLine(pr.left(), oy, pr.right(), oy);

    const double invW = 1.0 / (maxX - minX);
    const double invH = 1.0 / (maxY - minY);

    TQtChartPointSeriesList::ConstIterator sit = m_series.begin();
    int sidx = 0;
    for (; sit != m_series.end(); ++sit, ++sidx) {
        const TQtChartPointSeries& s = *sit;
        const TQColor col = s.hasColor ? s.color : colorForIndex_(sidx);

        p.setPen(TQPen(TQt::NoPen));
        p.setBrush(col);

        const TQtChartPointList& pts = s.points;
        TQtChartPointList::ConstIterator it = pts.begin();
        for (; it != pts.end(); ++it) {
            const double x = (*it).x;
            const double y = (*it).y;

            int px = pr.left() + (int)((double)pr.width() * ((x - minX) * invW));
            int py = pr.bottom() - (int)((double)pr.height() * ((y - minY) * invH));

            p.drawEllipse(px - m_pointRadius, py - m_pointRadius, m_pointRadius * 2, m_pointRadius * 2);
        }
    }

    paintLegend_(p, legendR);
}

#include "tqtscatterplotwidget.moc"
