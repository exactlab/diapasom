# DIAPASOM - DIstributed And PArallel Self Organizing Maps 
# 
# Copyright (c) 2022, eXact lab (<https://www.exact-lab.it>) All Rights Reserved.
# You may use, distribute and modify this code under the
# terms of the BSD 4-clause license. 
#
# This program is distributed in the hope that it will be useful, but WITHOUT 
#  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       
# FITNESS FOR A PARTICULAR PURPOSE. 
#
# You should have received a copy of the BSD 4-clause license along 
# with DIAPASOM; see the file LICENSE. If not see 
# <https://en.wikipedia.org/wiki/BSD_licenses>.
#
#
# Authors: 
#     Matteo Barnaba <matteo.barnaba@exact-lab.it>
#     Matteo Poggi <matteo.poggi@exact-lab.it>
#     Giuseppe Piero Brandino <giuseppe.brandino@exact-lab.it>
#

# variables "testsOutFolderBase" and  "implementation" are set by the main cmake file 
# that will include this file


set( latticedim 10 )  # 10x10 square lattice 
set( epochs 20 )      # max 20 epochs


foreach( dataset IN ITEMS 1 2 )
    set( datafile "${CMAKE_SOURCE_DIR}/tests/dataset${dataset}.txt" )

    if ( "${dataset}" EQUAL 1 ) 
        set( batchsizes 0 10 )
    else()
        set( batchsizes 0 100 )
    endif()

    foreach( rseed IN ITEMS 0 123 )                  # if rseed > 0 then the results are reproducible 
        foreach( batchsize IN ITEMS ${batchsizes} )  # the number of records presented before updating the state of the lattice
            set( testname "${implementation}DS${dataset}RS${rseed}BS${batchsize}" )
            set( outfolder "${testsOutFolderBase}/${testname}" )
            file( MAKE_DIRECTORY "${outfolder}" )

            set( executable "${CMAKE_BINARY_DIR}/som.${implementation}" )
            set( cmd "${CMAKE_SOURCE_DIR}/tests/wrapper ${executable}" )

            if ( "${implementation}" STREQUAL "mpi" )
                set( cmd "${mpiFolder}/bin/mpirun -np 3 ${cmd}" )
            endif()

            if ( "${implementation}" STREQUAL "oshmem" )
                set( cmd "${oshmemFolder}/bin/shmemrun -np 3 ${cmd}" )
            endif()

            string( 
                CONCAT cmd 
                "cd ${outfolder} && "
                "${cmd} "
                "dataset=${datafile} "
                "latticedim=${latticedim} "
                "batchsize=${batchsize} "
                "epochs=${epochs} "
                "rseed=${rseed} "
                "epcall=${CMAKE_BINARY_DIR}/libepcall.so "
            ) 
            
            message( STATUS "${testname}: ${cmd}" ) 
            add_test(
                NAME ${testname}
                COMMAND sh -c "${cmd}"
            )  
            
            # we check the results for 3 epochs against reference ones
            foreach( epoch IN ITEMS 0 1 20 ) 
                set( testname "${implementation}DS${dataset}RS${rseed}BS${batchsize}EP${epoch}" )
                
                set( output "${outfolder}/lattice${epoch}.out" )

                if ( epoch EQUAL 0 ) # state at epoch 0 (initial) does not depend on the batchsize, only rseed 
                    set( reference "${CMAKE_SOURCE_DIR}/tests/reference/dataset${dataset}" )
                else()
                    set( reference "${CMAKE_SOURCE_DIR}/tests/reference/dataset${dataset}RS${rseed}BS${batchsize}" )
                endif()
                set( reference "${reference}lattice${epoch}.out" ) 

                set( cmd "diff ${output} ${reference}")
                message( STATUS "${testname}: ${cmd}" ) 

                add_test( 
                    NAME ${testname}
                    COMMAND sh -c "${cmd}"
                )
                
                # if rseed == 0 results are no reproducible and the test shall fail
                if ( "${rseed}" STREQUAL 0 )
                    set_tests_properties(
                        ${testname}
                        PROPERTIES 
                        WILL_FAIL true
                    )
                endif()
            endforeach()
        endforeach()
    endforeach()
endforeach()
