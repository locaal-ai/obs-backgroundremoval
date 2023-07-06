#ifndef OBS_CONFIG_UTILS_H
#define OBS_CONFIG_UTILS_H

enum {
	OBS_BGREMOVAL_CONFIG_SUCCESS = 0,
	OBS_BGREMOVAL_CONFIG_FAIL = 1,
};

int getFlagFromConfig(const char *name, bool *returnValue);
int setFlagFromConfig(const char *name, const bool value);

#endif /* OBS_CONFIG_UTILS_H */
