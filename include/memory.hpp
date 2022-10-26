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

#ifndef SOM_MEMORY_HPP_GUARD
#define SOM_MEMORY_HPP_GUARD


#include "myprint.hpp"


#define self (*this)


namespace som 
{

using namespace myprint; 


/** @brief Alias for a function that allocates memory */
template <class T>
using Malloc = T*(*)(unsigned); 
/** @brief Alias for a function that frees memory */
template <class T>
using Free = void(*)(T*); 


/** @brief Allocates contiguous memory with std::malloc for count objects of type T */
template <class T>
inline 
T* 
malloc(unsigned count) 
{ 
    Context context{ 
        "allocating", count *sizeof(T),
        "bytes with std::malloc" 
    }; 
    T* ptr = static_cast<T*>(
        std::malloc( count *sizeof(T) )
    );  
    assert( ptr != nullptr ); 
    context.results( "address", ptr ); 
    return ptr; 
}
/** @brief Frees contiguous memory with std::free for count objects of type T */
template <class T>
inline 
void 
free(T* ptr) 
{ 
    assert( ptr != nullptr ); 
    Context context{ 
        "freeing", ptr,
        "with std::free" 
    }; 
    std::free( ptr ); 
}


/** @brief Simple class representing the global state of the allocated memory. 
 *  Used internally by som::Memory<T>.
 */
class State 
{   
    unsigned mblocks{ 0 }; 
    unsigned mbytes{ 0 }; 

    public: 
    /** @brief Returns the total number of active memory blocks */
    unsigned blocks() const noexcept 
    { return self.mblocks; }

    /** @brief Returns the total number of allocated bytes */
    unsigned bytes() const noexcept 
    { return self.mbytes; }

    /** @brief Increments the total number of active memory blocks and 
     *  self.bytes() by bytes 
     */
    void 
    inc(unsigned bytes) noexcept 
    { 
        self.mblocks++; 
        self.mbytes += bytes; 
    }
    /** @brief Decrements the total number of active memory blocks and 
     *  self.bytes() by bytes 
     */
    void 
    dec(unsigned bytes) noexcept 
    { 
        self.mblocks--; 
        self.mbytes -= bytes; 

        if ( self.blocks() == 0 ) 
            assert( self.bytes() == 0 ); 
    }
}; // class State  


/** @brief Simple class representing a block of contiguous memory that 
 *  holds size() objects of type T
 */
template <class T> 
class Memory 
{
    inline static State state{}; // C++17 feature  

    unsigned msize{ 0 };
    Free<T> mfree{ nullptr }; 
    T* data{ nullptr }; 


    public:
    /** @brief Constructs a Memory object in disengaged state */
    Memory() : Memory{ 0 } {}
    /** @brief Constructs a Memory object to hold count elements of type T 
     *  using the som::malloc<T> and som::free<T> functions
     */
    Memory(unsigned count) : Memory( count, nullptr, nullptr ) {}
    /** @brief Constructs a Memory object to hold count elements of type T 
     *  using the provided functions Malloc<T> and Free<T>
     */
    Memory(unsigned count, Malloc<T> malloc, Free<T> free)
        : 
        msize{ count }, 
        mfree( free )
    {
        if ( self.size() > 0 )
        {
            Context context{ 
                "allocating", self.bytes(), "bytes"
            }; 

            if ( malloc == nullptr ) 
            {
                malloc = som::malloc<T>; 
                free = som::free<T>; 
            }
            assert( malloc != nullptr );
            assert( free != nullptr ); 

            self.mfree = free; 
            self.data = malloc( self.size() ); 
            assert( self.get() != nullptr ); 
            
            self.state.inc( self.bytes() );

            context.results(
                self.get(), 
                "total blocks and bytes", 
                self.state.blocks(),
                self.state.bytes() 
            ); 
        }
    }

    ~Memory() noexcept 
    { self.free(); }


    Memory(const Memory&) = delete; 
    Memory& operator = (const Memory&) = delete;
    
    Memory(Memory&& rhs) noexcept 
    {
        self.msize = rhs.size();
        self.mfree = rhs.mfree; 
        self.data = rhs.data; 

        rhs.disengage(); 
    }
    Memory& operator = (Memory&& rhs) noexcept 
    { 
        self.free(); 

        new (&self) Memory{ std::move(rhs) }; 
        return self; 
    }


    void 
    swap(Memory<T>& rhs) noexcept 
    {
        assert( self.size() == rhs.size() ); 
        assert( self.mfree == rhs.mfree ); 
        std::swap( self.data, rhs.data );
    }


    unsigned size() const noexcept { return self.msize; }
    unsigned bytes() const noexcept { return self.size() *sizeof(T); }

    /** @brief Get access to the memory */
    T* get() noexcept { return self.data; }
    /** @brief Get access to the memory */
    const T* get() const noexcept { return self.data; }

    /** @brief Iterator interface */
    T* bebin() noexcept { return self.get(); }
    /** @brief Iterator interface */
    const T* cbegin() const noexcept { return self.get(); }
    /** @brief Iterator interface */
    const T* end() noexcept { return self.get() + self.size(); }
    /** @brief Iterator interface */
    const T* cend() const noexcept { return self.get() + self.size(); }


    public:
    /** @brief Release resouces and disengage the object */
    void 
    free() noexcept 
    { 
        if ( self.size() > 0 ) 
        {
            Context context{ 
                "deallocating", self.bytes(), 
                "bytes at", self.get(), 
            }; 

            self.mfree( self.get() );  
            self.state.dec( self.bytes() ); 

            context.results( 
                "total blocks and bytes", 
                self.state.blocks(),
                self.state.bytes() 
            ); 
        }
        self.disengage(); 
    }
    
    void 
    /** @brief Disengages the object, nothing is freed */
    disengage() noexcept 
    {
        self.msize = 0; 
        self.data = nullptr; 
    }
}; // class Memory  
} // namespace som
#undef self
#endif // SOM_MEMORY_HPP_GUARD
