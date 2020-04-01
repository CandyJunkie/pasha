#include "mpi.h"
#include <stdio.h>
#include <iostream>

double pow4 (double val)
{
	double val2 = val * val;
	return val2 * val2;
}

double integrate_fun(double from, double to, double step, double (*fun)(double val))
{
	double lval = from;
	double sum = 0;
	while (lval < to)
	{
		sum += fun (lval) * step;
		lval += step;
	}
	return sum;
}

int main(int argc, char* argv[])
{
	int nProcs, procRank;
	double step = 0.001;
	double total_integr;
	double result;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD,&nProcs);
	MPI_Comm_rank(MPI_COMM_WORLD,&procRank);

	double len = 1 / nProcs;
	double from = len * procRank;
	double to = len * (procRank + 1);
	if (procRank == nProcs - 1)
	{
		to = 1.0;
	}

	if (step < len)
	{
		step = len;
	}

	// int(x^4)
	double integr = integrate_fun(from, to, step, pow4);
	MPI_Reduce(&integr, &total_integr, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);
	
	if (procRank == 0)
	{
		// int(x^4 * y^4 * z^4) = int(x^4) * int(y^4) * int(z^4) = int(x^4)^3.
		result = total_integr * total_integr * total_integr;
		printf ("result = %lf\n", result);
	}

	MPI_Finalize();
	system("pause");
	return 0;
}
