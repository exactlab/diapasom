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

#include "dataset.hpp"

#define self (*this)

namespace som 
{

/** helper function for Dataset::Dataset(std::string& fname), 
 *  returns the number of dimensions (columns) of the dataset
 */
static inline 
unsigned dataset_dimensions(const std::string& fname) 
{
    /** read the first line and count its columns */
    std::ifstream ifile{ fname }; 
    if ( not ifile.is_open() )
        throw std::runtime_error{ "could not open dataset " + fname }; 

    std::string first{}; 
    std::getline( ifile, first ); 
    assert( first.size() > 0 ); 

    std::istringstream iss{ first };
    std::vector<double> values{}; 

    values.reserve( 1024 ); 
    double value; 
    while (iss >> value) 
        values.push_back( value ); 
    
    const unsigned dimensions{ static_cast<unsigned>(values.size()) };
    assert( dimensions > 0 );   // a dataset with no columns is empty 
    return dimensions; 
}


/** @brief helper function for Dataset(const std::string&) that computes the sum a double over all ranks */
static inline 
double 
valuesSum(double ranksum) noexcept 
{
    Context context{ __func__, "with ranksum", ranksum };

    som::parallel::Memory<double> sendbuff{ 1 }; 
    som::parallel::Memory<double> recvbuff{ 1 }; 
    
    sendbuff.get()[ 0 ] = ranksum; 
    sendbuff.sum_all( recvbuff ); 
    
    const double sum{ recvbuff.get()[ 0 ]  };  

    context.results( "sum", sum ); 
    return sum; 
}

Dataset::Dataset(const std::string& fname, unsigned batchsize) 
{
    Context context{ 
        "Dataset::Dataset() with", 
        fname, batchsize 
    }; 

    assert( fname.size() > 0 ); // fname must be provided 
    const unsigned dimensions{ dataset_dimensions(fname) }; // get number of dimensions (columns) 

    std::ifstream ifile{ fname }; 
    std::string line{};
    std::vector<double> values{}; 
    values.reserve( 1024 *dimensions ); 
    double value; 

    /*  read the file line by line and parse it when the line (record) 
     *  pertains the current rank. 
     *  valmean is assessed as well, via ranksum.
     */ 
    if ( batchsize < self.ranks() ) 
        batchsize = UINT_MAX; 

    const unsigned rank{ self.rank() };  
    const unsigned ranks{ self.ranks() }; 

    unsigned trecords; 
    double ranksum{ 0 }; 
    unsigned bidx{ 0 };   // index inside the batch
    for (trecords = 0; std::getline( ifile, line ); ++trecords) 
    {
        if ( bidx % ranks == rank ) 
        { 
            context.println( "gidx and bidx", trecords, bidx );

            std::istringstream iss{ line };
            for (unsigned d{ 0 }; d < dimensions; ++d) 
            {
                iss >> value; 
                values.push_back( value );

                ranksum += value; 
            }
        }
        bidx = (bidx < batchsize - 1)? bidx + 1 : 0; 
    }
    self.mvalmean = valuesSum( ranksum ) /static_cast<double>( trecords );


    /* initialize correctly the fields of the class */
    self.mdimensions = dimensions; 
    self.mtrecords = trecords; 
    self.mrrecords = values.size() /self.dimensions(); 
    self.mbatchsize = (batchsize < UINT_MAX)? batchsize : self.trecords(); 
    self.mrbatchsize = som::parallel::distribute( self.batchsize() );  

    /* allocate mdata and copy values in mdata */
    self.mdata = som::Memory<double>{ self.rrecords() *self.dimensions() }; 
    std::copy(
        values.cbegin(), 
        values.cend(), 
        self.mdata.get()
    );
    context.results( 
        "batchsize", self.batchsize(), "rbatchsize", self.rbatchsize(), 
        "trecords", self.trecords(), "rrecords", self.rrecords() 
    ); 
}

void Dataset::print(const std::string& fname) const noexcept 
{
    myprint::Printer printer{}; 
    if ( fname.size() > 0 ) 
        printer.file( fname ); 

    const unsigned rank{ self.rank() };
    const unsigned ranks{ self.ranks() }; 
    const double* record; 
    for (unsigned idx{0}; idx < self.trecords(); ++idx)
    {
        if ( rank == idx % ranks ) 
        {
            idx = idx / ranks; 
            record = self.rrecord( idx );

            for (unsigned d{ 0 }; d < self.dimensions(); ++d) 
                printer.print( record[ d ], " " ); 
            printer.println( "" ); 
        }
    }
}
} // namespace som
