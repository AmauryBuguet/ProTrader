#include <QSignalSpy>

#include "mainwindow.hpp"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), _wsHandler(this)
{
    _orderList = new OrderList(this);
    _orderList->show();

    _orderTypesList.insert({"",OrderType({},Utils::lineSeriesEnd,Utils::lineSeriesEnd)});
    _orderTypesList.insert({"limit",OrderType({Utils::STOP_LOSS,Utils::ENTRY,Utils::TAKE_PROFIT},Utils::ENTRY,Utils::lineSeriesEnd)});
    _orderTypesList.insert({"market",OrderType({Utils::STOP_LOSS,Utils::TAKE_PROFIT},Utils::STOP_LOSS,Utils::TAKE_PROFIT)});
    _orderTypesList.insert({"trigger limit",OrderType({Utils::STOP_LOSS,Utils::ENTRY,Utils::TRIGGER,Utils::TAKE_PROFIT},Utils::lineSeriesEnd,Utils::TRIGGER)});
    _orderTypesList.insert({"trigger market",OrderType({Utils::STOP_LOSS,Utils::TRIGGER,Utils::TAKE_PROFIT},Utils::lineSeriesEnd,Utils::TRIGGER)});

    _chartHandler = new ChartHandler(this);
    _chartHandler->setMinimumSize(640, 420);
    _resetChartButton = new QPushButton("reset",this);

    _balanceLabel = new QLabel("Balance : -- USDT", this);
    _overallStatsLabel = new QLabel("OVERALL STATS", this);
    _dailyStatsLabel = new QLabel("DAILY STATS", this);
    _orderTypeComboBox = new QComboBox(this);
    for(auto  type : _orderTypesList){
        _orderTypeComboBox->addItem(type.first);
    }
    _orderTypeComboBox->setMaximumWidth(350);
    _amountRiskedSlider = new QSlider(Qt::Horizontal, this);
    _amountRiskedSlider->setRange(0, 200);
    _amountRiskedSlider->setValue(50);
    _estimationLabel = new QLabel(this);
    _estimationLabel->setMinimumSize(250, 320);
    _estimationLabel->setMaximumSize(350, 320);
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

    QGridLayout *setupLayout = new QGridLayout();
    setupLayout->addWidget(_balanceLabel, 0, 0, 1, 2);
    setupLayout->addWidget(_overallStatsLabel, 1, 0);
    setupLayout->addWidget(_dailyStatsLabel, 1, 1);
    setupLayout->addWidget(_orderTypeComboBox, 2, 0, 1, 2);
    setupLayout->addWidget(_shortSetupButton, 3, 0);
    setupLayout->addWidget(_longSetupButton, 3, 1);
    setupLayout->addWidget(_amountRiskedSlider, 4, 0, 1, 2);
    setupLayout->addWidget(_estimationLabel, 5, 0, 1, 2);
    setupLayout->addWidget(_placeOrderButton, 6, 0, 1, 2);
    setupLayout->setSpacing(20);

    _mainLayout = new QGridLayout();
    _mainLayout->setColumnStretch(0,3);
    _mainLayout->setColumnStretch(1,1);
    _mainLayout->addWidget(_chartHandler, 0, 0);
    _mainLayout->addWidget(_resetChartButton, 0, 0, Qt::AlignRight | Qt::AlignBottom);
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

    connect(_orderList, &OrderList::cancelOrder, &_binance, &ApiHandler::cancelOrder);
    connect(_orderList, &OrderList::modifyOrder, this, &MainWindow::handleModifyOrder);

    connect(_amountRiskedSlider, &QSlider::valueChanged, this, &MainWindow::fillPositionEstimate);
    connect(_orderTypeComboBox, &QComboBox::currentTextChanged, this, &MainWindow::handleOrderType);
    connect(_shortSetupButton, &QPushButton::clicked, this, &MainWindow::quickShortSetup);
    connect(_longSetupButton, &QPushButton::clicked, this, &MainWindow::quickLongSetup);
    connect(_resetChartButton, &QPushButton::clicked, _chartHandler, &ChartHandler::reset);
    connect(_placeOrderButton, &QPushButton::clicked, this, &MainWindow::handlePlaceOrder);

    fillStats();
    getApiKeys();
    _binance.getBalance();
    _binance.getLastCandles();
    _binance.createListenKey();
    _binance.getOpenOrders();
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
        if(serie == Utils::STOP_LOSS) emit slCanceled(order["S"].toString());
        else if(serie == Utils::TAKE_PROFIT) emit tpCanceled(order["S"].toString(), order["q"].toString());
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
            _position = new Position(order["o"].toString(), order["z"].toString(), Utils::stringtoSideEnum(order["S"].toString()));
            _chartHandler->serie(Utils::ENTRY)->clear();

            _binance.placeSL(Utils::inverseSideString(order["S"].toString()), QString::number(_chartHandler->serie(Utils::STOP_LOSS)->pointsVector().first().y(), 'f', _chartHandler->precision()));

            _binance.placeTP(Utils::inverseSideString(order["S"].toString()), order["z"].toString(), QString::number(_chartHandler->serie(Utils::TAKE_PROFIT)->pointsVector().first().y(), 'f', _chartHandler->precision()));

        }
        else {
            _estimationEnabled = false;
            _chartHandler->setSeriePrice(Utils::ENTRY, _chartHandler->serie(Utils::REAL_ENTRY)->pointsVector().first().y());
            delete _position;
            _position = nullptr;
            _orderTypeComboBox->setCurrentIndex(0);
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
        if(order["c"] == "TAKE_PROFIT"){
            qDebug() << "TP partially filled";
        }
    }
}

void MainWindow::handleModifyOrder(QString name)
{
    if(name != "STOP_LOSS" && name != "TAKE_PROFIT"){
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
    if(name == "STOP_LOSS"){
        QString newPrice = QString::number(_chartHandler->serie(Utils::STOP_LOSS)->pointsVector().first().y(), 'f', 3);
        QMetaObject::Connection * const connection = new QMetaObject::Connection;
        *connection = connect(this, &MainWindow::slCanceled, [this, newPrice, connection](QString side){
            _binance.placeSL(side, newPrice);
            QObject::disconnect(*connection);
            delete connection;
        });
    }
    else if(name == "TAKE_PROFIT"){
        QString newPrice = QString::number(_chartHandler->serie(Utils::TAKE_PROFIT)->pointsVector().first().y(), 'f', 3);
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
    _binance.createOrder(_orderTypeComboBox->currentText(), _currentOrder);
}

Utils::SidesEnum MainWindow::currentPossibleOrder()
{
    QString typeName = _orderTypeComboBox->currentText();
    if(!typeName.isEmpty()){
        bool longIsPossible = true;
        bool shortIsPossible = true;
        QList<Utils::SeriesEnum> *list = _orderTypesList.at(typeName).linesInOrder();
        foreach (auto line, *list){
            if(_chartHandler->serie(line)->pointsVector().isEmpty()) return Utils::Undefined;
            if(list->indexOf(line) < list->size()-1 && !_chartHandler->serie(list->at(list->indexOf(line)+1))->pointsVector().isEmpty() && (_chartHandler->serie(line)->pointsVector().first().y() < _chartHandler->serie(list->at(list->indexOf(line)+1))->pointsVector().first().y())) shortIsPossible = false;
            if(list->indexOf(line) >= 1 && !_chartHandler->serie(list->at(list->indexOf(line)-1))->pointsVector().isEmpty() && (_chartHandler->serie(line)->pointsVector().first().y() < _chartHandler->serie(list->at(list->indexOf(line)-1))->pointsVector().first().y())) longIsPossible = false;
        }
        double currentPrice = _chartHandler->candles()->sets().last()->close();
        if(_orderTypesList.at(typeName).lowerBound() != Utils::lineSeriesEnd && !_chartHandler->serie(_orderTypesList.at(typeName).lowerBound())->pointsVector().isEmpty() && currentPrice < _chartHandler->serie(_orderTypesList.at(typeName).lowerBound())->pointsVector().first().y()) longIsPossible = false;
        if(_orderTypesList.at(typeName).upperBound() != Utils::lineSeriesEnd && !_chartHandler->serie(_orderTypesList.at(typeName).upperBound())->pointsVector().isEmpty() && currentPrice > _chartHandler->serie(_orderTypesList.at(typeName).upperBound())->pointsVector().first().y()) longIsPossible = false;
        if(_orderTypesList.at(typeName).lowerBound() != Utils::lineSeriesEnd && !_chartHandler->serie(_orderTypesList.at(typeName).lowerBound())->pointsVector().isEmpty() && currentPrice > _chartHandler->serie(_orderTypesList.at(typeName).lowerBound())->pointsVector().first().y()) shortIsPossible = false;
        if(_orderTypesList.at(typeName).upperBound() != Utils::lineSeriesEnd && !_chartHandler->serie(_orderTypesList.at(typeName).upperBound())->pointsVector().isEmpty() && currentPrice < _chartHandler->serie(_orderTypesList.at(typeName).upperBound())->pointsVector().first().y()) shortIsPossible = false;
        if(longIsPossible) return Utils::BUY;
        if(shortIsPossible) return Utils::SELL;
    }
    return Utils::Undefined;
}

void MainWindow::fillPositionEstimate()
{
    if(!_estimationEnabled) return;
    Utils::SidesEnum side;
    if(_position != nullptr){
        side = _position->side();
        double pnl = 0;
        if(_chartHandler->serie(Utils::REAL_ENTRY)->pointsVector().isEmpty()) return;
        double entry = _chartHandler->serie(Utils::REAL_ENTRY)->pointsVector().first().y();
        double volume = _position->volume().toDouble();
        double sl = _chartHandler->serie(Utils::STOP_LOSS)->pointsVector().first().y();
        double tp = _chartHandler->serie(Utils::TAKE_PROFIT)->pointsVector().first().y();
        double risk;
        double reward;
        if(_position->type() != "LIMIT"){
            if(side == Utils::BUY){
                pnl = volume*(_wsHandler.getBid()*0.9996-entry*1.0004);
                risk = volume*(sl*0.9996-entry*1.0004);
                reward = volume*(tp*0.9998-entry*1.0004);
            }
            else{
                pnl = volume*(entry*0.9996-_wsHandler.getAsk()*1.0004);
                risk = volume*(entry*0.9996-sl*1.0004);
                reward = volume*(entry*0.9996-tp*1.0002);
            }
        }
        else {
            if(side == Utils::BUY){
                pnl = volume*(_wsHandler.getBid()*0.9996-entry*1.0002);
                risk = volume*(sl*0.9996-entry*1.0002);
                reward = volume*(tp*0.9998-entry*1.0002);
            }
            else{
                pnl = volume*(entry*0.9998-_wsHandler.getAsk()*1.0004);
                risk = volume*(entry*0.9998-sl*1.0004);
                reward = volume*(entry*0.9998-tp*1.0002);
            }
        }
        QString text = "        Current Position\n\n";
        text.append(" PnL : " + QString::number(pnl) + "\n");
        text.append(" Risk : " + QString::number(risk) + "\n");
        text.append(" Reward : " + QString::number(reward) + "\n");
        text.append(" Side : " + Utils::sideEnumToString(side) + "\n");
        text.append(" Type : " + _position->type() + "\n");
        text.append(" Volume : " + _position->volume() + " XTZ\n");
        text.append(" Entry : " + QString::number(entry) + "\n");
        text.append(" SL : " + QString::number(sl) + "\n");
        text.append(" TP : " + QString::number(tp) + "\n");
        _estimationLabel->setText(text);
        return;
    }

    side = currentPossibleOrder();
    QString sideString;
    QString volumeXTZString;
    QString rewardString;
    QString ratioString;
    QString entryString;
    QString slString;
    QString tpString;
    double risk = double(_amountRiskedSlider->value())/10.0;
    if(side == Utils::Undefined){
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
        if(_chartHandler->serie(Utils::STOP_LOSS)->pointsVector().isEmpty() || _chartHandler->serie(Utils::TAKE_PROFIT)->pointsVector().isEmpty()) return;
        double stopLoss = _chartHandler->serie(Utils::STOP_LOSS)->pointsVector().first().y();
        double takeProfit = _chartHandler->serie(Utils::TAKE_PROFIT)->pointsVector().first().y();
        risk = double(_amountRiskedSlider->value())/10.0;
        if(_orderTypeComboBox->currentText().contains("limit")){
            if(_chartHandler->serie(Utils::ENTRY)->pointsVector().isEmpty()) return;
            entry = _chartHandler->serie(Utils::ENTRY)->pointsVector().first().y();
        }
        else if(_orderTypeComboBox->currentText() == "trigger market"){
            if(_chartHandler->serie(Utils::TRIGGER)->pointsVector().isEmpty()) return;
            entry = _chartHandler->serie(Utils::TRIGGER)->pointsVector().first().y();
        }
        else {
            if(side == Utils::BUY) entry = _wsHandler.getAsk();
            else entry = _wsHandler.getBid();
        }
        double volume;
        if(_orderTypeComboBox->currentText().contains("market")){
            if(side == Utils::BUY) volume = (-1*risk)/(stopLoss*0.9996-entry*1.0004);
            else volume = (-1*risk)/(entry*0.9996-stopLoss*1.0004);
        }
        else {
            if(side == Utils::BUY) volume = (-1*risk)/(stopLoss*0.9996-entry*1.0002);
            else volume = (-1*risk)/(entry*0.9998-stopLoss*1.0004);
        }
        double reward = 0;
        if(_orderTypeComboBox->currentText().contains("market")){
            if(side == Utils::BUY) reward = volume*(takeProfit*0.9998-entry*1.0004);
            else reward = volume*(entry*0.9996-takeProfit*1.0002);
        }
        else {
            if(side == Utils::BUY) reward = volume*(takeProfit*0.9998-entry*1.0002);
            else reward = volume*(entry*0.9998-takeProfit*1.0002);
        }
        rewardString = QString::number(reward,'f', 2);
        ratioString = QString::number(reward/risk);
        volumeXTZString = QString::number(volume, 'f', 1);
        entryString = QString::number(entry, 'f', _chartHandler->precision());
        slString = QString::number(stopLoss, 'f', _chartHandler->precision());
        tpString = QString::number(takeProfit, 'f', _chartHandler->precision());
        _currentOrder.side = Utils::sideEnumToString(side);
        _currentOrder.price = entryString;
        _currentOrder.volume = volumeXTZString;
        if(!_chartHandler->serie(Utils::TRIGGER)->pointsVector().isEmpty()) _currentOrder.trigger = QString::number(_chartHandler->serie(Utils::TRIGGER)->pointsVector().first().y(), 'f', _chartHandler->precision());
    }
    QString text = "        Position Estimation\n\n";
    text.append(" Side : " + Utils::sideEnumToString(side) + "\n");
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

void MainWindow::handleOrderType(const QString text)
{
    QList<Utils::SeriesEnum> *list = _orderTypesList.at(text).linesInOrder();
    for(int i=0; i<Utils::lineSeriesEnd; i++){
        if (!list->contains((Utils::SeriesEnum) i)) _chartHandler->serie((Utils::SeriesEnum) i)->clear();
    }
    if(!_chartHandler->serie(Utils::STOP_LOSS)->pointsVector().isEmpty()){
        foreach (auto serieName, *list){
            if (_chartHandler->serie(serieName)->pointsVector().isEmpty()) _chartHandler->setSeriePrice(serieName, _chartHandler->candles()->sets().last()->close());
        }
    }
    fillPositionEstimate();
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

    QString textStats = "OVERALL STATS\n\n";
    textStats.append("Wins : " + QString::number(nbWin) + "\n");
    textStats.append("Losses : " + QString::number(nbLoss) + "\n");
    textStats.append("Breakevens : " + QString::number(nbEven) + "\n");
    textStats.append("Change : " + QString::number(previousBalance - startBalance) + " USDT");
    QPalette pal = _overallStatsLabel->palette();
    pal.setColor(QPalette::WindowText, previousBalance > startBalance ? Qt::darkGreen : Qt::red);
    _overallStatsLabel->setPalette(pal);
    _overallStatsLabel->setText(textStats);

    QString textDailyStats = "DAILY STATS\n\n";
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
    QString typeName = _orderTypeComboBox->currentText();
    if(!typeName.isEmpty()){
        _chartHandler->setSeriePrice(Utils::STOP_LOSS, _chartHandler->candles()->sets().last()->close()*1.01);
        _chartHandler->setSeriePrice(Utils::TAKE_PROFIT, _chartHandler->candles()->sets().last()->close()/1.02);
        if(_orderTypesList.at(typeName).linesInOrder()->contains(Utils::ENTRY)) _chartHandler->setSeriePrice(Utils::ENTRY, _chartHandler->candles()->sets().last()->high());
        if(_orderTypesList.at(typeName).linesInOrder()->contains(Utils::TRIGGER)) _chartHandler->setSeriePrice(Utils::TRIGGER, _chartHandler->candles()->sets().last()->low());
    }
    _chartHandler->setYAxisRange();
    fillPositionEstimate();
}

void MainWindow::quickLongSetup()
{
    QString typeName = _orderTypeComboBox->currentText();
    if(!typeName.isEmpty()){
        _chartHandler->setSeriePrice(Utils::STOP_LOSS, _chartHandler->candles()->sets().last()->close()/1.01);
        _chartHandler->setSeriePrice(Utils::TAKE_PROFIT, _chartHandler->candles()->sets().last()->close()*1.02);
        if(_orderTypesList.at(typeName).linesInOrder()->contains(Utils::ENTRY)) _chartHandler->setSeriePrice(Utils::ENTRY, _chartHandler->candles()->sets().last()->low());
        if(_orderTypesList.at(typeName).linesInOrder()->contains(Utils::TRIGGER)) _chartHandler->setSeriePrice(Utils::TRIGGER, _chartHandler->candles()->sets().last()->high());
    }
    _chartHandler->setYAxisRange();
    fillPositionEstimate();
}

MainWindow::~MainWindow()
{
    if(_position != nullptr) delete _position;
}

