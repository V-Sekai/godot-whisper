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
		int32_t step_ms = 3000;
		int32_t keep_ms = 200;
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

		std::string language = "en";
		std::string model = "models/ggml-base.en.bin";
		std::string fname_out;
	};

	whisper_params params;
	whisper_context_params context_parameters;
	Vector<whisper_token> prompt_tokens;
	whisper_context *context_instance = nullptr;

protected:
	static void _bind_methods();

public:
	enum {
		SPEECH_SETTING_SAMPLE_RATE = 16000,
	};
	String transcribe(PackedVector2Array buffer);
	SpeechToText();
};

#endif // SPEECH_TO_TEXT_H
