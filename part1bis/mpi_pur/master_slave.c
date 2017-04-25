#include "master_slave.h"
#include <stdio.h>
#include <stdlib.h>

void create_mpi_result_t(MPI_Datatype* MPI_RESULT_T)
{
	/* creates MPI function handle for result_t structure */
	
	int count = 4;							// number of blocks

	int blocklen[] = { 1, 1, 1, 128 };		// block length
	
	MPI_Datatype type[] = 					// block type
		{MPI_INT,MPI_INT,MPI_INT,MPI_INT};
	
	MPI_Aint displacement[4];	// block displacement
		/*result_t result;
		displacement[0] = &result.score - &result;
		displacement[1] = &result.best_move - &result;
		displacement[2] = &result.pv_length - &result;
		displacement[3] = &result.PV - &result;*/
		displacement[0] = offsetof(result_t, score);
		displacement[1] = offsetof(result_t, best_move);
		displacement[2] = offsetof(result_t, pv_length);
		displacement[3] = offsetof(result_t, PV);
	
	/* MPI calls */
	MPI_Type_create_struct(count, blocklen, displacement, type, MPI_RESULT_T);
	MPI_Type_commit(MPI_RESULT_T);
}

void create_mpi_tree_t(MPI_Datatype* MPI_RESULT_T)
{
	/* creates MPI function handle for result_t structure */
	
	int count = 14;							// number of blocks

	// block length
	int blocklen[] = { 128, 128, 1, 
		1, 1, 1, 1, 1,
		2, 1, 128,
		1, 1, 128};

	// block type
	MPI_Datatype type[] = { MPI_CHAR, MPI_CHAR, MPI_INT,
		MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT,
		MPI_INT, MPI_INT, MPI_CHAR,
		MPI_INT, MPI_UNSIGNED_LONG_LONG, MPI_UNSIGNED_LONG_LONG}; 
	
	MPI_Aint displacement[14];	// block displacement
		displacement[0] = offsetof(tree_t, pieces);
		displacement[1] = offsetof(tree_t, colors);
		displacement[2] = offsetof(tree_t, side);
		displacement[3] = offsetof(tree_t, depth);
		displacement[4] = offsetof(tree_t, height);
		displacement[5] = offsetof(tree_t, alpha);
		displacement[6] = offsetof(tree_t, beta);
		displacement[7] = offsetof(tree_t, alpha_start);
		displacement[8] = offsetof(tree_t, king);
		displacement[9] = offsetof(tree_t, pawns);
		displacement[10] = offsetof(tree_t, attack);
		displacement[11] = offsetof(tree_t, suggested_move);
		displacement[12] = offsetof(tree_t, hash);
		displacement[13] = offsetof(tree_t, history);
		
	/* MPI calls */
	MPI_Type_create_struct(count, blocklen, displacement, type, MPI_RESULT_T);
	MPI_Type_commit(MPI_RESULT_T);
}

void broadcast_end(int np) {
	int buf = BAD_MOVE;
	for( int i=1; i<np; i++) {
		MPI_Send( &buf, 1, MPI_INT, i, TAG_END, MPI_COMM_WORLD); 
	}
}

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

void master_evaluate(tree_t * T, result_t *result)
{
	/*  Parametres MPI */
	MPI_Status		status;
	MPI_Request		req;
	int np, flag;
	int waiting_result=0;
	
	//create_mpi_result_t(&MPI_RESULT_T);	// creation du function handle
	MPI_Comm_size(MPI_COMM_WORLD, &np);	// recuperation du nombre de processus
	
	/* tableau des coups possibles */
	move_t moves[MAX_MOVES];	// coups legaux
	int current_work;			// index i du prochain moves[i] a evaluer
	int n_moves;				// nombre de coups legaux
	
	/* tableau des resultats */
	result_t* result_table;			// tableau de resultats pour chaque move
	int result_nb;		// number of results received in result_table
	int current_result;	// the index of the next result to analyse 
						// in result_table
						
	int* result_index;	// result_index[i] est l'index j du coup moves[j]
						// correspondant au resultat result_table[i]
						
	/* tableau des taches donnees */
	int* slave_work;	// slave_work[i] est l'index j du coup moves[j] sur
						// lequel l'esclave de rank i travaille 
	
	/* Initialise les valeurs de result et incremente node_searched */
	node_searched++;
	result->score = -MAX_SCORE - 1;
	result->pv_length = 0;
	
	if (test_draw_or_victory(T, result)) {
		broadcast_end(np);
		return;
	}
	
	n_moves = generate_legal_moves(T, &moves[0]);
	printf("[ROOT] n_moves: %d\n", n_moves);
	
	/* absence de coups légaux : pat ou mat */
	if (n_moves == 0) {
		result->score = check(T) ? -MAX_SCORE : CERTAIN_DRAW;
		broadcast_end(np);
		return;
	}
	
	/* allocations et initialisations des tableaux et index */
	result_table = (result_t*)malloc(n_moves*sizeof(result_t));
	result_index = (int*)malloc( n_moves*sizeof(int) );
	slave_work = (int*)malloc( np*sizeof(int) );
	current_work = current_result = result_nb = 0;
	
	if (ALPHA_BETA_PRUNING)
		  sort_moves(T, n_moves, moves);

	for(int i=1; i<np; i++) {
		
		if( current_work < n_moves ) {	// Si il reste une tache a accomplir
			tree_t child;
			play_move(T, moves[current_work], &child);
			MPI_Send( &child, 1, MPI_INT, i, TAG_TASK, MPI_COMM_WORLD);
			slave_work[i] = current_work++;
			printf("[ROOT] envoi du move %d à [%d]\n", slave_work[i], i);
		}
		else {
			int buf = BAD_MOVE;
			MPI_Send( &buf, 1, MPI_INT, i, TAG_END, MPI_COMM_WORLD);
			printf("[ROOT] envoi de fin à [%d]\n", i);
		}
	}
	
	// tant qu'il y a des taches à accomplir OU que tout n'as pas été analysé 
	// OU que tout n'as pas été reçu
	while( (current_work < n_moves)	|| (current_result<result_nb)
									|| (result_nb<n_moves) ) {
		
		// Si on attends encore des résultats, reception
		if( !waiting_result && (result_nb<n_moves) ) {
			MPI_Irecv( result_table+result_nb, 1, MPI_RESULT_T, MPI_ANY_SOURCE, TAG_RESULT, MPI_COMM_WORLD, &req);
			printf("[ROOT] attente de reception\n");
			waiting_result=1;
			flag = 0;
		}
		
		MPI_Test(&req, &flag, &status);
		
		// Si resultat reçu   
		if( flag && (result_nb<n_moves) ) {
			printf("[ROOT] result %d received from %d\n", slave_work[status.MPI_SOURCE], status.MPI_SOURCE);

			result_index[result_nb++] = slave_work[status.MPI_SOURCE];
			
			// S'il reste des taches à accomplir
			if( current_work < n_moves) {
				tree_t child;
				play_move(T, moves[current_work], &child);
				MPI_Send( moves+current_work, 1, MPI_INT, status.MPI_SOURCE, TAG_TASK, MPI_COMM_WORLD);
				slave_work[status.MPI_SOURCE] = current_work++;
				printf("[ROOT] envoi du move %d à [%d]\n", slave_work[status.MPI_SOURCE], status.MPI_SOURCE);
			}
			else {
				int buf = BAD_MOVE;
				MPI_Send( &buf, 1, MPI_INT, status.MPI_SOURCE, TAG_END, MPI_COMM_WORLD);
				printf("[ROOT] envoi de fin à [%d]\n", status.MPI_SOURCE);
			}
			MPI_Request_free(&req);
			flag = 0;
			waiting_result=0;
		}
		
		// Si des resultats peuvent etre analyses
		else if( current_result<result_nb ) {
			printf("[ROOT] processing result\n");
			int child_score = - result_table[current_result].score;
		
			if( child_score > result->score ) {
				result->score = child_score;
				result->best_move = moves[ result_index[current_result] ];
				
				result->pv_length = result_table[current_result].pv_length + 1;
				
				for(int j = 0; j < result_table[current_result].pv_length; j++)
					result->PV[j+1] = result_table[current_result].PV[j];
					
				result->PV[0] = moves[ result_index[current_result] ];
			}
			current_result++;
		}
	}
	free(result_index);
	free(slave_work);
	free(result_table);
}


void slave_evaluate(tree_t * T, result_t *result)
{

	/*  Parametres MPI */
	//MPI_Datatype	MPI_RESULT_T;	// result_t function handle
	MPI_Status		status;
	MPI_Request		req;
	int 			flag;
	int 			rank;
	
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	//create_mpi_result_t(&MPI_RESULT_T);	// creation du function handle
	
	/* log */
	/*char filename[5];
	sprintf(filename, "log%d", rank);
	FILE * f = fopen( filename, "a");*/
	
	/* Parametres de calcul */
	move_t move;			// le coup a analyser
	
	compute_attack_squares(T);
		
	// reception de la premiere tache
	MPI_Recv( &move, 1, MPI_INT, ROOT, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	//fprintf(f, "\t[%d] reception move\n",rank);
	
	// tant qu'il y a des taches à accomplir
	while( status.MPI_TAG != TAG_END ) {
		
		result_t child_result;	// le resultat a renvoyer
			result->score = -MAX_SCORE - 1;
			result->pv_length = 0;
		tree_t child;			// noeud fils
		
		play_move(T, move, &child);			// joue le move
		evaluate(&child, &child_result);	// evalue recursivement la position
		
		// envoi du resultat
		MPI_Send( &child_result, 1, MPI_RESULT_T, ROOT, TAG_RESULT, MPI_COMM_WORLD);
		//fprintf(f, "\t[%d] envoi du résultat\n",rank);
		
		// reception du move suivant à analyser
		MPI_Recv( &move, 1, MPI_INT, ROOT, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		//fprintf(f, "\t[%d] reception move\n",rank);
	}
	//fprintf(f, "\t[%d] exiting slave_evaluate\n", rank);
	//fclose(f);
	//MPI_Type_Free(&MPI_RESULT_T);

	return ;
}
