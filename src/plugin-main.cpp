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
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  UNUSED_PARAMETER(fdwReason);
  UNUSED_PARAMETER(lpvReserved);
  wchar_t mainDllPathBuf[MAX_PATH];
  GetModuleFileNameW(hinstDLL, mainDllPathBuf, MAX_PATH);
  *wcsrchr(mainDllPathBuf, L'\\') = L'\0';

  wchar_t auxDllPathBuf[MAX_PATH];
  swprintf(auxDllPathBuf, MAX_PATH, L"%ls\\obs-backgroundremoval\\", mainDllPathBuf);
  SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_USER_DIRS);
  AddDllDirectory(auxDllPathBuf);
  blog(LOG_INFO, "DLL PATH added: %ls", auxDllPathBuf);
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
