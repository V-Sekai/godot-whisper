#include "register_types.h"

#include "resource_loader_whisper.h"
#include "resource_whisper.h"
#include "speech_to_text.h"

#include <godot_cpp/classes/resource_loader.hpp>

static Ref<ResourceFormatLoaderWhisper> whisper_loader;

void initialize_whisper_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	GDREGISTER_CLASS(SpeechToText);
	GDREGISTER_CLASS(WhisperResource);
	GDREGISTER_CLASS(ResourceFormatLoaderWhisper);
	whisper_loader.instantiate();
	ResourceLoader::get_singleton()->add_resource_format_loader(whisper_loader);
}

void uninitialize_whisper_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	ResourceLoader::get_singleton()->remove_resource_format_loader(whisper_loader);
	whisper_loader.unref();
}

extern "C" {

GDExtensionBool GDE_EXPORT godot_whisper_library_init(const GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_whisper_module);
	init_obj.register_terminator(uninitialize_whisper_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}
