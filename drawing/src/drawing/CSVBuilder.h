/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Copyright (c) since 2018 University of West Bohemia.
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Univerzitni 8, 301 00 Pilsen
 * Czech Republic
 * 
 * 
 * Purpose of this software:
 * This software is intended to demonstrate work of the diabetes.zcu.cz research
 * group to other scientists, to complement our published papers. It is strictly
 * prohibited to use this software for diagnosis or treatment of any medical condition,
 * without obtaining all required approvals from respective regulatory bodies.
 *
 * Especially, a diabetic patient is warned that unauthorized use of this software
 * may result into severe injure, including death.
 *
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under these license terms is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *      GPLv3 license.
 * b) For any other use, especially commercial use, you must contact us and
 *       obtain specific terms and conditions for the use of the software.
 * c) When publishing work with results obtained using this software, you agree to cite the following paper:
 *       Tomas Koutny and Martin Ubl, "Parallel software architecture for the next generation of glucose
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
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
    IST_ISIG,

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
