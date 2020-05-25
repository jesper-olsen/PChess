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

import json
import ctypes
import pathlib

path = pathlib.Path(__file__).parent.absolute()
so_file = str(path) + "/build/chess.so"
Chess=ctypes.CDLL(so_file)

import random, time, sys, os
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
    x=7-i//8
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


        
Chess.new_chess_game.restype=ctypes.c_void_p
Chess.free_chess_game.argtypes=[ctypes.c_void_p]
Chess.game_over.argtypes=[ctypes.c_void_p]
Chess.post_mortem.restype=ctypes.c_char_p
Chess.post_mortem.argtypes=[ctypes.c_void_p]
Chess.compute_move.argtypes=[ctypes.c_void_p]
Chess.make_move.argtypes=[ctypes.c_void_p, ctypes.c_int, ctypes.c_int]
Chess.get_from.argtypes=[ctypes.c_void_p]
Chess.get_to.argtypes=[ctypes.c_void_p]
Chess.get_is_kill.argtypes=[ctypes.c_void_p]
Chess.get_val.argtypes=[ctypes.c_void_p]
Chess.get_searched.argtypes=[ctypes.c_void_p]
Chess.get_depth_actual.argtypes=[ctypes.c_void_p]
Chess.get_turn.restype=ctypes.c_char_p
Chess.get_turn.argtypes=[ctypes.c_void_p]
Chess.in_check_n.argtypes=[ctypes.c_void_p]
Chess.set_depth.argtypes=[ctypes.c_void_p, ctypes.c_int]
Chess.set_max_search.argtypes=[ctypes.c_void_p, ctypes.c_int]
Chess.set_max_time.argtypes=[ctypes.c_void_p, ctypes.c_float]
Chess.get_legal_moves_str.restype=ctypes.c_char_p
Chess.get_legal_moves_str.argtypes=[ctypes.c_void_p]
Chess.get_piece.argtypes=[ctypes.c_void_p, ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p]
Chess.PrintBoard.argtypes=[ctypes.c_void_p]
Chess.set_from.argtypes=[ctypes.c_void_p, ctypes.c_int]
Chess.set_to.argtypes=[ctypes.c_void_p, ctypes.c_int]
Chess.update_game.argtypes=[ctypes.c_void_p]



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
        #self.cg=Chess.new_game()
        self.cg=Chess.new_chess_game()
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
        Chess.free_chess_game(self.cg)

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

    def compute_move(self):
        tup=self.lookup_move()
        if tup!=None:
            return {'from': tup[0],
                    'to': tup[1],
                    'is_kill': 0,
                    'val': 0,
                    'searched': 0,
                    'depth_actual': 0}

        Chess.set_depth(self.cg, self.max_ply)
        Chess.set_max_search(self.cg, self.max_search)
        Chess.set_max_time(self.cg, self.max_time)
        Chess.compute_move(self.cg)
        return {'from': Chess.get_from(self.cg), 
                'to': Chess.get_to(self.cg),
                'is_kill': Chess.get_is_kill(self.cg),
                'val': Chess.get_val(self.cg),
                'searched': Chess.get_searched(self.cg),
                'depth_actual': Chess.get_depth_actual(self.cg)}

    def make_move(self, frm, to):
        Chess.set_from(self.cg, frm)
        Chess.set_to(self.cg, to)
        is_kill=Chess.update_game(self.cg)
        if is_kill<0:
            print("Not a legal move:", get_label((frm, to, 0)))
            raise
        else:
            self.log += [(frm,to,is_kill)]
        return is_kill

    def get_turn(self):
        return str(Chess.get_turn(self.cg),'utf-8')

    def get_status(self):
        if self.game_over():
            return self.post_mortem()
        if self.get_turn()=="white":
            c="White's turn"
        else:
            c="Black's turn"
        if self.in_check():
            c="Check! " + c
        return c

    def to_string(self):
        board=list("."*64)
        for (tp,i,is_white) in self.get_pieces():
            if is_white==1:
                board[i]=tp.upper()
            else:
                board[i]=tp.lower()
        return "".join(board)

    def display(self):
        Chess.PrintBoard(self.cg)

    def get_history(self):
        return self.log

    def get_possible(self):
        return json.loads(Chess.get_legal_moves_str(self.cg))
        #p=ctypes.create_string_buffer(10000)
        #Chess.get_legal_moves_str(self.cg, p)
        #return json.loads(p.value)

    #def save(self, filename):
    #    f=open(filename, "w")
    #    for tup in self.log:
    #        f.write(str(tup[0])+" "+str(tup[1])+"\n")
    #    f.close()

    def update(self,l):
        for (frm,to) in l:
            self.make_move(frm,to)

    def to_json(self):
        return json.dumps({'log': [tup[:2] for tup in self.log]})

    def from_json(self, s, max_move=None):
        d=json.loads(s)
        l=[]
        if 'log' in d:
            l=[(int(a),int(b)) for (a,b) in d['log']]
        if max_move: l=l[:max_move]
        self.update(l)

    def game_over(self):
        """true if game is over"""
        return Chess.game_over(self.cg)

    def post_mortem(self):
        """return game status"""
        return str(Chess.post_mortem(self.cg),'utf-8')

    def new_game(self):
        Chess.free_game(self.cg)
        self.cg=Chess.new_game()

    #def read(self, filename, max_move=-1):
    #    if len(self.log)>0:
    #        self.new_game()
    #    for line in open(filename, "r"):
    #        words=line.split()
    #        if len(words)!=2:
    #            continue 
    #        frm=int(words[0])
    #        to=int(words[1])
    #        self.make_move(frm, to)
    #        #print(len(self.log), get_label((frm, to, 0)))
    #        if max_move>=0 and len(self.log)>max_move:
    #            return

    def get_pieces(self):
        ptype=ctypes.c_char()
        pkoor=ctypes.c_int()
        is_white=ctypes.c_int()
        is_alive=ctypes.c_int()
        l=[]
        for i in range(32):
            Chess.get_piece(self.cg, i, ctypes.byref(is_alive), ctypes.byref(ptype), ctypes.byref(pkoor), ctypes.byref(is_white))
            l+=[(str(ptype.value,'utf-8'),pkoor.value,is_white.value)]
        return l

    def in_check(self):
        """True if king in check"""
        return Chess.in_check_n(self.cg)


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
        move=cg.compute_move()
        tot+=move['searched']
        label=get_label((move['from'],move['to'], move['is_kill']))
        s="%d %s Move: %s Value: %d #searched: %d / %d / %d nodes/s; depth: %d" % (len(cg.log),cg.get_turn(), 
                label, move['val'], move['searched'], tot,int(tot/(time.time()-t1)), move['depth_actual'])
        cg.make_move(move['from'], move['to'])
        cg.display()
        print(s)
        #cg.save("game.txt")
    print(cg.post_mortem())

if __name__=="__main__":
    auto_play(-1)
