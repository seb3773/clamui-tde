# tqt-charts

Mini-library providing lightweight custom-painted charts for TQt3.

## Goals

- Minimal dependencies (TQt3 only)
- Good-looking 2D charts
- Optional 2.5D ("fake 3D") rendering without OpenGL

## Provided widgets

- `TQtPieChartWidget`
  - Pie chart 2D
  - Optional 2.5D extrusion + shading
  - Optional legend
  - Optional value/percent labels
  - Optional donut mode (inner radius)

- `TQtBarChartWidget`
  - Bar chart 2D
  - Optional 2.5D extrusion + shading
  - Optional legend

- `TQtStackedBarChartWidget`
  - Stacked bars 2D
  - Vertical or horizontal
  - Optional 2.5D extrusion + shading
  - Optional 100% mode
  - Optional legend

- `TQtMosaicPlotWidget`
  - Mosaic plot 2D (2 levels: Group -> Subgroup)
  - Optional legend

- `TQtWaffleChartWidget`
  - Waffle chart 2D
  - Configurable grid size
  - Optional legend

- `TQtScatterPlotWidget`
  - Scatter plot 2D
  - Multiple point series
  - Optional legend

- `TQtHeatmapWidget`
  - Heatmap 2D
  - Optional cell values

## Data model

Charts are built from items:

- label (`TQString`)
- value (`double`)
- optional color (`TQColor`)

If a color is not provided, an automatic palette is used.

Additional structures:

- `TQtChartSeries` / `TQtChartSeriesList`
  - Stacked bar series (label + optional color + list of values)

- `TQtChartGroup` / `TQtChartGroupList`
  - Mosaic plot groups (group label + list of `TQtChartItem` as subgroups)

- `TQtChartPointSeries` / `TQtChartPointSeriesList`
  - Scatter plot series (label + optional color + list of `(x, y)` points)

- `TQtHeatmapData`
  - Heatmap grid (rows, cols, values)

## Example

Build target:

- `tqtcharts_example`

Source:

- `tqt-charts/example/main.cpp`
