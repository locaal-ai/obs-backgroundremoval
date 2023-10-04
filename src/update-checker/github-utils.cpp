#include <cstddef>
#include <string>

#include <obs.h>

#include "Client.hpp"
#include "github-utils.h"
#include "plugin-support.h"

static const std::string GITHUB_LATEST_RELEASE_URL =
	"https://api.github.com/repos/royshil/obs-backgroundremoval/releases/latest";

void github_utils_get_release_information(
	std::function<void(github_utils_release_information)> callback)
{
	fetchStringFromUrl(GITHUB_LATEST_RELEASE_URL.c_str(), [callback](
								      std::string
									      responseBody,
								      int code) {
		if (code != 0)
			return;
		// Parse the JSON response
		obs_data_t *data =
			obs_data_create_from_json(responseBody.c_str());
		if (!data) {
			obs_log(LOG_INFO,
				"Failed to parse latest release info");
			callback(
				{OBS_BGREMOVAL_GITHUB_UTILS_ERROR, NULL, NULL});
			return;
		}

		// The version is in the "tag_name" property
		char *version = strdup(obs_data_get_string(data, "tag_name"));
		char *body = strdup(obs_data_get_string(data, "body"));
		obs_data_release(data);

		// remove the "v" prefix in version, if it exists
		if (version[0] == 'v') {
			char *newVersion = (char *)malloc(strlen(version) - 1);
			strcpy(newVersion, version + 1);
			free(version);
			version = newVersion;
		}

		callback({OBS_BGREMOVAL_GITHUB_UTILS_SUCCESS, body, version});
	});
}

void github_utils_release_information_free(
	struct github_utils_release_information info)
{
	if (info.responseBody != NULL)
		free(info.responseBody);
	if (info.version != NULL)
		free(info.version);
	info.responseCode = OBS_BGREMOVAL_GITHUB_UTILS_ERROR;
}
