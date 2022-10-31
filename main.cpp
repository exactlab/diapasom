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

#include "som.hpp"


using som::TrainSettings; 
using som::Lattice; 


namespace som::parallel 
{
    State* state = nullptr; 
}


int main(const int argc, const char* argv[]) 
{
    som::parallel::State state{}; 
    som::parallel::state = &state; 

    myprint::debug( "porco dio" ); 

    /* Create the object that stores the command-line arguments */
    parameters::CliArgsParser cliargs{ argc, argv };    
    
    /* Let the TrainSettings constructor parse the CLI arguments and populate 
     *  itself accordingly. 
     */
    TrainSettings settings{ cliargs }; 


    /* Create the Lattice object */ 
    Lattice lattice{ settings.rows(), settings.cols() };


    /* Launch the training process of the Lattice for the given settings */
    lattice.train( settings ); 

    myprint::debug( "dio porco" ); 
    return 0; 
}
