/** \file
 * \brief Implementation of disjoint sets data structures (union-find functionality).
 *
 * \author Andrej Dudenhefner
 *
 * \par License:
 * This file is part of the Open Graph Drawing Framework (OGDF).
 *
 * \par
 * Copyright (C)<br>
 * See README.md in the OGDF root directory for details.
 *
 * \par
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 or 3 as published by the Free Software Foundation;
 * see the file LICENSE.txt included in the packaging of this file
 * for details.
 *
 * \par
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, see
 * http://www.gnu.org/copyleft/gpl.html
 */


#include <ogdf/basic/DisjointSets.h>

namespace ogdf {

const char *linkOptionNames[] = {"Naive Link","Link by Index","Link by Size","Link by Rank"};
const char *compressionOptionNames[] = {"Path Compression","Path Splitting","Path Halving","Reversal of Type 0","Reversal of Type 1","Collapsing","No Compression"};
const char *interleavingOptionNames[] = {"No Interleavintg","Rem Splicing","Tarjan and van Leeuwen","Interleaved Reversal of Type 0", "Interleaved Path Splitting Path Compression"};

}  // end namespace ogdf
