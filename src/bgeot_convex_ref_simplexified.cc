// -*- c++ -*- (enables emacs c++ mode)
//========================================================================
//
// Library : Basic GEOmetric Tool  (bgeot)
// File    : bgeot_convex_ref_simplexified.cc : simplexification of
//           convexes of reference
//           
// Date    : January 21, 2006.
// Author  : Yves Renard <Yves.Renard@insa-toulouse.fr>
//
//========================================================================
//
// Copyright (C) 2006-2006 Yves Renard
//
// This file is a part of GETFEM++
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; version 2 of the License.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
//========================================================================


#include <bgeot_convex_ref.h>


namespace bgeot {


  static size_type simplexified_parallelepiped_2[6] = {
    3,  0,  2,  3,  0,  1
  };

  static size_type simplexified_parallelepiped_2_nb = 2;

  static size_type simplexified_parallelepiped_3[24] = {
    3,  7,  0,  1,  7,  0,  5,  4,  7,  0,  1,  5,  3,  7,  0,  2,  6,  7,
    0,  4,  6,  7,  0,  2
  };

  static size_type simplexified_parallelepiped_3_nb = 6;

  static size_type simplexified_parallelepiped_4[80] = {
    9,  3,  1,  5,  0,  9, 12, 10,  8,  0,  9, 12,  5, 13, 15,  9,  3, 10,
    6,  0,  9,  3,  5,  6,  0,  9,  3, 10, 11, 15,  9, 12, 10,  6,  0,  9,
   12,  5,  6,  0,  9,  3, 10,  6, 15,  9,  3,  5,  6, 15,  9, 12, 10,  6,
   15,  9, 12,  5,  6, 15, 12,  4,  5,  6,  0,  3,  5,  6,  7, 15, 12, 10,
   14,  6, 15,  3, 10,  6,  2,  0
  };

  static size_type simplexified_parallelepiped_4_nb = 16;

  static size_type simplexified_parallelepiped_5[402] = {
   17,  9,  5,  3, 12,  0, 17,  5, 20,  3, 12,  0, 17,  9,  5,  3,  1,  0,
   17,  9,  5,  3, 12, 31, 17,  5, 20,  3, 12, 31, 17,  9, 29,  5, 12, 31,
   17, 29,  5, 20, 12, 31, 17, 29,  5, 20, 23, 31, 17,  5, 20,  3, 23, 31,
   17,  9,  3, 12, 18,  0, 17, 20,  3, 12, 18,  0, 17,  9, 29, 24, 12, 31,
   17, 29, 24, 20, 12, 31, 17,  9, 24,  3, 12, 31, 17, 29,  5, 20, 23, 21,
   17, 24, 20, 12, 18, 31, 17, 24,  3, 12, 18, 31, 17, 20,  3, 12, 18, 31,
   17, 20,  3, 23, 18, 31, 17,  3, 23, 18, 19, 31, 17,  9, 24, 12, 18,  0,
   17, 24, 20, 12, 18,  0, 17, 24,  3, 18, 27, 31, 17,  3, 18, 19, 27, 31,
   17, 24, 20, 18,  0, 16, 17,  9, 29, 24, 27, 31, 17,  9, 24,  3, 27, 31,
   17,  9, 29, 24, 27, 25,  9, 24,  3, 10, 27, 31,  9,  3, 10, 15, 27, 31,
    9,  3, 10, 15, 27, 11,  9, 29,  5, 12, 13, 15,  9, 24,  3, 10, 12, 31,
    9,  3, 10, 12, 15, 31,  9, 29,  5, 12, 15, 31,  9,  5,  3, 12, 15, 31,
    9, 24, 10, 12,  8,  0,  9, 24, 10, 12, 18,  0,  9,  3, 10, 12, 18,  0,
   29, 24, 20, 28, 12, 30, 29, 24, 20, 12, 31, 30,  5,  3, 23,  6, 15, 31,
    5,  3, 12,  6, 15, 31,  5, 20,  3, 23,  6, 31,  5, 20,  3, 12,  6, 31,
    5,  3, 23,  7,  6, 15,  5, 20,  3, 12,  6,  0,  5, 20, 12,  6,  0,  4,
   24,  3, 10, 12, 18, 31, 24,  3, 10, 18, 27, 31, 24, 10, 12, 18, 31, 30,
   24, 10, 18, 27, 31, 30, 24, 10, 18, 27, 26, 30, 24, 20, 12, 18, 31, 30,
   20, 23, 18,  6, 31, 30, 20, 12, 18,  6, 31, 30, 20,  3, 23, 18,  6, 31,
   20,  3, 12, 18,  6, 31, 20, 23, 18,  6, 22, 30, 20,  3, 12, 18,  6,  0,
    3, 10, 12, 18,  6, 31,  3, 10, 12,  6, 15, 31,  3, 10, 18,  6,  0,  2,
    3, 10, 12, 18,  6,  0, 10, 12,  6, 15, 14, 31, 10, 12, 18,  6, 31, 30,
   10, 12,  6, 14, 31, 30
  };

  static size_type simplexified_parallelepiped_5_nb = 67;

  static size_type simplexified_parallelepiped_6[2527] = {
   25, 17, 24, 59,  9, 63, 57, 25, 17, 24,  9, 29, 63, 57, 25, 17, 24, 59,
    9, 27, 63, 25, 17, 24,  9, 27, 63, 31, 25, 17, 24,  9, 29, 63, 31, 48,
   17, 24, 16, 18, 20,  0, 48, 17, 24, 40, 18, 20,  0, 48, 24, 58, 40, 63,
   18, 62, 48, 24, 40, 63, 18, 20, 62, 48, 34, 17, 40, 18, 20,  0, 48, 24,
   58, 60, 40, 63, 62, 48, 24, 60, 40, 63, 20, 62, 48, 34, 17, 40, 36, 20,
    0, 48, 17, 24, 40, 63, 57, 61, 48, 17, 24, 40, 63, 20, 61, 48, 24, 60,
   40, 63, 57, 61, 48, 24, 60, 40, 63, 20, 61, 48, 60, 55, 36, 20, 54, 62,
   48, 17, 24, 59, 40, 63, 18, 48, 17, 24, 40, 63, 18, 20, 48, 24, 59, 58,
   40, 63, 18, 48, 34, 58, 40, 63, 18, 62, 48, 34, 40, 63, 18, 20, 62, 48,
   34, 58, 63, 50, 18, 62, 48, 34, 63, 50, 18, 55, 62, 48, 34, 63, 18, 55,
   20, 62, 48, 34, 50, 18, 55, 54, 62, 48, 34, 18, 55, 20, 54, 62, 48, 34,
   55, 36, 20, 54, 62, 48, 17, 24, 59, 40, 63, 57, 48, 24, 59, 58, 60, 40,
   57, 48, 24, 58, 60, 40, 56, 57, 48, 24, 59, 60, 40, 63, 57, 48, 24, 59,
   58, 60, 40, 63, 48, 17, 40, 63, 36, 20, 61, 48, 17, 53, 55, 36, 20, 61,
   48, 60, 40, 63, 36, 20, 61, 48, 60, 55, 36, 20, 54, 52, 48, 60, 53, 55,
   36, 20, 61, 48, 60, 53, 55, 36, 20, 52, 48, 60, 40, 63, 36, 20, 62, 48,
   60, 63, 55, 36, 20, 62, 48, 34, 17, 59, 40, 63, 18, 48, 34, 17, 40, 63,
   18, 20, 48, 34, 59, 58, 40, 63, 18, 48, 34, 59, 58, 63, 50, 18, 48, 34,
   17, 33, 40, 36,  0, 48, 34, 32, 33, 40, 36,  0, 48, 34, 17, 40, 63, 36,
   20, 48, 34, 17, 63, 55, 36, 20, 48, 34, 40, 63, 36, 20, 62, 48, 34, 63,
   55, 36, 20, 62, 48, 34, 17, 59, 63, 18, 55, 48, 34, 17, 63, 18, 55, 20,
   48, 34, 59, 63, 50, 18, 55, 48, 17, 33, 40, 63, 57, 61, 48, 17, 33, 40,
   63, 36, 61, 48, 17, 33, 49, 63, 57, 61, 48, 17, 33, 49, 53, 55, 61, 48,
   17, 33, 53, 55, 36, 61, 48, 17, 63, 55, 36, 20, 61, 48, 60, 63, 55, 36,
   20, 61, 48, 34, 17, 33, 40, 63, 36, 48, 34, 17, 33, 63, 55, 36, 48, 17,
   59, 33, 49, 63, 57, 48, 17, 59, 33, 40, 63, 57, 48, 17, 33, 49, 63, 55,
   61, 48, 17, 33, 63, 55, 36, 61, 48, 34, 17, 59, 33, 40, 63, 48, 34, 17,
   59, 33, 63, 55, 48, 34, 17, 59, 18, 55, 51, 48, 34, 59, 50, 18, 55, 51,
   48, 17, 59, 33, 49, 63, 55, 48, 34, 17, 59, 33, 55, 51, 48, 17, 59, 33,
   49, 55, 51, 34, 17, 10,  6, 18, 20,  0, 34, 17, 10,  6, 36, 20,  0, 34,
    6, 18, 55, 20, 54, 62, 34,  6, 55, 36, 20, 54, 62, 34, 17, 33, 10,  6,
    3,  0, 34, 17, 10,  6,  3, 18,  0, 34, 17, 10, 63,  6, 18, 20, 34, 17,
   10, 63,  6, 36, 20, 34, 17, 40, 10, 18, 20,  0, 34, 17, 40, 10, 36, 20,
    0, 34, 17, 33, 10,  6, 36,  0, 34, 10, 63,  6, 18, 20, 62, 34, 10, 63,
    6, 36, 20, 62, 34, 10,  6,  3, 18,  2,  0, 34, 17, 63,  6, 18, 55, 20,
   34, 17, 63,  6, 55, 36, 20, 34, 63,  6, 18, 55, 20, 62, 34, 63,  6, 55,
   36, 20, 62, 34, 17, 33, 10, 63,  6,  3, 34, 17, 10, 63,  6,  3, 18, 34,
   10, 46, 63,  6, 36, 62, 34, 46, 63,  6, 38, 36, 62, 34,  6, 55, 38, 36,
   54, 62, 34, 17, 33, 10, 63,  6, 36, 34, 17, 40, 10, 63, 18, 20, 34, 17,
   40, 10, 63, 36, 20, 34, 17, 33, 40, 10, 36,  0, 34, 40, 10, 63, 18, 20,
   62, 34, 40, 10, 63, 36, 20, 62, 34, 17, 33, 63,  6, 55, 36, 34, 17, 33,
   63,  6,  3, 55, 34, 17, 63,  6,  3, 18, 55, 34, 17, 59, 33, 10, 63,  3,
   34, 17, 59, 10, 63,  3, 18, 34, 47, 10, 46, 63,  6, 36, 34, 47, 46, 63,
    6, 38, 36, 34, 47, 40, 10, 46, 63, 36, 34, 63,  6, 55, 38, 36, 62, 34,
   17, 59, 33, 40, 10, 63, 34, 17, 59, 40, 10, 63, 18, 34, 17, 33, 40, 10,
   63, 36, 34, 58, 40, 10, 63, 18, 62, 34, 58, 47, 40, 10, 46, 63, 34, 58,
   40, 10, 46, 63, 62, 34, 40, 10, 46, 63, 36, 62, 34, 33, 47, 10, 63,  6,
    3, 34, 33, 47, 63,  6,  3, 55, 34, 17, 59, 33, 63,  3, 55, 34, 17, 59,
   63,  3, 18, 55, 34, 33, 47, 10, 63,  6, 36, 34, 33, 47, 63,  6, 55, 36,
   34, 33, 47, 39,  6, 55, 36, 34, 47, 39,  6, 55, 38, 36, 34, 47, 63,  6,
   55, 38, 36, 34, 33, 47, 39,  6,  3, 55, 34, 58, 47, 40, 10, 46, 42, 34,
   59, 33, 47, 40, 10, 63, 34, 33, 47, 40, 10, 63, 36, 34, 59, 47, 40, 10,
   42, 43, 34, 59, 33, 47, 40, 10, 43, 34, 59, 33, 47, 10, 63,  3, 34, 59,
   33, 47, 63,  3, 55, 34, 59, 33, 47, 10,  3, 43, 34, 59, 33, 47,  3, 43,
   35, 34, 17, 59, 33,  3, 55, 51, 34, 17, 59,  3, 18, 55, 51, 34, 33, 47,
   39,  3, 55, 35, 34, 59, 58, 40, 10, 63, 18, 34, 59, 58, 47, 40, 10, 63,
   34, 59, 33, 47,  3, 55, 35, 34, 59, 58, 47, 40, 10, 42, 34, 59, 33,  3,
   55, 51, 35, 13, 15, 12,  9, 45,  5, 61, 13, 15, 12,  9, 29,  5, 61, 30,
   63,  6, 18, 20, 54, 62, 15, 12, 30, 10, 46,  6, 14, 15, 12, 47, 10, 46,
   63,  6, 15, 12, 30, 10, 46, 63,  6, 15, 12,  9, 45, 47,  5, 61, 15, 12,
   30, 10, 63,  6, 31, 15, 12,  9, 47, 63,  5, 61, 15, 12,  9, 29, 63,  5,
   61, 15, 12, 47, 10, 63,  6,  5, 15, 12, 10, 63,  6,  5, 31, 30, 63,  6,
   18, 31, 23, 20, 15, 59, 11,  9, 27, 10,  3, 15, 59,  9, 27, 10, 63,  3,
   15, 12,  9, 47, 10, 63,  5, 15, 12,  9, 10, 63,  5, 31, 15, 12,  9, 29,
   63,  5, 31, 15, 47, 39,  6,  5,  3, 23, 30,  6, 18, 22, 55, 20, 54, 15,
    7, 39,  6,  5,  3, 23, 30, 63,  6, 18, 55, 20, 54, 30,  6, 18, 22, 55,
   23, 20, 15, 47, 10, 63,  6,  5,  3, 30, 63,  6, 18, 55, 23, 20, 15, 10,
   63,  6,  5,  3, 31, 15, 59, 11,  9, 47, 10,  3, 15, 59,  9, 47, 10, 63,
    3, 15,  9, 27, 10, 63,  3, 31, 15,  9, 47, 10, 63,  5,  3, 15,  9, 10,
   63,  5,  3, 31, 15, 47, 63,  6,  5,  3, 23, 30, 10, 63,  6, 18, 20, 62,
   30, 10, 63,  6, 18, 31, 20, 15, 63,  6,  5,  3, 31, 23, 17,  9, 29, 63,
    5, 31, 20, 17, 29, 63,  5, 31, 23, 20, 17, 10, 63,  6,  5,  3, 31, 17,
   10, 63,  6,  5, 31, 20, 17, 24,  9, 27, 10, 63, 31, 17,  9, 27, 10, 63,
    3, 31, 17,  9, 10, 63,  5,  3, 31, 17,  9, 10, 63,  5, 31, 20, 17, 24,
   27, 10, 63, 18, 31, 17, 27, 10, 63,  3, 18, 31, 17, 63,  6,  5,  3, 31,
   23, 17, 63,  6,  5, 31, 23, 20, 17, 33,  9, 40, 63, 36, 61, 17,  9, 40,
   63, 36, 20, 61, 17, 33,  9, 10, 63,  5, 36, 17,  9, 10, 63,  5, 36, 20,
   17, 10, 63,  6,  3, 18, 31, 17, 10, 63,  6, 18, 31, 20, 17, 33, 10, 63,
    6,  5, 36, 17, 10, 63,  6,  5, 36, 20, 17, 24,  9, 40, 63, 57, 61, 17,
   33,  9, 40, 63, 57, 61, 17, 24,  9, 40, 63, 20, 61, 17, 24,  9, 29, 63,
   57, 61, 17, 24,  9, 29, 63, 20, 61, 17, 33,  9, 63,  5, 36, 61, 17,  9,
   63,  5, 36, 20, 61, 17,  9, 29, 63,  5, 20, 61, 17, 29, 63,  5, 21, 20,
   61, 17, 29, 63,  5, 21, 23, 20, 17, 24,  9, 29, 63, 31, 20, 17, 24, 59,
    9, 27, 10, 63, 17, 59,  9, 27, 10, 63,  3, 17, 24, 59, 27, 10, 63, 18,
   17, 59, 27, 10, 63,  3, 18, 17, 63,  6,  3, 18, 31, 23, 17, 63,  6, 18,
   31, 23, 20, 17, 33, 10, 63,  6,  5,  3, 17, 33,  9, 10, 63,  5,  3, 17,
   24,  9, 10, 63, 31, 20, 17, 24, 10, 63, 18, 31, 20, 17, 27, 63,  3, 18,
   31, 23, 17, 63,  5, 21, 55, 20, 61, 17, 33, 63,  5, 55, 36, 61, 17, 63,
    5, 55, 36, 20, 61, 17, 24, 59,  9, 40, 10, 63, 17, 59, 33,  9, 40, 10,
   63, 17, 24,  9, 40, 10, 63, 20, 17, 63,  6,  5,  3, 55, 23, 17, 63,  6,
    5, 55, 23, 20, 17, 63,  6,  3, 18, 55, 23, 17, 63,  6, 18, 55, 23, 20,
   17, 33,  9, 40, 10, 63, 36, 17,  9, 40, 10, 63, 36, 20, 17, 33, 63,  6,
    5,  3, 55, 17, 33, 63,  6,  5, 55, 36, 17, 63,  6,  5, 55, 36, 20, 17,
   33,  9, 10,  6, 36,  0, 17,  9, 10,  6, 36, 20,  0, 17, 24, 59,  9, 40,
   63, 57, 17, 59, 33,  9, 40, 63, 57, 17, 24, 59, 40, 10, 63, 18, 17, 59,
   33,  9, 10, 63,  3, 17, 33,  9, 10,  6,  3,  0, 17, 59, 27, 63,  3, 18,
   23, 17, 53,  5, 21, 55, 20, 61, 17, 63,  5, 21, 55, 23, 20, 17, 59, 63,
    3, 18, 55, 23, 17, 33, 53,  5, 55, 36, 61, 17, 53,  5, 55, 36, 20, 61,
   17, 24, 40, 10, 63, 18, 20, 17, 24,  9, 40, 10, 20,  0, 17, 33,  9, 40,
   10, 36,  0, 17,  9, 40, 10, 36, 20,  0, 17, 33,  9,  6,  5, 36,  0, 17,
    9,  6,  5, 36, 20,  0, 17, 33,  9,  6,  5,  3,  0, 17, 59, 19, 27,  3,
   18, 23, 17, 59, 19,  3, 18, 55, 23, 17, 24, 40, 10, 18, 20,  0, 17,  1,
   33,  9,  5,  3,  0, 17, 59, 19,  3, 18, 55, 51, 24, 59, 58, 27, 30, 10,
   18, 24, 58, 26, 27, 30, 10, 18, 24, 58, 30, 10, 63, 18, 62, 24, 30, 10,
   63, 18, 20, 62, 24, 27, 30, 10, 63, 18, 31, 24, 30, 10, 63, 18, 31, 20,
   24, 59, 27, 30, 10, 63, 18, 24, 59, 58, 30, 10, 63, 18, 24, 58, 40, 10,
   63, 18, 62, 24, 40, 10, 63, 18, 20, 62, 24, 12, 60, 30, 63, 20, 62, 24,
   12, 30, 10, 63, 20, 62, 24, 28, 12, 60, 30, 63, 20, 24, 28, 12, 30, 29,
   63, 20, 24, 59, 58, 40, 10, 63, 18, 24, 12,  9, 40, 10,  8,  0, 24, 12,
    9, 40, 10, 20,  0, 24, 12, 30, 10, 63, 31, 20, 24, 12, 30, 29, 63, 31,
   20, 24, 12, 60, 40, 63, 20, 62, 24, 12, 40, 10, 63, 20, 62, 24, 28, 12,
   60, 29, 63, 20, 24, 12,  9, 10, 63, 31, 20, 24, 12,  9, 29, 63, 31, 20,
   24, 12,  9, 40, 10, 63, 20, 24, 12, 60, 40, 63, 20, 61, 24, 12, 60, 29,
   63, 20, 61, 24, 12,  9, 40, 63, 20, 61, 24, 12,  9, 29, 63, 20, 61, 59,
   11,  9, 47, 10,  3, 43, 59, 33,  9, 47, 10, 63,  3, 59, 33,  9, 47, 10,
    3, 43, 59, 33,  9, 47, 40, 10, 63, 59, 33,  9, 47, 40, 10, 43, 59, 33,
    9, 47, 40, 41, 63, 59, 33,  9, 47, 40, 41, 43, 59, 33,  9, 40, 41, 63,
   57, 12,  4,  6,  5, 36, 20,  0, 12, 10, 46, 63,  6, 36, 62, 12, 10, 63,
    6, 36, 20, 62, 12, 30, 10, 46, 63,  6, 62, 12, 30, 10, 63,  6, 20, 62,
   12,  9, 10,  6, 36, 20,  0, 12,  9,  6,  5, 36, 20,  0, 12, 60, 47, 44,
   40, 46, 36, 12, 47, 10, 46, 63,  6, 36, 12, 60, 40, 46, 63, 36, 62, 12,
   40, 10, 46, 63, 36, 62, 12, 30, 10, 63,  6, 31, 20, 12,  9, 40, 10, 36,
   20,  0, 12, 47, 10, 63,  6,  5, 36, 12, 10, 63,  6,  5, 36, 20, 12, 10,
   63,  6,  5, 31, 20, 12, 60, 47, 40, 46, 63, 36, 12, 47, 40, 10, 46, 63,
   36, 12, 60, 40, 63, 36, 20, 62, 12, 40, 10, 63, 36, 20, 62, 12, 60, 45,
   47, 44, 40, 36, 12,  9, 47, 40, 10, 63, 36, 12,  9, 47, 10, 63,  5, 36,
   12,  9, 40, 10, 63, 36, 20, 12,  9, 10, 63,  5, 36, 20, 12,  9, 40, 63,
   36, 20, 61, 12,  9, 63,  5, 36, 20, 61, 12, 47, 40, 63,  5, 36, 61, 12,
   45, 47, 40,  5, 36, 61, 12,  9, 10, 63,  5, 31, 20, 12,  9, 29, 63,  5,
   31, 20, 12, 60, 47, 40, 63, 36, 61, 12, 60, 40, 63, 36, 20, 61, 12, 60,
   45, 47, 40, 36, 61, 12,  9, 47, 40, 63,  5, 61, 12,  9, 45, 47, 40,  5,
   61, 12,  9, 29, 63,  5, 20, 61, 33, 47, 10, 63,  6,  5,  3, 33, 47, 10,
   63,  6,  5, 36, 33, 47, 39,  6,  5,  3, 55, 33, 47, 39,  6,  5, 55, 36,
   33,  9, 47, 10, 63,  5,  3, 33,  9, 47, 10, 63,  5, 36, 33,  9, 47, 40,
   10, 63, 36, 33,  9, 45, 47, 40,  5, 61, 33, 45, 47, 40,  5, 36, 61, 33,
   47, 63,  6,  5,  3, 55, 33, 47, 63,  6,  5, 55, 36, 33, 53,  5, 37, 55,
   36, 61, 33, 47, 39,  5, 37, 55, 36, 33, 45, 47,  5, 37, 36, 61, 33,  9,
   45, 47, 40, 41, 61, 33,  9, 47, 40, 63,  5, 61, 33, 47, 40, 63,  5, 36,
   61, 33,  9, 40, 41, 63, 57, 61, 33, 47,  5, 37, 55, 36, 61, 33,  9, 47,
   40, 41, 63, 61, 33, 47, 63,  5, 55, 36, 61, 47, 39,  6,  5,  3, 55, 23,
   47, 63,  6,  5,  3, 55, 23
  };

  static size_type simplexified_parallelepiped_6_nb = 361;

  static size_type simplexified_prism_3[12] = {
    2,  4,  0,  1,  2,  4,  0,  3,  2,  4,  5,  3
  };

  static size_type simplexified_prism_3_nb = 3;

  static size_type simplexified_prism_4[20] = {
    6,  1,  3,  7,  0,  6,  1,  3,  2,  0,  6,  1,  4,  7,  0,  6,  1,  4,
    7,  5
  };

  static size_type simplexified_prism_4_nb = 4;

  static size_type simplexified_prism_5[30] = {
    4,  8,  1,  9,  5,  2,  8,  1,  7,  9,  5,  6,  4,  8,  1,  5,  3,  2,
    4,  1,  0,  5,  3,  2,  8,  1,  7,  9,  5,  2
  };

  static size_type simplexified_prism_5_nb = 5;

  static size_type simplexified_prism_6[42] = {
    1,  6,  2,  4,  5,  3,  0,  1,  6,  2,  9,  4,  5,  3,  1,  6,  2,  9,
    4,  5,  8,  1,  6,  9,  4,  5,  8,  7,  6, 11,  9,  4,  5,  8,  7,  6,
   11,  9,  4, 10,  8,  7
  };

  static size_type simplexified_prism_6_nb = 6;



  size_type simplexified_tab(pconvex_structure cvs,
                             size_type **tab) {
    if (cvs == parallelepiped_structure(2)) {
      *tab = simplexified_parallelepiped_2;
      return simplexified_parallelepiped_2_nb;
    }

    if (cvs == parallelepiped_structure(3)) {
      *tab = simplexified_parallelepiped_3;
      return simplexified_parallelepiped_3_nb;
    }

    if (cvs == parallelepiped_structure(4)) {
      *tab = simplexified_parallelepiped_4;
      return simplexified_parallelepiped_4_nb;
    }

    if (cvs == parallelepiped_structure(5)) {
      *tab = simplexified_parallelepiped_5;
      return simplexified_parallelepiped_5_nb;
    }

    if (cvs == parallelepiped_structure(6)) {
      *tab = simplexified_parallelepiped_6;
      return simplexified_parallelepiped_6_nb;
    }

    if (cvs == prism_structure(3)) {
      *tab = simplexified_prism_3;
      return simplexified_prism_3_nb;
    }

    if (cvs == prism_structure(4)) {
      *tab = simplexified_prism_4;
      return simplexified_prism_4_nb;
    }

    if (cvs == prism_structure(5)) {
      *tab = simplexified_prism_5;
      return simplexified_prism_5_nb;
    }

    if (cvs == prism_structure(6)) {
      *tab = simplexified_prism_6;
      return simplexified_prism_6_nb;
    }

    DAL_THROW(failure_error, "No simplexification  for this element");
  }

}