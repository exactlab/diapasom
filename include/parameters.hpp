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

#ifndef PARAMETERS_HPP_GUARD
#define PARAMETERS_HPP_GUARD

#include <unordered_map>
#include <vector>
#include <sstream>


#define self (*this)


namespace parameters
{

/** 
 * Simple class that manages parameters via an unordered_map.
 * It is basically a wrapper around that map, with useful functionalities for 
 * inserting and retrieving parameters' values. 
 */
class Handler
{
    protected:
    std::unordered_map<std::string, std::string> map{}; // the main data structure 
    std::vector<std::string> names{}; // names are stored as well to keep the order of insertion
    std::string mdel{ ":" };    // the delimiter between the name and the value of a parameter 
    std::string msep{ "\n" };   // the separator between parameters

    public:
    /** @brief Contruct a default Handler whit separator '\n' and delimiter ':' */
    Handler() {}

    Handler(const Handler&) = delete; 
    Handler& operator = (const Handler&) = delete; 
    Handler(Handler&&) = default; 
    Handler& operator = (Handler&&) = default; 
    
    
    /** @brief Returns the currently set delimiter */
    const std::string& 
    delimiter() const noexcept 
    { return self.mdel; }
    /** @brief Sets the new delimiter to be used */ 
    Handler& 
    delimiter(const std::string& del) noexcept 
    { 
        self.mdel = del; 
        return self; 
    }
    /** @brief Returns the currently set separator */
    const std::string 
    separator() const noexcept 
    { return self.msep; }
    /** @brief Sets the new separator to be used */ 
    Handler& 
    separator(const std::string& sep) noexcept 
    { 
        self.msep = sep; 
        return self; 
    }


    template <class T>
    T get(const std::string& name) const 
    {
        return self.get_<T>( name, nullptr ); 
    }
    template <class T>
    T vget(const std::string& name) const
    {
        T value{ self.get<T>( name ) }; 
        myprint::outln( name, ": ", value ); 
        return value;
    }

    template <class T>
    T get(const std::string& name, const T& defval) const 
    {
        return self.get_<T>( name, &defval );   
    }
    template <class T>
    T vget(const std::string& name, const T& defval) const
    {
        T value{ self.get<T>( name, defval ) }; 
        myprint::outln( name, ": ", value ); 
        return value;
    }


    private:
    template <class T>
    T get_(const std::string& name, const T* defval) const 
    {
        T value{};
        try 
        {
            std::stringstream ss{
                self.map.at( name ) 
            }; 
            ss >> value;    

        } catch(const std::out_of_range&) 
        {
            if ( defval == nullptr ) 
                throw std::invalid_argument{ 
                    std::string{ "parameter '" } 
                    + name + 
                    std::string{ "' is required but was not supplied!" }
                }; 
            
            value = *defval; 
        }
        return value; 
    }

    protected: 
    void insert(const std::string& param, const std::string& value) 
    {
        self.map[ param ] = value; 
        self.names.push_back( param );
    }
    

    friend std::ostream&  operator << (std::ostream& os, const Handler& ph) noexcept 
    {
        for (const auto& param : ph.names)
        {
            os << param;
            os << ": "; 
            os << ph.map.at( param ); 
            os << '\n';
        }
        return os; 
    }
}; 


/** simple class that parses CLI arguments and gives (via parameters::Handler) 
 * useful methods to get their values. 
 */
class CliArgsParser : public Handler
{
    public: 
    CliArgsParser(int argc, const char* argv[]) 
    {
        self.delimiter( "=" ); 

        for (auto aidx{ 1 }; aidx < argc; ++aidx) 
        {
            const auto arg = std::string( argv[ aidx ] );  

            const auto didx = arg.find( self.delimiter() ); 
            if ( didx == std::string::npos ) 
                continue; 
            
            const auto param = arg.substr( 0, didx ); 
            const auto value = arg.substr( didx + 1 ); 
            
            self.insert( param, value ); 
        }
    }

    CliArgsParser(const CliArgsParser&) = delete; 
    CliArgsParser& operator = (const CliArgsParser&) = delete; 
    CliArgsParser(CliArgsParser&&) = default; 
    CliArgsParser& operator = (CliArgsParser&&) = default; 
}; 
} // namespace parameters
#undef self
#endif // PARAMETERS_HPP_GUARD
