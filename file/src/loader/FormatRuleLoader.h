#pragma once

#include "FormatRecognizer.h"
#include "Extractor.h"

class CFormat_Rule_Loader
{
	protected:
		CFormat_Recognizer& mFormatRecognizer;
		CExtractor& mExtractor;

		bool Load_Format_Pattern_Config();
		bool Load_Format_Rule_Templates();
		bool Load_Format_Rules();

	public:
		CFormat_Rule_Loader(CFormat_Recognizer& recognizer, CExtractor& extractor);

		bool Load();
};
