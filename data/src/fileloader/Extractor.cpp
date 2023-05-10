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

#include "../../../../common/lang/dstrings.h"
#include "../../../../common/iface/DeviceIface.h"
#include "Extractor.h"
#include "time_utils.h"
#include "../../../../common/rtl/rattime.h"
#include "../../../../common/utils/string_utils.h"

#include <array>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <map>

template <typename TPosition>
struct TSeries_Source {
	TPosition position;
	TCell_Descriptor cell;
};

template <typename TPosition>
using TCursor = std::vector<TSeries_Source<TPosition>>;

template <typename TPosition>
using TCursors = std::vector<TCursor<TPosition>>;

//sheet format assumes single date time for entire row => just one cursor
TCursors<TSheet_Position> Break_Sheet_Layout_To_Cursors(CFormat_Layout& layout) {
	TCursor<TSheet_Position> cursor;
	for (auto elem : layout) {
		TSheet_Position pos = CellSpec_To_RowCol(elem.cursor_position);
		cursor.push_back(TSeries_Source<TSheet_Position>{ pos, elem });
	}

	return TCursors<TSheet_Position>{cursor};
}


//xml/hierarchical format is more tricky as each element can has its own datetime
//we need to sort them by path
TCursors<TXML_Position> Break_XML_Layout_To_Cursors(CFormat_Layout& layout) {
	TCursors<TXML_Position> result;

	std::map<std::string, std::vector<TCell_Descriptor>> cells;
	for (auto elem : layout) {
		auto key = elem.cursor_position;	//remove everything after the last dot as dot delimits the parameter
		auto offset = key.find_last_of('.');
		if (offset != key.npos)
			key.resize(offset);
		cells[key].push_back(elem);
	}

	for (auto& precursor : cells) {
		TCursor<TXML_Position> local_cursor;

		for (auto elem : precursor.second) {
			TXML_Position pos = CellSpec_To_TreePosition(elem.cursor_position);
			local_cursor.push_back(TSeries_Source<TXML_Position>{ pos, elem });
		}

		result.push_back(std::move(local_cursor));
	}

	return result;
}




template <typename TPosition>
CMeasured_Levels Extract_Series(CFormat_Adapter& source, TCursors<TPosition>& cursors)  {

	CMeasured_Levels result;
	double datetime = std::numeric_limits<double>::quiet_NaN();
	double date_part = 0.0, time_part = 0.0;

	source.Set_Cache_Mode(NCache_Mode::Single_Record_Cache);

	for (auto& cursor : cursors) {

		bool read_anything = false;	//keeping compiler happy
		do {
			read_anything = false;	//clear the cursor-watchdog
			CMeasured_Values_At_Single_Time mval;
			std::string comments;	//comments aggregation

			for (auto& elem : cursor) {	//this has to be non-const reference, otherwise we will be creating the expression convertor on and on as there's lazy init
				const GUID& sig = elem.cell.series.target_signal;

				const auto str_val_opt = source.Read<TPosition>(elem.position);

				if (str_val_opt.has_value()) {
					read_anything = true;	//signal the cursor-watchdog
					const std::string str_val = trim(str_val_opt.value());

					if (sig == signal_Comment) {
						if (!comments.empty())
							comments += "; ";
						
						if (!elem.cell.series.comment_name.empty()) {
							comments += elem.cell.series.comment_name;
							comments += ": ";
						}
						comments += str_val;
					}
					else if (sig == signal_Date_Only) {
						date_part = Local_Time_Str_To_Rat_Time(str_val, elem.cell.series.datetime_format.c_str());
					}
					else if (sig == signal_Date_Time) {
						datetime = Local_Time_Str_To_Rat_Time(str_val, elem.cell.series.datetime_format.c_str());
					}
					else if (sig == signal_Time_Only) {
						time_part = Local_Time_Str_To_Rat_Time(str_val, elem.cell.series.datetime_format.c_str());
					}
					else {
						double val = std::numeric_limits<double>::quiet_NaN();

						//are we reading a time marker?
						if (!elem.cell.series.datetime_format.empty()) {
							val = Local_Time_Str_To_Rat_Time(str_val, elem.cell.series.datetime_format.c_str());
						} else {
							//we are reading a generic signal, double value
							bool ok = false;
							val = str_2_dbl(str_val.c_str(), ok);
							if (ok) 
								val = elem.cell.series.conversion.eval(val);
						}

						if (!std::isnan(val))
							mval.push(sig, val);
					}
				}

				elem.position.Forward();
			}

			if (!comments.empty())
				mval.push(signal_Comment, comments);

			if (std::isnan(datetime)) {	//some formats give date and time in separate series
				datetime = date_part + time_part;
				if (datetime == 0.0) //did we got at least one of the date & time series?
					datetime = std::numeric_limits<double>::quiet_NaN();
			}

			if (!std::isnan(datetime)) {
				mval.set_measured_at(datetime);

				//check that mval actually contains any value other than the time, which is always required - done in CMeasuredLevels::update
				result.update(mval);
			}
			
		} while (read_anything);	//EOF proved to be a bad choice due to XML where we disabled adverse effect of modifying xml pos on reading, which should be const only
		//so, we check if the cursors provided at least one value
	}

	return result;
}


CMeasured_Levels Extract_From_File(CFormat_Adapter& source, const CFile_Format_Rules& format_rules) {

	auto layout = format_rules.Format_Layout(source.Format_Name());
	if (layout.has_value()) {
		source.Reset_EOF();

		switch (source.Get_File_Organization()) {
			case NFile_Organization_Structure::SPREADSHEET: {
				auto cursors = Break_Sheet_Layout_To_Cursors(layout.value());
				return Extract_Series<TSheet_Position>(source, cursors);
			
			}
			case NFile_Organization_Structure::HIERARCHY: {
				auto cursors = Break_XML_Layout_To_Cursors(layout.value());
				return Extract_Series<TXML_Position>(source, cursors);
			}
			default:
				return CMeasured_Levels{};	//empty object means no data
		}
	}
	else {
		return CMeasured_Levels{ };	//error, which we detect while loading the format descriptors
	}
}
