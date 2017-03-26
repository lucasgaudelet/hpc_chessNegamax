all: exe

exe: 
	make -C ./openMP
	make -C ./mpi_pur
	make -C ./mpi_pur_openMP 

clean:
	make clean -C ./openMP
	make clean -C ./mpi_pur
	make clean -C ./mpi_pur_openMP	

.PHONY : exe clean
