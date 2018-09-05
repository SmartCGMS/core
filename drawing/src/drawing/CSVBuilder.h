/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Technicka 8
 * 314 06, Pilsen
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *    GPLv3 license. When publishing any related work, user of this software
 *    must:
 *    1) let us know about the publication,
 *    2) acknowledge this software and respective literature - see the
 *       https://diabetes.zcu.cz/about#publications,
 *    3) At least, the user of this software must cite the following paper:
 *       Parallel software architecture for the next generation of glucose
 *       monitoring, Proceedings of the 8th International Conference on Current
 *       and Future Trends of Information and Communication Technologies
 *       in Healthcare (ICTH 2018) November 5-8, 2018, Leuven, Belgium
 * b) For any other use, especially commercial use, you must contact us and
 *    obtain specific terms and conditions for the use of the software.
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
