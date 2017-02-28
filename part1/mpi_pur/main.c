#include "projet.h"

/* 2017-02-23 : version 1.0 */

unsigned long long int node_searched = 0;

void evaluate(tree_t * T, result_t *result)
{
        node_searched++;
  
        move_t moves[MAX_MOVES];
        int n_moves;

        result->score = -MAX_SCORE - 1;
        result->pv_length = 0;
        
        if (test_draw_or_victory(T, result))
          return;

        if (TRANSPOSITION_TABLE && tt_lookup(T, result))	/* la réponse est-elle déjà connue ? */
          return;
        
        compute_attack_squares(T);

        /* profondeur max atteinte ? si oui, évaluation heuristique */
        if (T->depth == 0) {
          result->score = (2 * T->side - 1) * heuristic_evaluation(T);
          return;
        }
        
        n_moves = generate_legal_moves(T, &moves[0]);

        /* absence de coups légaux : pat ou mat */
		if (n_moves == 0) {
          result->score = check(T) ? -MAX_SCORE : CERTAIN_DRAW;
          return;
        }
        
        if (ALPHA_BETA_PRUNING)
          sort_moves(T, n_moves, moves);

        /* évalue récursivement les positions accessibles à partir d'ici */
        for (int i = 0; i < n_moves; i++) {
			tree_t child;
                result_t child_result;
                
                play_move(T, moves[i], &child);
                
                evaluate(&child, &child_result);
                         
                int child_score = -child_result.score;

			if (child_score > result->score) {
				result->score = child_score;
				result->best_move = moves[i];
	                    result->pv_length = child_result.pv_length + 1;
	                    for(int j = 0; j < child_result.pv_length; j++)
	                    	result->PV[j+1] = child_result.PV[j];
	                    result->PV[0] = moves[i];
	            }

            if (ALPHA_BETA_PRUNING && child_score >= T->beta)
              break;

            T->alpha = MAX(T->alpha, child_score);
        }

        if (TRANSPOSITION_TABLE)
          tt_store(T, result);
}


void master_evaluate(tree_t * T, result_t *result) {
	/* manque np, result_t function handle, appels MPI */
	move_t moves[MAX_MOVES];		// legal moves
	result_t* result_table;			// result table pointers
	int* result_index;	// result_index[i] stores the index j of moves
						// corresponding to result_table[i]
	int* slave_work;	// slave_work[i] stores the index of the move(s) that
						// slave i is currently working on. 
    int n_moves;		// number of legal moves
    
    int current_work;	// index of the next move to be evaluated
    int result_nb;		// number of results received in result_table
    int current_result;	// the index of the next result to analyse 
    					// in result_table
    
    // Initialise les valeurs de result
    result->score = -MAX_SCORE - 1;
    result->pv_length = 0;
    
	n_moves = generate_legal_moves(T, &moves[0]);
	
	if (test_draw_or_victory(T, result))
          // envoyer end à tous les processus
	
	// absence de coups légaux : pat ou mat
	if (n_moves == 0) {
      result->score = check(T) ? -MAX_SCORE : CERTAIN_DRAW;
      // envoyer end à tous les processus
    }
    
    // allocations and initialisations
	result_table = (result_t*)malloc(n_moves*sizeof(result_t));
	result_index = (int*)malloc( n_moves*sizeof(int) );
	slave_work = (int*)malloc( 2*np*sizeof(int) );
	current_work = current_result = result_nb = 0;
	
	if (ALPHA_BETA_PRUNING)
          sort_moves(T, n_moves, moves);

	for(int i=1; i<np; i++) {
		// Si on peut envoyer deux taches
		if( current_work+1 < n_moves ) {
			// send( moves+current_work, 1, ... )
			slave_work[2*i+0] = current_work++;
			// send( moves+current_work, 1, ... )
			slave_work[2*i+1] = current_work++;
		}
		// Si on peut envoyer une tache
		else if( current_work < n_moves ) {
			// send( moves+current_work, 1, ... )
			slave_work[2*i+0] = current_work++;
			// send end signal
			// slave_work[2*i+1] = -1;
		}
		// Si il n'y a plus rien à faire
		else {
			// send end signal
		}
	}
	
	// tant qu'il y a des taches à accomplir ou que tout n'as pas été analysé
	while( (current_work < n_moves) ||
		   ( (result_nb<n_moves) && (current_result<result_nb) ) ) {
		   
		// Si resultat reçu   
		if(  ) {
			// receive result_table[result_nb]
			result_index[result_nb++] = slave_work[2*slave_id+0];
			slave_work[2*slave_id+0] = slave_work[2*slave_id+1]
			
			// S'il reste des taches à accomplir
			if( current_work < n_moves) {
				// send( moves+current_work, 1, ... )
				slave_work[2*i+1] = current_work++;
			}
			// Sinon...
			else {
				// send end signal
				// slave_work[2*i+1] = -1;
			}
		}
		
		// Si des resultats peuvent etre analyses
		else if( current_result<result_nb ) {
		
			int child_score = - result_table[current_result].score;
		
			if( child_score > result->score ) {
				result->score = child_score;
				result->best_move = moves[ result_index[current_result] ];
				
				result->pv_length = result_table[current_result].pv_length + 1;
				
				for(int j = 0; j < result_table[current_result].pv_length; j++)
                	result->PV[j+1] = result_table[current_result].PV[j];
                	
                result->PV[0] = moves[ result_index[current_result] ];
			}
		}
	}
	free(result_index);
	free(slave_work);
	free(result_table);
}


void slave_evaluate(tree_t * T, result_t *result) {
	/* manque param mpi (statut, etc...) */
	result->score = -MAX_SCORE - 1;
    result->pv_length = 0;
        
	move_t move[2];	
	move = next_move = BAD_MOVE;
	
	result_t child_result[2];
	int i=0;
	
	compute_attack_squares(T);
	
	// reception de la premiere tache
	// recv( move+i, 1, int, ...)	
	
	// tant qu'il y a des taches à accomplir
	while( move[i] != BAD_MOVE ) {
		
		// reception de la tache suivante pendant le calcul de la premiere
		// recv( move+(1-i), 1, int, ...) 
	
		tree_t child;
		
		play_move(T, move[i], &child);				// joue le move
		evaluate(&child, &child_result[i]);		// evalue recursivement la position
		
		// envoi du resultat
		// send( child_result[i], 1, result_t, ...)		
		
		// preparation de l'iteration suivante
		i = 1-i;
	}
}


void decide(tree_t * T, result_t *result)
{
	for (int depth = 1;; depth++) {
		T->depth = depth;
		T->height = 0;
		T->alpha_start = T->alpha = -MAX_SCORE - 1;
		T->beta = MAX_SCORE + 1;

                printf("=====================================\n");
		evaluate(T, result);

                printf("depth: %d / score: %.2f / best_move : ", T->depth, 0.01 * result->score);
                print_pv(T, result);
                
        if (DEFINITIVE(result->score))
        	break;
	}
}

int main(int argc, char **argv)
{
	  
	tree_t root;
        result_t result;
	 
	/* Initiation of the MPI layer */
	int	rank, np;	// MPI variables 

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &np);
	
	// creating MPI function handle for result_t struct
	MPI_Datatype MPI_RESULT_T;			// new type function handle
	int count = 4;						// number of block
	int blocklen[4] = { 1, 1, 1, 128 };	// block length
    MPI_Datatype type[3] = { MPI_INT, MPI_INT, MPI_INT, MPI_INT }; // block type
	MPI_Aint displacement[4];	// block displacement
	displacement[0] = &result.score - &result;
	displacement[1] = &result.best_move - &result;
	displacement[2] = &result.pv_length - &result;
	displacement[3] = &result.PV - &result;
	
	MPI_Type_create_struct(4, blocklen, displacement, type, &MPI_RESULT_T);
	MPI_Type_commit(&MPI_RESULT_T);
	
        if (argc < 2) {
          printf("usage: %s \"4k//4K/4P w\" (or any position in FEN)\n", argv[0]);
          exit(1);
        }

        if (ALPHA_BETA_PRUNING)
          printf("Alpha-beta pruning ENABLED\n");

        if (TRANSPOSITION_TABLE) {
          printf("Transposition table ENABLED\n");
          init_tt();
        }
        
        parse_FEN(argv[1], &root);
        print_position(&root);
        
	decide(&root, &result);

	printf("\nDécision de la position: ");
        switch(result.score * (2*root.side - 1)) {
        case MAX_SCORE: printf("blanc gagne\n"); break;
        case CERTAIN_DRAW: printf("partie nulle\n"); break;
        case -MAX_SCORE: printf("noir gagne\n"); break;
        default: printf("BUG\n");
        }

        printf("Node searched: %llu\n", node_searched);
        
        if (TRANSPOSITION_TABLE)
          free_tt();
	return 0;
}
