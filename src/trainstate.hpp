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

#ifndef SOM_TRAINSTATE_HPP_GUARD
#define SOM_TRAINSTATE_HPP_GUARD


#include "dataset.hpp"
#include "lattice.hpp"
#include "parallel.hpp"


#define self (*this)


namespace som 
{

/** 
 * @brief Simple class representing the state (the weights) of the Lattice. 
 * This class is private to lattice.cpp
 */
template <class Memory=som::Memory<double>>
class Weights 
{
    unsigned mrows{ 0 };
    unsigned mcols{ 0 };
    unsigned mdimensions{ 0 }; 
    Memory mem{ 0 }; 

    public:
    /** @brief Weights in a disengaged state */
    Weights() {} 
    Weights(unsigned rows, unsigned cols, unsigned dimensions) 
        : mrows{ rows }, 
        mcols{ cols },
        mdimensions{ dimensions }
    {
        Context context{ "Weights::Weights" };
        self.mem = Memory{ self.size3() }; 
    }

    getter(rows, unsigned, (number of rows))
    getter(cols, unsigned, (number of columns))
    getter(dimensions, unsigned, (dimensions of the dataset))

    
    Memory& 
    memory() noexcept { return self.mem; }

    const Memory&
    memory() const noexcept { return self.mem; }
    

    unsigned size2() const noexcept 
    { return self.rows() *self.cols(); }

    unsigned size3() const noexcept 
    { return self.size2() *self.dimensions(); }


    unsigned index(unsigned row, unsigned col) const noexcept 
    { 
        assert( row < self.rows() ); 
        assert( col < self.cols() ); 
        return row *self.cols() + col; 
    }
    
    unsigned offset(unsigned row, unsigned col) const noexcept 
    { 
        return self.offset( self.index(row, col) ); 
    }
    unsigned offset(unsigned index) const noexcept 
    { 
        assert( index < self.size2() ); 
        return index *self.dimensions(); 
    }

    double* entry(unsigned index) noexcept 
    { return self.memory().get() + self.offset(index); }
    
    const double* entry(unsigned index) const noexcept 
    { return self.memory().get() + self.offset(index); }

    
    void swap(Weights& rhs) noexcept 
    { self.memory().swap( rhs.memory() ); }
}; // class Weights


/** @brief The numerator of the batch fraction has the same 
 *  dimensions and structure as the Weights 
 */
using BatchNumerator = Weights<som::parallel::Memory<double>>; 

/** @brief Simple class representing the batch fraction */
struct BatchFraction 
{
    BatchNumerator numerator{}; 
    BatchNumerator numbuffer{}; 
    som::parallel::Memory<double> denominator{ 1 }; 
    som::parallel::Memory<double> denbuffer{ 1 };

    /** @brief BatchFraction in a disengaged state */
    BatchFraction() {} 
    BatchFraction(unsigned rows, unsigned cols, unsigned dimensions) 
    {
        Context context{ "BatchFraction::BatchFraction" }; 

        self.numerator = Weights<som::parallel::Memory<double>>{
            rows, cols, dimensions 
        };  
        self.numbuffer = Weights<som::parallel::Memory<double>>{
            rows, cols, dimensions 
        };  
    }

    /** @brief Initializes the BatchFraction to 0 */
    void init() noexcept 
    {
        Context context{ "BatchFraction::init" }; 

        std::memset( 
            self.numerator.entry( 0 ), 
            0, 
            self.numerator.size3() *sizeof(double)
        ); 
        self.denominator.get()[ 0 ] = 0; 
    }

    /** @brief Sums the contribution to the batch fraction from all the 
     *  ranks and sends the results to all ranks 
     */
    void reduce() noexcept 
    {       
        Context context{ "BatchFraction::reduce" }; 

        self.numerator.memory().sum_all( 
            self.numbuffer.memory()
        ); 
        self.numerator.swap( self.numbuffer ); 

        self.denominator.sum_all(
            self.denbuffer
        ); 
        self.denominator.swap( self.denbuffer ); 
    }
}; // class BatchFraction


/** @brief Simple class representing the state of the training process of the Lattice */
struct TrainState 
{
    /** @brief Constant parameters set at the begining of the training process */
    struct Constants
    {
        unsigned rank{ 0 };
        unsigned ranks{ 1 }; 
        Dataset dataset{}; 
        unsigned rrecords{ 0 }; 
        unsigned epochs{ 0 }; 
        double diffmin{ 0 };
        unsigned batchsize{ 0 }; 
        unsigned rbatchsize{ 0 }; 
        unsigned batches{ 0 }; 
        double nradius{ 0 }; 
        double rdecay{ 1e-1 }; 
        unsigned rseed{ 0 }; 
        double valmean{ 1 }; 
        
        /** @brief Initialize the constant parameters given TrainSettings */
        Constants(const Lattice& lattice, const TrainSettings& training); 
    }; // struct Constants

    const Constants constants;
    

    /** the state (weights) of the Lattice */
    Weights<som::Memory<double>> weights{};

    /** the batch fraction */
    BatchFraction bfraction{}; 

    unsigned epoch{ 0 }; 
    unsigned batch{ 0 }; 
    double nradius{ 0 }; 
    double diff{ 1 }; 
    som::parallel::Timer<std::chrono::microseconds> total{}; 

    TrainState(const Lattice& lattice, const TrainSettings& settings); 
}; // struct TrainState 


TrainState::Constants::Constants(const Lattice& lattice, const TrainSettings& settings) 
{
    Context context{ "TrainState::Constants::Constants" }; 

    self.rank = lattice.rank();
    self.ranks = lattice.ranks(); 

    self.dataset = som::Dataset{ 
        settings.dataset(), settings.batchsize()
    }; 

    self.rrecords = dataset.rrecords(); 
    
    self.epochs = (settings.epochs() > 0)? 
        settings.epochs() : dataset.trecords();

    self.diffmin = (settings.diffmin() > 1e-6)?
        settings.diffmin() : 0; 
    
    self.batchsize = dataset.batchsize(); 
    self.rbatchsize = dataset.rbatchsize();
    self.batches = static_cast<unsigned>( 
        std::ceil( static_cast<double>(dataset.trecords()) /static_cast<double>(self.batchsize) ) 
    );

    self.nradius = (settings.nradius() > 1e-6)?
        settings.nradius() : 0.5 *std::min( lattice.rows(), lattice.cols() ); 
    self.rdecay = settings.rdecay(); 

    self.rseed = settings.rseed(); 

    self.valmean = dataset.valmean();
}
        
TrainState::TrainState(const Lattice& lattice, const TrainSettings& settings)
    : 
    constants{ lattice, settings }
{   
    Context context{ "TrainState::TrainState" }; 

    self.weights = Weights<som::Memory<double>>{ 
        lattice.rows(), lattice.cols(), constants.dataset.dimensions() 
    };     
    self.bfraction = BatchFraction{ 
        lattice.rows(), lattice.cols(), constants.dataset.dimensions()
    }; 

    self.epoch = 0; 
    self.batch = 0; 
    self.nradius = self.constants.nradius; 
    self.diff = 1; 
}
} // namespace som
#endif // SOM_TRAINSTATE_HPP_GUARD
