#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <regex>
#include <stdio.h>
#include <stdlib.h>

#include "INIParser.h"


CINIParser::CINIParser()
{
}


CINIParser::~CINIParser()
{
}

bool try_stoi(const std::string& s, int& i) {
	try {
		size_t pos;
		i = std::stoi(s, &pos);
		return pos == s.size();
	}
	catch (const std::invalid_argument&) {
		return false;
	}
}

bool CINIParser::LoadIniFile(const char * _pcFilename)
{
	const std::regex kstrREGEX_COMMENT(R"(;(.*)$)");
	const std::regex krgxREGEX_SECTION(R"(\[\s*([^\s;]+)\s*\])");
	const std::regex krgxPARAMETER(R"(([^\s;]+)\s*=\s*([^\s;]*(\s+[^\s;]+)*))");
	const std::regex krgxQUOTED_PARAMETER(R"(([^\s;]+)\s*=\s*\"(.*)\")"); 

	std::ifstream ifs(_pcFilename);
	std::string line;

	// Loop over lines
	std::string strCurSection  = "__GLOBAL__";
	if (ifs)
	{
		while (std::getline(ifs, line))
		{
			std::smatch sm;

			if (std::regex_search(line, sm, krgxREGEX_SECTION))
			{
				strCurSection = sm.str(1);
			}
			else if (
				std::regex_search(line, sm, krgxQUOTED_PARAMETER) ||
				std::regex_search(line, sm, krgxPARAMETER))
			{
				AddValue(strCurSection.c_str(), sm.str(1).c_str(), sm.str(2).c_str());
			}
			else {}
		}

		return true;
	}
	else
	{
		return false;
	}

	//TODO: Make everything lowercase (case insensitive)
}

std::string CINIParser::GenKey(const char * _pcSection, const char * _pcKey)
{
	return std::string(_pcSection) + '|' + std::string(_pcKey);
}

bool CINIParser::AddValue(const char * _pcSection, const char * _pcKey, const char * _pcValue)
{
	auto itResult = m_mapPairs.insert(std::make_pair(GenKey(_pcSection, _pcKey), _pcValue));
	return itResult.second;
}

bool CINIParser::GetStringValue(const char * _pcSection, const char * _pcKey, std::string & _rStrValue)
{
	auto it = m_mapPairs.find(GenKey(_pcSection, _pcKey));
	if (it != m_mapPairs.end())
	{
		_rStrValue = it->second;
		return true;
	}
	else
	{
		return false;
	}
}

bool CINIParser::GetIntValue(const char * _pcSection, const char * _pcKey, int & _riValue)
{
	std::string strValue;
	GetStringValue(_pcSection, _pcKey, strValue);

	bool bResult = try_stoi(strValue, _riValue);
	return bResult;
}

bool CINIParser::GetFloatValue(const char * _pcSection, const char * _pcKey, float & _rfValue)
{
	return false;
}

bool CINIParser::GetBoolValue(const char * _pcSection, const char * _pcKey, bool & _rbValue)
{
	return false;
}
