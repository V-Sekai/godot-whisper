#include "register_types.h"

#include "resource_loader_whisper.h"
#include "resource_whisper.h"
#include "speech_to_text.h"

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

static Ref<ResourceFormatLoaderWhisper> whisper_loader;

void register_setting(
		const String &p_name,
		const Variant &p_value,
		PropertyHint p_hint,
		const String &p_hint_string) {
	ProjectSettings *project_settings = ProjectSettings::get_singleton();

	if (!project_settings->has_setting(p_name)) {
		project_settings->set(p_name, p_value);
	}

	Dictionary property_info;
	property_info["name"] = p_name;
	property_info["type"] = p_value.get_type();
	property_info["hint"] = p_hint;
	property_info["hint_string"] = p_hint_string;

	project_settings->add_property_info(property_info);
	project_settings->set_initial_value(p_name, p_value);
}

void whisper_callback(enum ggml_log_level level, const char *text, void *user_data) {
	switch (level) {
		case GGML_LOG_LEVEL_ERROR: {
			ERR_PRINT(text);
		} break;
		case GGML_LOG_LEVEL_WARN: {
			WARN_PRINT(text);
		} break;
		case GGML_LOG_LEVEL_INFO: {
			UtilityFunctions::print(text);
		} break;
		case GGML_LOG_LEVEL_DEBUG: {
			UtilityFunctions::print(text);
		} break;
	}
}

void initialize_whisper_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	GDREGISTER_CLASS(SpeechToText);
	GDREGISTER_CLASS(WhisperResource);
	GDREGISTER_CLASS(ResourceFormatLoaderWhisper);
	whisper_log_set(whisper_callback, nullptr);

	whisper_loader.instantiate();
	ResourceLoader::get_singleton()->add_resource_format_loader(whisper_loader);

	// register settings
	register_setting("audio/input/transcribe/entropy_treshold", 2.8, PROPERTY_HINT_NONE, {});
	register_setting("audio/input/transcribe/freq_treshold", 200, PROPERTY_HINT_NONE, {});
	register_setting("audio/input/transcribe/max_tokens", 32, PROPERTY_HINT_NONE, {});
	register_setting("audio/input/transcribe/vad_treshold", 0.3, PROPERTY_HINT_NONE, {});
	register_setting("audio/input/transcribe/use_gpu", true, PROPERTY_HINT_NONE, {});
	register_setting("audio/input/transcribe/speed_up_2x", false, PROPERTY_HINT_NONE, {});
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
