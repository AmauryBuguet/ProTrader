#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageAuthenticationCode>
#include <QSignalSpy>

#include "globals.hpp"
#include "wshandler.hpp"

WsHandler::WsHandler(QObject *parent) : QObject(parent)
{
}

bool WsHandler::setup(QString listenKey)
{
    qDebug() << "Connecting to binance websocket ...";
    connect(&_webSocket, &QWebSocket::connected, this, &WsHandler::onWsConnected);
    _webSocket.open(QUrl("wss://fstream.binance.com/stream?streams=" + PAIR.toLower() + "@kline_5m/" + listenKey + "/" + PAIR.toLower() + "@bookTicker"));
    return true;
}

double WsHandler::getBid()
{
    return _highestBid;
}

double WsHandler::getAsk()
{
    return _lowestAsk;
}

void WsHandler::onWsConnected()
{
    qDebug() << "WebSocket connected";
    connect(&_webSocket, &QWebSocket::textMessageReceived, this, &WsHandler::onMsgReceived);
}

void WsHandler::onMsgReceived(QString message)
{
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    QJsonObject object = doc.object();
    if(object["stream"] == PAIR.toLower() + "@bookTicker"){
        bool changed = false;
        double bid = object["data"].toObject()["b"].toString().toDouble();
        if(_highestBid != bid) {
            changed = true;
            _highestBid = bid;
        }
        double ask = object["data"].toObject()["a"].toString().toDouble();
        if (_lowestAsk != ask){
            changed = true;
            _lowestAsk = ask;
        }
        if (changed) emit bidaskChanged();
    }
    else if(object["stream"] == PAIR.toLower() + "@kline_5m"){
        QJsonObject kline = object["data"].toObject()["k"].toObject();
        emit receivedKline(kline);
    }
    else if(object["stream"] == LISTENKEY){
        if(object["data"].toObject()["e"] == "ORDER_TRADE_UPDATE"){
            QJsonObject order = object["data"].toObject()["o"].toObject();
            emit orderUpdate(order);
        }
        else if(object["data"].toObject()["e"] == "ACCOUNT_UPDATE"){
            double accUSDT = 0;
            QJsonArray accountBalances = object["data"].toObject()["a"].toObject()["B"].toArray();
            foreach (QJsonValue val, accountBalances){
                QJsonObject asset = val.toObject();
                if(asset["a"] == "USDT") accUSDT = asset["wb"].toString().toDouble();
            }
            emit accountUpdate(accUSDT);
        }
    }
    else qDebug().noquote() << message;
}
