#ifndef ORDERHANDLER_HPP
#define ORDERHANDLER_HPP

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QWidget>

#include "globals.hpp"

class OrderHandler : public QWidget
{
    Q_OBJECT
public:
    OrderHandler(QString order, QString type, QString side, QString volume, QString price, QString trigger, QWidget *parent = nullptr, bool global = false);
    QString name() { return _order->text(); }

signals:
    void cancel(QString name);
    void modify(QString name);

private:
    QPushButton *_cancelButton = nullptr;
    QPushButton *_modifyButton = nullptr;

    QLabel *_order;
    QLabel *_type;
    QLabel *_side;
    QLabel *_volume;
    QLabel *_price;
    QLabel *_trigger;
};

class OrderList : public QWidget
{
    Q_OBJECT
public:
    OrderList(QWidget *parent = nullptr) : QWidget(parent)
    {
        QJsonObject topBand;
        topBand.insert("c", "Order");
        topBand.insert("o", "Type");
        topBand.insert("S", "Side");
        topBand.insert("q", "Volume");
        topBand.insert("p", "Price");
        topBand.insert("sp", "Trigger");

        _mainLayout = new QVBoxLayout(this);
        addOrder(topBand, true);
    }
    void addOrder(QJsonObject &order, bool top = false){
        OrderHandler *newOrder = new OrderHandler(order["c"].toString(), order["o"].toString(), order["S"].toString(), order["q"].toString(), order["p"].toString(), order["sp"].toString(), this, top);
        _mainLayout->addWidget(newOrder);
        _currentOrders.insert({newOrder->name(), newOrder});
        connect(newOrder, &OrderHandler::cancel, this, &OrderList::cancelOrderSlot);
        connect(newOrder, &OrderHandler::modify, this, &OrderList::modifyOrderSlot);
    }
    void removeOrder(QString name){
        auto it = _currentOrders.find(name);
        if(it != _currentOrders.end()){
            _mainLayout->removeWidget(it->second);
            delete it->second;
            _currentOrders.erase(it);
        }
    }

signals:
    void cancelOrder(QString name);
    void modifyOrder(QString name);

public slots:
    void cancelOrderSlot(QString name) { emit cancelOrder(name);}
    void modifyOrderSlot(QString name) { emit modifyOrder(name);}

private:
    QVBoxLayout *_mainLayout;
    std::map<QString,OrderHandler*> _currentOrders;
};
#endif // ORDERHANDLER_HPP
