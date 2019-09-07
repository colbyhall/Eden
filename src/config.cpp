#include "config.h"

#include <ch_stl/filesystem.h>

static Config* loaded_config;

Config default_config;

static const ch::Path config_path = CH_TEXT(".yeetconfig");

const Config& get_config() {
	if (!loaded_config) return default_config;
	return *loaded_config;
}

static bool parse_config(const ch::File_Data& fd, Config* config) {
	ch::String file_string = fd.to_string();
	defer(file_string.free());

	return false;
}

static void export_config(const Config& config, const ch::Path& path = config_path) {
	ch::File f;
	f.open(path, ch::FO_Read | ch::FO_Write | ch::FO_Create);
	assert(f.is_open);
	f.seek_top();

#define EXPORT_VARS(t, n, v) f << #n << ' ' << v << ch::eol;
	CONFIG_VAR(EXPORT_VARS);
#undef EXPORT_VARS

	f.set_end_of_file();
	f.close();
}

void init_config() {
	ch::File_Data fd;
	if (ch::load_file_into_memory(config_path, &fd)) {
		Config* new_conf = ch_new Config;
		if (parse_config(fd, new_conf)) {
			if (loaded_config) {
				ch_delete loaded_config;
			}

			loaded_config = new_conf;
		}
	}
}

void try_refresh_config() {

}

void shutdown_config() {
	
}
