#include "tqtmosaicplotwidget.h"

#include <ntqfontmetrics.h>
#include <ntqpainter.h>
#include <ntqpen.h>
#include <ntqbrush.h>

TQtMosaicPlotWidget::TQtMosaicPlotWidget(TQWidget* parent)
    : TQtAbstractChartWidget(parent)
{
}

TQtMosaicPlotWidget::~TQtMosaicPlotWidget() {}

void TQtMosaicPlotWidget::setGroups(const TQtChartGroupList& groups)
{
    m_groups = groups;
    update();
}

const TQtChartGroupList& TQtMosaicPlotWidget::groups() const
{
    return m_groups;
}

void TQtMosaicPlotWidget::paintLegend_(TQPainter& p, const TQRect& r)
{
    if (!m_showLegend) return;
    if (m_groups.isEmpty()) return;

    p.save();
    p.setFont(m_legendFont);

    const int box = 10;
    const int pad = 6;

    TQValueList<TQtChartItem> uniq;

    TQtChartGroupList::ConstIterator git = m_groups.begin();
    for (; git != m_groups.end(); ++git) {
        const TQtChartItemList& items = (*git).items;
        TQtChartItemList::ConstIterator it = items.begin();
        for (; it != items.end(); ++it) {
            int found = 0;
            TQValueList<TQtChartItem>::ConstIterator uit = uniq.begin();
            for (; uit != uniq.end(); ++uit) {
                if ((*uit).label == (*it).label) { found = 1; break; }
            }
            if (!found) uniq.append(*it);
        }
    }

    int x = r.x();
    int y = r.y();

    int idx = 0;
    TQValueList<TQtChartItem>::ConstIterator uit = uniq.begin();
    for (; uit != uniq.end(); ++uit, ++idx) {
        const TQtChartItem& item = *uit;
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

void TQtMosaicPlotWidget::paintEvent(TQPaintEvent* ev)
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

    if (m_groups.isEmpty() || plotR.width() <= 8 || plotR.height() <= 8) {
        paintLegend_(p, legendR);
        return;
    }

    double totalAll = 0.0;
    {
        TQtChartGroupList::ConstIterator git = m_groups.begin();
        for (; git != m_groups.end(); ++git) {
            const TQtChartItemList& items = (*git).items;
            TQtChartItemList::ConstIterator it = items.begin();
            double sum = 0.0;
            for (; it != items.end(); ++it) {
                if ((*it).value > 0.0) sum += (*it).value;
            }
            totalAll += sum;
        }
    }
    if (totalAll <= 0.0) {
        paintLegend_(p, legendR);
        return;
    }

    TQValueList<TQtChartItem> uniq;
    {
        TQtChartGroupList::ConstIterator git = m_groups.begin();
        for (; git != m_groups.end(); ++git) {
            const TQtChartItemList& items = (*git).items;
            TQtChartItemList::ConstIterator it = items.begin();
            for (; it != items.end(); ++it) {
                int found = 0;
                TQValueList<TQtChartItem>::ConstIterator uit = uniq.begin();
                for (; uit != uniq.end(); ++uit) {
                    if ((*uit).label == (*it).label) { found = 1; break; }
                }
                if (!found) uniq.append(*it);
            }
        }
    }

    const int gap = 6;
    const int nGroups = (int)m_groups.count();
    int plotW = plotR.width() - (nGroups - 1) * gap;
    if (plotW < 1) plotW = 1;

    TQFontMetrics fm(font());

    int x = plotR.left();
    int groupIdx = 0;
    TQtChartGroupList::ConstIterator git = m_groups.begin();
    for (; git != m_groups.end(); ++git, ++groupIdx) {
        const TQtChartGroup& g = *git;

        double sumG = 0.0;
        {
            const TQtChartItemList& items = g.items;
            TQtChartItemList::ConstIterator it = items.begin();
            for (; it != items.end(); ++it) {
                if ((*it).value > 0.0) sumG += (*it).value;
            }
        }
        if (sumG <= 0.0) sumG = 0.0;

        int w = (int)((double)plotW * (sumG / totalAll));
        if (groupIdx == nGroups - 1) {
            w = plotR.right() - x + 1;
        }
        if (w < 2) w = 2;

        int y = plotR.top();
        const int hAll = plotR.height();

        int subIdx = 0;
        const TQtChartItemList& items = g.items;
        int nSub = (int)items.count();
        TQtChartItemList::ConstIterator it = items.begin();
        for (; it != items.end(); ++it, ++subIdx) {
            const TQtChartItem& item = *it;
            const double v = (item.value < 0.0 ? 0.0 : item.value);

            int h = 0;
            if (sumG > 0.0) h = (int)((double)hAll * (v / sumG));
            if (subIdx == nSub - 1) {
                h = plotR.bottom() - y + 1;
            }
            if (h < 1) h = 1;

            int k = 0;
            {
                TQValueList<TQtChartItem>::ConstIterator uit = uniq.begin();
                int kk = 0;
                for (; uit != uniq.end(); ++uit, ++kk) {
                    if ((*uit).label == item.label) { k = kk; break; }
                }
            }

            const TQColor col = item.hasColor ? item.color : colorForIndex_(k);

            p.setPen(TQPen(TQt::NoPen));
            p.setBrush(col);
            p.drawRect(TQRect(x, y, w, h));

            p.setPen(TQPen(TQColor(80, 80, 80), 1));
            p.setBrush(TQBrush(TQt::NoBrush));
            p.drawRect(TQRect(x, y, w, h));

            y += h;
            if (y > plotR.bottom()) break;
        }

        if (!g.label.isEmpty()) {
            const int tw = fm.width(g.label);
            const int tx = x + (w - tw) / 2;
            const int ty = plotR.bottom() - 2;
            p.setPen(TQPen(black, 1));
            p.drawText(tx, ty, g.label);
        }

        x += w + gap;
        if (x > plotR.right()) break;
    }

    paintLegend_(p, legendR);
}

#include "tqtmosaicplotwidget.moc"
