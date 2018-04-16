/**
 * Portal for online glucose level calculation
 * Final calculation and SVG generating part
 * https://diabetes.zcu.cz/
 *
 * Author: Marek Rasocha (mrasocha@students.zcu.cz)
 * Maintainer: Martin Ubl (ublm@students.zcu.cz)
 */

#pragma once

#include <map>
#include <sstream>

#include "Containers/Value.h"

// enumeration of all CSV header fields that are supported; for translation constants, see CSVBuilder.cpp
enum class CSV_Header_Fields
{
    DATETIME,
    IST,
    BLOOD,
    BLOOD_CALIBRATION,
    DIFF2,
    DIFF3,
    INSULIN,
    CARBOHYDRATES,
    ISIG,

    _END,
    _BEGIN = DATETIME,
    _BEGIN_VALUE = IST
};

class CCSV_Builder
{
    private:
        // internal stream instance
        std::ostringstream mCsvStream;
        // map of vectors used for output
        std::map<CSV_Header_Fields, ValueVector*> mValueVectors;
        // localization strings
        LocalizationMap& mLocalization;

    protected:
        // writes CSV header to stream
        void Write_Headers();
        // sorts input, so it could be processed using better algorithm
        void Sort_Inputs();
        // writes single value row to stream, aggregates values if possible
        void Write_Row(std::map<CSV_Header_Fields, size_t>& vecPos, time_t timeAt);

    public:
        CCSV_Builder(LocalizationMap& localizationMap);
        virtual ~CCSV_Builder();

        // enables header field and sets value vector as data source
        void Enable_Header_Field(CSV_Header_Fields field, ValueVector* values);
        // disables header field
        void Disable_Header_Field(CSV_Header_Fields field);
        // determines, whether the field is enabled
        bool Is_Header_Field_Enabled(CSV_Header_Fields field) const;

        // builds CSV and returns generated string
        std::string Build();

        // translation method
        std::string tr(std::string key)
        {
            LocalizationMap::iterator search = mLocalization.find(key);
            if (search != mLocalization.end())
                return search->second;
            else
                return "<not loaded>";
        }
};
