#!/usr/local/bin/python
""" Copyright (c) 2005 Jesper Olsen
    License: MIT, see License.txt

    CGI based chess program - output is HTML, with JavaScript to
    allow move selection using the mouse...
"""

import cgitb; cgitb.enable()
import cgi, os, sys, time, string, glob
import Cookie
sys.path+=["lib/python"]
import PChess, cgiskak
from genshi import XML, Markup
from genshi.template import MarkupTemplate
import collections

def host2country(rhost):
    country=""
    if rhost!="":
        words=rhost.split(".")
        if len(words)>0 and len(words[-1])==2:
            country=words[-1]
    if country=="uk":
        country="gb"
    return country

def host2flag(rhost):
    ext=host2country(rhost)
    if ext=="": return ""
    oponent="http://flagspot.net/images/%s/%s.gif"%(ext[0],ext)
    return oponent

def get_game_parameters(data):
    keys=data.keys()
    keys.sort()
    l=[]
    for key in keys:
        if key in ["User", "Moves", "REMOTE_HOST", "REMOTE_ADDR", "Start"]:
            continue
        if key=="remote_host":
            flag=host2flag(data[key])
            if flag!="":
                l+=[("player h", flag)]
        else:
            l+=[(key, data[key])]
    return l

def is_int(s):
    try:
        a=int(s)
    except:
        return False
    return True

def read_stat(filename):
    data={}
    N=-1
    NM=-1
    for line in open(filename, "r"):
        words=line.split()
        if len(words)<=1: continue
        if is_int(words[0]):
            N=int(words[0])
            NM=max(NM,int(words[0]))
        data[words[0]]=words[1:]

    t0=int(data["Start"][0])
    moves=[]
    for i in range(N+1):
        t=data[str(i)]
        frm=int(t[1])
        to=int(t[2])
        is_kill=int(t[3])
        time=int(t[4])
        d=""
        if len(t)>5:
            score=t[5]
            n=t[6]
            if len(t)>7:
                d=t[7]
        else:
            score=""
            n=""
        moves+=[[frm,to,is_kill,t[0],str(time-t0),score,n,d]]
        t0=time
    for i in range(NM+1):
        del data[str(i)]
    d=data["Date"]
    for key in data:
        data[key]=data[key][0]
    data["Date"]="%s %s (GMT)" % (string.join(d[0:3], "-"), string.join(d[3:6], ":"))
    data["Moves"]=moves
    return data

def get_status(filename):
    data=read_stat(filename)
    cg=PChess.PChess()
    prev=None
    change=False
    for i, tup in enumerate(data["Moves"]):
        cg.make_move(tup[0],tup[1])
        if tup[3]==prev:
            change=True
        prev=tup[3]
    if cg.game_over():
        msg=cg.post_mortem()
    else:
        msg="Unfinished"
    try:
        host=data["remote_host"]
    except:
        host=""
    return msg, len(data["Moves"]), host, change
    
def review(id, gobackto):
    try:
        data=read_stat("GAMES/game_%s.stat"%(id))
    except:
        print """Content-Type: text/html\n"""
        #print "Sorry - no game records were found"
        print "<html><head></head>\n<body>Sorry - no game records were found: %s; </body></html>\n" %(id,)
        sys.exit(1)
    
    cg=PChess.PChess()
    boards=[]
    for i, tup in enumerate(data["Moves"]):
        cg.make_move(tup[0],tup[1])
        boards+=[(cgiskak.str_board(cg), cgiskak.get_status(cg))]

    print """Content-Type: text/html\n"""

    tmpl=MarkupTemplate(open("Genshi/review.html", 'r').read())
    stream=tmpl.generate(moves=data["Moves"], boards=boards, info=get_game_parameters(data))
    #TODO wstrip still necessary?
    #print cgiskak.wstrip( stream.render(doctype="html") , "td")
    print stream.render(doctype="xhtml-transitional")


def get_game_info(fname):
    try:
        msg,nmoves,rhost,change=get_status(fname)
    except:
        return None
    if nmoves==0: 
        return None
    name, ext=os.path.splitext(fname)
    id=name[11:]
    t=os.path.getmtime(fname)
    date=time.strftime("%Y-%b-%d (%A) %H:%M:%S (GMT)", time.gmtime(t))
    label=date+"; %s (%d half moves)"%(msg,nmoves)
    url="cgireview.py?gameid="+id
    return label, url, rhost, nmoves, change

def lookup_games():
    fnames=glob.glob("GAMES/*.stat")
    fnames.sort(None, os.path.getmtime, True)
    return fnames

def get_games(offset, N, l=None):
    if l==None:
        l=lookup_games()
    l2=[]
    i=0
    for fname in l[offset:]:
        if i>=N:
            break
        tup=get_game_info(fname)
        if tup==None: continue
        l2+=[tup]
        i+=1
    return l2

def html_list_games(offset):
    N=20
    l=lookup_games()
    prev=None
    next=None
    if offset>0:
        prev="cgireview.py?list=1&offset=%d"%(offset-N)
    if offset+N<len(l):
        next="cgireview.py?list=1&offset=%d"%(offset+N)

    games=[]
    for i, (label, url, rhost, nmoves, change) in enumerate(get_games(offset, N, l)):
        flag=None
        if rhost!="":
            flag=host2flag(rhost)
        games+=[(i+offset+1, url, label, flag)]
    print """Content-Type: text/html\n"""
    tmpl=MarkupTemplate(open("Genshi/list.html", 'r').read())
    stream=tmpl.generate(next=next, prev=prev, games=games)
    print stream.render(doctype="html") 

def local2gmt(s):
    w=s.split(":")
    h=int(w[0]) + time.timezone/3600
    h=str(h)
    if len(h)==1: h="0"+h
    return "%s:%s:%s" % (h, w[1], w[2])

def rss_list_games(filename):
    RURL="http://JesperOlsen.Net/PChess/"
    l=[]
    for label, url, rhost, nmoves, change in get_games(0,50):
        country=host2country(rhost)
        if country!="":
            label+="; Country: "+country
        l+=[(label,RURL+url)]
    w=time.asctime().split()
    now="%s, %s %s %s %s GMT" % (w[0],w[2],w[1],w[4], local2gmt(w[3]))

    tmpl=MarkupTemplate(open("Genshi/rss.xml", 'r').read())
    stream=tmpl.generate(games=l, time=now)
    open(filename, "w").write( stream.render(doctype="xhtml")  )

def main():
    form=cgi.FieldStorage()
    cookie=Cookie.SimpleCookie()
    try:
        cookie.load(os.environ["HTTP_COOKIE"])
    except:
        pass

    try:
        gobackto=int(form.getfirst("gobackto", -1))
        gameid=form.getfirst("gameid", "")
        if gameid=="":
            gameid="%s_%s"%(cookie["user"].value,cookie["game"].value)
        offset=int(form.getfirst("offset", "0"))
    except:
        gobackto=-1
        gameid=""
        offset=0

    if form.has_key("list") or gameid=="":
        html_list_games(max(0,offset))
    else:
        review(gameid, gobackto)

def cleanup():
    """Remove 0 move games"""
    count_finished=0
    count_unchange=0
    dic=collections.defaultdict(int)
    first=collections.defaultdict(int)
    for fname in lookup_games():
        tup=get_game_info(fname)
        if tup==None:
            base, ext=os.path.splitext(fname)
            print "Remove:", fname 
            os.remove(base+".stat")
            os.remove(base+".txt")
        else:
            (label,url,rhost,nmoves,change)=tup
            i=label.find("Unfinished")
            if (i>0):
                dic[nmoves]+=1
            else:
                count_finished+=1
                if not change:
                    count_unchange+=1
                    data=read_stat(fname)
                    tup=data["Moves"][0]
                    first[PChess.get_label(tup[:2])]+=1
    total=sum(dic.values())
    cum=0
    for key in range(max(dic.keys())+1):
        if dic[key]==0: continue
        cum+=dic[key]
        print "#moves: %d; #games: %d (%.2f)" %(key, dic[key], 100.0*cum/total)
    print "Total unfinished games:", total
    print "Total finished games:", count_finished
    print "Total finished & unchanged:", count_unchange
    for key in first:
        print key, ":", first[key]


if __name__=="__main__":
    if len(sys.argv)>1 and sys.argv[1]=="-rss":
        rss_list_games("review.rss")
    elif len(sys.argv)>1 and sys.argv[1]=="-cleanup":
        cleanup()
    elif len(sys.argv)>1 and sys.argv[1]=="-gameid":
        gameid=sys.argv[2]
        review(gameid, -1)
    else:
        main()
