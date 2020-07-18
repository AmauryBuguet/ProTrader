#include "orderhandler.hpp"

OrderHandler::OrderHandler(QString order, QString type, QString side, QString volume, QString price, QString trigger, QWidget *parent, bool global) :
    QWidget(parent)
{
    _order = new QLabel(order, this);
    _type = new QLabel(type, this);
    _side = new QLabel(side, this);
    _volume = new QLabel(volume, this);
    _price = new QLabel(price, this);
    _trigger = new QLabel(trigger, this);

    if(global){
        _cancelButton = new QPushButton("CANCEL ALL", this);
        _modifyButton = new QPushButton("MARKET CLOSE", this);
    }
    else {
        _cancelButton = new QPushButton("CANCEL", this);
        _modifyButton = new QPushButton("MODIFY", this);
    }
    _cancelButton->setMaximumWidth(150);
    _modifyButton->setMaximumWidth(150);

    QHBoxLayout *mainlayout = new QHBoxLayout(this);
    mainlayout->addWidget(_order);
    mainlayout->addWidget(_type);
    mainlayout->addWidget(_side);
    mainlayout->addWidget(_volume);
    mainlayout->addWidget(_price);
    mainlayout->addWidget(_trigger);
    mainlayout->addWidget(_modifyButton);
    mainlayout->addWidget(_cancelButton);

    connect(_cancelButton, &QPushButton::clicked, [this] {cancel(_order->text());} );
    connect(_modifyButton, &QPushButton::clicked, [this] {modify(_order->text());} );
}
