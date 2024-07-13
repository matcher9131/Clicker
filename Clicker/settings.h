#pragma once
#include <string>

typedef struct {
	std::wstring className;
	int interval;
} Settings;

Settings LoadSettings();