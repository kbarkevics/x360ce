/*  x360ce - XBOX360 Controler Emulator
*  Copyright (C) 2002-2010 Racer_S
*  Copyright (C) 2010-2013 Robert Krawczyk
*
*  x360ce is free software: you can redistribute it and/or modify it under the terms
*  of the GNU Lesser General Public License as published by the Free Software Found-
*  ation, either version 3 of the License, or (at your option) any later version.
*
*  x360ce is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
*  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
*  PURPOSE.  See the GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License along with x360ce.
*  If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"
#include "globals.h"
#include "Utilities\Ini.h"
#include "Log.h"
#include "Utilities\Misc.h"
#include "Config.h"
#include "DirectInput.h"
#include "InputHook\InputHook.h"

bool g_bInitBeep = false;
bool g_bNative = false;
bool g_bDisable = false;

extern iHook* pHooks;

static const char* const buttonNames[] =
{
    "A",
    "B",
    "X",
    "Y",
    "Left Shoulder",
    "Right Shoulder",
    "Back",
    "Start",
    "Left Thumb",
    "Right Thumb",
};

static const char* const povNames[] =
{
    "D-pad Up",
    "D-pad Down",
    "D-pad Left",
    "D-pad Right"
};

static const char* const axisNames[] =
{
    "Left Analog X",
    "Left Analog Y",
    "Right Analog X",
    "Right Analog Y"
};

static const char* const axisDZNames[] =
{
    "Left Analog X DeadZone",
    "Left Analog Y DeadZone",
    "Right Analog X DeadZone",
    "Right Analog Y DeadZone",
};

static const char* const axisADZNames[] =
{
    "Left Analog X AntiDeadZone",
    "Left Analog Y AntiDeadZone",
    "Right Analog X AntiDeadZone",
    "Right Analog Y AntiDeadZone",
};

static const char* const axisLNames[] =
{
    "Left Analog X Linear",
    "Left Analog Y Linear",
    "Right Analog X Linear",
    "Right Analog Y Linear"
};

static const char* const axisBNames[] =
{
    "Left Analog X+ Button",
    "Left Analog X- Button",
    "Left Analog Y+ Button",
    "Left Analog Y- Button",
    "Right Analog X+ Button",
    "Right Analog X- Button",
    "Right Analog Y+ Button",
    "Right Analog Y- Button"
};

static const char* const padNames[] =
{
    "PAD1",
    "PAD2",
    "PAD3",
    "PAD4",
};

std::vector<Mapping> g_Mappings;

extern std::string exename;

DWORD ReadGameDatabase()
{
    Ini ini("x360ce.gdb");
    return ini.GetDword(exename.c_str(), "HookMask",0);
}

void ReadConfig()
{
    Ini ini("x360ce.ini");

    // Read global options
    g_bDisable = ini.GetBool("Options", "Disable",0);
    if(g_bDisable) return;

    g_bInitBeep = ini.GetBool("Options", "UseInitBeep",1);

    bool log = ini.GetBool("Options", "Log",0);
    bool con = ini.GetBool("Options", "Console",0);
    InitLog(log,con);

    // Simple Game Database support
    // InputHook
    bool overrride = ini.GetBool("InputHook", "Override",0);
    DWORD hookMask = ReadGameDatabase();
    if(hookMask && overrride == false)
    {
        pHooks->SetMask(hookMask);
        pHooks->Enable();
    }
    else
    {
        // InputHook
        hookMask = ini.GetDword("InputHook", "HookMask",0);
        if(hookMask)
        {
            pHooks->SetMask(hookMask);
            pHooks->Enable();
        }
        else
        {
            bool hookCheck = ini.GetBool("InputHook", "HookLL",0);
            if(hookCheck) pHooks->EnableHook(iHook::HOOK_LL);

            hookCheck = ini.GetBool("InputHook", "HookCOM",0);
            if(hookCheck) pHooks->EnableHook(iHook::HOOK_COM);

            hookCheck = ini.GetBool("InputHook", "HookDI",0);
            if(hookCheck) pHooks->EnableHook(iHook::HOOK_DI);

            hookCheck = ini.GetBool("InputHook", "HookVIDPID",0);
            if(hookCheck) pHooks->EnableHook(iHook::HOOK_VIDPID);

            hookCheck = ini.GetBool("InputHook", "HookSA",0);
            if(hookCheck) pHooks->EnableHook(iHook::HOOK_SA);

            hookCheck = ini.GetBool("InputHook", "HookNAME",0);
            if(hookCheck) pHooks->EnableHook(iHook::HOOK_NAME);

            hookCheck = ini.GetBool("InputHook", "HookSTOP",0);
            if(hookCheck) pHooks->EnableHook(iHook::HOOK_STOP);

            hookCheck = ini.GetBool("InputHook", "HookWT",0);
            if(hookCheck) pHooks->EnableHook(iHook::HOOK_WT);

            if(pHooks->GetMask()) pHooks->Enable();
        }
    }
    if(pHooks->CheckHook(iHook::HOOK_VIDPID))
    {
        DWORD vid = ini.GetDword("InputHook", "FakeVID",0x045E);
        DWORD pid = ini.GetDword("InputHook", "FakePID",0x028E);
        if(vid != 0x045E || pid != 0x28E) pHooks->SetFakePIDVID(MAKELONG(vid,pid));
    }

    // Read pad mappings
    for (DWORD i = 0; i < 4; ++i)
        ReadPadConfig(i, ini);
}


void ReadPadConfig(DWORD idx, Ini &ini)
{
    DWORD ret;
    char section[MAX_PATH] = "Mappings";
    char key[MAX_PATH];
    sprintf_s(key,"PAD%u",idx+1);

    char buffer[MAX_PATH];

    ret = ini.GetString(section, key, buffer);
    if(!ret) return;

#if _MSC_VER < 1700
    g_Devices.push_back(DInputDevice());
    DInputDevice& device = g_Devices.back();

    g_Mappings.push_back(Mapping());
    Mapping& mapping = g_Mappings.back();
#else
    g_Devices.emplace_back();
    DInputDevice& device = g_Devices.back();

    g_Mappings.emplace_back();
    Mapping& mapping = g_Mappings.back();
#endif

    //store value as section name
    strcpy_s(section,buffer);

    ini.GetString(section, "ProductGUID", buffer, 0);
    Misc::StringToGUID(buffer,device.productid);

    ini.GetString(section, "InstanceGUID", buffer, 0);
    Misc::StringToGUID(buffer,device.instanceid);

    device.passthrough = ini.GetBool(section, "PassThrough",1);

    if(device.passthrough)
    {
        mapping.enabled = false;
        g_Mappings.push_back(mapping);
        g_Devices.push_back(device);
        return;
    }

    if (!(IsEqualGUID(device.productid,GUID_NULL))
            && !(IsEqualGUID(device.instanceid,GUID_NULL)))
    {
        mapping.enabled = true;
    }
    else return;

    device.dwUserIndex = idx;

    device.useproduct = ini.GetBool(section, "UseProductGUID",0);
    device.swapmotor = ini.GetBool(section, "SwapMotor",0);
    device.triggerdeadzone = ini.GetLong(section, "TriggerDeadzone",0);
    device.useforce = ini.GetBool(section, "UseForceFeedback",0);
    device.gamepadtype = static_cast<BYTE>(ini.GetLong(section, "ControllerType",1));
    device.axistodpad = ini.GetBool(section, "AxisToDPad",0);
    device.a2ddeadzone = static_cast<INT>(ini.GetLong(section, "AxisToDPadDeadZone",0));
    device.a2doffset = static_cast<INT>(ini.GetLong(section, "AxisToDPadOffset",0));
    device.ff.type = (BYTE) ini.GetLong(section, "FFBType",0);
    device.ff.forcepercent = static_cast<FLOAT>(ini.GetLong(section, "ForcePercent",100) * 0.01);
    device.ff.leftPeriod = ini.GetLong(section, "LeftMotorPeriod",60);
    device.ff.rightPeriod = ini.GetLong(section, "RightMotorPeriod",20);

    mapping.guide = static_cast<WORD>(ini.GetLong(section, "GuideButton",0));

    //memset(mapping.Button,-1,sizeof(mapping.Button));

    for (INT i = 0; i < 2; ++i) mapping.Trigger[i].type = NONE;

    ///////////////////////////////////////////////////////////////////////////////////////
    for (INT i = 0; i < 2; ++i) mapping.Trigger[i].but = -1;

    ///////////////////////////////////////////////////////////////////////////////////////

    mapping.DpadPOV = (WORD) -1;

    for (INT i=0; i<10; ++i)
    {
        if (ini.GetLong(section,buttonNames[i],0) > 0)
        {
            mapping.Button[i] = static_cast<WORD>(ini.GetLong(section,buttonNames[i],0)) - 1;
        }
    }

    for (INT i=0; i<4; ++i)
    {
        if (ini.GetString(section, povNames[i], buffer) > 0)
        {
            int val = atoi(buffer);
            if(val == 0)
            {
                //for compatibility with x360ce.App
                if(strstr(buffer,"UP")) mapping.pov[i] = 36000;
                if(strstr(buffer,"DOWN")) mapping.pov[i] = 18000;
                if(strstr(buffer,"LEFT")) mapping.pov[i] = 27000;
                if(strstr(buffer,"RIGHT")) mapping.pov[i] = 9000;
                mapping.PovIsButton = false;
            }
            else if(val < 100)
            {
                mapping.pov[i] = static_cast<WORD>(val - 1);
                mapping.PovIsButton = true;
            }
            else
            {
                mapping.pov[i] = static_cast<WORD>(val);
                mapping.PovIsButton = false;
            }
        }

        if (ini.GetString(section, axisNames[i], buffer) > 0)
        {
            char* a = buffer;

            if (towlower(*a) == L's')   // Slider
            {
                mapping.Axis[i].analogType = SLIDER;
                ++a;
                mapping.Axis[i].id = atoi(a);
            }
            else
            {
                // Axis
                mapping.Axis[i].analogType = AXIS;
                mapping.Axis[i].id = atoi(a);
            }
        }

        SHORT tmp = static_cast<SHORT>(ini.GetLong(section, axisADZNames[i], 0));
        device.antideadzone[i] =  static_cast<SHORT>(((Misc::clamp))(tmp,0,32767));

        device.axisdeadzone[i] =  static_cast<SHORT>(ini.GetLong(section, axisDZNames[i], 0));
        device.axislinear[i] = static_cast<SHORT>(ini.GetLong(section, axisLNames[i], 0));

        INT ret = ini.GetLong(section, axisBNames[i*2]);
        if (ret > 0)
        {
            mapping.Axis[i].hasDigital = true;
            mapping.Axis[i].positiveButtonID = ret - 1;
        }
        ret = ini.GetLong(section, axisBNames[i*2+1]);
        if (ret > 0)
        {
            mapping.Axis[i].hasDigital = true;
            mapping.Axis[i].negativeButtonID = ret - 1;
        }
    }

    if (ini.GetString(section, "Left Trigger", buffer) > 0)
    {
        char* a = buffer;

        if ((mapping.Trigger[0].type = getTriggerType(a)) == DIGITAL)
        {
            mapping.Trigger[0].id = atoi(a) - 1;
        }
        else
        {
            ++a;
            mapping.Trigger[0].id = atoi(a);
        }
    }

    if (ini.GetString(section, "Right Trigger", buffer) > 0)
    {
        char* a = buffer;

        if ((mapping.Trigger[1].type = getTriggerType(a)) == DIGITAL)
        {
            mapping.Trigger[1].id = atoi(a) - 1;
        }
        else
        {
            ++a;
            mapping.Trigger[1].id = atoi(a);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////
    if (ini.GetString(section, "Left Trigger But", buffer) > 0)
    {
        char* a = buffer;
        mapping.Trigger[0].but = atoi(a) - 1;
    }

    if (ini.GetString(section, "Right Trigger But", buffer) > 0)
    {
        char* a = buffer;
        mapping.Trigger[1].but = atoi(a) - 1;
    }

    ///////////////////////////////////////////////////////////////////////////////////////

    if (ini.GetLong(section, "D-pad POV") > 0)
    {
        mapping.DpadPOV = static_cast<WORD>(ini.GetLong(section, "D-pad POV",0)) - 1;
    }
}

// NOTE: Letters corresponding to mapping types changed. Include in update notes.
MappingType getTriggerType(const char* s)
{
    if (tolower(*s) == 'a') return AXIS;	// Axis

    if (tolower(*s) == 's') return SLIDER;	// Slider

    if (tolower(*s) == 'x') return HAXIS;	// Half range axis

    if (tolower(*s) == 'h') return HSLIDER;	// Half range slider

    ////////////////////////////////////////////////////////////////////
    if (tolower(*s) == 'z') return CBUT;

    ////////////////////////////////////////////////////////////////////
    return DIGITAL;							// Digital
}
