#include "speech_to_text.h"
#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/memory.hpp>
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
		p_process_buffer_out[i] = (p_process_buffer_in[i].x + p_process_buffer_in[i].y) / 2.0;
	}
}

Array SpeechToText::transcribe(PackedVector2Array buffer) {
	ERR_FAIL_COND_V(buffer.size() < buffer_len, Array());
	double start = Time::get_singleton()->get_unix_time_from_system();
	ERR_FAIL_COND_V(!context_instance, Array());
	_vector2_array_to_float_array(buffer_len, buffer.ptr(), buffer_float);
	// Speaker frame.
	int result_size = _resample_audio_buffer(
			buffer_float, // Pointer to source buffer
			buffer_len, // Size of source buffer * sizeof(float)
			AudioServer::get_singleton()->get_mix_rate(), // Source sample rate
			SPEECH_SETTING_SAMPLE_RATE, // Target sample rate
			resampled_float);

	whisper_full_params whispher_params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
	whispher_params.print_progress = false;
	whispher_params.print_special = params.print_special;
	whispher_params.print_realtime = false;
	whispher_params.duration_ms = params.duration_ms;
	whispher_params.print_timestamps = true;
	whispher_params.translate = params.translate;
	whispher_params.single_segment = true;
	whispher_params.no_timestamps = false;
	whispher_params.token_timestamps = true;
	whispher_params.max_tokens = params.max_tokens;
	whispher_params.language = params.language.c_str();
	whispher_params.n_threads = params.n_threads;
	whispher_params.audio_ctx = params.audio_ctx;
	whispher_params.speed_up = params.speed_up;
	whispher_params.prompt_tokens = nullptr;
	whispher_params.prompt_n_tokens = 0;
	whispher_params.suppress_non_speech_tokens = true;
	//whispher_params.suppress_blank = true;
	//whispher_params.entropy_thold = 2.8;

	//whispher_params.prompt_tokens = params.no_context ? nullptr : prompt_tokens.data();
	//whispher_params.prompt_n_tokens = params.no_context ? 0 : prompt_tokens.size();

	if (whisper_full(context_instance, whispher_params, resampled_float, result_size) != 0) {
		ERR_PRINT("Failed to process audio");
		return Array();
	}
	const int n_segments = whisper_full_n_segments(context_instance);
	Array results;
	// should be just 1 segment for realtime
	for (int i = 0; i < n_segments; ++i) {
		const int n_tokens = whisper_full_n_tokens(context_instance, i);
		// fprintf(stderr,"tokens: %d\n",n_tokens);
		for (int j = 0; j < n_tokens; j++) {
			auto token = whisper_full_get_token_data(context_instance, i, j);
			auto text = whisper_full_get_token_text(context_instance, i, j);
			Dictionary token_result;
			token_result["id"] = token.id;
			token_result["tid"] = token.tid;
			token_result["p"] = token.p;
			token_result["plog"] = token.plog;
			token_result["pt"] = token.pt;
			token_result["ptsum"] = token.ptsum;
			token_result["t0"] = token.t0;
			token_result["t1"] = token.t1;
			token_result["vlen"] = token.vlen;
			token_result["text"] = text;
			results.append(token_result);
		}
	}
	return results;
}

SpeechToText::SpeechToText() {
	int mix_rate = ProjectSettings::get_singleton()->get_setting("audio/driver/mix_rate");
	buffer_len = params.duration_ms / 1000 * mix_rate;
	buffer_float = (float *)memalloc(sizeof(float) * buffer_len);
	resampled_float = (float *)memalloc(sizeof(float) * params.duration_ms / 1000 * SPEECH_SETTING_SAMPLE_RATE);
	context_instance = whisper_init_from_file_with_params(params.model.c_str(), context_parameters);
}

void SpeechToText::set_language_model(String p_model) {
	//params.model = p_model.utf8().get_data();
	//whisper_free(context_instance);
	//context_instance = whisper_init_from_file_with_params(params.model.c_str(), context_parameters);
}

void SpeechToText::set_duration_ms(int32_t duration_ms) {
	//params.duration_ms = duration_ms;
	//int mix_rate = ProjectSettings::get_singleton()->get_setting("audio/driver/mix_rate");
	//memrealloc(buffer_float, sizeof(float) * params.duration_ms / 1000 * mix_rate);
	//memrealloc(resampled_float, sizeof(float) * params.duration_ms / 1000 * SPEECH_SETTING_SAMPLE_RATE);
}

SpeechToText::~SpeechToText() {
	memfree(buffer_float);
	memfree(resampled_float);
	whisper_free(context_instance);
}

void SpeechToText::_bind_methods() {
	ClassDB::bind_method(D_METHOD("transcribe", "buffer"), &SpeechToText::transcribe);
	ClassDB::bind_method(D_METHOD("get_language"), &SpeechToText::get_language);
	ClassDB::bind_method(D_METHOD("set_language", "language"), &SpeechToText::set_language);
	ClassDB::bind_method(D_METHOD("get_language_model"), &SpeechToText::get_language_model);
	ClassDB::bind_method(D_METHOD("set_language_model", "model"), &SpeechToText::set_language_model);
	ClassDB::bind_method(D_METHOD("get_duration_ms"), &SpeechToText::get_duration_ms);
	ClassDB::bind_method(D_METHOD("set_duration_ms", "duration_ms"), &SpeechToText::set_duration_ms);
	ClassDB::bind_method(D_METHOD("is_use_gpu"), &SpeechToText::is_use_gpu);
	ClassDB::bind_method(D_METHOD("set_use_gpu", "use_gpu"), &SpeechToText::set_use_gpu);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "language"), "set_language", "get_language");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "language_model"), "set_language_model", "get_language_model");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "duration_ms"), "set_duration_ms", "get_duration_ms");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_gpu"), "set_use_gpu", "is_use_gpu");
	BIND_CONSTANT(SPEECH_SETTING_SAMPLE_RATE);
}
