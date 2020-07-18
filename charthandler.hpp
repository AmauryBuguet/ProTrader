#ifndef CHARTHANDLER_HPP
#define CHARTHANDLER_HPP

// C++ includes
#include <map>

// Qt includes
#include <QtCharts>

// Projets includes
#include "globals.hpp"

class ChartHandler : public QChartView
{
    Q_OBJECT
public:
    ChartHandler(QWidget *parent = nullptr);
    QLineSeries* serie(Utils::SeriesEnum name);
    void setSeriePrice(Utils::SeriesEnum name, double price);
    void setSerieMovable(Utils::SeriesEnum name, bool movable);
    QCandlestickSeries* candles();
    int precision() { return _precision; }

signals:
    void lineMoved();

public slots:
    void initializeKlines(QJsonArray &array);
    void handleKline(QJsonObject &kline);
    void setYAxisRange();
    void reset();

private:
    double _x_pos = 0;
    double _Ymultiplier = 0.001;
    int _precision = 3;
    QChart *_chart;
    QCandlestickSeries *_priceSeries;
    QValueAxis *_priceAxis;
    QDateTimeAxis *_timeAxis;
    std::map<Utils::SeriesEnum, std::pair<QLineSeries*, bool> > _seriesList;
    Utils::SeriesEnum _clickedSerie = Utils::lineSeriesEnd;
    bool _oneSerieClicked = false;

protected:
    void mousePressEvent(QMouseEvent* event) Q_DECL_OVERRIDE
    {
        if (event->button() == Qt::LeftButton) {
            auto const widgetPos = event->localPos();
            auto const scenePos = mapToScene(QPoint(static_cast<int>(widgetPos.x()), static_cast<int>(widgetPos.y())));
            auto const coord = chart()->mapToValue(chart()->mapFromScene(scenePos));
            for (auto serie : _seriesList){
                if(serie.second.second && !serie.second.first->pointsVector().isEmpty() && coord.y() < serie.second.first->pointsVector().first().y()*1.0003 && coord.y() > serie.second.first->pointsVector().first().y()/1.0003){
                    _oneSerieClicked = true;
                    _clickedSerie = serie.first;
                    break;
                }
                _oneSerieClicked = false;
            }
            _x_pos = event->x();
        }
    }

    void mouseMoveEvent(QMouseEvent* event) Q_DECL_OVERRIDE
    {
        if (event->buttons() & Qt::LeftButton) {
            if(_oneSerieClicked){
                auto const widgetPos = event->localPos();
                auto const scenePos = mapToScene(QPoint(static_cast<int>(widgetPos.x()), static_cast<int>(widgetPos.y())));
                auto const coord = chart()->mapToValue(chart()->mapFromScene(scenePos));
                QString coordTrunc = QString::number(coord.y(), 'f', _precision);
                setSeriePrice(_clickedSerie, coordTrunc.toDouble());
                emit lineMoved();
            }
            else {
                this->chart()->scroll(_x_pos - event->x(), 0);
                _x_pos = event->x();
            }
            setYAxisRange();
        }
    }

    void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE
    {
        if(QApplication::keyboardModifiers() & Qt::ControlModifier){
            _Ymultiplier = _Ymultiplier+double(-1.0*event->angleDelta().y())/120000.0;
            event->accept();
            setYAxisRange();
        }
        else {
            qreal spread = _timeAxis->max().toSecsSinceEpoch() - _timeAxis->min().toSecsSinceEpoch();
            _timeAxis->setRange(_timeAxis->min().addSecs(spread/5000 * event->angleDelta().y()), _timeAxis->max().addSecs(-spread/5000 * (event->angleDelta().y())));
            event->accept();
            setYAxisRange();
        }
    }
};

#endif // CHARTHANDLER_HPP
