from flask_wtf import FlaskForm
from wtforms import StringField, IntegerField, SubmitField
from wtforms.validators import DataRequired, NumberRange
from flask import render_template, flash, redirect, url_for
from flask import request
from flask import make_response
from app import app
import Chess
import uuid
import os
import glob
import re
import json
import time
import pprint
import datetime

def sq_is_black(i):
    x=7-i//8
    y=i % 8
    return x % 2==y % 2

def xy2src(x,y,board):
    i=(7-x)*8+y
    src="/static/bitmaps/"
    if sq_is_black(i):
        src+="b-"
    else:
        src+="w-"
    if board[i]!='.':
        if board[i]==board[i].upper():
            src+='w'
        else:
            src+='b'
    return src+board[i]+".gif"


@app.route('/')
@app.route('/index')
def index():
    return redirect(url_for('pchess'))

@app.route('/PChess')
def pchess():
    status="Fancy a nice game of chess?"
    board="."*64
    l=[]
    gid=request.cookies.get('game_id')
    uid=request.cookies.get('user_id')
    if gid!=None and uid!=None:
        fname="GAMES/{}_{}.txt".format(uid,gid)
        cg,info=read_game(fname)
        if cg!=None:
            board=cg.to_string()
            l=cg.get_possible()
            status=cg.get_status()
    return render_template('main.html', status=status, board=board, legal=l, xy2src=xy2src)

@app.route('/recent')
@app.route('/recent/<offset>')
def recent(offset=0):
    print("offset=",offset)
    offset=int(offset)
    N=20
    l=lookup_games()
    prev=None
    nxt=None
    if offset>0:
        prev="/recent/{}".format(offset-N)
    if offset+N<len(l):
        nxt="/recent/{}".format(offset+N)

    games=[]
    for i, (label, url, rhost, nmoves) in enumerate(get_games(offset, N, l)):
        flag=None
        if rhost!="":
            flag=host2flag(rhost)
            games+=[(i+offset+1, url, label, flag)]
            #games+=[{'n':i+offset+1, 'url':url, 'label':label, 'flag':flag}]
    return render_template('list.html', prev=prev, nxt=nxt, games=games)

def host2country(rhost):
    country=""
    if rhost=="": return country

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

@app.route('/review/<gid>')
@app.route('/review')
def review(gid=""):                                                       
    cg=None                                                                     
    if gid!='':                                                                 
        fname="GAMES/%s.txt"%(gid,)                                             
        if os.path.exists(fname): cg,info=read_game(fname)                      
    if cg==None:                                                                
        uid=request.cookies.get('user_id')
        gid=request.cookies.get('game_id')
        fname="GAMES/{}_{}.txt".format(uid,gid)                                 
        cg,info=read_game(fname)                                                
                                                                                
    moves=cg.log                                                                
    cg=Chess.PChess()                                                          
    boards=[]                                                                   
    board=cg.to_string()                                                        
    for i, tup in enumerate(moves):                                             
        cg.make_move(tup[0],tup[1])                                             
        boards+=[(cg.to_string(),cg.get_status())]                              
                                                                                
    gdir=os.path.dirname(os.path.realpath(__file__))                            
    ifo={}                                                                      
    flag=""
    if "remote_host" in info:
        flag=host2flag(info["remote_host"])                                         
    if flag!="":ifo["player h"]=flag                                            
                                                                                
    print(info['stat'])                                                         
                                                                                
    moves=[]                                                                    
    for d in info['stat']:
        t=float(d['time'])
        value = datetime.datetime.fromtimestamp(t)
        t=value.strftime('%Y-%m-%d %H:%M:%S')                                   
        moves+=[(d['from'], d['to'], d['is_kill'], t, d['val'], d['searched'], d['depth_actual'])]
                                                                                
    return render_template('review.html', moves=moves, board=board, status="White's turn", boards=boards, info=ifo, xy2src=xy2src, get_label=Chess.get_label, enumerate=enumerate, len=len)

def new_gid(gdir,uid):
    #max old gid + 1
    p = re.compile(r'_(?P<gid>[0-9]+).txt$')
    g=glob.glob("{}/{}_*.txt".format(gdir,uid))
    l=(p.search(s) for s in g)
    l=(m.group('gid') for m in l if m!=None)
    l=[int(m) for m in l]
    if l==[]: return '1'
    return str(max(l)+1)

def save_game(cg, request, fname, info):
    with open(fname, 'w') as f:
        if request.origin!=None: 
            info["http_host"]=request.origin
        info["remote_addr"]=request.remote_addr
        f.write(cg.to_json()+'\n')
        f.write(json.dumps(info))

def read_game(fname, max_move=None):
    if not os.path.exists(fname):
        return None, None
    
    cg=Chess.PChess()
    d={"stat":[]}
    with open(fname, 'r') as f:
        cg.from_json(f.readline(), max_move)
        d=json.loads(f.readline())
    return cg, d

def get_game_info(fname):
    try:
        msg,nmoves,rhost=get_status(fname)
    except:
        raise
        return None
    if rhost=="": rhost='unk'
    if nmoves==0:
        return None
    name, ext=os.path.splitext(fname)
    gid=name[6:]
    t=os.path.getmtime(fname)
    date=time.strftime("%Y-%b-%d (%A) %H:%M:%S (GMT)", time.gmtime(t))
    label=date+"; %s (%d half moves)"%(msg,nmoves)
    url="/review/"+gid
    return label, url, rhost, nmoves

def get_status(filename):
    cg, d=read_game(filename)
    if cg.game_over():
        msg=cg.post_mortem()
    else:
        msg="Unfinished"
    host="unk"
    if "remote_host" in d:
        host=d["remote_host"]
    return msg, len(cg.log), host

def lookup_games():
    fnames=glob.glob("GAMES/*.txt")
    fnames.sort(key=os.path.getmtime)
    return fnames

def get_games(offset, N, l=None):
    if l==None:
        l=lookup_games()
    l2=[]
    i=0
    for fname in l[offset:]:
        print(fname)
        if i>=N:
            break
        tup=get_game_info(fname)
        if tup==None: continue
        l2+=[tup]
        i+=1
    return l2

def xml_response(cg):
    legal=cg.get_possible()
    if len(cg.log)==0:
        lastFrom,lastTo=-1,-1
    else:
        t=cg.log[-1]
        lastFrom,lastTo=t[0],t[1]

    resp=make_response( render_template('chessgame.xml', board=cg.to_string(), \
                        status=cg.get_status(), \
                        color=cg.get_turn(), \
                        lastFrom=lastFrom, \
                        lastTo=lastTo, \
                        legal=str(legal), \
                        nmoves=len(cg.log)//2+1) )
    resp.headers['Content-Type'] = 'application/xml'
    return resp

@app.route('/new')
def new():
    uid=request.cookies.get('user_id')
    if uid==None: uid=str(uuid.uuid4())

    gdir="GAMES"
    if not os.path.exists(gdir):
        os.mkdir(gdir)
    #gid=str(len(glob.glob("{}/{}_*.txt".format(gdir,uid))))
    gid=new_gid(gdir,uid)

    fname="GAMES/{}_{}.txt".format(uid,gid)
    cg=Chess.PChess()
    info={"stat":[]}
    save_game(cg, request, fname, info)

    resp=xml_response(cg)
    resp.set_cookie('user_id',uid)
    resp.set_cookie('game_id',gid)
    return resp

@app.route('/change')
def change():
    uid=request.cookies.get('user_id')
    gid=request.cookies.get('game_id')
    fname="GAMES/{}_{}.txt".format(uid,gid)

    cg,info=read_game(fname)
    if cg==None:
        cg=Chess.PChess()
        info={"stat":[]}
    elif not cg.game_over():
        move=cg.compute_move()
        is_kill=cg.make_move(move['from'],move['to'])
        move['time']=time.time()
        info["stat"]+=[move]
    save_game(cg, request, fname, info)
    return xml_response(cg)

@app.route('/move/<frm>/<to>')
def move(frm, to):
    uid=request.cookies.get('user_id')
    gid=request.cookies.get('game_id')
    fname="GAMES/{}_{}.txt".format(uid,gid)
    cg,info=read_game(fname)

    move=(int(frm),int(to))
    is_kill=cg.make_move(move[0], move[1])
    move= {'from': int(frm),
           'to': int(to),
           'is_kill': is_kill,
           'val': 0,
           'searched': 0,
           'depth_actual': 0,
           'time': time.time()}
    info["stat"]+=[move]

    if not cg.game_over():
        move=cg.compute_move()
        is_kill=cg.make_move(move['from'],move['to'])
        move['time']=time.time()
        info["stat"]+=[move]
    save_game(cg, request, fname, info)

    return xml_response(cg)

class PrefForm(FlaskForm):
    ply = IntegerField('Max Ply', validators=[DataRequired(), NumberRange(min=0, max=25, message='haha')])
    search = IntegerField('Max Search', validators=[DataRequired(), NumberRange(min=0, max=200000, message='haha')])
    submit = SubmitField('Save')

@app.route('/settings', methods=['GET', 'POST'])
def settings():
    form = PrefForm()
    if request.method=='GET':
        ply=request.cookies.get('ply')
        if ply==None: ply=25
        else: ply=int(ply)
        search=request.cookies.get('search')
        if search==None: search=100000
        else: search=int(search)
        form.ply.data=ply
        form.search.data=search
    resp=make_response( render_template('pref.html', form=form) )
    if form.validate_on_submit():
        resp.set_cookie('ply',str(form.ply.data))
        resp.set_cookie('search',str(form.search.data))
        flash('Saving preferences')
    return resp
