#include <curl/curl.h>

#include <cstddef>
#include <string>

#include <plugin-support.h>
#include <obs.h>

#include "github-utils.h"

static const char GITHUB_LATEST_RELEASE_URL[] =
	"https://api.github.com/repos/royshil/obs-backgroundremoval/releases/latest";
static const char USER_AGENT[] = "obs-backgroundremoval/1.0.3";

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
		curl_easy_setopt(curl, CURLOPT_URL, GITHUB_LATEST_RELEASE_URL);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
				 writeFunctionStdString);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &str);
		code = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		obs_log(LOG_INFO, "%s\n", str.c_str());
		obs_log(LOG_INFO, "%s\n", curl_easy_strerror(code));
	}
}
