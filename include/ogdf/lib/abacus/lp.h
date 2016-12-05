/*!\file
 * \author Matthias Elf
 * \brief linear program.
 *
 * \par License:
 * This file is part of ABACUS - A Branch And CUt System
 * Copyright (C) 1995 - 2003
 * University of Cologne, Germany
 *
 * \par
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * \par
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * \see http://www.gnu.org/copyleft/gpl.html
 */

#pragma once

#include <ogdf/lib/abacus/optsense.h>
#include <ogdf/lib/abacus/lpvarstat.h>
#include <ogdf/lib/abacus/slackstat.h>
#include <ogdf/basic/Stopwatch.h>


namespace abacus {

class Master;
class SparVec;
class Row;
class Column;


//! Linear programs.
/**
 * The following section provides a generic interface class to linear
 * programs, from which we will derive further classes both
 * for the solution of LP-relaxations (LpSub) with a linear-programming based branch-and-bound algorithm
 * and for interfaces to LP-solvers (OsiIF).
 *
 * The framework should be very flexible in the use of different
 * LP-solvers. Therefore, we implement in the class LP a very general
 * interface to the linear program. All functions of the framework
 * communicate with the linear program only by the public functions
 * of the class LP.
 *
 * The public functions call pure virtual functions
 * starting with the prefix \a _, which have to be implemented in the
 * derived class for each specific LP-solver.
 *
 * Linear programs cannot only be used for solving the LP-relaxation
 * within the branch-and-cut algorithm. There are also techniques in
 * integer programming where linear programming is used for
 * generating cutting planes and for applying heuristics.
 * Therefore, we design the class LP that it can be used very
 * generally.
 */
class OGDF_EXPORT LP : public AbacusRoot {
public:

	//! The optimization status of the linear program.
	enum OPTSTAT {
		Optimal,		/*!< The optimal solution has been computed. */
		Unoptimized,	/*!< Unoptimized Optimization is still required, this is
						 *   also the case for reoptimization. */
		Error,			/*!< An error has happened during optimization. */
		Feasible,		/*!< A primal feasible solution for the linear program,
						 *   but not the optimal solution has been found. */
		Infeasible,		/*!< The linear program is primal infeasible. */
		Unbounded,		/*!< The linear program is unbounded. */
		LimitReached	/*!< The iteration limit was reached while optimizing. */
	};

	//! Describes if parts of the solution like x-values, reduced costs, etc. are available.
	enum SOLSTAT {
		Available,	/*!< The part of the solution is available. */
		Missing		/*!< Missing The part of the solution is missing. */
	};

	//! The solution method for the linear program.
	enum METHOD {
		Primal,				 /*!< The primal simplex method. */
		Dual,				 /*!< The dual simplex method. */
		BarrierAndCrossover, /*!< The barrier method followed by a crossover to a basis. */
		BarrierNoCrossover,	 /*!< The barrier method without crossover. */
		Approximate			 /*!< An approximative solver. */
	};

	//! Creates a linear program.
	/**
	 * \param master A pointer to the corresponding master of the optimization.
	 */
	LP (Master *master) :
		master_(master),
		optStat_(Unoptimized),
		xValStatus_(Missing),
		barXValStatus_(Missing),
		yValStatus_(Missing),
		recoStatus_(Missing),
		slackStatus_(Missing),
		basisStatus_(Missing),
		nOpt_(0)
	{ }

	//! The destructor.
	virtual ~LP () { }

	/*! \brief The output operator
	 *  writes the objective function, followed by
	 *  the constraints, the bounds on the columns and the solution values
	 *  (if available) to an output stream.
	 *
	 * Every ten output columns we perform a line break for better
	 * readability. This has also the advantage that LP-solvers with
	 * an input function requiring a limited length of a line (e.g., Cplex
	 * 255 characters) have a higher chance to read a file generated by
	 * this output operator.
	 *
	 * \return A reference to the output stream.
	 *
	 * \param out The output stream.
	 * \param rhs The linear program being output.
	 */
	friend ostream &operator<<(ostream& out, const LP& rhs);

	/*! \brief Loads the linear program defined by its arguments.
	 *
	 * We do not perform the initialization via arguments of a constructor,
	 * since for the most frequent application of linear programs within
	 * ABACUS, the solution of the linear programming relaxations in
	 * the subproblems, the problem data is preprocessed before it is
	 * loaded. Only after the preprocessing in the constructor of the derived
	 * class, we can call \a initialize().
	 *
	 * Of course, it would be possible to provide an extra constructor
	 * with automatic initialization if required.
	 *
	 * \param sense     The sense of the objective function.
	 * \param nRow      The number of rows.
	 * \param maxRow    The maximal number of rows.
	 * \param nCol      The number of columns (variables).
	 * \param maxCol    The maximal number of columns.
	 * \param obj       An array with the objective function coefficients.
	 * \param lBound    An array with the lower bounds of the columns.
	 * \param uBound    An array with the upper bounds of the columns.
	 * \param rows      An array storing the rows of the problem.
	 */
	void initialize(
		OptSense sense,
		int nRow,
		int maxRow,
		int nCol,
		int maxCol,
		Array<double> &obj,
		Array<double> &lBound,
		Array<double> &uBound,
		Array<Row*> &rows) {
			_initialize(sense, nRow, maxRow, nCol, maxCol, obj, lBound, uBound, rows);
	}

	/*! \brief This version of the function \a initialize()
	 *   performs like its previous version, but also initializes the basis with the
	 *   arguments:
	 *
	 * \param sense     The sense of the objective function.
	 * \param nRow      The number of rows.
	 * \param maxRow    The maximal number of rows.
	 * \param nCol      The number of columns (variables).
	 * \param maxCol    The maximal number of columns.
	 * \param obj       An array with the objective function coefficients.
	 * \param lBound    An array with the lower bounds of the columns.
	 * \param uBound    An array with the upper bounds of the columns.
	 * \param rows      An array storing the rows of the problem.
	 * \param lpVarStat An array storing the status of the columns.
	 * \param slackStat An array storing the status of the slack variables.
	 */
	void initialize(
		OptSense sense,
		int nRow, int maxRow,
		int nCol, int maxCol,
		Array<double> &obj,
		Array<double> &lBound,
		Array<double> &uBound,
		Array<Row*> &rows,
		Array<LPVARSTAT::STATUS> &lpVarStat,
		Array<SlackStat::STATUS> &slackStat) {
			_initialize(sense, nRow, maxRow, nCol, maxCol, obj, lBound, uBound, rows);
			loadBasis(lpVarStat,slackStat);
	}


	/*! Loads a new basis for the linear program.
	 *
	 * \param lpVarStat An array storing the status of the columns.
	 * \param slackStat An array storing the status of the slack variables.
	 */
	virtual void loadBasis(
		Array<LPVARSTAT::STATUS> &lpVarStat,
		Array<SlackStat::STATUS> &slackStat) {
			_loadBasis(lpVarStat,slackStat);
	}

	OptSense sense() const { return _sense(); }

	void sense(const OptSense &newSense) { _sense(newSense); }

	int nRow() const { return _nRow(); }

	int maxRow() const { return _maxRow(); }

	int nCol() const { return _nCol(); }

	int maxCol() const { return _maxCol(); }

	int nnz() const { return _nnz(); }

	double obj(int i) const {
#ifdef OGDF_DEBUG
		colRangeCheck(i);
#endif
		return _obj(i);
	}

	double lBound(int i) const {
#ifdef OGDF_DEBUG
		colRangeCheck(i);
#endif
		return _lBound(i);
	}

	double uBound(int i) const {
#ifdef OGDF_DEBUG
		colRangeCheck(i);
#endif
		return _uBound(i);
	}

	void row(int i, Row &r) const {
#ifdef OGDF_DEBUG
		rowRangeCheck(i);
#endif
		_row(i, r);
	}

	double rhs(int i) const {
#ifdef ABACUSSAFE
		rowRangeCheck(i);
#endif
		return _rhs(i);
	}

	virtual double value() const { return _value(); }

	virtual double xVal(int i) const {
#ifdef OGDF_DEBUG
		colRangeCheck(i);
#endif
		return _xVal(i);
	}

	virtual double barXVal(int i) const {
#ifdef OGDF_DEBUG
		colRangeCheck(i);
#endif
		return _barXVal(i);
	}

	virtual double reco(int i) const {
#ifdef OGDF_DEBUG
		colRangeCheck(i);
#endif
		return _reco(i);
	}

	virtual double yVal(int c) const {
#ifdef OGDF_DEBUG
		rowRangeCheck(c);
#endif
		return _yVal(c);
	}

	virtual double slack(int c) const {
#ifdef OGDF_DEBUG
		rowRangeCheck(c);
#endif
		return _slack(c);
	}

	SOLSTAT xValStatus() const { return xValStatus_; }

	SOLSTAT barXValStatus() const { return barXValStatus_; }

	SOLSTAT yValStatus() const { return yValStatus_; }

	SOLSTAT recoStatus() const { return recoStatus_; }

	SOLSTAT slackStatus() const { return slackStatus_; }

	SOLSTAT basisStatus() const { return basisStatus_; }

	int nOpt() const { return nOpt_; }

	virtual bool infeasible() const {
		return ( optStat_ == Infeasible );
	}


	/*! \brief Can be called if the last linear program
	 *   has been solved with the dual simplex method and is infeasible and
	 *   all inactive variables price out correctly.
	 *
	 * Then, the basis is dual
	 * feasible, but primal infeasible, i.e., some variables or slack variables
	 * violate their bounds.
	 * In this case the function \a getInfeas() determines an infeasible variable
	 * or slack variable.
	 *
	 * If \a getInfeas() is successful, then either \a infeasRow or \a infeasVar is
	 *   \f$-1\f$ and the other argument holds the nonnegative number of the infeasible
	 *   variable.
	 *
	 * \return 0 On success,
	 * \return 1 otherwise.
	 *
	 * \param infeasRow Holds after the execution the number of an infeasible
	 *                       slack variable, or \f$-1\f$.
	 * \param infeasCol Holds after the execution the number of an infeasible
	 *                      column, or \f$-1\f$.
	 * \param bInvRow Holds after the execution the row of the basis inverse
	 *                    corresponding to the infeasible column or
	 *                    slack variable, which
	 *                    is always a basic variable.
	 */
	virtual int getInfeas(int &infeasRow, int &infeasCol, double *bInvRow) const {
		return _getInfeas(infeasRow,infeasCol,bInvRow);
	}

	virtual LPVARSTAT::STATUS lpVarStat(int i) const {
#ifdef OGDF_DEBUG
		colRangeCheck(i);
#endif
		return _lpVarStat(i);
	}

	virtual SlackStat::STATUS slackStat(int i) const {
#ifdef OGDF_DEBUG
		rowRangeCheck(i);
#endif
		return _slackStat(i);
	}

	/*! \brief Performs the optimization of the linear program.
	 *
	 * \return The status of the optimization.
	 *
	 * \param method The method with which the optimization is performed.
	 */
	virtual OPTSTAT optimize(METHOD method);

	/*! \brief Removes rows of the linear program.
	 *
	 * \param ind The numbers of the rows that should be removed.
	 */
	void remRows(ArrayBuffer<int> &ind) {
		initPostOpt();
		_remRows(ind);
	}

	/*! \brief Adds rows to the linear program.
	 *
	 * If the new number of rows exceeds the maximal number of rows a reallocation
	 * is performed.
	 *
	 * \param newRows The rows that should be added to the linear program.
	 */
	void addRows(ArrayBuffer<Row*> &newRows);

	/*! \brief Removes  columns from the linear program.
	 *
	 * \param cols The numbers of the columns that should be removed.
	 */
	void remCols(ArrayBuffer<int> &cols) {
		initPostOpt();
		_remCols(cols);
	}

	/*! \brief Adds columns to the linear program.
	 *
	 * If the new number of columns exceeds the maximal number
	 * of columns a reallocation is performed.
	 *
	 * \param newCols The new columns that are added.
	 */
	void addCols(ArrayBuffer<Column*> &newCols);

	/*! \brief Changes the complete right hand side of the linear program.
	 *
	 * \param newRhs The new right hand side of the rows.
	 */
	void changeRhs(Array<double> &newRhs) {
		initPostOpt();
		_changeRhs(newRhs);
	}

	/*! \brief Changes the lower bound of a single column.
	 *
	 * \param i The column.
	 * \param newLb The new lower bound of the column.
	 */
	virtual void changeLBound(int i, double newLb);

	/*! \brief Changes the upper bound of a single column.
	 *
	 * \param i The column.
	 * \param newUb The new upper bound of the column.
	 */
	virtual void changeUBound(int i, double newUb);

	/*! \brief Pivots the slack variables  stored in the buffer \a rows into the basis.
	 *
	 * \return 0 All variables could be pivoted in,
	 * \return 1 otherwise.
	 *
	 * \param rows The numbers of the slack variables that should be
	 *                 pivoted in.
	 */
	virtual int pivotSlackVariableIn(ArrayBuffer<int> &rows);

	/*! \brief Performs a reallocation of the row space of the linear program.
	 *
	 * \param newSize The new maximal number of rows of the linear program.
	 */
	void rowRealloc(int newSize) {
		_rowRealloc(newSize);
	}

	/*! \brief Performs a reallocation of the column space of the linear program.
	 *
	 * \param newSize The new maximal number of columns of the linear program.
	 */
	void colRealloc(int newSize) {
		_colRealloc(newSize);
	}


	/*! \brief Writes the complete basis
	 *   of an optimal linear program to a file.
	 *
	 * \return 0 If a basis is available and could be written,
	 * \return 1 otherwise.
	 *
	 * \param fileName The name of the file the basis is written to.
	 */
	int writeBasisMatrix(const char *fileName);

	/*! \brief Changes the iteration limit of the Simplex algorithm.
	 *
	 * \return 0 If the iteration limit could be set,
	 * \return 1 otherwise.
	 *
	 * \param limit The new value of the iteration limit.
	 */
	int setSimplexIterationLimit(int limit) {
		return _setSimplexIterationLimit(limit);
	}

	/*!
	 * \return 0 If the iteration limit could be get,
	 * \return 1 otherwise.
	 *
	 * \param limit Stores the iteration limit if the  return value is 0.
	 */
	int getSimplexIterationLimit(int &limit) const {
		return _getSimplexIterationLimit(limit);
	}

	ogdf::StopwatchCPU* lpSolverTime() { return &lpSolverTime_; }

protected:

	/*! \brief Computes the number of nonzero elements in each column of a given set of rows.
	 *
	 * \param nRow The number of rows.
	 * \param rows The array storing the rows.
	 * \param nnz An array of length at least the number of columns
	 *                   of the linear program which will
	 *                   hold the number of nonzero elements of each column.
	 */
	void colsNnz(int nRow, Array<Row*> &rows, Array<int> &nnz);

	/*! \brief Computes the columnwise representation of the row matrix.
	 *
	 * \param nRow The number of rows.
	 * \param rows The array storing the rows.
	 * \param cols An array holding pointers to sparse vectors which
	 *                 will contain the columnwise representation of
	 *                 the constraint matrix defined by \a rows. The
	 *                 length of this array must be at least the number
	 *                 of columns. The elements of the array must not be
	 *                 0-pointers. Sparse vectors of sufficient length
	 *                 should be allocated before the function is called.
	 *                 The size of these sparse vectors can be
	 *                 determined with the function \a colsNnz().
	 */
	void rows2cols(int nRow, Array<Row*> &rows,
		Array<SparVec*> &cols);

	/*! \brief Terminates the program if there is no row with index \a r.
	 *
	 * \param r The number of a row of the linear program.
	 */
	void rowRangeCheck(int r) const;

	/*! \brief Terminates the program if there is no column with index \a i.
	 *
	 * \param i The number of a column.
	 */
	void colRangeCheck(int i) const;

	/*! \brief The pure virtual function \a _sense() must be defined by the
	 *  used LP-solver and return the sense of the optimization.
	 */
	virtual OptSense _sense() const = 0;
	virtual void _sense(const OptSense &newSense) = 0;

	/*! \brief The pure virtual function \a _nRow() must be defined by the
	 *  used LP-solver and return the number of rows of the problem
	 */
	virtual int      _nRow()  const = 0;

	/*! \brief The pure virtual function \a _maxRow() must be defined by the used
	 *  LP-solver and return the maximal number of rows.
	 */
	virtual int      _maxRow() const = 0;

	/*! \brief The pure virtual function \a _nCol() must be defined by the used
	 *  LP-solver and return the number of columns.
	 */
	virtual int      _nCol() const = 0;

	/*! \brief The pure virtual function \a _maxCol() must be defined by the
	 *  the used LP-solver and return the maximal number of columns.
	 */
	virtual int      _maxCol() const = 0;

	/*! \brief The pure virtual function \a _nnz() must be defined by the
	 *  used LP-solver and return the number of nonzero elements of the
	 *  constraint matrix not including the right hand side and the
	 *  bounds of the variables.
	 */
	virtual int      _nnz() const = 0;

	/*! \brief The pure virtual function \a _obj() must be defined by the
	 *  used LP-solver and return the objective function coefficient
	 *  of variable \a i.
	 */
	virtual double   _obj(int i) const = 0;

	/*! \brief The pure virtual function \a _lBound() must be defined by the
	 *  used LP-solver and return the lower bound of variable \a i.
	 */
	virtual double   _lBound(int i) const = 0;

	/*! \brief The pure virtual function \a _uBound() must be defined by the
	 *  used LP-solver and return the upper bound of variable \a i.
	 */
	virtual double   _uBound(int i) const = 0;

	/*! \brief The pure virtual function \a _rhs() must be defined by the
	 *  used LP-solver and return the right hand side of constraint \a i.
	 */
	virtual double   _rhs(int i) const = 0;

	/*!  The pure virtual function \a _row() must be defined by the used LP-solver
	 *  and store the \a i-th row of the problem in the row \a r.
	 */
	virtual void     _row(int i, Row &r) const = 0;

	/*! \brief The pure virtual function \a _initialize() must be defined by the used
	 *  LP-solver and should initialize the LP-solver with
	 *
	 * \param sense The sense of the optimization.
	 * \param nRow The number of rows.
	 * \param maxRow The maximal number of rows.
	 * \param nCol The number of columns.
	 * \param maxCol The maximal number of columns.
	 * \param obj An array with the objective functions coefficients.
	 * \param lBound An array with the lower bounds of the variables.
	 * \param uBound An array with the upper bounds of the variables.
	 * \param rows An array storing the constraint matrix in row format.
	 */
	virtual void    _initialize(OptSense sense, int nRow, int maxRow,
		int nCol, int maxCol,
		Array<double> &obj, Array<double> &lBound,
		Array<double> &uBound, Array<Row*> &rows) = 0;

	/*! This pure virtual function should load a basis into the LP-solver.
	 *
	 *  \param lpVarStat An array storing the status of the variables.
	 *  \param slackStat An array storing the status of the slack variables.
	 */
	virtual void _loadBasis(Array<LPVARSTAT::STATUS> &lpVarStat,
		Array<SlackStat::STATUS> &slackStat) = 0;

	/*! \brief The pure virtual function \a _primalSimplex() must be defined
	 *   by the used LP-solver and should call the
	 *   primal simplex method of the used LP-solver.
	 */
	virtual OPTSTAT _primalSimplex()                   = 0;

	/*! \brief The pure virtual function \a _dualSimplex() must be defined by the
	 *   used LP-solver and should call the dual simplex
	 *   method of the used LP-solver.
	 */
	virtual OPTSTAT _dualSimplex()                     = 0;

	/*! \brief The pure virtual function \a _barrier() must be defined by the used
	 *  LP-solver and should call the barrier method
	 *  of the used LP-solver.
	 */
	virtual OPTSTAT _barrier(bool doCrossover)         = 0;

	/*! \brief The pure virtual function \a _approx() must be defined by the used
	 *  LP-solver and should call the approximative method
	 *  of the used LP-solver.
	 */
	virtual OPTSTAT _approx()			         = 0;

	/*! \brief The pure virtual function \a _value() must be defined by the used
	 *  LP-solver and should return the optimum value of the linear program
	 *  after it has been solved.
	 */
	virtual double _value() const = 0;

	/*! \brief The pure virtual function \a _xVal() must be defined by the used
	 *  LP-solver and should return the value of variable \a i in the
	 *  LP-solution.
	 */
	virtual double _xVal(int i) const  = 0;
	virtual double _barXVal(int i) const  = 0;

	/*! \brief The pure virtual function \a _reco() must be defined by the used
	 *  LP-solver and should return the reduced cost of variable \a i.
	 */
	virtual double _reco(int i) const  = 0;

	/*! \brief The pure virtual function \a _slack() must be defined by the used
	 *  LP-solver and should return the value of the slack variable \a i.
	 */
	virtual double _slack(int i) const = 0;

	/*! \brief The pure virtual function \a _yVal() must be defined by the used
	 *  LP-solver and should return the value of the dual variable of
	 *  the constraint \a i.
	 */
	virtual double _yVal(int i) const  = 0;

	/*! \brief The pure virtual function \a _lpVarStat() must be defined by the used
	 *  LP-solver and should return the status of the variable \a i in the
	 *  LP-solution.
	 */
	virtual LPVARSTAT::STATUS _lpVarStat(int i) const = 0;

	/*! \brief The pure virtual function \a _slackStat() must be defined by the used
	 *  LP-solver and should return the status of the slack variable \a i in
	 *  the LP-solution.
	 */
	virtual SlackStat::STATUS _slackStat(int i) const = 0;

	/*! \brief The pure virtual function \a _getInfeas() must be defined by the used
	 *  LP-solver and can be called if the last linear program
	 *  has been solved with the dual simplex method and is infeasible.
	 *
	 * In this case it should compute the infeasible basic variable or constraint
	 * and the corresponding row \a bInvRow of the basis inverse.
	 * Either \a infeasRow or \a infeasCol is nonnegative. The nonnegative
	 * argument is an infeasible row or column, respectively.
	 *
	 * \return 0 if it is successful
	 * \return 1 otherwise.
	 */
	virtual int _getInfeas(int &infeasRow, int &infeasCol,
		double *bInvRow) const = 0;

	/*! \brief The pure virtual function \a _remRows() must be defined by the used
	 *  LP-solver and should remove
	 *  the rows with numbers given in the buffer \a ind from the LP-solver.
	 */
	virtual void _remRows(ArrayBuffer<int> &ind)         = 0;

	/*! \brief The pure virtual function \a _addRows() must be defined by the used
	 *  LP-solver and should add the rows given in the buffer \a newRows
	 *  to the LP.
	 */
	virtual void _addRows(ArrayBuffer<Row*> &newRows)    = 0;

	/*! \brief The pure virtual function \a _remCols() must be defined by the used
	 *  LP-solver and should remove the columns with
	 *  numbers given in \a vars from the LP.
	 */
	virtual void _remCols(ArrayBuffer<int> &vars)        = 0;

	/*! \brief The pure virtual function \a _addCols() must be defined
	 *  by the used LP-solver and should add the columns
	 *  \a newCols to the LP.
	 */
	virtual void _addCols(ArrayBuffer<Column*> &newCols) = 0;

	/*! \brief The pure virtual function \a _changeRhs() must be defined by the used
	 *  LP-solver and should set the right hand
	 *  side of the constraint matrix of the LP to \a newRhs.
	 */
	virtual void _changeRhs(Array<double> &newRhs)  = 0;

	/*! \brief The pure virtual function \a _changeLBound() must be defined
	 *  by the used LP-solver and should set the lower bound of variable \a i
	 *  to \a newLb.
	 */
	virtual void _changeLBound(int i, double newLb) = 0;

	/*! \brief The pure virtual function \a _changeLBound() must be defined
	 *  by the used LP-solver and should set the upper bound of variable \a i
	 *  to \a newUb.
	 */
	virtual void _changeUBound(int i, double newUb) = 0;

	/*! \brief The function \a pivotSlackVariableIn() pivots the slack variables
	 *  stored in the buffer \a rows into the basis.
	 *
	 * \return 0 All variables could be pivoted in,
	 * \return 1 otherwise.
	 *
	 * \param rows The numbers of the slack variables that should be
	 *             pivoted in.
	 */
	virtual int _pivotSlackVariableIn(ArrayBuffer<int> &rows) = 0;

	/*! \brief The pure virtual function \a _rowRealloc() must be defined in the used
	 *  LP-solver and should reallocate
	 *  its memory such that up to \a newSize rows can
	 *  be handled.
	 */
	virtual void _rowRealloc(int newSize) = 0;

	/*! \brief The pure virtual function \a _colRealloc() must be defined by the used
	 *  LP-solver and should reallocate its memory such that up to \a newSize
	 *  columns can be handled.
	 */
	virtual void _colRealloc(int newSize) = 0;

	/*! \brief The function \a setSimplexIterationLimit() changes the iteration
	 *  limit of the Simplex algorithm.
	 *
	 * \return 0 If the iteration limit could be set,
	 * \return 1 otherwise.
	 *
	 * \param limit The new value of the iteration limit.
	 */
	virtual int _setSimplexIterationLimit(int limit) = 0;

	/*! \brief The function \a getSimplexIterationLimit() retrieves the value
	 *  of the iteration limit of the simplex algorithm.
	 *
	 * \return 0 If the iteration limit could be get,
	 * \return 1 otherwise.
	 *
	 * \param limit Stores the value of the iteration limit if the
	 *               function returns 0.
	 */
	virtual int _getSimplexIterationLimit(int &limit) const = 0;

	/*! A pointer to the corresponding master of the optimization.
	 */
	Master *master_;

	/*! The status of the linear program.
	 */
	OPTSTAT optStat_;

	/*! \brief This member becomes \a Available if the \f$x\f$-values of the optimal
	 *  solution can be accessed with the function \a xVal(), otherwise
	 *  it has the value \a Missing.
	 */
	SOLSTAT xValStatus_;
	SOLSTAT barXValStatus_;

	/*! \brief This member becomes \a Available if the values of the dual
	 *  variables of the optimal
	 *  solution can be accessed with the function \a yVal(), otherwise
	 *  it has the value \a Missing/
	 */
	SOLSTAT yValStatus_;

	/*! \brief This member becomes \a Available if the reduced costs of the optimal
	 *  solution can be accessed with the function \a reco(), otherwise
	 *  it has the value \a Missing.
	 */
	SOLSTAT recoStatus_;

	/*! \brief This member becomes \a Available if the values of the slack variables
	 *  of the optimal
	 *  solution can be accessed with the function \a slack(), otherwise
	 *  it has the value \a Missing.
	 */
	SOLSTAT slackStatus_;

	/*! \brief This member becomes \a Available if the status of the variables and
	 *  the slack variables of the optimal
	 *  solution can be accessed with the functions \a lpVarStat()
	 *  and \a slackStat(), otherwise
	 *  it has the value \a Missing.
	 */
	SOLSTAT basisStatus_;

	/*! The number of optimizations of the linear program.
	 */
	int nOpt_;
	ogdf::StopwatchCPU lpSolverTime_;

private:

	/*! \brief Resets the optimization status and the availability statuses of
	 *    the solution.
	 *
	 * The function \a initPostOpt() must be called after each modification
	 * of the linear program. It resets the optimization status and the
	 * availability status of the solution.
	 */
	void initPostOpt() {
		optStat_= Unoptimized;
		xValStatus_= barXValStatus_= recoStatus_= Missing;
		slackStatus_= yValStatus_= basisStatus_= Missing;
	}

	LP(const LP &rhs);
	const LP &operator=(const LP &rhs);
};


#if 0
inline OptSense LP::sense() const
{
	return _sense();
}

inline void LP::sense(const OptSense &newSense)
{
	_sense(newSense);
}

inline int LP::nRow() const
{
	return _nRow();
}

inline int LP::maxRow() const
{
	return _maxRow();
}

inline int LP::nCol() const
{
	return _nCol();
}

inline int LP::maxCol() const
{
	return _maxCol();
}

inline int LP::nnz() const
{
	return _nnz();
}

inline double LP::obj(int i) const
{
#ifdef OGDF_DEBUG
	colRangeCheck(i);
#endif
	return _obj(i);
}

inline double LP::lBound(int i) const
{
#ifdef OGDF_DEBUG
	colRangeCheck(i);
#endif
	return _lBound(i);
}

inline double LP::uBound(int i) const
{
#ifdef OGDF_DEBUG
	colRangeCheck(i);
#endif
	return _uBound(i);
}

inline void LP::row(int i, Row &r) const
{
#ifdef OGDF_DEBUG
	rowRangeCheck(i);
#endif
	_row(i, r);
}

inline double LP::rhs(int i) const
{
#ifdef ABACUSSAFE
	rowRangeCheck(i);
#endif
	return _rhs(i);
}

inline double LP::value() const
{
	return _value();
}

inline double LP::xVal(int i)
{
#ifdef OGDF_DEBUG
	colRangeCheck(i);
#endif
	return _xVal(i);
}

inline double LP::barXVal(int i)
{
#ifdef OGDF_DEBUG
	colRangeCheck(i);
#endif
	return _barXVal(i);
}

inline double LP::reco(int i)
{
#ifdef OGDF_DEBUG
	colRangeCheck(i);
#endif
	return _reco(i);
}

inline double LP::yVal(int c)
{
#ifdef OGDF_DEBUG
	rowRangeCheck(c);
#endif
	return _yVal(c);
}

inline double LP::slack(int c)
{
#ifdef OGDF_DEBUG
	rowRangeCheck(c);
#endif
	return _slack(c);
}

inline LP::SOLSTAT LP::xValStatus() const
{
	return xValStatus_;
}

inline LP::SOLSTAT LP::barXValStatus() const
{
	return barXValStatus_;
}

inline LP::SOLSTAT LP::recoStatus() const
{
	return recoStatus_;
}

inline LP::SOLSTAT LP::yValStatus() const
{
	return yValStatus_;
}

inline LP::SOLSTAT LP::slackStatus() const
{
	return slackStatus_;
}

inline LP::SOLSTAT LP::basisStatus() const
{
	return basisStatus_;
}

inline int LP::nOpt() const
{
	return nOpt_;
}

inline bool LP::infeasible() const
{
	if (optStat_ == Infeasible) return true;
	else                        return false;
}

inline LPVARSTAT::STATUS LP::lpVarStat(int i)
{
#ifdef OGDF_DEBUG
	colRangeCheck(i);
#endif
	return _lpVarStat(i);
}

inline SlackStat::STATUS LP::slackStat(int i)
{
#ifdef OGDF_DEBUG
	rowRangeCheck(i);
#endif
	return _slackStat(i);
}
#endif

} //namespace abacus
