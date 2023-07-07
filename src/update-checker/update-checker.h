#pragma once

#include "github-utils.h"

#ifdef __cplusplus
extern "C" {
#endif

void check_update(struct github_utils_release_information latestRelease);

#ifdef __cplusplus
}
#endif
