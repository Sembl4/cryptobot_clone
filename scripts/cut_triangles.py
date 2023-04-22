import requests, json, csv

# берем все рабочие маркеты с сайта binance
info = requests.get("https://api.binance.com/api/v3/exchangeInfo")
text_info = info.text

data = json.loads(text_info)

lines = []
for i in data['symbols'] :
    if i['status'] == 'TRADING' :
        lines.append(i['baseAsset'] + '-' + i['quoteAsset'])

def trianglesToPairs(tr, pr) :
    f = open(tr, 'r')
    pairs_list = set()
    for triangle in f.readlines():
        markets = triangle.strip().split(' ')
        for pair in markets:
            if pair == "SELL" or pair == "BUY":
                continue
            pairs_list.add(pair)

    pairs_file = open(pr,'w')
    symdols_info = data['symbols']
    step_size = ""
    min_notional = ""
    for pair in pairs_list:
        for i in symdols_info:
            if i['symbol'] == pair:
                for filt in i['filters']:
                    if filt['filterType'] == 'LOT_SIZE' :
                        step_size = filt['stepSize']
                    if filt['filterType'] == 'MIN_NOTIONAL':
                        min_notional = filt['minNotional']
        pairs_file.write(pair + ' ' + step_size + ' ' + min_notional + '\n')
    return
