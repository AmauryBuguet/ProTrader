#ifndef WSHANDLER_HPP
#define WSHANDLER_HPP

#include <QObject>
#include <QWebSocket>

#include "globals.hpp"

class WsHandler : public QObject
{
    Q_OBJECT
public:
    WsHandler(QObject *parent = nullptr);
    void onWsConnected();
    void onMsgReceived(QString message);
    double getBid();
    double getAsk();
    void unsubscribe();

signals:
    void receivedKline(QJsonObject &kline);
    void orderUpdate(QJsonObject &order);
    void accountUpdate(double account);
    void bidaskChanged();

public slots:
    bool setup(QString listenKey);

private:
    QWebSocket _webSocket;

    double _lowestAsk = 0;
    double _highestBid = 0;
};

#endif // WSHANDLER_HPP
