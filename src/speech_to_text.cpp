#include "speech_to_text.h"
#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <thread>

#include <libsamplerate/src/samplerate.h>

uint32_t _resample_audio_buffer(
		const float *p_src, const uint32_t p_src_frame_count,
		const uint32_t p_src_samplerate, const uint32_t p_target_samplerate,
		float *p_dst) {
	if (p_src_samplerate != p_target_samplerate) {
		SRC_DATA src_data;

		src_data.data_in = p_src;
		src_data.data_out = p_dst;

		src_data.input_frames = p_src_frame_count;
		src_data.output_frames = p_src_frame_count;

		src_data.src_ratio = (double)p_target_samplerate / (double)p_src_samplerate;
		src_data.end_of_input = 0;
		int error = src_simple(&src_data, SRC_SINC_BEST_QUALITY, 1);
		if (error != 0) {
			ERR_PRINT(String(src_strerror(error)));
			return 0;
		}
		return src_data.output_frames_gen;
	} else {
		memcpy(p_dst, p_src,
				static_cast<size_t>(p_src_frame_count) * sizeof(float));
		return p_src_frame_count;
	}
}

void _vector2_array_to_float_array(const uint32_t &p_mix_frame_count,
		const Vector2 *p_process_buffer_in,
		float *p_process_buffer_out) {
	for (size_t i = 0; i < p_mix_frame_count; i++) {
		float mono = p_process_buffer_in[i].x * 0.5f + p_process_buffer_in[i].y * 0.5f;
		p_process_buffer_out[i] = mono;
	}
}

String SpeechToText::transcribe(PackedVector2Array buffer) {
	if (!context_instance) {
		ERR_PRINT("Context not instantiated.");
		return String();
	}
	ERR_PRINT("before size " + rtos(buffer.size()));
	float buffer_float[buffer.size()];
	_vector2_array_to_float_array(buffer.size(), buffer.ptr(), buffer_float);
	float resampled_float[buffer.size()];
	// Speaker frame.
	int result_size = _resample_audio_buffer(
			buffer_float, // Pointer to source buffer
			buffer.size(), // Size of source buffer * sizeof(float)
			AudioServer::get_singleton()->get_mix_rate(), // Source sample rate
			SPEECH_SETTING_SAMPLE_RATE, // Target sample rate
			resampled_float);

	whisper_full_params whispher_params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
	whispher_params.print_progress = false;
	whispher_params.print_special = params.print_special;
	whispher_params.print_realtime = false;
	whispher_params.print_timestamps = !params.no_timestamps;
	whispher_params.translate = params.translate;
	whispher_params.single_segment = true;
	whispher_params.max_tokens = params.max_tokens;
	whispher_params.language = params.language.c_str();
	whispher_params.n_threads = params.n_threads;
	whispher_params.audio_ctx = params.audio_ctx;
	whispher_params.speed_up = params.speed_up;
	whispher_params.prompt_tokens = nullptr;
	whispher_params.prompt_n_tokens = 0;

	//whispher_params.prompt_tokens = params.no_context ? nullptr : prompt_tokens.data();
	//whispher_params.prompt_n_tokens = params.no_context ? 0 : prompt_tokens.size();

	if (whisper_full(context_instance, whispher_params, resampled_float, result_size) != 0) {
		ERR_PRINT("Failed to process audio");
		return String();
	}

	const int n_segments = whisper_full_n_segments(context_instance);
	String texts;
	for (int i = 0; i < n_segments; ++i) {
		const char *text = whisper_full_get_segment_text(context_instance, i);
		texts += String(text) + "\n";
	}
	return String();
}

SpeechToText::SpeechToText() {
	params.n_threads = MIN(4, (int32_t)std::thread::hardware_concurrency());
	params.step_ms = 3000;
	params.keep_ms = 200;
	params.capture_id = -1;
	params.max_tokens = 32;
	params.audio_ctx = 0;
	params.vad_thold = 0.6f;
	params.freq_thold = 100.0f;
	params.speed_up = false;
	params.translate = false;
	params.no_fallback = false;
	params.print_special = false;
	params.no_context = true;
	params.no_timestamps = false;
	params.language = "en";
	params.model = "./addons/godot_whisper/models/ggml-tiny.en.bin";

	context_instance = whisper_init_from_file_with_params(params.model.c_str(), context_parameters);
}

void SpeechToText::_bind_methods() {
	ClassDB::bind_method(D_METHOD("transcribe", "buffer"), &SpeechToText::transcribe);
	BIND_CONSTANT(SPEECH_SETTING_SAMPLE_RATE);
}
