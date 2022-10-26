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

cmake_minimum_required( VERSION 3.0 )
# suppress warning about quotes ...
cmake_policy( SET CMP0054 NEW ) 

project( DIAPASOM LANGUAGES C CXX )

foreach( language IN ITEMS CXX ) 
    set( CMAKE_${language}_STANDARD 17 )
    set( CMAKE_${language}_EXTENSIONS OFF )
endforeach()

add_compile_options(
    -Wall -Wextra -pedantic -Werror -fmax-errors=1
)
add_link_options(
    "LINKER:--copy-dt-needed-entries"
)


if( NOT DEFINED CMAKE_BUILD_TYPE )
    set( CMAKE_BUILD_TYPE "Debug" )
endif()


option( verbose_make "make make verbose" ON )
option( testing "enable testing" ON )
option( serial "build serial version" ON )
option( mpi "build mpi parallel version" ON )
option( oshmem "build openshmem parallel version" OFF )

if( verbose_make ) 
    message( STATUS "making make verbose" )
    set( CMAKE_VERBOSE_MAKEFILE true )
endif()


if ( DEFINED ENV{CMAKE_INSTALL_PREFIX} )
    set( CMAKE_INSTALL_PREFIX "$ENV{CMAKE_INSTALL_PREFIX}" )
else()
    set( CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" ) 
endif()

set( CMAKE_SKIP_BUILD_RPATH FALSE )
set( CMAKE_BUILD_WITH_INSTALL_RPATH FALSE )
set( CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE )
set( CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib" )


set( implementations )
if ( serial )
    list( APPEND implementations "serial" ) 
endif()
if ( mpi )
    list( APPEND implementations "mpi" ) 
endif()
if ( oshmem )
    list( APPEND implementations "oshmem" ) 
endif()
message( STATUS "implementations: ${implementations}" )



message( STATUS "building core som ibrary" )
set( cpps dataset lattice )
foreach( cpp IN ITEMS ${cpps} )
    list( APPEND sources "${CMAKE_SOURCE_DIR}/src/${cpp}.cpp" ) 
endforeach()

set( coreLib "somcore" ) 
set( includes "${CMAKE_SOURCE_DIR}/include" )

add_library(
    ${coreLib}
    SHARED
    ${sources}
)
target_include_directories(
    ${coreLib}
    PUBLIC 
    ${includes}
)
target_link_libraries( # dlopen() support
    ${coreLib}
    PRIVATE
    dl
)

foreach( implementation IN ITEMS ${implementations} )  
    message( STATUS "building ${implementation} som library and executable" )

    set( sources "${CMAKE_SOURCE_DIR}/src/parallel.c" ) 
    
    set( parallelLib "som_parallel_${implementation}" )
    set( mainLib "som_${implementation}" )
    set( executable "som.${implementation}" )

    set( includes "${CMAKE_SOURCE_DIR}/include" )
    set( definitions )
    set( libraries )    

    if ( "${implementation}" STREQUAL "mpi" )
        find_package( MPI REQUIRED ) 
        list( APPEND includes "${MPI_C_INCLUDE_DIRS}" )
        list( APPEND definitions "USE_PARALLEL_MPI=1" )
        list( APPEND libraries "${MPI_LIBRARIES}" )
        
        list( GET MPI_LIBRARIES 0 mpiFolder )
        get_filename_component( mpiFolder ${mpiFolder} DIRECTORY ) # "${mpiFolder}/lib" 
        get_filename_component( mpiFolder ${mpiFolder} DIRECTORY )
    endif()
    
    if ( "${implementation}" STREQUAL "oshmem" ) 
        find_library( oshmemLibrary oshmem )
        get_filename_component( oshmemFolder ${oshmemLibrary} DIRECTORY )
        get_filename_component( oshmemFolder ${oshmemFolder} DIRECTORY )

        list( APPEND includes "${oshmemFolder}/include" )
        list( APPEND definitions "USE_PARALLEL_OSHMEM=1" )
        list( APPEND libraries "${oshmemLibrary}" )
    endif()

    message( STATUS "includes: ${includes}" )
    message( STATUS "definitions: ${definitions}" ) 
    message( STATUS "libraries: ${libraries}" )

    # the parallel library
    add_library(
        ${parallelLib}
        SHARED
        ${sources}
    )
    target_include_directories(
        ${parallelLib}
        PUBLIC 
        ${includes}
    )
    target_compile_definitions(
        ${parallelLib} 
        PRIVATE 
        ${definitions}
    )
    target_link_libraries(
        ${parallelLib}
        PRIVATE 
        ${libraries}
    )

    # the main SOM library
    add_library( 
        ${mainLib} 
        SHARED
        "${CMAKE_SOURCE_DIR}/src/som.cpp"
    )
    target_include_directories(
        ${mainLib}
        PUBLIC 
        ${includes}
    )
    target_link_libraries(
        ${mainLib}
        PRIVATE
        ${coreLib}  
        ${parallelLib}
    )

    add_executable(
        ${executable}
        "${CMAKE_SOURCE_DIR}/main.cpp"
    )
    target_link_libraries(
        ${executable}
        PRIVATE
        ${mainLib} 
    )

    list( APPEND targets "${coreLib}" "${parallelLib}" "${mainLib}" "${executable}" )
endforeach()


set( CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib" )
list( REMOVE_DUPLICATES targets )
install( 
    TARGETS ${targets}
    RUNTIME DESTINATION "bin" 
    LIBRARY DESTINATION "lib"
    ARCHIVE DESTINATION "lib/static"    
)
install(
    DIRECTORY "${CMAKE_SOURCE_DIR}/include"
    DESTINATION "include"
    FILES_MATCHING PATTERN "*.h*" 
)


if ( testing ) 
    message( STATUS "enabling testing" )
    enable_testing()

    add_library(
        epcall 
        SHARED
        "${CMAKE_SOURCE_DIR}/tests/epcall.cpp" 
    )
    target_include_directories(
        epcall 
        PUBLIC 
        "${CMAKE_SOURCE_DIR}/include"
    )

    set( testsOutFolderBase "${CMAKE_BINARY_DIR}/testsout" )
    file( REMOVE_RECURSE "${testsOutFolderBase}" )

    foreach( implementation IN ITEMS ${implementations} ) 
        include( "${CMAKE_SOURCE_DIR}/tests/tests.cmake" )
    endforeach()
endif()
