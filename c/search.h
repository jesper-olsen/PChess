/*
 *  Copyright (c) 2004 Jesper Olsen
 *  License: MIT, see License.txt
 *          
 */

ChessGame* new_chess_game(void);
void free_chess_game(ChessGame* cg);
void compute_move(ChessGame* cg);
int game_over(ChessGame* cg);
char* post_mortem(ChessGame* cg);
int is_legal(ChessGame* cg, Move* move);
int update_game(ChessGame* cg);
int get_legal_moves(ChessGame* cg, Move *moves);

const char* get_legal_moves_str(ChessGame* cg);
int get_from(ChessGame* cg);
int get_to(ChessGame* cg);
int get_is_kill(ChessGame* cg);
int get_val(ChessGame* cg);
int get_searched(ChessGame* cg);
int get_depth_actual(ChessGame* cg);
const char* get_turn(ChessGame* cg);
int get_piece(ChessGame* cg, int n, int* is_alive, int* ptype, int* pkoor, int* is_white);
int set_max_ply(ChessGame* cg, int max_ply);
void set_from(ChessGame* cg, int from);
void set_to(ChessGame* cg, int to);

