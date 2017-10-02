//
// Bachelor of Software Engineering
// Media Design School
// Auckland
// New Zealand
//
// (c) 2017 Media Design School
//
// Description  : An ini file parser.
// Author       : Lance Chaney
// Mail         : lance.cha7337@mediadesign.school.nz
//

#include "INIParser.h"

#include <algorithm>
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <regex>
#include <stdio.h>
#include <stdlib.h>

INIParser::INIParser()
{
}


INIParser::~INIParser()
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

bool try_stosz(const std::string& s, size_t& sz) {
	try {
		size_t pos;
		sz = static_cast<size_t>(std::stoul(s, &pos));
		return pos == s.size();
	}
	catch (const std::invalid_argument&) {
		return false;
	}
}

bool try_stof(const std::string& s, float& f) {
	try {
		size_t pos;
		f = std::stof(s, &pos);
		return pos == s.size();
	} catch (const std::invalid_argument&) {
		return false;
	}
}

bool try_stod(const std::string& s, double& d) {
	try {
		size_t pos;
		d = std::stod(s, &pos);
		return pos == s.size();
	}
	catch (const std::invalid_argument&) {
		return false;
	}
}

bool try_stob(const std::string& _s, bool& b) {
	// Convert to lowercase for case insensitive comparison
	std::string s;
	std::transform(_s.begin(), _s.end(), s.begin(), ::tolower);

	// Check if the string is "true" or "false"
	if (s == "true") {
		b = true;
		return true;
	} 
	if (s == "false") {
		b = false;
		return false;
	} 
	
	// Interpret ints as bools, e.g. foo = 1 is true
	int i;
	if (try_stoi(s, i)) {
		b = i != 0;
		return true;
	}

	return false;
}

bool INIParser::LoadIniFile(const char * filename)
{
	const std::regex kRegexComment(R"(;(.*)$)");
	const std::regex kRegexSection(R"(\[\s*([^\s;]+)\s*\])");
	const std::regex kRegexParameter(R"(([^\s;]+)\s*=\s*([^\s;]*(\s+[^\s;]+)*))");
	const std::regex kRegexQuotedParameter(R"(([^\s;]+)\s*=\s*\"(.*)\")"); 

	std::ifstream ifs(filename);
	std::string line;

	// Loop over lines
	bool result;
	std::string strCurSection  = "__GLOBAL__";
	if (ifs) {
		while (std::getline(ifs, line)) {
			std::smatch sm;

			if (std::regex_search(line, sm, kRegexSection)) {
				strCurSection = sm.str(1);
			} else if (std::regex_search(line, sm, kRegexQuotedParameter) 
			        || std::regex_search(line, sm, kRegexParameter)) {
				AddValue(strCurSection.c_str(), sm.str(1).c_str(), sm.str(2).c_str());
			}
		}

		result = true;
	} else {
		result = false;
	}
	
	return result;
}

std::string INIParser::GenKey(const char * section, const char * key)
{
	return std::string(section) + '|' + std::string(key);
}

bool INIParser::AddValue(const char * section, const char * key, const char * value)
{
	auto itResult = m_mapPairs.insert(std::make_pair(GenKey(section, key), value));
	return itResult.second;
}

bool INIParser::GetStringValue(const char * section, const char * key, std::string & value)
{
	auto it = m_mapPairs.find(GenKey(section, key));
	if (it != m_mapPairs.end())
	{
		value = it->second;
		return true;
	}
	else
	{
		return false;
	}
}

bool INIParser::GetIntValue(const char * section, const char * key, int & value)
{
	std::string strValue;
	GetStringValue(section, key, strValue);

	bool bResult = try_stoi(strValue, value);
	return bResult;
}

bool INIParser::GetIntValue(const char * section, const char * key, size_t & value)
{
	std::string strValue;
	GetStringValue(section, key, strValue);

	bool bResult = try_stosz(strValue, value);
	return bResult;
}

bool INIParser::GetFloatValue(const char * section, const char * key, float & value)
{
	std::string strValue;
	GetStringValue(section, key, strValue);

	bool bResult = try_stof(strValue, value);
	return bResult;
}

bool INIParser::GetFloatValue(const char * section, const char * key, double & value)
{
	std::string strValue;
	GetStringValue(section, key, strValue);

	bool bResult = try_stod(strValue, value);
	return bResult;
}

bool INIParser::GetBoolValue(const char * section, const char * key, bool & value)
{
	std::string strValue;
	GetStringValue(section, key, strValue);

	bool bResult = try_stob(strValue, value);
	return bResult;
}
