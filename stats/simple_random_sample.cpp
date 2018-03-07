// Copyright 2018 Google LLC. All Rights Reserved.
/*
  Copyright (C) 2005 Steven L. Scott

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

<<<<<<< HEAD
#include <stats/simple_random_sample.hpp>
#include <random>
=======
#include "stats/simple_random_sample.hpp"
>>>>>>> stable

namespace BOOM{
  std::vector<bool> SRS_indx(uint N, uint n){
    // generate a vector<bool> of length N such that n elements are true
    std::vector<bool> in(N, false);
    std::fill_n(in.begin(), n, true);
    std::shuffle(in.begin(), in.end(), std::default_random_engine());
    return in;
  }

}
