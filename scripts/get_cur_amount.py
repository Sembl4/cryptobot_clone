import sys

# print(sys.argv[1])


from binance.spot import Spot as Client

# Виталий
client = Client("yOQ6q3jZwlM4jn9chWXlK8xZpiAyaHJgR1MM5bHj9QCvRQt4ZTkscpQCYVzJsu8h","0eK1ZuQ71v7ElSJ5NEC1VLrNNVrXvbZ333S37tkhWWMGk2tz8Y57KOZ7uK6cYZJe")
# Семён
# client = Client("HQP47UX6YQMBaAQtufvAS9GlYgnouZghP5boxa3x2HqLbnLpuyhvHgbgGYAeprWo","COBjwRly14dBDixQzpr1RemCOrnhID4ri0LVBN6Y5kejiIzgxfvmwSjbQl69VhXc")
if len(sys.argv) < 2:
    exit()

currency = sys.argv[1]
if (len(sys.argv) > 2) & (sys.argv[2] == '1'):
    dust_text = client.sign_request('POST','/sapi/v1/asset/dust-btc')
    can_be_BNB = []
    for asset in dust_text['details']:
        if asset['asset'] != currency:
            can_be_BNB.append(asset['asset'])
    if len(can_be_BNB) != 0:
        converted = client.transfer_dust(can_be_BNB)
        converted_assets = []
        print('Transefed to BNB result:')
        for asset in converted['transferResult']:
            print(asset['fromAsset'],'(',asset['amount'],')',end=';')
        print('\nTotal transfered:',converted['totalTransfered'],'BNB')
    else:
        print('Don`t have asset to transfer to BNB')
    
class Balance:
    def __init__(self,symbol, amount, cur_amount):
        self.symbol = symbol
        self.amount = amount
        self.cur_amount = cur_amount

pairs_list = []

# i = 0
info = client.exchange_info()['symbols']
text = client.account()
for asset in text['balances']:
    if float(asset['free']) > 0:
        balance = Balance(symbol=asset['asset'],amount=asset['free'],cur_amount=0.0)
        if balance.symbol != currency:
            for market in info:
                if (market['baseAsset'] == balance.symbol) & (market['quoteAsset'] == currency):
                    price = client.ticker_price(market['symbol'])['price']
                    balance.cur_amount = float(balance.amount) * float(price)
                    break
                if (market['quoteAsset'] == balance.symbol) & (market['baseAsset'] == currency):
                    price = client.ticker_price(market['symbol'])['price']
                    balance.cur_amount = float(balance.amount) / float(price)
                    break
        else:
            balance.cur_amount = balance.amount
        pairs_list.append(balance)

total_cur_amount = 0.0
for asset in pairs_list:
    total_cur_amount += float(asset.cur_amount)
    print(asset.symbol,asset.amount,currency,'amount',asset.cur_amount)

print('Total', currency, 'amount:',total_cur_amount)

# dust_text = client.sign_request('POST','/sapi/v1/asset/dust-btc')
# can_be_BNB = []
# for asset in dust_text['details']:
#     if asset['asset'] != 'USDT':
#         can_be_BNB.append(asset['asset'])

# print(client.transfer_dust(can_be_BNB))
