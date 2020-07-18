#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <map>

#include <QMainWindow>

#include "apihandler.hpp"
#include "charthandler.hpp"
#include "globals.hpp"
#include "orderhandler.hpp"
#include "wshandler.hpp"

class OrderType
{
public:
    OrderType(QList<Utils::SeriesEnum> linesInOrder, Utils::SeriesEnum lowerBound, Utils::SeriesEnum upperBound) :
        _linesInOrder(linesInOrder), _lowerBound(lowerBound), _upperBound(upperBound) {}
    QList<Utils::SeriesEnum>* linesInOrder() { return &_linesInOrder; }
    Utils::SeriesEnum lowerBound() { return _lowerBound; }
    Utils::SeriesEnum upperBound() { return _upperBound; }

private:
    QList<Utils::SeriesEnum> _linesInOrder;
    Utils::SeriesEnum _lowerBound;
    Utils::SeriesEnum _upperBound;
};

class Position
{
public:
    Position(QString type, QString volume, Utils::SidesEnum side) : _type(type), _volume(volume), _side(side) {}
    QString type() { return _type; }
    QString volume() { return _volume; }
    Utils::SidesEnum side() { return _side; }

private:
    QString _type;
    QString _volume;
    Utils::SidesEnum _side;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void getApiKeys();
    void handleOrderType(const QString text);
    void quickShortSetup();
    void quickLongSetup();
    Utils::SidesEnum currentPossibleOrder();
    void handlePlaceOrder();
    void handleModifyOrder(QString name);
    void modifyOrder();
    QJsonArray getBalanceHistory(bool remove = false);
    void updateBalanceHistory(qint64 timestamp, double newBalance);
    void fillStats();

signals:
    void slCanceled(QString side);
    void tpCanceled(QString side, QString volume);
    void balanceUpdated();

public slots:
    void fillPositionEstimate();
    void handleOrderUpdate(QJsonObject &order);
    void setBalance(double value);

private:
    ChartHandler *_chartHandler;
    WsHandler _wsHandler;
    ApiHandler _binance;
    std::map<QString,OrderType> _orderTypesList;
    OrderData _currentOrder;
    OrderList *_orderList;
    double _balance = 500.0;
    Position *_position = nullptr;
    bool _estimationEnabled = true;

    // Widgets
    QPushButton *_resetChartButton;
    QComboBox *_orderTypeComboBox;
    QSlider *_amountRiskedSlider;
    QPushButton *_shortSetupButton;
    QPushButton *_longSetupButton;
    QPushButton *_placeOrderButton;
    QLabel *_estimationLabel;
    QLabel *_balanceLabel;
    QLabel *_dailyStatsLabel;
    QLabel *_overallStatsLabel;
    QGridLayout *_mainLayout;
};
#endif // MAINWINDOW_HPP