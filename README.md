# DIAPASOM - DIstributed And PArallel Self Organizing Maps 

Simple implementation of the parallel data-partitioned batch SOM algorithm, 
see section 3.2 of 
[Lawrence at al., 1995](http://syllabus.cs.manchester.ac.uk/pgt/2017/COMP61021/reference/parallel-batch-SOM.pdf). 


### Documentation 

To genetate the documentation simply run
```
doxygen 
``` 
in the current directory.  
Now the documentation is available at ./docs/html/index.html

### Build 

To build this software you need 
- a C++17 compiler (inline variables are used)
- cmake >= 3.0 
- openmpi (optional, for mpi parallel support) 
- openshmem (optional, for openshmem parallel support)

Both MPI and openshmem are supported for parallel computations.  
The serial and MPI implementation are enabled by default in CMakeLists.txt, 
while the openshmem implementation is not.
Thus by running 
```
cmake SOURCE_DIRECTORY
cmake --build .
```
you will compile the both the serial and MPI implementations, 
both the executable (serial and mpi) 
and the shared libraries (libsom_serial and libsom_mpi). 

To compile the openshmem implementation (disabled by default), 
and, say, disable the other 2 implementations, you can run. 
```
cmake -D serial=OFF -D mpi=OFF -D oshmem=ON SOURCE_DIRECTORY
cmake --build .
```

By default cmake will build the Debug versions of the libraries and the executables, 
you can alter this behaviour, say for the Release versions, by running: 
```
cmake -DCMAKE_BUILD_TYPE=Release SOURCE_DIRECTORY
cmake --build .
```

The shell script compile.sh can be used to compile both the Debug and Release versions of the code. 

By default also testing (via ctest) is enabled, if you wish to disable it, compile with: 
```
cmake -D testing=OFF SOURCE_DIRECTORY 
cmake --build .
```

Finally, the binaries and libraries can be installed to a location of your choice (${CMAKE_BINARY_DIR}/install by default) 
by passing to cmake the CMAKE_INSTALL_PREFIX option: 
```
cmake -D CMAKE_INSTALL_PREFIX="the path of your choice" SOURCE_DIRECTORY 
cmake --build .
cmake --install .
```


### Usage 

After cmake run you can use the different versions simply by typing, 
say for the MPI with 3 ranks version: 
```
mpirun -np 3 ./som.mpi 
```

This will run the program with the default values of the parameters and print in the current directory 
the initial (epoch 0) state of the lattice (lattice0.out) 
and the state of the Lattice for the last epoch of the training (say lattice23.out).

It is quite easy to modify the values of the parameters for the training process, 
you just need to pass them as CLI arguments. 
For example this how test "mpiBS0RS0" (batchsize = 0 and random seed = 0) is run:  
```
mpirun -np 3 /home/mate/test/build/som.mpi dataset=/home/mate/test/tests/dataset.txt latticedim=10 batchsize=0 epochs=20 outevery=10 rseed=0 epcall=/home/mate/test/build/libepcall.so
```

As it can be seen you can provide a dynamic library for the "epcall" parameter containing an 
"extern "C" void epcall(const som::Lattice\*)" function that will be called at each epoch.  
This is used by the tests suite 
(tests/epcall.cpp) 
to produce more output then the default one in order to aid 
testing wether the results are correct and reproducible. 

For a complete list of available parameters see the documentation for the 
som::TrainSettings class.
