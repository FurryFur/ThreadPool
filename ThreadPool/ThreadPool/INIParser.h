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

#pragma once

#ifndef INIPARSER_H
#define INIPARSER_H

#include <map>
#include <string>

class INIParser
{
public:
	INIParser();
	~INIParser();

	// Loads an INI file.
	bool LoadIniFile(const char* filename);

	// Adds a value to the map.
	bool AddValue(const char* section, const char* key, const char* value);

	// Retrieves a string value stored in the INI file by section and key.
	// Returns true if the value was found.
	// On success, outValue will be populated with the retrieved value.
	bool GetStringValue(const char* section, const char* key, std::string& outValue);

	// Retrieves an int value stored in the INI file by section and key.
	// Returns true if the value was found, and was convertible to an int
	// On success, outValue will be populated with the retrieved value.
	bool GetIntValue(const char* section, const char* key, int& value);

	// Retrieves a size_t value stored in the INI file by section and key.
	// Returns true if the value was found, and was convertible to a size_t
	// On success, outValue will be populated with the retrieved value.
	bool GetIntValue(const char* section, const char* key, size_t& value);

	// Retrieves a float value stored in the INI file by section and key.
	// Returns true if the value was found, and was convertible to a float.
	// On success, outValue will be populated with the retrieved value.
	bool GetFloatValue(const char* section, const char* key, float& value);

	// Retrieves a double value stored in the INI file by section and key.
	// Returns true if the value was found, and was convertible to a double.
	// On success, outValue will be populated with the retrieved value.
	bool GetFloatValue(const char* section, const char* key, double& value);

	// Retrieves a bool value stored in the INI file by section and key.
	// Returns true if the value was found, and was convertible to a bool.
	// On success, outValue will be populated with the retrieved value.
	bool GetBoolValue(const char* section, const char* key, bool& value);

private:
	//Create a map member variable to store the ini file.
	std::map<std::string, std::string> m_mapPairs;

	std::string INIParser::GenKey(const char * section, const char * key);
};

#endif // !INIPARSER_H
