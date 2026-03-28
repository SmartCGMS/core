#include "TProblem.h"

TProblem::TProblem(const solver::TSolver_Setup& setup, solver::TSolver_Progress& progress)
    : mSetup(setup),
      mRemap(setup),
      mProgress(progress)
{
}

TProblem::TProblem()
    : mSetup(solver::Default_Solver_Setup),
      mRemap(solver::Default_Solver_Setup),
      mProgress(mVoid_Progress)
{
}

TProblem::TProblem(const TProblem& other)
    : mSetup(other.mSetup),
      mRemap(other.mRemap),
      mProgress(other.mProgress)
{
}

pagmo::vector_double TProblem::fitness(const pagmo::vector_double& x) const
{
    assert(mSetup.problem_size > 0);

    const auto solution = mRemap.Expand_Solution(x);

    pagmo::vector_double result(
        solver::Maximum_Objectives_Count,
        std::numeric_limits<double>::quiet_NaN()
    );

    mSetup.objective(mSetup.data, 1, solution.data(), result.data());

    result.resize(mSetup.objectives_count);
    return result;
}

std::pair<pagmo::vector_double, pagmo::vector_double> TProblem::get_bounds() const
{
    return mRemap.get_bounds();
}

pagmo::vector_double::size_type TProblem::get_nobj() const
{
    return mSetup.objectives_count;
}

std::string TProblem::get_lib_file_name()
{
    return "tproblem_udp";
}

size_t TProblem::problem_size() const
{
    return mRemap.problem_size();
}

const CRemap& TProblem::remap() const
{
    return mRemap;
}

void run_after_load()
{
}

TProblem* allocator(const std::any& params)
{
    return new TProblem();
}

void deleter(TProblem* ptr)
{
    delete ptr;
}

TProblem* cloner(const TProblem* other)
{
    if (!other)
        return nullptr;
    return new TProblem(*other);
}

BOOST_CLASS_EXPORT_IMPLEMENT(TProblem)
