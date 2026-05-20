#include "tqtpiecechartwidget.h"

#include <math.h>
#include <ntqfontmetrics.h>
#include <ntqpainter.h>
#include <ntqpen.h>

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

TQtPieChartWidget::TQtPieChartWidget(TQWidget* parent)
    : TQtAbstractChartWidget(parent),
      m_showPercentLabels(0),
      m_showValueLabels(0),
      m_thickness(12),
      m_innerRadiusPermille(0)
{
}

TQtPieChartWidget::~TQtPieChartWidget() {}

void TQtPieChartWidget::setShowPercentLabels(int on)
{
    m_showPercentLabels = on ? 1 : 0;
    update();
}

int TQtPieChartWidget::showPercentLabels() const
{
    return m_showPercentLabels;
}

void TQtPieChartWidget::setShowValueLabels(int on)
{
    m_showValueLabels = on ? 1 : 0;
    update();
}

int TQtPieChartWidget::showValueLabels() const
{
    return m_showValueLabels;
}

void TQtPieChartWidget::setThickness(int px)
{
    if (px < 0) px = 0;
    m_thickness = px;
    update();
}

int TQtPieChartWidget::thickness() const
{
    return m_thickness;
}

void TQtPieChartWidget::setInnerRadiusPermille(int permille)
{
    if (permille < 0) permille = 0;
    if (permille > 900) permille = 900;
    m_innerRadiusPermille = permille;
    update();
}

int TQtPieChartWidget::innerRadiusPermille() const
{
    return m_innerRadiusPermille;
}

double TQtPieChartWidget::total_() const
{
    double t = 0.0;
    TQtChartItemList::ConstIterator it = m_items.begin();
    for (; it != m_items.end(); ++it) {
        if ((*it).value > 0.0) t += (*it).value;
    }
    return t;
}

void TQtPieChartWidget::paintLegend_(TQPainter& p, const TQRect& r)
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

void TQtPieChartWidget::paintEvent(TQPaintEvent* ev)
{
    (void)ev;

    TQPainter p(this);
    p.fillRect(rect(), colorGroup().base());

    const int margin = 8;
    const int legendW = (m_showLegend ? 130 : 0);

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

    if (plotR.width() <= 8 || plotR.height() <= 8) {
        paintLegend_(p, legendR);
        return;
    }

    const double t = total_();
    if (t <= 0.0) {
        paintLegend_(p, legendR);
        return;
    }

    int dia = plotR.width();
    if (plotR.height() < dia) dia = plotR.height();

    const int th = (m_render2_5D ? m_thickness : 0);

    TQRect pieR(plotR.center().x() - dia / 2, plotR.center().y() - dia / 2, dia, dia);
    pieR.moveBy(0, -(th / 2));

    int start16 = 90 * 16;

    const int rx = pieR.width() / 2;
    const int ry = pieR.height() / 2;
    const int innerR = (m_innerRadiusPermille > 0) ? (rx * m_innerRadiusPermille) / 1000 : 0;
    const int innerRx = innerR;
    const int innerRy = (ry * innerR) / (rx > 0 ? rx : 1);
    TQRect innerR0(pieR.center().x() - innerRx, pieR.center().y() - innerRy, innerRx * 2, innerRy * 2);

    if (th > 0) {
        const int steps = th;
        for (int s = steps; s >= 1; --s) {
            int a16 = start16;
            TQtChartItemList::ConstIterator it2 = m_items.begin();
            int idx2 = 0;
            for (; it2 != m_items.end(); ++it2, ++idx2) {
                const TQtChartItem& item = *it2;
                if (item.value <= 0.0) continue;

                const TQColor base = item.hasColor ? item.color : colorForIndex_(idx2);
                const TQColor side = darkenColor(base, 40);

                const int span16 = (int)(-360.0 * 16.0 * (item.value / t));

                p.setPen(TQPen(TQt::NoPen));
                p.setBrush(side);
                TQRect rr = pieR;
                rr.moveBy(0, s);
                p.drawPie(rr, a16, span16);

                a16 += span16;
            }

            if (innerR > 0) {
                p.setPen(TQPen(TQt::NoPen));
                p.setBrush(colorGroup().base());
                TQRect hi = innerR0;
                hi.moveBy(0, s);
                p.drawEllipse(hi);
            }
        }
    }

    int a16 = start16;
    TQtChartItemList::ConstIterator it = m_items.begin();
    int idx = 0;
    for (; it != m_items.end(); ++it, ++idx) {
        const TQtChartItem& item = *it;
        if (item.value <= 0.0) continue;

        const int span16 = (int)(-360.0 * 16.0 * (item.value / t));
        const TQColor col = item.hasColor ? item.color : colorForIndex_(idx);

        p.setPen(TQPen(TQt::NoPen));
        p.setBrush(col);
        p.drawPie(pieR, a16, span16);

        a16 += span16;
    }

    if (innerR > 0) {
        p.setPen(TQPen(TQt::NoPen));
        p.setBrush(colorGroup().base());
        p.drawEllipse(innerR0);
    }

    if (m_showPercentLabels || m_showValueLabels) {
        p.save();
        p.setPen(black);

        int a16l = start16;
        TQtChartItemList::ConstIterator itl = m_items.begin();
        int idxl = 0;
        for (; itl != m_items.end(); ++itl, ++idxl) {
            const TQtChartItem& item = *itl;
            if (item.value <= 0.0) continue;

            const int span16 = (int)(-360.0 * 16.0 * (item.value / t));
            const int mid16 = a16l + span16 / 2;

            const double midDeg = (double)mid16 / 16.0;
            const double rad = (3.14159265358979323846 / 180.0) * (90.0 - midDeg);

            const int cx = pieR.center().x();
            const int cy = pieR.center().y();

            int rr;
            if (innerR > 0) {
                rr = (innerR + rx) / 2;
            } else {
                rr = (int)(0.62 * (double)rx);
            }
            const int tx = cx + (int)((double)rr * cos(rad));
            const int ty = cy - (int)((double)rr * sin(rad));

            TQString s;
            if (m_showValueLabels) {
                s = TQString::number(item.value, 'f', 1);
            }
            if (m_showPercentLabels) {
                const double pct = 100.0 * (item.value / t);
                if (!s.isEmpty()) s += " ";
                s += TQString::number(pct, 'f', 1) + "%";
            }

            if (!s.isEmpty()) {
                TQFontMetrics fm(font());
                const int w = fm.width(s);
                const int h = fm.height();
                p.drawText(tx - w / 2, ty + h / 2, s);
            }

            a16l += span16;
        }

        p.restore();
    }

    paintLegend_(p, legendR);
}

#include "tqtpiecechartwidget.moc"
