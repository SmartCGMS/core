#pragma once

#include <scgms/iface/SolverIface.h>

#include <atomic>
#include <vector>
#include <algorithm>
#include <string>
#include <mutex>

// Because pathfinder is linking against this at build time, we need to export some classes
#include <dll_visibility.h>

#undef max
#undef min

//#####################################################################################
//# Global constants
//#####################################################################################

extern const double pi;
extern const double E;


//#####################################################################################
//# Core data structure
//#####################################################################################

class DLL_PUBLIC CSolution : public std::vector<double> {
public:
	CSolution();
	void setConstant(const double new_value, const size_t new_size = std::numeric_limits<size_t>::max());
};


//#####################################################################################
//# Abstract problem definition
//#####################################################################################

class DLL_PUBLIC CCommon_Problem {
protected:
	const size_t mProblem_Size;
	double mUpper_Bound_1D = std::numeric_limits<double>::quiet_NaN();
	double mOptimum_Fitness = std::numeric_limits<double>::quiet_NaN(); //cached value to save some calls from Check_Objective_Call

	CSolution mAnalytic_Optimum, mOptimum_Parameters, mShift, mParams001;
	std::atomic<bool> m001_reached{ false }, mOptimum_Reached{ false };
	std::mutex mStat_Guard;
protected:
	//double mShift = -4.0;	//e.g., to eliminate too good results which occurs by a chance when the algorithm constructs
							//initial estimate as average of the bounds and accidentally hits the optimum
	std::atomic<uint64_t> mObjective_Calls{ 0 };	//number of callings to the objective functions
	uint64_t mLeast_Objective_Call = std::numeric_limits<uint64_t>::max();		//number of objective call, when the difference between optimum got to zero
	uint64_t mLeast_Objective_Call_001 = std::numeric_limits<uint64_t>::max();	//number of objective call, when the difference between optimum got to zero

protected:
	inline void Check_Objective_Call(const double fitness, const double *solution);
	virtual std::unique_ptr<CCommon_Problem> Clone_Internal() = 0;
	virtual const char* Get_Name_Internal() = 0;
public:
	CCommon_Problem(const size_t problem_size);
	virtual ~CCommon_Problem() {};

	virtual void Init_Optimum();

	std::unique_ptr<CCommon_Problem> Clone();

	void get_bounds(CSolution &lower, CSolution &upper);
	void get_optimum(CSolution &parameters, double &fitness);
	void randomize_shift();
	void reset_counters();

	void Get_Objective_Calls(double &total, double &least, double &least001, CSolution &params001);
	virtual double Calculate_Fitness(const double *solution) = 0;
	virtual bool Can_Be_Solved();	//tests whether the problem can be solved for the given problem size
	std::string Get_Name();
	size_t Problem_Size();
};


//#####################################################################################
//# Concrete fitness functions
//#####################################################################################

// The classes below possibly don't need DLL_PUBLIC?

class DLL_PUBLIC CSphere_Fitness : public virtual CCommon_Problem { //aka DeJong1
public:
	CSphere_Fitness(const size_t problem_size);
	virtual double Calculate_Fitness(const double *solution) final;
	virtual std::unique_ptr<CCommon_Problem> Clone_Internal();
	virtual const char* Get_Name_Internal() final;
};


class DLL_PUBLIC CRosenbrock_Fitness : public virtual CCommon_Problem {	//aka De Jong 2
public:
	CRosenbrock_Fitness(const size_t problem_size);
	void Init_Optimum();
	virtual double Calculate_Fitness(const double *solution) final;
	virtual bool Can_Be_Solved();
	virtual std::unique_ptr<CCommon_Problem> Clone_Internal();
	virtual const char* Get_Name_Internal() final;
};


class DLL_PUBLIC CAbsSum_Fitness : public virtual CCommon_Problem {	//aka De Jong 3
public:
	CAbsSum_Fitness(const size_t problem_size);
	void Init_Optimum();
	virtual double Calculate_Fitness(const double *solution) final;
	virtual std::unique_ptr<CCommon_Problem> Clone_Internal();
	virtual const char* Get_Name_Internal() final;
};


class DLL_PUBLIC CDeJong4_Fitness : public virtual CCommon_Problem {
public:
	CDeJong4_Fitness(const size_t problem_size);
	void Init_Optimum();
	virtual double Calculate_Fitness(const double *solution) final;
	virtual std::unique_ptr<CCommon_Problem> Clone_Internal();
	virtual const char* Get_Name_Internal() final;
};


class DLL_PUBLIC CRastrigin_Fitness : public virtual CCommon_Problem {
protected:
	const double mBase_Result;
public:
	CRastrigin_Fitness(const size_t problem_size);
	virtual double Calculate_Fitness(const double *solution) final;
	virtual std::unique_ptr<CCommon_Problem> Clone_Internal();
	virtual const char* Get_Name_Internal() final;
};


class DLL_PUBLIC CSchwefel_Fitness : public virtual CCommon_Problem {
public:
	CSchwefel_Fitness(const size_t problem_size);
	void Init_Optimum();
	virtual double Calculate_Fitness(const double *solution) final;
	virtual std::unique_ptr<CCommon_Problem> Clone_Internal();
	virtual const char* Get_Name_Internal() final;
};


class DLL_PUBLIC CGriewank_Fitness : public virtual CCommon_Problem {
public:
	CGriewank_Fitness(const size_t problem_size);
	virtual double Calculate_Fitness(const double *solution) final;
	virtual std::unique_ptr<CCommon_Problem> Clone_Internal();
	virtual const char* Get_Name_Internal() final;
};


class DLL_PUBLIC CStretchedSineV_Fitness : public virtual CCommon_Problem {	//Stretched V sine wave function
public:
	CStretchedSineV_Fitness(const size_t problem_size);
	virtual double Calculate_Fitness(const double *solution) final;
	virtual bool Can_Be_Solved();
	virtual std::unique_ptr<CCommon_Problem> Clone_Internal();
	virtual const char* Get_Name_Internal() final;
};


class DLL_PUBLIC CMasters_Fitness : public virtual CCommon_Problem {
public:
	CMasters_Fitness(const size_t problem_size);
	virtual double Calculate_Fitness(const double *solution) final;
	virtual bool Can_Be_Solved();
	virtual std::unique_ptr<CCommon_Problem> Clone_Internal();
	virtual const char* Get_Name_Internal() final;
};


//#####################################################################################
//# Type aliases
//#####################################################################################

using TProblem_Collection = std::vector<std::unique_ptr<CCommon_Problem>>;


//#####################################################################################
//# Factory
//#####################################################################################

DLL_PUBLIC TProblem_Collection Create_Problem_Collection(const size_t problem_size);