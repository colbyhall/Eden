#include "config.h"

static Config* loaded_config;

Config default_config;

Config get_config() {
	if (!loaded_config) return default_config;
	return *loaded_config;
}