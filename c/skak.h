/* 
 *  Copyright (c) 2004 Jesper Olsen
 *  License: MIT, see License.txt
 *
 */

#define SET_BIT(b,bit) (b |= power[bit])
#define TURN_BIT(b,bit) (b ^= power[bit])
#define TEST_BIT(b,bit) ((b & power[bit])>0)

enum {
    rook  ='R',
    knight='N',
    bishop='B',
    queen ='Q',
    king  ='K',
    pawn  ='P',
    RIGHT=0,
    LEFT=1,
    WHITE=0,
    BLACK=1,
    REP_MASK=127,          /* 2^7 - 1 */
    REP_SIZE=REP_MASK+1,    
    //HASH_MASK=524287,      /* 2^19 - 1 */
    //HASH_MASK=262143,      /* 2^18 - 1 */
    //HASH_MASK=131071,      /* 2^17 - 1 */ 
    //HASH_MASK=65535,       /* 2^16 - 1 */
    HASH_MASK=262143,      /* 2^18-1 */
    HASH_SIZE=HASH_MASK+1,
};

#ifdef VISUALCPP
typedef __int64 uint64;
#else
typedef unsigned long long int uint64;
#endif

typedef struct {
    char type;
    unsigned char color;
    const int *val; /* pointer to a 8x8 array */
    uint64* hash_key;
    unsigned char alive;
    unsigned char koor;
    int (*can_capture)(void*,int,uint64);
} Piece;


typedef struct Move_t Move;
struct Move_t {
    unsigned char from;
    unsigned char to;
    unsigned char castle; 
    unsigned char transform;
    unsigned char en_passant;
    int val;
    uint64 hash_key; 
    Piece *kill;
    Piece *enpassant_cap;
    Move *child;
    Move *sibling;
};

  
typedef struct {
    int n1, n2;
    Move k1, k2;
} KillerTable;


typedef struct {
    uint64 hash_key;
    int score;
    char bound;  /* Is the score exact, a lower or an upper bound */
    char height; /* Height of the subtree searched under this position */
    char from; /* best move from this position */
    char to;
} TranspositionHash;


typedef struct {
    uint64 hash_key;
    int nr;
} RepetitionTable;


typedef struct {                /* static Bitmaps */
    uint64 ray[5];
} FourRay;

typedef struct 
{
    int number,       // # searched positions
        material,     // weighted sum
        depth,        // search depth - ply
        depth_actual, // search depth - ply
        max_search,   // don't extend search beyond search_max moves
        max_abs_val;
    float max_time;   // don't extend search beyond max_time seconds
    unsigned char color;
    Move last, next, best_child;
    KillerTable* kill_tab;
    TranspositionHash* trans_table;
    RepetitionTable RepTable[REP_SIZE];
    int CastlingOn[2][2];
    Piece* board[64]; 
    Piece piece[32];
    uint64 bmpieces[2], bmpawns[2];
    unsigned int mobility[2];
    char message[10000];
} ChessGame;

