/*
 *  Copyright (c) 2004 Jesper Olsen
 *  License: MIT, see License.txt
 *          
 */


ChessGame* new_chess_game(void);
void free_chess_game(ChessGame* cg);
void compute_move(ChessGame* cg, Move* best_child);
int game_over(ChessGame* cg);
char* post_mortem(ChessGame* cg);
int is_legal(ChessGame* cg, Move* move);
int update_game(ChessGame* cg);
