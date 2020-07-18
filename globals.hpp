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
    enum SeriesEnum {
        STOP_LOSS = 0,
        TAKE_PROFIT,
        ENTRY,
        REAL_ENTRY,
        TRIGGER,
        lineSeriesEnd
    };
    Q_ENUM(SeriesEnum)

    enum SidesEnum {
        BUY,
        SELL,
        Undefined
    };
    Q_ENUM(SidesEnum)

    static SeriesEnum stringToSerieEnum(QString serie){
        if(serie == "STOP_LOSS") return STOP_LOSS;
        else if(serie == "TAKE_PROFIT") return TAKE_PROFIT;
        else if(serie == "ENTRY") return ENTRY;
        else if(serie == "REAL_ENTRY") return REAL_ENTRY;
        else if(serie == "TRIGGER") return TRIGGER;
        else return lineSeriesEnd;
    }
    static QString serieEnumToString(SeriesEnum serie){
        if(serie == STOP_LOSS) return "STOP_LOSS";
        else if(serie == TAKE_PROFIT) return "TAKE_PROFIT";
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
extern QString API_KEY;
extern QString API_SECRET;
extern QString ENDPOINT;
extern QString LISTENKEY;

extern int CURRENT_SUBSCRIBE_ID;
extern int CURRENT_ORDER_ID;

#endif // GLOBALS_HPP
