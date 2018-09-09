/*
 *  Copyright (c) 2004 Jesper Olsen
 *  License: MIT, see License.txt
 *
 *  Move generation.
 */

/* Generate all pseudo legal moves */
int get_pos(ChessGame* cg, Move* last, int color, Move pos[]);

/* Generate only caputres */
int get_captures(ChessGame* cg, Move* last, int color, unsigned int *mobility, Move pos[]);

/* Generate replys to last move*/
int get_rpos(ChessGame* cg, Move* last, int color, Move pos[]);

int compute_mobility(ChessGame* cg,  int color);

//assignment
void move_eq(Move* move1, Move* move2);

//true if color is in check
int in_check(ChessGame* cg, int color);

int pawn_can_capture(void* p, int to, uint64 PieceBitmap);
int king_can_capture(void* p, int to, uint64 PieceBitmap);
int knight_can_capture(void* p, int to, uint64 PieceBitmap);
int bishop_can_capture(void* p, int to, uint64 PieceBitmap);
int rook_can_capture(void* p, int to, uint64 PieceBitmap);
int queen_can_capture(void* p, int to, uint64 PieceBitmap);

