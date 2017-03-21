#include "projet.h"
#include <mpi.h>

/* 2017-02-23 : version 1.0 */

unsigned long long int node_searched = 0;
double time_tot=0;
//double time_decide=0;
double time_evaluate=0;
double time_compute_attack_squares=0;
double time_heuristic_evaluation=0;
double time_generate_legal_moves=0;
double time_check=0;
double time_sort_moves=0;
double time_play_move=0;


void evaluate(tree_t * T, result_t *result)
{
        node_searched++;
	double tmp;

        move_t moves[MAX_MOVES];
        int n_moves;

        result->score = -MAX_SCORE - 1;
        result->pv_length = 0;
        
        if (test_draw_or_victory(T, result))
          return;

        if (TRANSPOSITION_TABLE && tt_lookup(T, result))     /* la réponse est-elle déjà connue ? */
          return;
        
	tmp = MPI_Wtime();
        compute_attack_squares(T);
	time_compute_attack_squares += MPI_Wtime() - tmp;

        /* profondeur max atteinte ? si oui, évaluation heuristique */
        if (T->depth == 0) {
	  tmp = MPI_Wtime();
          result->score = (2 * T->side - 1) * heuristic_evaluation(T);
	  time_heuristic_evaluation += MPI_Wtime() - tmp;
          return;
        }
        
	tmp = MPI_Wtime();
        n_moves = generate_legal_moves(T, &moves[0]);
	time_generate_legal_moves += MPI_Wtime() - tmp;

        /* absence de coups légaux : pat ou mat */
	if (n_moves == 0) {
	  tmp = MPI_Wtime();
          result->score = check(T) ? -MAX_SCORE : CERTAIN_DRAW;
	  time_check += MPI_Wtime() - tmp;
          return;
        }
        
        if (ALPHA_BETA_PRUNING) {
	  tmp = MPI_Wtime();
          sort_moves(T, n_moves, moves);
	  time_sort_moves += MPI_Wtime() - tmp;
	}

        /* évalue récursivement les positions accessibles à partir d'ici */
        for (int i = 0; i < n_moves; i++) {
		tree_t child;
                result_t child_result;
                
		tmp = MPI_Wtime();
                play_move(T, moves[i], &child);
                time_play_move += MPI_Wtime() - tmp;

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


void decide(tree_t * T, result_t *result)
{
	for (int depth = 1;; depth++) {
		T->depth = depth;
		T->height = 0;
		T->alpha_start = T->alpha = -MAX_SCORE - 1;
		T->beta = MAX_SCORE + 1;

                printf("=====================================\n");
		double time_depth = MPI_Wtime();
		evaluate(T, result);
		time_depth = MPI_Wtime()-time_depth;
		time_evaluate += time_depth;

		printf("time: %f\n",time_depth);
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
        
	time_tot = MPI_Wtime();
	decide(&root, &result);
	time_tot = MPI_Wtime() - time_tot;

	printf("\nDécision de la position: ");
        switch(result.score * (2*root.side - 1)) {
        case MAX_SCORE: printf("blanc gagne\n"); break;
        case CERTAIN_DRAW: printf("partie nulle\n"); break;
        case -MAX_SCORE: printf("noir gagne\n"); break;
        default: printf("BUG\n");
        }

        printf("Node searched: %llu\t time: %f\n", node_searched, time_tot);

	printf("time decide: %f\n",time_tot);
	printf("\ttime evaluate: %.2f%%\n",100*time_evaluate/time_tot);
	printf("\t\ttime compute_attack_squares: %.2f%%\n",100*time_compute_attack_squares/time_evaluate); 
	printf("\t\ttime heuristic_evaluation: %.2f%%\n",100*time_heuristic_evaluation/time_evaluate);
	printf("\t\ttime generate_legal_moves: %.2f%%\n",100*time_generate_legal_moves/time_evaluate);
	printf("\t\ttime check: %.2f%%\n",100*time_check/time_evaluate);
	printf("\t\ttime sort_moves: %.2f%%\n",100*time_sort_moves/time_evaluate);
	printf("\t\ttime play_move: %.2f%%\n",100*time_play_move/time_evaluate);
/*        
double time_tot=0;
//double time_decide=0;
double time_evaluate=0;
double time_compute_attack_squares=0;
double time_heuristic_evaluation=0;
double time_generate_legal_moves=0;
double time_check=0;
double time_sort_moves=0;
double time_play_move=0;
*/
        if (TRANSPOSITION_TABLE)
          free_tt();
    
	return 0;
}
