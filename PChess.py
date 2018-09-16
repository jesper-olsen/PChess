""" Copyright (c) 2004 Jesper Olsen
    License: MIT, see License.txt

    Implements the PChess class, which is mainly a wrapper for the 
    the C-extension "Chess".
    PChess adds an opening library (see ChessLib.txt) to the engine,
    and allows games to be read/saved to file.

    The 64 squares of the chessboard are represented by integers:
        H1 is 0
        H8 is 7
    and A8 is 63
"""

import random, time, sys, os
#sys.path+=["lib/python"]
import Chess
import chesslib

def fen_expand(fen):
    """Expand fen chessboard to include *every* square. 
       Use '.' for empty squares
       fen="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -"
    """
    s=""
    for c in fen:
        if c=="/": continue
        if c in "0123456789":
            s+="."*int(c)
        else:
            s+=c
    return s 

def i2str(i):
    """Translate integer coordinates to chess square"""
    x=7-i/8
    y=i % 8 +1
    return "ABCDEFGH"[x]+str(y)

def xy2i(cx,cy):
    """Translate chess square to integer coordinate"""
    x="abcdefgh".index(cx)
    y=int(cy)-1
    return (7-x)*8+y

def get_label(tup):
    """Translate 1D koordinates to algebraic move"""
    frm=tup[0]
    to=tup[1]
    if len(tup)>2 and tup[2]==1:
        kill="x"
    else:
        kill=" "
    label=i2str(frm)+kill+i2str(to)
    return label

def read_opening_lib(filename):
    f=open(filename, "r")
    openings=[]
    seq=[]
    for line in f:
        if line[0]=="#":
            pass
        elif line[0] in "-.":
            if len(seq)>0:
                openings.append(seq)
                seq=[]
        else:
            words=line.split()
            for w in words:
                seq+=[(xy2i(w[0],w[1]),xy2i(w[2],w[3]))]
    return openings
        

class PChess:
    """ PChess is a chess engine.
        
        self.compute_move() computes a move, which the engine considers
        optimal. The move is computed by doing an iterative max_ply first
        search, which is broken off when a certain max_ply (self.max_ply) has
        been reached and/or the number of positions visited exceeds a certain
        number (self.max_search).
    """
    def __init__(self):
        self.name="PChess"
        self.version="1.0.2"
        self.cg=Chess.new_game()
        self.max_ply=25            #max search max_ply - ply
        self.max_search=200000     #max number of positions to investigate
        self.max_search=100000     #max number of positions to investigate
        self.max_time=-1.0         #seconds
        self.log=[]
        self.openings=list(chesslib.read_openings())
        for i, seq in enumerate(self.openings):
            self.openings[i]=[(xy2i(x0,y0),xy2i(x1,y1)) for ((x0,y0),(x1,y1)) in seq]
        #self.openings=read_opening_lib("ChessLib.txt")
        #self.openings=[]
        random.seed()

    def __del__(self):
        Chess.free_game(self.cg)

    def lookup_move(self):
        """return next move from opening lib"""
        if self.openings==[]:
            return None

        #todo - update Lib...
        log=[]
        for tup in self.log:
            log+=[(tup[0],tup[1])]

        #find all openings that are compatible with present game history
        l=[]
        i=len(log)
        for seq in self.openings:
            if log==seq[0:i]:
                if len(seq)>i:
                    l+=[ seq[i] ]

        #pick random opening
        if len(l)>0:
            return l[random.randint(0,len(l)-1)]

        #print "Exit opening book..."
        self.openings=[] 
        return None

    def setup(self, fen):
        """Setup fen format position on internal chess board, eg.
            fen="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -"
        """
        words=fen.split()
        fene=fen_expand(words[0])
        color='b'
        if len(words)>1: color=words[1][0]
        castling="-"
        if len(words)>2: castling=words[2]
        enp_sq=-1
        if len(words)>3 and len(words[3])==2:
            enp_sq=xy2i(words[3][0], words[3][1])
            
        self.openings=[]  # op lookup confused by lack of history
        self.log=[]
        print(len(fene), fene)
        return Chess.setup(self.cg, fene, color, castling, enp_sq)

    def get_position(self):
        tup=Chess.get_position(self.cg)
        (fene, castling, enp_sq)=tup
        print(tup)

    def compute_move(self):
        """Return move (frm, to, kill, val, n, depth) or None """
        tup=self.lookup_move()
        if tup!=None:
            return (tup[0], tup[1], 0, 0, 0, 0)

        tup=Chess.compute_move(self.cg, self.max_ply, self.max_search, self.max_time)
        return tup

    def make_move(self, frm, to):
        is_kill=Chess.make_move(self.cg, frm, to)
        if is_kill<0:
            print("Not a legal move:", get_label((frm, to, 0)))
            raise
        else:
            self.log += [(frm,to,is_kill)]
        return is_kill

    def get_turn(self):
        return Chess.get_turn(self.cg)

    def display(self):
        Chess.display(self.cg)

    def get_history(self):
        return self.log

    def get_possible(self):
        return Chess.get_possible(self.cg)

    def save(self, filename):
        f=open(filename, "w")
        for tup in self.log:
            f.write(str(tup[0])+" "+str(tup[1])+"\n")
        f.close()

    def game_over(self):
        """true if game is over"""
        return Chess.game_over(self.cg)

    def post_mortem(self):
        """return game status"""
        return Chess.post_mortem(self.cg)

    def new_game(self):
        Chess.free_game(self.cg)
        self.cg=Chess.new_game()

    def read(self, filename, max_move=-1):
        if len(self.log)>0:
            self.new_game()
        for line in open(filename, "r"):
            words=line.split()
            if len(words)!=2:
                continue 
            frm=int(words[0])
            to=int(words[1])
            self.make_move(frm, to)
            #print(len(self.log), get_label((frm, to, 0)))
            if max_move>=0 and len(self.log)>max_move:
                return

    def get_pieces(self):
        return Chess.get_pieces(self.cg)

    def in_check(self):
        """True if king in check"""
        return Chess.in_check(self.cg)


def auto_play(N):
    t1=time.time()
    cg=PChess()
    #pos=".......................................k.......p.....K.......... w"
    #cg.setup(pos)

    #cg.read("game.txt")
    tot=0
    while not cg.game_over():
        if N>0 and len(cg.log)>N:
            break 
        tup=cg.compute_move()
        (frm, to, kill, val, n, depth)=tup
        tot+=n
        s="%d %s Move: %s Value: %d #searched: %d / %d / %d nodes/s; depth: %d" % (len(cg.log),cg.get_turn(), get_label(tup),val,n,tot,int(tot/(time.time()-t1)), depth)
        cg.make_move(frm, to)
        cg.display()
        print(s)
        #cg.save("game.txt")
    print(cg.post_mortem())

if __name__=="__main__":
    auto_play(-1)
