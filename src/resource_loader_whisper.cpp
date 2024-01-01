#include "resource_loader_whisper.h"
#include "resource_whisper.h"

Variant ResourceFormatLoaderWhisper::_load(const String &p_path, const String &original_path, bool use_sub_threads, int32_t cache_mode) const {
	Ref<WhisperResource> whisper_model = memnew(WhisperResource);
	// Error err = whisper_model->load_file(p_path);
	// Error err = whisper_model->set_file(p_path);
	// if (err) {
	// 	ERR_PRINT("Error loading whisper model " + rtos(err));
	// 	return Ref<Resource>();
	// }
	whisper_model->set_file(p_path);
	return whisper_model;
}
PackedStringArray ResourceFormatLoaderWhisper::_get_recognized_extensions() const {
	PackedStringArray array;
	array.push_back("bin");
	return array;
}
bool ResourceFormatLoaderWhisper::_handles_type(const StringName &type) const {
	return ClassDB::is_parent_class(type, "WhisperResource");
}
String ResourceFormatLoaderWhisper::_get_resource_type(const String &p_path) const {
	String el = p_path.get_extension().to_lower();
	if (el == "bin") {
		return "WhisperResource";
	}
	return "";
}
