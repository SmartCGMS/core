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
 * a) Without a specific agreement, you are not authorized to make or keep any copies of this file.
 * b) For any use, especially commercial use, you must contact us and obtain specific terms and conditions 
 *    for the use of the software.
 * c) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#include <algorithm>
#include "StringHelpers.h"

/**
 * Splits supplied string using delimiter
 * @param str - input string
 * @param delimiterOpen - starting delimiter
 * @param delimiterClose - closing delimiter
 * @return a vector of strings originally delimited by delimiter
 */
std::vector<std::string> split_string(std::string str, std::string delimiterOpen, std::string delimiterClose)
{
    std::vector<std::string> data;
    std::replace(str.begin(), str.end(), '"', ' ');
    size_t pos_open = 0;
    size_t pos_close = 0;
    size_t search_by = 0;
    size_t start_by = 0;
    int zanoreni = 0;

    while (true)
    {
        pos_close = str.find(delimiterClose, search_by);
        pos_open = str.find(delimiterOpen, search_by);
        if (search_by == 0)
            start_by = pos_open;

        if (pos_close == std::string::npos)
            break;

        if (pos_open < pos_close)
        {
            search_by = pos_open + 1;
            zanoreni++;
        }
        else
        {
            search_by = pos_close;
            zanoreni--;
        }

        if (zanoreni == 0)
        {
            size_t start = start_by + delimiterOpen.length();
            data.push_back(str.substr(start, search_by - start));
            str.erase(0, search_by + delimiterClose.length());
            search_by = 0;
        }
    }

    return data;
}

/**
 * Splits supplied string using delimiter
 * @param str - input string
 * @param delimiter - delimiter
 * @return a vector of strings originally delimited by delimiter
 */
std::vector<std::string> split_string(std::string str, std::string delimiter)
{
    std::vector<std::string> data;
    size_t pos = 0;

    while ((pos = str.find(delimiter)) != std::string::npos)
    {
        std::string pom = (str.substr(0, pos));
        trim(pom);
        data.push_back(pom);
        str.erase(0, pos + delimiter.length());
    }

    trim(str);
    data.push_back(str);
    return data;
}

/**
 * Splits supplied string in position of first occurence of specified delimiter
 * @param str - string
 * @param delimiter - delimiter
 * @return a vector of string with maximum size of 2, containing parts of string split by delimiter
 */
std::vector<std::string> split_first_string(std::string str, std::string delimiter)
{
    std::vector<std::string> data;
    size_t pos = 0;

    if ((pos = str.find(delimiter)) != std::string::npos)
    {
        std::string pom = (str.substr(0, pos));
        trim(pom);
        data.push_back(pom);
        str.erase(0, pos + delimiter.length());
    }

    trim(str);
    data.push_back(str);

    return data;
}
