#include "TProblemData.h"

#include <math.h>
#include <typeinfo>
#include <random>
#include <numeric>
#include <algorithm>
#include <execution>

//#####################################################################################
//# Global constants
//#####################################################################################

const double pi = atan(1.0) * 4.0;
const double E = std::exp(1.0);


//#####################################################################################
//# Core data structure
//#####################################################################################

CSolution::CSolution() : std::vector<double>()
{
};

void CSolution::setConstant(const double new_value, const size_t new_size)
{
    if (new_size != std::numeric_limits<size_t>::max())
        resize(new_size);

    for (auto& value : *this)
        value = new_value;
}


//#####################################################################################
//# Abstract problem definition
//#####################################################################################

CCommon_Problem::CCommon_Problem(const size_t problem_size) : mProblem_Size(problem_size)
{
}

void CCommon_Problem::Check_Objective_Call(const double fitness, const double* solution)
{
    uint64_t current_objective_call = ++mObjective_Calls;

    const double diff = fabs(fitness - mOptimum_Fitness);


    if (!mOptimum_Reached)
    {
        if (diff == 0.0)
        {
            mOptimum_Reached = true;
            //at this point, it is still possible that more than one thread could have entered this branch

            std::lock_guard<std::mutex> lock(mStat_Guard);
            if (current_objective_call < mLeast_Objective_Call) mLeast_Objective_Call = current_objective_call;
            //it was simpler to use the mutex than to construct a spin-lock while that is not performance critical
        }
    }

    if (!m001_reached)
    {
        if (diff < 0.001)
        {
            m001_reached = true;
            //at this point, it is still possible that more than one thread could have entered this branch
            std::lock_guard<std::mutex> lock(mStat_Guard);

            if (current_objective_call < mLeast_Objective_Call_001)
            {
                mLeast_Objective_Call_001 = current_objective_call;
                mParams001.assign(solution, solution + mProblem_Size);
            }
        }
    }
}

void CCommon_Problem::reset_counters()
{
    mOptimum_Reached = false;
    mObjective_Calls = 0;
    m001_reached = false;

    mLeast_Objective_Call = std::numeric_limits<uint64_t>::max();
    mLeast_Objective_Call_001 = std::numeric_limits<uint64_t>::max();
}

void CCommon_Problem::Init_Optimum()
{
    mAnalytic_Optimum.setConstant(0.0, mProblem_Size); //we assume that optimum is at the origin
    mShift.setConstant(-4.0, mProblem_Size); //optimum params = analytical optm - shift
    mOptimum_Parameters.setConstant(-4.0, mProblem_Size); //parameters we want to determine

    mOptimum_Fitness = Calculate_Fitness(mOptimum_Parameters.data());
    mObjective_Calls = 0;
}

void CCommon_Problem::get_bounds(CSolution& lower, CSolution& upper)
{
    lower.setConstant(-mUpper_Bound_1D, mProblem_Size);
    upper.setConstant(mUpper_Bound_1D, mProblem_Size);
}

void CCommon_Problem::get_optimum(CSolution& parameters, double& fitness)
{
    parameters = mOptimum_Parameters;
    fitness = mOptimum_Fitness;
}

static thread_local std::mt19937_64 random_generator;
static thread_local std::uniform_real_distribution<double> uniform_distribution_double{0.0, 1.0};

void CCommon_Problem::randomize_shift()
{
    for (size_t i = 0; i < mProblem_Size; i++)
    {
        mShift[i] = uniform_distribution_double(random_generator) * 2.0 * mUpper_Bound_1D - mUpper_Bound_1D;
        mOptimum_Parameters[i] = mAnalytic_Optimum[i] + mShift[i];
    }
    //mOptimum_Fitness = Calculate_Fitness(mOptimum_Parameters.data());
    //mObjective_Calls = 0;
    //mOptimum_Reached = false;
}

void CCommon_Problem::Get_Objective_Calls(double& total, double& least, double& least001, CSolution& params001)
{
    //we return double as a convenience as we calculate average later on
    const uint64_t total_calls = mObjective_Calls;
    total = static_cast<double>(total_calls);
    least = static_cast<double>(std::min(mLeast_Objective_Call, total_calls));
    least001 = static_cast<double>(std::min(mLeast_Objective_Call_001, total_calls));
    params001 = mParams001;
}

std::string CCommon_Problem::Get_Name()
{
    const std::string name = Get_Name_Internal();
    const char* begin = strchr(name.data(), 'C') + 1;
    const char* end = strchr(begin, '_');

    return std::string{begin, static_cast<size_t>(std::distance(begin, end))};
}

size_t CCommon_Problem::Problem_Size()
{
    return mProblem_Size;
}

bool CCommon_Problem::Can_Be_Solved()
{
    return true;
}

std::unique_ptr<CCommon_Problem> CCommon_Problem::Clone()
{
    std::unique_ptr<CCommon_Problem> result = Clone_Internal();
    result->Init_Optimum();
    return result;
}


//#####################################################################################
//# Concrete fitness functions
//#####################################################################################

CSphere_Fitness::CSphere_Fitness(const size_t problem_size) : CCommon_Problem(problem_size) { mUpper_Bound_1D = 5.0; };

double CSphere_Fitness::Calculate_Fitness(const double* solution)
{
    double result = 0.0;
    for (size_t i = 0; i < mProblem_Size; i++)
    {
        const double tmp = solution[i] - mShift[i];
        result += tmp * tmp;
    }


    CCommon_Problem::Check_Objective_Call(result, solution);
    return result;
}

std::unique_ptr<CCommon_Problem> CSphere_Fitness::Clone_Internal()
{
    return std::make_unique<CSphere_Fitness>(mProblem_Size);
}

const char* CSphere_Fitness::Get_Name_Internal() { return typeid(this).name(); }


CRosenbrock_Fitness::CRosenbrock_Fitness(const size_t problem_size) : CCommon_Problem(problem_size)
{
    //aka De Jong 2
    mUpper_Bound_1D = 2.0;
    mShift.setConstant(0.0, mProblem_Size);
}

void CRosenbrock_Fitness::Init_Optimum()
{
    mAnalytic_Optimum.setConstant(1.0, mProblem_Size);
    mOptimum_Parameters.setConstant(1.0, mProblem_Size); //we have zero shift
    mOptimum_Fitness = Calculate_Fitness(mOptimum_Parameters.data());
}

double CRosenbrock_Fitness::Calculate_Fitness(const double* solution)
{
    double adjusted_element_previous = solution[0] - mShift[0];
    double adjusted_element_current;

    double result = 0.0;
    for (size_t i = 1; i < mProblem_Size; i++)
    {
        adjusted_element_current = solution[i] - mShift[i];

        const double a = adjusted_element_previous * adjusted_element_previous - adjusted_element_current;
        const double b = (adjusted_element_previous - 1.0);

        result += 100.0 * a * a + b * b;

        adjusted_element_previous = adjusted_element_current;
    }

    CCommon_Problem::Check_Objective_Call(result, solution);
    return result;
}

bool CRosenbrock_Fitness::Can_Be_Solved() { return mProblem_Size >= 2; }

std::unique_ptr<CCommon_Problem> CRosenbrock_Fitness::Clone_Internal()
{
    return std::make_unique<CRosenbrock_Fitness>(mProblem_Size);
}

const char* CRosenbrock_Fitness::Get_Name_Internal() { return typeid(this).name(); }


CAbsSum_Fitness::CAbsSum_Fitness(const size_t problem_size) : CCommon_Problem(problem_size)
{
    //aka De Jong 3
    mUpper_Bound_1D = 2.0;
}

void CAbsSum_Fitness::Init_Optimum()
{
    mAnalytic_Optimum.setConstant(0.0, mProblem_Size);
    mShift.setConstant(-1.0, mProblem_Size);
    mOptimum_Parameters.setConstant(-1.0, mProblem_Size);
    mOptimum_Fitness = Calculate_Fitness(mOptimum_Parameters.data());
}

double CAbsSum_Fitness::Calculate_Fitness(const double* solution)
{
    double result = 0.0;
    for (size_t i = 0; i < mProblem_Size; i++)
    {
        result += fabs(solution[i] - mShift[i]);
    }

    CCommon_Problem::Check_Objective_Call(result, solution);
    return result;
}

std::unique_ptr<CCommon_Problem> CAbsSum_Fitness::Clone_Internal()
{
    return std::make_unique<CAbsSum_Fitness>(mProblem_Size);
}

const char* CAbsSum_Fitness::Get_Name_Internal() { return typeid(this).name(); }


CDeJong4_Fitness::CDeJong4_Fitness(const size_t problem_size) : CCommon_Problem(problem_size)
{
    mUpper_Bound_1D = 1.28;
};

void CDeJong4_Fitness::Init_Optimum()
{
    mAnalytic_Optimum.setConstant(0.0, mProblem_Size);
    mShift.setConstant(-1.0, mProblem_Size);
    mOptimum_Parameters.setConstant(-1.0, mProblem_Size);
    mOptimum_Fitness = Calculate_Fitness(mOptimum_Parameters.data());
}

double CDeJong4_Fitness::Calculate_Fitness(const double* solution)
{
    double result = 0.0;
    double di = 0.0;
    for (size_t i = 0; i < mProblem_Size; i++)
    {
        di += 1.0;
        result += di * pow(solution[i] - mShift[i], 4.0);
    }

    CCommon_Problem::Check_Objective_Call(result, solution);
    return result;
}

std::unique_ptr<CCommon_Problem> CDeJong4_Fitness::Clone_Internal()
{
    return std::make_unique<CDeJong4_Fitness>(mProblem_Size);
}

const char* CDeJong4_Fitness::Get_Name_Internal() { return typeid(this).name(); }


CRastrigin_Fitness::CRastrigin_Fitness(const size_t problem_size) : CCommon_Problem(problem_size),
                                                                    mBase_Result(
                                                                        10.0 * static_cast<double>(mProblem_Size))
{
    mUpper_Bound_1D = 5.12;
};

double CRastrigin_Fitness::Calculate_Fitness(const double* solution)
{
    double result = mBase_Result;
    for (size_t i = 0; i < mProblem_Size; i++)
    {
        const double tmp = solution[i] - mShift[i];
        result += tmp * tmp - 10.0 * cos(2.0 * pi * tmp);
    }

    CCommon_Problem::Check_Objective_Call(result, solution);
    return result;
}

std::unique_ptr<CCommon_Problem> CRastrigin_Fitness::Clone_Internal()
{
    return std::make_unique<CRastrigin_Fitness>(mProblem_Size);
}

const char* CRastrigin_Fitness::Get_Name_Internal() { return typeid(this).name(); }


CSchwefel_Fitness::CSchwefel_Fitness(const size_t problem_size) : CCommon_Problem(problem_size)
{
    mUpper_Bound_1D = 512.0;
}

void CSchwefel_Fitness::Init_Optimum()
{
    mOptimum_Parameters.setConstant(420.9687, mProblem_Size);
    mShift.setConstant(0.0, mProblem_Size);
    mAnalytic_Optimum.setConstant(420.9687, mProblem_Size);
    mOptimum_Fitness = Calculate_Fitness(mAnalytic_Optimum.data());
}

double CSchwefel_Fitness::Calculate_Fitness(const double* solution)
{
    double result = 0.0;
    for (size_t i = 0; i < mProblem_Size; i++)
    {
        const double tmp = solution[i] - mShift[i];
        result -= tmp * sin(sqrt(fabs(tmp)));
    }
    //in the original, there is result+= tmp*... and the following line - but it is useless to calculate it
    //result = 418.9828872724338 * static_cast<double>(mProblem_Size) - result;

    CCommon_Problem::Check_Objective_Call(result, solution);
    return result;
}

std::unique_ptr<CCommon_Problem> CSchwefel_Fitness::Clone_Internal()
{
    return std::make_unique<CSchwefel_Fitness>(mProblem_Size);
}

const char* CSchwefel_Fitness::Get_Name_Internal() { return typeid(this).name(); }


CGriewank_Fitness::CGriewank_Fitness(const size_t problem_size) : CCommon_Problem(problem_size)
{
    mUpper_Bound_1D = 600.0;
};

double CGriewank_Fitness::Calculate_Fitness(const double* solution)
{
    double sum = 0.0;
    double mul = 1.0;

    double di = 0.0;
    for (size_t i = 0; i < mProblem_Size; i++)
    {
        di += 1.0;
        const double tmp = solution[i] - mShift[i];

        sum += tmp * tmp;
        mul *= cos(tmp / sqrt(di));
    }

    const double result = 1.0 + (sum / 4000.0) - mul;
    CCommon_Problem::Check_Objective_Call(result, solution);
    return result;
}

std::unique_ptr<CCommon_Problem> CGriewank_Fitness::Clone_Internal()
{
    return std::make_unique<CGriewank_Fitness>(mProblem_Size);
}

const char* CGriewank_Fitness::Get_Name_Internal() { return typeid(this).name(); }


CStretchedSineV_Fitness::CStretchedSineV_Fitness(const size_t problem_size) : CCommon_Problem(problem_size)
{
    mUpper_Bound_1D = 10.0;
}; //Stretched V sine wave function

double CStretchedSineV_Fitness::Calculate_Fitness(const double* solution)
{
    double adjusted_element_previous = solution[0] - mShift[0];
    double adjusted_element_current;

    double result = 0.0;
    for (size_t i = 1; i < mProblem_Size; i++)
    {
        adjusted_element_current = solution[i] - mShift[i];

        const double sum = adjusted_element_previous * adjusted_element_previous + adjusted_element_current *
            adjusted_element_current;

        result += pow(sum, 0.25) * (pow(sin(50.0 * pow(sum, 0.1)), 2.0) + 1.0);

        adjusted_element_previous = adjusted_element_current;
    }

    CCommon_Problem::Check_Objective_Call(result, solution);
    return result;
}

bool CStretchedSineV_Fitness::Can_Be_Solved() { return mProblem_Size >= 2; }

std::unique_ptr<CCommon_Problem> CStretchedSineV_Fitness::Clone_Internal()
{
    return std::make_unique<CStretchedSineV_Fitness>(mProblem_Size);
}

const char* CStretchedSineV_Fitness::Get_Name_Internal() { return typeid(this).name(); }


CMasters_Fitness::CMasters_Fitness(const size_t problem_size) : CCommon_Problem(problem_size)
{
    mUpper_Bound_1D = 5.0;
};

double CMasters_Fitness::Calculate_Fitness(const double* solution)
{
    double adjusted_element_previous = solution[0] - mShift[0];
    double adjusted_element_current;

    double result = 0.0;
    for (size_t i = 1; i < mProblem_Size; i++)
    {
        adjusted_element_current = solution[i] - mShift[i];

        const double x = adjusted_element_previous * adjusted_element_previous + adjusted_element_current *
            adjusted_element_current + 0.5 * adjusted_element_previous * adjusted_element_current;

        result -= pow(E, -x / 8.0) * cos(4.0 * sqrt(x));

        adjusted_element_previous = adjusted_element_current;
    }

    CCommon_Problem::Check_Objective_Call(result, solution);
    return result;
}

bool CMasters_Fitness::Can_Be_Solved() { return mProblem_Size >= 2; }

std::unique_ptr<CCommon_Problem> CMasters_Fitness::Clone_Internal()
{
    return std::make_unique<CMasters_Fitness>(mProblem_Size);
}

const char* CMasters_Fitness::Get_Name_Internal() { return typeid(this).name(); }


//#####################################################################################
//# Factory
//#####################################################################################

TProblem_Collection Create_Problem_Collection(const size_t problem_size)
{
    TProblem_Collection result;

    result.push_back(std::make_unique<CSphere_Fitness>(problem_size));
    result.push_back(std::make_unique<CRosenbrock_Fitness>(problem_size));
    result.push_back(std::make_unique<CAbsSum_Fitness>(problem_size));
    result.push_back(std::make_unique<CDeJong4_Fitness>(problem_size));
    result.push_back(std::make_unique<CRastrigin_Fitness>(problem_size));
    result.push_back(std::make_unique<CSchwefel_Fitness>(problem_size));
    result.push_back(std::make_unique<CGriewank_Fitness>(problem_size));
    //	result.push_back(std::make_unique<CStretchedSineV_Fitness>(problem_size));
    result.push_back(std::make_unique<CMasters_Fitness>(problem_size));

    for (auto& problem : result)
        problem->Init_Optimum();

    return result;
}

//#####################################################################################
//# Boost serialization implements
//#####################################################################################
BOOST_CLASS_EXPORT_IMPLEMENT(CSphere_Fitness)

BOOST_CLASS_EXPORT_IMPLEMENT(CRosenbrock_Fitness)

BOOST_CLASS_EXPORT_IMPLEMENT(CAbsSum_Fitness)

BOOST_CLASS_EXPORT_IMPLEMENT(CDeJong4_Fitness)

BOOST_CLASS_EXPORT_IMPLEMENT(CRastrigin_Fitness)

BOOST_CLASS_EXPORT_IMPLEMENT(CSchwefel_Fitness)

BOOST_CLASS_EXPORT_IMPLEMENT(CGriewank_Fitness)

BOOST_CLASS_EXPORT_IMPLEMENT(CStretchedSineV_Fitness)

BOOST_CLASS_EXPORT_IMPLEMENT(CMasters_Fitness)
