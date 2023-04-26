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
#include <string>
#include <windows.h>
static void AppendPath()
{
  HMODULE mod;
  char buf[MAX_PATH];
  GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)&AppendPath, &mod);
  GetModuleFileNameA(mod, buf, MAX_PATH);
  std::string myPath = std::string(buf, std::strrchr(buf, '\\')) + "\\obs-backgroundremoval\\";
    
  char envPath[65536];
  GetEnvironmentVariableA("PATH", envPath, 65536);
  std::string newPath = myPath + ";" + envPath;
  SetEnvironmentVariableA("PATH", newPath.c_str());
  blog(LOG_INFO, "PATH %s", newPath.c_str());
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
#ifdef _WIN32
  AppendPath();
#endif
  obs_register_source(&background_removal_filter_info);
  blog(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
  return true;
}

void obs_module_unload()
{
  blog(LOG_INFO, "plugin unloaded");
}
