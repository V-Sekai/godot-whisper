#ifndef SPEECH_TO_TEXT_H
#define SPEECH_TO_TEXT_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/thread.hpp>
#include <godot_cpp/templates/vector.hpp>

#include <libsamplerate/src/samplerate.h>
#include <whisper.cpp/whisper.h>

using namespace godot;

class SpeechToText : public Node {
	GDCLASS(SpeechToText, Node);

	struct whisper_params {
		int32_t n_threads = MIN(4, (int32_t)OS::get_singleton()->get_processor_count());
		int32_t duration_ms = 5000;
		int32_t capture_id = -1;
		int32_t max_tokens = 32;
		int32_t audio_ctx = 0;

		float vad_thold = 0.6f;
		float freq_thold = 100.0f;

		bool speed_up = false;
		bool translate = false;
		bool no_fallback = false;
		bool print_special = false;
		bool no_context = true;
		bool no_timestamps = false;
		bool diarize = false;

		std::string language = "en";
		std::string model = "./addons/godot_whisper/models/ggml-tiny.en.bin";
		std::string fname_out;
	};

	whisper_params params;
	whisper_context_params context_parameters{ true };
	Vector<whisper_token> prompt_tokens;
	whisper_context *context_instance = nullptr;

protected:
	static void _bind_methods();

public:
	enum {
		SPEECH_SETTING_SAMPLE_RATE = 16000,
	};
	String transcribe(PackedVector2Array buffer);
	_FORCE_INLINE_ void set_language(String p_language) { params.language = p_language.utf8().get_data(); }
	_FORCE_INLINE_ String get_language() { return String(params.language.c_str()); }
	_FORCE_INLINE_ void set_language_model(String p_model) { params.model = p_model.utf8().get_data(); }
	_FORCE_INLINE_ String get_language_model() { return String(params.model.c_str()); }
	_FORCE_INLINE_ void set_duration_ms(int32_t duration_ms) { params.duration_ms = duration_ms; }
	_FORCE_INLINE_ int32_t get_duration_ms() { return params.duration_ms; }
	_FORCE_INLINE_ void set_use_gpu(bool use_gpu) { context_parameters.use_gpu = use_gpu; }
	_FORCE_INLINE_ bool is_use_gpu() { return context_parameters.use_gpu; }
	SpeechToText();
};

#endif // SPEECH_TO_TEXT_H
