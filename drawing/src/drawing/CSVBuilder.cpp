/**
 * Portal for online glucose level calculation
 * Final calculation and SVG generating part
 * https://diabetes.zcu.cz/
 *
 * Author: Marek Rasocha (mrasocha@students.zcu.cz)
 * Maintainer: Martin Ubl (ublm@students.zcu.cz)
 */

#include "general.h"
#include "CSVBuilder.h"
#include "Misc/Utility.h"
#include <algorithm>
#include <limits>

// user-defined operator for incrementing enum variable value
CSV_Header_Fields& operator++(CSV_Header_Fields &left)
{
    left = (CSV_Header_Fields)((size_t)left + 1);
    return left;
}

// translation constants used to translate CSV header fields
const char* CSV_Header_Fields_tr[] = {
    "title_datetime",
    "title_ist",
    "title_blood",
    "title_blood_calibration",
    "title_diff2",
    "title_diff3",
    "title_insulin",
    "title_carbohydrates",
    "title_isig"
};

CCSV_Builder::CCSV_Builder(LocalizationMap& localizationMap) : mLocalization(localizationMap)
{
}

CCSV_Builder::~CCSV_Builder()
{
}

void CCSV_Builder::Enable_Header_Field(CSV_Header_Fields field, ValueVector* values)
{
    if (!values || values->empty())
        return;

    mValueVectors[field] = values;
}

void CCSV_Builder::Disable_Header_Field(CSV_Header_Fields field)
{
    auto itr = mValueVectors.find(field);

    if (itr != mValueVectors.end())
        mValueVectors.erase(itr);
}

bool CCSV_Builder::Is_Header_Field_Enabled(CSV_Header_Fields field) const
{
    auto itr = mValueVectors.find(field);

    return itr != mValueVectors.end();
}

void CCSV_Builder::Write_Headers()
{
    // datetime header is always present
    mCsvStream << tr(CSV_Header_Fields_tr[(size_t)CSV_Header_Fields::DATETIME]);

    // iterate through all header fields, determine whether they are enabled and write them here, delimited by semicolon
    for (CSV_Header_Fields a = CSV_Header_Fields::_BEGIN_VALUE; a < CSV_Header_Fields::_END; ++a)
    {
        if (!Is_Header_Field_Enabled(a))
            continue;

        mCsvStream << ";" << tr(CSV_Header_Fields_tr[(size_t)a]);
    }

    mCsvStream << std::endl;
}

void CCSV_Builder::Sort_Inputs()
{
    // just go through all value headers and sort their values by date (ascending)
    for (CSV_Header_Fields a = CSV_Header_Fields::_BEGIN_VALUE; a < CSV_Header_Fields::_END; ++a)
    {
        if (!Is_Header_Field_Enabled(a))
            continue;

        std::sort(mValueVectors[a]->begin(), mValueVectors[a]->end(), [](Value& a, Value& b) {
            return a.date < b.date;
        });
    }
}

void CCSV_Builder::Write_Row(std::map<CSV_Header_Fields, size_t>& vecPos, time_t timeAt)
{
    // datetime field is always there
    mCsvStream << Utility::Format_Date_CSV(timeAt);

    // iterate through all value fields and attempt to aggregate output
    for (CSV_Header_Fields a = CSV_Header_Fields::_BEGIN_VALUE; a < CSV_Header_Fields::_END; ++a)
    {
        if (!Is_Header_Field_Enabled(a))
            continue;

        // the semicolon is always here (even though the value is empty)
        mCsvStream << ";";

        // beyond limit or the value is not measured at time of interest
        if (vecPos[a] == std::numeric_limits<size_t>::max() || (*mValueVectors[a])[vecPos[a]].date != timeAt)
            continue;

        // write the value
        mCsvStream << (*mValueVectors[a])[vecPos[a]].Get_Value();

        // increase vector position, so the value is not taken twice
        vecPos[a]++;
        // indicate end of parsing by setting limit value
        if (vecPos[a] >= mValueVectors[a]->size())
            vecPos[a] = std::numeric_limits<size_t>::max();
    }

    mCsvStream << std::endl;
}

std::string CCSV_Builder::Build()
{
    mCsvStream.clear();
    mCsvStream.str(std::string(""));

    Write_Headers();

    Sort_Inputs();

    // initialize vector positions to zero
    std::map<CSV_Header_Fields, size_t> vecPos;
    for (CSV_Header_Fields a = CSV_Header_Fields::_BEGIN_VALUE; a < CSV_Header_Fields::_END; ++a)
    {
        if (Is_Header_Field_Enabled(a))
        {
            if (mValueVectors[a]->empty())
                vecPos[a] = std::numeric_limits<size_t>::max();
            else
                vecPos[a] = 0;
        }
    }

    CSV_Header_Fields curVec;
    time_t curTime;

    do
    {
        // put "invalid values" here
        curTime = std::numeric_limits<time_t>::max();
        curVec = CSV_Header_Fields::_END;

        // go through all fields and get the earliest waiting value from all fields
        for (CSV_Header_Fields a = CSV_Header_Fields::_BEGIN_VALUE; a < CSV_Header_Fields::_END; ++a)
        {
            auto itr = vecPos.find(a);
            // not found or already beyond limit
            if (itr == vecPos.end() || itr->second == std::numeric_limits<size_t>::max())
                continue;

            // no vector present or we have earlier time than currently selected one
            if (curVec == CSV_Header_Fields::_END || (*mValueVectors[a])[itr->second].date < curTime)
            {
                curTime = (*mValueVectors[a])[itr->second].date;
                curVec = a;
            }
        }

        // if something had been found, write it
        if (curVec != CSV_Header_Fields::_END)
            Write_Row(vecPos, curTime);

    } while (curVec != CSV_Header_Fields::_END);

    return mCsvStream.str();
}
