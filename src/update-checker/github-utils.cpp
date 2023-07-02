#include <curl/curl.h>

#include <cstddef>
#include <string>

#include <obs.h>

#include "github-utils.h"

static const std::string GITHUB_LATEST_RELEASE_URL =
	"https://api.github.com/repos/royshil/obs-backgroundremoval/releases/latest";

extern const char *PLUGIN_NAME;
extern const char *PLUGIN_VERSION;

static const std::string USER_AGENT =
	std::string(PLUGIN_NAME) + "/" + std::string(PLUGIN_VERSION);

std::size_t writeFunctionStdString(void *ptr, std::size_t size, size_t nmemb,
				   std::string *data)
{
	data->append(static_cast<char *>(ptr), size * nmemb);
	return size * nmemb;
}

void github_utils_get_release(void)
{
	CURL *curl = curl_easy_init();
	if (curl) {
		std::string str;
		CURLcode code;
		curl_easy_setopt(curl, CURLOPT_URL,
				 GITHUB_LATEST_RELEASE_URL.c_str());
		curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
				 writeFunctionStdString);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &str);
		code = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		blog(LOG_INFO, "%s\n", str.c_str());
		blog(LOG_INFO, "%s\n", curl_easy_strerror(code));
	}
}
