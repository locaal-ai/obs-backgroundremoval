/*
OBS Background Removal Filter Plugin
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

#include <plugin-support.h>
#include <curl/curl.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return obs_module_text("PortraitBackgroundFilterPlugin");
}

extern struct obs_source_info background_removal_filter_info;
extern struct obs_source_info enhance_filter_info;

struct string {
	char *ptr;
	size_t len;
};

void init_string(struct string *s)
{
	s->len = 0;
	s->ptr = malloc(s->len + 1);
	if (s->ptr == NULL) {
		fprintf(stderr, "malloc() failed\n");
		exit(EXIT_FAILURE);
	}
	s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s)
{
	size_t new_len = s->len + size * nmemb;
	s->ptr = realloc(s->ptr, new_len + 1);
	if (s->ptr == NULL) {
		fprintf(stderr, "realloc() failed\n");
		exit(EXIT_FAILURE);
	}
	memcpy(s->ptr + s->len, ptr, size * nmemb);
	s->ptr[new_len] = '\0';
	s->len = new_len;

	return size * nmemb;
}

bool obs_module_load(void)
{
	obs_register_source(&background_removal_filter_info);
	obs_register_source(&enhance_filter_info);
	obs_log(LOG_INFO, "plugin loaded successfully (version %s)",
		PLUGIN_VERSION);
	CURL *curl = curl_easy_init();
	if (curl) {
		struct string s;
		init_string(&s);
		curl_easy_setopt(
			curl, CURLOPT_URL,
			"https://api.github.com/repos/royshil/obs-backgroundremoval/releases/latest");
		curl_easy_setopt(curl, CURLOPT_USERAGENT,
				 "obs-backgroundremoval");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
		curl_easy_perform(curl);
		blog(LOG_INFO, "%s\n", s.ptr);
		free(s.ptr);
		curl_easy_cleanup(curl);
	}
	return true;
}

void obs_module_unload()
{
	obs_log(LOG_INFO, "plugin unloaded");
}
