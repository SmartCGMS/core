/**
 * Portal for online glucose level calculation
 * Final calculation and SVG generating part
 * https://diabetes.zcu.cz/
 *
 * Author: Marek Rasocha (mrasocha@students.zcu.cz)
 * Maintainer: Martin Ubl (ublm@students.zcu.cz)
 */

#pragma once

#include <ctime>
#include <map>
#include <vector>

typedef std::map<std::string, std::string> LocalizationMap;

struct Value
{
    Value(double _value = 0.0, time_t _date = 0, uint64_t _segment_id = 0);
    ~Value();

    double Get_Value();
    void Set_Value(double _value);

    double value;
    time_t date;
    time_t normalize;
    uint64_t segment_id;
};

typedef std::vector<Value> ValueVector;

struct Coordinate
{
    Coordinate(double _x, double _y);
    ~Coordinate();

    double x, y;
};

class Stats
{
    private:
        std::vector<double> mAbsuluteError;
        std::vector<double> mRelativeError;
        double mSumAbsolute;
        double mSumRelative;
        bool mIsSorted;
        bool mPrintStats;

    public:
        Stats();
        ~Stats();

        void Sort();
        void Quartile_Rel(double& q0, double& q1, double& q2, double& q3, double& q4);
        double Avarage_Rel() const;
        void Quartile_Abs(double& q0, double& q1, double& q2, double& q3, double& q4);
        double Avarage_Abs() const;
        void Next_Error(double measured, double counted);

        void Set_Print_Stats(bool state);
        bool Get_Print_Stats() const;
};

struct Data
{
    Data();
    ~Data();
    Data(ValueVector _values, bool _visible, bool _empty, bool _calculated = false);

    bool Is_Visible();
    bool Is_Visible_Legend();

    ValueVector values;
    bool visible;
    bool empty;
	bool calculated;
	std::string identifier;
};

typedef std::map<std::string, Data> DataMap;
