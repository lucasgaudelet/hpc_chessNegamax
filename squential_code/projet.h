#define _POSIX_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <err.h>

/* 2017-02-23 : version 1.0 */

/******************************************************** 
 * Faire passer ALPHA_BETA_PRUNING à 1 pour la 2ème phase du projet. 
 * Faire passer TRANSPOSITION_TABLE à 1 pour un véritable challenge !
 ********************************************************/

#define ALPHA_BETA_PRUNING  0
#define TRANSPOSITION_TABLE 0

/*******************************************************
 Macros, définitions, types de données
******************************************************/

#define DEFINITIVE(x) (x==CERTAIN_DRAW||abs(x) > 900000) 
#define MAX_SCORE 1000000
#define CERTAIN_DRAW 0

#define MAX_MOVES 32
#define MAX_DEPTH 128

#define MAX(x,y) ((x> y) ?x:y)
#define MIN(x,y) ((x<y) ?x:y)

/*** structures de données ***/
typedef int square_t;
typedef int move_t;

typedef char board_t[128];


/* répresentation d'une position */
typedef struct tree_t {
  board_t pieces;   /* couleur des pièces */
  board_t colors;   /* nature des pièces */
  int side;         /* joueur dont c'est le tour */
  
  int depth;        /* nombre d'appels récursifs à evaluate encore autorisés */
  int height;       /* nombre d'appels récursifs depuis la racine */
  int alpha;        /* borne inférieure sur le score qu'il est possible d'obtenir */
  int beta;         /* borne supérieure ... */
  int alpha_start;  /* valeur de alpha au début de l'exploration de la position */

  square_t king[2]; /* positions des deux rois */
  int pawns[2];     /* nombre de pions de chaque joueur */
  board_t attack;   /* cases attaquées par les différentes pièces */

  move_t suggested_move;         /* meilleur coup suggéré par la table de hachage */
  unsigned long long int hash;   /* haché de la position (technique de zobrist) */
  unsigned long long int history[MAX_DEPTH];  /* hachés des positions rencontrées depuis la racine de l'exploration */
} tree_t;


/* représentation du résultat de l'évaluation d'une position */
typedef struct {
  int score;
  move_t best_move;
  int pv_length;
  int PV[MAX_DEPTH];
} result_t;

/********************************************
 * Fonctions auxiliaires
 *******************************************/

/* affiche le coup [move] */
void print_move(move_t move);

/* affiche la position [T] */
void print_position(tree_t * T);

/* détermine un score approximatif de la position [T] Largement améliorable. */
int heuristic_evaluation(tree_t * T);

/* détermine quelles cases sont attaquées par chaque camp. Ceci est nécessaire
   pour l'évaluation heuristique, et pour la génération des coups légaux. Le
   résultat est stocké dans [T]. */
void compute_attack_squares(tree_t *T);

/* Indique si le joueur dont c'est le tour est échec. */
int check(tree_t *T);

/* Détermine si la partie est nulle (absence de pions...) ou si le joueur dont
   c'est le tour a gagné. */
int test_draw_or_victory(tree_t *T, result_t *result);

/* détermine les coups légaux à partir de la position [T]. Ils sont écrits dans
   le tableau [moves]. Leur nombre est renvoyé. */
int generate_legal_moves(tree_t *T, move_t *moves);

/* trie la liste des coups légaux par force (supposée) décroissante. Tester le
   meilleur coup d'abord est avantageux, puisque ça tend à provoquer des
   coupures dans les suivants. */
void sort_moves(tree_t *T, int n_moves, move_t *moves);

/* écrit dans [child] la position obtenue en jouant le coup [move] à partir de
   [T]. */
void play_move(tree_t *T, move_t move, tree_t *child);


/* récupère dans la table de hachage des informations sur [T]. Met à jour [T] et
   [result]. Renvoie 1 si [result] contient l'évaluation complète de [T]. Sinon,
   les bornes alpha et beta dans [T] peuvent être modifiées, et un meilleur coup
   potentiel suggéré. */
int tt_lookup(tree_t *T, result_t *result);

/* stocke dans la table de hachage le résultat de l'évaluation de [T]. */
void tt_store(tree_t *T, result_t *result);


int tt_fetch(tree_t *T, result_t *result);

/* initialise la position [root] à partir de la chaine de caractère [FEN], qui
   est en "Forsyth-Edwards Notation" (cf. Wikipedia) */
void parse_FEN(const char *FEN, tree_t *root);

/* alloue et initialise la table de hachage */
void init_tt();

/* libère la table de hachage et affiche des statistiques */
void free_tt();

/* affiche la variation principale et la position résultante */
void print_pv(tree_t *T, result_t *result);
