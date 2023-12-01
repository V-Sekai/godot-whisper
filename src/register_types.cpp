#include "register_types.h"

#include "audio_effect_transcribe.h"

void initialize_whisper_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	GDREGISTER_CLASS(AudioEffectTranscribe);
	GDREGISTER_CLASS(AudioEffectTranscribeInstance);
}

void uninitialize_whisper_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

extern "C" {

// Initialization.

GDExtensionBool GDE_EXPORT godot_whisper_library_init(const GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_whisper_module);
	init_obj.register_terminator(uninitialize_whisper_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}
