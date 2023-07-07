#pragma once

#ifdef __cplusplus
extern "C" {
#endif

enum {
	OBS_BGREMOVAL_GITHUB_UTILS_SUCCESS = 0,
	OBS_BGREMOVAL_GITHUB_UTILS_ERROR = -1,
};

struct github_utils_release_information {
	int responseCode;
	char *responseBody;
	char *version;
};

struct github_utils_release_information
github_utils_get_release_information(void);

void github_utils_release_information_free(
	struct github_utils_release_information info);

#ifdef __cplusplus
}
#endif
