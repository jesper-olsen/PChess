/*
 *  Copyright (c) 2004 Jesper Olsen
 *  License: MIT, see License.txt
 *
 */


#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "skak.h"
#include "eval.h"
#include "mgenerator.h"
#include "search.h"

enum {
    INFINITE=10000,
    MAX_DEPTH=50,          // Killer table
    EXACT=0,               // Transposition table
    UPPER_BOUND=-1,
    LOWER_BOUND=1
};


void init_game(ChessGame* cg)
{
    int i;
   
    cg->depth   = 10;
    cg->max_search = 50000;
    cg->max_time = -1;
    get_chess_board(cg);
    cg->max_abs_val = abs_eval_material(cg);
    cg->color = WHITE;
    for (i=0; i<REP_SIZE; i++) 
        cg->RepTable[i].nr=0; 
    for (i=0; i<MAX_DEPTH; i++)            // slet killer tabellen 
       cg->kill_tab[i].n1 = cg->kill_tab[i].n2 = 0;
    for (i=0; i<HASH_SIZE; i++)             // slet hash-tabellen 
       cg->trans_table[i].height=0;
} // init_game


ChessGame* new_chess_game(void)
{
    ChessGame* cg;
    cg=(ChessGame*) malloc(sizeof(ChessGame));
    cg->kill_tab=(KillerTable*) malloc(MAX_DEPTH*sizeof(KillerTable));
    cg->trans_table=(TranspositionHash*) malloc(HASH_SIZE*sizeof(TranspositionHash));
    init_game(cg);
    return cg;
} // new_chess_game


void free_chess_game(ChessGame* cg)
{
    free(cg->kill_tab);
    free(cg->trans_table);
    free(cg);
} // free_chess_game


int eval(ChessGame* cg, unsigned char color, int material)
{
    int best;

    //compute_mobility(cg, color, &mobility[color]);
    //compute_mobility(cg, !color, &mobility[!color]);
    best=material + eval_pawn_structure(cg) + cg->mobility[WHITE] - cg->mobility[BLACK];
    return (color==WHITE)?best:-best;
} // eval


void print_rep_table(ChessGame* cg)
{
    unsigned long int adr;
    for (adr = 0; adr<REP_SIZE; adr++) {
        if (cg->RepTable[adr].nr==0) 
            continue;
        printf("%lu %llu: %d\n", adr, cg->RepTable[adr].hash_key,
                                      cg->RepTable[adr].nr);
    }
} /* print_rep_table */

    
int count_rep(ChessGame* cg, uint64 hash_key, unsigned long int* adr)
{
    for (*adr = hash_key & REP_MASK; *adr<REP_SIZE; (*adr)++) {
        if (cg->RepTable[*adr].nr==0) {
             cg->RepTable[*adr].hash_key=hash_key;
             return 0; 
        }

        if (cg->RepTable[*adr].hash_key==hash_key)
           return cg->RepTable[*adr].nr;
    }
    for (*adr = (hash_key & REP_MASK)-1; *adr>=0; (*adr)--) {
        if (cg->RepTable[*adr].nr==0) {
             cg->RepTable[*adr].hash_key=hash_key;
             return 0; 
        }

        if (cg->RepTable[*adr].hash_key==hash_key)
           return cg->RepTable[*adr].nr;
    }
    exit(1);
    return(1);  /* happy lint! */
} /* count_rep */
    

char* post_mortem(ChessGame* cg)
{
   int i, fifty;
   unsigned long int adr; 

   //print_rep_table(cg);
   fifty=0;
   for (i=0; i<REP_SIZE; i++)
        if (cg->RepTable[i].nr>0)
            fifty++;

   if (fifty>=100) 
        return "Draw by the 50-move rule.";

   if (count_rep(cg, cg->last.hash_key, &adr)>=3) 
       return "Draw by repetition.";

   if (in_check(cg, cg->color)) {
       if (cg->color == WHITE) 
           return "White is check mate.      ";
       return "Black is check mate.      ";
   } 

   if (cg->color == WHITE)
       return "White is stale mate.      ";
   return "Black is stale mate.      ";
} /* post_mortem */


void do_castle(ChessGame* cg, Move* c)
{
    int color = cg->board[c->from]->color;

    cg->CastlingOn[color][LEFT]=0;
    cg->CastlingOn[color][RIGHT]=0;
    cg->board[c->to] = cg->board[c->from];
    cg->board[c->from] = 0;
    cg->board[c->to]->koor = c->to;
    TURN_BIT(cg->bmpieces[color], c->from);
    SET_BIT(cg->bmpieces[color], c->to);
    if (c->from-c->to>0) {      /* right */
        TURN_BIT(cg->bmpieces[color], c->from-24);
        SET_BIT(cg->bmpieces[color], c->from-8);
        cg->board[c->from-8] = cg->board[c->from-24];
        cg->board[c->from-24] = 0;
        cg->board[c->from-8]->koor = c->from-8;
    } else {
        TURN_BIT(cg->bmpieces[color], c->from+32);
        SET_BIT(cg->bmpieces[color], c->from+8);
        cg->board[c->from+8] = cg->board[c->from+32];
        cg->board[c->from+32] = 0;
        cg->board[c->from+8]->koor = c->from+8;
    }
} /* do_castle */


void back_castle(ChessGame* cg, int from, int to)
{
    cg->CastlingOn[cg->board[to]->color][LEFT]=1;
    cg->CastlingOn[cg->board[to]->color][RIGHT]=1;
    cg->board[from] = cg->board[to];
    cg->board[from]->koor = from;
    cg->board[to] = 0;
    if (from-to>0) {    /* right */
        cg->board[from-24] = cg->board[from-8];
        cg->board[from-8] = 0;
        cg->board[from-24]->koor = from-24;
    } else {
        cg->board[from+32] = cg->board[from+8];
        cg->board[from+8] = 0;
        cg->board[from+32]->koor = from+32;
    }  
} /* back_castle */


void update_board(ChessGame* cg, Move* c, int* material)
{
    const int color=cg->board[c->from]->color;

    *material += c->val;

    if (c->castle) {
        do_castle(cg, c);
        return;
    }

    if (c->en_passant) 
        cg->board[(c->to/8)*8 + (c->from % 8)] = 0;

    if (c->kill) 
        c->kill->alive = 0;

    cg->board[c->to] = cg->board[c->from];
    cg->board[c->to]->koor = c->to;
    cg->board[c->from] = 0;

    /* Update bitmaps */
    if (c->transform) {
        pawn_transform(cg->board[c->to]);
        TURN_BIT(cg->bmpawns[color], c->from);
    }
    TURN_BIT(cg->bmpieces[color], c->from);
    SET_BIT(cg->bmpieces[color], c->to);
    if (cg->board[c->to]->type==pawn) {
        TURN_BIT(cg->bmpawns[color], c->from);
        SET_BIT(cg->bmpawns[color], c->to);
    }
    if (c->kill) {
       TURN_BIT(cg->bmpieces[!color], c->kill->koor);
       if (c->kill->type==pawn)
           TURN_BIT(cg->bmpawns[!color], c->kill->koor);
    }
} /* update_board */


void backdate_board(ChessGame* cg, Move* c, uint64* bitmaps)
{
    cg->bmpieces[WHITE] = bitmaps[0];
    cg->bmpieces[BLACK] = bitmaps[1];
    cg->bmpawns[WHITE] = bitmaps[2];
    cg->bmpawns[BLACK] = bitmaps[3];
  
    if (c->castle) {
        back_castle(cg, c->from, c->to);
        return;
    }

    if (c->transform)
        pawn_transform_back(cg->board[c->to]);
    cg->board[c->from] = cg->board[c->to];
    if (c->kill) {
        c->kill->alive = 1;
        if (c->en_passant) {
            cg->board[(c->to/8)*8 + (c->from % 8)] = c->kill;
            cg->board[c->to] = 0;
        } else
            cg->board[c->to] = c->kill;
    } else
        cg->board[c->to] = 0;
    cg->board[c->from]->koor = c->from;
} /* backdate_board */


void UpdateCastlingPermision(ChessGame* cg)
{
    const int row[2]={0,7};

    if (!cg->board[24+row[cg->color]]) { // King has moved
        cg->CastlingOn[cg->color][LEFT]=0;
        cg->CastlingOn[cg->color][RIGHT]=0;
        return;
    }

    if (cg->CastlingOn[cg->color][RIGHT]) 
        if (cg->board[row[cg->color]]==NULL ||  // RIGHT rook has moved
            cg->board[row[cg->color]]->color != cg->color) // RIGHT rook captured
            cg->CastlingOn[cg->color][RIGHT]=0;

    if (cg->CastlingOn[cg->color][LEFT]) 
       if (cg->board[56+row[cg->color]]==NULL ||  // LEFT rook has moved
           cg->board[56+row[cg->color]]->color != cg->color) // LEFT rook captured
           cg->CastlingOn[cg->color][LEFT]=0;
}/* UpdateCastlingPermision */


void shift_killers(ChessGame* cg)
{
    int i;
    for (i=0; i<MAX_DEPTH-1; i++) {
        cg->kill_tab[i].k1=cg->kill_tab[i+1].k1;
        cg->kill_tab[i].k2=cg->kill_tab[i+1].k2;
        if (cg->kill_tab[i+1].n1>0) cg->kill_tab[i].n1=1;
        if (cg->kill_tab[i+1].n2>0) cg->kill_tab[i].n2=1;
    }
    cg->kill_tab[i].n1=0;
    cg->kill_tab[i].n2=0;
} // shift_killers


void make_move(ChessGame* cg)
{
    int i;
    unsigned long int adr; /* address  in RepTable */

    if (cg->next.kill || cg->board[cg->next.from]->type==pawn) {
        for (i=0; i<REP_SIZE; i++) /* non-reversible move => clear RepTable */
            cg->RepTable[i].nr=0;
        for (i=0; i<HASH_SIZE; i++)             // slet hash-tabellen 
            cg->trans_table[i].height=0;
    }
    count_rep(cg, cg->next.hash_key, &adr); // lookup adr
    cg->RepTable[adr].nr += 1;
    UpdateCastlingPermision(cg);
    cg->material = eval_material(cg);
    update_board(cg, &cg->next, &cg->material);
    cg->color    = !cg->color;
    cg->last=cg->next;
    if (abs_eval_material(cg) < 0.3*cg->max_abs_val) 
        adjust_king_val(cg, 1);
    shift_killers(cg);
} // make_move


Move* find_move(int from, int to, Move* pos, int num)
{
    int i;
    for (i=0; i<num; i++) 
        if (from==pos[i].from && to==pos[i].to)
            return &pos[i];
    return NULL;        
} // find_move


// Return true/false - indicate if move is legal
int is_legal(ChessGame* cg, Move* move)
{
    Move pos[100], *p;
    int num, material=0, flag;
    uint64 bitmaps[4];
       
    num=get_pos(cg, &cg->last, cg->color, pos);
    p=find_move(move->from, move->to, pos, num);
    if (p==NULL) return 0;
    *move=*p; // fill out the remaining move fields
    get_bitmaps(cg, bitmaps);
    update_board(cg, p, &material);
    flag=in_check(cg, cg->color);
    backdate_board(cg, p, bitmaps);
    return (flag==0);
} // is_legal


int update_game(ChessGame* cg)
{
    if (is_legal(cg, &cg->next)) {
        make_move(cg);
        return (cg->next.kill!=NULL);
    }
    printf("Illegal move:"); 
    PrintMove(&cg->next);
    PrintBoard(cg);
    return -1;
} // update_game


void Swap(Move* p1, Move* p2)
{
    Move temp;

    temp=*p1;
    *p1=*p2;
    *p2=temp;
} /* Swap */


/* (Bubble-)Sort the moves in pos[], acording to score[]. */
void SortMoves(Move pos[], int num, int score[])
{
    int i, j, temp;

    for (i=0; i<num-1; i++)
       for (j=num-1; j>=i+1; j--)
           if (score[j]>score[j-1]) {
                Swap(&pos[j], &pos[j-1]);
                temp = score[j];
                score[j]=score[j-1];
                score[j-1]=temp;
           }
} /* SortMoves */


int get_legal_moves(ChessGame* cg, Move *moves)
{
    int i, j=0, num;
    Move pos[100];
    num=get_pos(cg, &cg->last, cg->color, pos);
    for (i=0; i<num; i++)
        if (is_legal(cg, &pos[i])) 
            moves[j++]=pos[i];
    return j;
} // get_legal_moves


int game_over(ChessGame* cg)
{
    Move pos[100], *p;
    int num, material=0, i, fifty;
    unsigned long int adr; /* address  in RepTable */
    uint64 bitmaps[4];

    fifty=0;
    for (i=0; i<REP_SIZE; i++) 
       if (cg->RepTable[i].nr>0)
           fifty++;
    if (fifty>=100) {
        //printf("Draw by the fifty move rule\n");
        return(1);          /* Draw by the 50-move rule */
    }

    if (count_rep(cg, cg->last.hash_key, &adr)>=3) {
        //printf("draw by 3xrep\n");
        return(1);
    } 

    num=get_pos(cg, &cg->last, cg->color, pos);
    if (num==0) {
       //printf("no possible moves\n");
       return(1);
    }

    get_bitmaps(cg, bitmaps);
    p=&pos[0];
    while (p) {
        update_board(cg, p, &material);
        if (!in_check(cg, cg->color)) {
             backdate_board(cg, p, bitmaps);
             return(0);
        }
        backdate_board(cg, p, bitmaps);
        p=p->sibling;
    } // while 
    //printf("it is over\n");
    return(1);
} /* game_over */


void Retrieve(ChessGame* cg, uint64 key, int* bound, int* height, int* score, Move* move)
{
    unsigned long int address;
    int i;

    address=key & HASH_MASK;
    for (i=0; i<4; i++) {
        address = (address + i) % HASH_SIZE;
        if (cg->trans_table[address].height==0) {
            *height=-INFINITE;
            return ;
        }
        if (cg->trans_table[address].hash_key==key) {
            *bound = cg->trans_table[address].bound;
            *height = cg->trans_table[address].height;
            *score = cg->trans_table[address].score;
            move->from =  cg->trans_table[address].from;
            move->to = cg->trans_table[address].to;
            return ;
        }
    }
    *height=-INFINITE;
    return ;
} /* Retrieve */


void Store(ChessGame* cg, uint64 key, int height, int score, int alpha, int beta, Move* move)
{
    unsigned long int address;
    int i;

    address=key & HASH_MASK;
    for(i=0; i<3; i++) {                        /* Check 3. first poss. */
        address = (address + i) % HASH_SIZE;    /* if occupied, then store */
        if (cg->trans_table[address].height==0) /* in no. 4 */
              break;
        if (cg->trans_table[address].hash_key==key)
              break;
    }

    if (cg->trans_table[address].height<=height) { /* Overwrite old possition */ 
        cg->trans_table[address].bound = EXACT;
        if (score<=alpha)
            cg->trans_table[address].bound = UPPER_BOUND;
        if (score>=beta)
            cg->trans_table[address].bound = LOWER_BOUND;
  
        cg->trans_table[address].hash_key = key;
        cg->trans_table[address].height = height;
        cg->trans_table[address].score = score;
        cg->trans_table[address].from = move->from;
        cg->trans_table[address].to = move->to;
    }
} /* Store */


// Move killers to the front
void move_killers(int num, Move pos[], Move** first, Move* killers, int n_killers)
{
    Move *prev, *p, *head;
    int j;

    if (num==0) {        /* no legal moves */
        *first=0;
        return;
    }

    head=&pos[0];
    for (j=n_killers-1; j>=0; j--) {
        prev=0;
        for (p=head; p; p=p->sibling) {
            if (killers[j].to==p->to && killers[j].from==p->from) {
               if (prev!=0) {
                   prev->sibling = p->sibling;
                   p->sibling=head;
                   head=p;
               }
               break;
            }
            prev=p;
        }
    }
    *first=head;
} /* move_killers */


void update_killer_table(KillerTable* kill_tab, Move* p)
{
    if ((kill_tab->n1>0) && 
        (kill_tab->k1.from == p->from) &&
        (kill_tab->k1.to == p->to)) {
        (kill_tab->n1)++;
        return ;
    }

    if ((kill_tab->n2>0) && 
        (kill_tab->k2.from == p->from) &&
        (kill_tab->k2.to == p->to)) {
        (kill_tab->n2)++;
        if (kill_tab->n2 > kill_tab->n1) {
            int temp_n = kill_tab->n1; 
            Swap(&(kill_tab->k1), &(kill_tab->k2));
            kill_tab->n1 = kill_tab->n2; 
            kill_tab->n2 = temp_n; 
        }
    } else { 
        kill_tab->k2=*p;
        kill_tab->n2=1;
    }
} /* update_killer_table */


int reply_fab(ChessGame* cg, Move* last,
             int depth, int ply, int mt, unsigned char color, int alpha, int beta)
{
    int num, material;
    Move pos[100], *p;
    int score, best, legal_exist=0;
    uint64 bitmaps[4];

    num=get_rpos(cg, last, color, pos);
    if (num==0) {
        cg->mobility[color]=compute_mobility(cg, color);
        cg->mobility[!color]=compute_mobility(cg, !color);
        return eval(cg, color, mt);
    }
    cg->number += num;

    get_bitmaps(cg, bitmaps);

    best = -INFINITE + ply;
    for (p=&pos[0]; p; p=p->sibling) {
        material = mt;
        update_board(cg, p, &material);
        if (!in_check(cg, color)) {
            legal_exist=1;
            score = -reply_fab(cg, p, depth-1, ply+1, material, !color,
                     -beta, -MAX(alpha, best));
            if (legal_exist==0 || score > best) {
               best = score;
               if (best >= beta) {
                   backdate_board(cg, p, bitmaps);
                   return(best);
               }
            }
        } /* if */
        backdate_board(cg, p, bitmaps);
    } /* for */
    if (legal_exist==0) {
        cg->mobility[color]=compute_mobility(cg, color);
        cg->mobility[!color]=compute_mobility(cg, !color);
        return eval(cg, color, mt);
    }

    return(best);
} /* reply_fab */ 

/* Generate only Capture moves */
int quiescence_fab(ChessGame* cg, Move* last, int depth, int ply, int mt, int color, int alpha, int beta)
{
    int num, material=0;
    Move pos[100], *p;
    int score, best, quiescent;
    uint64 bitmaps[4];

    quiescent = cg->board[last->to]->type!=pawn || 
                ((last->to % 8) != 6 || 
                 (last->to % 8) != 1);

    best = -INFINITE + ply;
    if (!quiescent) {
        num=get_pos(cg, last, color, pos);
    } else {
        num=get_captures(cg, last, color, &cg->mobility[color], pos);
        cg->mobility[!color]=compute_mobility(cg, !color);
	if (num==0)
            return eval(cg, color, mt);
        //if (num==0)
        //    return -INFINITE + ply;
	/*
	printf("done eval\n"); fflush(stdout);
	printf("pawn st=%d\n", num); fflush(stdout);
	printf("mob w=%d\n", cg->mobility[WHITE]); fflush(stdout);
	printf("mob b=%d\n", cg->mobility[BLACK]); fflush(stdout);
	printf("material =%d\n", material); fflush(stdout);
        //cg->mobility[!color]=compute_mobility(cg, !color);
	*/
        //best=eval(cg, color, mt);
	//return best;
    }

    get_bitmaps(cg, bitmaps);
    cg->number += num;
    for (p=&pos[0]; p; p=p->sibling) {
        material = mt;
        update_board(cg, p, &material);
        if (!in_check(cg, color)) {
            if (quiescent)
               score = -reply_fab(cg, p, depth-1, ply+1,
                            material, !color, -beta, -MAX(alpha, best));
            else
               score = -quiescence_fab(cg, p, depth-1, ply+1,
                            material, !color, -beta, -MAX(alpha, best));
            if (score > best) {
               best = score;
               if (best >= beta) {
                   backdate_board(cg, p, bitmaps);
                   return(best);
               }
            }
        } else
            num--;
        backdate_board(cg, p, bitmaps);
    } /* for */
    if (num==0)
        return eval(cg, color, mt);

    return(best);
} /* quiescence_fab */ 


int pvs(ChessGame* cg, Move* last, Move* best_move, int depth, int ply, int mt, int color, int alpha, int beta)
{
    int num=0, material;
    Move pos[100], *p, *first, killers[3], best_child;
    int score, best;
    int legal_exist=0, n_killers=0;
    int bound, height;
    uint64 bitmaps[4];
    unsigned long int adr;

    if (count_rep(cg, last->hash_key, &adr)>=2) 
        return(0);

    (cg->RepTable[adr].nr)++;

    if (in_check(cg, color))
        depth++;

    Retrieve(cg, last->hash_key, &bound, &height, &score, &killers[0]);
    if (height>=depth) {
        if (bound==EXACT) {
            (cg->RepTable[adr].nr)--;
            return(score);      /* position seen before */
        }
        if (bound==LOWER_BOUND)
            alpha=MAX(alpha, score);  /* narrow the window */
        if (bound==UPPER_BOUND)
            beta=MIN(beta, score);     /* narrow the window */
        if (alpha>=beta) {
           (cg->RepTable[adr].nr)--;
           return(score);             /* cutoff */
        }
    }

    if (depth <= 0) {
        (cg->RepTable[adr].nr)--;
        return( quiescence_fab(cg, last, depth, ply+1, mt, color, alpha, beta) );
    }

    get_bitmaps(cg, bitmaps);
    best = -INFINITE + ply;

    /* Try move indicated in transpos.table */
    if (height>0) 
         n_killers++;
    /* Try the first killer move */
    if (cg->kill_tab[ply].n1>0 && ply<MAX_DEPTH) {
         killers[n_killers].from=cg->kill_tab[ply].k1.from;
         killers[n_killers].to=cg->kill_tab[ply].k1.to;
         n_killers++;
    }
    // Try the second killer move 
    if (cg->kill_tab[ply].n2>0 && ply<MAX_DEPTH) {
         killers[n_killers].from=cg->kill_tab[ply].k2.from;
         killers[n_killers].to=cg->kill_tab[ply].k2.to;
         n_killers++;
    }
    num=get_pos(cg, last, color, pos);
    cg->number+=num;
    move_killers(num, pos, &first, killers, n_killers);

    for (p=first; p; p=p->sibling) {
        material = mt;
        update_board(cg, p, &material);
        if (!in_check(cg, color)) { 
            if (!legal_exist) {
                *best_move=*p;
                legal_exist=1;
                best = -pvs(cg, p, &best_child, depth-1,
                       ply+1, material, !color, -beta, -MAX(best, alpha));
            }
            else {
                alpha=MAX(best, alpha);
                score = -pvs(cg, p, &best_child, depth-1, 
                        ply+1, material, !color, -alpha-1, -alpha);
                if (score > best) {
                    if (score>alpha && score<beta && depth>2)
                        score = -pvs(cg, p, &best_child,
                                depth-1, ply+1, material, !color, 
                                -beta, -score);
                    best = score;
                    *best_move=*p;
                }
            }
        } 
        backdate_board(cg, p, bitmaps);
        if (best>=beta) {
            if (ply<MAX_DEPTH)
                update_killer_table(&cg->kill_tab[ply], best_move);
            Store(cg, last->hash_key, depth, best, alpha, beta, best_move);
            (cg->RepTable[adr].nr)--;
            return best;
        }
    } /* for */ 

    (cg->RepTable[adr].nr)--;
    if (legal_exist==0 && !in_check(cg, color)) 
        return 0;

    Store(cg, last->hash_key, depth, best, alpha, beta, best_move);
    return(best);
} /* pvs */ 


void compute_move(ChessGame* cg, Move* best_child)
{
    int d, j, material, mt, num; 
    int alpha, beta, best=0, score[100];
    clock_t t0;
    Move pos[100], BChild;
    uint64 bitmaps[4];

    num=get_legal_moves(cg, pos);
    cg->number = num;  // # generated moves
    cg->next=pos[0];   // make sure it is defined

    mt = eval_material(cg);
    get_bitmaps(cg, bitmaps);
    t0=clock();
    for (d=1; d<=cg->depth; d++) {

	// break off iterative search?
        if ( abs(best)>INFINITE/2 ||    // check mate
             (cg->max_search>0 && cg->max_search<cg->number) ||
	     (cg->max_time>0 && cg->max_time<(clock()-t0)/(float) CLOCKS_PER_SEC) 
	    )
            break;
        alpha = -INFINITE;
        best = -INFINITE;   /* 1. traek huskes uht. vaerdi */
        beta = INFINITE;

        // search first move with full beam
        material = mt;
        update_board(cg, &pos[0], &material);
        score[0] = -pvs(cg, &pos[0], &BChild, 
                           d-1, 1, material, !cg->color, 
                           -beta, -MAX(alpha, best));
        best = score[0];
        cg->next=pos[0];
        *best_child=BChild;
        cg->next.val=best;
        backdate_board(cg, &pos[0], bitmaps);

        for (j=1; j<num; j++) { 
            if (best>=beta) 
                return ;

            material = mt;
            update_board(cg, &pos[j], &material);
            alpha=MAX(best, alpha);
            score[j] = -pvs(cg, &pos[j], &BChild, 
                       d-1, 1, material, !cg->color, -alpha-1, -alpha);
            if (score[j] > best) {
                if (score[j]>alpha && score[j]<beta && cg->depth>2)
                    score[j] = -pvs(cg, &pos[j], &BChild,
                               d-1, 1, material, !cg->color, 
                               -beta, -score[j]);
                best=score[j];
                cg->next=pos[j];
                *best_child=BChild;
                cg->next.val=best;
            } 
            backdate_board(cg, &pos[j], bitmaps);
        } /* for */ 

        SortMoves(&pos[0], num, &score[0]);
    } /* for */
    cg->depth_actual=d-1;

    if (cg->color==BLACK)
        cg->next.val*=-1;
} /* compute_move */ 
