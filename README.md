## ProTrader : a simple trading interface for Binance that prioritize risk managment

For now this trading UI is adapted to my own trading style, which is scalping on the 1min or 5min chart on highly volatile cryptos like Tezos.  
To use this interface, just move the slider on the right to define the amount risked, then click on the buttons corresponding to the lines needed to build your order and move them as you want.
If an order is possible, the "Position Estimation" tab will be filled, and if you are happy with the R/R calculated you can go ahead and hit "Place Order". (The "long" and "short" buttons are only to quickly setup the lines in the right order, they DO NOT sent any order to Binance).  

IMPORTANT : you need a text file named "keys.ini" in the location specified by the Resources.qrc file, with your api keys info disposed like this (with the quotes):  
```key="<yourAPIkey>"```  
```secret="<yourAPIsecret>"```

#### Features:
* Auto-calculation of your order volume according to the amount you are willing to risk on a trade (fees included)
* Auto order type and side according to lines present on chart
* Move SL and TP lines easily with your mouse
* Overall and daily account statistics
* Support LIMIT, STOP, STOP_MARKET and MARKET orders

#### Will be added
* Possibility to trade other cryptos than XTZ
* ~~Possibility to trade on other timeframes than 5 min~~
* ~~Keyboard shortcuts~~
* Possibility to have multiple SL/TP
* Telegram bot to alert user when an order is filled

#### Bugs
* cant replace a TP order if the previous one has been canceled (advised to ALWAYS have a TP when placing an order and ONLY use the MODIFY button to edit it)
* it is advised to have a Binance tab opened in case something goes wrong with this program, this is an open source software in developpement and i'm not responsible in case of any unexpected gains/losses due to a malfunction
* trading is risky, always have a risk managment strategy :)

