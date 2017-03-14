void slave_evaluate(tree_t * T, result_t *result)
{

	/*  Parametres MPI */
	MPI_Datatype	MPI_RESULT_T;	// result_t function handle
	MPI_Status		status;
	MPI_Request		req;
	int 			flag;
	int 			rank;
	
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	create_mpi_result_t(&MPI_RESULT_T);	// creation du function handle
	
	/* Parametres de calcul */
	move_t move;			// le coup a analyser
	
	compute_attack_squares(T);
	
	// reception de la premiere tache
	MPI_Recv( &move, 1, MPI_INT, ROOT, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	printf("[%d] reception move\n",rank);
	
	// tant qu'il y a des taches à accomplir
	while( move != BAD_MOVE ) {
		
		result_t child_result;	// le resultat a renvoyer
			result->score = -MAX_SCORE - 1;
			result->pv_length = 0;
		tree_t child;			// noeud fils
		
		play_move(T, move, &child);			// joue le move
		evaluate(&child, &child_result);	// evalue recursivement la position
		
		// envoi du resultat
		MPI_Send( &child_result, 1, MPI_RESULT_T, ROOT, TAG_RESULT, MPI_COMM_WORLD);
		printf("[%d] envoi du résultat\n",rank);
		
		// reception du move suivant à analyser
		MPI_Recv( &move, 1, MPI_INT, ROOT, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		printf("[%d] reception move\n",rank);
	}
	printf("[%d] out\n",rank);
	return ;
}
