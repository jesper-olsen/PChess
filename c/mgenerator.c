/*
 *  Copyright (c) 2004 Jesper Olsen
 *  License: MIT, see License.txt
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "skak.h"
#include "eval.h"
#include "mgenerator.h"

const int dpawn_cap[2][2]={{-7,9},{-9,7}};
const int dpawn_enp_cap[2]={-8,8};      /* RIGHT, LEFT */
const int castle_rook_from[2][2]={{0,56},{7,63}};
const int castle_rook_to[2][2]={{16,32},{23,39}};
const int dking_castle[2]={-16, +16};
const int initial_king[2]={24,31};
const int king_idx[2]={4,28};   /* piece index */
const unsigned char first_piece_idx[2]={0,16};
const unsigned char last_piece_idx[2]={15,31};


int can_castle(ChessGame* cg, int color, unsigned int dir)
{
    int rf, kf; 

    if (!cg->CastlingOn[color][dir])
        return(0);

    rf=castle_rook_from[color][dir];
    kf=initial_king[color];

    if (!cg->board[kf] || !cg->board[rf])
        return(0);

    if (cg->board[kf]->type!=king || cg->board[rf]->type!=rook)
        return(0);

    if (dir==RIGHT) {
        if (cg->board[kf-8] || cg->board[kf-16])     /* ikke helt nok !! */
            return(0);
    } else
        if (cg->board[kf+24] || cg->board[kf+16] || cg->board[kf+8])
            return(0);

    return(!in_check(cg, color));
} /* can_castle */


int is_pawn_trans(Piece* board[], Move *c, int color)
{
    if (board[c->from]->type!=pawn)
       return (0);
    return ((c->to % 8)==7 || (c->to % 8)==0);
} /* is_pawn_trans */


// _can_capture() - referenced by function pointers
int pawn_can_capture(void* vp, int to, uint64 PieceBitmap)
{
    Piece* p=(Piece*) vp;
    return (p->koor+dpawn_cap[p->color][LEFT]==to || 
            p->koor+dpawn_cap[p->color][RIGHT]==to);
}

int knight_can_capture(void* vp, int to, uint64 PieceBitmap)
{
    Piece* p=(Piece*) vp;
    return( (Knight[p->koor] & power[to])!=0 );
}


int king_can_capture(void* vp, int to, uint64 PieceBitmap)
{
    Piece* p=(Piece*) vp;
    return( (King[p->koor] & power[to])!=0 );
}


int rook_can_capture(void* vp, int to, uint64 PieceBitmap)
{
    Piece* p=(Piece*) vp;
    return ((distance[p->koor][to] & PieceBitmap)==0 &&
            (Rook[p->koor].ray[0] & power[to])!=0 );
}


int bishop_can_capture(void* vp, int to, uint64 PieceBitmap)
{
    Piece* p=(Piece*) vp;
    return ((distance[p->koor][to] & PieceBitmap)==0 &&
            (Bishop[p->koor].ray[0] & power[to])!=0 );
}


int queen_can_capture(void* vp, int to, uint64 PieceBitmap)
{
    Piece* p=(Piece*) vp;
    return ((distance[p->koor][to] & PieceBitmap)==0 &&
            ((Rook[p->koor].ray[0] & power[to])!=0 ||
             (Bishop[p->koor].ray[0] & power[to])!=0 ));
}

int in_check_n(ChessGame* cg)
{
    return in_check(cg, cg->color);
}

int in_check(ChessGame* cg, int color)
{
    Piece *p;
    Piece *first=&cg->piece[first_piece_idx[!color]];
    Piece *last=&cg->piece[last_piece_idx[!color]];
    uint64 PieceBitmap = cg->bmpieces[BLACK] | cg->bmpieces[WHITE];
    int to=cg->piece[king_idx[color]].koor;

    for (p=first; p<=last; p++)
        if (p->alive && p->can_capture(p, to, PieceBitmap)) 
            return 1;
    return(0);
} /* in_check */


void move_eq(Move* move1, Move* move2)
{
    if (move2!=NULL) 
        *move1=*move2;
    else {
        move1->castle=move1->transform=move1->en_passant = 0; 
        move1->from = 0;
        move1->to = 0;
        move1->val = 0;
        move1->kill = NULL;
        move1->enpassant_cap=0; /* can't be captured en passant */
        move1->hash_key = 0;
    }
} // move_eq


// for sorting
static int CompareWhite(const void* v1, const void* v2)
{
    Move* s1=(Move*) v1;
    Move* s2=(Move*) v2;
    return(s2->val - s1->val);
} /* compare */


// for sorting
static int CompareBlack(const void* v1, const void* v2)
{
    Move* s1=(Move*) v1;
    Move* s2=(Move*) v2;
    return(s1->val - s2->val);
} /* compare */


void assign_move(Piece **board, int from, Move* pos, int to)
{
      Piece *k;

      pos->castle=pos->transform=pos->en_passant = 0; 
      pos->from = from;
      pos->to = to;
      pos->val =   *(board[pos->from]->val + pos->to) 
                 - *(board[pos->from]->val + pos->from);
      pos->kill = k = board[pos->to];
      pos->enpassant_cap=0; /* can't be captured en passant */
      pos->hash_key = board[from]->hash_key[from] ^
                      board[from]->hash_key[to];
      if (pos->kill) {
          pos->val -= *(k->val + k->koor);
          pos->hash_key ^= board[k->koor]->hash_key[k->koor];
      }
} /* assign_move */

void GetPawnCPos(Piece *board[], Piece *p,
                 Move *last_move,
                 int color, unsigned int dir, unsigned int *n, Move pos[])
{
    Piece *k;
    int s = p->koor+dpawn_cap[color][dir];
    if (s>=0 && s<=63) {    
       if (board[s]) {
           if (board[s]->color != color) {
              assign_move(board, p->koor, &pos[*n], s);
              if ((s % 8)==7 || (s % 8)==0) {
                  pos[*n].hash_key ^= pawn_key[color][s] ^ queen_key[color][s];
                  pos[*n].transform=1;
              } 
              (*n)++;
            }
       } else if (board[p->koor+dpawn_enp_cap[dir]]!=NULL &&
                  board[p->koor+dpawn_enp_cap[dir]]==last_move->enpassant_cap &&
                  board[p->koor+dpawn_enp_cap[dir]]->color!=color) {
           pos[*n].castle=pos->transform=0;
           pos[*n].enpassant_cap=0; /* can't be captured en passant */
           pos[*n].en_passant = 1; 
           pos[*n].from = p->koor;
           pos[*n].to = s;
           pos[*n].val =   *(board[pos[*n].from]->val + pos[*n].to) 
                      - *(board[pos[*n].from]->val + pos[*n].from);
           pos[*n].kill = k = board[pos[*n].from+dpawn_enp_cap[dir]];
           pos[*n].val -= *(k->val + k->koor);
           pos[*n].hash_key = 
                board[p->koor]->hash_key[p->koor] ^
                board[p->koor]->hash_key[s] ^
                board[k->koor]->hash_key[k->koor];
           (*n)++;
       }
   }
} /* GetPawnCPos */

void GetPawnNonCaptures(Piece *board[], Piece *p,
                        unsigned int *n, Move pos[])
{
    const int dy_pawn[2]={1,-1};
    const int pawn_start_row[2]={1,6};
    int s = p->koor + dy_pawn[p->color];  /* 1 step forward */ 

    if (!board[s]) {    /* empty        */
        assign_move(board, p->koor, &pos[*n], s);
        if ((s % 8)==7 || (s % 8)==0) {
            pos[*n].hash_key ^= pawn_key[p->color][s] ^ queen_key[p->color][s];
            pos[*n].transform=1;
        } 
        (*n)++;

        /* 2 steps forward */
        if ((p->koor % 8) == pawn_start_row[p->color]) {
            s+=dy_pawn[p->color];
            if (!board[s]) {   /* empty        */
                assign_move(board, p->koor, &pos[*n], s);
                pos[*n].enpassant_cap=board[p->koor]; /* can be captured enpassant */
                pos[*n].hash_key ^= en_passant_key[s];
                (*n)++;
            }
        }
    } 
} /* GetPawnNonCaptures */


void GetRookBlockers(ChessGame* cg, int s, uint64 *blockers)
{
    uint64 t = cg->bmpieces[WHITE] | cg->bmpieces[BLACK];
    blockers[0] = (Rook[s].ray[1] & t);
    blockers[1] = (Rook[s].ray[2] & t);
    blockers[2] = (Rook[s].ray[3] & t);
    blockers[3] = (Rook[s].ray[4] & t);
} // GetRookBlockers


void GetBishopBlockers(ChessGame* cg, int s, uint64 *blockers)
{
    uint64 t = cg->bmpieces[WHITE] | cg->bmpieces[BLACK];
    blockers[0] = Bishop[s].ray[1] & t;
    blockers[1] = Bishop[s].ray[2] & t;
    blockers[2] = Bishop[s].ray[3] & t;
    blockers[3] = Bishop[s].ray[4] & t;
} // GetBishopBlockers


/* Ray1 = x+ */
int GetBlockerInRookRay1(uint64 EnemyPieces, uint64 blockers, int from, int* bit)
{
    for (*bit=from-8; *bit>=0; *bit-=8) 
        if (blockers & power[*bit]) 
            return ((power[*bit] & EnemyPieces)!=0);
    return 0;
} /* GetBlockerInRookRay1 */


/* Ray2 = x- */
int GetBlockerInRookRay2(uint64 EnemyPieces, uint64 blockers, int from, int* bit)
{
    for (*bit = from+8; *bit<64; *bit+=8) 
        if (blockers & power[*bit]) 
            return ((power[*bit] & EnemyPieces)!=0);
    return(0); 
} /* GetBlockerInRookRay2 */


/* Ray2 = y+ */
int GetBlockerInRookRay3(uint64 EnemyPieces, uint64 blockers, int from, int* bit)
{
    for (*bit=from+1;*bit<64; *bit+=1) 
        if (blockers & power[*bit]) 
            return ((power[*bit] & EnemyPieces)!=0);
   return(0);    
} /* GetBlockerInRookRay3 */


/* Ray4 = y- */
int GetBlockerInRookRay4(uint64 EnemyPieces, uint64 blockers, int from, int* bit)
{
    for (*bit=from-1;*bit>=0;*bit-=1)
        if (blockers & power[*bit])
            return ((power[*bit] & EnemyPieces)!=0);
    
    return(0);  
} /* GetBlockerInRookRay4 */


/* Find blocker i bishop ray 1 (x+, y+) */
int GetBlockerInBishopRay1(uint64 EnemyPieces, uint64 blockers, int from, int *bit)
{
    for (*bit=(from-7); *bit>=0; *bit-=7)
       if (blockers & power[*bit]) 
          return ((power[*bit] & EnemyPieces)!=0);
    return(0);
} /* GetBlockerInBishopRay1 */


/* Find blocker i bishop ray 2 (x+, y-) */
int GetBlockerInBishopRay2(uint64 EnemyPieces, uint64 blockers, int from, int* bit)
{
    for (*bit=(from-9); *bit>=0; *bit-=9)
       if (blockers & power[*bit]) 
          return ((power[*bit] & EnemyPieces)!=0);
    return(0); 
} /* GetBlockerInBishopRay2 */


/* Find blocker i bishop ray 3 (x-, y-) */
int GetBlockerInBishopRay3(uint64 EnemyPieces, uint64 blockers, int from, int* bit)
{
    for (*bit=from+7;*bit<64; *bit+=7)
        if (blockers & power[*bit]) 
            return ((power[*bit] & EnemyPieces)!=0);
    return(0);
} /* GetBlockerInBishopRay3 */


/* Find blocker i bishop ray 4 (x-, y+) */
int GetBlockerInBishopRay4(uint64 EnemyPieces, uint64 blockers, int from, int* bit)
{
    for (*bit=from+9; *bit<64; *bit+=9)
        if (blockers & power[*bit]) 
            return ((power[*bit] & EnemyPieces)!=0);
    return(0); 
} /* GetBlockerInBishopRay4 */


int GetRookCaptures(ChessGame* cg, Piece *p,
                     unsigned int *mobility,
                     Move pos[],
                     uint64 EnemyPieces)
{
    uint64 blockers[4];
    int bit, n=0;
    
    GetRookBlockers(cg, p->koor, blockers);

    /* Captures on ray 1 (x+) */
    if (blockers[0]!=0) {
       if (GetBlockerInRookRay1(EnemyPieces, blockers[0], p->koor, &bit)) 
           assign_move(cg->board, p->koor, &pos[n++], bit);
       else (*mobility)--;
       *mobility += p->koor/8 - bit/8;
    }
    else *mobility += p->koor/8;

    /* Captures on ray 2 (x-) */
    if (blockers[1]!=0) {
       if (GetBlockerInRookRay2(EnemyPieces, blockers[1], p->koor, &bit)) 
           assign_move(cg->board, p->koor, &pos[n++], bit);
       else (*mobility)--;
       *mobility += bit/8 - p->koor/8;
    }
    else *mobility += 7-p->koor/8;

    /* Captures on ray 3 (y+) */
    if (blockers[2]!=0) {
       if (GetBlockerInRookRay3(EnemyPieces, blockers[2], p->koor, &bit)) 
           assign_move(cg->board, p->koor, &pos[n++], bit);
       else {
           if (cg->board[bit]->type==rook)      /* dobbel rook */
               *mobility+=8;
           else
               (*mobility)--;
       }
       *mobility += (bit % 8) - (p->koor % 8);
    }
    else *mobility += 7-(p->koor % 8);

    /* Captures on ray 4 (y-) */
    if (blockers[3]!=0) {
       if (GetBlockerInRookRay4(EnemyPieces, blockers[3], p->koor, &bit)) 
           assign_move(cg->board, p->koor, &pos[n++], bit);
        else {
           if (cg->board[bit]->type==rook)      /* dobbel rook */
               *mobility+=8;
           else
               (*mobility)--;
        }
        *mobility += (p->koor % 8) - (bit % 8);
    }
    else *mobility += (p->koor % 8);
    return n;
} /* GetRookCaptures */


/* Extract the moves indicated in the bitmap */
int ExtractFromBitmap(Piece **board, Piece *p, Move* pos, uint64 b)
{
    int i, n=0; 

    if (b>0)
        for (i=0; i<64; i++) 
            if (b & power[i]) 
                assign_move(board, p->koor, &pos[n++], i);
    return n;
} /* ExtractFromBitmap */


int GetBishopCaptures(ChessGame* cg, Piece* p,
                       unsigned int *mobility, Move pos[],
                       uint64 EnemyPieces)
{
    uint64 blockers[4];
    int bit, n=0;

    GetBishopBlockers(cg, p->koor, blockers);

    /* Captures on ray 1 (x+, y+) */
    if (blockers[0]!=0) {
       if (GetBlockerInBishopRay1(EnemyPieces, blockers[0], p->koor, &bit)) 
           assign_move(cg->board, p->koor, &pos[n++], bit);
       else (*mobility)--;
       *mobility += p->koor/8 - bit/8;
    }
    else *mobility+=MIN(p->koor/8, 7-(p->koor % 8) );

    /* Captures on ray 2 (x+, y-) */
    if (blockers[1]!=0) {
       if (GetBlockerInBishopRay2(EnemyPieces, blockers[1], p->koor, &bit)) 
           assign_move(cg->board, p->koor, &pos[n++], bit);
       else (*mobility)--;
       *mobility += p->koor/8 - bit/8;
    }
    else *mobility+=MIN(p->koor/8, p->koor % 8);

    /* Captures on ray 3 (x-, y-) */
    if (blockers[2]!=0) {
       if (GetBlockerInBishopRay3(EnemyPieces, blockers[2], p->koor, &bit)) 
           assign_move(cg->board, p->koor, &pos[n++], bit);
       else (*mobility)--;
       *mobility += bit/8 - p->koor/8;
    }
    else *mobility += MIN(7-p->koor/8, p->koor % 8);

    /* Captures on ray 4 (x-, y+) */
    if (blockers[3]!=0) {
       if (GetBlockerInBishopRay4(EnemyPieces, blockers[3], p->koor, &bit)) 
           assign_move(cg->board, p->koor, &pos[n++], bit);
       else (*mobility)--;
       *mobility += bit/8 - p->koor/8;
    }
    else *mobility += MIN(7-p->koor/8, 7 - (p->koor % 8) );
    return n;
} /* GetBishopCaptures */


uint64 get_bishop_mbitmap(ChessGame* cg, int from, uint64 EnemyPieces) 
{
    uint64 m=0, blockers[4];
    int to;

    GetBishopBlockers(cg, from, blockers);

    /* Moves on ray 1 (x+, y+) */
    if (blockers[0]!=0) {
        if (GetBlockerInBishopRay1(EnemyPieces, blockers[0], from, &to)) 
            m |= power[to];
        m |= distance[from][to];
    } else 
        m |= Bishop[from].ray[1];
 
    /* Moves on ray 2 (x+, y-) */
    if (blockers[1]!=0) {
        if (GetBlockerInBishopRay2(EnemyPieces, blockers[1], from, &to)) 
            m |= power[to];
        m |= distance[from][to];
    } else 
        m |= Bishop[from].ray[2];

    /* Moves on ray 3 (x-, y-) */
    if (blockers[2]!=0) {
        if (GetBlockerInBishopRay3(EnemyPieces, blockers[2], from, &to)) 
            m |= power[to];
        m |= distance[from][to];
    } else 
        m |= Bishop[from].ray[3];
 
    /* Moves on ray 4 (x-, y+) */
    if (blockers[3]!=0) {
        if (GetBlockerInBishopRay4(EnemyPieces, blockers[3], from, &to)) 
            m |= power[to];
        m |= distance[from][to];
    } else 
        m |= Bishop[from].ray[4];
    return m;
} /* get_bishop_mbitmap */


void get_castle_move(ChessGame* cg, Piece* p, int color, Move* pos, int dir)
{
    const int castle_rook_idx[2][2]={{7, 0},{31,24}};
    const int ki=king_idx[color]; 
    const int ri=castle_rook_idx[color][dir]; 

    pos->castle = 1;
    pos->transform=pos->en_passant = 0; 
    pos->from = p->koor;
    pos->to = p->koor + dking_castle[dir];
    pos->enpassant_cap=0;
    pos->val=0;
    pos->val -= *(cg->piece[ki].val + pos->from); 
    pos->val += *(cg->piece[ki].val + pos->to);
    pos->val -= *(cg->piece[ri].val + castle_rook_from[color][dir]);
    pos->val += *(cg->piece[ri].val + castle_rook_to[color][dir]);
    pos->kill = 0;
    pos->hash_key = 
    cg->board[p->koor]->hash_key[pos->from] ^
    cg->board[p->koor]->hash_key[pos->to] ^
    cg->board[castle_rook_from[color][dir]]->hash_key[castle_rook_from[color][dir]] ^
    cg->board[castle_rook_from[color][dir]]->hash_key[castle_rook_to[color][dir]];
} /* get_castle_move */


int GetKingMoves(ChessGame* cg, Piece* p, int color, Move* pos)
{
    uint64 cap;
    int n=0;

    if (can_castle(cg, color, RIGHT)) 
        get_castle_move(cg, p, color, &pos[n++], RIGHT);
    if (can_castle(cg, color, LEFT)) 
        get_castle_move(cg, p, color, &pos[n++], LEFT);

    cap = King[p->koor] & (~(cg->bmpieces[color]));
    n+=ExtractFromBitmap(cg->board, p, &pos[n], cap);
    return n;
} /* GetKingMoves */


uint64 get_rook_mbitmap(ChessGame* cg, int from, uint64 EnemyPieces) 
{
    uint64 m=0, blockers[4];
    int to;

    GetRookBlockers(cg, from, blockers);

    /* Captures on ray 1 (x+) */
    if (blockers[0]!=0) {
        if (GetBlockerInRookRay1(EnemyPieces, blockers[0], from, &to)) 
           m |= power[to];
        m |= distance[from][to];
    } else 
        m |= Rook[from].ray[1];

    /* Captures on ray 2 (x-) */
    if (blockers[1]!=0) {
        if (GetBlockerInRookRay2(EnemyPieces, blockers[1], from, &to)) 
           m |= power[to];
        m |= distance[from][to];
    } else 
        m |= Rook[from].ray[2];

    /* Captures on ray 3 (y+) */
    if (blockers[2]!=0) {
        if (GetBlockerInRookRay3(EnemyPieces, blockers[2], from, &to)) 
            m |= power[to];
        m |= distance[from][to];
    } else 
        m |= Rook[from].ray[3];

    /* Captures on ray 4 (y-) */
    if (blockers[3]!=0) {
        if (GetBlockerInRookRay4(EnemyPieces, blockers[3], from, &to)) 
            m |= power[to];
        m |= distance[from][to];
    } else 
        m |= Rook[from].ray[4];
    return m;
} /* get_rook_mbitmap */


int get_rook_mobility(ChessGame* cg, Piece *p, uint64 EnemyPieces)
{
    uint64 blockers[4];
    int bit, mobility=0;
    
    GetRookBlockers(cg, p->koor, blockers);

    /* Captures on ray 1 (x+) */
    if (blockers[0]!=0) {
       if (!GetBlockerInRookRay1(EnemyPieces, blockers[0], p->koor, &bit)) 
            mobility--;
       mobility += p->koor/8 - bit/8;
    }
    else mobility += p->koor/8;

    /* Captures on ray 2 (x-) */
    if (blockers[1]!=0) {
       if (!GetBlockerInRookRay2(EnemyPieces, blockers[1], p->koor, &bit))
            mobility--;
       mobility += bit/8 - p->koor/8;
    }
    else mobility += 7-p->koor/8;

    /* Captures on ray 3 (y+) */
    if (blockers[2]!=0) {
       if (!GetBlockerInRookRay3(EnemyPieces, blockers[2], p->koor, &bit)) {
           if (cg->board[bit]->type==rook)      /* double rook */
               mobility+=8;
           else
               mobility--;
       }
       mobility += (bit % 8) - (p->koor % 8); 
    }
    else mobility += 7-(p->koor % 8);

    /* Captures on ray 4 (y-) */
    if (blockers[3]!=0) {
       if (!GetBlockerInRookRay4(EnemyPieces, blockers[3], p->koor, &bit)) {
           if (cg->board[bit]->type==rook)      /* double rook */
               mobility+=8;
           else
               mobility--;
       }
       mobility += (p->koor % 8) - (bit % 8);
    }
    else mobility += (p->koor % 8);

    return mobility;
} /* get_rook_mobility */


int get_bishop_mobility(ChessGame* cg, Piece* p, uint64 EnemyPieces)
{
    int bit, mobility=0;
    uint64 blockers[4];
    
    GetBishopBlockers(cg, p->koor, blockers);

    /* Captures on ray 1 (x+, y+) */
    if (blockers[0]!=0) {
       if (!GetBlockerInBishopRay1(EnemyPieces, blockers[0], p->koor, &bit)) 
           mobility--;
       mobility += p->koor/8 - bit/8;
    }
    else mobility+=MIN(p->koor/8, 7-(p->koor % 8) );

    /* Captures on ray 2 (x+, y-) */
    if (blockers[1]!=0) {
       if (!GetBlockerInBishopRay2(EnemyPieces, blockers[1], p->koor, &bit)) 
            mobility--;
       mobility += p->koor/8 - bit/8;
    }
    else mobility+=MIN(p->koor/8, p->koor % 8);

    /* Captures on ray 3 (x-, y-) */
    if (blockers[2]!=0) {
       if (!GetBlockerInBishopRay3(EnemyPieces, blockers[2], p->koor, &bit)) 
            mobility--;
       mobility += bit/8 - p->koor/8;
    }
    else mobility += MIN(7-p->koor/8, p->koor % 8);

    /* Captures on ray 4 (x-, y+) */
    if (blockers[3]!=0) {
       if (!GetBlockerInBishopRay4(EnemyPieces, blockers[3], p->koor, &bit)) 
            mobility--;
       mobility += bit/8 - p->koor/8;
    }
    else mobility += MIN(7-p->koor/8, 7 - (p->koor % 8) );
    return mobility;
} /* get_bishop_mobility */


int compute_mobility(ChessGame* cg, int color) 
{
    Piece *tl, *tr, *bl, *br, *q;
    uint64 EnemyPieces;
    int mobility=0;
 
    if (color==WHITE) {
        tl = &cg->piece[0];
        tr = &cg->piece[7];
        bl = &cg->piece[2];
        br = &cg->piece[5];
        q = &cg->piece[3];
    } else {
        tl = &cg->piece[24];
        tr = &cg->piece[31];
        bl = &cg->piece[26];
        br = &cg->piece[29];
        q = &cg->piece[27];
    }
    EnemyPieces = cg->bmpieces[!color];

    if (tl->alive) 
        mobility+=get_rook_mobility(cg, tl, EnemyPieces);
    if (tr->alive)
        mobility+=get_rook_mobility(cg, tr, EnemyPieces);
    if (bl->alive) 
        mobility+=get_bishop_mobility(cg, bl, EnemyPieces)/2;
    if (br->alive) 
        mobility+=get_bishop_mobility(cg, br, EnemyPieces)/2;
    if (q->alive) 
        mobility+=(get_rook_mobility(cg, q, EnemyPieces) +
                   get_bishop_mobility(cg, q, EnemyPieces))/2;
    return mobility;
} /* compute_mobility */


void make_move_list(Move* last_move, Move pos[], int n)
{
    uint64 key;
    Move* s;

    if (n>0) {
        key = last_move->hash_key ^ wb_key;

        /* If set, remove last moves en passant key */
        if (last_move->enpassant_cap) 
            key ^= en_passant_key[last_move->enpassant_cap->koor];

        for (s=&pos[0]; s<&pos[n-1]; s++) {
            s->sibling = s+1; 
            s->hash_key ^= key;
        }
        pos[n-1].sibling=0;
        pos[n-1].hash_key ^= key;
    }
} // make_move_list

int get_pos(ChessGame* cg, Move* last_move, int color, Move pos[])
{
    Piece *p;
    Piece *first=&cg->piece[first_piece_idx[color]];
    Piece *last=&cg->piece[last_piece_idx[color]];
    uint64 EnemyPieces, m;
    unsigned int n=0; /* number of moves */

    EnemyPieces = cg->bmpieces[!color];
 
    for (p=first; p<=last; p++) 
        if (p->alive) 
            switch(p->type) {
                case pawn:
                   GetPawnCPos(cg->board, p, last_move, color, LEFT, &n, pos);
                   GetPawnCPos(cg->board, p, last_move, color, RIGHT, &n, pos);
                   GetPawnNonCaptures(cg->board, p, &n, pos);
                   break;
                case rook:
                   n+=ExtractFromBitmap(cg->board, p, &pos[n],
                       get_rook_mbitmap(cg, p->koor, EnemyPieces));
                   break;
                case knight:
                   n+=ExtractFromBitmap(cg->board, p, &pos[n], 
                       Knight[p->koor] & (~(cg->bmpieces[color])) );
                   break;
                case bishop:
                   n+=ExtractFromBitmap(cg->board, p, &pos[n],
                       get_bishop_mbitmap(cg, p->koor, EnemyPieces));
                   break;
                case queen:
                   m  = get_rook_mbitmap(cg, p->koor, EnemyPieces);
                   m |= get_bishop_mbitmap(cg, p->koor, EnemyPieces);
                   n+=ExtractFromBitmap(cg->board, p, &pos[n], m);
                   break;
                case king:
                   n+=GetKingMoves(cg, p, color, &pos[n]);
                   break;
                default: 
                   printf("Fejl i GetMPos: %c\n", p->type);
            } /* switch */
  
    if (color==WHITE)
        qsort(&pos[0], n, sizeof(Move), CompareWhite);
    else
        qsort(&pos[0], n, sizeof(Move), CompareBlack);
    make_move_list(last_move, pos, n);
    return n;
} /* get_pos */


int get_captures(ChessGame* cg, Move* last_move, 
                 int color, unsigned int *mobility,
                 Move pos[])
{
    Piece *p;
    Piece *first=&cg->piece[first_piece_idx[color]];
    Piece *last=&cg->piece[last_piece_idx[color]];
    uint64 EnemyPieces = cg->bmpieces[!color];
    unsigned int mobl, n=0; /* number of moves */
    *mobility=0;
 
    for (p=first; p<=last; p++) 
        if (p->alive) 
            switch(p->type) {
                case pawn:
                   GetPawnCPos(cg->board, p, last_move, color, LEFT, &n, pos);
                   GetPawnCPos(cg->board, p, last_move, color, RIGHT, &n, pos);
                   break;
                case rook:
                   n+=GetRookCaptures(cg, p, mobility, &pos[n], EnemyPieces);
                   break;
                case knight:
                   n+=ExtractFromBitmap(cg->board, p, &pos[n], 
                                     Knight[p->koor] & EnemyPieces);
                   break;
                case bishop:
                   mobl=0;
                   n+=GetBishopCaptures(cg, p, &mobl, &pos[n], EnemyPieces);
                   *mobility+=mobl/2;
                   break;
                case queen:
                   mobl=0;
                   n+=GetRookCaptures(cg, p, &mobl, &pos[n], EnemyPieces);
                   n+=GetBishopCaptures(cg, p, &mobl, &pos[n], EnemyPieces);
                   *mobility+=mobl/2;
                   break;
                case king:
                   n+=ExtractFromBitmap(cg->board, p, &pos[n], 
                                     King[p->koor] & EnemyPieces);
                   break;
                default: 
                   printf("Fejl i GetCPos: %c\n", p->type);
            } /* switch */

    make_move_list(last_move, pos, n);
    return n;
} /* get_captures */

/* Generate replys to move m */
int get_rpos(ChessGame* cg, Move* last_move, int color, Move pos[])
{
    int n=0;   /* number of moves */
    Piece *p;
    Piece *first=&cg->piece[first_piece_idx[color]];
    Piece *last=&cg->piece[last_piece_idx[color]];
    uint64 PieceBitmap = cg->bmpieces[BLACK] | cg->bmpieces[WHITE];
 
    for (p=first; p<=last; p++) 
        if (p->alive && p->can_capture(p, last_move->to, PieceBitmap)) {
            assign_move(cg->board, p->koor, &pos[n], last_move->to);
            pos[n].transform=is_pawn_trans(cg->board, &pos[n], color);
            n++;
        } 
    make_move_list(last_move, pos, n);
    return n;
} /* get_rpos */
