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

#ifndef SOM_LATTICE_HPP_GUARD 
#define SOM_LATTICE_HPP_GUARD 


#include "common.hpp"
#include "parallel.hpp"


#define self (*this)


namespace som 
{

class Lattice; 
/** @brief An alias for a callable that will be called by the training process at each epoch */
using TrainCallable = void(*)(const Lattice*);


/** A simple class representing the settings that can be passed to 
 *  the training process of the Lattice 
 */
class TrainSettings 
{
    std::string mdataset{ "" }; 
    unsigned mlatticedim{ 10 };
    unsigned mrows{ 10 }; 
    unsigned mcols{ 10 }; 

    unsigned mepochs{ 0 }; 
    double mdiffmin{ 0 }; 
    unsigned mbatchsize{ 0 }; 
    unsigned mbatches{ 0 };
    double mnradius{ 0 }; 
    double mrdecay{ 1e-1 }; 
    unsigned mrseed{ 0 }; 
    TrainCallable mepcall{}; 

    public: 
    /** @brief Constructs a TrainSettings object with the given dataset and 
     *  default values for the other parameters 
     */
    TrainSettings(const std::string& dataset) 
    { self.dataset( dataset ); }

    /** @brief Constructs a TrainSettings object retrieving value 
     *  directly from a parameters::Handler object. 
     *  Very handy for expample to pass directly CLI arguments, 
     *  like in main().
     */
    TrainSettings(const parameters::Handler& params); 
    
    getfsetter(
        TrainSettings, dataset, std::string, 
        (space-separated file containing the dataset)
    ) 
    getfsetter(
        TrainSettings, latticedim, unsigned, 
        (dimension of a square Lattice)
    ) 
    getfsetter(
        TrainSettings, rows, unsigned, 
        (rows of a rectangular Lattice)
    ) 
    getfsetter(
        TrainSettings, cols, unsigned, 
        (columns of a rectangular Lattice)
    ) 
    getfsetter(
        TrainSettings, epochs, unsigned, 
        (maximum number of epochs)
    ) 
    getfsetter(
        TrainSettings, diffmin, double, 
        (minimum difference (tolerance) with respect to previous epoch)
    ) 
    getfsetter(
        TrainSettings, batchsize, unsigned, 
        (records to present before updating the state of the Lattice)
    )
    getfsetter(
        TrainSettings, batches, unsigned, 
        (number of batches for each epoch)
    )
    getfsetter(
        TrainSettings, nradius, double, 
        (initial neighboring radius of the SOM training)
    )
    getfsetter(
        TrainSettings, rdecay, double,
        (decay rate of nradius(), applied at each epoch)
    )
    getfsetter(
        TrainSettings, rseed, unsigned, 
        (seed for the random number generator, if rseed() > 0 the results are replicable)
    )
    getfsetter(
        TrainSettings, epcall, TrainCallable, 
        (callable than will be called at each epoch)
    )
    /** @brief Sets the new value of epcall (callable than will be called at each epoch)
     *  to the "epcall(const Lattice*)" inside the supplied dynamic library 
     */
    TrainSettings& epcall(const std::string& dlname) noexcept;
}; 


struct TrainState; 

/** @brief A simple class representing that can be trained with a Dataset */
class Lattice 
{
    unsigned mrows{ 0 }; 
    unsigned mcols{ 0 }; 
    TrainState* state{ nullptr }; 
    
    public: 
    /** @brief Constructs a (cols x rows) Lattice */
    Lattice(unsigned rows, unsigned cols)
        : mrows{ rows }, 
          mcols{ cols }
    {}

    /** @brief Constructs a square (dim x dim) Lattice */
    Lattice(unsigned dim) 
        : Lattice{ dim, dim }
    {}


    Lattice(const Lattice&) = delete; 
    Lattice& operator = (const Lattice&) = delete; 
    Lattice(Lattice&&) noexcept = default; 
    Lattice& operator = (Lattice&&) noexcept = default; 

    
    getter(rows, unsigned, (number of rows of the Lattice))
    getter(cols, unsigned, (number of columns of the Lattice))


    unsigned rank() const noexcept { return som::parallel::rank(); }
    unsigned ranks() const noexcept { return som::parallel::ranks(); }


    /** @brief Prints the state of the Lattice to fname (if provided) or to stdout */
    void print(const std::string& fname="") const;


    /** @brief Trains the Lattice with the given Dataset according to the supplied TrainSettings */
    Lattice& 
    train(const TrainSettings& settings); 
    
    /** @brief Returns the current epoch of the training process */ 
    unsigned epoch() const noexcept; 
}; 
} // namespace som 
#undef self
#endif // SOM_LATTICE_HPP_GUARD
