#include "charthandler.hpp"

ChartHandler::ChartHandler(QWidget *parent) : QChartView(parent)
{
    _priceSeries = new QCandlestickSeries(this);
    _priceSeries->setIncreasingColor(Qt::green);
    _priceSeries->setDecreasingColor(Qt::red);

    _chart = new QChart();
    _chart->addSeries(_priceSeries);
    _chart->setTitle("XTZ/USDT");
    _chart->legend()->hide();
    _chart->layout()->setContentsMargins(0, 0, 0, 0);
    _chart->setBackgroundBrush(QBrush(Qt::lightGray));

    QDateTimeAxis *axisX = new QDateTimeAxis(this);
    axisX->setTitleText("Time");
    axisX->setFormat("dd/MM-HH:mm");
    _chart->addAxis(axisX, Qt::AlignBottom);
    axisX->setRange(QDateTime::currentDateTime(), QDateTime::currentDateTime().addDays(1));
    _priceSeries->attachAxis(axisX);

    _timeAxis = axisX;

    _priceAxis = new QValueAxis(this);
    _priceAxis->setLabelFormat("%.3f");
    _priceAxis->setTitleText("Price ($)");
    _priceAxis->setRange(0, 5);
    _chart->addAxis(_priceAxis, Qt::AlignLeft);
    _priceSeries->attachAxis(_priceAxis);

    for(uint i=0; i< Utils::lineSeriesEnd; i++){
        QLineSeries *serie = new QLineSeries(this);
        _seriesList.insert({Utils::SeriesEnum(i),{serie, true}});
        _chart->addSeries(serie);
        serie->attachAxis(axisX);
        serie->attachAxis(_priceAxis);
    }

    QPen pen(Qt::red);
    pen.setWidth(2);
    setSeriePen(Utils::SL, pen);
    pen.setColor(Qt::green);
    setSeriePen(Utils::TP1, pen);
    pen.setColor(Qt::darkGreen);
    setSeriePen(Utils::TP2, pen);
    pen.setColor(Qt::black);
    setSeriePen(Utils::TRIGGER, pen);
    pen.setColor(Qt::blue);
    setSeriePen(Utils::ENTRY, pen);
    pen.setStyle(Qt::DashLine);
    setSeriePen(Utils::REAL_ENTRY, pen);
    setSerieMovable(Utils::REAL_ENTRY, false);

    setChart(_chart);
    setRenderHint(QPainter::Antialiasing);
}

void ChartHandler::setSeriePen(Utils::SeriesEnum name, QPen pen)
{
    if(name == Utils::lineSeriesEnd) return;
    auto it = _seriesList.find(name);
    if(it != _seriesList.end()){
        it->second.first->setPen(pen);
    }
}

void ChartHandler::clearSerie(Utils::SeriesEnum name)
{
    if(name == Utils::lineSeriesEnd) return;
    auto it = _seriesList.find(name);
    if(it != _seriesList.end()){
        it->second.first->clear();
    }
}

double ChartHandler::seriePrice(Utils::SeriesEnum name)
{
    if(name == Utils::lineSeriesEnd) return 0;
    auto it = _seriesList.find(name);
    if(it != _seriesList.end()){
        if(it->second.first->pointsVector().isEmpty()) return 0;
        return it->second.first->pointsVector().first().y();
    }
    else return 0;
}

void ChartHandler::initializeKlines(QJsonArray &array)
{
    foreach (const QJsonValue & value, array) {
        QJsonArray kline = value.toArray();
        QCandlestickSet *candle = new QCandlestickSet;

        candle->setTimestamp(kline[0].toDouble());
        candle->setOpen(kline[1].toString().toDouble());
        candle->setHigh(kline[2].toString().toDouble());
        candle->setLow(kline[3].toString().toDouble());
        candle->setClose(kline[4].toString().toDouble());

        _priceSeries->append(candle);
    }
    _timeAxis->setRange(QDateTime::fromMSecsSinceEpoch(_priceSeries->sets().last()->timestamp()).addSecs(-14100), QDateTime::fromMSecsSinceEpoch(_priceSeries->sets().last()->timestamp()).addSecs(300));
    setYAxisRange();
}

void ChartHandler::handleKline(QJsonObject &kline)
{
    if(_priceSeries->sets().isEmpty() || (_priceSeries->sets().last()->timestamp() != kline["t"].toDouble())){
        QCandlestickSet *candle = new QCandlestickSet;
        candle->setTimestamp(kline["t"].toDouble());
        candle->setOpen(kline["o"].toString().toDouble());
        candle->setHigh(kline["h"].toString().toDouble());
        candle->setLow(kline["l"].toString().toDouble());
        candle->setClose(kline["c"].toString().toDouble());
        _priceSeries->append(candle);
    }
    else if(_priceSeries->sets().last()->timestamp() == kline["t"].toDouble()){
        QCandlestickSet *lastCandlePtr = _priceSeries->sets().last();
        lastCandlePtr->setOpen(kline["o"].toString().toDouble());
        lastCandlePtr->setHigh(kline["h"].toString().toDouble());
        lastCandlePtr->setLow(kline["l"].toString().toDouble());
        lastCandlePtr->setClose(kline["c"].toString().toDouble());
    }
    setYAxisRange();
}

void ChartHandler::setYAxisRange()
{
    double min = 1000000;
    double max = 0;

    foreach (QCandlestickSet *candle, _priceSeries->sets()) {
        if (candle->timestamp() > _timeAxis->min().toMSecsSinceEpoch() && candle->timestamp() < _timeAxis->max().toMSecsSinceEpoch()) {
            if (candle->high() > max) max = candle->high();
            if (candle->low() < min) min = candle->low();
        }
    }

    for (auto serie : _seriesList){
        if(!serie.second.first->pointsVector().isEmpty()){
            if(serie.second.first->pointsVector().first().y() > max) max = serie.second.first->pointsVector().first().y();
            if(serie.second.first->pointsVector().first().y() < min) min = serie.second.first->pointsVector().first().y();
        }
    }

    _priceAxis->setRange(min * (1-_Ymultiplier), max * (1+_Ymultiplier));
}

void ChartHandler::setSerieMovable(Utils::SeriesEnum name, bool movable)
{
    if(name == Utils::lineSeriesEnd) return;
    auto it = _seriesList.find(name);
    if(it != _seriesList.end()){
        it->second.second = movable;
    }
}

QLineSeries* ChartHandler::serie(Utils::SeriesEnum name)
{
    return _seriesList.at(name).first;
}

QCandlestickSeries* ChartHandler::candles()
{
    return _priceSeries;
}

void ChartHandler::setSeriePrice(Utils::SeriesEnum name, double price)
{
    serie(name)->clear();

    serie(name)->append(_priceSeries->sets().first()->timestamp(), price);
    serie(name)->append(_priceSeries->sets().last()->timestamp() + 86400000, price);
}

void ChartHandler::reset()
{
    _timeAxis->setRange(QDateTime::fromMSecsSinceEpoch(_priceSeries->sets().last()->timestamp()).addSecs(-14100), QDateTime::fromMSecsSinceEpoch(_priceSeries->sets().last()->timestamp()).addSecs(60));
    _Ymultiplier = 0.001;
    setYAxisRange();
}
