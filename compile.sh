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

#! /bin/sh 


srcdir="$( pwd )"

for build in Debug Release; do
    bindir="build/${build}"
    mkdir -p $bindir

    cd $bindir 
    cmake -DCMAKE_BUILD_TYPE="$build" $srcdir 
    make -j 
    cd $srcdir
done 
