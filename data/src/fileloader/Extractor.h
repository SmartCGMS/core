#pragma once

#include <vector>
#include <map>
#include <locale>

#include "FormatAdapter.h"
#include "Structures.h"

// enumerator of recognized columns
enum class ExtractorColumns
{
	NONE,
	COL_DATE,
	COL_TIME,
	COL_DATETIME,
	COL_ISIG,
	COL_IST,
	COL_BLOOD,
	COL_BLOOD_DATE,
	COL_BLOOD_TIME,
	COL_BLOOD_DATETIME,
	COL_BLOOD_CALIBRATION,
	COL_DEVICE,
	COL_INSULIN,
	COL_CARBOHYDRATES,
	COL_EVENT,
	COL_EVENT_DATE,
	COL_EVENT_TIME,
	COL_EVENT_DATETIME,
	COL_EVENT_CONDITION
};

// type of iteration during extraction
enum class ExtractionIterationType
{
	IST,
	BLOOD,
	EVENT
};

// enumerator of known datetime formats
enum class NKnownDateFormat
{
	DATEFORMAT_DDMMYYYY = 0,
	DATEFORMAT_YYYYMMDD,
	DATEFORMAT_CZ,
	DATEFORMAT_DB_YYYYMMDD,
	UNKNOWN_DATEFORMAT	
};

// recognized datetime format formatter strings
extern const std::array<const char*, static_cast<size_t>(NKnownDateFormat::UNKNOWN_DATEFORMAT)> KnownDateFormatFmtStrings;

/*const std::string KnownDateFormatFmtStrings[] = {
	"%d/%m/%Y %H:%M",        // DATEFORMAT_DDMMYYYY
	"%Y/%m/%d %H:%M",        // DATEFORMAT_YYYYMMDD
	"%d.%m.%Y %H:%M",        // DATEFORMAT_CZ
	"%Y-%m-%d %H:%M:%S",     // DATEFORMAT_DB_YYYYMMDD
};
*/

/*
 * Structure used as result of extraction
 */
struct ExtractionResult
{
	// ordered measured values of files, initially IST-only, then used for merging blood glucose levels; primary index: file index, secondary index: value position
	std::vector<std::vector<CMeasured_Value*>> SegmentValues;
	// ordered measured BG values of files; primary index: file index, secondary index: value position
	std::vector<std::vector<CMeasured_Value*>> SegmentBloodValues;
	// ordered measured values of miscellanous quantities; primary index: file index, secondary index: value position
	std::vector<std::vector<CMeasured_Value*>> SegmentMiscValues;
	// segment file index assigned
	std::vector<size_t> SegmentFileIndex;
	// recognized device for each file; index: file index, value: device name; empty string if not recognized
	std::vector<std::string> FileDeviceNames;

	// total value count within all segments
	uint32_t Value_Count;
};

typedef std::map<std::string, ExtractorColumns> ExtractorRuleMap;
typedef std::map<std::string, double> ExtractorMultiplierMap;
typedef std::map<std::string, std::string> ExtractorStringFormatMap;
typedef std::map<std::string, std::string> ExtractorFormatRuleMap;
typedef std::map<std::string, ExtractorColumns> ExtractorConditionMap;
typedef std::map<std::string, std::string> ExtractorConditionRuleMap;

/*
 * Extractor class used for managing extract (header) template rules and format-specific rules for data extraction
 */
class CExtractor
{
	private:
		// map of header schemes, key = rule name, value = column (value) identifier
		ExtractorRuleMap mRuleTemplates;
		// map of header multipliers, key = rule name, value = multiplier
		ExtractorMultiplierMap mRuleMultipliers;
		// map of header multipliers, key = rule name, value = string format
		ExtractorStringFormatMap mRuleStringFormats;
		// map of header rules for cells, primary key = format name, secondary key = cell spec, value = rule name
		std::map<std::string, ExtractorFormatRuleMap> mRuleSet;
		// map of conditions, primary key = format name, secondary key = condition rule identifier, tertiary key = condition value, value = conditional resolution
		std::map<std::string, std::map<ExtractorColumns, ExtractorConditionMap>> mConditionRuleSet;
		// map of conditions, primary key = format name, secondary key = condition rule identifier, tertiary key = condition value, value = conditional resolution rule name
		std::map<std::string, std::map<ExtractorColumns, ExtractorConditionRuleMap>> mConditionReverseRuleSet;

		// map of header transcriptions to internal identifier
		std::map<std::string, ExtractorColumns> mColumnTypes;

	protected:
		// finds colspec for specified column value and file format; returns nullptr if not found
		const char* Find_ColSpec_For_Column(const std::string& formatName, ExtractorColumns colType) const;
		// fills SheetPosition structure with parsed collspec for specified format and column value
		void Fill_SheetPosition_For(std::string& formatName, ExtractorColumns colType, SheetPosition& target) const;
		// fills TreePosition structure with parsed collspec for specified format and column value
		void Fill_TreePosition_For(std::string& formatName, ExtractorColumns colType, TreePosition& target) const;
		// retrieves multiplier for numeric column
		double Get_Column_Multiplier(std::string& formatName, ExtractorColumns colType) const;
		// retrieves string format for column
		const char* Get_Column_String_Format(std::string& formatName, ExtractorColumns colType) const;
		// resolves conditional column based on inputs
		ExtractorColumns Get_Conditional_Column(std::string& formatName, ExtractorColumns condColType, std::string& condValue) const;

		// extracts information from supplied spreadsheet (csv, xls, xlsx, ..) file and stores all needed data to database; returns true on success
		bool Extract_Spreadsheet_File(std::string& formatName, CFormat_Adapter& source, ExtractionResult& result, size_t fileIndex = 0) const;
		// extracts information from supplied hierarchy file (xml, ..) and stores all needed data to database; returns true on success
		bool Extract_Hierarchy_File(std::string& formatName, CFormat_Adapter& source, ExtractionResult& result, size_t fileIndex = 0) const;

		// extracts single stream of values from supplied file
		bool Extract_Hierarchy_File_Stream(TreePosition& valuePos, TreePosition& isigPos, TreePosition& datetimePos, TreePosition& datePos, TreePosition& timePos, ExtractionIterationType itrtype, std::string& formatName, CFormat_Adapter& source, ExtractionResult& result, size_t fileIndex = 0) const;

		// extracts formatted double for specified column
		bool Formatted_Read_Double(std::string& formatName, ExtractorColumns colType, std::string& source, double& target) const;

	public:
		CExtractor();

		// adds new header template rule; format independent
		void Add_Template(const char* rule, const char* header);
		// adds header template rule multiplier
		void Add_Template_Multiplier(const char* rule, const char* header, double multiplier);
		// adds header template rule string format
		void Add_Template_String_Format(const char* rule, const char* header, const char* stringFormat);
		// adds format-specific header rule; returns true on success, false when no such rule found
		bool Add_Format_Rule(const char* formatName, const char* cellLocation, const char* ruleName);
		// adds format-specific condition rule
		bool Add_Format_Condition_Rule(const char* formatName, const char* condition, const char* headerRuleName);

		// extracts information from supplied file and stores all needed data to database; returns true on success
		bool Extract(std::string& formatName, CFormat_Adapter& source, ExtractionResult& result, size_t fileIndex = 0) const;
};
