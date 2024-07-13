#include "settings.h"
#include <fstream>

Settings LoadSettings()
{
	Settings settings{ L"", 10000 };

	std::wifstream stream(L"settings.ini");
	if (!stream) return settings;

	std::wstring line;
	while (!stream.eof())
	{
		std::getline(stream, line);
		if (line.starts_with(L"#")) continue;
		if (line.starts_with(L"className="))
		{
			settings.className = line.substr(10);
		}
		if (line.starts_with(L"interval="))
		{
			try
			{
				int interval = std::stoi(line.substr(9));
				if (interval > 0)
				{
					settings.interval = interval;
				}
			}
			catch (const std::invalid_argument& e)
			{
				// Nothing special...
			}
			catch (const std::out_of_range& e)
			{
				// Nothing special...
			}
		}
	}

	return settings;
}