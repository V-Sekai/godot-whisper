#ifndef WHISPER_RESOURCE_H
#define WHISPER_RESOURCE_H

#include <godot_cpp/classes/resource.hpp>

using namespace godot;

class WhisperResource : public Resource {
	GDCLASS(WhisperResource, Resource);

protected:
	static void _bind_methods() {}

private:
	PackedByteArray content;

public:
	Error load_file(const String &p_path);
	PackedByteArray get_content() { return content; }
	WhisperResource() {}
	~WhisperResource() {}
};
#endif // RESOURCE_JSON_H
