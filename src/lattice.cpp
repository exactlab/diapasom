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

#include "trainfuncs.hpp"

extern "C" 
{
#include <dlfcn.h> /** dlopen, dlclose, dlsym */
}

namespace som 
{

using namespace myprint; 


TrainSettings::TrainSettings(const parameters::Handler& params) 
{
    self.dataset( params.vget<std::string>( "dataset" ) ); 
    self.latticedim( params.vget<unsigned>( "latticedim", 10 ) ); 
    self.rows( params.vget<unsigned>( "rows", self.latticedim() ) ); 
    self.cols( params.vget<unsigned>( "cols", self.latticedim() ) ); 

    self.epochs( params.vget<unsigned>( "epochs", 0 ) ); 
    self.diffmin( params.vget<double>( "diffmin", 0 ) ); 
    self.batchsize( params.vget<unsigned>( "batchsize", 0 ) ); 
    self.nradius( params.vget<double>( "nradius", 0 ) ); 
    self.rdecay( params.vget<double>( "rdecay", 1e-1 ) ); 
    self.rseed( params.vget<unsigned>( "rseed", 0 ) ); 
    
    /** if a dl is supplied epcall is read from it */
    auto dlname = params.vget<std::string>( "epcall", "" );
    if (dlname.size() > 0) 
        self.epcall( dlname ); 
}


TrainSettings& TrainSettings::epcall(const std::string& dlname) noexcept 
{
    Context context{
        "reading epcall(const Lattice*) from", 
        dlname 
    }; 
    const auto function = (void(*)(const Lattice*)) dlsym( 
        dlopen( dlname.c_str(), RTLD_LAZY ), "epcall" 
    ); 

    if ( function == nullptr ) 
        context.results( "failed" ); 
    else
    {
        self.epcall( function ); 
        context.results( "epcall", function ); 
    }
    return self; 
}


unsigned 
Lattice::epoch() const noexcept 
{ 
    return (self.state != nullptr)? 
        self.state->epoch : 0; 
}


Lattice& 
Lattice::train(const TrainSettings& settings) 
{
    Context c1{ 
        "rank", self.rank(),
        "of", self.ranks(),
        "training process" 
    }; 

    TrainState state{ self, settings }; 
    self.state = &state; 

    state.total.start();

    weights_random_init( state ); 
    
    state.epoch = 0;    
    self.print(); 

    if ( settings.epcall() )
    {
        Context context{ "calling epcall(const Lattice&) for epoch", state.epoch }; 
        settings.epcall()( &self );  
    }

    for (state.epoch = 1; state.epoch <= state.constants.epochs; ++state.epoch)
    {
        Context c2{
            "epoch", state.epoch, "of", state.constants.epochs
        }; 

        for (state.batch = 1; state.batch <= state.constants.batches; ++state.batch)
            batch_present( state ); 
    

        // update nradius
        state.nradius = state.constants.nradius *std::exp(
            static_cast<double>( state.epoch ) *state.constants.rdecay
        );  

        if ( state.diff < state.constants.diffmin )
            break; 

        if ( settings.epcall() )
        {
            Context context{ "calling epcall(const Lattice&) for epoch", state.epoch }; 
            settings.epcall()( &self );  
        }
        
        c2.results( "diff:", state.diff ); 
       
        myprint::outln( "epoch", state.epoch, "diff", state.diff ); 
    }
    state.total.stop(); 

    self.print(); 
    
    myprint::outln( "training total time (microseconds):", state.total.max() ); 
    myprint::outln( "training communication time (microseconds)", som::parallel::timer().max() ); 
    return self;  
}


void 
Lattice::print(const std::string& fname) const 
{ 
    // only the master rank will print out the state of the Lattice 
    if ( self.rank() > 0 ) return; 
    
    Printer printer{}; 
    printer.file(
        (fname.size() > 0)? fname :
            "lattice" + std::to_string( self.state->epoch ) + ".out" 
    );
    
    Context context{ 
        "rank", self.rank(), 
        "of", self.ranks(), 
        "printing Lattice state at epoch", self.state->epoch, 
        "to", printer.file(), 
    }; 
    
    const Weights<som::Memory<double>>& weights{ self.state->weights };
    const double* entry; 
    for (unsigned row{ 0 }; row < weights.rows(); ++row) 
        for (unsigned col{ 0 }; col < weights.cols(); ++col)
        {
            entry = weights.entry( weights.index(row, col) ); 
            printer.print( row, col, " " ); 
            for (unsigned d{ 0 }; d < weights.dimensions(); ++d)
                printer.print( entry[ d ], " " ); 
            printer.println( "" ); 
        }
}
} // namespace som
