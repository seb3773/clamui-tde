#include "tqtbarchartwidget.h"

#include <ntqfontmetrics.h>
#include <ntqpainter.h>
#include <ntqpen.h>
#include <ntqpointarray.h>
#include <ntqbrush.h>

static inline TQColor darkenColor(const TQColor& c, int d)
{
    int r = c.red() - d;
    int g = c.green() - d;
    int b = c.blue() - d;
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    return TQColor(r, g, b);
}

static inline TQColor lightenColor(const TQColor& c, int d)
{
    int r = c.red() + d;
    int g = c.green() + d;
    int b = c.blue() + d;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    return TQColor(r, g, b);
}

TQtBarChartWidget::TQtBarChartWidget(TQWidget* parent)
    : TQtAbstractChartWidget(parent),
      m_showValueLabels(0),
      m_barSpacing(6),
      m_thickness(10)
{
}

TQtBarChartWidget::~TQtBarChartWidget() {}

void TQtBarChartWidget::setShowValueLabels(int on)
{
    m_showValueLabels = on ? 1 : 0;
    update();
}

int TQtBarChartWidget::showValueLabels() const
{
    return m_showValueLabels;
}

void TQtBarChartWidget::setBarSpacing(int px)
{
    if (px < 0) px = 0;
    m_barSpacing = px;
    update();
}

int TQtBarChartWidget::barSpacing() const
{
    return m_barSpacing;
}

void TQtBarChartWidget::setThickness(int px)
{
    if (px < 0) px = 0;
    m_thickness = px;
    update();
}

int TQtBarChartWidget::thickness() const
{
    return m_thickness;
}

void TQtBarChartWidget::paintLegend_(TQPainter& p, const TQRect& r)
{
    if (!m_showLegend) return;

    p.save();
    p.setFont(m_legendFont);

    const int box = 10;
    const int pad = 6;

    int x = r.x();
    int y = r.y();

    TQtChartItemList::ConstIterator it = m_items.begin();
    int idx = 0;
    for (; it != m_items.end(); ++it, ++idx) {
        const TQtChartItem& item = *it;
        const TQColor col = item.hasColor ? item.color : colorForIndex_(idx);

        p.fillRect(x, y + 2, box, box, col);
        p.setPen(TQPen(black, 1));
        p.drawRect(x, y + 2, box, box);

        p.setPen(black);
        p.drawText(x + box + pad, y + box + 2, item.label);

        y += box + pad;
        if (y + box + pad > r.bottom()) break;
    }

    p.restore();
}

void TQtBarChartWidget::paintEvent(TQPaintEvent* ev)
{
    (void)ev;

    TQPainter p(this);
    p.fillRect(rect(), colorGroup().base());

    const int margin = 8;
    const int legendW = (m_showLegend ? 160 : 0);

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

    const int n = (int)m_items.count();
    if (n <= 0 || plotR.width() <= 8 || plotR.height() <= 8) {
        paintLegend_(p, legendR);
        return;
    }

    double maxv = 0.0;
    TQtChartItemList::ConstIterator it = m_items.begin();
    for (; it != m_items.end(); ++it) {
        if ((*it).value > maxv) maxv = (*it).value;
    }
    if (maxv <= 0.0) {
        paintLegend_(p, legendR);
        return;
    }

    const int th = (m_render2_5D ? m_thickness : 0);
    const int dx = th;
    const int dy = th;

    TQRect pr = plotR;
    if (dy > 0) pr.setTop(pr.top() + dy);

    const int totalSpacing = (n - 1) * m_barSpacing;
    int plotW = pr.width() - dx;
    if (plotW < 1) plotW = 1;
    int barW = (plotW - totalSpacing) / n;
    if (barW < 3) barW = 3;

    const int baseY = pr.bottom();
    const int frontBaseY = baseY;

    p.setPen(TQPen(TQColor(120, 120, 120), 1));
    p.drawLine(plotR.left(), baseY, plotR.right(), baseY);

    int x = pr.left();
    it = m_items.begin();
    int idx = 0;

    for (; it != m_items.end(); ++it, ++idx) {
        const TQtChartItem& item = *it;
        const double v = (item.value < 0.0 ? 0.0 : item.value);
        const int h = (int)((double)(pr.height() - 10) * (v / maxv));

        const int yTop = frontBaseY - h;
        const TQColor col = item.hasColor ? item.color : colorForIndex_(idx);

        if (th > 0) {
            const TQRect front(x, yTop, barW, h);

            p.setPen(TQPen(TQt::NoPen));
            p.setBrush(col);
            p.drawRect(front);

            p.setBrush(darkenColor(col, 40));
            TQPointArray poly(4);
            poly.setPoint(0, x + barW, yTop);
            poly.setPoint(1, x + barW + dx, yTop - dy);
            poly.setPoint(2, x + barW + dx, frontBaseY - dy);
            poly.setPoint(3, x + barW, frontBaseY);
            p.drawPolygon(poly);

            p.setBrush(lightenColor(col, 25));
            TQPointArray top(4);
            top.setPoint(0, x, yTop);
            top.setPoint(1, x + barW, yTop);
            top.setPoint(2, x + barW + dx, yTop - dy);
            top.setPoint(3, x + dx, yTop - dy);
            p.drawPolygon(top);

            p.setPen(TQPen(TQColor(80, 80, 80), 1));
            p.setBrush(TQBrush(TQt::NoBrush));
            p.drawRect(front);
        } else {
            const TQRect front(x, yTop, barW, h);
            p.setPen(TQPen(TQt::NoPen));
            p.setBrush(col);
            p.drawRect(front);
            p.setPen(TQPen(TQColor(80, 80, 80), 1));
            p.setBrush(TQBrush(TQt::NoBrush));
            p.drawRect(front);
        }

        if (m_showValueLabels) {
            p.setPen(black);
            TQString s = TQString::number(item.value, 'f', 1);
            TQFontMetrics fm(font());
            const int tw = fm.width(s);
            p.drawText(x + (barW - tw) / 2, yTop - 2, s);
        }

        x += barW + m_barSpacing;
    }

    paintLegend_(p, legendR);
}

#include "tqtbarchartwidget.moc"
