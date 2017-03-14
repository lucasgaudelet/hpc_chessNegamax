#ifndef MASTER_SLAVE
#define MASTER_SLAVE

#define TAG_TASK	0
#define TAG_RESULT	1
#define	TAG_END		2

#define ROOT	0

#include <mpi.h>
#include <stddef.h>

#include "projet.h"

extern unsigned long long int node_searched;

void create_mpi_result_t(MPI_Datatype* MPI_RESULT_T);

void broadcast_end(int np);

void evaluate(tree_t * T, result_t *result);

void master_evaluate(tree_t * T, result_t *result);

void slave_evaluate(tree_t * T, result_t *result);

#endif

#ifndef BAD_MOVE
#define BAD_MOVE 0xdead
#endif
