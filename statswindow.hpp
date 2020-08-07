#ifndef STATSWINDOW_HPP
#define STATSWINDOW_HPP

#include <QWidget>
#include <QtCharts>

class StatsWindow : public QWidget
{
    Q_OBJECT
public:
    StatsWindow(QJsonArray array, QWidget *parent = nullptr);

private:
    // CHART
    QChartView *_chartView;
    QChart *_chart;
    QLineSeries *_balanceSeries;
    QValueAxis *_valueAxis;
    QDateTimeAxis *_timeAxis;
};

#endif // STATSWINDOW_HPP
