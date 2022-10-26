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

#ifndef MYPRINT_HPP_GUARD 
#define MYPRINT_HPP_GUARD


#include <iostream>
#include <fstream>
#include <cassert>


#define self (*this)


namespace myprint 
{

/** @brief A simple class representing a Printer able to print to stdout (default), stderr 
 *  or any file defined via the file(const std::string&) method. 
 */
template <bool err_>
class Printer_
{
    std::string mfile{};            // filename associates to mfos, empty by default
    std::ofstream mfos{};           // file handle used, closed by default, when writing to stdout or stderr
    std::ostream* mos{ nullptr };   // pointer to the used stream handle, std::cout by default
    std::string sep{ " " };         // fields separator 

    /** @brief Returns the handle for the used stream */
    std::ostream&
    os() 
    { 
        assert( self.mos != nullptr );
        return *(self.mos); 
    }


    public: 
    /** @brief Constructs a Printer for stdout (by default) or stderr when err=true */
    explicit Printer_() 
    {
        if (err_) 
            self.stream( std::cerr ); 
        else
            self.stream( std::cout ); 
    }

    Printer_(const Printer_&) = delete; 
    Printer_& operator = (const Printer_&) = delete; 
    Printer_(Printer_&&) = default; 
    Printer_& operator = (Printer_&&) = default; 


    /** @brief Sets the filename to write to */
    Printer_&
    file(const std::string& fname) {
        self.mfile = fname; 
        self.mfos.open( self.mfile );
        assert( self.mfos.is_open() ); 

        self.stream( self.mfos ); 
        return self; 
    }
    /** @brief Returns the filename associated to the file handle used by the printer */ 
    const std::string& 
    file() const noexcept 
    { return self.mfile; }


    /** @brief Sets the stream to write to */
    Printer_& 
    stream(std::ostream& os) 
    {
        self.mos = &os; 
        return self; 
    }


    /** @brief Returns the fields separator used */
    const std::string& 
    separator() noexcept 
    { return self.sep; }

    /** @brief Sets the fields separator to use */
    Printer_& 
    separator(const std::string& sep) 
    {
        self.sep = sep; 
        return self; 
    }

    
    /** @brief Prints the last argument without appending a newline */
    template <class A>
    Printer_&
    print(const A& a) 
    {
        self.os() << a; 
        return self; 
    }

    /** @brief Prints an arbitrary number of arguments to the used stream without appending a newline */ 
    template <class A1, class ... A2toN> 
    Printer_&
    print(const A1& a1, const A2toN& ... a2toN)  
    {
        self.os() << a1; 
        self.os() << self.sep; 
        self.print( a2toN ... ); 
        return self; 
    }

    /** @brief Prints an arbitrary number of arguments to the used stream appending a newline */ 
    template <class A1, class ... A2toN> 
    Printer_& 
    println(const A1& a1, const A2toN& ... a2toN)  
    {
        self.print( a1, a2toN ... ); 
        self.os() << std::endl; 
        return self; 
    }
}; // class Printer_


using OutPrinter = Printer_<false>;
using Printer = OutPrinter; 
using ErrPrinter = Printer_<true>;



/** @brief A simple class representing a dummy printer, i.e., 
 *  it exposes the same interface as Printer_ but does really nothing, 
 *  very useful for debugging (all operations can be turned off if NDEBUG is defined). 
 */
class DummyPrinter 
{
    public:
    DummyPrinter& 
    file(const std::string&) { return self; }

    DummyPrinter&
    stream(std::ostream&) { return self; }
    
    DummyPrinter&
    separator(const std::string&) { return self; }

    template <class ... Args>
    DummyPrinter&
    print(const Args& ...) { return self; }
    
    template <class ... Args>
    DummyPrinter&
    println(const Args& ...) { return self; }
}; // class DummyPrinter


/** @brief Prints an arbitrary number of arguments to stdout without appending a newline */ 
template <class ... Args>
void 
out(const Args& ... args) 
{
    OutPrinter{}.print( args ... ); 
}

/** @brief Prints an arbitrary number of arguments to stdout appending a newline */
template <class ... Args>
void 
outln(const Args& ... args) 
{
    OutPrinter{}.println( args ... ); 
}

/** @brief Prints an arbitrary number of arguments to stderr without appending a newline */ 
template <class ... Args>
void 
err(const Args& ... args) 
{
    ErrPrinter{}.print( args ... ); 
}

/** @brief Prints an arbitrar number of arguments to stderr appending a newline */
template <class ... Args>
void 
errln(const Args& ... args) 
{   
    ErrPrinter{}.println( args ... ); 
}


#ifdef NDEBUG 
#undef NDEBUG 
#define NDEBUG true 
#else 
#define NDEBUG false 
#endif 


using DebugPrinter = typename std::conditional<NDEBUG, DummyPrinter, ErrPrinter>::type; 


#if NDEBUG 
template <class ... Args>
void 
debug(const Args& ... ) {}

#else // not NDEBUG
template <class ... Args>
void
debug(const Args& ... args) 
{ 
    DebugPrinter{}.println( "DEBUG", args ... ); 
}
#endif // not NDEBUG


/** @brief Simple class representing a context: 
 *  an arbitrary message is show by the constructor and 
 *  the same message (with addition via results()) is show 
 *  by the destructor. Very handy for debugging 
 */
class Context_
{
    inline static unsigned slevel{ 0 }; // C++17 feature 

    std::vector<std::string> mfields{}; 
    std::vector<std::string> mresults{}; 
    DebugPrinter printer{};
    unsigned level{ 0 }; 

    public:
    /** @brief Constructs a Context_ object and accepts an arbitrary number 
     *  of arguments that will be printed as message by the constructor and 
     *  the destructor
     */
    template <class ... Args>
    Context_(const Args& ... args) 
    {   
        self.level = Context_::slevel++;

        self.poputate( self.mfields, args ... ); 
        self.print_begin(); 
    }

    ~Context_() 
    { 
        self.print_end();
        Context_::slevel--; 
    }
    
    /** @brief Adds results that will be printed by the destructor after the ones 
     *  passed to the constructor 
     */
    template <class ... Args>
    Context_&
    results(const Args& ... args) 
    { 
        self.poputate( self.mresults, args ... ); 
        return self; 
    }


    template <class ... Args> 
    Context_& 
    println(const Args& ... args) 
    { 
        self.indent(); 
        self.printer.println( "", args ... ); 
        return self; 
    }
    
    template <class ... Args> 
    Context_& 
    print(const Args& ... args) 
    { 
        self.indent(); 
        self.printer.print( "", args ... ); 
        return self; 
    }
    
    
    private:
    template <class T>
    std::string 
    to_string(const T& arg) 
    {
        std::stringstream ss{};
        ss << arg; 
        return ss.str(); 
    }

    template <class A1, class ... A2toN>
    void 
    poputate(std::vector<std::string>& v, const A1& a1, const A2toN& ... a2toN)  
    {
        v.emplace_back( self.to_string(a1) ); 
        self.poputate( v, a2toN ... ); 
    }
    void poputate(std::vector<std::string>&) {}
    
    
    void indent(unsigned n=0)  
    {
        n = std::max( n, self.level );
        for (unsigned level{ 0 }; level < n; ++level)
            self.printer.print( ">" ); 
    }


    void print_begin() 
    {
        self.indent(); 
        for (const auto& field : self.mfields)  
            self.printer.print( field, "" ); 
        
        self.printer.println( "" );
        self.indent(); 
        self.printer.println( "{" ); 
        self.level++; 
    }

    void print_end() 
    {
        self.level--; 
        self.indent(); 
        self.printer.print( "} " ); 

        for (const auto& field : self.mfields) 
            self.printer.print( field, "" );  
        self.printer.println( "" ); 

        if ( self.mresults.size() > 0 ) 
        {
            self.indent(); 
            self.printer.print( "+=> " ); 
            for (const auto& field : self.mresults)
                self.printer.print( field, "" ); 
            self.printer.println( "" );
            self.printer.println( "" );
        }
    }
}; // class Context_
   

/** @brief Dummy Context_ object that exposes the same interface as Context_ 
 *  but does nothing (debugging) 
 */
class DummyContext_
{
    public: 
    template <class ... Args>
    DummyContext_(const Args& ...) {}
    
    template <class ... Args>
    DummyContext_&
    results(const Args& ...) 
    { return self; }
    
    template <class ... Args>
    DummyContext_&
    print(const Args& ...) { return self; }
    
    template <class ... Args>
    DummyContext_&
    println(const Args& ...) { return self; }
}; 

/** @brief Simple class representing a context: 
 *  an arbitrary message is show by the constructor and 
 *  the same message (with addition via Context_::results()) is show 
 *  by the destructor. Very handy for debugging 
 */
using Context = typename std::conditional<NDEBUG, DummyContext_, Context_>::type; 
} // namespace myprint 
#undef self
#endif // MYPRINT_HPP_GUARD 
