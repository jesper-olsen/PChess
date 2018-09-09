/*
 *  Copyright (c) 2004 Jesper Olsen
 *  License: MIT, see License.txt
 *
 *  Initialisation of bitmaps + definition of piece value.
 */

#define MAX(x,y) ((x)>(y)?x:y)
#define MIN(x,y) ((x)<(y)?x:y)
#define NELM(array) (sizeof(array)/sizeof(array[0]))

int setup(ChessGame* cg, char* fene, char* castling, int nc);
void set_bit2(uint64* bitmap, int x, int y);
void GetBitmaps(ChessGame* cg, uint64* bitmaps);
void print_bitmap(uint64 b);
void CalcPieceBitmaps(ChessGame* cg);
void get_chess_board(ChessGame* cg);
int eval_material(ChessGame* cg);
int eval_pawn_structure(ChessGame* cg);
int abs_eval_material(ChessGame* cg);
void adjust_king_val(ChessGame* cg, int is_end);
void pawn_transform(Piece* piece);
void pawn_transform_back(Piece* piece);

void get_fene(ChessGame* cg, char *buffer);
void get_castling_rights(ChessGame* cg, char *buffer);

void PrintBoard(ChessGame* cg);
void PrintMove(Move* c);

extern uint64 distance[64][64], Knight[64], King[64];
extern FourRay Rook[64], Bishop[64];

extern uint64 pawn_key[2][64];
extern uint64 rook_key[2][64];
extern uint64 knight_key[2][64];
extern uint64 bishop_key[2][64];
extern uint64 queen_key[2][64];
extern uint64 king_key[2][64];
extern uint64 en_passant_key[64];
extern uint64 wb_key;

extern uint64 power[64];  /* tabel over 2^n fra n=0,...,63 */
