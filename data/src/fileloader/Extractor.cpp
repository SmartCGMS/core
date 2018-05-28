#include "../../../../common/lang/dstrings.h"
#include "Extractor.h"
#include "TimeRoutines.h"
#include "../../../../common/rtl/rattime.h"

#include <sstream>
#include <iomanip>
#include <ctime>

const char* rsExtractorColumnDate = "date";
const char* rsExtractorColumnTime = "time";
const char* rsExtractorColumnDatetime = "datetime";
const char* rsExtractorColumnIsig = "isig";
const char* rsExtractorColumnIst = "ist";
const char* rsExtractorColumnBlood = "blood";
const char* rsExtractorColumnBloodDate = "blood-date";
const char* rsExtractorColumnBloodTime = "blood-time";
const char* rsExtractorColumnBloodDatetime = "blood-datetime";
const char* rsExtractorColumnBloodCalibration = "blood-calibration";
const char* rsExtractorColumnDevice = "device";
const char* rsExtractorColumnInsulin = "insulin";
const char* rsExtractorColumnCarbohydrates = "carbohydrates";
const char* rsExtractorColumnEvent = "event";
const char* rsExtractorColumnEventDate = "event-date";
const char* rsExtractorColumnEventTime = "event-time";
const char* rsExtractorColumnEventDatetime = "event-datetime";
const char* rsExtractorColumnCondition = "event-condition";

const char* dsDatabaseTimestampFormatShort = "%FT%T";

// recognized datetime format formatter strings
const std::array<const char*, static_cast<size_t>(NKnownDateFormat::UNKNOWN_DATEFORMAT)> KnownDateFormatFmtStrings {
	"%d/%m/%Y %H:%M",        // DATEFORMAT_DDMMYYYY
	"%Y/%m/%d %H:%M",        // DATEFORMAT_YYYYMMDD
	"%d.%m.%Y %H:%M",        // DATEFORMAT_CZ
	"%Y-%m-%dT%H:%M:%S",     // DATEFORMAT_DB_YYYYMMDD_T
	"%Y-%m-%d %H:%M:%S",     // DATEFORMAT_DB_YYYYMMDD
};


NKnownDateFormat Recognize_Date_Format(const wchar_t *str) {
	if (str == nullptr) return NKnownDateFormat::UNKNOWN_DATEFORMAT;
	std::string date_time_str{ str, str + wcslen(str) };
	return Recognize_Date_Format(date_time_str);
};

// Recognizes date format from supplied string representation
NKnownDateFormat Recognize_Date_Format(std::string& str)
{
	size_t i;
	std::tm temp;
	memset(&temp, 0, sizeof(std::tm));

	// TODO: slightly rework this method to safely check for parse failure even in debug mode

	for (i = 0; i < (size_t)NKnownDateFormat::UNKNOWN_DATEFORMAT; i++)
	{
		std::istringstream ss(str);
		try
		{
			ss >> std::get_time(&temp, KnownDateFormatFmtStrings[i]);
		}
		catch (...)
		{
			continue;
		}

		// conversion result is not valid - try next format
		if (!Is_Valid_Tm(temp))
			continue;

		break;
	}

	return static_cast<NKnownDateFormat>(i);
}

bool Str_Time_To_Unix_Time(const wchar_t *src, NKnownDateFormat fmtIdx, std::string outFormatStr, const char* outFormat, time_t& target) {
	if (src == nullptr) return false;
	std::string tmp{ src, src + wcslen(src) };
	return Str_Time_To_Unix_Time(tmp, fmtIdx, outFormatStr, outFormat, target);
}

bool Str_Time_To_Unix_Time(const std::string& src, NKnownDateFormat fmtIdx, std::string outFormatStr, const char* outFormat, time_t& target)
{
	return Convert_Timestamp(src, KnownDateFormatFmtStrings[static_cast<size_t>(fmtIdx)], outFormatStr, outFormat, &target);
}

CExtractor::CExtractor()
{
	mColumnTypes[rsExtractorColumnDate] = ExtractorColumns::COL_DATE;
	mColumnTypes[rsExtractorColumnTime] = ExtractorColumns::COL_TIME;
	mColumnTypes[rsExtractorColumnDatetime] = ExtractorColumns::COL_DATETIME;
	mColumnTypes[rsExtractorColumnIsig] = ExtractorColumns::COL_ISIG;
	mColumnTypes[rsExtractorColumnIst] = ExtractorColumns::COL_IST;
	mColumnTypes[rsExtractorColumnBlood] = ExtractorColumns::COL_BLOOD;
	mColumnTypes[rsExtractorColumnBloodDate] = ExtractorColumns::COL_BLOOD_DATE;
	mColumnTypes[rsExtractorColumnBloodTime] = ExtractorColumns::COL_BLOOD_TIME;
	mColumnTypes[rsExtractorColumnBloodDatetime] = ExtractorColumns::COL_BLOOD_DATETIME;
	mColumnTypes[rsExtractorColumnDevice] = ExtractorColumns::COL_DEVICE;
	mColumnTypes[rsExtractorColumnInsulin] = ExtractorColumns::COL_INSULIN;
	mColumnTypes[rsExtractorColumnCarbohydrates] = ExtractorColumns::COL_CARBOHYDRATES;
	mColumnTypes[rsExtractorColumnEvent] = ExtractorColumns::COL_EVENT;
	mColumnTypes[rsExtractorColumnEventDate] = ExtractorColumns::COL_EVENT_DATE;
	mColumnTypes[rsExtractorColumnEventTime] = ExtractorColumns::COL_EVENT_TIME;
	mColumnTypes[rsExtractorColumnEventDatetime] = ExtractorColumns::COL_EVENT_DATETIME;
	mColumnTypes[rsExtractorColumnCondition] = ExtractorColumns::COL_EVENT_CONDITION;
	mColumnTypes[rsExtractorColumnBloodCalibration] = ExtractorColumns::COL_BLOOD_CALIBRATION;
}

void CExtractor::Add_Template(const char* rule, const char* header)
{
	// the column must have column identifier specified in known extractor column map
	if (mColumnTypes.find(header) == mColumnTypes.end())
		return;

	mRuleTemplates[rule] = mColumnTypes[header];
}

void CExtractor::Add_Template_Multiplier(const char* rule, const char* header, double multiplier)
{
	// the column must have column identifier specified in known extractor column map
	if (mColumnTypes.find(header) == mColumnTypes.end())
		return;

	mRuleMultipliers[rule] = multiplier;
}

void CExtractor::Add_Template_String_Format(const char* rule, const char* header, const char* stringFormat)
{
	// the column must have column identifier specified in known extractor column map
	if (mColumnTypes.find(header) == mColumnTypes.end())
		return;

	// the formatter %f must be present
	if (std::string(stringFormat).find("%f") == std::string::npos)
		return;

	mRuleStringFormats[rule] = stringFormat;
}

bool CExtractor::Add_Format_Rule(const char* formatName, const char* cellLocation, const char* ruleName)
{
	// the rule must exist at the time of addition
	if (mRuleTemplates.find(ruleName) == mRuleTemplates.end())
		return false;

	// '?' character at the beginning marks condition rule
	if (cellLocation[0] == '?')
		return Add_Format_Condition_Rule(formatName, cellLocation, ruleName);

	mRuleSet[formatName][cellLocation] = ruleName;
	return true;
}

bool CExtractor::Add_Format_Condition_Rule(const char* formatName, const char* condition, const char* headerRuleName)
{
	std::string condStr(&condition[1]); // cut the initial '?' mark

	// find splitter
	size_t pos = condStr.find('>');
	if (pos == std::string::npos)
		return false;

	// extract rule
	std::string condRule = condStr.substr(0, pos);

	// find rule in ruleset - it has to exist

	auto fitr = mRuleSet.find(formatName);
	if (fitr == mRuleSet.end())
		return false;

	ExtractorColumns col = ExtractorColumns::NONE;

	for (auto itr = fitr->second.begin(); itr != fitr->second.end(); ++itr)
	{
		if (mRuleTemplates.find(itr->second)->first == condRule)
		{
			col = mRuleTemplates.find(itr->second)->second;
			break;
		}
	}

	if (col == ExtractorColumns::NONE)
		return false;

	// extract condition value
	std::string condValue = condStr.substr(pos + 1);

	// find column to resolve condition into
	auto hitr = mRuleTemplates.find(headerRuleName);
	if (hitr == mRuleTemplates.end())
		return false;

	// and finally add condition rule
	mConditionRuleSet[formatName][col][condValue] = hitr->second;
	mConditionReverseRuleSet[formatName][col][condValue] = headerRuleName;

	return true;
}

ExtractorColumns CExtractor::Get_Conditional_Column(std::string& formatName, ExtractorColumns condColType, std::string& condValue) const
{
	auto fitr = mConditionRuleSet.find(formatName);
	if (fitr == mConditionRuleSet.end())
		return ExtractorColumns::NONE;

	auto ctitr = fitr->second.find(condColType);
	if (ctitr == fitr->second.end())
		return ExtractorColumns::NONE;

	auto vitr = ctitr->second.find(condValue);
	if (vitr == ctitr->second.end())
		return ExtractorColumns::NONE;

	return vitr->second;
}

const char* CExtractor::Find_ColSpec_For_Column(const std::string& formatName, ExtractorColumns colType) const
{
	auto fitr = mRuleSet.find(formatName);
	if (fitr == mRuleSet.end())
		return nullptr;

	for (auto itr = fitr->second.begin(); itr != fitr->second.end(); ++itr)
	{
		if (mRuleTemplates.find(itr->second)->second == colType)
			return itr->first.c_str();
	}

	return nullptr;
}

void CExtractor::Fill_SheetPosition_For(std::string& formatName, ExtractorColumns colType, SheetPosition& target) const
{
	const char* tmp = Find_ColSpec_For_Column(formatName, colType);
	if (tmp)
		CFormat_Adapter::CellSpec_To_RowCol(tmp, target.row, target.column, target.sheetIndex);
}

void CExtractor::Fill_TreePosition_For(std::string& formatName, ExtractorColumns colType, TreePosition& target) const
{
	const char* tmp = Find_ColSpec_For_Column(formatName, colType);
	if (tmp)
		CFormat_Adapter::CellSpec_To_TreePosition(tmp, target);

	// if no blood-date/time/datetime column found, try to reach ist-date/time/datetime columns
	if (!target.Valid())
	{
		switch (colType)
		{
			case ExtractorColumns::COL_BLOOD_DATE:
				Fill_TreePosition_For(formatName, ExtractorColumns::COL_DATE, target);
				break;
			case ExtractorColumns::COL_BLOOD_TIME:
				Fill_TreePosition_For(formatName, ExtractorColumns::COL_TIME, target);
				break;
			case ExtractorColumns::COL_BLOOD_DATETIME:
				Fill_TreePosition_For(formatName, ExtractorColumns::COL_DATETIME, target);
				break;
		}
	}
}

double CExtractor::Get_Column_Multiplier(std::string& formatName, ExtractorColumns colType) const
{
	auto fitr = mRuleSet.find(formatName);
	if (fitr == mRuleSet.end())
		return 1.0;

	for (auto itr = fitr->second.begin(); itr != fitr->second.end(); ++itr)
	{
		if (mRuleTemplates.find(itr->second)->second == colType)
		{
			if (mRuleMultipliers.find(itr->second) != mRuleMultipliers.end())
				return mRuleMultipliers.find(itr->second)->second;
			else
				return 1.0;
		}
	}

	// search also in conditional fields

	auto citr = mConditionRuleSet.find(formatName);
	if (citr != mConditionRuleSet.end())
	{
		for (auto srcCol : citr->second)
		{
			for (auto cond : srcCol.second)
			{
				if (cond.second == colType)
				{
					auto a = mConditionReverseRuleSet.find(formatName);
					auto b = a->second.find(srcCol.first);
					auto c = b->second.find(cond.first);

					auto ritr = mRuleMultipliers.find(c->second);
					if (ritr != mRuleMultipliers.end())
						return ritr->second;
					else
						return 1.0;
				}
			}
		}
	}

	return 1.0;
}

const char* CExtractor::Get_Column_String_Format(std::string& formatName, ExtractorColumns colType) const
{
	auto fitr = mRuleSet.find(formatName);
	if (fitr == mRuleSet.end())
		return nullptr;

	for (auto itr = fitr->second.begin(); itr != fitr->second.end(); ++itr)
	{
		if (mRuleTemplates.find(itr->second)->second == colType)
		{
			if (mRuleStringFormats.find(itr->second) != mRuleStringFormats.end())
				return mRuleStringFormats.find(itr->second)->second.c_str();
			else
				return nullptr;
		}
	}

	// search also in conditional fields

	auto citr = mConditionRuleSet.find(formatName);
	if (citr != mConditionRuleSet.end())
	{
		for (auto srcCol : citr->second)
		{
			for (auto cond : srcCol.second)
			{
				if (cond.second == colType)
				{
					auto a = mConditionReverseRuleSet.find(formatName);
					auto b = a->second.find(srcCol.first);
					auto c = b->second.find(cond.first);

					auto ritr = mRuleStringFormats.find(c->second);
					if (ritr != mRuleStringFormats.end())
						return ritr->second.c_str();
					else
						return nullptr;
				}
			}
		}
	}

	return nullptr;
}

bool CExtractor::Formatted_Read_Double(std::string& formatName, ExtractorColumns colType, std::string& source, double& target) const
{
	const char* fmt = Get_Column_String_Format(formatName, colType);

	if (!fmt)
	{
		try
		{
			target = std::stod(source);
		}
		catch (...)
		{
			return false;
		}

		return true;
	}

	size_t spos = 0, dpos = 0;

	// parse formatted input
	while (spos < source.length() && dpos < strlen(fmt))
	{
		// when we reached format string...
		if (fmt[dpos] == '%' && fmt[dpos + 1] == 'f')
		{
			dpos += 2;

			size_t ppos = spos;
			// iterate to first equal character
			while (spos < source.length() && source[spos] != fmt[dpos])
				spos++;

			// characters are not equal / buffer overrun, no match
			if (spos == source.length() && source[spos] != fmt[dpos])
				return false;

			// read and convert value
			std::string valsub = source.substr(ppos, spos - ppos);

			try
			{
				target = std::stod(valsub);
			}
			catch (...)
			{
				return false;
			}
		}
		else
		{
			// match non-formatted characters
			if (source[spos] != fmt[dpos])
				return false;

			spos++;
			dpos++;
		}
	}

	return true;
}

bool CExtractor::Extract(std::string& formatName, CFormat_Adapter& source, ExtractionResult& result, size_t fileIndex) const
{
	source.Reset_EOF();

	switch (source.Get_File_Organization())
	{
		case FileOrganizationStructure::SPREADSHEET:
			return Extract_Spreadsheet_File(formatName, source, result, fileIndex);
		case FileOrganizationStructure::HIERARCHY:
			return Extract_Hierarchy_File(formatName, source, result, fileIndex);
		default:
			return false;
	}
}

bool CExtractor::Extract_Hierarchy_File(std::string& formatName, CFormat_Adapter& source, ExtractionResult& result, size_t fileIndex) const
{
	TreePosition datePos, timePos, datetimePos, dateBloodPos, timeBloodPos, datetimeBloodPos, isigPos, istPos, bloodPos,
		eventPos, dateEventPos, timeEventPos, datetimeEventPos;

	Fill_TreePosition_For(formatName, ExtractorColumns::COL_DATE, datePos);
	Fill_TreePosition_For(formatName, ExtractorColumns::COL_TIME, timePos);
	if (!datePos.Valid() || !timePos.Valid())
		Fill_TreePosition_For(formatName, ExtractorColumns::COL_DATETIME, datetimePos);
	Fill_TreePosition_For(formatName, ExtractorColumns::COL_ISIG, isigPos);
	Fill_TreePosition_For(formatName, ExtractorColumns::COL_IST, istPos);
	Fill_TreePosition_For(formatName, ExtractorColumns::COL_BLOOD, bloodPos);
	Fill_TreePosition_For(formatName, ExtractorColumns::COL_EVENT, eventPos);

	Fill_TreePosition_For(formatName, ExtractorColumns::COL_BLOOD_DATE, dateBloodPos);
	Fill_TreePosition_For(formatName, ExtractorColumns::COL_BLOOD_TIME, timeBloodPos);
	if (!dateBloodPos.Valid() || !timeBloodPos.Valid())
		Fill_TreePosition_For(formatName, ExtractorColumns::COL_BLOOD_DATETIME, datetimeBloodPos);

	Fill_TreePosition_For(formatName, ExtractorColumns::COL_EVENT_DATE, dateEventPos);
	Fill_TreePosition_For(formatName, ExtractorColumns::COL_EVENT_TIME, timeEventPos);
	if (!dateEventPos.Valid() || !timeEventPos.Valid())
		Fill_TreePosition_For(formatName, ExtractorColumns::COL_EVENT_DATETIME, datetimeEventPos);

	bool status = true;

	// extract IST if available
	if (istPos.Valid() && (datetimePos.Valid() || (datePos.Valid() && timePos.Valid())))
		status = status && Extract_Hierarchy_File_Stream(istPos, isigPos, datetimePos, datePos, timePos, ExtractionIterationType::IST, formatName, source, result, fileIndex);

	if (bloodPos.Valid() && (datetimeBloodPos.Valid() || (dateBloodPos.Valid() && timeBloodPos.Valid())))
		status = status && Extract_Hierarchy_File_Stream(bloodPos, isigPos, datetimeBloodPos, dateBloodPos, timeBloodPos, ExtractionIterationType::BLOOD, formatName, source, result, fileIndex);

	if (eventPos.Valid() && (datetimeEventPos.Valid() || (dateEventPos.Valid() && timeEventPos.Valid())))
		status = status && Extract_Hierarchy_File_Stream(eventPos, isigPos, datetimeEventPos, dateEventPos, timeEventPos, ExtractionIterationType::EVENT, formatName, source, result, fileIndex);

	// extract device name if available
	if (result.FileDeviceNames.size() < fileIndex + 1)
		result.FileDeviceNames.resize(fileIndex + 1);

	TreePosition devName;
	Fill_TreePosition_For(formatName, ExtractorColumns::COL_DEVICE, devName);
	if (devName.Valid())
		result.FileDeviceNames[fileIndex] = source.Read(devName);
	else
		result.FileDeviceNames[fileIndex] = "";

	return status;
}

bool CExtractor::Extract_Hierarchy_File_Stream(TreePosition& valuePos, TreePosition& isigPos, TreePosition& datetimePos, TreePosition& datePos, TreePosition& timePos, ExtractionIterationType itrtype, std::string& formatName, CFormat_Adapter& source, ExtractionResult& result, size_t fileIndex) const
{
	double bloodMultiplier = Get_Column_Multiplier(formatName, ExtractorColumns::COL_BLOOD);
	double istMultiplier = Get_Column_Multiplier(formatName, ExtractorColumns::COL_IST);
	double isigMultiplier = Get_Column_Multiplier(formatName, ExtractorColumns::COL_ISIG);

	TreePosition condPos;

	// if parsing event, we probably would need conditional field
	if (itrtype == ExtractionIterationType::EVENT)
		Fill_TreePosition_For(formatName, ExtractorColumns::COL_EVENT_CONDITION, condPos);

	std::string tmp;
	CMeasured_Value* mval = nullptr;
	NKnownDateFormat dateformat = NKnownDateFormat::UNKNOWN_DATEFORMAT; // will be recognized after first date read
	time_t curTime;

	std::string sval;
	double dval;
	bool validValue;

	// declare lambda for position incrementation
	auto posInc = [&]() {
		valuePos.ToNext(); isigPos.ToNext(); datetimePos.ToNext(); datePos.ToNext(); timePos.ToNext(); condPos.ToNext();
	};

	while (valuePos.Valid())
	{
		// in case of reuse (i.e. failed to parse all columns), no need to reallocate
		if (!mval)
			mval = new CMeasured_Value();
		else
			mval->reuse();

		validValue = false;

		sval = source.Read(valuePos);

		switch (itrtype)
		{
		case ExtractionIterationType::BLOOD:
		{
			if (sval.length())
			{
				if (!Formatted_Read_Double(formatName, ExtractorColumns::COL_BLOOD, sval, dval))
					break;

				mval->mBlood = dval * bloodMultiplier;
				validValue = true;
			}
			break;
		}
		case ExtractionIterationType::EVENT:
		{
			std::string condition;

			if (condPos.Valid())
				condition = source.Read(condPos);

			// resolve condition - retrieve column based on condition value
			ExtractorColumns ccol = Get_Conditional_Column(formatName, ExtractorColumns::COL_EVENT_CONDITION, condition);
			// unknown condition value - no column, break
			if (ccol == ExtractorColumns::NONE)
				break;

			// attempt to extract formatted double value from file
			if (!Formatted_Read_Double(formatName, ccol, sval, dval))
				break;

			switch (ccol)
			{
			case ExtractorColumns::COL_INSULIN:
			{
				mval->mInsulin = dval;
				validValue = true;
				break;
			}
			case ExtractorColumns::COL_CARBOHYDRATES:
			{
				mval->mCarbohydrates = dval;
				validValue = true;
				break;
			}
			}

			break;
		}
		case ExtractionIterationType::IST:
		{
			if (isigPos.Valid())
			{
				if (source.Read(isigPos).length())
					mval->mIsig = source.Read_Double(isigPos) * isigMultiplier;
			}

			if (sval.length())
			{
				if (!Formatted_Read_Double(formatName, ExtractorColumns::COL_IST, sval, dval))
					break;

				mval->mIst = dval * istMultiplier;
				validValue = true;
			}

			break;
		}
		}

		if (!valuePos.Valid())
			break;

		// prefer separate date and time columns
		if (datePos.Valid() && timePos.Valid())
			tmp = source.Read_Date(datePos) + " " + source.Read_Time(timePos);
		// if not available, try datetime column
		else if (datetimePos.Valid())
			tmp = source.Read_Datetime(datetimePos);
		else // date+time is mandatory - if not present, go to next row
		{
			posInc();
			continue;
		}

		// just sanity check
		if (tmp.length() < 6)
		{
			posInc();
			continue;
		}

		// recognize date format if not already known
		if (dateformat == NKnownDateFormat::UNKNOWN_DATEFORMAT)
		{
			dateformat = Recognize_Date_Format(tmp);
			if (dateformat == NKnownDateFormat::UNKNOWN_DATEFORMAT)
			{
				posInc();
				continue;
			}
		}

		std::string dst;
		// is conversion result valid? if not, try next line
		if (!Str_Time_To_Unix_Time(tmp, dateformat, dst, dsDatabaseTimestampFormatShort, curTime))
		{
			posInc();
			continue;
		}

		mval->mMeasuredAt = Unix_Time_To_Rat_Time(curTime);

		// if the row is valid (didn't reach end of file, ist or blood value is greater than zero)
		if (!source.Is_EOF() && validValue)
		{
			// separate IST(+BG) and BG-only values; BG-only values will be merged into IST to obtain single vector of values
			if (mval->mIst > 0.0)
				result.SegmentValues[fileIndex].push_back(mval);
			else if (mval->mBlood.has_value())
				result.SegmentBloodValues[fileIndex].push_back(mval);
			else
				result.SegmentMiscValues[fileIndex].push_back(mval);

			mval = nullptr; // do not reuse
		}

		posInc();
	}

	// last value is always invalid and never used - delete it
	if (mval)
		delete mval;

	return true;
}

bool CExtractor::Extract_Spreadsheet_File(std::string& formatName, CFormat_Adapter& source, ExtractionResult& result, size_t fileIndex) const
{
	SheetPosition datePos, timePos, datetimePos, isigPos, istPos, bloodPos, bloodCalPos, insulinPos, carbsPos,
		bloodDatePos, bloodTimePos, bloodDatetimePos, eventPos, eventDatePos, eventTimePos, eventDateTimePos, eventCondPos;

	// find positions of important columns in current format
	Fill_SheetPosition_For(formatName, ExtractorColumns::COL_DATE, datePos);
	Fill_SheetPosition_For(formatName, ExtractorColumns::COL_TIME, timePos);
	// we prefer having separate date and time columns, but if not available, use datetime column
	if (!datePos.Valid() || !timePos.Valid())
		Fill_SheetPosition_For(formatName, ExtractorColumns::COL_DATETIME, datetimePos);
	Fill_SheetPosition_For(formatName, ExtractorColumns::COL_ISIG, isigPos);
	Fill_SheetPosition_For(formatName, ExtractorColumns::COL_IST, istPos);
	Fill_SheetPosition_For(formatName, ExtractorColumns::COL_BLOOD, bloodPos);
	Fill_SheetPosition_For(formatName, ExtractorColumns::COL_BLOOD_DATE, bloodDatePos);
	Fill_SheetPosition_For(formatName, ExtractorColumns::COL_BLOOD_TIME, bloodTimePos);
	// we prefer having separate date and time columns, but if not available, use datetime column
	if (!bloodDatePos.Valid() || !bloodTimePos.Valid())
		Fill_SheetPosition_For(formatName, ExtractorColumns::COL_BLOOD_DATETIME, bloodDatetimePos);
	Fill_SheetPosition_For(formatName, ExtractorColumns::COL_BLOOD_CALIBRATION, bloodCalPos);
	Fill_SheetPosition_For(formatName, ExtractorColumns::COL_INSULIN, insulinPos);
	Fill_SheetPosition_For(formatName, ExtractorColumns::COL_CARBOHYDRATES, carbsPos);

	Fill_SheetPosition_For(formatName, ExtractorColumns::COL_EVENT, eventPos);
	Fill_SheetPosition_For(formatName, ExtractorColumns::COL_EVENT_CONDITION, eventCondPos);
	Fill_SheetPosition_For(formatName, ExtractorColumns::COL_EVENT_DATE, eventDatePos);
	Fill_SheetPosition_For(formatName, ExtractorColumns::COL_EVENT_TIME, eventTimePos);
	if (!eventDatePos.Valid() || !eventTimePos.Valid())
		Fill_SheetPosition_For(formatName, ExtractorColumns::COL_EVENT_DATETIME, eventDateTimePos);

	if (result.FileDeviceNames.size() < fileIndex + 1)
		result.FileDeviceNames.resize(fileIndex + 1);

	// extract device name if available
	SheetPosition devName;
	Fill_SheetPosition_For(formatName, ExtractorColumns::COL_DEVICE, devName);
	if (devName.Valid())
		result.FileDeviceNames[fileIndex] = source.Read(devName.row, devName.column, devName.sheetIndex);
	else
		result.FileDeviceNames[fileIndex] = "";

	double bloodMultiplier = Get_Column_Multiplier(formatName, ExtractorColumns::COL_BLOOD);
	double bloodCalMultiplier = Get_Column_Multiplier(formatName, ExtractorColumns::COL_BLOOD_CALIBRATION);
	double istMultiplier = Get_Column_Multiplier(formatName, ExtractorColumns::COL_IST);
	double isigMultiplier = Get_Column_Multiplier(formatName, ExtractorColumns::COL_ISIG);
	double insulinMultiplier = Get_Column_Multiplier(formatName, ExtractorColumns::COL_INSULIN);
	double carbsMultiplier = Get_Column_Multiplier(formatName, ExtractorColumns::COL_CARBOHYDRATES);

	// we need at least one type to be fully available (either datetime or date AND time)
	if (!datetimePos.Valid() && !(datePos.Valid() && timePos.Valid()))
		return false;

	// if IST and blood have separate columns specified, check if they're the same - if not, they are separate and should be extracted independently
	const bool separatedInputStreams = ((bloodDatePos.Valid() && bloodTimePos.Valid() && bloodDatePos != datePos && bloodTimePos != timePos)
		|| (bloodDatetimePos.Valid() && bloodDatetimePos != datetimePos));
	ExtractionIterationType phase = ExtractionIterationType::IST;

	// declare lambdas for all row position incrementation
	auto posInc_ist = [&]() {
		datePos.row++; timePos.row++; datetimePos.row++; isigPos.row++; istPos.row++; insulinPos.row++; carbsPos.row++;
	};
	auto posInc_blood = [&]() {
		bloodPos.row++; bloodCalPos.row++; bloodDatePos.row++; bloodTimePos.row++; bloodDatetimePos.row++;
	};
	auto posInc_event = [&]() {
		eventPos.row++; eventCondPos.row++; eventDatePos.row++; eventTimePos.row++; eventDateTimePos.row++;
	};
	auto posInc = [&]() {
		if (!separatedInputStreams || phase == ExtractionIterationType::IST)
			posInc_ist();
		if (!separatedInputStreams || phase == ExtractionIterationType::EVENT)
			posInc_event();
		if (!separatedInputStreams || phase == ExtractionIterationType::BLOOD)
			posInc_blood();
	};

	// initial incrementation - the collspec points to table header
	posInc();

	// NOTE: we assume equal count of rows for each column

	std::string tmp;
	CMeasured_Value* mval = nullptr;
	NKnownDateFormat dateformat = NKnownDateFormat::UNKNOWN_DATEFORMAT; // will be recognized after first date read
	time_t curTime;

	bool validValue;

	do
	{
		if (separatedInputStreams && (source.Is_EOF() ||
			(phase == ExtractionIterationType::IST && !istPos.Valid())
			|| (phase == ExtractionIterationType::EVENT && !eventPos.Valid())
			|| (phase == ExtractionIterationType::BLOOD && !bloodPos.Valid())
			))
		{
			switch (phase)
			{
			case ExtractionIterationType::IST:
				phase = ExtractionIterationType::EVENT;
				break;
			case ExtractionIterationType::EVENT:
				phase = ExtractionIterationType::BLOOD;
				break;
			case ExtractionIterationType::BLOOD:
				break;
			}
			source.Reset_EOF();
		}

		// in case of reuse (i.e. failed to parse all columns), no need to reallocate
		if (!mval)
			mval = new CMeasured_Value();
		else
			mval->reuse();

		validValue = false;

		if (!separatedInputStreams || phase == ExtractionIterationType::BLOOD)
		{
			if (bloodPos.Valid())
			{
				if (source.Read(bloodPos.row, bloodPos.column, bloodPos.sheetIndex).length())
				{
					mval->mBlood = source.Read_Double(bloodPos.row, bloodPos.column, bloodPos.sheetIndex) * bloodMultiplier;
					validValue = true;
				}
			}
			if (bloodCalPos.Valid())
			{
				if (source.Read(bloodCalPos.row, bloodCalPos.column, bloodCalPos.sheetIndex).length())
				{
					mval->mCalibration = source.Read_Double(bloodCalPos.row, bloodCalPos.column, bloodCalPos.sheetIndex) * bloodCalMultiplier;
					// calibration value can be used also as regular blood-glucose value
					if (!mval->mBlood.has_value())
						mval->mBlood = mval->mCalibration.value();
					validValue = true;
				}
			}
		}

		if (!separatedInputStreams || phase == ExtractionIterationType::IST)
		{
			if (isigPos.Valid())
			{
				if (source.Read(isigPos.row, isigPos.column, isigPos.sheetIndex).length())
				{
					mval->mIsig = source.Read_Double(isigPos.row, isigPos.column, isigPos.sheetIndex) * isigMultiplier;
					validValue = true;
				}
			}
			if (istPos.Valid())
			{
				if (source.Read(istPos.row, istPos.column, istPos.sheetIndex).length())
				{
					mval->mIst = source.Read_Double(istPos.row, istPos.column, istPos.sheetIndex) * istMultiplier;
					validValue = true;
				}
			}
			if (insulinPos.Valid())
			{
				if (source.Read(insulinPos.row, insulinPos.column, insulinPos.sheetIndex).length())
				{
					mval->mInsulin = source.Read_Double(insulinPos.row, insulinPos.column, insulinPos.sheetIndex) * insulinMultiplier;
					validValue = true;
				}
			}
			if (carbsPos.Valid())
			{
				if (source.Read(carbsPos.row, carbsPos.column, carbsPos.sheetIndex).length())
				{
					mval->mCarbohydrates = source.Read_Double(carbsPos.row, carbsPos.column, carbsPos.sheetIndex) * carbsMultiplier;
					validValue = true;
				}
			}

			// prefer separate date and time columns
			if (datePos.Valid() && timePos.Valid())
				tmp = source.Read_Date(datePos.row, datePos.column, datePos.sheetIndex) + " " + source.Read_Time(timePos.row, timePos.column, timePos.sheetIndex);
			// if not available, try datetime column
			else if (datetimePos.Valid())
				tmp = source.Read_Datetime(datetimePos.row, datetimePos.column, datetimePos.sheetIndex);
			else // date+time is mandatory - if not present, go to next row
			{
				posInc();
				continue;
			}
		}

		if (!separatedInputStreams || phase == ExtractionIterationType::EVENT)
		{
			std::string condition;
			std::string sval;
			double dval;

			if (eventPos.Valid())
			{

				sval = source.Read(eventPos.row, eventPos.column, eventPos.sheetIndex);

				if (eventCondPos.Valid())
					condition = source.Read(eventCondPos.row, eventCondPos.column, eventCondPos.sheetIndex);

				// resolve condition - retrieve column based on condition value
				ExtractorColumns ccol = Get_Conditional_Column(formatName, ExtractorColumns::COL_EVENT_CONDITION, condition);
				// unknown condition value - no column? try to read formatted input value
				if (ccol != ExtractorColumns::NONE && Formatted_Read_Double(formatName, ccol, sval, dval))
				{
					switch (ccol)
					{
					case ExtractorColumns::COL_INSULIN:
					{
						mval->mInsulin = dval;
						validValue = true;
						break;
					}
					case ExtractorColumns::COL_CARBOHYDRATES:
					{
						mval->mCarbohydrates = dval;
						validValue = true;
						break;
					}
					}
				}
			}
		}

		if (separatedInputStreams)
		{
			if (phase == ExtractionIterationType::BLOOD)
			{
				// prefer separate date and time columns
				if (bloodDatePos.Valid() && bloodTimePos.Valid())
					tmp = source.Read_Date(bloodDatePos.row, bloodDatePos.column, bloodDatePos.sheetIndex) + " " + source.Read_Time(bloodTimePos.row, bloodTimePos.column, bloodTimePos.sheetIndex);
				// if not available, try datetime column
				else if (bloodDatetimePos.Valid())
					tmp = source.Read_Datetime(bloodDatetimePos.row, bloodDatetimePos.column, bloodDatetimePos.sheetIndex);
				else // date+time is mandatory - if not present, go to next row
				{
					posInc();
					continue;
				}
			}
			else if (phase == ExtractionIterationType::EVENT)
			{
				// prefer separate date and time columns
				if (eventDatePos.Valid() && eventTimePos.Valid())
					tmp = source.Read_Date(eventDatePos.row, eventDatePos.column, eventDatePos.sheetIndex) + " " + source.Read_Time(eventTimePos.row, eventTimePos.column, eventTimePos.sheetIndex);
				// if not available, try datetime column
				else if (eventDateTimePos.Valid())
					tmp = source.Read_Datetime(eventDateTimePos.row, eventDateTimePos.column, eventDateTimePos.sheetIndex);
				else // date+time is mandatory - if not present, go to next row
				{
					posInc();
					continue;
				}
			}
		}

		// just sanity check
		if (tmp.length() < 6)
		{
			posInc();
			continue;
		}

		// recognize date format if not already known
		if (dateformat == NKnownDateFormat::UNKNOWN_DATEFORMAT)
		{
			dateformat = Recognize_Date_Format(tmp);
			if (dateformat == NKnownDateFormat::UNKNOWN_DATEFORMAT)
			{
				posInc();
				continue;
			}
		}

		std::string dst;
		// is conversion result valid? if not, try next line
		if (!Str_Time_To_Unix_Time(tmp, dateformat, dst, dsDatabaseTimestampFormatShort, curTime))
		{
			posInc();
			continue;
		}

		mval->mMeasuredAt = Unix_Time_To_Rat_Time(curTime);

		// if the row is valid (didn't reach end of file, ist or blood value is greater than zero)
		if (!source.Is_EOF() && validValue)
		{
			// separate IST(+BG) and BG-only values; BG-only values will be merged into IST to obtain single vector of values
			if (mval->mIst > 0.0)
				result.SegmentValues[fileIndex].push_back(mval);
			else if (mval->mBlood.has_value())
				result.SegmentBloodValues[fileIndex].push_back(mval);
			else
				result.SegmentMiscValues[fileIndex].push_back(mval);

			mval = nullptr; // do not reuse
		}

		posInc();

	} while (!source.Is_EOF() || (separatedInputStreams && phase != ExtractionIterationType::BLOOD /* blood is ending iteration */));

	// last value is always invalid and never used - delete it
	if (mval)
		delete mval;

	return true;
}
