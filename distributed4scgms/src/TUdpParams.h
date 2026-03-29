#pragma once
#include "scgms/iface/SolverIface.h"
#include <vector>
#include <limits>
#include <pagmo/s11n.hpp>

/**
 * - Contains all serializable information from the original TSolver_Setup struct
 * - Use ExtractSerializable() to construct it from TSolver_Setup
 * - Apart from C++ STL vectors, it also provides C-style pointer access like the original struct
 */
struct TUdpParamsSerializable
{
    size_t problem_size;
    size_t objectives_count;
    std::vector<double> lower_bound_vec;
    std::vector<double> upper_bound_vec;
    std::vector<std::vector<double>> hints_vec;
    size_t hint_count;
    size_t max_generations;
    size_t population_size;
    double tolerance;

    // Backwards compatible C-style fields
    const double* lower_bound = nullptr;
    const double* upper_bound = nullptr;
    const double** hints = nullptr;

private:
    // Internal storage for pointers to hint vectors
    std::vector<const double*> c_hints_storage;

public:
    TUdpParamsSerializable() : problem_size(0), objectives_count(0), hint_count(0), max_generations(0),
                               population_size(0), tolerance(0.0), lower_bound(nullptr), upper_bound(nullptr),
                               hints(nullptr)
    {
    }

    // Call after modifying vectors to update C-style pointers
    void update_c_style_pointers()
    {
        lower_bound = lower_bound_vec.empty() ? nullptr : lower_bound_vec.data();
        upper_bound = upper_bound_vec.empty() ? nullptr : upper_bound_vec.data();

        c_hints_storage.clear();
        for (auto& v : hints_vec)
            c_hints_storage.push_back(v.empty() ? nullptr : v.data());

        hints = c_hints_storage.empty() ? nullptr : c_hints_storage.data();
    }

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void save(Archive& ar, const unsigned) const
    {
        ar & problem_size;
        ar & objectives_count;
        ar & lower_bound_vec;
        ar & upper_bound_vec;
        ar & hints_vec;
        ar & hint_count;
        ar & max_generations;
        ar & population_size;
        ar & tolerance;
    }

    template <typename Archive>
    void load(Archive& ar, const unsigned)
    {
        ar & problem_size;
        ar & objectives_count;
        ar & lower_bound_vec;
        ar & upper_bound_vec;
        ar & hints_vec;
        ar & hint_count;
        ar & max_generations;
        ar & population_size;
        ar & tolerance;

        update_c_style_pointers();
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

inline TUdpParamsSerializable ExtractSerializable(const solver::TSolver_Setup& setup)
{
    TUdpParamsSerializable result;

    result.problem_size = setup.problem_size;
    result.objectives_count = setup.objectives_count;

    if (setup.lower_bound)
        result.lower_bound_vec.assign(setup.lower_bound, setup.lower_bound + setup.problem_size);
    if (setup.upper_bound)
        result.upper_bound_vec.assign(setup.upper_bound, setup.upper_bound + setup.problem_size);

    result.hint_count = setup.hint_count;
    if (setup.hints && setup.hint_count > 0)
    {
        result.hints_vec.reserve(setup.hint_count);
        for (size_t i = 0; i < setup.hint_count; ++i)
        {
            if (setup.hints[i])
                result.hints_vec.emplace_back(setup.hints[i], setup.hints[i] + setup.problem_size);
            else
                result.hints_vec.emplace_back(setup.problem_size, std::numeric_limits<double>::quiet_NaN());
        }
    }

    result.max_generations = setup.max_generations;
    result.population_size = setup.population_size;
    result.tolerance = setup.tolerance;

    result.update_c_style_pointers();

    return result;
}

/**
 * - This struct is passed to a UDP, it's up to UDP to properly process the void pointer
 * - Contains all original information from TSolver_Setup, except for the function pointers
 */
struct TUdpParams
{
    const void* data;
    TUdpParamsSerializable params;
};
