/*
 *  Copyright (c) 2004 Jesper Olsen
 *  License: MIT, see License.txt
 *
 */

#include <string.h>
#include "Python.h"
#include "skak.h"
#include "eval.h"
#include "search.h"
#include "mgenerator.h"


static PyObject * Chess_new_game
(
    PyObject *self,
    PyObject *args
)
{
    ChessGame* h;
    if(!PyArg_ParseTuple(args,":Chess_new_game")) return NULL;
    h=new_chess_game();
    return Py_BuildValue("l", h);
} /* Chess_new_game */

    
static PyObject * Chess_free_game
(
    PyObject *self,
    PyObject *args
)
{
    long l;
    ChessGame* h;
    if(!PyArg_ParseTuple(args,"l", &l)) return NULL;
    h=(ChessGame*) l;
    free_chess_game(h);
    Py_INCREF(Py_None);
    return Py_None;
} /* Chess_free_game */


static PyObject * Chess_setup
(
    PyObject *self,
    PyObject *args
)
{
    long l, n, nc, res=0, enp_sq;
    char *fene, ch, *castling;
    ChessGame* cg;

    if(!PyArg_ParseTuple(args,"ls#cs#i", 
        &l, &fene, &n, &ch, &castling, &nc, &enp_sq)) return NULL;
    cg=(ChessGame*) l;
    
    if (n==64) {
        if (ch=='w')
            cg->color=WHITE;
        else
            cg->color=BLACK;

        if (enp_sq>0) {
            if ((enp_sq % 8)==2) 
                cg->last.enpassant_cap=cg->board[enp_sq+1];
            else
                cg->last.enpassant_cap=cg->board[enp_sq-1];
        }
            
        res=setup(cg, fene, castling, nc);
    } else
        res=0;

    return Py_BuildValue("i", res);
} /* Chess_setup */
    

static PyObject * Chess_get_position
(
    PyObject *self,
    PyObject *args
)
{
    long l, enp_sq;
    char fene[65], castling[5];
    ChessGame* cg;

    if(!PyArg_ParseTuple(args,"l", &l)) return NULL;
    cg=(ChessGame*) l;

    if (cg->last.enpassant_cap==NULL)
        enp_sq=-1;
    else
        enp_sq=cg->last.enpassant_cap->koor;

    get_fene(cg, fene); 
    get_castling_rights(cg, castling);
    return Py_BuildValue("ssl", fene, castling, enp_sq);
} /* Chess_get_position */


static PyObject * Chess_game_over
(
    PyObject *self,
    PyObject *args
)
{
    long l;
    ChessGame* h;

    if(!PyArg_ParseTuple(args,"l", &l)) return NULL;
    h=(ChessGame*) l;

    return Py_BuildValue("i", game_over(h));
} /* Chess_game_over */
    

static PyObject * Chess_post_mortem
(
    PyObject *self,
    PyObject *args
)
{
    long l;
    ChessGame* h;
    char* msg;

    if(!PyArg_ParseTuple(args,"l", &l)) return NULL;
    h=(ChessGame*) l;

    if (game_over(h)) 
        return Py_BuildValue("s", post_mortem(h));

    if (h->color==WHITE)
        msg="White's Turn";
    else
        msg="Black's Turn";
    return Py_BuildValue("s", msg);
} /* Chess_post_mortem */


static PyObject * Chess_get_turn
(
    PyObject *self,
    PyObject *args
)
{
    long l;
    ChessGame* h;

    if(!PyArg_ParseTuple(args,"l", &l)) return NULL;
    h=(ChessGame*) l;

    if (h->color==WHITE)
        return Py_BuildValue("s", "white");
    else
        return Py_BuildValue("s", "black");
} /* Chess_get_turn */


static PyObject * Chess_compute_move
(
    PyObject *self,
    PyObject *args
)
{
    long l, depth, max_search;
    float max_time;
    ChessGame* cg;
    Move best_reply;

    if(!PyArg_ParseTuple(args,"liif", &l, &depth, &max_search, &max_time)) return NULL;
    cg=(ChessGame*) l;
    if (depth<1)
        cg->depth=1;
    else
        cg->depth=depth;
      
    cg->max_search=max_search;
    cg->max_time=max_time;
    compute_move(cg, &best_reply);
    return Py_BuildValue("(iiiiii)", cg->next.from, cg->next.to, cg->next.kill!=NULL, cg->next.val, cg->number, cg->depth_actual);
} /* Chess_compute_move */


static PyObject * Chess_make_move
(
    PyObject *self,
    PyObject *args
)
{
    long l, from, to;
    ChessGame* cg;

    if(!PyArg_ParseTuple(args,"lii", &l, &from, &to)) return NULL;
    cg=(ChessGame*) l;
    cg->next.from=(unsigned char) from;
    cg->next.to=(unsigned char) to;
    return Py_BuildValue("i", update_game(cg));
} /* Chess_make_move */


static PyObject * Chess_in_check 
(
    PyObject *self,
    PyObject *args
)
{
    long l;
    ChessGame* cg;

    if(!PyArg_ParseTuple(args,"l", &l)) return NULL;
    cg=(ChessGame*) l;
    return Py_BuildValue("i", in_check(cg, cg->color));
} /* Chess_in_check */


static PyObject * Chess_get_possible
(
    PyObject *self,
    PyObject *args
)
{
    long l, i;
    ChessGame* cg;
    PyObject *p, *item;

    Move pos[100];
    int num;

    if(!PyArg_ParseTuple(args,"l", &l)) return NULL;
    cg=(ChessGame*) l;
    num=get_pos(cg, &cg->last, cg->color, pos);

    p=PyList_New(0);
    for (i=0; i<num; i++)
        if (is_legal(cg, &pos[i])) {
            item=Py_BuildValue("(iii)", pos[i].from, pos[i].to, pos[i].kill!=NULL);
            PyList_Append(p, item); Py_DECREF(item);
        }
    return p;
} /* Chess_get_possible */


static PyObject * Chess_get_pieces
(
    PyObject *self,
    PyObject *args
)
{
    long l, i;
    ChessGame* cg;
    PyObject *p, *item;

    if(!PyArg_ParseTuple(args,"l", &l)) return NULL;
    cg=(ChessGame*) l;

    p=PyList_New(0);
    for (i=0; i<32; i++)
        if (cg->piece[i].alive==1) {
            item=Py_BuildValue("(cii)", cg->piece[i].type, 
                                        cg->piece[i].koor,
                                        (cg->piece[i].color==WHITE));
            PyList_Append(p, item); Py_DECREF(item);
        }
    return p;
} /* Chess_get_pieces */


static PyObject * Chess_display
(
    PyObject *self,
    PyObject *args
)
{
    long l;
    ChessGame* h;
    if(!PyArg_ParseTuple(args,"l", &l)) return NULL;
    h=(ChessGame*) l;
    PrintBoard(h);
    Py_INCREF(Py_None);
    return Py_None;
} /* Chess_display */


static PyMethodDef ChessMethods[] =
{
    {"new_game", Chess_new_game, METH_VARARGS, "just that"},
    {"free_game", Chess_free_game, METH_VARARGS, "deallocate"},
    {"setup", Chess_setup, METH_VARARGS, "setup a new position"},
    {"game_over", Chess_game_over, METH_VARARGS, "true if game is over"},
    {"post_mortem", Chess_post_mortem, METH_VARARGS, "Game status"},
    {"get_turn", Chess_get_turn, METH_VARARGS, "black | white"},
    {"compute_move", Chess_compute_move, METH_VARARGS, "return move"},
    {"make_move", Chess_make_move, METH_VARARGS, "update game"},
    {"display", Chess_display, METH_VARARGS, "display chess baord"},
    {"get_position", Chess_get_position, METH_VARARGS, "get chess position - expanded fen notation"},
    {"get_possible", Chess_get_possible, METH_VARARGS, "get possible moves"},
    {"get_pieces", Chess_get_pieces, METH_VARARGS, "get list of live pieces"},
    {"in_check", Chess_in_check, METH_VARARGS, "true if King in check"},
    {NULL, NULL, 0, NULL}               /* Sentinel */
};


void initChess(void)
{
    Py_InitModule("Chess", ChessMethods);    
}
    


