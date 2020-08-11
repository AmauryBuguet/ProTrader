#ifndef APIHANDLER_HPP
#define APIHANDLER_HPP

#include <queue>

#include <QObject>
#include <QtNetwork>
#include <QTimer>

#include "globals.hpp"

struct OrderData {
    Utils::OrderTypesEnum type;
    Utils::SidesEnum side;
    QString volume;
    QString price;
    QString trigger;
};

struct Request {
    QString type = "";
    QByteArray url = "";
    QByteArray data = QByteArray();
    bool timestampNeeded = false;
    bool signatureNeeded = false;
};


class ApiHandler : public QObject
{
    Q_OBJECT

public:

    ApiHandler();
    // REST REQUESTS
    void getBalance();
    void getLastCandles();
    void getOpenOrders();
    void createOrder(OrderData orderData);
    void placeSL(QString inverseSide, QString stopPrice);
    void placeTP(QString inverseSide, QString qty, QString price);
    void marketClosePosition(QString volume, Utils::SidesEnum side);
    void cancelOrder(QString name);
    void createListenKey();
    void pingListenKey();
    void deleteListenKey();
    void getRequest(QByteArray url, QByteArray data, bool priv = false);
    void postRequest(QByteArray url, QByteArray data, bool timestampNeeded = true);
    void deleteRequest(QByteArray url, QByteArray data, bool timestampNeeded);
    void pushRequest(Request &request);
    void sendRequest();
    void handleApiResponse(QNetworkReply *reply);
    void checkRemainingRequests(QString msg);

signals:
    void initialKlineArrayReady(QJsonArray &array);
    void listenKeyCreated(QString listenKey);
    void requestAdded();
    void orderUpdate(QJsonObject &order);
    void balanceReady(double value);

private:
    QNetworkAccessManager *_manager;
    QTimer *_timerListenKey;
    std::queue<Request> _requestsList;
    int _currentRequestRetryCount = 0;
};

#endif // APIHANDLER_HPP
