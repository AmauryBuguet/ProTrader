#include "statswindow.hpp"

StatsWindow::StatsWindow(QJsonArray array, QWidget *parent) : QWidget(parent)
{
    setWindowModality( Qt::ApplicationModal );
    setAttribute( Qt::WA_DeleteOnClose );
    setWindowTitle( "Trading Account Stats" );
    resize(600, 400);

    // CHART
    _balanceSeries = new QLineSeries();
    _chart = new QChart;
    _timeAxis = new QDateTimeAxis();
    _valueAxis = new QValueAxis();
    _chartView = new QChartView(this);

    _balanceSeries->setColor(Qt::darkBlue);

    _chart->addSeries(_balanceSeries);
    _chart->setTitle("Balance evolution");
    _chart->addAxis(_timeAxis, Qt::AlignBottom);
    _chart->addAxis(_valueAxis, Qt::AlignLeft);

    _balanceSeries->attachAxis(_timeAxis);
    _balanceSeries->attachAxis(_valueAxis);

    _valueAxis->setLabelFormat("%.2f");
    _valueAxis->setTitleText("Value (USDT)");

    _timeAxis->setTitleText("Time");
    _timeAxis->setFormat("dd/MM-HH:mm");

    _chart->legend()->hide();
    _chart->layout()->setContentsMargins(0, 0, 0, 0);

    _chartView->setChart(_chart);
    _chartView->setRenderHint(QPainter::Antialiasing);

    bool first = true;
    qint64 startDate = 0;
    qint64 endDate = 0;
    double min = 1000000;
    double max = 0;
    foreach(const QJsonValue &val, array){
        QJsonObject obj = val.toObject();
        if(first){
            startDate = obj["timestamp"].toString().toDouble();
            first = false;
        }
        if(obj["balance"].toString().toDouble() < min) min = obj["balance"].toString().toDouble();
        if(obj["balance"].toString().toDouble() > max) max = obj["balance"].toString().toDouble();
        endDate = obj["timestamp"].toString().toDouble();
        _balanceSeries->append(obj["timestamp"].toString().toDouble(), obj["balance"].toString().toDouble());
    }
    _timeAxis->setRange(QDateTime::fromMSecsSinceEpoch(startDate), QDateTime::fromMSecsSinceEpoch(endDate));
    _valueAxis->setRange(min/1.01, max*1.01);

    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->addWidget(_chartView);
}
