#pragma once
#include <vector>
#include "scgms/iface/SolverIface.h"

#include "pagmo/types.hpp"
#include <pagmo/s11n.hpp>

//#############################################################################################
//# CRemap - "dimension reduction / dimension expansion"
//#############################################################################################
/**
 * - This class is needed because:
 *  - SCGMS problem (defined in TSolver_Setup) can have parameters which are "fixed"
 *      - Those parameters cannot change because their lower_bound == upper_bound
 *      - Therefore, it makes no sense to use them with a Pagmo optimization algorithm
 *
 *  - Expand_Solution(x)
 *      - This function converts the "pagmo" representation of individual into a "full" representation
 *      - "full" representation contains both the "fixed" parameters and the parameters we're optimizing
 *
 *  - Reduce_Solution(solution)
 *      - Takes a "full" individual vector and extracts only those parameters which aren't "fixed"
 *
 *  - get_bounds() and problem_size()
 *      - Adapters for pagmo, bounds and problem size only for the non-fixed parameters
 */
class CRemap
{
protected:
    pagmo::vector_double mRemapped_Lower, mRemapped_Upper;
    std::vector<size_t> mDimension_Remap;

    size_t mProblem_Size;
    std::vector<double> mLower_Bound;
    std::vector<double> mUpper_Bound;

public:
    // Default constructor required for boost serialization
    CRemap() : mProblem_Size(0) {}

    CRemap(const solver::TSolver_Setup& setup)
        : mProblem_Size(setup.problem_size),
          mLower_Bound(setup.lower_bound, setup.lower_bound + setup.problem_size),
          mUpper_Bound(setup.upper_bound, setup.upper_bound + setup.problem_size)
    {
        for (size_t i = 0; i < mProblem_Size; i++)
        {
            if (mLower_Bound[i] != mUpper_Bound[i])
            {
                mDimension_Remap.push_back(i);
                mRemapped_Lower.push_back(mLower_Bound[i]);
                mRemapped_Upper.push_back(mUpper_Bound[i]);
            }
        }
    }

    pagmo::vector_double Expand_Solution(const pagmo::vector_double& x) const
    {
        pagmo::vector_double solution(mLower_Bound.begin(), mLower_Bound.end());

        for (size_t i = 0; i < x.size(); i++)
        {
            solution[mDimension_Remap[i]] = std::min(mRemapped_Upper[i], std::max(x[i], mRemapped_Lower[i]));
        }
        return solution;
    }

    pagmo::vector_double Reduce_Solution(const double* solution) const
    {
        pagmo::vector_double result;

        for (size_t i = 0; i < mDimension_Remap.size(); i++)
            result.push_back(solution[mDimension_Remap[i]]);

        return result;
    }

    std::pair<pagmo::vector_double, pagmo::vector_double> get_bounds() const
    {
        return {mRemapped_Lower, mRemapped_Upper};
    }

    size_t problem_size() const
    {
        return mDimension_Remap.size();
    }

private:
    //####################################
    //# BOOST SERIALIZE
    //####################################
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & mProblem_Size;
        ar & mLower_Bound;
        ar & mUpper_Bound;
        ar & mRemapped_Lower;
        ar & mRemapped_Upper;
        ar & mDimension_Remap;
    }
};