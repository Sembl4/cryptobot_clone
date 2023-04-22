f_markets = open("../research_data/pairs_usdt.txt",'r')
f_triangles = open("../research_data/triangles_usdt.txt",'r')

class Market:
    symbols: str
    stepSize: float
    minNotional: float
    tickSize: float
    c1: int
    c2: int
    c3: int
    def __init__(self, symbols, stepSize, minNotional, tickSize):
        self.symbols = symbols
        self.stepSize = stepSize
        self.minNotional = minNotional
        self.tickSize = tickSize
        self.c1 = 0
        self.c2 = 0
        self.c3 = 0
    
    def print(self):
        print(self.symbols, self.stepSize, self.minNotional, self.tickSize, self.c1, self.c2, self.c3)

    def to_str(self):
        return str(str(self.symbols) + ' ' + str(self.stepSize) + ' ' + str(self.minNotional) + ' ' + str(self.tickSize))


class Triangle:
    s1: str
    o1: str
    s2: str
    o2: str
    s3: str
    o3: str    
    def __init__(self,s1, o1, s2, o2, s3, o3):
        self.s1 = s1
        self.o1 = o1
        self.s2 = s2
        self.o2 = o2
        self.s3 = s3
        self.o3 = o3

    def print(self):
        print(self.s1, self.o1, self.s2, self.o2, self.s3, self.o3)

    def to_str(self):
        return str(str(self.s1) + ' ' + str(self.o1) + ' ' + str(self.s2) + ' ' + str(self.o2) + ' ' + str(self.s3) + ' ' + str(self.o3))

# lines_markets = f_markets.readlines()

# Заполняем списки маркетов и треугольников
markets = [Market]
triangles = [Triangle]

for line in f_markets.readlines():
    arr = line.strip().split(" ")
    markets.append(Market(arr[0],arr[1],arr[2],arr[3]))
markets.pop(0)

for line in f_triangles.readlines():
    arr = line.strip().split(" ")
    triangles.append(Triangle(arr[0],arr[1],arr[2],arr[3],arr[4],arr[5]))
triangles.pop(0)

# Сортировка маркетов по количеству их на ребрах треугольников

for tr in triangles:
    for mk in markets:
        if tr.s1 == mk.symbols:
            mk.c1 = mk.c1 + 1
        if tr.s2 == mk.symbols:
            mk.c2 = mk.c2 + 1
        if tr.s3 == mk.symbols:
            mk.c3 = mk.c3 + 1

sort_c1 = [Market]
sort_c2 = [Market]
sort_c3 = [Market]
sort_c1.pop(0)
sort_c2.pop(0)
sort_c3.pop(0)
for mk in markets:
    sort_c1.append(mk)
    sort_c2.append(mk)
    sort_c3.append(mk)


for i in range(len(sort_c1) - 1):
    for j in range(len(sort_c1) - i - 1):
        if sort_c1[j].c1 < sort_c1[j+1].c1 :
            sort_c1[j], sort_c1[j+1] = sort_c1[j+1], sort_c1[j]
    
for i in range(len(sort_c2) - 1):
    for j in range(len(sort_c2) - i - 1):
        if sort_c2[j].c2 < sort_c2[j+1].c2 :
            sort_c2[j], sort_c2[j+1] = sort_c2[j+1], sort_c2[j]

for i in range(len(sort_c3) - 1):
    for j in range(len(sort_c3) - i - 1):
        if sort_c3[j].c3 < sort_c3[j+1].c3 :
            sort_c3[j], sort_c3[j+1] = sort_c3[j+1], sort_c3[j]

# for mk in sort_c1:
#     mk.print()

mark = sort_c1[7]

new_markets = set()
new_triangles = set()
for tr in triangles:
    if (tr.s1 == mark.symbols) | (mark.symbols == tr.s3) :
        new_triangles.add(tr)
        for mk in markets:
            if mk.symbols == tr.s1:
                new_markets.add(mk)
            if mk.symbols == tr.s2:
                new_markets.add(mk)
            if mk.symbols == tr.s3:
                new_markets.add(mk)

        # if len(new_markets) >= 300:
            # break; 

print(len(new_markets), len(new_triangles))

# for mk in new_markets:
#     print(mk.to_str())
# for tr in new_triangles:
#     print(tr.to_str())
    
f_new_markets = open("../research_data/pairs_usdt_BUSDUSDT.txt",'w')
f_new_triangles = open("../research_data/triangles_usdt_BUSDUSDT.txt",'w', encoding='UTF8')

for mk in new_markets:
    f_new_markets.write(mk.to_str() + '\n')

for tr in new_triangles:
    f_new_triangles.write(tr.to_str() + '\n')

# print(markets[0].symbols, markets[0].tickSize)
# print(markets[1320].symbols, markets[1320].tickSize)

