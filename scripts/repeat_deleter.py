from re import search
file = open("../build_log/4h_log.txt","r")

wait_start = 0

s = ""
while wait_start == 0 :
    s += file.readline()
    if search('Triangle handler started for segment',s):
        print(s)
        wait_start = 1
s = ""
triang = 0
triangles_list = []
under_tr_list = []

stop = 0
while stop == 0:
    triang = 0
    
    while triang == 0:
        s += file.readline()
        if not s: 
            triang = 1
            stop = 1
            print(s)
            
        if search('finish',s):
            # print(s)
            if s not in triangles_list:
                triangles_list.append(s)
                s = ""
                for i in range(0,5):
                    s += file.readline()
                under_tr_list.append(s)
            else :
                s = ""
                for i in range(0,5):
                    s += file.readline()
            s = ""
            triang = 1

for i in range (0,len(triangles_list)):
    print(triangles_list[i])
    print(under_tr_list[i])
