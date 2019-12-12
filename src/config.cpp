#include "config.h"
#include "editor.h"

#include <ch_stl/filesystem.h>

static Config* loaded_config;
static u64 loaded_config_last_save;

Config default_config;

static const ch::Path config_path = ".edenconfig";

const Config& get_config() {
	if (loaded_config) return *loaded_config;
	return default_config;
}

template <typename T>
static bool parse_type(ch::String& v, T* t) {
	static_assert(false, "Config var type has no parse code");
	return false;
}

template <>
static bool parse_type<bool>(ch::String& v, bool* b) {
	if (!v.count) return false;

	v.to_lowercase();
	if (v == "1" || v == "true") *b = true;
	else if (v == "0" || v == "false") *b = false;
	else return false;

	return true;
}

template <>
static bool parse_type<f32>(ch::String& v, f32* f) {
	if (!v.count) return false;

	return ch::atof(v, f);
}

template <>
static bool parse_type<u16>(ch::String& v, u16* u) {
	if (!v.count) return false;

	s32 i = *u;
	const bool r = ch::atoi(v, &i);
	*u = (u16)i;
	return r;
}

template <>
static bool parse_type<u32>(ch::String& v, u32* u) {
	if (!v.count) return false;

	s32 i = *u;
	const bool r = ch::atoi(v, &i);
	*u = (u32)i;
	return r;
}

template <>
static bool parse_type<ch::Color>(ch::String& v, ch::Color* c) {
	ch::Color result;

	if (!v.count) return false;
	
	ssize index = v.find_from_left(' ');
	if (index == -1) return false;
	ch::String current_value = v;
	current_value.count = index;
	if (!ch::atof(current_value, &result.r)) return false;
	v.advance(index + 1);

	index = v.find_from_left(' ');
	if (index == -1) return false;
	current_value = v;
	current_value.count = index;
	if (!ch::atof(current_value, &result.g)) return false;
	v.advance(index + 1);

	index = v.find_from_left(' ');
	if (index == -1) return false;
	current_value = v;
	current_value.count = index;
	if (!ch::atof(current_value, &result.b)) return false;
	v.advance(index + 1);

	current_value = v;
	current_value.count = index;
	if (!ch::atof(current_value, &result.a)) return false;

	*c = result;

	v.eat_whitespace();
	return !v.count;
}

static bool parse_config(const ch::File_Data& fd, Config* config) {
	ch::String file_string = fd.to_string();
	defer(file_string.free());

	ch::String fs_copy = file_string;

	ch::String line = fs_copy.eat_line();
	while(line) {
		defer(line = fs_copy.eat_line());
		ssize index = line.find_from_left(' ');
		if (index == -1) {
			// @TODO(Chall): Error handling
			continue;
		}

		ch::String var_name = line;
		var_name.count = index;
		
		line.advance(index);
		line.eat_whitespace();
		ch::String value = line;

#define FIND_AND_PARSE(t, n, v) if (var_name == #n) parse_type<t>(value, &config->n);
        CONFIG_VAR(FIND_AND_PARSE);
	}

	return true;
}

static void export_config(const Config& config, const ch::Path& path = config_path) {
	ch::File f;
	f.open(path, ch::FO_Read | ch::FO_Write | ch::FO_Create);
	assert(f.is_open);
	f.seek_top();

#define EXPORT_VARS(t, n, v) f << #n << ' ' << (t)config.n << ch::eol;
	CONFIG_VAR(EXPORT_VARS);

	f.set_end_of_file();
	f.close();
}

void init_config() {
	Config* new_conf = ch_new Config;
	*new_conf = default_config;
	
	ch::File_Data fd;
	if (ch::load_file_into_memory(config_path, &fd)) {
		if (parse_config(fd, new_conf) && loaded_config) {
			ch_delete loaded_config;
		}
	}

	loaded_config = new_conf;
}

void try_refresh_config() {
	
}

void shutdown_config() {
	if (loaded_config) {
		export_config(*loaded_config);
	}
}

void on_window_resize_config() {
	const ch::Vector2 window_size = the_window.get_size();
	if (loaded_config) {
		loaded_config->last_window_width = window_size.ux;
		loaded_config->last_window_height = window_size.uy;
		loaded_config->was_maximized = the_window.is_maximized();
	}
}

void on_window_maximized_config() {
	if (loaded_config) loaded_config->was_maximized = true;
}