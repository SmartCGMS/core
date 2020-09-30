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

#include "PacketStructures.h"

#include <string>

const wchar_t* memMapFileName_DataToSmartCGMS = L"t1dms_to_smartcgms_8F101568-5ED8-4E6E-AA4F-0B1A9980EA3D_Proc_Id_";
const wchar_t* memMapFileName_DataFromSmartCGMS = L"smartcgms_to_t1dms_03D8D819-F753-46D8-BB42-7EB06FFE9397_Proc_Id_";
const wchar_t* eventName_DataToSmartCGMS = L"t1dms_to_smartcgms_15CA6771-E5CD-45D3-99B6-A7208A644A8C_Proc_Id_";
const wchar_t* eventName_DataFromSmartCGMS = L"smartcgms_to_t1dms_7A29737B-146F-4B44-932B-245CAA801B9A_Proc_Id_";


TDMMS_IPC Establish_DMMS_IPC(const DWORD process_id) {
    auto make_id = [process_id](const wchar_t* str)->std::wstring {
        std::wstring result = str;
        result += std::to_wstring(process_id);
        return result;
    };

    TDMMS_IPC result;
       
    auto id_str = make_id(memMapFileName_DataToSmartCGMS);
    result.file_DataToSmartCGMS = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 256, id_str.c_str());
    id_str = make_id(memMapFileName_DataFromSmartCGMS);
    result.file_DataFromSmartCGMS = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 256, id_str.c_str());
    id_str = make_id(eventName_DataToSmartCGMS);
    result.event_DataToSmartCGMS = CreateEventW(NULL, TRUE, FALSE, id_str.c_str());
    id_str = make_id(eventName_DataFromSmartCGMS);
    result.event_DataFromSmartCGMS = CreateEventW(NULL, TRUE, FALSE, id_str.c_str());

    result.filebuf_DataToSmartCGMS = reinterpret_cast<LPTSTR>(MapViewOfFile(result.file_DataToSmartCGMS, FILE_MAP_ALL_ACCESS, 0, 0, 256));
    result.filebuf_DataFromSmartCGMS = reinterpret_cast<LPTSTR>(MapViewOfFile(result.file_DataFromSmartCGMS, FILE_MAP_ALL_ACCESS, 0, 0, 256));

    ResetEvent(result.event_DataToSmartCGMS);
    ResetEvent(result.event_DataFromSmartCGMS);

    return result;
}

void Clear_DMMS_IPC(TDMMS_IPC& dmms_ipc) {
    dmms_ipc.filebuf_DataToSmartCGMS = NULL;
    dmms_ipc.filebuf_DataFromSmartCGMS = NULL;
    dmms_ipc.file_DataToSmartCGMS = INVALID_HANDLE_VALUE;
    dmms_ipc.file_DataFromSmartCGMS = INVALID_HANDLE_VALUE;
    dmms_ipc.event_DataToSmartCGMS = INVALID_HANDLE_VALUE;
    dmms_ipc.event_DataFromSmartCGMS = INVALID_HANDLE_VALUE;
}

void Release_DMMS_IPC(TDMMS_IPC &dmms_ipc) {
    if (dmms_ipc.filebuf_DataToSmartCGMS)
        UnmapViewOfFile(dmms_ipc.filebuf_DataToSmartCGMS);
    if (dmms_ipc.filebuf_DataFromSmartCGMS)
        UnmapViewOfFile(dmms_ipc.filebuf_DataFromSmartCGMS);

    if (dmms_ipc.file_DataToSmartCGMS)
        CloseHandle(dmms_ipc.file_DataToSmartCGMS);
    if (dmms_ipc.file_DataFromSmartCGMS)
        CloseHandle(dmms_ipc.file_DataFromSmartCGMS);
    if (dmms_ipc.event_DataToSmartCGMS)
        CloseHandle(dmms_ipc.event_DataToSmartCGMS);
    if (dmms_ipc.event_DataFromSmartCGMS)
        CloseHandle(dmms_ipc.event_DataFromSmartCGMS);

    Clear_DMMS_IPC(dmms_ipc);
}
