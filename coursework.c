/*
	This is the starting point for Coursework 3 of COMP2444. Please see the
	coursework specification on the VLE for instructions of how to compile
	and execute this code.
*/


/* Includes */
#include <stdio.h>			/* Standard input-output (i.e. printf) */
#include <stdlib.h>			/* Standard lib, needed for e.g. malloc(), atoi() etc. */
#include <mpi.h>			/* Needed for all MPI routines */

#include "routines.c"		/* Contains various routines to help with the coursework. */



/*
	Main
*/
int main( int argc, char **argv )
{
	int numprocs, rank, i, n;		/* n is the problem size, and i is used as a loop variable below. */

	/* Initialise the MPI library and get the number of processes and rank, as usual. */
	MPI_Init( &argc, &argv );
	MPI_Comm_size( MPI_COMM_WORLD, &numprocs );
	MPI_Comm_rank( MPI_COMM_WORLD, &rank     );

	/* Get the list size n from the command line. The routine getProblemSize() is in the file "routines.c",
	   and returns the value -1 (after outputting an error message) if something went wrong. */
	n = getProblemSize(argc,argv,rank,numprocs);
	if( n==-1 )
	{
		MPI_Finalize();
		return EXIT_FAILURE;
	}

	/* Create an array of size n and fill it with random numbers (rank 0 only). The memory allocation and
	   setting the random values is performed by the routine initialiseRandomList() in routines.c. */
	float *globalArray = NULL;
	if( rank==0 ) globalArray = initialiseRandomList(n);

	/* Display the initial, unsorted list. Note that nothing is displayed for n>100. */
	displayFullList(globalArray,rank,numprocs,n);			/* This and other display routines are in routines.c */




	  /*
	            Task 1:   Scatter big buckets two processes
		*/
		int dataPerProc = n/numprocs;
		float *bigBucket  = (float*) malloc( 2*dataPerProc*sizeof(float) );
		MPI_Scatter( globalArray, dataPerProc, MPI_FLOAT, bigBucket, dataPerProc, MPI_FLOAT, 0, MPI_COMM_WORLD );
		displayBigBuckets( bigBucket, dataPerProc,rank, numprocs, 12);

		/*
		        Task 2: allocate memory for small buckets
		*/
		//float **smallBucket;

		int *size = (int*) malloc(sizeof(dataPerProc));
		float **smallBucket = (float**) malloc( sizeof(float*)*n );		/* n rows in total */


		for( i=0; i<n; i++ )
			smallBucket[i] = (float*) malloc( sizeof(float)*n );	/* Allocate memory for each row */

		/* Put some initial values in the array */
		int j;
		for( i=0; i<n; i++ )
			for( j=0; j<n; j++ )
				smallBucket[i][j] = (int) (bigBucket[i] * numprocs);


		displaySmallBuckets( smallBucket, size, rank, numprocs, n );




	/*
		Display the final (hopefully sorted) list, and check all entries are indeed in order.
	*/
	displayFullList(globalArray,rank,numprocs,n);			/* Again, nothing is displayed if n>100. */
	if( rank==0 )
	{
		for( i=0; i<n-1; i++ )
			if( globalArray[i] > globalArray[i+1] )
			{
				printf( "List not sorted correctly.\n" );
				break;
			}
		if( i==n-1 ) printf( "List correctly sorted.\n" );
	}





	/*
		Clear up and quit. As ever, each malloc() needs a free().
	*/
	if( rank==0 ) free( globalArray );
	MPI_Finalize();
	return EXIT_SUCCESS;



}
