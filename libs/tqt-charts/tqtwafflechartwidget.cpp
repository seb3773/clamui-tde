#include "tqtwafflechartwidget.h"

#include <ntqfontmetrics.h>
#include <ntqpainter.h>
#include <ntqpen.h>
#include <ntqbrush.h>
#include <ntqvaluevector.h>

TQtWaffleChartWidget::TQtWaffleChartWidget(TQWidget* parent)
    : TQtAbstractChartWidget(parent),
      m_cols(10),
      m_rows(10)
{
}

TQtWaffleChartWidget::~TQtWaffleChartWidget() {}

void TQtWaffleChartWidget::setGridSize(int cols, int rows)
{
    if (cols < 1) cols = 1;
    if (rows < 1) rows = 1;
    if (cols > 64) cols = 64;
    if (rows > 64) rows = 64;
    m_cols = cols;
    m_rows = rows;
    update();
}

int TQtWaffleChartWidget::gridCols() const { return m_cols; }
int TQtWaffleChartWidget::gridRows() const { return m_rows; }

void TQtWaffleChartWidget::paintLegend_(TQPainter& p, const TQRect& r)
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

void TQtWaffleChartWidget::paintEvent(TQPaintEvent* ev)
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

    const int n = (int)m_items.count();
    if (n <= 0 || plotR.width() <= 8 || plotR.height() <= 8) {
        paintLegend_(p, legendR);
        return;
    }

    double total = 0.0;
    {
        TQtChartItemList::ConstIterator it = m_items.begin();
        for (; it != m_items.end(); ++it) {
            if ((*it).value > 0.0) total += (*it).value;
        }
    }
    if (total <= 0.0) {
        paintLegend_(p, legendR);
        return;
    }

    const int cells = m_cols * m_rows;

    int cell = plotR.width() / m_cols;
    int cellH = plotR.height() / m_rows;
    if (cellH < cell) cell = cellH;
    if (cell < 2) cell = 2;

    const int gridW = cell * m_cols;
    const int gridH = cell * m_rows;

    const int ox = plotR.left() + (plotR.width() - gridW) / 2;
    const int oy = plotR.top() + (plotR.height() - gridH) / 2;

    TQValueVector<int> counts;
    counts.clear();
    counts.reserve(n);

    TQValueVector<TQColor> colors;
    colors.clear();
    colors.reserve(n);

    int used = 0;
    {
        TQtChartItemList::ConstIterator it = m_items.begin();
        int idx = 0;
        for (; it != m_items.end(); ++it, ++idx) {
            double frac = ((*it).value > 0.0 ? ((*it).value / total) : 0.0);
            int c = (int)((double)cells * frac + 0.5);
            if (c < 0) c = 0;
            counts.push_back(c);

            const TQColor col = (*it).hasColor ? (*it).color : colorForIndex_(idx);
            colors.push_back(col);

            used += c;
        }
    }

    if (used > cells) {
        int delta = used - cells;
        for (int i = (int)counts.size() - 1; i >= 0 && delta > 0; --i) {
            int c = counts[(unsigned)i];
            if (c > 0) {
                int sub = (c < delta ? c : delta);
                counts[(unsigned)i] = c - sub;
                delta -= sub;
            }
        }
    } else if (used < cells) {
        int delta = cells - used;
        if (counts.size() != 0) {
            counts[(unsigned)(counts.size() - 1)] += delta;
        }
    }

    int curItem = 0;
    int curLeft = (counts.size() == 0 ? 0 : counts[(unsigned)0]);

    p.setPen(TQPen(TQt::NoPen));

    for (int i = 0; i < cells; ++i) {
        while (curLeft <= 0 && curItem + 1 < (int)counts.size()) {
            ++curItem;
            curLeft = counts[(unsigned)curItem];
        }

        const int cx = i % m_cols;
        const int cy = i / m_cols;

        const int x = ox + cx * cell;
        const int y = oy + cy * cell;

        p.setBrush(colors[(unsigned)curItem]);
        p.drawRect(x + 1, y + 1, cell - 2, cell - 2);

        --curLeft;
    }

    p.setPen(TQPen(TQColor(80, 80, 80), 1));
    p.setBrush(TQBrush(TQt::NoBrush));
    p.drawRect(TQRect(ox, oy, gridW, gridH));

    paintLegend_(p, legendR);
}

#include "tqtwafflechartwidget.moc"
