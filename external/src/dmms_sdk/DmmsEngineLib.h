//*************************************************************************************************
//                Revision Record
//  Date         Issue      Author               Description
// ----------  ---------  ---------------  --------------------------------------------------------
// 09/12/2019  DMMSD-288  Yung-Yeh Chang    Original Release
// 11/12/2019  DMMSD-288  Yung-Yeh Chang    Adapt the COM-like model
//*************************************************************************************************

#ifndef DMMSENGLIB_H
#define DMMSENGLIB_H

#if defined(LIBBUILD)
#  define DMMSSHARED_EXPORT __declspec(dllexport)
#else
#  define DMMSSHARED_EXPORT __declspec(dllimport)
#endif

#include "DmmsEngineDef.h"

/*
 * A dynamic library callback approach that allows a user to run a simulation with DMMS as a dynamic
 * library in other C++ project(currently, DLL for PC only).A user is able to run DMMS in other C++
 * project. This header file defines the interface for DMMS engine object. Use getDmmsEngine() to
 * construct a DMMS engine that utilize a callback plugin interface (ISimCallbacks) to run as one of the
 * feedback control elements (plug-ins) in the simulation.
 */

class IDmms
{
public:
    /* Run DMMS with desginated Rx plan xml file and a feedback control element plugin (defined as a <ISimCallbacks> object)
     * cfgPath - The complete filepath of a config xml from which DMMS generates.
     * logPath - The complete filepath to save the log file that is generated from a simulation.
     * resultPath - The directory to save the results
     * callbackObj - The ISimCallbacks object
     * Note: After simulation is completed, run IDmms::close() to terminate the engine.
     */
    virtual int runSim(const char *cfgPath,const char *logPath,const char *resultPath, ISimCallbacks* const callbackObj) = 0;
    virtual void close() = 0;
};

//For convenience, define IDmmsFactory as a pointer to a function returning a pointer to an IDmms.
//This can be used as a pointer to the getDmmsEngine() function provided by the DMMS DLL.
typedef IDmms* (*IDmmsFactory)();

// Simulator object factory
extern "C" DMMSSHARED_EXPORT IDmms* __stdcall getDmmsEngine();

#endif // DMMSENGLIB_H
