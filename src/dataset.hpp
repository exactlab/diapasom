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

#ifndef SOM_DATASET_HPP_GUARD 
#define SOM_DATASET_HPP_GUARD


#include "common.hpp"
#include "parallel.hpp"


#define self (*this)


namespace som 
{

/** @brief Simple class representing a Dataset distributed among an arbitrary number of ranks */
class Dataset 
{
    unsigned mbatchsize{ 0 };
    unsigned mrbatchsize{ 0 }; 
    unsigned mtrecords{ 0 }; 
    unsigned mrrecords{ 0 };
    unsigned mdimensions{ 0 }; 
    double mvalmean{ 0 }; 
    som::Memory<double> mdata{ 0 };

    public: 
    /** @brief Constructs a disengaged Dataset */ 
    Dataset() {}

    /** @brief Constructs a Dataset and populates with the records read from fname */ 
    Dataset(const std::string& fname, unsigned batchsize); 
    
    Dataset(const Dataset&) = delete; 
    Dataset& operator = (const Dataset&) = delete; 
    Dataset(Dataset&&) noexcept = default; 
    Dataset& operator = (Dataset&&) noexcept = default; 

    getter(batchsize, unsigned, (the number of records to be added before updating the state of the Lattice))
    getter(rbatchsize, unsigned, (the batchsize of the current rank))
    getter(trecords, unsigned, (total number of records, accross all ranks))
    getter(rrecords, unsigned, (number of records for the current rank))
    getter(dimensions, unsigned, (dimensions (columns) of the dataset))
    getter(valmean, double, (mean value in the dataset))


    unsigned rank() const noexcept { return som::parallel::rank(); }
    unsigned ranks() const noexcept { return som::parallel::ranks(); }


    /** @brief Returns the local (to the current rank) record at index idx. 
     *  It is you responsability to ensure that idx < rrecords(). 
     */
    const double* 
    rrecord(unsigned idx) const noexcept 
    {
        assert( idx < self.rrecords() ); 
        return self.mdata.get() + idx *self.dimensions(); 
    }

    /** @brief Returns the global (all ranks are considered) record at index idx. 
     *  It returns nullptr if the current rank is not the one hanving the desired record.
     *  It is you responsability to ensure that idx < trecords().
     */
    const double* 
    grecord(unsigned idx) const noexcept 
    {
        unsigned rank, lidx;  
        self.global_to_local( idx, &rank, &lidx ); 

        return (self.rank() == rank)? 
            self.rrecord( lidx ) : nullptr;      
    } 

    /** @brief Converts a global indexing (gidx) to local one (rank and lidx). 
     *  It is you responsability to ensure that gidx < trecords() 
     */
    void 
    global_to_local(unsigned gidx, unsigned* rankp, unsigned* lidxp) const noexcept 
    {
        assert( gidx < self.trecords() ); 

        const unsigned batch{ gidx /self.batchsize() }; 
        const unsigned bidx{ gidx - batch *self.batchsize() }; 
        const unsigned rank{ bidx % self.ranks() }; 
        const unsigned rbatchsize{ som::parallel::distribute( self.batchsize(), rank ) }; 
        const unsigned lidx{ batch *rbatchsize + bidx /self.ranks() }; 

        *rankp = rank; 
        *lidxp = lidx; 
    }

    /** @brief Prints the dataset to fname (if provided) or to stdout */
    void print(const std::string& fname="") const noexcept; 
}; // class Dataset
} // namespace som  
#undef self
#endif // SOM_DATASET_HPP_GUARD
