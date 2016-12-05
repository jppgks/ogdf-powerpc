/** \file
 * \brief Declaration of a constraint class for the Branch&Cut algorithm
 * for the Maximum C-Planar SubGraph problem.
 *
 * These constraints represent the planarity-constraints belonging to the
 * ILP formulation. These constraints are dynamically separated.
 * For the separation the planarity test algorithm by Boyer and Myrvold is used.
 *
 * \author Mathias Jansen
 *
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

#pragma once

#include <ogdf/planarity/BoyerMyrvold.h>
#include <ogdf/internal/cluster/Cluster_EdgeVar.h>
#include <ogdf/internal/cluster/basics.h>
//#include <ogdf/internal/cluster/MaxCPlanar_Master.h>
//#include <ogdf/lib/abacus/master.h>

#include <ogdf/lib/abacus/constraint.h>

namespace ogdf {


class ClusterKuratowskiConstraint : public abacus::Constraint {

public:

	ClusterKuratowskiConstraint(abacus::Master *master, int nEdges, SListPure<nodePair> &ks);

	virtual ~ClusterKuratowskiConstraint();

	// Computes and returns the coefficient for the given variable
	virtual double coeff(const abacus::Variable *v) const override;

	void printMe(ostream& out) const {
		out << "[KuraCon: ";
		for(const nodePair &p : m_subdivision) {
			p.printMe(out);
			out << ",";
		}
		out << "]";
	}

private:

	// The subdivision containing edges forming a SubGraph that is not planar
	List<nodePair> m_subdivision;

};

}
