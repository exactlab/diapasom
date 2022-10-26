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

#ifndef SOM_PARALLEL_HPP_GUARD
#define SOM_PARALLEL_HPP_GUARD


#include "common.hpp"
#include "memory.hpp"


extern "C" 
{
#include "parallel.h"
}

#define self (*this)

namespace som 
{
namespace parallel 
{

using namespace myprint; 


#define u64 uint64_t


/** @brief Simple class representing a Timer of type T (std::chrono::seconds by default) */
template <class T=std::chrono::seconds>
class Timer 
{
    u64 mtotal{ 0 }; 
    bool running{ false }; 

    public: 
    Timer() {} 

    Timer(const Timer&) = delete; 
    Timer& operator = (const Timer&) = delete; 
    Timer(Timer&&) = default; 
    Timer& operator = (Timer&&) = default; 

    /** @brief Returns the total amount of T counted */
    u64 total() const noexcept; 
    
    /** @brief Returns the T count since epoch */ 
    static u64 now() noexcept; 

    /** @brief Starts the Timer */
    Timer<T>& start() noexcept; 
    
    /** @brief Stops the Timer */
    Timer<T>& stop() noexcept; 
    
    /** @brief Returns the max() time accross all ranks() */
    u64 max() const; 

    private: 
    void start(u64 now) noexcept;
    void stop(u64 now) noexcept; 
}; // class Timer  


/** @brief Simple class representing the state of the parallel infrastructure */
class State 
{
    unsigned mrank{ 0 };
    unsigned mranks{ 0 }; 
    Timer<std::chrono::microseconds> mtimer{}; // time spent in communication  

    public:
    /** @brief Initializes the parallel infrastructure */
    State(); 
    
    /** @brief Finalizes the parallel infrastructure */
    ~State() noexcept; 

    /** @brief Returns the rank of the process */
    unsigned rank() const noexcept { return self.mrank; }
    /** @brief Returns the total number ranks of the computation */ 
    unsigned ranks() const noexcept { return self.mranks; }
    /** @brief Returns the name of the parallel backend in use */ 
    const char* backend() const noexcept { return parallel_backend(); }
    /** @brief Returns a handle to the Timer object used */
    const Timer<std::chrono::microseconds>& 
    timer() const noexcept { return self.mtimer; }

    /** @brief Sends "bytes" bytes (at memory) to all other ranks. 
     *  Do not use, use Memory<T>::broadcast() instead 
     */
    void broadcast(void *memory, unsigned bytes, unsigned root) noexcept; 
    /** @brief Sums "count" double (at src) accross all ranks() (at dst) and 
     *  sends the result to all ranks(). 
     *  Do not use, use Memory<T>::sum_all() instead 
     */
    void sum_all(const double* src, double* dst, unsigned count) noexcept; 
    /** @brief Computes the max for "count" double (at src) accross all ranks() (at dst) and 
     *  sends the result to all ranks(). 
     *  Do not use, use Memory<T>::max_all() instead 
     */
    void max_all(const double* src, double* dst, unsigned count) noexcept; 
}; // class State  

/* This takes care that the parallel infrastructure is initialized and it initialized only once */
inline State state{}; 



/** @brief Returns the rank of the process */
inline
unsigned 
rank() noexcept 
{ return som::parallel::state.rank(); }

/** @brief Returns the total number ranks of the computation */ 
inline
unsigned 
ranks() noexcept 
{ return som::parallel::state.ranks(); }

/** @brief Returns the name of the parallel backend in use */ 
inline 
const char* 
backend() noexcept 
{ return som::parallel::state.backend(); }

/** @brief Returns a handle to the Timer object used by the parallel infrastructure */
inline 
const Timer<std::chrono::microseconds>& 
timer() noexcept 
{ return som::parallel::state.timer(); }


/** @brief Allocates (contiguous) memory for count elements of type T that 
 *  can be safely sent to and received from other ranks.
 *  Do not use directly, use som::parallel::Memory<T> instead  
 */
template <class T>
T* 
malloc(unsigned count) noexcept 
{
    assert( count > 0 );  
    Context context{
        "allocating", count *sizeof(T), 
        "bytes with som::parallel_malloc" 
    }; 
    T* ptr = static_cast<T*>( 
        parallel_malloc( count *sizeof(T) )
    ); 
    assert( ptr != nullptr ); 
    context.results( "address", ptr ); 
    return ptr; 
}


/** @brief Frees (contiguous) memory for count elements of type T that 
 *  can be safely sent to and received from other ranks.
 *  Do not use directly, use som::parallel::Memory<T> instead  
 */
template <class T>
void 
free(T* ptr) noexcept 
{
    assert( ptr != nullptr ); 
    Context context{
        "freeing", ptr, 
        "with som::parallel_free" 
    }; 
    parallel_free( ptr );
}


/** @brief Simple class representing a block of memory that can be safely sent to and 
 *  received from other ranks (it provides methods to achieve that)
 */
template <class T>
class Memory : public som::Memory<T>
{ 
    public: 
    /** @bief Constructs a Memory object in disengaged state */
    Memory() : Memory{ 0 } {}
    /** @brief Constructs a Memory object to hold count elements of type T 
     *  using som::parallel::malloc<T>. 
     *  Memory is freed with som::parallel::free<T>.
     */
    Memory(unsigned count) 
        : 
        som::Memory<T>{ count, som::parallel::malloc<T>, som::parallel::free<T> } 
    {} 

    /** @brief Sends Memory<T> (from rank = root) to all other ranks */
    void broadcast(unsigned root) noexcept; 
    /** @brief Sums Memory<T> accross all ranks() and sends the result to all ranks() */
    void sum_all(som::parallel::Memory<T>& dst) const noexcept; 
    /** @brief Computes the max of Memory<T> accross all ranks() and sends the result to all ranks() */
    void max_all(som::parallel::Memory<T>& dst) const noexcept; 
}; // class Memory 


/** @brief Distributes a number (total) among ranks() and 
 *  return the share for the given rank
 */
inline
unsigned 
distribute(const unsigned total, const unsigned rank) noexcept
{
    const unsigned perrank{ total /som::parallel::ranks() };     
    const unsigned remainder{ total - (perrank *som::parallel::ranks()) }; 
    return (rank < remainder)? perrank + 1 : perrank; 
}

/** @brief Distributes a number (total) among ranks() and 
 *  return the share for the current rank
 */
inline 
unsigned 
distribute(const unsigned total) noexcept 
{ 
    return som::parallel::distribute( 
        total, 
        som::parallel::rank() 
    ); 
}


/* Timer<T> methods BEGIN */
template <class T>
inline 
u64 
Timer<T>::total() const noexcept 
{ 
    // does not make sense to read it when still running 
    assert( not self.running ); 
    return self.mtotal; 
}

template <class T>
inline 
void 
Timer<T>::start(u64 now) noexcept
{ 
    assert( not self.running );  
    self.mtotal -= now; 
    self.running = true; 
}
template <class T>
inline 
Timer<T>&
Timer<T>::start() noexcept
{ 
    self.start( self.now() );
    return self; 
}

template <class T>
inline 
void 
Timer<T>::stop(u64 now) noexcept
{ 
    assert( self.running );  
    self.mtotal += now; 
    self.running = false; 
}
template <class T>
inline 
Timer<T>&
Timer<T>::stop() noexcept
{ 
    self.stop( self.now() ); 
    return self; 
}

template <class T>
inline 
u64
Timer<T>::max() const
{
    som::parallel::Memory<double> send{ 1 }; 
    som::parallel::Memory<double> recv{ 1 }; 

    *send.get() = static_cast<double>( self.total() );
    send.max_all( recv );
    return static_cast<u64>( *recv.get() ); 
}
    
template <class T>
u64 
Timer<T>::now() noexcept 
{ 
    return std::chrono::duration_cast<T>( 
        std::chrono::steady_clock::now().time_since_epoch()
    ).count(); 
}
/* Timer<T> methods END */


/* State methods BEGIN */
inline 
State::State() 
{
    if ( self.ranks() > 0 )
        return; 

    Context context{
        self.backend(), "initializing"
    }; 
    parallel_initialize( &self.mrank, &self.mranks ); 
    assert( self.ranks() > 0 ); 
    context.results(
        "rank", self.rank(), 
        "of", self.ranks()
    ); 
}

inline 
State::~State() noexcept
{
    if ( self.ranks() == 0 ) 
        return; 

    Context context{
        "rank", self.rank(), 
        "of", self.ranks(),
        self.backend(), "finalizing"
    }; 
    parallel_finalize(); 
    self.mrank = self.mranks = 0; 
}


inline 
void 
State::broadcast(void* memory, unsigned bytes, unsigned root) noexcept
{

    assert( memory != nullptr );   
    assert( bytes > 0 ); 
    assert( root < self.ranks() ); 
    Context context{
        "rank", self.rank(),  
        "of", self.ranks(),
        "broadcasting", bytes, 
        "bytes at", memory, 
        "using", self.backend(), 
        "with root", root
    }; 
    self.mtimer.start(); 
    parallel_broadcast(
        memory, bytes, root 
    );  
    self.mtimer.stop(); 
}


inline 
void 
State::sum_all(const double* src, double* dst, unsigned count) noexcept 
{
    assert( src != nullptr );
    assert( dst != nullptr ); 
    assert( count > 0 ); 
    Context context{
        "rank", self.rank(),  
        "of", self.ranks(), 
        "summing", count, 
        "doubles using", self.backend()
    };
    self.mtimer.start();  
    parallel_sum_all_double(
        src, dst, count 
    ); 
    self.mtimer.stop();  
}


inline 
void 
State::max_all(const double* src, double* dst, unsigned count) noexcept 
{
    assert( src != nullptr );
    assert( dst != nullptr ); 
    assert( count > 0 ); 
    Context context{
        "rank", self.rank(),  
        "of", self.ranks(), 
        "maxing", count, 
        "doubles using", self.backend()
    };
    self.mtimer.start();  
    parallel_max_all_double(
        src, dst, count
    ); 
    self.mtimer.stop();
}
/* State methods END */


/* Memory<T> methods BEGIN */
template <class T>
void 
Memory<T>::broadcast(unsigned root) noexcept
{
    som::parallel::state.broadcast(
        static_cast<void*>( self.get() ),
        self.bytes(), 
        root
    ); 
}
    
template <>
inline 
void 
Memory<double>::sum_all(som::parallel::Memory<double>& dst) const noexcept 
{
    assert( self.bytes() == dst.bytes() );  
    som::parallel::state.sum_all(
        self.cbegin(), 
        dst.bebin(), 
        self.size()
    ); 
}

template <>
inline 
void 
Memory<double>::max_all(som::parallel::Memory<double>& dst) const noexcept 
{
    assert( self.bytes() == dst.bytes() );  
    som::parallel::state.max_all(
        self.cbegin(), 
        dst.bebin(), 
        self.size()
    ); 
}
/* Memory<T> methods END */
} // namespace parallel
} // namespace som 
#undef self
#endif // SOM_PARALLEL_HPP_GUARD
