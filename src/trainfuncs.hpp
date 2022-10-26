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

#ifndef SOM_TRAINFUNCS_HPP_GUARD
#define SOM_TRAINFUNCS_HPP_GUARD


#include "trainstate.hpp"


namespace som 
{

static inline 
double 
square(double a) noexcept 
{ return a*a; }

static inline 
double
squared_distance(const double* a, const double* b, unsigned size) noexcept 
{
    double distance{ 0 };
    for (unsigned idx{ 0 }; idx < size; ++idx)
        distance += square( a[idx] - b[idx] ); 
    return distance;
}

static inline 
double 
squared_hypot(double a, double b) noexcept 
{ return square(a) + square(b); }



/** @brief Initializes the weights selecting randomly records from the dataset */ 
static inline 
void 
weights_random_init(TrainState& state) noexcept 
{
    Context context{ __func__ }; 

    if ( state.constants.rseed > 0 )
        std::srand( state.constants.rseed ); 
    else
        std::srand( std::time(nullptr) ); 

    const Dataset& dataset{ state.constants.dataset }; 
    Weights<som::Memory<double>>& weights{ state.weights }; 
    double* entry; 
    const double* record; 
    unsigned gidx, rank, lidx; // global and local indexing 

    auto sendbuff = som::parallel::Memory<double>{ dataset.dimensions() }; // A buffer for sending data to other ranks

    for (unsigned row{ 0 }; row < weights.rows(); ++row)  
    {
        for (unsigned col{ 0 }; col < weights.cols(); ++col)  
        {
            entry = weights.entry( weights.index(row, col) ); 

            gidx = std::rand() % dataset.trecords();
            dataset.global_to_local( gidx, &rank, &lidx );
            context.println( gidx, rank, lidx ); 

            if ( state.constants.rank == rank ) 
            {
                record = dataset.rrecord( lidx ); 
                std::copy(
                    record, 
                    record + dataset.dimensions(), 
                    sendbuff.get()
                ); 
            }
            sendbuff.broadcast( rank );  
            std::copy(
                sendbuff.cbegin(), 
                sendbuff.cend(), 
                entry
            );
        }
    }
}


/** @brief commits the BatchFraction to the TrainState modifying the Weights. 
 *  Returns the difference with respect to the previous batch
 */
static inline 
double 
batch_fraction_commit(const BatchFraction& fraction, Weights<som::Memory<double>>& weights)  
{
    const double* numerator{ fraction.numerator.entry(0) }; 
    const double denominator{ *fraction.denominator.get() }; 
    double* state{ weights.entry(0) };
    double diff{ 0 };
    double update;
    const unsigned size{ fraction.numerator.size3() };

    for (unsigned idx{0}; idx < size; ++idx)
    {
        update = *numerator /denominator;
        *state += update;

        diff += std::abs( update );

        numerator++;
        state++;
    }
    return diff; 
}


static inline 
void 
bmu_find(const Weights<som::Memory<double>>& weights, const double* record, unsigned* brow, unsigned* bcol) noexcept 
{
    unsigned row{ 0 };
    unsigned col{ 0 };
    double mindist{ 
        squared_distance( 
            weights.entry( weights.index(row, col) ), record, weights.dimensions()
        )
    };

    unsigned r, c;
    double distance;
    for (r = 0; r < weights.rows(); ++r)
        for (c = 0; c < weights.cols(); ++c)
        {
            distance = squared_distance(
                weights.entry( weights.index(row, col) ), record, weights.dimensions()
            );
            if (distance < mindist)
            {
                mindist = distance;
                row = r;
                col = c;
            }
        }
    *brow = row;
    *bcol = col;
}


/** modify the BatchFraction to account for the new presented record */
static inline 
void
batch_fraction_accumulate(TrainState& state, const double* record, unsigned brow, unsigned bcol) 
{
    const unsigned share{ static_cast<unsigned>(state.nradius) };

    const unsigned r1{ (share > brow)? 0 : brow -share };
    const unsigned c1{ (share > bcol)? 0 : bcol -share };
    const unsigned r2{ std::min(brow + share + 1, state.weights.rows()) };
    const unsigned c2{ std::min(bcol + share + 1, state.weights.cols()) };
    const unsigned dimensions{ state.weights.dimensions() }; 

    const double* entry;
    double* numerator;
    double* denominator{ state.bfraction.denominator.get() }; 
    assert( denominator ); 
    unsigned row, col, d;
    unsigned index; 
    double sqdist, distfunc;
    const double nradius{ state.nradius };

    for (row = r1; row < r2; ++row)
        for (col = c1; col < c2; ++col)
        {
            index = state.weights.index( row, col ); 

            entry = state.weights.entry( index );  
            numerator = state.bfraction.numerator.entry( index );   

            sqdist = squared_hypot( row -brow, col -bcol );
            distfunc = std::exp( -sqdist / (2.0 *nradius) );

            *denominator += distfunc;
            for (d = 0; d < dimensions; ++d)
                numerator[ d ] += distfunc *( record[ d ] - entry[ d ] );
        }
}


/** @brief presents at most rbatchsize records to the Lattice, 
 *  modifying first the BatchFraction and then committing BatchFraction to Weights 
 */
static inline 
void 
batch_present(TrainState& state) noexcept 
{
    state.bfraction.init();
    
    const Dataset& dataset{ state.constants.dataset }; 
    const unsigned first{ (state.batch - 1) *state.constants.rbatchsize }; 
    const unsigned end{ std::min(dataset.rrecords(), first + state.constants.rbatchsize) }; 
    
    Context context{
        __func__, "batch", state.batch, "of", state.constants.batches, 
        "records", first, "to", end
    };    

    const double* record;
    unsigned brow, bcol; 
    for (unsigned lidx{ first }; lidx < end; ++lidx)
    {
        record = dataset.rrecord( lidx );    
        context.println( "record", record[0], record[1] );

        bmu_find( state.weights, record, &brow, &bcol ); 

        batch_fraction_accumulate( state, record, brow, bcol ); 
    }

    state.bfraction.reduce(); 

    state.diff = batch_fraction_commit( state.bfraction, state.weights )
        /( state.constants.valmean *static_cast<double>(state.weights.size3()) ); 
}
} // namespace som
#endif // SOM_TRAINFUNCS_HPP_GUARD
