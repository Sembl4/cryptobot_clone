from re import search
file = open("/home/azureuser/WorksSembl4/startup/ver_0.2/crptobot_internal_private/build_O3/12h_log_with_traders.txt","r")

# BTCUSDT BUY 
# BTCTRY  SELL
# USDTTRY BUY
hours = 12
symbol = 'USDT'

wait_start = 0

s = ""
while wait_start == 0 :
    s += file.readline()
    if search('Triangle handler started for segment',s):
        # print(s)
        wait_start = 1
gfile = open('good_log_12h_with_trades.txt','w')
gfile.write(s)
s = ""

triangles_list=[]
benefitial_list=[]
strange_list=[]
stop = 0

benefit = 0
new_benefit = 0
#  This triangle does not meet the minNorional.
while stop == 0:
    triang = 0
    
    while triang == 0:
        line = file.readline()
        s += line
        if not line: 
            triang = 1
            stop = 1
            # print(s)
        if search('Benefit:',line):
            benefit = float(line.strip().split('Benefit:')[1])
        if search('New benefit:',line):
            new_benefit = float(line.strip().split('New benefit:')[1])

        if search('Triangle check costed',line):
            if not search('IS beneficial by prices',s):
                strange_list.append(s)
            else :
                if search('New benefit:',s):
                    triangles_list.append(s.strip())
                    if new_benefit > 1.00225:
                        benefitial_list.append(s.strip())
            s = ""
            triang = 1

print("Выгодных треугольников за ",hours," часов работы: ", len(triangles_list), \
"\nВыгодных треугольников после обрезания по stepSize: ", len(benefitial_list), '(которые выполняться)\n')
for tr in triangles_list:
    gfile.write(tr)
    gfile.write('\n\n')
gfile.write(s)
triangle_info = []
# print(benefitial_list[0].split('\n')[0])

start_op_type = ''
fin_op_type = ''
start_volume = 0
fin_volume = 0
start_price = 0
fin_price = 0
new_benefit = 0
max_benefit = 0
min_benefit = 0
first_step = 0

sum_profit = 0
min_profit = 0
max_profit = 0

min_start_vol = 0
max_start_vol = 0
avg_start_vol = 0


ex_1 = ""
ex_2 = ""
ex_3 = ""
ex_4 = ""
ex_5 = ""
ex_6 = ""


for tr in benefitial_list:
    line_list = tr.split('\n')
    # Расчет выгод в процентах
    benefit = float(line_list[17].strip().split('New benefit:')[1])
    benefit -= 0.00225
    if first_step == 0:
        min_benefit = benefit
        max_benefit = benefit

    if benefit < min_benefit : 
        min_benefit = benefit 
        ex_1 = tr
    if benefit > max_benefit : 
        max_benefit = benefit
        ex_2 = tr
    new_benefit += benefit
    # Расчет прибыли
    start_op_type = line_list[2].strip().split(' ')[1]
    fin_op_type = line_list[8].strip().split(' ')[1]
    start_price = float(line_list[4].strip().split('price:')[1])
    fin_price = float(line_list[10].strip().split('price:')[1])
    start_volume = float(line_list[13].strip().split('Volumes counted: start:')[1])
    fin_volume = float(line_list[15].strip().split(' ')[1])
    if start_op_type.strip() == 'BUY':  start_income = start_volume * start_price
    else : start_income = start_volume

    if fin_op_type.strip() == 'SELL':  fin_income = fin_volume * fin_price
    else : fin_income = fin_volume
    profit = start_income * (benefit - 1)
    sum_profit += profit
    if first_step == 0:
        min_profit = profit
        ex_3 = ex_4 = ex_5 = ex_6 = tr
        max_profit = profit
        min_start_vol = start_income
        max_start_vol = start_income
        first_step = 1
    if profit < min_profit : 
        min_profit = profit
        ex_3 = tr
    if profit > max_profit : 
        max_profit = profit
        ex_4 = tr
    avg_start_vol+=start_income
    if start_income < min_start_vol: 
        min_start_vol = start_income
        ex_5 = tr 
    if start_income > max_start_vol: 
        max_start_vol = start_income
        ex_6 = tr
    
avg_start_vol/=len(benefitial_list)


print('Под словом <выгода> подразумевается чистая выгода с учетом всех комиссий в процентах.')
print(' Минимальная выгода с одного треугольника: ', min_benefit*100,'% (Пример 1)')
print(' Максимальная выгода с одного треугольника: ', max_benefit*100,'% (Пример 2)')
print(' Средняя выгода с',len(benefitial_list),'треугольников: ', (new_benefit/len(benefitial_list))*100, '%\n')

print('Под словом <прибыль> подразумевается чистая прибыль с учетом всех комиссий в',symbol)
print(' Минимальная прибыль с одного треугольника: ', min_profit, symbol ,'(Пример 3)')
print(' Максимальная прибыль с одного треугольника: ', max_profit, symbol ,'(Пример 4)')
print(' Средняя прибыль: ',sum_profit/len(benefitial_list),symbol)
print(' Суммарная прибыль: ', sum_profit,symbol,'\n')

print('Под словом <вложение> подразумевается сколько мы должны вложить в треугольник',symbol)
print(' Минимальное вложение в один треугольник: ', min_start_vol,symbol,'(Пример 5)')
print(' Максимальное вложение в один треугольник: ', max_start_vol,symbol,'(Пример 6)')
print(' Среднее вложение: ', avg_start_vol,symbol,'\n')
print('Примеры (могут совпадать):')
examples = [ex_1,ex_2,ex_3,ex_4,ex_5,ex_6]

for i in range(0,len(examples)):
    print('------Пример', i + 1, '\n\n',examples[i],'\n')
# c = len(triangles_list)
# print(c)
# print(triangles_list[0])
# print(triangles_list[c - 1])

# print(benefit, '\n', new_benefit, '\n', benefit - new_benefit)

# print(strange_list[0])