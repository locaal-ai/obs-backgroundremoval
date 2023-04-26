/*
Plugin Name
Copyright (C) 2021 Roy Shilkrot roy.shil@gmail.com

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>

#include "plugin-macros.generated.h"

#ifdef _WIN32
#include <iostream>
#include <windows.h>
#include <tchar.h>
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD dwReason, LPVOID lpReserved)
{
  UNUSED_PARAMETER(lpReserved);
  std::cout << "aaaaa" << std::endl;
  if (dwReason == DLL_PROCESS_ATTACH) {
    char myPathBuf[MAX_PATH];
    GetModuleFileNameA(hInstDLL, myPathBuf, MAX_PATH);
    std::string myPath = std::string(myPathBuf, std::strrchr(myPathBuf, '\\')) + "\\obs-backgroundremoval";

    char envPath[MAX_PATH];
    GetEnvironmentVariableA("PATH", envPath, MAX_PATH);
    std::string newPath = myPath + ";" + envPath;
    std::cout << newPath << std::endl;
    SetEnvironmentVariableA("PATH", newPath.c_str());
  }
  return TRUE;
}
#endif

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
  return obs_module_text("PortraitBackgroundFilterPlugin");
}

extern struct obs_source_info background_removal_filter_info;

bool obs_module_load(void)
{
  obs_register_source(&background_removal_filter_info);
  blog(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
  return true;
}

void obs_module_unload()
{
  blog(LOG_INFO, "plugin unloaded");
}
