#include "tqtstackedbarchartwidget.h"

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

TQtStackedBarChartWidget::TQtStackedBarChartWidget(TQWidget* parent)
    : TQtAbstractChartWidget(parent),
      m_orient(TQt::Vertical),
      m_normalizeTo100(0),
      m_showValueLabels(0),
      m_showPercentLabels(0),
      m_categorySpacing(8),
      m_thickness(10)
{
}

TQtStackedBarChartWidget::~TQtStackedBarChartWidget() {}

void TQtStackedBarChartWidget::setCategories(const TQStringList& cats)
{
    m_categories = cats;
    update();
}

const TQStringList& TQtStackedBarChartWidget::categories() const
{
    return m_categories;
}

void TQtStackedBarChartWidget::setSeries(const TQtChartSeriesList& series)
{
    m_series = series;
    update();
}

const TQtChartSeriesList& TQtStackedBarChartWidget::series() const
{
    return m_series;
}

void TQtStackedBarChartWidget::clearSeries()
{
    m_series.clear();
    update();
}

void TQtStackedBarChartWidget::setOrientation(TQt::Orientation o)
{
    m_orient = o;
    update();
}

TQt::Orientation TQtStackedBarChartWidget::orientation() const
{
    return m_orient;
}

void TQtStackedBarChartWidget::setNormalizeTo100(int on)
{
    m_normalizeTo100 = on ? 1 : 0;
    update();
}

int TQtStackedBarChartWidget::normalizeTo100() const
{
    return m_normalizeTo100;
}

void TQtStackedBarChartWidget::setShowValueLabels(int on)
{
    m_showValueLabels = on ? 1 : 0;
    update();
}

int TQtStackedBarChartWidget::showValueLabels() const
{
    return m_showValueLabels;
}

void TQtStackedBarChartWidget::setShowPercentLabels(int on)
{
    m_showPercentLabels = on ? 1 : 0;
    update();
}

int TQtStackedBarChartWidget::showPercentLabels() const
{
    return m_showPercentLabels;
}

void TQtStackedBarChartWidget::setCategorySpacing(int px)
{
    if (px < 0) px = 0;
    m_categorySpacing = px;
    update();
}

int TQtStackedBarChartWidget::categorySpacing() const
{
    return m_categorySpacing;
}

void TQtStackedBarChartWidget::setThickness(int px)
{
    if (px < 0) px = 0;
    m_thickness = px;
    update();
}

int TQtStackedBarChartWidget::thickness() const
{
    return m_thickness;
}

TQColor TQtStackedBarChartWidget::seriesColor_(int idx) const
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

void TQtStackedBarChartWidget::paintLegend_(TQPainter& p, const TQRect& r)
{
    if (!m_showLegend) return;

    p.save();
    p.setFont(m_legendFont);

    const int box = 10;
    const int pad = 6;

    int x = r.x();
    int y = r.y();

    TQtChartSeriesList::ConstIterator it = m_series.begin();
    int idx = 0;
    for (; it != m_series.end(); ++it, ++idx) {
        const TQtChartSeries& s = *it;
        const TQColor col = s.hasColor ? s.color : seriesColor_(idx);

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

void TQtStackedBarChartWidget::paintEvent(TQPaintEvent* ev)
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

    const int nCats = (int)m_categories.count();
    const int nSeries = (int)m_series.count();

    if (nCats <= 0 || nSeries <= 0 || plotR.width() <= 8 || plotR.height() <= 8) {
        paintLegend_(p, legendR);
        return;
    }

    int valuesOk = 0;
    for (int si = 0; si < nSeries && !valuesOk; ++si) {
        if ((int)m_series[si].values.count() > 0) valuesOk = 1;
    }
    if (!valuesOk) {
        paintLegend_(p, legendR);
        return;
    }

    const int th = (m_render2_5D ? m_thickness : 0);
    const int dx = th;
    const int dy = th;

    if (m_orient == TQt::Vertical) {
        TQRect pr = plotR;
        if (dy > 0) pr.setTop(pr.top() + dy);

        TQFontMetrics fm(font());
        const int labelH = fm.height() + 4;
        
        const int baseY = pr.bottom() - labelH;
        const int frontBaseY = baseY;

        p.setPen(TQPen(TQColor(120, 120, 120), 1));
        p.drawLine(plotR.left(), baseY, plotR.right(), baseY);

        const int stepSpacing = m_categorySpacing + dx;
        const int totalSpacing = (nCats - 1) * stepSpacing;
        int plotW = pr.width() - dx;
        if (plotW < 1) plotW = 1;
        int barW = (plotW - totalSpacing);
        barW = (nCats > 0) ? (barW / nCats) : barW;
        if (barW < 3) barW = 3;

        const int availH = pr.height() - 10;
        if (availH <= 4) {
            paintLegend_(p, legendR);
            return;
        }

        int x = pr.left();
        for (int ci = 0; ci < nCats; ++ci) {
            double total = 0.0;
            for (int si = 0; si < nSeries; ++si) {
                const TQtChartSeries& s = m_series[si];
                if (ci < (int)s.values.count() && s.values[ci] > 0.0) total += s.values[ci];
            }
            if (m_normalizeTo100) {
                if (total <= 0.0) total = 1.0;
            }

            int lastNonZeroSeries = -1;
            for (int si = 0; si < nSeries; ++si) {
                const TQtChartSeries& s = m_series[si];
                const double v = (ci < (int)s.values.count() ? s.values[ci] : 0.0);
                if (v > 0.0) lastNonZeroSeries = si;
            }

            int yCur = frontBaseY;
            for (int si = 0; si < nSeries; ++si) {
                const TQtChartSeries& s = m_series[si];
                const double v = (ci < (int)s.values.count() ? s.values[ci] : 0.0);
                if (v <= 0.0) continue;

                double frac;
                if (m_normalizeTo100) frac = v / total;
                else frac = v;

                int segH;
                if (m_normalizeTo100) segH = (int)((double)availH * frac);
                else {
                    double maxSum = 0.0;
                    for (int cj = 0; cj < nCats; ++cj) {
                        double sum = 0.0;
                        for (int sj = 0; sj < nSeries; ++sj) {
                            const TQtChartSeries& ss = m_series[sj];
                            if (cj < (int)ss.values.count() && ss.values[cj] > 0.0) sum += ss.values[cj];
                        }
                        if (sum > maxSum) maxSum = sum;
                    }
                    if (maxSum <= 0.0) maxSum = 1.0;
                    segH = (int)((double)availH * (v / maxSum));
                }

                if (segH <= 0) continue;

                const int yBottom = yCur;
                const int yTop = yCur - segH;
                const TQColor col = s.hasColor ? s.color : seriesColor_(si);

                const TQRect front(x, yTop, barW, segH);

                p.setPen(TQPen(TQt::NoPen));
                p.setBrush(col);
                p.drawRect(front);

                if (th > 0) {
                    p.setPen(TQPen(TQt::NoPen));
                    p.setBrush(darkenColor(col, 40));
                    TQPointArray side(4);
                    side.setPoint(0, x + barW, yTop);
                    side.setPoint(1, x + barW + dx, yTop - dy);
                    side.setPoint(2, x + barW + dx, yBottom - dy);
                    side.setPoint(3, x + barW, yBottom);
                    p.drawPolygon(side);
                }

                if (m_showValueLabels || m_showPercentLabels) {
                    p.setPen(black);
                    TQString txt;
                    if (m_showValueLabels) txt = TQString::number(v, 'f', 1);
                    if (m_showPercentLabels) {
                        double pct;
                        if (m_normalizeTo100) pct = 100.0 * (v / total);
                        else pct = (total > 0.0) ? (100.0 * (v / total)) : 0.0;
                        if (!txt.isEmpty()) txt += " ";
                        txt += TQString::number(pct, 'f', 1) + "%";
                    }
                    if (!txt.isEmpty()) {
                        TQFontMetrics fm(font());
                        const int tw = fm.width(txt);
                        const int ty = yTop + segH / 2 + fm.ascent() / 2;
                        p.drawText(x + (barW - tw) / 2, ty, txt);
                    }
                }

                yCur = yTop;
            }

            if (th > 0) {
                if (yCur < frontBaseY && lastNonZeroSeries >= 0) {
                    const TQColor colTop = m_series[lastNonZeroSeries].hasColor ? m_series[lastNonZeroSeries].color : seriesColor_(lastNonZeroSeries);

                    p.setBrush(lightenColor(colTop, 25));
                    TQPointArray top(4);
                    top.setPoint(0, x, yCur);
                    top.setPoint(1, x + barW, yCur);
                    top.setPoint(2, x + barW + dx, yCur - dy);
                    top.setPoint(3, x + dx, yCur - dy);
                    p.drawPolygon(top);
                }
            }

            p.setPen(TQPen(TQColor(80, 80, 80), 1));
            p.setBrush(TQBrush(TQt::NoBrush));
            p.drawRect(TQRect(x, yCur, barW, frontBaseY - yCur));

            // Draw category label on X-axis
            if (ci < (int)m_categories.count()) {
                p.setPen(black);
                TQString catLabel = m_categories[ci];
                int tw = fm.width(catLabel);
                p.drawText(x + (barW - tw) / 2, frontBaseY + fm.ascent() + 4, catLabel);
            }

            x += barW + stepSpacing;
        }
    } else {
        const int baseX = plotR.left();
        const int frontBaseX = baseX;
        TQRect pr = plotR;
        if (dy > 0) pr.setTop(pr.top() + dy);
        const int originY = pr.top();

        p.setPen(TQPen(TQColor(120, 120, 120), 1));
        p.drawLine(baseX, plotR.top(), baseX, plotR.bottom());

        const int stepSpacing = m_categorySpacing + dy;
        const int totalSpacing = (nCats - 1) * stepSpacing;
        int plotH = pr.height();
        if (plotH < 1) plotH = 1;
        int barH = (plotH - totalSpacing);
        barH = (nCats > 0) ? (barH / nCats) : barH;
        if (barH < 3) barH = 3;

        int availW = pr.width() - 10 - dx;
        if (availW <= 4) {
            paintLegend_(p, legendR);
            return;
        }

        double maxSum = 0.0;
        if (!m_normalizeTo100) {
            for (int cj = 0; cj < nCats; ++cj) {
                double sum = 0.0;
                for (int sj = 0; sj < nSeries; ++sj) {
                    const TQtChartSeries& ss = m_series[sj];
                    if (cj < (int)ss.values.count() && ss.values[cj] > 0.0) sum += ss.values[cj];
                }
                if (sum > maxSum) maxSum = sum;
            }
            if (maxSum <= 0.0) maxSum = 1.0;
        }

        int y = originY;
        for (int ci = 0; ci < nCats; ++ci) {
            double total = 0.0;
            for (int si = 0; si < nSeries; ++si) {
                const TQtChartSeries& s = m_series[si];
                if (ci < (int)s.values.count() && s.values[ci] > 0.0) total += s.values[ci];
            }
            if (m_normalizeTo100) {
                if (total <= 0.0) total = 1.0;
            }

            int lastNonZeroSeries = -1;
            for (int si = 0; si < nSeries; ++si) {
                const TQtChartSeries& s = m_series[si];
                const double v = (ci < (int)s.values.count() ? s.values[ci] : 0.0);
                if (v > 0.0) lastNonZeroSeries = si;
            }

            int xCur = frontBaseX;
            for (int si = 0; si < nSeries; ++si) {
                const TQtChartSeries& s = m_series[si];
                const double v = (ci < (int)s.values.count() ? s.values[ci] : 0.0);
                if (v <= 0.0) continue;

                int segW;
                if (m_normalizeTo100) segW = (int)((double)availW * (v / total));
                else segW = (int)((double)availW * (v / maxSum));

                if (segW <= 0) continue;

                const TQColor col = s.hasColor ? s.color : seriesColor_(si);
                const TQRect front(xCur, y, segW, barH);

                p.setPen(TQPen(TQt::NoPen));
                p.setBrush(col);
                p.drawRect(front);

                if (th > 0) {
                    p.setPen(TQPen(TQt::NoPen));
                    p.setBrush(lightenColor(col, 25));
                    TQPointArray top(4);
                    top.setPoint(0, xCur, y);
                    top.setPoint(1, xCur + segW, y);
                    top.setPoint(2, xCur + segW + dx, y - dy);
                    top.setPoint(3, xCur + dx, y - dy);
                    p.drawPolygon(top);
                }

                if (m_showValueLabels || m_showPercentLabels) {
                    p.setPen(black);
                    TQString txt;
                    if (m_showValueLabels) txt = TQString::number(v, 'f', 1);
                    if (m_showPercentLabels) {
                        double pct = 100.0 * (v / total);
                        if (!txt.isEmpty()) txt += " ";
                        txt += TQString::number(pct, 'f', 1) + "%";
                    }
                    if (!txt.isEmpty()) {
                        TQFontMetrics fm(font());
                        const int tw = fm.width(txt);
                        const int tx = xCur + (segW - tw) / 2;
                        const int ty = y + barH / 2 + fm.ascent() / 2;
                        p.drawText(tx, ty, txt);
                    }
                }

                xCur += segW;
            }

            if (th > 0 && xCur > frontBaseX) {
                const int topMost = (lastNonZeroSeries >= 0 ? lastNonZeroSeries : 0);
                const TQColor colTop = m_series[topMost].hasColor ? m_series[topMost].color : seriesColor_(topMost);

                p.setPen(TQPen(TQt::NoPen));
                p.setBrush(darkenColor(colTop, 40));
                TQPointArray side(4);
                side.setPoint(0, xCur, y);
                side.setPoint(1, xCur + dx, y - dy);
                side.setPoint(2, xCur + dx, y + barH - dy);
                side.setPoint(3, xCur, y + barH);
                p.drawPolygon(side);
            }

            p.setPen(TQPen(TQColor(80, 80, 80), 1));
            p.setBrush(TQBrush(TQt::NoBrush));
            p.drawRect(TQRect(frontBaseX, y, xCur - frontBaseX, barH));

            y += barH + stepSpacing;
        }
    }

    paintLegend_(p, legendR);
}

#include "tqtstackedbarchartwidget.moc"
