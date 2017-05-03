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
		2, 2, 128,
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


void generate_control_tree(node_t* root, int level){

	/* initialise les variables du noeud */
	if(level==0) {
		root->father = NULL;
	}
	root->current_work = 0;
	root->result_nb = 0;
	root->n_moves = 0;
	
	root->result.pv_length = 0;
	root->result.score = -MAX_SCORE - 1;
	
	/* si retour rapide */
	if (test_draw_or_victory(&root->tree, &root->result))
		  return;
	
	/* génération des fils */
	if(level != LEVEL_MAX) {
		compute_attack_squares(&root->tree);
		root->n_moves = generate_legal_moves(&root->tree, root->moves);
		if (root->n_moves == 0) {
			root->result.score = check(&root->tree) ? -MAX_SCORE : CERTAIN_DRAW;
			root->sons = NULL;
			return;
		}
		
		root->sons = (node_t*)malloc(root->n_moves*sizeof(node_t));
		
		/* appel récursif sur les fils */
		for(int i=0; i<root->n_moves; i++) {
			root->sons[i].father = root;
			play_move(&root->tree, root->moves[i], &root->sons[i].tree);
			generate_control_tree( root->sons+i, level+1);
		}
	}
	else {
		root->sons = NULL;
	}
}

void free_control_tree(node_t* root) {
	if(root->sons) {
		for(int i=0; i<root->n_moves; i++) {
			free_control_tree(root->sons+i);
		}
		free(root->sons);
	}
}

node_t* next_task(node_t* root) {
	
	// si toutes les taches ont été distribuées
	if(root->current_work == root->n_moves) {
		return NULL;
	}
	
	// selection de la feuille à retourner
	while(root->sons) {	
			root = &root->sons[root->current_work];
	}
	
	// incrémentation des parents
	node_t* tmp = root;
	while(tmp->father && tmp->current_work == tmp->n_moves) {
		tmp = tmp->father;
		tmp->current_work++;
	}
	
	return root;
}


/* Le noeud passé en argument doit être une feuille */
void manage_result(result_t* result, node_t* node) {
	
	node_t* father = node->father;
	int i = (int)(node - father->sons);
	
	int child_score = -result->score;
	
	if ( child_score > father->result.score ) {
		father->result.score = child_score;
		father->result.best_move = father->moves[i];
		
			father->result.pv_length = result->pv_length + 1;
			for(int j = 0; j < result->pv_length; j++)
				father->result.PV[j+1] = result->PV[j];
			father->result.PV[0] = father->moves[i];
	}

	father->result_nb++;
	if(father->result_nb==father->n_moves) { // si tous les fils ont été traités
		if(father->father) manage_result(&father->result, father);
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

// a changer
void master_evaluate(node_t* root)
{
	/*  Parametres MPI */
	MPI_Status		status;
	MPI_Request		req;
	int np, flag;
	int waiting_result=0;
	
	MPI_Comm_size(MPI_COMM_WORLD, &np);	// recuperation du nombre de processus
	
	node_t** slave_work = (node_t**)malloc( np*sizeof(node_t*) );
	result_t result;
	
	/* Initialise les valeurs de result et incremente node_searched */
	node_searched++;
	
	// géré dans generate_control_tree
	/*
	if (test_draw_or_victory(root->tree, root->result)) {
		broadcast_end(np);
		return;
	}
	/*
	
	/* Generation de l'arbre de controle */
	generate_control_tree(root, 0);
	
	
	/* absence de coups légaux : pat ou mat */ 
	/*if (root->n_moves == 0) {
		broadcast_end(np);
		return;
	}*/
	
	if (ALPHA_BETA_PRUNING)
		  sort_moves(&root->tree, root->n_moves, root->moves);

	for(int i=1; i<np; i++) {
		node_t* task = next_task(root);
		if( task ) {	// Si il reste une tache a accomplir
			MPI_Send(&task->tree, 1, MPI_TREE_T, i, TAG_TASK, MPI_COMM_WORLD);
			slave_work[i] = task;
			printf("[ROOT] envoi d'une tache à [%d]\n", i);
		}
		else {
			tree_t buf;
			MPI_Send(&buf, 1, MPI_TREE_T, i, TAG_END, MPI_COMM_WORLD);
			printf("[ROOT] envoi de fin à [%d]\n", i);
		}
		
	}
	
	// Premier appel de réception
	//MPI_Recv( &result, 1, MPI_RESULT_T, MPI_ANY_SOURCE, TAG_RESULT, MPI_COMM_WORLD);
	
	// tant qu'il y a des taches à accomplir OU que tout n'as pas été analysé 
	// OU que tout n'as pas été reçu
	while( root->result_nb < root->n_moves ) {
	
		MPI_Recv(&result, 1, MPI_RESULT_T, MPI_ANY_SOURCE, TAG_RESULT, MPI_COMM_WORLD, &status);
		manage_result( &result, slave_work[status.MPI_SOURCE]);
		
		node_t* task = next_task(root);
		if( task ) {	// Si il reste une tache a accomplir
			MPI_Send(&task->tree, 1, MPI_TREE_T, status.MPI_SOURCE, TAG_TASK, MPI_COMM_WORLD);
			slave_work[status.MPI_SOURCE] = task;
			printf("[ROOT] envoi d'une tache à [%d]\n", status.MPI_SOURCE);
		}
		else {
			tree_t buf;
			MPI_Send( &buf, 1, MPI_TREE_T, status.MPI_SOURCE, TAG_END, MPI_COMM_WORLD);
			printf("[ROOT] envoi de fin à [%d]\n", status.MPI_SOURCE);
		}
	}
	free(slave_work);
}

// a changer
void slave_evaluate()
{
	
	/*  Parametres MPI */
	MPI_Status		status;
	int 			rank;
	
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	/* log */
	/*char filename[5];
	sprintf(filename, "log%d", rank);
	FILE * f = fopen( filename, "a");*/
	
	/* Parametres de calcul */
	tree_t T;			// l'arbre à analyser
		
	// reception de la premiere tache
	MPI_Recv( &T, 1, MPI_TREE_T, ROOT, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	//fprintf(f, "\t[%d] reception move\n",rank);
	
	// tant qu'il y a des taches à accomplir
	while( status.MPI_TAG != TAG_END ) {
		result_t child_result;	// le resultat a renvoyer
			child_result.score = -MAX_SCORE - 1;
			child_result.pv_length = 0;
		
		evaluate(&T, &child_result);	// evalue recursivement la position
		
		// envoi du resultat
		MPI_Send(&child_result, 1, MPI_RESULT_T, ROOT, TAG_RESULT, MPI_COMM_WORLD);
		//fprintf(f, "\t[%d] envoi du résultat\n",rank);
		
		// reception du move suivant à analyser
		MPI_Recv( &T, 1, MPI_TREE_T, ROOT, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		//fprintf(f, "\t[%d] reception move\n",rank);
	}
	//fprintf(f, "\t[%d] exiting slave_evaluate\n", rank);
	//fclose(f);
	return ;
}
