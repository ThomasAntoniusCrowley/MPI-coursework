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



		double startTime = MPI_Wtime(); //start timer


	  /*
	            Task 1:   Scatter big buckets two processes
		*/
		int dataPerProc = n/numprocs;
		float *bigBucket  = (float*) malloc( n*sizeof(float) );
		MPI_Scatter( globalArray, dataPerProc, MPI_FLOAT, bigBucket, dataPerProc, MPI_FLOAT, 0, MPI_COMM_WORLD );
		displayBigBuckets( bigBucket, dataPerProc,rank, numprocs, n);

		/*
		        Task 2: allocate memory for small buckets
		*/

		float **smallBucket = (float**) malloc( sizeof(float)*dataPerProc );		/* n rows in total */
		for( i=0; i<numprocs; i++ )
		{
			smallBucket[i] = (float*) malloc( sizeof(float)*numprocs );
		}
		/* create array of containing the size of the small buckets */

		int smallSizes[n];
		for (i=0; i<n; i++ )
		{
			smallSizes[i]=0;
		}


		/*populate the array */
		int j;
		for( i=0; i<n/numprocs; i++ )
		{
			int smallBucketIndex = (int) (bigBucket[i] * numprocs);
			smallBucket[smallBucketIndex][smallSizes[smallBucketIndex]]=bigBucket[i];
			smallSizes[smallBucketIndex] += 1;
		}

		displaySmallBuckets( smallBucket, smallSizes, rank, numprocs, n );

		/*Task 3
		 send the small buckets to the correct big buckets */

		/*send the items*/
		for (i=0; i<numprocs; i++)
		{
			if (rank == i)
			{
				for (j=0; j<smallSizes[i]; j++ )
				{
					bigBucket[j] = smallBucket[i][j];
				}
			}
			else
			{
				MPI_Send(smallBucket[i], smallSizes[i], MPI_FLOAT, i, 0, MPI_COMM_WORLD);
			}
		}

		int currentSize = smallSizes[rank];
		int count;
		MPI_Status status;
		/* receives the array */
		for (i=0; i<numprocs; i++)
		{

			//MPI_Status status;

			if (!(rank == i))
			{
				MPI_Probe(i, 0, MPI_COMM_WORLD, &status);
				MPI_Get_count(&status, MPI_FLOAT, &count);
				MPI_Recv(&bigBucket[currentSize], count, MPI_FLOAT, i, 0, MPI_COMM_WORLD, &status);
				currentSize += count;

			}
		}


		/*check it works*/
		displayBigBuckets( bigBucket, currentSize,rank, numprocs, n);

		/* Task 4
		sort each big bucket using serial sort */
		serialQuicksort(bigBucket,0,(currentSize));

		displayBigBuckets( bigBucket, currentSize,rank, numprocs, n);

		/*Task 5
		concatenate each rank's big bucket into a single sorter list with rank 0 */

			int currentBigSize = 0;

			if (rank == 0)
			{
				for (j=0; j<currentSize; j++ )
				{
					globalArray[j] = bigBucket[j];
					currentBigSize += 1;
				}
			}
			else
			{
				MPI_Send(bigBucket, currentSize, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
			}


			/* receives the array */
			count = 0;
			if (rank == 0)
			{
				for (i=0; i<numprocs; i++)
				{
					MPI_Status status;
					MPI_Probe(i, 0, MPI_COMM_WORLD, &status);
					MPI_Get_count(&status, MPI_FLOAT, &count);
					MPI_Recv(&globalArray[currentBigSize], count, MPI_FLOAT, i, 0, MPI_COMM_WORLD, &status);
					currentBigSize += count;
				}
			}


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


	double timeTaken = MPI_Wtime() - startTime;
	if( rank==0 )
	  printf( "Finished. Time taken: %g seconds\n", timeTaken );


	/*
		Clear up and quit. As ever, each malloc() needs a free().
	*/
	if( rank==0 ) free( globalArray );
	MPI_Finalize();
	return EXIT_SUCCESS;
}
