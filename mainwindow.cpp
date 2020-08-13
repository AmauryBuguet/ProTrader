#include <QSignalSpy>

#include "mainwindow.hpp"
#include "statswindow.hpp"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), _wsHandler(this)
{
    _orderList = new OrderList(this);
    _orderList->show();

    _seriesBtnList = new SeriesBtnList(this);

    _orderTypesList.insert({Utils::LIMIT,OrderType({Utils::SL,Utils::ENTRY},Utils::ENTRY,Utils::lineSeriesEnd)});
    _orderTypesList.insert({Utils::MARKET,OrderType({Utils::SL},Utils::SL,Utils::TP1)});
    _orderTypesList.insert({Utils::STOP,OrderType({Utils::SL,Utils::ENTRY,Utils::TRIGGER},Utils::lineSeriesEnd,Utils::TRIGGER)});
    _orderTypesList.insert({Utils::STOP_MARKET,OrderType({Utils::SL,Utils::TRIGGER},Utils::lineSeriesEnd,Utils::TRIGGER)});
    _orderTypesList.insert({Utils::TAKE_PROFIT,OrderType({Utils::TP1},Utils::lineSeriesEnd,Utils::TP1)});

    _chartHandler = new ChartHandler(this);
    _chartHandler->setMinimumSize(640, 420);
    _resetChartButton = new QPushButton("reset",this);

    _5mButton = new QPushButton("5m", this);
    _1mButton = new QPushButton("1m", this);

    _balanceLabel = new QLabel("Balance : -- USDT", this);
    _overallStatsLabel = new QLabel(this);
    _dailyStatsLabel = new QLabel(this);
    _amountRiskedSlider = new QSlider(Qt::Horizontal, this);
    _amountRiskedSlider->setRange(0, 200);
    _amountRiskedSlider->setValue(100);
    _estimationLabel = new QLabel(this);
    _estimationLabel->setMinimumSize(250, 320);
    _estimationLabel->setMaximumSize(350, 375);
    _estimationLabel->setFont(QFont("Arial", 12));
    _estimationLabel->setStyleSheet("QLabel { background-color : black; color : LawnGreen; }");
    _shortSetupButton = new QPushButton("SHORT", this);
    _shortSetupButton->setMaximumWidth(150);
    _longSetupButton = new QPushButton("LONG", this);
    _longSetupButton->setMaximumWidth(150);
    _placeOrderButton = new QPushButton("PLACE ORDER", this);
    _placeOrderButton->setMinimumSize(160, 120);
    _placeOrderButton->setMaximumSize(350, 200);
    _placeOrderButton->setEnabled(false);
    _dailyStatsButton = new QPushButton("DAILY STATS", this);
    _overallStatsButton = new QPushButton("OVERALL STATS", this);

    QGridLayout *setupLayout = new QGridLayout();
    setupLayout->addWidget(_balanceLabel, 0, 0, 1, 2);
    setupLayout->addWidget(_overallStatsButton, 1, 0);
    setupLayout->addWidget(_dailyStatsButton, 1, 1);
    setupLayout->addWidget(_overallStatsLabel, 2, 0);
    setupLayout->addWidget(_dailyStatsLabel, 2, 1);
    setupLayout->addWidget(_seriesBtnList, 3, 0, 1, 2);
    setupLayout->addWidget(_amountRiskedSlider, 4, 0, 1, 2);
    setupLayout->addWidget(_shortSetupButton, 5, 0);
    setupLayout->addWidget(_longSetupButton, 5, 1);
    setupLayout->addWidget(_estimationLabel, 6, 0, 1, 2);
    setupLayout->addWidget(_placeOrderButton, 7, 0, 1, 2);
    setupLayout->setSpacing(20);

    QHBoxLayout *timeFramesLayout = new QHBoxLayout();
    timeFramesLayout->addWidget(_1mButton);
    timeFramesLayout->addWidget(_5mButton);

    _mainLayout = new QGridLayout();
    _mainLayout->setColumnStretch(0,3);
    _mainLayout->setColumnStretch(1,1);
    _mainLayout->addWidget(_chartHandler, 0, 0);
    _mainLayout->addWidget(_resetChartButton, 0, 0, Qt::AlignRight | Qt::AlignBottom);
    _mainLayout->addLayout(timeFramesLayout, 0, 0, Qt::AlignLeft | Qt::AlignTop);
    _mainLayout->addLayout(setupLayout, 0, 1);
    _mainLayout->addWidget(_orderList, 1, 0, 1, 2);
    _mainLayout->setSpacing(20);

    QWidget *widget = new QWidget();
    widget->setLayout(_mainLayout);
    setCentralWidget(widget);

    connect(&_binance, &ApiHandler::initialKlineArrayReady, _chartHandler, &ChartHandler::initializeKlines);
    connect(&_binance, &ApiHandler::listenKeyCreated, &_wsHandler, &WsHandler::setup);
    connect(&_binance, &ApiHandler::orderUpdate, this, &MainWindow::handleOrderUpdate);
    connect(&_binance, &ApiHandler::balanceReady, this, &MainWindow::setBalance);

    connect(&_wsHandler, &WsHandler::receivedKline, _chartHandler, &ChartHandler::handleKline);
    connect(&_wsHandler, &WsHandler::bidaskChanged, this, &MainWindow::fillPositionEstimate);
    connect(&_wsHandler, &WsHandler::orderUpdate, this, &MainWindow::handleOrderUpdate);
    connect(&_wsHandler, &WsHandler::accountUpdate, this, &MainWindow::setBalance);

    connect(_chartHandler, &ChartHandler::lineMoved, this, &MainWindow::fillPositionEstimate);

    connect(_seriesBtnList, &SeriesBtnList::buttonChecked, [this](Utils::SeriesEnum serie){ _chartHandler->setSeriePrice(serie, _chartHandler->candles()->sets().last()->close()); fillPositionEstimate(); });
    connect(_seriesBtnList, &SeriesBtnList::buttonUnchecked, [this](Utils::SeriesEnum serie){ _chartHandler->clearSerie(serie); fillPositionEstimate(); });

    connect(_orderList, &OrderList::cancelOrder, &_binance, &ApiHandler::cancelOrder);
    connect(_orderList, &OrderList::modifyOrder, this, &MainWindow::handleModifyOrder);

    connect(_amountRiskedSlider, &QSlider::valueChanged, this, &MainWindow::fillPositionEstimate);
    connect(_shortSetupButton, &QPushButton::clicked, this, &MainWindow::quickShortSetup);
    connect(_longSetupButton, &QPushButton::clicked, this, &MainWindow::quickLongSetup);
    connect(_resetChartButton, &QPushButton::clicked, _chartHandler, &ChartHandler::reset);
    connect(_placeOrderButton, &QPushButton::clicked, this, &MainWindow::handlePlaceOrder);
    connect(_overallStatsButton, &QPushButton::clicked, [this](){ openStatsChart(); });
    connect(_dailyStatsButton, &QPushButton::clicked, [this](){ openStatsChart(true); });

    connect(_1mButton, &QPushButton::clicked, [this](){ changeInterval("1m"); });
    connect(_5mButton, &QPushButton::clicked, [this](){ changeInterval("5m"); });

    fillStats();
    getApiKeys();
    _binance.getBalance();
    _binance.getLastCandles();
    _binance.createListenKey();
    _binance.getOpenOrders();
}

void MainWindow::changeInterval(QString newInterval)
{
    _estimationEnabled = false;
    _wsHandler.unsubscribe();
    INTERVAL = newInterval;
    _chartHandler->candles()->clear();
    QSignalSpy spy(&_binance, &ApiHandler::initialKlineArrayReady);
    _binance.getLastCandles();
    spy.wait(3000);
    _wsHandler.subscribe();
    QTimer::singleShot(1000, [this](){_estimationEnabled = true;});
}

void MainWindow::setBalance(double value)
{
    _balance = value;
    _balanceLabel->setText("Balance : " + QString::number(_balance) + " USDT");
    emit balanceUpdated();
}

void MainWindow::handleOrderUpdate(QJsonObject &order)
{
//    qDebug().noquote() << order;
    Utils::SeriesEnum serie = Utils::stringToSerieEnum(order["c"].toString());
    if(order["setup"] == "true"){
        _orderList->addOrder(order);
    }
    else if(order["X"] == "NEW"){
        _orderList->addOrder(order);
        _chartHandler->setSerieMovable(serie, false);
    }
    else if(order["X"] == "CANCELED" || order["X"] == "EXPIRED"){
        _orderList->removeOrder(order["c"].toString());
        _chartHandler->setSerieMovable(serie, true);
        if(serie == Utils::SL) emit slCanceled(order["S"].toString());
        else if(serie == Utils::TP1) emit tpCanceled(order["S"].toString(), order["q"].toString());
    }
    else if(order["X"] == "FILLED"){
        _orderList->removeOrder(order["c"].toString());
        _chartHandler->setSerieMovable(serie, true);
        if(order["c"] == "ENTRY"){
            QString price = "0";
            if(order["ap"].toString() != "0") price = order["ap"].toString();
            else if(order["sp"].toString() != "0") price = order["sp"].toString();
            else qDebug() << "could't get entry price from order update :" << order;
            _chartHandler->setSeriePrice(Utils::REAL_ENTRY, price.toDouble());
            _position = new Position(Utils::stringToOrderTypeEnum(order["o"].toString()), order["z"].toString(), Utils::stringtoSideEnum(order["S"].toString()));
            _chartHandler->serie(Utils::ENTRY)->clear();

            _binance.placeSL(Utils::inverseSideString(order["S"].toString()), QString::number(_chartHandler->serie(Utils::SL)->pointsVector().first().y(), 'f', _chartHandler->precision()));

            if(_chartHandler->seriePrice(Utils::TP1) != 0){
                _binance.placeTP(Utils::inverseSideString(order["S"].toString()), order["z"].toString(), QString::number(_chartHandler->serie(Utils::TP1)->pointsVector().first().y(), 'f', _chartHandler->precision()));
            }
        }
        else {
            _estimationEnabled = false;
            _chartHandler->setSeriePrice(Utils::ENTRY, _chartHandler->serie(Utils::REAL_ENTRY)->pointsVector().first().y());
            delete _position;
            _position = nullptr;
            qDebug().noquote() << order["c"].toString() << "hit";
            qDebug().noquote() << "Realized PnL on this trade :" << order["rp"].toString() << "USDT";
            QTimer::singleShot(500, [this](){ updateBalanceHistory(QDateTime::currentMSecsSinceEpoch(), _balance); });
            _chartHandler->serie(Utils::REAL_ENTRY)->clear();
            QTimer::singleShot(500, [this](){ _estimationEnabled = true; });
        }
    }
    else if(order["X"] == "PARTIALLY_FILLED"){
        if(order["c"] == "ENTRY"){
            qDebug() << "entry partially filled";
        }
        if(order["c"] == "TP1"){
            qDebug() << "TP partially filled";
        }
    }
}

void MainWindow::handleModifyOrder(QString name)
{
    if(name != "SL" && name != "TP1"){
        if(name == "Order" && _position != nullptr) _binance.marketClosePosition(_position->volume(), _position->side());
    }
    else {
        // set name movable
        _chartHandler->setSerieMovable(Utils::stringToSerieEnum(name), true);
        disconnect(_placeOrderButton, &QPushButton::clicked, this, &MainWindow::handlePlaceOrder);
        connect(_placeOrderButton, &QPushButton::clicked, this, &MainWindow::modifyOrder);
        _placeOrderButton->setText("MODIFY " + name);
    }
}

void MainWindow::modifyOrder()
{
    QString name = _placeOrderButton->text().mid(7);
    _binance.cancelOrder(name);
    if(name == "SL"){
        QString newPrice = QString::number(_chartHandler->serie(Utils::SL)->pointsVector().first().y(), 'f', 3);
        QMetaObject::Connection * const connection = new QMetaObject::Connection;
        *connection = connect(this, &MainWindow::slCanceled, [this, newPrice, connection](QString side){
            _binance.placeSL(side, newPrice);
            QObject::disconnect(*connection);
            delete connection;
        });
    }
    else if(name == "TP1"){
        QString newPrice = QString::number(_chartHandler->serie(Utils::TP1)->pointsVector().first().y(), 'f', 3);
        QMetaObject::Connection * const connection = new QMetaObject::Connection;
        *connection = connect(this, &MainWindow::tpCanceled, [this, newPrice, connection](QString side, QString volume){
            _binance.placeTP(side, volume, newPrice);
            QObject::disconnect(*connection);
            delete connection;
        });
    }
    _placeOrderButton->setText("PLACE ORDER");
    disconnect(_placeOrderButton, &QPushButton::clicked, this, &MainWindow::modifyOrder);
    connect(_placeOrderButton, &QPushButton::clicked, this, &MainWindow::handlePlaceOrder);
}

void MainWindow::handlePlaceOrder()
{
    _binance.createOrder(_currentOrder);
}

Utils::OrderTypesEnum MainWindow::currentPossibleType()
{
    Utils::OrderTypesEnum realType = Utils::Other;
    for(auto type : _orderTypesList){
        bool thisOne = true;
        QList<Utils::SeriesEnum> list = type.second.linesInOrder();
        for(uint i=0; i< Utils::TP1; i++){
            if(_chartHandler->seriePrice(Utils::SeriesEnum(i)) == 0 && list.contains(Utils::SeriesEnum(i))) thisOne = false;
            if(_chartHandler->seriePrice(Utils::SeriesEnum(i)) != 0 && !list.contains(Utils::SeriesEnum(i))) thisOne = false;
        }
        if(thisOne){
            realType = type.first;
            break;
        }
    }
    return realType;
}

std::pair<Utils::OrderTypesEnum, Utils::SidesEnum> MainWindow::currentPossibleOrder()
{
    Utils::OrderTypesEnum realType = currentPossibleType();

    // check also current price bounds

    if(realType == Utils::Other) return {Utils::Other, Utils::Undefined};
    QList<Utils::SeriesEnum> *displayedSeries = new QList<Utils::SeriesEnum>;
    QList<Utils::SeriesEnum> list = _orderTypesList.at(realType).linesInOrder();

    for(uint i=0; i< Utils::lineSeriesEnd; i++){
        if(_chartHandler->seriePrice(Utils::SeriesEnum(i)) != 0) displayedSeries->push_back(Utils::SeriesEnum(i));
    }
    if(displayedSeries->contains(Utils::TP1)) list.push_back(Utils::TP1);
    if(displayedSeries->contains(Utils::TP2)) list.push_back(Utils::TP2);

    std::sort(displayedSeries->begin(), displayedSeries->end(), [=](Utils::SeriesEnum serieA, Utils::SeriesEnum serieB){ return _chartHandler->seriePrice(serieA) < _chartHandler->seriePrice(serieB);});
    if(list == *displayedSeries
            && ((_orderTypesList.at(realType).lowerBound() == Utils::lineSeriesEnd) || (_chartHandler->candles()->sets().last()->close() > _chartHandler->seriePrice(_orderTypesList.at(realType).lowerBound())))
            && ((_chartHandler->seriePrice(_orderTypesList.at(realType).upperBound()) == 0) || (_orderTypesList.at(realType).upperBound() == Utils::lineSeriesEnd) || (_chartHandler->candles()->sets().last()->close() < _chartHandler->seriePrice(_orderTypesList.at(realType).upperBound()))))
        return {realType, Utils::BUY};

    std::sort(displayedSeries->begin(), displayedSeries->end(), [=](Utils::SeriesEnum serieA, Utils::SeriesEnum serieB){ return _chartHandler->seriePrice(serieA) > _chartHandler->seriePrice(serieB);});
    if(list == *displayedSeries
            && ((_orderTypesList.at(realType).lowerBound() == Utils::lineSeriesEnd) || (_chartHandler->candles()->sets().last()->close() < _chartHandler->seriePrice(_orderTypesList.at(realType).lowerBound())))
            && ((_chartHandler->seriePrice(_orderTypesList.at(realType).upperBound()) == 0) || (_chartHandler->candles()->sets().last()->close() > _chartHandler->seriePrice(_orderTypesList.at(realType).upperBound()))))
        return {realType, Utils::SELL};

    return {realType, Utils::Undefined};
}

void MainWindow::fillPositionEstimate()
{
    if(!_estimationEnabled) return;
    if(_position != nullptr){
        if(!_placeOrderButton->isEnabled()) _placeOrderButton->setEnabled(true);
        Utils::SidesEnum side = _position->side();
        double pnl = 0;
        if(_chartHandler->serie(Utils::REAL_ENTRY)->pointsVector().isEmpty()) return;
        double entry = _chartHandler->serie(Utils::REAL_ENTRY)->pointsVector().first().y();
        double volume = _position->volume().toDouble();
        double sl = _chartHandler->serie(Utils::SL)->pointsVector().first().y();
        double tp = _chartHandler->serie(Utils::TP1)->pointsVector().first().y();
        double risk;
        double reward = 0;
        if(_position->type() != Utils::LIMIT){
            if(side == Utils::BUY){
                pnl = volume*(_wsHandler.getBid()*0.9996-entry*1.0004);
                risk = volume*(sl*0.9996-entry*1.0004);
                if(tp != 0) reward = volume*(tp*0.9998-entry*1.0004);
            }
            else{
                pnl = volume*(entry*0.9996-_wsHandler.getAsk()*1.0004);
                risk = volume*(entry*0.9996-sl*1.0004);
                if(tp != 0) reward = volume*(entry*0.9996-tp*1.0002);
            }
        }
        else {
            if(side == Utils::BUY){
                pnl = volume*(_wsHandler.getBid()*0.9996-entry*1.0002);
                risk = volume*(sl*0.9996-entry*1.0002);
                if(tp != 0) reward = volume*(tp*0.9998-entry*1.0002);
            }
            else{
                pnl = volume*(entry*0.9998-_wsHandler.getAsk()*1.0004);
                risk = volume*(entry*0.9998-sl*1.0004);
                if(tp != 0) reward = volume*(entry*0.9998-tp*1.0002);
            }
        }
        QString text = "        Current Position\n\n";
        text.append(" Type : " + Utils::orderTypeEnumToString(_position->type()) + "\n");
        text.append(" Side : " + Utils::sideEnumToString(side) + "\n");
        text.append(" PnL : " + QString::number(pnl) + "\n");
        text.append(" Risk : " + QString::number(risk) + "\n");
        text.append(" Reward : " + (tp ? QString::number(reward) : "--") + "\n");
        text.append(" Volume : " + _position->volume() + " XTZ\n");
        text.append(" Entry : " + QString::number(entry) + "\n");
        text.append(" SL : " + QString::number(sl) + "\n");
        text.append(" TP : " + (tp ? QString::number(tp) : "--") + "\n");
        _estimationLabel->setText(text);
        return;
    }

    auto pair = currentPossibleOrder();
    QString sideString;
    QString volumeXTZString;
    QString rewardString;
    QString ratioString;
    QString entryString;
    QString slString;
    QString tpString;
    double risk = double(_amountRiskedSlider->value())/10.0;
    if(pair.second == Utils::Undefined){
        if(_placeOrderButton->text().contains("MODIFY")) _placeOrderButton->setEnabled(true);
        else _placeOrderButton->setEnabled(false);
        volumeXTZString = "--";
        rewardString = "--";
        ratioString = "--";
        entryString = "--";
        slString = "--";
    }
    else {
        _placeOrderButton->setEnabled(true);
        double entry;
        double stopLoss = _chartHandler->serie(Utils::SL)->pointsVector().first().y();
        risk = double(_amountRiskedSlider->value())/10.0;
        if(!_chartHandler->serie(Utils::ENTRY)->pointsVector().isEmpty()){
            entry = _chartHandler->serie(Utils::ENTRY)->pointsVector().first().y();
        }
        else if(!_chartHandler->serie(Utils::TRIGGER)->pointsVector().isEmpty()){
            entry = _chartHandler->serie(Utils::TRIGGER)->pointsVector().first().y();
        }
        else {
            if(pair.second == Utils::BUY) entry = _wsHandler.getAsk();
            else entry = _wsHandler.getBid();
        }
        double volume;
        if(pair.first == Utils::MARKET || pair.first == Utils::STOP_MARKET){
            if(pair.second == Utils::BUY) volume = (-1*risk)/(stopLoss*0.9996-entry*1.0004);
            else volume = (-1*risk)/(entry*0.9996-stopLoss*1.0004);
        }
        else {
            if(pair.second == Utils::BUY) volume = (-1*risk)/(stopLoss*0.9996-entry*1.0002);
            else volume = (-1*risk)/(entry*0.9998-stopLoss*1.0004);
        }
        double takeProfit = _chartHandler->seriePrice(Utils::TP1);
        double reward = 0;
        rewardString = "--";
        ratioString = "--";
        if(takeProfit != 0){
            if(pair.first == Utils::MARKET || pair.first == Utils::STOP_MARKET){
                if(pair.second == Utils::BUY) reward = volume*(takeProfit*0.9998-entry*1.0004);
                else reward = volume*(entry*0.9996-takeProfit*1.0002);
            }
            else {
                if(pair.second == Utils::BUY) reward = volume*(takeProfit*0.9998-entry*1.0002);
                else reward = volume*(entry*0.9998-takeProfit*1.0002);
            }
            rewardString = QString::number(reward,'f', 2);
            ratioString = QString::number(reward/risk);
        }

        volumeXTZString = QString::number(volume, 'f', 1);
        entryString = QString::number(entry, 'f', _chartHandler->precision());
        slString = QString::number(stopLoss, 'f', _chartHandler->precision());
        tpString = QString::number(takeProfit, 'f', _chartHandler->precision());
        _currentOrder.type = pair.first;
        _currentOrder.side = pair.second;
        _currentOrder.price = entryString;
        _currentOrder.volume = volumeXTZString;
        if(!_chartHandler->serie(Utils::TRIGGER)->pointsVector().isEmpty()) _currentOrder.trigger = QString::number(_chartHandler->serie(Utils::TRIGGER)->pointsVector().first().y(), 'f', _chartHandler->precision());
    }
    QString text = "        Position Estimation\n\n";
    text.append(" Type : " + Utils::orderTypeEnumToString(pair.first) + "\n");
    text.append(" Side : " + Utils::sideEnumToString(pair.second) + "\n");
    text.append(" Risk : " + QString::number(risk) + " USD\n");
    text.append(" Reward : " + rewardString + " USD\n");
    text.append(" Ratio : " + ratioString + "\n");
    text.append(" Volume : " + volumeXTZString + " XTZ\n");
    text.append(" Entry : " + entryString + "\n");
    text.append(" SL : " + slString + "\n");
    text.append(" TP : " + tpString + "\n");
    _estimationLabel->setText(text);
}

void MainWindow::getApiKeys()
{
    int success = 0;
    QFile fichier( ":/keys.ini" );
    if( fichier.open( QIODevice::ReadOnly | QIODevice::Text ) ){
        QTextStream flux( &fichier );
        do {
            QString line = flux.readLine();
            QRegularExpressionMatch rem = QRegularExpression( QString( "^(?<varname>(key|secret|token))=\"(?<value>(.*?))\"$" ),
                                                             QRegularExpression::CaseInsensitiveOption ).match( line );
            if (rem.hasMatch()) {
                if( rem.captured( "varname" ) == "key" ){
                    API_KEY = rem.captured("value");
                    success++;
                }
                else if( rem.captured( "varname" ) == "secret" ){
                    API_SECRET = rem.captured("value");
                    success++;
                }
                else if( rem.captured( "varname" ) == "token" ){
                    // TODO
                }
            }
        } while ( !flux.atEnd() );
    }
    else {
        qDebug() << "Couldn't open keys.ini file";
        success = 0;
    }
    if(success < 2){
        qDebug() << "Can't launch ProTrader, api key and/or secret not found";
        qApp->exit();
    }
    else qDebug() << "API keys set";
}

void MainWindow::openStatsChart(bool daily)
{
    QJsonArray balanceHistory = getBalanceHistory();

    QString previousBalance = "";
    if(daily){
        foreach(const QJsonValue &val, balanceHistory){
            QJsonObject obj = val.toObject();
            if(obj["timestamp"].toString().toDouble() > QDateTime::currentDateTime().addDays(-1).toMSecsSinceEpoch()){
                QJsonObject item;
                item.insert("timestamp", QString::number(obj["timestamp"].toString().toDouble()-3600000));
                item.insert("balance", previousBalance);
                balanceHistory.prepend(item);
                break;
            }
            previousBalance = obj["balance"].toString();
            balanceHistory.removeFirst();
        }
    }

    StatsWindow *stats = new StatsWindow(balanceHistory);
    stats->show();
}

QJsonArray MainWindow::getBalanceHistory(bool remove)
{
    QFile logFile(QApplication::applicationDirPath() + "/balanceHistory.json");

    logFile.open(QIODevice::ReadOnly);
    QJsonArray fileArray = QJsonDocument::fromJson(logFile.readAll()).array();
    logFile.close();
    if(remove) logFile.remove();
    return fileArray;
}

void MainWindow::fillStats()
{
    QJsonArray balanceHistory = getBalanceHistory();

    bool first = true;
    bool firstOfTheDay = true;
    double startBalance = 0;
    double previousBalance = 0;
    int nbEven = 0;
    int nbWin = 0;
    int nbLoss = 0;
    double startBalanceOfTheDay = 0;
    int nbEvenOfTheDay = 0;
    int nbWinOfTheDay = 0;
    int nbLossOfTheDay = 0;
    foreach(const QJsonValue &val, balanceHistory){
        QJsonObject obj = val.toObject();
        if(first){
            startBalance = obj["balance"].toString().toDouble();
            first = false;
        }
        else {
            double diff = obj["balance"].toString().toDouble() - previousBalance;
            if(diff >= 3){
                nbWin++;
            }
            else if(diff < -3){
                nbLoss++;
            }
            else nbEven++;
        }
        if(obj["timestamp"].toString().toDouble() > QDateTime::currentDateTime().addDays(-1).toMSecsSinceEpoch()){
            if(firstOfTheDay){
                startBalanceOfTheDay = previousBalance;
                firstOfTheDay = false;
            }
            double diff = obj["balance"].toString().toDouble() - previousBalance;
            if(diff >= 3){
                nbWinOfTheDay++;
            }
            else if(diff < -3){
                nbLossOfTheDay++;
            }
            else nbEvenOfTheDay++;
        }
        previousBalance = obj["balance"].toString().toDouble();
    }

    QString textStats = "\n";
    textStats.append("Wins : " + QString::number(nbWin) + "\n");
    textStats.append("Losses : " + QString::number(nbLoss) + "\n");
    textStats.append("Breakevens : " + QString::number(nbEven) + "\n");
    textStats.append("Change : " + QString::number(previousBalance - startBalance) + " USDT");
    QPalette pal = _overallStatsLabel->palette();
    pal.setColor(QPalette::WindowText, previousBalance > startBalance ? Qt::darkGreen : Qt::red);
    _overallStatsLabel->setPalette(pal);
    _overallStatsLabel->setText(textStats);

    QString textDailyStats = "\n";
    textDailyStats.append("Wins : " + QString::number(nbWinOfTheDay) + "\n");
    textDailyStats.append("Losses : " + QString::number(nbLossOfTheDay) + "\n");
    textDailyStats.append("Breakevens : " + QString::number(nbEvenOfTheDay) + "\n");
    textDailyStats.append("Change : " + QString::number(previousBalance - startBalanceOfTheDay) + " USDT");
    pal = _overallStatsLabel->palette();
    pal.setColor(QPalette::WindowText, previousBalance > startBalanceOfTheDay ? Qt::darkGreen : Qt::red);
    _dailyStatsLabel->setPalette(pal);
    _dailyStatsLabel->setText(textDailyStats);
}

void MainWindow::updateBalanceHistory(qint64 timestamp, double newBalance)
{
    QJsonArray balanceHistory = getBalanceHistory(true);
    QJsonObject newEntry;
    newEntry.insert("timestamp", QString::number(timestamp, 'f', 0));
    newEntry.insert("balance", QString::number(newBalance, 'f', 2));
    balanceHistory.append(newEntry);

    QFile logFile(QApplication::applicationDirPath() + "/balanceHistory.json");
    logFile.open(QIODevice::WriteOnly);
    logFile.write(QJsonDocument(balanceHistory).toJson());
    logFile.close();

    fillStats();
}

void MainWindow::quickShortSetup()
{
    Utils::OrderTypesEnum realType = currentPossibleType();

    if(realType != Utils::Other){
        _chartHandler->setSeriePrice(Utils::SL, _chartHandler->candles()->sets().last()->close()*1.01);
        if(_chartHandler->seriePrice(Utils::TP1) != 0) _chartHandler->setSeriePrice(Utils::TP1, _chartHandler->candles()->sets().last()->close()/1.02);
        if(_chartHandler->seriePrice(Utils::TP2) != 0)_chartHandler->setSeriePrice(Utils::TP2, _chartHandler->candles()->sets().last()->close()/1.03);
        if(_orderTypesList.at(realType).linesInOrder().contains(Utils::ENTRY)) _chartHandler->setSeriePrice(Utils::ENTRY, _chartHandler->candles()->sets().last()->high());
        if(_orderTypesList.at(realType).linesInOrder().contains(Utils::TRIGGER)) _chartHandler->setSeriePrice(Utils::TRIGGER, _chartHandler->candles()->sets().last()->low());
    }
    _chartHandler->setYAxisRange();
    fillPositionEstimate();
}

void MainWindow::quickLongSetup()
{
    Utils::OrderTypesEnum realType = currentPossibleType();

    if(realType != Utils::Other){
        _chartHandler->setSeriePrice(Utils::SL, _chartHandler->candles()->sets().last()->close()/1.01);
        if(_chartHandler->seriePrice(Utils::TP1) != 0)_chartHandler->setSeriePrice(Utils::TP1, _chartHandler->candles()->sets().last()->close()*1.02);
        if(_chartHandler->seriePrice(Utils::TP2) != 0)_chartHandler->setSeriePrice(Utils::TP2, _chartHandler->candles()->sets().last()->close()*1.03);
        if(_orderTypesList.at(realType).linesInOrder().contains(Utils::ENTRY)) _chartHandler->setSeriePrice(Utils::ENTRY, _chartHandler->candles()->sets().last()->low());
        if(_orderTypesList.at(realType).linesInOrder().contains(Utils::TRIGGER)) _chartHandler->setSeriePrice(Utils::TRIGGER, _chartHandler->candles()->sets().last()->high());
    }
    _chartHandler->setYAxisRange();
    fillPositionEstimate();
}

MainWindow::~MainWindow()
{
    if(_position != nullptr) delete _position;
}

