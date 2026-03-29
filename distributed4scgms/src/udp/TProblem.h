#pragma once

// We're assuming that the following two headers are available (or we can simply copy them if they're not)
#include "../CRemap.h"
#include "../TUdpParams.h"

#include "udp_base.h"
#include "scgms/iface/SolverIface.h"

#include <pagmo/s11n.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <any>

#include "TProblemData.h"
#include "TProblemObjective.h"

//#############################################################################################
//# TProblem - Pagmo-style UDP
//#############################################################################################
class TProblem : public udp_base
{
    // We're using this remapper to convert SCGMS individual vector representation into a pagmo-compatible one
    CRemap mRemap;
    TUdpParamsSerializable mParams{};

    // Declared in TProblemData.h
    std::shared_ptr<CCommon_Problem> mUdpData{};

public:
    TProblem(const TUdpParams& params);

    TProblem();

    TProblem(const TProblem& other);

    TProblem& operator=(const TProblem& other);

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
    void _process_data_pointer(const void* data);

    //####################################
    //# BOOST SERIALIZE
    //####################################
    friend class boost::serialization::access;

    template <typename Archive>
    void save(Archive& ar, unsigned) const
    {
        boost::serialization::void_cast_register<TProblem, udp_base>();

        ar << mRemap;
        ar << mParams;
        ar << mUdpData;
    }

    template <typename Archive>
    void load(Archive& ar, unsigned)
    {
        boost::serialization::void_cast_register<TProblem, udp_base>();

        ar >> mRemap;
        ar >> mParams;
        ar >> mUdpData;
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

extern "C" void run_after_load();

extern "C" TProblem* allocator(const std::any& params);

extern "C" void deleter(TProblem* ptr);

extern "C" TProblem* cloner(const TProblem* other);

BOOST_CLASS_EXPORT_KEY(TProblem)
