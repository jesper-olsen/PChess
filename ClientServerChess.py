""" Copyright (c) 2005 Jesper Olsen
    License: MIT, see License.txt

    Client Server Chess - allows two chess engines to play tournaments 
    against each other over a network connection - TCP socket.

    PROTOCOL: messages are strings terminated by "\n"
              Only two messages are allowed:
              * Client sends the color it wishes to play as the first
                message - i.e. "white\n" or "black\n"
              * Chess move - by the side whose turn it is to play.
                Chess moves are the from to coordinates in the form of 
                two numbers in the range [0-63] - e.g. "2 4\n"
"""

import sys, time, thread, optparse
sys.path+=["lib/python"]
import PChess
from vigrid import MySocket

class ChessServer:
    def __init__(self, port=50000):
        print "initialising Chess Server"
        self.sock=MySocket.create_server_socket(port)
        if self.sock==None:
            print "Try again later - socket is buisy:", port
            sys.exit(1)

        self.tstat={"white":0, "black":0, "nmoves":0}
        self.astat={}

        while True:
            csock=self.sock.accept()
            thread.start_new_thread(self.run_engine, (csock,port))

    def run_engine(self, csock, port):
        stat, i, used_time=run_chess_engine(csock,port,verbose=1,is_server=True)
        if not stat in self.astat:
            self.astat[stat]=0
        self.tstat["white"]+=used_time["white"]
        self.tstat["black"]+=used_time["black"]
        self.tstat["nmoves"]+=i/2
        self.astat[stat]+=1
        print "\nTurnament Status Status:"
        for key in self.astat:
            print self.astat[key], key
        print "Total #moves:", self.tstat["nmoves"]
        print "White time: %f sec / %f sec per move" % (self.tstat["white"], self.tstat["white"]/(0.5*self.tstat["nmoves"]))
        print "Black time: %f / %f per move" % (self.tstat["black"], self.tstat["black"]/(0.5*self.tstat["nmoves"]))


def run_chess_engine(sock, port, **args):
    """run chess engine - play color"""
    cg=PChess.PChess()

    color="white"
    if "color" in args:
        color=args["color"]

    tot=0
    used_time={"white":0, "black":0}
    if args["is_server"]:
        data=sock.receive_sep("\n")
        words=data.split()
        if words[0]!="playing" or len(words)!=2:
            raise
        if words[1]=="black":
            color="white"
        else:
            color="black"
    else:
        sock.send("playing %s\n"%(args["color"]))

    while not cg.game_over():
        #compute move
        t1=time.time()
        if cg.get_turn()==color:
            tup=cg.compute_move()
            (frm, to, kill, val, n, depth)=tup
            tot+=n
            print len(cg.log), color, "Move:", PChess.get_label(tup), "Value:", val, "#searched:", n, "/", tot, "/", int(tot/(0.01+used_time[cg.get_turn()])), "nodes/s", "Depth:", depth
            sock.send("move %d %d\n" % (frm,to))
        else:
            #get oponent move
            data=sock.receive_sep("\n")
            words=data.split()
            try:
                frm=int(words[1])
                to=int(words[2])
            except:
                return ("Abnormal termination;%s"%(str(words)), len(cg.log), used_time)
        used_time[cg.get_turn()]+=time.time()-t1
        cg.make_move(frm, to)
        if not "verbose" in args:
            cg.display()
    return (cg.post_mortem(),len(cg.log), used_time)


class ChessClient:
    def __init__(self, host, port, ngames, color):
        for game in range(ngames):
            sock=MySocket.create_client_socket(host, port)
            if sock==None:
                return
            (status, i, used_time)=run_chess_engine(sock, port, is_server=False, color=color)
            print status
            print "#moves:", i
            print "White time:", used_time["white"]
            print "Black time:", used_time["black"]
           
if __name__=="__main__":
    parser = optparse.OptionParser()
    parser.add_option("-s", "--server", action="store_false", dest="client_mode", help="server mode")
    parser.add_option("-c", "--client", action="store_true", dest="client_mode", help="client mode", default=True)
    parser.add_option("-p", "--port", type="int", dest="port", default=50000)
    parser.add_option("-n", "--num", type="int", dest="ngames", help="turnament - run n games (assumes client mode)", default=1)
    parser.add_option("-w", "--white", action="store_true", dest="is_white", help="play white (assumes client mode)", default=True)
    parser.add_option("-b", "--black", action="store_false", dest="is_white", help="play black (assumes client mode)")
    parser.add_option("-l", "--host", type="string", default="localhost", dest="host", help="hostname - assumes server mode")
    (options,args)=parser.parse_args()

    if options.is_white:
        color="white"
    else:
        color="black"

    if options.client_mode:
        ChessClient(options.host, options.port, options.ngames, color)
    else:
        ChessServer(options.port)
