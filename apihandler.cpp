#include <iostream>

#include <QSignalSpy>

#include "apihandler.hpp"

ApiHandler::ApiHandler()
{
    _manager = new QNetworkAccessManager(this);

    _timerListenKey = new QTimer(this);
    _timerListenKey->setInterval(1200000);
    _timerListenKey->start();

    connect(_timerListenKey, &QTimer::timeout, this, &ApiHandler::pingListenKey);
    connect(_manager, &QNetworkAccessManager::finished, this, &ApiHandler::handleApiResponse);
    connect(this, &ApiHandler::requestAdded, this, &ApiHandler::sendRequest);
}

void ApiHandler::sendRequest()
{
    qDebug().noquote() << "sending" << _requestsList.front().type << "request to url :" << _requestsList.front().url.mid(9);

    QByteArray data = _requestsList.front().data;
    if(_requestsList.front().timestampNeeded){
        qint64 time = QDateTime::currentMSecsSinceEpoch();
        if(data.isEmpty()) data.append("timestamp=" + QString::number(time));
        else data.append("&timestamp=" + QString::number(time));
    }
    if(_requestsList.front().signatureNeeded){
        QByteArray signature = QMessageAuthenticationCode::hash(data, API_SECRET.toUtf8(), QCryptographicHash::Sha256).toHex();
        if(data.isEmpty()) data.append("signature=" + signature);
        else data.append("&signature=" + signature);
    }
    QString requestURL(ENDPOINT + _requestsList.front().url + "?" + data);

    QNetworkRequest request(requestURL);
    if(_requestsList.front().signatureNeeded){
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        request.setRawHeader("X-MBX-APIKEY", API_KEY.toUtf8());
    }

    if(_requestsList.front().type == "GET") _manager->get(request);
    else if(_requestsList.front().type == "POST") _manager->post(request, QByteArray());
    else if(_requestsList.front().type == "DELETE") _manager->deleteResource(request);
    else if(_requestsList.front().type == "PUT") _manager->put(request, QByteArray());
}

void ApiHandler::handleApiResponse(QNetworkReply *reply)
{
    QJsonDocument response = QJsonDocument::fromJson(reply->readAll());
    if(response["code"].toInt() < 0){
        if(response["code"].toInt() == -1021){
            _currentRequestRetryCount++;
            if(_currentRequestRetryCount < 3) {
                qDebug() << "failed attempt no" << _currentRequestRetryCount << ", retrying in 2s";
                QTimer::singleShot(2000, this, &ApiHandler::sendRequest);
            }
            else {
                checkRemainingRequests(_requestsList.front().type + " request FAILED 3 times, error : " + response["msg"].toString());
            }
        }
        else checkRemainingRequests("API request error : " + response["msg"].toString());
    }
    else {
        if(response.object().contains("listenKey")){
            LISTENKEY = response["listenKey"].toString();
            emit listenKeyCreated(response["listenKey"].toString());
        }
        else if(!response.array().isEmpty()){
            if(_requestsList.front().url == "/fapi/v1/openOrders"){
                foreach (const QJsonValue val, response.array()){
                    QJsonObject object = val.toObject();
                    QJsonObject order;
                    order.insert("setup", "true");
                    order.insert("X", object["status"].toString());
                    order.insert("c", object["clientOrderId"].toString());
                    order.insert("o", object["origType"].toString());
                    order.insert("S", object["side"].toString());
                    order.insert("q", object["origQty"].toString());
                    order.insert("p", object["price"].toString());
                    order.insert("sp", object["stopPrice"].toString());
                    emit orderUpdate(order);
                }
            }
            else if(_requestsList.front().url.contains("klines")){
                QJsonArray array = response.array();
                emit initialKlineArrayReady(array);
            }
            else if(_requestsList.front().url.contains("balance")){
                QJsonArray array = response.array();
                foreach(const QJsonValue &val, array){
                    if(val.toObject()["asset"].toString() == "USDT") emit balanceReady(val.toObject()["balance"].toString().toDouble());
                }
            }
        }

        checkRemainingRequests(_requestsList.front().type + " request successful");
    }
    reply->deleteLater();
}

void ApiHandler::checkRemainingRequests(QString msg)
{
    qDebug().noquote() << msg;
    _requestsList.pop();
    _currentRequestRetryCount = 0;
    if(!_requestsList.empty()) sendRequest();
    else connect(this, &ApiHandler::requestAdded, this, &ApiHandler::sendRequest);
}

void ApiHandler::getBalance()
{
    getRequest("/fapi/v2/balance", QByteArray(), true);
}

void ApiHandler::getLastCandles()
{
    getRequest("/fapi/v1/klines","symbol=" + PAIR.toLower().toUtf8() + "&interval=" + INTERVAL.toUtf8() + "&limit=500");
}

void ApiHandler::getOpenOrders()
{
    getRequest("/fapi/v1/openOrders","symbol=" + PAIR.toLower().toUtf8(), true);
}

void ApiHandler::createOrder(OrderData orderData)
{
    QByteArray data;
    data.append("symbol=" + PAIR);
    data.append("&side=" + Utils::sideEnumToString(orderData.side));
    data.append("&type=" + Utils::orderTypeEnumToString(orderData.type));
    if (orderData.type == Utils::LIMIT){
        data.append("&timeInForce=GTC");
    }
    data.append("&quantity=" + orderData.volume);
    if (orderData.type == Utils::LIMIT || orderData.type == Utils::STOP){
        data.append("&price=" + orderData.price);
    }
    if (Utils::orderTypeEnumToString(orderData.type).contains("STOP")){
        data.append("&stopPrice=" + orderData.trigger);
    }
    data.append("&newOrderRespType=RESULT");
    data.append("&recvWindow=3000");
    data.append("&newClientOrderId=" + Utils::serieEnumToString(Utils::ENTRY));
    postRequest("/fapi/v1/order", data);
}

void ApiHandler::placeSL(QString inverseSide, QString stopPrice)
{
    QByteArray data;
    data.append("symbol=" + PAIR);
    data.append("&side=" + inverseSide);
    data.append("&type=STOP_MARKET");
    data.append("&closePosition=true");
    data.append("&stopPrice=" + stopPrice);
    data.append("&recvWindow=3000");
    data.append("&newOrderRespType=RESULT");
    data.append("&newClientOrderId=" + Utils::serieEnumToString(Utils::SL));
    postRequest("/fapi/v1/order", data);
}

void ApiHandler::marketClosePosition(QString volume, Utils::SidesEnum side)
{
    QByteArray data;
    data.append("symbol=" + PAIR);
    data.append("&side=" + Utils::inverseSideString(side));
    data.append("&type=MARKET");
    data.append("&reduceOnly=true");
    data.append("&quantity=" + volume);
    data.append("&recvWindow=3000");
    data.append("&newOrderRespType=RESULT");
    data.append("&newClientOrderId=MARKET_CLOSE");
    postRequest("/fapi/v1/order", data);
}

void ApiHandler::placeTP(QString inverseSide, QString qty, QString price)
{
    QByteArray data;
    data.append("symbol=" + PAIR);
    data.append("&side=" + inverseSide);
    data.append("&timeInForce=GTC");
    data.append("&type=LIMIT");
    data.append("&quantity=" + qty);
    data.append("&price=" + price);
    data.append("&reduceOnly=true");
    data.append("&recvWindow=3000");
    data.append("&newOrderRespType=RESULT");
    data.append("&newClientOrderId=" + Utils::serieEnumToString(Utils::TP1));
    postRequest("/fapi/v1/order", data);
}

void ApiHandler::cancelOrder(QString name)
{
    QByteArray data;
    data.append("symbol=" + PAIR);
    if(name != "Order"){
        data.append("&origClientOrderId=" + name);
        deleteRequest("/fapi/v1/order", data, true);
    }
    else {
        deleteRequest("/fapi/v1/allOpenOrders", data, true);
    }
}

void ApiHandler::createListenKey()
{
    QByteArray data;
    postRequest("/fapi/v1/listenKey", data, false);
}

void ApiHandler::pingListenKey()
{
    QByteArray data;
    data.append("listenKey=" + LISTENKEY);

    Request request;
    request.type = "PUT";
    request.url = "/fapi/v1/listenKey";
    request.data = data;
    request.signatureNeeded = true;

    pushRequest(request);
}

void ApiHandler::deleteListenKey()
{
    QByteArray data;
    data.append("listenKey=" + LISTENKEY);
    deleteRequest("/fapi/v1/listenKey", data, false);
}

void ApiHandler::getRequest(QByteArray url, QByteArray data, bool priv)
{
    Request request;
    request.type = "GET";
    request.url = url;
    request.data = data;

    if(priv){
        request.timestampNeeded = true;
        request.signatureNeeded = true;
    }

    pushRequest(request);
}

void ApiHandler::postRequest(QByteArray url, QByteArray data, bool timestampNeeded)
{
    Request request;
    request.type = "POST";
    request.url = url;
    request.data = data;
    request.timestampNeeded = timestampNeeded;
    request.signatureNeeded = true;

    pushRequest(request);
}

void ApiHandler::deleteRequest(QByteArray url, QByteArray data, bool timestampNeeded)
{
    Request request;
    request.type = "DELETE";
    request.url = url;
    request.data = data;
    request.timestampNeeded = timestampNeeded;
    request.signatureNeeded = true;

    pushRequest(request);
}

void ApiHandler::pushRequest(Request &request)
{
    _requestsList.push(request);
    emit requestAdded();
    disconnect(this, &ApiHandler::requestAdded, this, &ApiHandler::sendRequest);
}
