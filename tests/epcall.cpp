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


using som::Lattice; 


/** @brief Simple function that prints the state of the 
 * Lattice for epoch 1 and 20 
 */
static inline 
void 
epcall(const Lattice& lattice) noexcept 
{
    const unsigned epoch{ lattice.epoch() }; 

    if ( epoch == 1 or epoch == 20 ) 
        lattice.print(); 
}


/** @brief C wrapper for epcall(const Lattice&), 
 *  needed since 
 *  <a href="https://man7.org/linux/man-pages/man3/dlopen.3.html">dlopen</a> 
 *  is used by the SOM library to load the dynamic libraries at runtime.
 */
extern "C"
void epcall(const Lattice* lattice) 
{
    epcall( *lattice ); 
}
