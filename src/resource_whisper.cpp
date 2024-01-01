#include "resource_whisper.h"
#include <iostream>

#include <godot_cpp/classes/file_access.hpp>

// Error WhisperResource::load_file(const String &p_path) {
// 	if (!FileAccess::file_exists(p_path)) {
// 		return ERR_FILE_NOT_FOUND;
// 	}
// 	content = FileAccess::get_file_as_bytes(p_path);
// 	if (content.is_empty()) {
// 		return ERR_FILE_CANT_READ;
// 	}
// 	return OK;
// }

PackedByteArray WhisperResource::get_content() {
	PackedByteArray content;
	String p_path = get_file();
	content = FileAccess::get_file_as_bytes(p_path);
	return content;
}
