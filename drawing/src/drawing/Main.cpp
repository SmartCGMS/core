/**
 * Portal for online glucose level calculation
 * Final calculation and SVG generating part
 * https://diabetes.zcu.cz/
 *
 * Author: Marek Rasocha (mrasocha@students.zcu.cz)
 * Maintainer: Martin Ubl (ublm@students.zcu.cz)
 */

#include "general.h"

#include "Containers/Diff2.h"
#include "Containers/Diff3.h"
#include "Containers/Value.h"
#include "Misc/StringHelpers.h"
#include "Method.h"
#include "Misc/Utility.h"
#include "CSVBuilder.h"

#include "Generators/IGenerator.h"
#include "Generators/GraphGenerator.h"
#include "Generators/ClarkGenerator.h"
#include "Generators/ParkesGenerator.h"
#include "Generators/AGPGenerator.h"
#include "Generators/DayGenerator.h"

#ifdef _MSC_VER
#pragma message("Warning: compiling just to executable, not using emscripten; to use emscripten, follow instructions in README")
#endif

std::string ist, blood, bloodCalibration, isig, insulin, carbs, par2, par3, resultSvg, stringsJson, resultCSV;
ValueVector istVector, bloodVector, bloodCalibrationVector, isigVector, insulinVector, carbsVector, diff2Result, diff3Result;
bool have_data = false;
Diff3 diff3;
Diff2 diff2;

Diff2 Parse_Diffusion2_Params(std::string object)
{
    Diff2 diff2;
    std::vector<std::string> attribute = split_string(object, ",");
    size_t j;

    for (j = 0; j < attribute.size(); j++)
    {
        std::vector<std::string> value = split_string(attribute[j], ":");

        std::string& paramName = value[0];
        std::string& paramValue = value[1];

        if (paramName == "p")
            diff2.p = std::atof(paramValue.c_str());
        else if (paramName == "dt")
            diff2.dt = std::atof(paramValue.c_str());
        else if (paramName == "c")
            diff2.c = std::atof(paramValue.c_str());
        else if (paramName == "s")
            diff2.s = std::atof(paramValue.c_str());
        else if (paramName == "cg")
            diff2.cg = std::atof(paramValue.c_str());
        else if (paramName == "h")
            diff2.h = std::atof(paramValue.c_str());
        else if (paramName == "k")
            diff2.k = std::atof(paramValue.c_str());
        else if (paramName == "visible")
        {
            std::istringstream sstr(paramValue);
            sstr >> std::boolalpha >> diff2.visible;
        }
    }

    return diff2;
}

Diff3 Parse_Diffusion3_Params(std::string object)
{
    Diff3 diff2;
    std::vector<std::string> attribute = split_string(object, ",");
    size_t j;

    for (j = 0; j < attribute.size(); j++)
    {
        std::vector<std::string> value = split_string(attribute[j], ":");

        std::string& paramName = value[0];
        std::string& paramValue = value[1];

        if (paramName == "p")
            diff2.p = std::atof(paramValue.c_str());
        else if (paramName == "cg")
            diff2.cg = std::atof(paramValue.c_str());
        else if (paramName == "c")
            diff2.c = std::atof(paramValue.c_str());
        else if (paramName == "dt")
            diff2.dt = std::atof(paramValue.c_str());
        else if (paramName == "h")
            diff2.h = std::atof(paramValue.c_str());
        else if (paramName == "tau")
            diff2.tau = std::atof(paramValue.c_str());
        else if (paramName == "kpos")
            diff2.kpos = std::atof(paramValue.c_str());
        else if (paramName == "kneg")
            diff2.kneg = std::atof(paramValue.c_str());
        else if (paramName == "visible")
        {
            std::istringstream sstr(paramValue);
            sstr >> std::boolalpha >> diff2.visible;
        }
    }
    return diff2;
}

time_t Parse_Date(const std::string date)
{
    std::istringstream is(date);
    struct tm t = { 0 };
    int d, m, y, h, M, s;
    char delimiter;

    if ((is >> y >> delimiter >> m >> delimiter >> d >> delimiter >> h >> delimiter >> M >> delimiter >> s) && !is.fail())
    {
        t.tm_mday = d;
        t.tm_mon = m - 1;
        t.tm_year = y - 1900;
        t.tm_isdst = -1;
        t.tm_hour = h;
        t.tm_min = M;
        t.tm_sec = s;
    }

    return mktime(&t);
}

Value Parse_Values(std::string object)
{
    Value val;
    std::istringstream is(object);
    std::string date, value;
    char ch;
    std::string inputD;
    std::string inputV;

    if ((is >> inputD >> ch >> date >> ch >> inputV >> ch >> value) && !is.fail())
    {
        val.date = Parse_Date(date);

        if (inputD == "date")
            val.value = std::atof(value.c_str());
        else
            val.value = std::atof(date.c_str());
    }

    return val;
}

void Parse_Single_Vector(std::string& input, ValueVector& target)
{
    std::vector<std::string> objects;

    objects = split_string(input, "{", "}");

    for (size_t i = 0; i < objects.size(); i++)
    {
        Value v = Parse_Values(objects.at(i));

        target.push_back(v);
    }
}

double Parse_Input()
{
    size_t i;
    std::vector<std::string> objects;
    objects = split_string(par3, "{", "}");

    if (objects.empty())
        diff3.empty = true;

    for (i = 0; i < objects.size(); i++)
        diff3 = Parse_Diffusion3_Params(objects[i]);

    objects = split_string(par2, "{", "}");

    if (objects.empty())
        diff2.empty = true;

    for (i = 0; i < objects.size(); i++)
        diff2 = Parse_Diffusion2_Params(objects[i]);

    objects = split_string(ist, "{", "}");

    // initially set to out of range value (negative or very large) to indicate, that this parameter hasn't been set
    double max_value = -1;
    for (i = 0; i < objects.size(); i++)
    {
        Value v = Parse_Values(objects[i]);

        if (v.value > max_value)
            max_value = v.value;

        istVector.push_back(v);
    }

    objects = split_string(blood, "{", "}");

    for (i = 0; i < objects.size(); i++)
    {
        Value v = Parse_Values(objects.at(i));

        if (v.value > max_value)
            max_value = v.value;

        bloodVector.push_back(v);
    }

    Parse_Single_Vector(bloodCalibration, bloodCalibrationVector);

    Parse_Single_Vector(isig, isigVector);
    Parse_Single_Vector(insulin, insulinVector);
    Parse_Single_Vector(carbs, carbsVector);

    return max_value;
}

void Reset_Globals()
{
    istVector.clear();
    bloodVector.clear();
    bloodCalibrationVector.clear();
    isigVector.clear();
    insulinVector.clear();
    carbsVector.clear();
    diff2Result.clear();
    diff3Result.clear();
    diff3 = Diff3();
    diff2 = Diff2();
}

LocalizationMap Parse_Localization_Map(std::string strings)
{
    std::string find = "strings = ";
    std::size_t found = strings.find(find);

    if (found != std::string::npos)
        strings.erase(found, found + find.size());

    std::vector<std::string> removedString = split_string(strings, "{", "}");

    std::map<std::string, std::string> smap;
    if (removedString.empty())
        return smap;

    std::vector<std::string> attribute = split_string(removedString.at(0), ",");
    for (size_t i = 0; i < attribute.size(); i++)
    {
        std::vector<std::string> row = split_string(attribute.at(i), ":");
        smap.insert(std::pair<std::string, std::string>(row.at(0), row.at(1)));
    }

    return smap;
}

DataMap Prepare_Data_Map(int istVisible)
{
    bool istBool = istVisible ? true : false;

    if (!diff2.empty && diff2.visible)
        diff2Result = diffuse2(diff2, bloodVector, istVector);

    if (!diff3.empty && diff3.visible)
        diff3Result = diffuse3(diff3, bloodVector, istVector);

    DataMap vectorsMap;
    if (!istBool)
        istVector.clear();

    vectorsMap["ist"] = Data(istVector, istBool, false);
    vectorsMap["blood"] = Data(bloodVector, true, false);
    vectorsMap["bloodCalibration"] = Data(bloodCalibrationVector, true, false);
    vectorsMap["isig"] = Data(isigVector, true, false);
    vectorsMap["insulin"] = Data(insulinVector, true, false);
    vectorsMap["carbs"] = Data(carbsVector, true, false);
    vectorsMap["diff2"] = Data(diff2Result, diff2.visible, diff2.empty, true);
    vectorsMap["diff3"] = Data(diff3Result, diff3.visible, diff3.empty, true);

    return vectorsMap;
}

extern "C" const char* get_drawing_svg_result()
{
    return resultSvg.c_str();
}

extern "C" const char* get_drawing_csv_result()
{
    return resultCSV.c_str();
}

extern "C" long graph_main(char* ist2, char* blood2, char* bloodCalibration2, char* isig2, char* insulin2, char* carbs2, char* param2a, char* param3a, char* stringsJson, int mmol)
{
    Reset_Globals();

    ist = ist2;
    blood = blood2;
    bloodCalibration = bloodCalibration2;
    isig = isig2;
    insulin = insulin2;
    carbs = carbs2;
    par2 = param2a;
    par3 = param3a;
    std::stringstream stream;
    double max_value = Parse_Input();
    int istVisible = 1;

    DataMap vecMap = Prepare_Data_Map(istVisible);
    LocalizationMap strMap = Parse_Localization_Map(stringsJson);

    // generator scope
    {
        CGraph_Generator graph(vecMap, max_value, strMap, mmol);

        stream << graph.Build_SVG();
    }

    resultSvg = std::string(stream.str());
    have_data = true;

    return (long)resultSvg.size();
}

extern "C" long clark_main(char* ist2, char* blood2, char* bloodCalibration2, char* isig2, char* insulin2, char* carbs2, char* param2a, char* param3a, char* stringsJson, int mmol)
{
    Reset_Globals();

    ist = ist2;
    blood = blood2;
    bloodCalibration = bloodCalibration2;
    isig = isig2;
    insulin = insulin2;
    carbs = carbs2;
    par2 = param2a;
    par3 = param3a;
    std::stringstream stream;
    double max_value = Parse_Input();
    int istVisible = 1;

    DataMap vecMap = Prepare_Data_Map(istVisible);
    LocalizationMap strMap = Parse_Localization_Map(stringsJson);

    // generator scope
    {
        CClark_Generator clark(vecMap, max_value, strMap, mmol);

        stream << clark.Build_SVG();
    }

    resultSvg = std::string(stream.str());
    have_data = true;

    return (long)resultSvg.size();
}

extern "C" long parkes_main(char* ist2, char* blood2, char* bloodCalibration2, char* isig2, char* insulin2, char* carbs2, char* param2a, char* param3a, char* stringsJson, int type_diabetes, int mmol)
{
    Reset_Globals();

    ist = ist2;
    blood = blood2;
    bloodCalibration = bloodCalibration2;
    isig = isig2;
    insulin = insulin2;
    carbs = carbs2;
    par2 = param2a;
    par3 = param3a;
    std::stringstream stream;
    double max_value = Parse_Input();
    int istVisible = 1;
    bool type = type_diabetes == 1 ? true : false;

    DataMap vecMap = Prepare_Data_Map(istVisible);
    LocalizationMap strMap = Parse_Localization_Map(stringsJson);

    // generator scope
    {
        CParkes_Generator parkes(vecMap, max_value, strMap, mmol, type);

        stream << parkes.Build_SVG();
    }

    resultSvg = std::string(stream.str());
    have_data = true;

    return (long)resultSvg.size();
}

extern "C" long agp_main(char* ist2, char* blood2, char* bloodCalibration2, char* isig2, char* insulin2, char* carbs2, char* param2a, char* param3a, char* stringsJson, int istVisible, int mmol, int diff2Visible, int diff3Visible)
{
    Reset_Globals();

    ist = ist2;
    blood = blood2;
    bloodCalibration = bloodCalibration2;
    isig = isig2;
    insulin = insulin2;
    carbs = carbs2;
    par2 = param2a;
    par3 = param3a;
    std::stringstream stream;
    double max_value = Parse_Input();

    diff2.visible = diff2Visible;
    diff3.visible = diff3Visible;

    DataMap vecMap = Prepare_Data_Map(istVisible);
    LocalizationMap strMap = Parse_Localization_Map(stringsJson);

    // generator scope
    {
        CAGP_Generator agp(vecMap, max_value, strMap, mmol);

        stream << agp.Build_SVG();
    }

    resultSvg = std::string(stream.str());
    have_data = true;

    return (long)resultSvg.size();
}

extern "C" long day_main(char* ist2, char* blood2, char* bloodCalibration2, char* isig2, char* insulin2, char* carbs2, char* param2a, char* param3a, char* stringsJson, int mmol)
{
    Reset_Globals();

    ist = ist2;
    blood = blood2;
    bloodCalibration = bloodCalibration2;
    isig = isig2;
    insulin = insulin2;
    carbs = carbs2;
    par2 = param2a;
    par3 = param3a;

    std::stringstream stream;
    double max_value = Parse_Input();
    int istVisible = 1;

    DataMap vecMap = Prepare_Data_Map(istVisible);
    LocalizationMap strMap = Parse_Localization_Map(stringsJson);

    // generator scope
    {
        CDay_Generator day(vecMap, max_value, strMap, mmol);

        stream << day.Build_SVG();
    }

    resultSvg = std::string(stream.str());
    have_data = true;

    return (long)resultSvg.size();
}

extern "C" long csv_main(int istVisible, int bloodVisible, int bloodCalibrationVisible, int isigVisible, int insulinVisible, int carbsVisible, int par2Visible, int par3Visible, char* stringsJson)
{
    LocalizationMap strMap = Parse_Localization_Map(stringsJson);

    CCSV_Builder csvBldr(strMap);

    if (istVisible)
        csvBldr.Enable_Header_Field(CSV_Header_Fields::IST, &istVector);
    if (bloodVisible)
        csvBldr.Enable_Header_Field(CSV_Header_Fields::BLOOD, &bloodVector);
    if (bloodCalibrationVisible)
        csvBldr.Enable_Header_Field(CSV_Header_Fields::BLOOD_CALIBRATION, &bloodCalibrationVector);
    if (isigVisible)
        csvBldr.Enable_Header_Field(CSV_Header_Fields::ISIG, &isigVector);
    if (insulinVisible)
        csvBldr.Enable_Header_Field(CSV_Header_Fields::INSULIN, &insulinVector);
    if (carbsVisible)
        csvBldr.Enable_Header_Field(CSV_Header_Fields::CARBOHYDRATES, &carbsVector);
    if (par2Visible)
        csvBldr.Enable_Header_Field(CSV_Header_Fields::DIFF2, &diff2Result);
    if (par3Visible)
        csvBldr.Enable_Header_Field(CSV_Header_Fields::DIFF3, &diff3Result);

    resultCSV = csvBldr.Build();

    return (long)resultCSV.size();
}

// if the output is executable, include development methods like these
#ifdef OUTPUT_EXECUTABLE

bool Read_Testing_Values()
{
    std::ifstream myfile("json.txt");
    if (myfile.is_open())
    {
        getline(myfile, ist);
        //std::cout << ist;
        getline(myfile, blood);
        getline(myfile, par3);
        getline(myfile, par2);
        //getline(myfile, stringsJson);
        myfile.close();
        return true;
    }
    else
        return false;
}

int main(int argc, char ** argv)
{
    Reset_Globals();

    ist = "";
    blood = "";
    bloodCalibration = "";
    isig = "";
    insulin = "";
    carbs = "";
    par2 = "";
    par3 = "";
    stringsJson = "";
    Read_Testing_Values();

    std::stringstream stream;
    double max_value = Parse_Input();
    int istVisible = 1;
    int mmol = 1;

    DataMap vecMap = Prepare_Data_Map(istVisible);
    LocalizationMap strMap = Parse_Localization_Map(stringsJson);

    // generator scope
    {
        CAGP_Generator agp(vecMap, max_value, strMap, 1);

        stream << agp.Build_SVG();
    }

    std::ofstream svg("graph.svg");
    if (svg.is_open())
        svg << stream.str();
    else
        std::cout << "File not created";

    svg.close();
    std::cout << "END";

    return 0;
}

#endif
