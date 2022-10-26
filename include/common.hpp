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

#ifndef SOM_COMMON_HPP_GUARD 
#define SOM_COMMON_HPP_GUARD


#include <memory>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <cassert>
#include <cstring>
#include <cmath>
#include <ctime>
#include <climits>
#include <chrono>

#include "myprint.hpp"
#include "parameters.hpp"


/** @brief Handy macro that generates a getter method for the given field */
#define getter(field_, type_, desc_) \
/** @brief Returns the current value of field_ desc_ */ \
const type_ & field_ () const noexcept \
{ return self.m ## field_ ; } \

/** @brief Handy macro that generates a setter method for the given field */
#define setter(field_, type_, desc_) \
/** @brief Sets the new value of field_ desc_ */ \
void field_ (const type_ & value) noexcept \
{ self.m ## field_ = value; } \

/** @brief Handy macro that generates a fluent setter method for the given field */
#define fsetter(class_, field_, type_, desc_) \
/** @brief Sets the new value of field_ desc_ */ \
class_ & field_ (const type_ & value) noexcept \
{ \
    self.m ## field_ = value; \
    return (*this); \
} \


/** @brief Handy macro that generates both a getter and a setter methods for the given field */
#define getsetter(field_, type_, desc_) \
getter( field_, type_, desc_ ) \
setter( field_, type_, desc_ ) 

/** @brief Handy macro that generates both a getter and a fluent setter methods for the given field */
#define getfsetter(class_, field_, type_, desc_) \
getter( field_, type_, desc_ ) \
fsetter( class_, field_, type_, desc_ ) 


#endif // SOM_COMMON_HPP_GUARD
