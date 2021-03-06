openings="""
#Spansk_v1.
e2e4 e7e5
g1f3 b8c6
f1b5 a7a6
b5a4 g8f6
e1g1 f8e7
f1e1 b7b5
a4b3 d7d6
c2c3 e8g8
-
#Spansk_v2.
e2e4 e7e5
g1f3 b8c6
f1b5 a7a6
b5a4 g8f6
e1g1 f6e4
-
#Spansk_v3.
e2e4 e7e5
g1f3 b8c6
f1b5 a7a6
b5c6 d7c6
-
#Philidors_Forsvar_v1.
e2e4 e7e5
g1f3 d7d6
d2d4 e5d4
-
#Philidors_Forsvar_v2.
e2e4 e7e5
g1f3 d7d6
d2d4 b8d7 
-
#Fransk.
e2e4 e7e6
d2d4 d7d5
b1c3 g8f6
g1f3 f8e7
-
#Caro-Kann.
e2e4 c7c6
d2d4 d7d5
b1c3 d5e4
c3e4 c8f5
-
#Siciliansk.
e2e4 c7c5
g1f3 d7d6
d2d4 c5d4
f3d4 g8f6
-
#Dronninggambit.
d2d4 d7d5
c2c4 e7e6
b1c3 g8f6
c1g5 f8e7
-
#Nimzo-Indisk.
d2d4 g8f6
c2c4 e7e6
b1c3 f8b4
d1c2 b8c6
-
#Dronningeindisk.
d2d4 g8f6
c2c4 e7e6
g1f3 b7b6
g2g3 c8b7
f1g2 f8e7
.
"""

def read_openings():
    seq=[]
    for line in openings.split('\n'):
        if line=='' or line[0]=="#": continue
        if line[0] in "-." and seq!=[]:
            yield seq
            seq=[]
        else:
            seq+=[((w[0],w[1]),(w[2],w[3])) for w in line.split()]

if __name__=="__main__":
    for i,seq in enumerate(read_openings()):
        print("Opening",i)
        for mv in seq:
            print("{}->{}".format(mv[0], mv[1]))
