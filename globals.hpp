#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>

class Utils : public QObject
{
    Q_OBJECT
public:
    enum OrderTypesEnum {
        LIMIT = 0,
        MARKET,
        STOP,
        STOP_MARKET,
        TAKE_PROFIT,
        Other
    };
    Q_ENUM(OrderTypesEnum)

    enum SeriesEnum {
        SL = 0,
        ENTRY,
        TRIGGER,
        TP1,
        TP2,
        REAL_ENTRY,
        lineSeriesEnd
    };
    Q_ENUM(SeriesEnum)

    enum SidesEnum {
        BUY,
        SELL,
        Undefined
    };
    Q_ENUM(SidesEnum)

    static QString orderTypeEnumToString(OrderTypesEnum type){
        return QVariant::fromValue(type).toString();
    }
    static OrderTypesEnum stringToOrderTypeEnum(QString type){
        if(type == "LIMIT") return LIMIT;
        else if(type == "MARKET") return MARKET;
        else if(type == "STOP") return STOP;
        else if(type == "STOP_MARKET") return STOP_MARKET;
        else return Other;

    }

    static SeriesEnum stringToSerieEnum(QString serie){
        if(serie == "SL") return SL;
        else if(serie == "TP1") return TP1;
        else if(serie == "TP2") return TP2;
        else if(serie == "ENTRY") return ENTRY;
        else if(serie == "REAL_ENTRY") return REAL_ENTRY;
        else if(serie == "TRIGGER") return TRIGGER;
        else return lineSeriesEnd;
    }
    static QString serieEnumToString(SeriesEnum serie){
        if(serie == SL) return "SL";
        else if(serie == TP1) return "TP1";
        else if(serie == TP2) return "TP2";
        else if(serie == ENTRY) return "ENTRY";
        else if(serie == REAL_ENTRY) return "REAL_ENTRY";
        else if(serie == TRIGGER) return "TRIGGER";
        else return "lineSeriesEnd";
    }
    static SidesEnum stringtoSideEnum(QString side){
        if(side == "BUY") return BUY;
        else if(side == "SELL") return SELL;
        else return Undefined;
    }
    static QString sideEnumToString(SidesEnum side){
        if(side == BUY) return "BUY";
        else if(side == SELL) return "SELL";
        else return "Undefined";
    }
    static SidesEnum inverseSide(QString side){
        if(side == "BUY") return SELL;
        else if(side == "SELL") return BUY;
        else return Undefined;
    }
    static SidesEnum inverseSide(SidesEnum side){
        if(side == BUY) return SELL;
        else if(side == SELL) return BUY;
        else return Undefined;
    }
    static QString inverseSideString(QString side){
        if(side == "BUY") return "SELL";
        else if(side == "SELL") return "BUY";
        else return "Undefined";
    }
    static QString inverseSideString(SidesEnum side){
        if(side == BUY) return "SELL";
        else if(side == SELL) return "BUY";
        else return "Undefined";
    }
};

extern QString PAIR;
extern QString INTERVAL;
extern QString API_KEY;
extern QString API_SECRET;
extern QString ENDPOINT;
extern QString LISTENKEY;

extern int CURRENT_SUBSCRIBE_ID;
extern int CURRENT_ORDER_ID;

extern int VOLUME_PRECISION;
extern int PRICE_PRECISION;

#endif // GLOBALS_HPP
