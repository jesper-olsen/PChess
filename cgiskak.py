#!/usr/local/bin/python
""" Copyright (c) 2004-2005 Jesper Olsen
    License: MIT, see License.txt

    CGI based chess program - output is HTML, with JavaScript to
    allow move selection using the mouse...
"""

import cgitb; cgitb.enable()
import cgi, os, sys, time, os, string, StringIO, glob
import Cookie
from genshi import XML, Markup
from genshi.template import MarkupTemplate
sys.path=["lib/python"]+sys.path
import PChess

COOKIE_PATH=""

# GAME PREFERENCES - min,max,default
PREF={}
PREF["max_ply"]=(0,25,25)            # search depth
PREF["max_search"]=(0,200000,100000) # n positions to search


def wstrip(data, tag="td"):
    """Strip leading/trailing white space in the text block of tag
       -- to compensate for kid 0.8's introduction of white space
    """
    o=""
    BEGIN="<"+tag+">"
    END="</"+tag+">"
    offset=0
    while 1:
        try:
            i1=data.index(BEGIN, offset)
            i2=data.index(END, i1)
        except:
            o+=data[offset:]
            return o
        o+=data[offset:i1]
        o+=BEGIN + string.strip(data[i1+4:i2]) + END
        offset=i2+5

def sq_is_black(i):
    x=7-i/8
    y=i % 8
    flag=(x % 2==y % 2)
    return flag

def xy2src(x,y,board):
    i=(7-x)*8+y
    src="bitmaps/"
    if sq_is_black(i):
        src+="b-"
    else:
        src+="w-"
    return src+board[i]+".gif"

def str_board(cg):
    board=list("."*64)
    for (type,i,is_white) in cg.get_pieces():
        if is_white==1:
            type=type.upper()
        else:
            type=type.lower()
        board[i]=type
    return string.join(board,"")

def get_status(cg):
    if cg.game_over():
        return cg.post_mortem()
    else:
        if cg.get_turn()=="white":
            c="White's turn"
        else:
            c="Black's turn"
        if cg.in_check():
            c="Check! " + c
        return c

def xml_response(cg):
    legal=[[frm,to] for (frm,to,kill) in cg.get_possible()]
    if len(cg.log)==0:
        lastFrom,lastTo=-1,-1
    else:
        t=cg.log[-1]
        lastFrom,lastTo=t[0],t[1]

    tmpl=MarkupTemplate(open("Genshi/chessgame.xml", 'r').read())
    stream=tmpl.generate( board=str_board(cg), \
                          status=get_status(cg), \
                          color=cg.get_turn(), \
                          lastFrom=lastFrom, \
                          lastTo=lastTo, \
                          legal=str(legal), \
                          nmoves=len(cg.log)/2+1)
    return """Content-Type: text/xml\n\n"""+stream.render(doctype='xhtml')


def display_game(cg, review):
    """Display chess board (HTML) for current position """

    if cg==None:
        status="Fancy a nice game of chess?"
        board="."*64
        l=[]
    else:
        status=get_status(cg)
        board=str_board(cg)
        l=[[frm,to] for (frm,to,kill) in cg.get_possible()]

    tmpl=MarkupTemplate(open("Genshi/main.html", 'r').read())
    stream=tmpl.generate( legal=str(l), board=board, status=status)
    return "Content-Type: text/html\n\n" + stream.render(doctype='xhtml-transitional')


def process_turn(cg, m, do_change):
    log=""
    if m!="":
        move=[int(x) for x in m.split(",")]
        is_kill=cg.make_move(move[0], move[1])
        log+="%d h %d %d %d %d\n" % (len(cg.log)-1,move[0],move[1],is_kill, time.time())
    if do_change or m!="":
        if not cg.game_over():
            (frm, to, kill, val, count, depth)=cg.compute_move()
            is_kill=cg.make_move(frm,to)
            log+="%d c %d %d %d %d %d %d %d\n" % (len(cg.log)-1,frm,to,is_kill,time.time(),val,count, depth)
    return log


def set_preferences(cg, param):
    for pname in PREF:
        pmin,pmax,pdefault=PREF[pname]
        if pname in param:
            val=param[pname]
        else:
            val=pdefault 
        val=int(val)
        if val>=pmin and val<=pmax:
            setattr(cg, pname, val)
            set_cookie(pname, val)

def show_pref(cg):
    """Display chess board (HTML) for current position """
    pref=[]
    for pname in PREF:
        pmin,pmax,pdefault=PREF[pname]
        pref+=[(pname, pmin, pmax, pdefault, getattr(cg,pname))]

    tmpl=MarkupTemplate(open("Genshi/pref.html", 'r').read())
    stream=tmpl.generate(pref=pref)
    print "Content-Type: text/html\n\n" + stream.render(doctype='html')

def set_cookie(name, value, expires=None):
    if expires==None:
        expires=time.time()+365*24*60*60
    print "Set-Cookie: %s=%s; path=/%s; expires=%s GMT" % (name, value, COOKIE_PATH, time.strftime("%A, %d-%b-%Y %H:%M:%S", time.gmtime(expires)))

def get_log_head(cg,user):
    args=["REMOTE_HOST", "REMOTE_ADDR"]
    log="Date %d %d %d %d %d %d\n" % time.gmtime()[0:6]
    log+="Start %d\n"%(time.time())
    log+="User %s\n"%(user)
    for arg in args:
        if arg in os.environ:
            val=os.environ[arg]
            log+="%s %s\n"%(arg,val)
    log+="MaxDepth %d\n" %(cg.max_ply)
    log+="MaxSearch %d\n" %(cg.max_search)
    log+="EngineName %s\n" %(cg.name)
    log+="EngineVersion %s\n" %(cg.version)

    if "REMOTE_ADDR" in os.environ:
        try:
            import socket
            (host,l1,l2)=socket.gethostbyaddr(os.environ["REMOTE_ADDR"])
            log+="remote_host %s\n" %(host)
        except:
            pass
    return log

def process(cg, new, change, param):
    if "user" in param:
        user=param["user"]
    else:
        if not os.path.exists("GAMES/IDs"):
            os.makedirs("GAMES/IDs")
        user=str(len(os.listdir("GAMES/IDs")))
        set_cookie("user", user)
        open("GAMES/IDs/"+user, "w").write(user)

    if "gameid" in param:
        gameid=param["gameid"]
    else:
        #create user id and gameid
        gameid=None

    if change and gameid==None:
        new=True

    if new:
        if gameid==None:
            gameid=0
        while os.path.exists("GAMES/game_%s_%d" % (user,gameid)):
            gameid+=1

    if gameid==None:
        print display_game(None, "")
        return

    filename="GAMES/game_%s_%d" % (user,gameid)
    if not new:
        try:
            cg.read(filename+".txt", param["gobackto"])
        except:  #game not found - or corrupted
            gameid+=1
            new=True
            filename="GAMES/game_%s_%d" % (user,gameid)

    log=None
    if new:
        set_cookie("game", gameid)
    if change or param["move"]!="":
        log=process_turn(cg, param["move"], change)

    if param["xml"]:
        print xml_response(cg)
    else:
        review="cgireview.py"
        print display_game(cg, review)

    cg.save(filename+".txt")

    if log==None:
        log=get_log_head(cg,user)
    else:
        log=get_log_head(cg,user)+log
    open(filename+".stat", "a").write(log)
        
def main():
    cg=PChess.PChess()
    form=cgi.FieldStorage()

    new=form.has_key("new")
    change=form.has_key("change")
    param={}
    param["move"]=form.getfirst("move","")
    param["gobackto"]=int(form.getfirst("gobackto", "-1"))

    #Cookies used:
    # * user       - player id
    # * game       - game id
    # * max_ply    - search depth
    # * max_search - #moves to search
    cookie=Cookie.SimpleCookie()
    try:
        cookie.load(os.environ["HTTP_COOKIE"])
    except:
        pass

    param["xml"]=form.has_key("xml")
    for pname in ["max_ply", "max_search"]:
        if form.has_key(pname):
            param[pname]=form.getfirst(pname,-1)
        elif pname in cookie:
            param[pname]=cookie[pname].value
    if "user" in cookie:
        param["user"]=str(int(cookie["user"].value))
    if "game" in cookie:
        param["gameid"]=int(cookie["game"].value)
    set_preferences(cg, param)
    if form.has_key("pref"):
        show_pref(cg)
    else:
        process(cg, new, change, param)

if __name__=="__main__":
    main()

