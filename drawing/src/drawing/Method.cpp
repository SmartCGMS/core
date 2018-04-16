/**
 * Portal for online glucose level calculation
 * Final calculation and SVG generating part
 * https://diabetes.zcu.cz/
 *
 * Author: Marek Rasocha (mrasocha@students.zcu.cz)
 * Maintainer: Martin Ubl (ublm@students.zcu.cz)
 */

#include "general.h"

#include "Method.h"
#include "Misc/Utility.h"

#include <limits>

double solveBloodLevelDiffusion3(double present_value, double derivate, double future_value, double cg, double p, double tauP, double c)
{
    double blood;
    double alpha = cg;
    double beta = p - (alpha*present_value);
    double tau = tauP; // u diff3 je v diff i tau
    double gamma = tau*derivate + c - future_value;
    double D = beta * beta - 4.0 * alpha * gamma; // diskriminant
    bool nzalpha = (fabs(alpha) > 0.0);

    if (D >= 0.0 && nzalpha)
        blood = (-beta + sqrt(D)) * 0.5 / alpha;
    else
    {
        if (nzalpha)
            blood = -beta * 0.5 / alpha;
        else
        {
            if (fabs(beta) >= 0.0)
                blood = -gamma / beta;
            else
                blood = tau * derivate + c;
        }
    }
    return blood;
}

bool istNumericDerivation(int index, std::vector<Value> &ist, double &derivate)
{
    if ((index < 0) || ((size_t)(index + 1) >= ist.size()))
        return false;

    Value& v = ist[index];
    Value& next = ist[index + 1];

    // subtract times
    derivate = (next.value + v.value) / (2.0 * (next.value - v.date));

    return true;
}

std::vector<Value> diffuse2(Diff2 &param, std::vector<Value> &blood, std::vector<Value> &ist)
{
    std::vector<Value> predicted;
    param.h = param.h * (double)SECONDS_IN_DAY;
    param.dt = param.dt * (double)SECONDS_IN_DAY;

    for (size_t i = 0; i < ist.size(); i++)
    {
        Value& v = ist[i];

        double appxIstMinHLevel = Utility::Find_Ist_Level(Utility::Time_To_Double(v.date) - param.h, ist);
        if (appxIstMinHLevel < 0)
            continue;

        double future_time = Utility::Time_To_Double(v.date) + param.dt;
        if (param.h != 0)
            future_time += param.k * v.value * (v.value - appxIstMinHLevel) / param.h;

        double future_value = Utility::Find_Ist_Level(future_time, ist);
        if (future_value < 0)
            continue;

        double blood_value = solveBloodLevelDiffusion3(v.value, 0, future_value, param.cg, param.p, 0, param.c);

        Value s;
        s.date = v.date;
        s.value = blood_value;

        predicted.push_back(s);
    }

    return predicted;
}

std::vector<Value> diffuse3(Diff3 &param, std::vector<Value> &blood, std::vector<Value> &ist)
{
    std::vector<Value> predicted;
    param.h *= (double)SECONDS_IN_DAY;
    param.dt *= (double)SECONDS_IN_DAY;

    for (size_t i = 0; i < ist.size(); i++)
    {
        Value& v = ist[i];

        double appxIstMinHLevel = Utility::Find_Ist_Level(v.date - param.h, ist);
        if (appxIstMinHLevel < 0)
            continue;

        double derivedIst;
        if (!istNumericDerivation((int)i, ist, derivedIst))
            continue;

        double psi = v.value - appxIstMinHLevel;
        if (psi >= 0.0)
            psi *= param.kpos;
        else
            psi *= param.kneg;

        double predictedTime = Utility::Time_To_Double(v.date) + param.dt + v.value * psi;
        double future_value = Utility::Find_Ist_Level(predictedTime, ist);

        if (future_value < 0)
            continue;

        double blood_value = solveBloodLevelDiffusion3(v.value, derivedIst, future_value, param.cg, param.p, param.tau, param.c);
        if (blood_value > 0)
        {
            Value s;
            s.date = v.date;
            s.value = blood_value;
            predicted.push_back(s);
        }
    }

    return predicted;
}
