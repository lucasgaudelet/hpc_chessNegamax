#include "projet.h"
#include "master_slave.h"

/* 2017-02-23 : version 1.0 */

unsigned long long int node_searched = 0;
double time_tot = 0;

void decide(tree_t * T, result_t *result)
{	
	/* MPI Parameters */
	int decision_reached=0;
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	/* depth loop */
	for (int depth = 1; !decision_reached; depth++) {
	
		T->depth = depth;
		T->height = 0;
		T->alpha_start = T->alpha = -MAX_SCORE - 1;
		T->beta = MAX_SCORE + 1;
		
		// master
		if(rank==ROOT) {
				printf("=====================================\n");
			
			//double time_depth = MPI_Wtime();
			(depth>DEPTH_PAR)? master_evaluate(T, result):evaluate(T, result);
			//time_depth = MPI_Wtime()-time_depth;

				printf("[ROOT] depth: %d / score: %.2f / best_move : \n",T->depth,0.01*result->score);
				//printf("time: %f\n",time_depth);
				//print_pv(T, result);
			
			if (DEFINITIVE(result->score)) {
				decision_reached = 1;
        		}
			// send decision to all slaves ==> broadcast ?
			printf("[ROOT] broadcast decision_reached\n");
			MPI_Bcast( &decision_reached, 1, MPI_INT, ROOT, MPI_COMM_WORLD);
			printf("[ROOT] decision_reached=%d\n", decision_reached);
		}
		// slave
		else {
			if(depth>DEPTH_PAR)	slave_evaluate(T, result);
			MPI_Bcast( &decision_reached, 1, MPI_INT, ROOT, MPI_COMM_WORLD);
			//printf("[%d] decision_reached=%d\n", rank, decision_reached);
		}
                
        
	}
}


int main(int argc, char **argv)
{
	tree_t root;
	result_t result;

	if (ALPHA_BETA_PRUNING)
		printf("Alpha-beta pruning ENABLED\n");

	if (TRANSPOSITION_TABLE) {
		printf("Transposition table ENABLED\n");
		init_tt();
	}
	
	/* Initiation of the MPI layer */
	MPI_Init(&argc, &argv);
	
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	create_mpi_result_t(&MPI_RESULT_T);	

	if (argc < 2) {
        	printf("usage: %s \"4k//4K/4P w\" (or any position in FEN)\n", argv[0]);
                exit(1);
        }
	
	parse_FEN(argv[1], &root);
	if(rank==ROOT) print_position(&root);

	time_tot = MPI_Wtime();
	decide(&root, &result);
	time_tot = MPI_Wtime() - time_tot;
	
	if( rank==ROOT ) {
		unsigned long long int node_tot = 0;
		MPI_Reduce( &node_searched, &node_tot, 1, MPI_INT, MPI_SUM, ROOT, MPI_COMM_WORLD);
		
		printf("\nDécision de la position: ");
		switch(result.score * (2*root.side - 1)) {
			case MAX_SCORE: printf("blanc gagne\n"); break;
			case CERTAIN_DRAW: printf("partie nulle\n"); break;
			case -MAX_SCORE: printf("noir gagne\n"); break;
			default: printf("BUG\n");
		}

		printf("Node searched: %llu\t time: %f\n", node_tot, time_tot);
	}
	else {
		MPI_Reduce( &node_searched, NULL, 1, MPI_INT, MPI_SUM, ROOT, MPI_COMM_WORLD);
	}

	if (TRANSPOSITION_TABLE)
		free_tt();
	
	MPI_Type_free(&MPI_RESULT_T);		
	MPI_Finalize();
	
	return 0;
}
