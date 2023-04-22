

# from binance.spot import Spot as Client


# Виталий
# client = Client("yOQ6q3jZwlM4jn9chWXlK8xZpiAyaHJgR1MM5bHj9QCvRQt4ZTkscpQCYVzJsu8h","0eK1ZuQ71v7ElSJ5NEC1VLrNNVrXvbZ333S37tkhWWMGk2tz8Y57KOZ7uK6cYZJe")
# Семён
# client = Client("HQP47UX6YQMBaAQtufvAS9GlYgnouZghP5boxa3x2HqLbnLpuyhvHgbgGYAeprWo","COBjwRly14dBDixQzpr1RemCOrnhID4ri0LVBN6Y5kejiIzgxfvmwSjbQl69VhXc")
# client.transfer_dust
# client.cancel_open_orders(symbol = 'TKOBTC')
# print(client.get_open_orders())
# print(client.funding_wallet())
# print(client.margin_interest_history())
# print(client.user_asset())
# print(client.account_snapshot("SPOT"))

# print(client.withdraw_history())
# print(client.convert_trade_history(startTime=1675484462, endTime=1680592862))
# print(client.new_order(type='MARKET',symbol='BTCUSDT',side='BUY',quoteOrderQty='1382.1'))
# print(client.new_order(type='MARKET',symbol='BTCUSDT',side='SELL',quantity='0.0506'))

import requests, json, csv

info = requests.get("https://api.binance.com/api/v3/exchangeInfo")
# info = requests.get("https://api.binance.com/api/v3/exchangeInfo")

text = json.loads(info.text)
print(json.dumps(text,indent=4))