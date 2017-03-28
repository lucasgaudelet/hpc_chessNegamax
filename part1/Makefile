all: compile

compile:
	make -C ./openMP
	make -C ./mpi_pur
	make -C ./mpi_openMP

clean:
	make clean -C ./openMP
	make clean -C ./mpi_pur
	make clean -C ./mpi_openMP

exec:
	make exec -C ./openMP
	make exec -C ./mpi_pur
	make exec -C ./mpi_openMP
	#cd openMP/ && export OMP_NUM_THREADS=2 &&./decide "4k//4K/4P w"
	#cd mpi_pur/ && mpirun -np 5 -hostfile hostfile --map-by node ./decide "4k//4K/4P w"
	#cd mpi_openMP/ && mpirun -np 5 -hostfile hostfile --map-by node ./decide "4k//4K/4P w"

.PHONY : compile clean

