#pragma once
#include "../CRemap.h"
#include "udp_base.h"
#include "scgms/iface/SolverIface.h"

#include <pagmo/s11n.hpp>
#include <boost/serialization/export.hpp>
#include <any>

//#############################################################################################
//# TProblem - Pagmo-style UDP
//#############################################################################################
class TProblem : public udp_base
{
    solver::TSolver_Setup mSetup;

    // We're using this remapper (defined above) to convert SCGMS individual vector representation into a pagmo-compatible one
    CRemap mRemap;

public:
    TProblem(const solver::TSolver_Setup& setup, solver::TSolver_Progress& progress);

    TProblem();

    TProblem(const TProblem& other);

    /**
     * Pagmo UDP's standard fitness function,
     * called internally by Pagmo algorithms
     */
    pagmo::vector_double fitness(const pagmo::vector_double& x) const override;

    //TODO: Batch fitness could be added here

    /**
     * Pagmo UDP's standard bounds function,
     * called internally by Pagmo algorithms
     */
    std::pair<pagmo::vector_double, pagmo::vector_double> get_bounds() const override;

    /**
     * Pagmo UDP's standard number of objectives function,
     * called internally by Pagmo algorithms
     */
    pagmo::vector_double::size_type get_nobj() const override;

    // REQUIRED by udp_base
    std::string get_lib_file_name() override;

    size_t problem_size() const;

    const CRemap& remap() const;

private:
    //####################################
    //# BOOST SERIALIZE
    //####################################
    friend class boost::serialization::access;

    template <typename Archive>
    void save(Archive& ar, unsigned) const
    {
        boost::serialization::void_cast_register<TProblem, udp_base>();
        ar << mRemap;
        // mSetup intentionally not serialized (TODO)
    }

    template <typename Archive>
    void load(Archive& ar, unsigned)
    {
        boost::serialization::void_cast_register<TProblem, udp_base>();

        try
        {
            ar >> mRemap;
            // mSetup and mProgress left at defaults set by the default constructor
        }
        catch (...)
        {
            //*this = TProblem{};
            throw;
        }
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

extern "C" void run_after_load();

extern "C" TProblem* allocator(const std::any& params);

extern "C" void deleter(TProblem* ptr);

extern "C" TProblem* cloner(const TProblem* other);

BOOST_CLASS_EXPORT_KEY(TProblem)