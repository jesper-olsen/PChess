/*
 *  Copyright (c) 2004 Jesper Olsen
 *  License: MIT, see License.txt
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "skak.h"
#include "eval.h"
#include "mgenerator.h"

enum {
    COL0 = 255
};

const int pawn_val[2][64] = {
    { 100, 100, 101, 102, 104, 106, 108, 900,
      100, 100, 102, 104, 106, 109, 112, 900,
      100, 100, 104, 108, 112, 115, 118, 900,
      100, 100, 107, 114, 121, 128, 135, 900,
      100, 100, 106, 112, 118, 124, 132, 900,
      100, 100, 104, 108, 112, 116, 120, 900,
      100, 100, 102, 104, 106, 108, 112, 900,
      100, 100, 101, 102, 104, 106, 108, 900},
    { -900, -108, -106, -104, -102, -101, -100, -100,
      -900, -112, -109, -106, -103, -102, -100, -100,
      -900, -118, -115, -112, -109, -104, -100, -100,
      -900, -135, -128, -121, -114, -107, -100, -100,
      -900, -132, -124, -118, -112, -106, -100, -100,
      -900, -120, -116, -112, -108, -104, -100, -100,
      -900, -112, -108, -106, -104, -102, -100, -100,
      -900, -108, -106, -104, -102, -101, -100, -100}
    };

const int rook_val[2][64] = {
     {500, 500, 500, 500, 500, 500, 522, 500,
      500, 500, 500, 500, 500, 500, 522, 500,
      500, 500, 500, 500, 500, 500, 522, 500,
      500, 500, 500, 500, 500, 500, 522, 500,
      500, 500, 500, 500, 500, 500, 522, 500,
      500, 500, 500, 500, 500, 500, 522, 500,
      500, 500, 500, 500, 500, 500, 522, 500,
      500, 500, 500, 500, 500, 500, 522, 500},
     {-500, -522, -500, -500, -500, -500, -500, -500,
      -500, -522, -500, -500, -500, -500, -500, -500,
      -500, -522, -500, -500, -500, -500, -500, -500,
      -500, -522, -500, -500, -500, -500, -500, -500,
      -500, -522, -500, -500, -500, -500, -500, -500,
      -500, -522, -500, -500, -500, -500, -500, -500,
      -500, -522, -500, -500, -500, -500, -500, -500,
      -500, -522, -500, -500, -500, -500, -500, -500}
     };

const int knight_val[2][64] = {
      {315, 315, 315, 315, 315, 315, 315, 315,
       315, 320, 320, 320, 320, 320, 320, 315,
       315, 320, 325, 325, 330, 330, 320, 315,
       315, 320, 325, 325, 330, 330, 320, 315,
       315, 320, 325, 325, 330, 330, 320, 315,
       315, 320, 325, 325, 330, 330, 320, 315,
       315, 320, 320, 320, 320, 320, 320, 315,
       315, 315, 315, 315, 315, 315, 315, 315},
      {-315, -315, -315, -315, -315, -315, -315, -315,
       -315, -320, -320, -320, -320, -320, -320, -315,
       -315, -320, -330, -330, -325, -325, -320, -315,
       -315, -320, -330, -330, -325, -325, -320, -315,
       -315, -320, -330, -330, -325, -325, -320, -315,
       -315, -320, -330, -330, -325, -325, -320, -315,
       -315, -320, -320, -320, -320, -320, -320, -315,
       -315, -315, -315, -315, -315, -315, -315, -315}
    };

const int bishop_val[2][64] = {
      {339, 350, 350, 350, 350, 350, 350, 350,
       339, 350, 350, 350, 350, 350, 350, 350,
       339, 350, 350, 350, 350, 350, 350, 350,
       339, 350, 350, 350, 350, 350, 350, 350,
       339, 350, 350, 350, 350, 350, 350, 350,
       339, 350, 350, 350, 350, 350, 350, 350,
       339, 350, 350, 350, 350, 350, 350, 350,
       339, 350, 350, 350, 350, 350, 350, 350},
      {-350, -350, -350, -350, -350, -350, -350, -339,
       -350, -350, -350, -350, -350, -350, -350, -339,
       -350, -350, -350, -350, -350, -350, -350, -339,
       -350, -350, -350, -350, -350, -350, -350, -339,
       -350, -350, -350, -350, -350, -350, -350, -339,
       -350, -350, -350, -350, -350, -350, -350, -339,
       -350, -350, -350, -350, -350, -350, -350, -339,
       -350, -350, -350, -350, -350, -350, -350, -339}
      };
 
const int queen_val[2][64] = {
      {900, 900, 900, 900, 900, 900, 900, 900, 
       900, 900, 900, 900, 900, 900, 900, 900, 
       900, 900, 900, 900, 900, 900, 900, 900, 
       900, 900, 900, 900, 900, 900, 900, 900, 
       900, 900, 900, 900, 900, 900, 900, 900, 
       900, 900, 900, 900, 900, 900, 900, 900, 
       900, 900, 900, 900, 900, 900, 900, 900, 
       900, 900, 900, 900, 900, 900, 900, 900}, 
      {-900, -900, -900, -900, -900, -900, -900, -900, 
       -900, -900, -900, -900, -900, -900, -900, -900, 
       -900, -900, -900, -900, -900, -900, -900, -900, 
       -900, -900, -900, -900, -900, -900, -900, -900, 
       -900, -900, -900, -900, -900, -900, -900, -900, 
       -900, -900, -900, -900, -900, -900, -900, -900, 
       -900, -900, -900, -900, -900, -900, -900, -900, 
       -900, -900, -900, -900, -900, -900, -900, -900} 
      };

const int king_val[2][64] = {
      { 24,  24,  12,  6,  6,  12,  24,  24, 
        24,  12,  6,   0,  0,  6,   12,  24, 
        12,  6,   0,  -6, -6,  0,   6,  12, 
        6,   0,  -6, -12, -12, -6,  0,  6, 
        6,   0,  -6, -12, -12, -6,  0,  6, 
        12,  6,   0,  -6, -6,  0,   6,  12, 
        24,  12,  6,   0,  0,  6,   12,  24, 
        24,  24,  12,  6,  6,  12,  24,  24}, 

      {-24, -18, -12, -6, -6, -12, -24, -24, 
       -24, -12, -6,  -0, -0, -6,  -12, -24, 
       -12, -6,  -0,   6,  6, -0,  -6,  -12, 
       -6,  -0,   6,  12,  12,  6, -0,  -6, 
       -6,  -0,   6,  12,  12,  6, -0,  -6, 
       -12, -6,  -0,   6,  6, -0,  -6,  -12, 
       -24, -12, -6,  -0, -0, -6,  -12, -24, 
       -24, -24, -12, -6, -6, -12, -24, -24}, 
      };

/* Bitmaps */
uint64 distance[64][64], Knight[64], King[64];
FourRay Rook[64], Bishop[64];
uint64 pawn_key[2][64];
uint64 rook_key[2][64];
uint64 knight_key[2][64];
uint64 bishop_key[2][64];
uint64 queen_key[2][64];
uint64 king_key[2][64];
uint64 en_passant_key[64];
uint64 wb_key;

uint64 power[64];  /* tabel over 2^n fra n=0,...,63 */

int eval_material(ChessGame* cg)
{
    int i, sum;

    sum = 0;
    for (i=0; i<32; i++)
        if (cg->piece[i].alive)
            sum += cg->piece[i].val[cg->piece[i].koor];
    return sum;
} /* eval */

int abs_eval_material(ChessGame* cg)
{
    int i, sum;

    sum = 0;
    for (i=0; i<32; i++)
        if (cg->piece[i].alive)
            sum += abs( cg->piece[i].val[cg->piece[i].koor] );
    return sum;
} /* abs_eval_material */


int eval_pawn_structure(ChessGame* cg)
{
    int ps=0, i, x, y;
    unsigned char wf[10], bf[10];

    wf[0] = 0;
    wf[1] = COL0 & cg->bmpawns[WHITE];
    wf[2] = COL0 & (cg->bmpawns[WHITE]>>8);
    wf[3] = COL0 & (cg->bmpawns[WHITE]>>16);
    wf[4] = COL0 & (cg->bmpawns[WHITE]>>24);
    wf[5] = COL0 & (cg->bmpawns[WHITE]>>32);
    wf[6] = COL0 & (cg->bmpawns[WHITE]>>40);
    wf[7] = COL0 & (cg->bmpawns[WHITE]>>48);
    wf[8] = COL0 & (cg->bmpawns[WHITE]>>56);
    wf[9] = 0;
    bf[0] = 0;
    bf[1] = COL0 & cg->bmpawns[BLACK];
    bf[2] = COL0 & (cg->bmpawns[BLACK]>>8);
    bf[3] = COL0 & (cg->bmpawns[BLACK]>>16);
    bf[4] = COL0 & (cg->bmpawns[BLACK]>>24);
    bf[5] = COL0 & (cg->bmpawns[BLACK]>>32);
    bf[6] = COL0 & (cg->bmpawns[BLACK]>>40);
    bf[7] = COL0 & (cg->bmpawns[BLACK]>>48);
    bf[8] = COL0 & (cg->bmpawns[BLACK]>>56);
    bf[9] = 0;
    for (i=8; i<16; i++) {
        if (cg->piece[i].alive && cg->piece[i].type==pawn) {
           x=cg->piece[i].koor/8 + 1;
           y=cg->piece[i].koor % 8;
           if (wf[x] > power[y])                     /* double pawn */
               ps-=4;
           if (!wf[x-1] && !wf[x+1])                 /* isolated pawn */
               ps-=20;
           if (!bf[x] && bf[x-1]<=power[y] && bf[x+1]<=power[y]) /* passed pawn */
               ps+=2*y*y;
        }
        if (cg->piece[i+8].alive && cg->piece[i+8].type==pawn) {
           x=cg->piece[i+8].koor/8 + 1;
           y=cg->piece[i+8].koor % 8;
           if (bf[x] > power[y])                     /* double pawn */
               ps+=4;
           if (!bf[x-1] && !bf[x+1])                 /* isolated pawn */
               ps+=20;
           if (!wf[x] && (!wf[x-1] || wf[x-1]>=power[y]) &&      
                         (!wf[x+1] || wf[x+1]>=power[y]) )      /* passed pawn */
               ps-=2*(7-y)*(7-y);
        }
    }
    return ps;
} /* eval_pawn_structure */


void adjust_king_val(ChessGame* cg, int is_end)
{
    if (is_end!=0) { // prefer king center
       cg->piece[4].val = &king_val[BLACK][0];
       cg->piece[28].val = &king_val[WHITE][0];
    } else {        // prefer king border
       cg->piece[4].val = &king_val[WHITE][0];
       cg->piece[28].val = &king_val[BLACK][0];
    }
} /* adjust_king_val */


void pawn_transform(Piece* piece)
{
    piece->type = queen;
    piece->val = &queen_val[piece->color][0];
    piece->hash_key = &queen_key[piece->color][0];
    piece->can_capture=&queen_can_capture;
} /* pawn_transform */


void pawn_transform_back(Piece* piece)
{
     piece->type = pawn;
     piece->val = &pawn_val[piece->color][0];
     piece->hash_key = &pawn_key[piece->color][0];
     piece->can_capture=&pawn_can_capture;
} /* pawn_transform_back */


void set_bit2(uint64* bitmap, int x, int y)
{
    int bit;

    if ((x>=0) && (y>=0) && (x<8) && (y<8)) {
        bit = (7-x)*8 + y; 
        *bitmap |= power[bit];
    }
} /* set_bit2 */


void get_bitmaps(ChessGame* cg, uint64* bitmaps)
{
    bitmaps[0]=cg->bmpieces[WHITE];
    bitmaps[1]=cg->bmpieces[BLACK];
    bitmaps[2]=cg->bmpawns[WHITE];
    bitmaps[3]=cg->bmpawns[BLACK];
} /* get_bitmaps */


void print_bitmap(uint64 b)
{
    int x, y, bit;

    printf("\n");
    for (y=7; y>=0; y--) {
        printf("%d ", y);
        for (x=0; x<8; x++) { 
            bit=(7-x)*8+y;
            if ((b & power[bit]) != 0)
     //     if (TEST_BIT(b, bit))
               printf("1");
            else
               printf("0");
        }
        printf("\n");
   }
} /* print_bitmap */


// get board position in expanded fen notation - len(buffer)=65
void get_fene(ChessGame* cg, char *buffer)
{
    int x, y, bit, z=0;

    for (y=7; y>=0; y--) {
        for (x=0; x<8; x++) { 
            bit=(7-x)*8+y;
            if (cg->board[bit]==NULL)
               buffer[z++]='.';
            else {
               if (cg->board[bit]->color==WHITE)
                  buffer[z++]=cg->board[bit]->type;
               else
                  buffer[z++]=cg->board[bit]->type - 'A' + 'a';
            }
        }
   }
   buffer[64]='\0';
} /* get_fene */


// get castling rights len(s)=5
void get_castling_rights(ChessGame* cg, char *s)
{
    int z=0;
    if (cg->CastlingOn[WHITE][RIGHT]=1)
        s[z++]='K';
    if (cg->CastlingOn[WHITE][LEFT]==1)
        s[z++]='Q';
    if (cg->CastlingOn[BLACK][RIGHT]=1)
        s[z++]='k';
    if (cg->CastlingOn[BLACK][LEFT]==1)
        s[z++]='q';
    if (z==0) 
        s[z++]='-';
    s[z]='\0';
}


void PrintBoard(ChessGame* cg)
{
    int x, y, bit;

    printf("\n");
    for (y=7; y>=0; y--) {
        printf("%d ", y+1);
        for (x=0; x<8; x++) { 
            bit=(7-x)*8+y;
            if (cg->board[bit]==NULL)
               printf(".");
            else {
               if (cg->board[bit]->color==WHITE)
                  printf("%c", cg->board[bit]->type);
               else
                  printf("%c", cg->board[bit]->type - 'A' + 'a');
            }
        }
        printf("\n");
   }
   printf("  ABCDEFGH\n");
} /* PrintBoard*/


void PrintMove(Move* c)
{
    if (c->kill)
        printf("Move: %c%dx%c%d\n",
                 7-c->from/8+'A', (c->from % 8)+1,
                 7-c->to/8+'A', (c->to % 8)+1);
    else
        printf("Move: %c%d %c%d\n",
                 7-c->from/8+'A', (c->from % 8)+1,
                 7-c->to/8+'A', (c->to % 8)+1);
    fflush(stdout);
} /* PrintMove */


/* Init of bitmaps for BLACK/WHITE pieces and pawns */
void init_dynamic_bitmaps(ChessGame* cg)
{
    int i;

    cg->bmpieces[WHITE]=cg->bmpieces[BLACK]=0;
    cg->bmpawns[WHITE]=cg->bmpawns[BLACK]=0;
    for (i=0; i<32; i++)
        if (cg->piece[i].alive) 
            SET_BIT(cg->bmpieces[cg->piece[i].color], cg->piece[i].koor);

    for (i=8; i<24; i++)
        if (cg->piece[i].alive)
            SET_BIT(cg->bmpawns[cg->piece[i].color], cg->piece[i].koor);
} /* init_dynamic_bitmaps */


void init_hash_key(uint64 *bm)
{
    *bm = rand();
    *bm = (*bm<<32) | rand();
} /* init_hash_key */


void calc_hash_keys(void)
{
    time_t seed;
    int bit, color;

    time(&seed);
    //seed=1;
    srand((int) seed);

    for (bit=0; bit<64; bit++) {
        for (color=WHITE; color<=BLACK; color++) {
            init_hash_key(&pawn_key[color][bit]);
            init_hash_key(&rook_key[color][bit]);
            init_hash_key(&knight_key[color][bit]);
            init_hash_key(&bishop_key[color][bit]);
            init_hash_key(&queen_key[color][bit]);
            init_hash_key(&king_key[color][bit]);
        }
        init_hash_key(&en_passant_key[bit]);
    }
    init_hash_key(&wb_key);  
} /* calc_hash_keys */


void init_static_bitmaps(ChessGame* cg)
{
    int x, y, i, bit, s; 
    int x1, x2, y1, y2, i1, i2, dx, dy;
    uint64 z;

    if (sizeof(unsigned long int)<4) {
          printf("Sorry, need long int's of at least 32 bit\n");
          exit(1);
    }

    z=1;
    for (i=0; i<64; i++) {
       power[i] = z;
       z*=2;
    }
    
    for (s=0; s<64; s++) {
        for (i=0; i<=4; i++) { 
            Rook[s].ray[i]=0;
            Bishop[s].ray[i]=0;
        }
        King[s] = 0;
        Knight[s] = 0;
    }

    /* Init of bitmaps for a rook */
    for (x=0; x<8; x++)
       for (y=0; y<8; y++) {
          s = (7-x)*8 + y;
          for (i=x+1; i<8; i++) 
             set_bit2(&Rook[s].ray[1], i, y);
          for (i=0; i<x; i++) 
             set_bit2(&Rook[s].ray[2], i, y);
          for (i=y+1; i<8; i++) 
             set_bit2(&Rook[s].ray[3], x, i);
          for (i=0; i<y; i++) 
             set_bit2(&Rook[s].ray[4], x, i);
          for (i=1; i<=4; i++) {
              Rook[s].ray[0] |= Rook[s].ray[i];
          }
       } /* for */

    /* Init of bitmaps for a bishop */
    for (x=0; x<8; x++)
       for (y=0; y<8; y++) {
           s = (7-x)*8 + y;
           for (i=1; ((x+i<8) && (y+i<8)); i++) { 
               bit = (7-(x+i))*8 + y+i;   /* ray 1 (x+, y+) */
               SET_BIT(Bishop[s].ray[1], bit);
           } /* for */
           for (i=1; ((x+i<8) && (y-i>=0)); i++) { 
               bit = (7-(x+i))*8 + y-i;   /* ray 2 (x+, y-) */
               SET_BIT(Bishop[s].ray[2], bit);
           } /* for */
           for (i=1; ((x-i>=0) && (y-i>=0)); i++) { 
               bit = (7-(x-i))*8 + y-i;   /* ray 3 (x-, y-) */
               SET_BIT(Bishop[s].ray[3], bit);
           } /* for */
           for (i=1; ((x-i>=0) && (y+i<8)); i++) { 
               bit = (7-(x-i))*8 + y+i;   /* ray 4 (x-, y+) */
               SET_BIT(Bishop[s].ray[4], bit);
           } /* for */
           for (i=1; i<=4; i++) 
               Bishop[s].ray[0] |= Bishop[s].ray[i];
       } /* for */


    /* Init of bitmaps for a knight */
    for (x1=0; x1<8; x1++)
        for (y1=0; y1<8; y1++) {
            s=(7-x1)*8+y1; 
            for (x2=-2; x2<=2; x2++)
                for (y2=-2; y2<=2; y2++)
                    if (abs(x2)!=abs(y2) && x2!=0 && y2!=0)
                        set_bit2(&Knight[s], x1+x2, y1+y2);
        }
        
    /* Init of bitmaps for a king */
    for (x1=0; x1<8; x1++)
        for (y1=0; y1<8; y1++) {
            s = (7-x1)*8+y1;
            for (x2=-1; x2<=1; x2++)
                for (y2=-1; y2<=1; y2++)
                    if (x2!=0 || y2!=0)
                        set_bit2(&King[s], x1+x2, y1+y2);
        }
 
    /* Init of distance table */
    for (x1=0; x1<8; x1++)
       for (y1=0; y1<8; y1++)
          for (x2=0; x2<8; x2++)
             for (y2=0; y2<8; y2++) {
                i1 = (7-x1)*8+y1;
                i2 = (7-x2)*8+y2;
                distance[i1][i2] = 0;
                if (x1==x2) dx=0;
                else
                if (x1<x2) dx=1;
                else dx=-1; 
                if (y1==y2) dy=0;
                else
                if (y1<y2) dy=1;
                else dy=-1;
                if ( (x1!=x2 || y1!=y2) &&
                     ((x1==x2 || y1==y2) || (abs(x1-x2) == abs(y1-y2)))
                   ) {
                    x=x1+dx;
                    y=y1+dy;
                    while ((x!=x2) || (y!=y2)) {
                       bit=(7-x)*8+y;
                       distance[i1][i2] |= power[bit];
                       x+=dx; 
                       y+=dy;
                    } /* while */
                } /* if */
             } /* for */
} /* init_bitmaps */
          

void calc_hash_key(ChessGame* cg, uint64* hash_key)
{
    int bit;

    if (cg->color==WHITE) 
        *hash_key=wb_key;
    else 
        *hash_key=0;

    for (bit=0; bit<64; bit++)
        if (cg->board[bit]) 
            *hash_key ^= cg->board[bit]->hash_key[bit];
} // calc_hash_key 


void init_piece(Piece* p, char type, unsigned char color)
{
    p->type=type;
    p->color=color;
    p->koor=0;
    p->alive=0;

    switch(type) {
    case pawn:
        p->hash_key=&pawn_key[color][0];
        p->val=&pawn_val[color][0];
	p->can_capture=&pawn_can_capture;
        break;
    case rook:
        p->hash_key=&rook_key[color][0];
        p->val=&rook_val[color][0];
	p->can_capture=&rook_can_capture;
        break;
    case knight:
        p->hash_key=&knight_key[color][0];
        p->val=&knight_val[color][0];
	p->can_capture=&knight_can_capture;
        break;
    case bishop:
        p->hash_key=&bishop_key[color][0];
        p->val=&bishop_val[color][0];
	p->can_capture=&bishop_can_capture;
        break;
    case queen:
        p->hash_key=&queen_key[color][0];
        p->val=&queen_val[color][0];
	p->can_capture=&queen_can_capture;
        break;
    case king:
        p->hash_key=&king_key[color][0];
        p->val=&king_val[color][0];
	p->can_capture=&king_can_capture;
        break;
    }
} /* init_piece */


Piece* create_piece(ChessGame* cg, char type, int offset)
{
    int i;
    for (i=offset; i<offset+16; i++)
        if (cg->piece[i].type==type && cg->piece[i].alive==0) 
            return &cg->piece[i];
    if (type=='Q') 
        for (i=offset; i<offset+16; i++) 
            if (cg->piece[i].type=='P' && cg->piece[i].alive==0) { 
                pawn_transform(&cg->piece[i]);
                return &cg->piece[i];
            }
    return NULL;
} /* create_piece */


void init_castling_rights(ChessGame* cg, char* castling, int nc)
{
    int n;
    cg->CastlingOn[WHITE][LEFT]=cg->CastlingOn[WHITE][RIGHT]=0;
    cg->CastlingOn[BLACK][LEFT]=cg->CastlingOn[BLACK][RIGHT]=0;
    for (n=0; n<nc; n++) {
        switch (castling[n]) {
        case 'K':
            cg->CastlingOn[WHITE][RIGHT]=1;
            break;
        case 'k':
            cg->CastlingOn[BLACK][RIGHT]=1;
            break;
        case 'Q':
            cg->CastlingOn[WHITE][LEFT]=1;
            break;
        case 'q':
            cg->CastlingOn[BLACK][LEFT]=1;
            break;
        default:   // catch '-'
            break;
        }
    }
} /* init_castling_rights */


int setup(ChessGame* cg, char* fene, char* castling, int nc)
{
    int i, offset;
    Piece* p;

    init_castling_rights(cg, castling, nc);
    for (i=0; i<32; i++)
        cg->piece[i].alive=0;
    for (i=0; i<64; i++)
        cg->board[i]=NULL;

    for (i=0; i<64; i++) {
        if (fene[i]=='.')
            continue;
        if (isupper(fene[i]))
            offset=0;
        else
            offset=16;
        p=create_piece(cg, toupper(fene[i]), offset);
        if (p==NULL) {
            printf("Failed to create piece %c %d %d\n", fene[i], i, offset);
	    return 0;
	}
        p->alive=1;
        p->koor=63-8*(i%8)-i/8;
        cg->board[p->koor]=p;
    }
    init_dynamic_bitmaps(cg);
    move_eq(&cg->last, NULL);
    calc_hash_key(cg, &cg->last.hash_key);
    return 1;
} /* setup */


void get_chess_board(ChessGame* cg)
{
    int i;
    char* startpos="rnbqkbnrpppppppp................................PPPPPPPPRNBQKBNR";

    calc_hash_keys();

    init_piece(&cg->piece[0], rook, WHITE);
    init_piece(&cg->piece[1], knight, WHITE);
    init_piece(&cg->piece[2], bishop, WHITE);
    init_piece(&cg->piece[3], queen, WHITE);
    init_piece(&cg->piece[4], king, WHITE);
    init_piece(&cg->piece[5], bishop, WHITE);
    init_piece(&cg->piece[6], knight, WHITE);
    init_piece(&cg->piece[7], rook, WHITE);

    for (i=8; i<16; i++) 
        init_piece(&cg->piece[i], pawn, WHITE);

    for (i=16; i<24; i++) 
        init_piece(&cg->piece[i], pawn, BLACK);

    init_piece(&cg->piece[24], rook, BLACK);
    init_piece(&cg->piece[25], knight, BLACK);
    init_piece(&cg->piece[26], bishop, BLACK);
    init_piece(&cg->piece[27], queen, BLACK);
    init_piece(&cg->piece[28], king, BLACK);
    init_piece(&cg->piece[29], bishop, BLACK);
    init_piece(&cg->piece[30], knight, BLACK);
    init_piece(&cg->piece[31], rook, BLACK);

    init_static_bitmaps(cg);
    setup(cg, startpos, "KQkq", 4);
} /* get_chess_board */
