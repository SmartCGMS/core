#include "TProblem.h"

TProblem::TProblem(const TUdpParams& params)
{
    mParams = params.params;
    mRemap = CRemap{mParams.lower_bound_vec, mParams.upper_bound_vec};
    _process_data_pointer(params.data);
}

TProblem::TProblem() : mRemap(solver::Default_Solver_Setup)
{
}

TProblem::TProblem(const TProblem& other): udp_base(other),
                                           mRemap(other.mRemap),
                                           mParams(other.mParams),
                                           mUdpData(other.mUdpData.get()->Clone()) // CLONE THE OBJECT INSIDE PTR INSTEAD OF COPYING PTR
{
}

TProblem& TProblem::operator=(const TProblem& other)
{
    if (this == &other)
        return *this;
    udp_base::operator =(other);
    mRemap = other.mRemap;
    mParams = other.mParams;
    mUdpData = other.mUdpData.get()->Clone(); // CLONE THE OBJECT INSIDE PTR INSTEAD OF COPYING PTR
    return *this;
}


//#############################################################################################
//# Pagmo UDP functions
//#############################################################################################

pagmo::vector_double TProblem::fitness(const pagmo::vector_double& x) const
{
    //assert(mSetup.problem_size > 0);

    const auto solution = mRemap.Expand_Solution(x);

    pagmo::vector_double result(
        solver::Maximum_Objectives_Count,
        std::numeric_limits<double>::quiet_NaN()
    );

    // Fitness_Wrapper is declared in TProblemObjective.h, this replaces the original mSetup.objective call, mUdpData.get() replaces mObjective.data
    Fitness_Wrapper(mUdpData.get(), 1, solution.data(), result.data());

    // TODO: Does this really work? Why is remapper not used here?
    result.resize(mParams.objectives_count);
    return result;
}

std::pair<pagmo::vector_double, pagmo::vector_double> TProblem::get_bounds() const
{
    return mRemap.get_bounds();
}

pagmo::vector_double::size_type TProblem::get_nobj() const
{
    return mParams.objectives_count;
}

//#############################################################################################
//# Other member functions
//#############################################################################################

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

void TProblem::_process_data_pointer(const void* data)
{
    // We assume that the data is passed correctly as this type
    CCommon_Problem* cProblemPtr = reinterpret_cast<CCommon_Problem*>(const_cast<void*>(data));

    mUdpData = cProblemPtr->Clone();
}

//#############################################################################################
//# Extern C lib functions
//#############################################################################################

void run_after_load()
{
}

TProblem* allocator(const std::any& params)
{
    TUdpParams castedParams;
    if (params.has_value())
    {
        try
        {
            castedParams = std::any_cast<TUdpParams>(params);
        }
        catch (...)
        {
            throw std::runtime_error("TProblem UDP only accepts TUdpParams as its parameter.");
        }
    }

    return new TProblem(castedParams);
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
