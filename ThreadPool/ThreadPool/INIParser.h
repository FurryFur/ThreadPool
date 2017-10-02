#pragma once

#ifndef INIPARSER_H
#define INIPARSER_H

#include <map>
#include <string>

class CINIParser
{
public:
	CINIParser();
	~CINIParser();

	// Load an INI file.
	// Populates the map with keys and values using
	// the add value function.
	bool LoadIniFile(const char* _pcFilename);

	// Adds a value to the map.
	// Combines the _pcSection, with _pcKey to create a
	// key for the map.
	bool AddValue(const char* _pcSection, const char* _pcKey, const char* _pcValue);

	// GetStringValue
	// Returns true if the value was found.
	// _rStrValue will be populated with the correct data if // the key is found in the map.
	bool GetStringValue(const char* _pcSection, const char* _pcKey, std::string& _rStrValue);

	// GetIntValue
	// Returns true if the value was found.
	// _riValue will be populated with the correct data if
	// the key is found in the map.
	bool GetIntValue(const char* _pcSection, const char* _pcKey, int& _riValue);

	// GetFloatValue
	// Returns true if the value was found.
	// _rfValue will be populated with the correct data if
	// the key is found in the map.
	bool GetFloatValue(const char* _pcSection, const char* _pcKey, float& _rfValue);

	// GetBoolValue
	// Returns true if the value was found.
	// _rbValue will be populated with the correct data if
	// the key is found in the map.
	bool GetBoolValue(const char* _pcSection, const char* _pcKey, bool& _rbValue);

private:
	//Create a map member variable to store the ini file.
	std::map<std::string, std::string> m_mapPairs;

	std::string CINIParser::GenKey(const char * _pcSection, const char * _pcKey);
};

#endif // !INIPARSER_H
