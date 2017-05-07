#ifndef MASTER_SLAVE
#define MASTER_SLAVE

/* include */

#include <mpi.h>
#include <stddef.h>

#include "projet.h"

/* constants */
#define TAG_TASK	0
#define TAG_RESULT	1
#define TAG_PRUNE	2
#define TAG_ALPHA	3
#define	TAG_END		4

#define ROOT		0
#define DEPTH_PAR	3
#define LEVEL_MAX	1

/* variables and data types*/
extern unsigned long long int node_searched;
extern double time_tot;
MPI_Datatype	MPI_RESULT_T;	// result_t function handle
MPI_Datatype	MPI_TREE_T;	// tree_t function handle

typedef struct node_t {
	struct node_t* father;
	struct node_t* sons;
	
	tree_t tree;
	result_t result;
	
	move_t moves[MAX_MOVES];
	int n_moves;
	
	int current_work;
	int result_nb;
	
} node_t;

/* functions */

// function handle creation
void create_mpi_result_t(MPI_Datatype* MPI_RESULT_T);
void create_mpi_tree_t(MPI_Datatype* MPI_RESULT_T);



// control tree functions
void generate_control_tree(node_t* root, int level);
void free_control_tree(node_t* root);
node_t* next_task(node_t* root);
void manage_result(result_t* result, node_t** slave_work, node_t* slave);
void prune(node_t** slave_work, node_t* node);
void broadcast_alpha(int alpha);

// evaluate functions
void evaluate(tree_t * T, result_t *result);
int evaluate_comm(tree_t * T, result_t *result);
void master_evaluate(node_t* root);
void slave_evaluate();

// miscellaneous
void broadcast_end(int np);

#endif

#ifndef BAD_MOVE
#define BAD_MOVE 0xdead
#endif
