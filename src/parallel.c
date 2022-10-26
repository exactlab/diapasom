// DIAPASOM - DIstributed And PArallel Self Organizing Maps 
// 
// Copyright (c) 2022, eXact lab (<https://www.exact-lab.it>) All Rights Reserved.
// You may use, distribute and modify this code under the
// terms of the BSD 4-clause license. 
//
// This program is distributed in the hope that it will be useful, but WITHOUT 
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       
// FITNESS FOR A PARTICULAR PURPOSE. 
//
// You should have received a copy of the BSD 4-clause license along 
// with DIAPASOM; see the file LICENSE. If not see 
// <https://en.wikipedia.org/wiki/BSD_licenses>.
//
//
// Authors: 
//     Matteo Barnaba <matteo.barnaba@exact-lab.it>
//     Matteo Poggi <matteo.poggi@exact-lab.it>
//     Giuseppe Piero Brandino <giuseppe.brandino@exact-lab.it>
//

#include "parallel.h"


#include <stdlib.h>
#include <stdio.h>


static unsigned myrank = 0;
static unsigned nranks = 0; 


#ifdef USE_PARALLEL_MPI 
#include <mpi.h>

#define PARALLEL_BACKEND "ompi" 

#define MPICOMM MPI_COMM_WORLD

static inline 
void 
initialize(unsigned* rank, unsigned* ranks)
{
    MPI_Init( NULL, NULL ); 
    
    MPI_Comm_rank( MPICOMM, (int*) rank );
    MPI_Comm_size( MPICOMM, (int*) ranks );
}

static inline 
void 
finalize()
{
    MPI_Finalize(); 
}


static inline
void* 
pmalloc(unsigned bytes) 
{
    return malloc( bytes );
}
static inline 
void 
pfree(void* ptr) 
{
    free( ptr );  
}


static inline 
void 
broadcast(void* ptr, unsigned bytes, unsigned root) 
{
    MPI_Bcast(
        ptr, 
        bytes, 
        MPI_CHAR, 
        root,
        MPICOMM
    ); 
}


static inline 
void 
sum_all_double(const double* local, double* global, unsigned count)
{
    MPI_Allreduce(
        local, 
        global, 
        count, 
        MPI_DOUBLE, 
        MPI_SUM,
        MPICOMM
    ); 
}


static inline 
void 
max_all_double(const double* local, double* global, unsigned count)
{
    MPI_Allreduce(
        local, 
        global, 
        count, 
        MPI_DOUBLE, 
        MPI_MAX,
        MPICOMM
    ); 
}

#elif USE_PARALLEL_OSHMEM 
#include <shmem.h>

#define PARALLEL_BACKEND "oshmem"


static inline 
void 
initialize(unsigned* rank, unsigned* ranks) 
{
    shmem_init();
    *rank = shmem_my_pe();
    *ranks = shmem_n_pes();  
}

static inline 
void 
finalize()
{
    shmem_finalize(); 
}


static inline
void* 
pmalloc(unsigned bytes) 
{
    return shmem_malloc( bytes );
}
static inline 
void 
pfree(void* ptr) 
{
    shmem_free( ptr );  
}


static inline 
void 
broadcast(void* ptr, unsigned bytes, unsigned root) 
{
    static long pSync[ _SHMEM_BCAST_SYNC_SIZE ];
    for (unsigned i = 0; i < _SHMEM_BCAST_SYNC_SIZE; i++) 
        pSync[ i ] = _SHMEM_SYNC_VALUE;
    shmem_barrier_all(); /** wait for all ranks to initialize pSync */

    shmem_broadcast64(
        ptr, 
        ptr, 
        bytes, 
        root, 
        0, 0, 
        nranks, 
        pSync
    ); 
}


static inline 
void 
sum_all_double(const double* local, double* global, unsigned count)
{
    static long pSync[ _SHMEM_REDUCE_SYNC_SIZE ];
    for (unsigned i = 0; i < _SHMEM_BCAST_SYNC_SIZE; i++) 
        pSync[ i ] = _SHMEM_SYNC_VALUE;
    shmem_barrier_all(); /** wait for all ranks to initialize pSync */

    shmem_double_sum_to_all(
        global,
        local, 
        count, 
        0, 0, 
        nranks, 
        global, 
        pSync
    ); 
}


static inline 
void 
max_all_double(const double* local, double* global, unsigned count) 
{
    static long pSync[ _SHMEM_REDUCE_SYNC_SIZE ];
    for (unsigned i = 0; i < _SHMEM_BCAST_SYNC_SIZE; i++) 
        pSync[ i ] = _SHMEM_SYNC_VALUE;
    shmem_barrier_all(); /** wait for all ranks to initialize pSync */

    shmem_double_max_to_all(
        global,
        local, 
        count, 
        0, 0, 
        nranks, 
        global, 
        pSync
    ); 
}


#else // serial 
#include <string.h>

#define PARALLEL_BACKEND "serial"

static inline 
void 
initialize(unsigned* rank, unsigned* ranks) 
{
    *rank = 0;
    *ranks = 1; 
}

static inline 
void 
finalize() {}


static inline 
void* 
pmalloc(unsigned bytes) 
{
    return malloc( bytes );
}

static inline 
void 
pfree(void* ptr) 
{
    free( ptr ); 
}


static inline 
void 
broadcast(void* ptr, unsigned bytes, unsigned root) 
{
    /** completely useless operation, just to use the parameters */
    ptr = (unsigned*) ptr + bytes + root; 
}


static inline 
void 
sum_all_double(const double* local, double* global, unsigned count)
{
    memcpy( 
        (void*) global, 
        (const void*) local, 
        count *sizeof(*local)
    );  
}


static inline 
void 
max_all_double(const double* local, double* global, unsigned count)
{
    sum_all_double( local, global, count ); 
}
#endif


void
parallel_initialize(unsigned* rank, unsigned* ranks) 
{
    initialize( rank, ranks );

    myrank = *rank; 
    nranks = *ranks; 
}

void 
parallel_finalize()
{
    finalize(); 
}


const char*
parallel_backend() 
{
    return PARALLEL_BACKEND; 
}


void*
parallel_malloc(unsigned bytes)
{
    return pmalloc( bytes ); 
}

void 
parallel_free(void* ptr)
{
    pfree( ptr ); 
}


void 
parallel_broadcast(void* ptr, unsigned bytes, unsigned root)  
{
    broadcast( ptr, bytes, root );  
}


void 
parallel_sum_all_double(const double* local, double* global, unsigned count) 
{
    sum_all_double( local, global, count ); 
}


void 
parallel_max_all_double(const double* local, double* global, unsigned count) 
{
    max_all_double( local, global, count ); 
}
