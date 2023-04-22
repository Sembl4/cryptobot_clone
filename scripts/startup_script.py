import requests, json, csv

import sys
start_token = sys.argv[1]
platform = sys.argv[2]
name_triangles_file = '../research_data/triangles_' + start_token + '_' +platform + '.txt'
name_pairs_file = '../research_data/pairs_' + start_token + '_' +platform + '.txt'

# берем все рабочие маркеты с сайта binance
if platform == 'Binance': 
    terminator = ''
    info = requests.get("https://api.binance.com/api/v3/exchangeInfo")
    text_info = info.text

    data = json.loads(text_info)

    lines = []
    for i in data['symbols'] :
        if i['status'] == 'TRADING' :
            if 'SPOT' in i['permissions']:
                lines.append(i['baseAsset'] + '-' + i['quoteAsset'])

if platform == 'OKX':
    terminator = '-'
    info = requests.get("https://www.okx.com/api/v5/public/instruments?instType=SPOT")
    text_info = info.text
    data = json.loads(text_info)
    lines = []
    for i in data['data'] :
        lines.append(i['baseCcy'] + '-' + i['quoteCcy'])


# составление треугольников (код Игоря)

from collections import defaultdict
forward = defaultdict(list)
for pair in lines:
    forward[pair.split('-')[0]].append(pair.split('-')[1])
    
backward = defaultdict(list)
for pair in lines:
    backward[pair.split('-')[1]].append(pair.split('-')[0])

result = []

temp_path = []

for first_transaction in forward[start_token]:
    temp_path.append([[start_token, first_transaction, 'SELL']])
for first_transaction in backward[start_token]:
    temp_path.append([[start_token,  first_transaction, 'BUY']])
    
while len(temp_path[0]) == 1:
    for second_transaction in forward[temp_path[0][0][1]]:
        temp_path.append([temp_path[0][0], [temp_path[0][0][1], second_transaction, 'SELL']])
    for second_transaction in backward[temp_path[0][0][1]]:
        temp_path.append([temp_path[0][0], [temp_path[0][0][1], second_transaction, 'BUY']])
                                       
    del temp_path[0]


while len(temp_path[0]) == 2:
    
    for third_transaction in forward[temp_path[0][1][1]]:
        if third_transaction == start_token:
            temp_path.append([temp_path[0][0], temp_path[0][1], [temp_path[0][1][1], third_transaction, 'SELL']])
        
    for third_transaction in backward[temp_path[0][1][1]]:
        if third_transaction == start_token:
            temp_path.append([temp_path[0][0], temp_path[0][1], [temp_path[0][1][1], third_transaction, 'BUY']])
                                       
    del temp_path[0]

index = 0
while index < len(temp_path):
    temp = []
    for pair in temp_path[index]:
        temp += [pair[0], pair[1]]
    if len(set(temp)) < 3:
        del temp_path[index]
        index -= 1
    index += 1

result += temp_path


# вывод в файл
with open(name_triangles_file, 'w', encoding='UTF8') as f:
    writer = csv.writer(f, delimiter=' ')
    for row in result:
        final_row = [ [pair[0] +terminator+ pair[1], pair[2]] if pair[2] == 'SELL' else [pair[1] +terminator+ pair[0], pair[2]] for pair in row]
        writer.writerow(final_row[0]+final_row[1]+final_row[2])

# разделение треугольников на BTC и USDT (код Акопа) в соответствующие файлы
# import copy as cp

# file = open('../research_data/triangles_'+start_token+'.txt', 'r')
# берем все маркеты для треугольников (код Акопа) и записываем в соответствующие файлы

def trianglesToPairsBinance(tr, pr) :
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
    tickSize = ""
    for pair in pairs_list:
        for i in symdols_info:
            if i['symbol'] == pair:
                for filt in i['filters']:
                    if filt['filterType'] == 'LOT_SIZE' :
                        step_size = filt['stepSize']
                    if filt['filterType'] == 'NOTIONAL':
                        min_notional = filt['minNotional']
                    if filt['filterType'] == 'PRICE_FILTER':
                        tickSize = filt['tickSize']
                    
        pairs_file.write(pair + ' ' + step_size + ' ' + min_notional + ' ' + tickSize + '\n')
    return

def trianglesToPairsOKX(tr, pr) :
    f = open(tr, 'r')
    pairs_list = set()
    for triangle in f.readlines():
        markets = triangle.strip().split(' ')
        for pair in markets:
            if pair == "SELL" or pair == "BUY":
                continue
            pairs_list.add(pair)

    pairs_file = open(pr,'w')
    symdols_info = data['data']
    step_size = ""
    min_notional = ""
    tickSize = ""
    for pair in pairs_list:
        for i in symdols_info:
            if i['instId'] == pair:
                step_size = i['lotSz']
                min_notional = i['minSz']
                tickSize = i['tickSz']
                    
        pairs_file.write(pair + ' ' + step_size + ' ' + min_notional + ' ' + tickSize + '\n')
    return

if platform == 'Binance':
    trianglesToPairsBinance(name_triangles_file, name_pairs_file)

if platform == 'OKX':
    trianglesToPairsOKX(name_triangles_file, name_pairs_file)

