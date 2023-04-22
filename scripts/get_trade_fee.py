from binance.spot import Spot as Client

client = Client("yOQ6q3jZwlM4jn9chWXlK8xZpiAyaHJgR1MM5bHj9QCvRQt4ZTkscpQCYVzJsu8h","0eK1ZuQ71v7ElSJ5NEC1VLrNNVrXvbZ333S37tkhWWMGk2tz8Y57KOZ7uK6cYZJe")
# client = Client()
info = client.trade_fee()
exchange_info = client.exchange_info()
for market in info:
    if market['takerCommission'] == '0':
        for ex in exchange_info['symbols']:
            if market['symbol'] == ex['symbol']:
                if ex['status'] == 'TRADING':
                    print(ex['baseAsset'],'/',ex['quoteAsset'],sep='',end=', ')