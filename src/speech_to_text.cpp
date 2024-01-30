#include "speech_to_text.h"
#include <libsamplerate/src/samplerate.h>
#include <atomic>
#include <cmath>
#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <string>
#include <vector>

uint32_t _resample_audio_buffer(
		const float *p_src, const uint32_t p_src_frame_count,
		const uint32_t p_src_samplerate, const uint32_t p_target_samplerate,
		float *p_dst) {
	if (p_src_samplerate != p_target_samplerate) {
		SRC_DATA src_data;

		src_data.data_in = p_src;
		src_data.data_out = p_dst;

		src_data.input_frames = p_src_frame_count;
		src_data.src_ratio = (double)p_target_samplerate / (double)p_src_samplerate;
		src_data.output_frames = int(p_src_frame_count * src_data.src_ratio);

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

void _high_pass_filter(PackedFloat32Array &data, float cutoff, float sample_rate) {
	const float rc = 1.0f / (2.0f * Math_PI * cutoff);
	const float dt = 1.0f / sample_rate;
	const float alpha = dt / (rc + dt);

	float y = data[0];

	for (size_t i = 1; i < data.size(); i++) {
		y = alpha * (y + data[i] - data[i - 1]);
		data[i] = y;
	}
}

/** Check if speech is ending. */
bool _vad_simple(PackedFloat32Array &pcmf32, int sample_rate, int last_ms, float vad_thold, float freq_thold, bool verbose) {
	const int n_samples = pcmf32.size();
	const int n_samples_last = (sample_rate * last_ms) / 1000;

	if (n_samples_last >= n_samples) {
		// not enough samples - assume no speech
		return false;
	}

	if (freq_thold > 0.0f) {
		_high_pass_filter(pcmf32, freq_thold, sample_rate);
	}

	float energy_all = 0.0f;
	float energy_last = 0.0f;

	for (int i = 0; i < n_samples; i++) {
		energy_all += fabsf(pcmf32[i]);

		if (i >= n_samples - n_samples_last) {
			energy_last += fabsf(pcmf32[i]);
		}
	}

	energy_all /= n_samples;
	if (n_samples_last != 0) {
		energy_last /= n_samples_last;
	}

	if (verbose) {
		fprintf(stderr, "%s: energy_all: %f, energy_last: %f, vad_thold: %f, freq_thold: %f\n", __func__, energy_all, energy_last, vad_thold, freq_thold);
	}

	if ((energy_all < 0.0001f && energy_last < 0.0001f) == false || energy_last > vad_thold * energy_all) {
		return false;
	}
	return true;
}

SpeechToText::SpeechToText() {
}

void SpeechToText::set_language(int p_language) {
	language = (Language)p_language;
}

int SpeechToText::get_language() {
	return language;
}

std::string SpeechToText::_language_to_code(Language language) {
	switch (language) {
		case Auto:
			return "auto";
		case English:
			return "en";
		case Chinese:
			return "zh";
		case German:
			return "de";
		case Spanish:
			return "es";
		case Russian:
			return "ru";
		case Korean:
			return "ko";
		case French:
			return "fr";
		case Japanese:
			return "ja";
		case Portuguese:
			return "pt";
		case Turkish:
			return "tr";
		case Polish:
			return "pl";
		case Catalan:
			return "ca";
		case Dutch:
			return "nl";
		case Arabic:
			return "ar";
		case Swedish:
			return "sv";
		case Italian:
			return "it";
		case Indonesian:
			return "id";
		case Hindi:
			return "hi";
		case Finnish:
			return "fi";
		case Vietnamese:
			return "vi";
		case Hebrew:
			return "he";
		case Ukrainian:
			return "uk";
		case Greek:
			return "el";
		case Malay:
			return "ms";
		case Czech:
			return "cs";
		case Romanian:
			return "ro";
		case Danish:
			return "da";
		case Hungarian:
			return "hu";
		case Tamil:
			return "ta";
		case Norwegian:
			return "no";
		case Thai:
			return "th";
		case Urdu:
			return "ur";
		case Croatian:
			return "hr";
		case Bulgarian:
			return "bg";
		case Lithuanian:
			return "lt";
		case Latin:
			return "la";
		case Maori:
			return "mi";
		case Malayalam:
			return "ml";
		case Welsh:
			return "cy";
		case Slovak:
			return "sk";
		case Telugu:
			return "te";
		case Persian:
			return "fa";
		case Latvian:
			return "lv";
		case Bengali:
			return "bn";
		case Serbian:
			return "sr";
		case Azerbaijani:
			return "az";
		case Slovenian:
			return "sl";
		case Kannada:
			return "kn";
		case Estonian:
			return "et";
		case Macedonian:
			return "mk";
		case Breton:
			return "br";
		case Basque:
			return "eu";
		case Icelandic:
			return "is";
		case Armenian:
			return "hy";
		case Nepali:
			return "ne";
		case Mongolian:
			return "mn";
		case Bosnian:
			return "bs";
		case Kazakh:
			return "kk";
		case Albanian:
			return "sq";
		case Swahili:
			return "sw";
		case Galician:
			return "gl";
		case Marathi:
			return "mr";
		case Punjabi:
			return "pa";
		case Sinhala:
			return "si";
		case Khmer:
			return "km";
		case Shona:
			return "sn";
		case Yoruba:
			return "yo";
		case Somali:
			return "so";
		case Afrikaans:
			return "af";
		case Occitan:
			return "oc";
		case Georgian:
			return "ka";
		case Belarusian:
			return "be";
		case Tajik:
			return "tg";
		case Sindhi:
			return "sd";
		case Gujarati:
			return "gu";
		case Amharic:
			return "am";
		case Yiddish:
			return "yi";
		case Lao:
			return "lo";
		case Uzbek:
			return "uz";
		case Faroese:
			return "fo";
		case Haitian_Creole:
			return "ht";
		case Pashto:
			return "ps";
		case Turkmen:
			return "tk";
		case Nynorsk:
			return "nn";
		case Maltese:
			return "mt";
		case Sanskrit:
			return "sa";
		case Luxembourgish:
			return "lb";
		case Myanmar:
			return "my";
		case Tibetan:
			return "bo";
		case Tagalog:
			return "tl";
		case Malagasy:
			return "mg";
		case Assamese:
			return "as";
		case Tatar:
			return "tt";
		case Hawaiian:
			return "haw";
		case Lingala:
			return "ln";
		case Hausa:
			return "ha";
		case Bashkir:
			return "ba";
		case Javanese:
			return "jw";
		case Sundanese:
			return "su";
		case Cantonese:
			return "yue";
	}
}

void SpeechToText::set_language_model(Ref<WhisperResource> p_model) {
	model = p_model;
	_load_model();
}

void SpeechToText::_load_model() {
	whisper_free(context_instance);
	UtilityFunctions::print(whisper_print_system_info());
	if (model.is_null()) {
		return;
	}
	PackedByteArray data = model->get_content();
	if (data.is_empty()) {
		return;
	}
	const whisper_context_params context_params{
		_is_use_gpu()
	};
	context_instance = whisper_init_from_buffer_with_params((void *)(data.ptr()), data.size(), context_params);
}

SpeechToText::~SpeechToText() {
	whisper_free(context_instance);
}

PackedFloat32Array SpeechToText::resample(PackedVector2Array buffer) {
	int buffer_len = buffer.size();
	float *buffer_float = (float *)memalloc(sizeof(float) * buffer_len);
	float *resampled_float = (float *)memalloc(sizeof(float) * buffer_len * _get_speech_sample_rate() / AudioServer::get_singleton()->get_mix_rate());
	_vector2_array_to_float_array(buffer_len, buffer.ptr(), buffer_float);
	// Speaker frame.
	int result_size = _resample_audio_buffer(
			buffer_float, // Pointer to source buffer
			buffer_len, // Size of source buffer * sizeof(float)
			AudioServer::get_singleton()->get_mix_rate(), // Source sample rate
			_get_speech_sample_rate(), // Target sample rate
			resampled_float);

	PackedFloat32Array array;
	array.resize(result_size);
	std::memcpy(array.ptrw(), resampled_float, result_size);
	memfree(buffer_float);
	memfree(resampled_float);
	return array;
}

whisper_full_params SpeechToText::_get_whisper_params() {
	whisper_full_params whisper_params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
	// See here for example https://github.com/ggerganov/whisper.cpp/blob/master/examples/stream/stream.cpp#L302
	whisper_params.max_len = 0;
	whisper_params.print_progress = false;
	whisper_params.print_special = false;
	whisper_params.print_realtime = false;
	// This is set later on based on how much frames we can process
	whisper_params.duration_ms = 0;
	whisper_params.print_timestamps = false;
	whisper_params.translate = _is_translate();
	whisper_params.single_segment = false;
	whisper_params.no_timestamps = false;
	whisper_params.token_timestamps = true;
	whisper_params.max_tokens = _get_max_tokens();
	whisper_params.language = _language_to_code(language).c_str();
	whisper_params.n_threads = _get_n_threads();
	whisper_params.speed_up = _is_speed_up();
	whisper_params.prompt_tokens = nullptr;
	whisper_params.prompt_n_tokens = 0;
	whisper_params.suppress_non_speech_tokens = true;
	whisper_params.suppress_blank = true;
	whisper_params.entropy_thold = _get_entropy_threshold();
	whisper_params.temperature = 0.0;
	whisper_params.no_context = true;
	/**
	 * Experimental optimization: Reduce audio_ctx to 15s (half of the chunk
	 * size whisper is designed for) to speed up 2x.
	 * https://github.com/ggerganov/whisper.cpp/issues/137#issuecomment-1318412267
	 */
	whisper_params.audio_ctx = 768;
	return whisper_params;
}

bool SpeechToText::voice_activity_detection(PackedFloat32Array buffer) {
	/* VAD parameters */
	// The most recent 3s.
	const int vad_window_s = 3;
	const int n_samples_vad_window = WHISPER_SAMPLE_RATE * vad_window_s;
	// In VAD, compare the energy of the last 500ms to that of the total 3s.
	const int vad_last_ms = 500;
	const float vad_thold = _get_vad_thold();
	const float freq_thold = _get_freq_thold();
	/**
	 * Simple VAD from the "stream" example in whisper.cpp
	 * https://github.com/ggerganov/whisper.cpp/blob/231bebca7deaf32d268a8b207d15aa859e52dbbe/examples/stream/stream.cpp#L378
	 */
	/* Need enough accumulated audio to do VAD. */
	if ((int)buffer.size() >= n_samples_vad_window) {
		PackedFloat32Array pcmf32_window;
		pcmf32_window.resize(n_samples_vad_window);
		std::memcpy(pcmf32_window.ptrw(), buffer.ptr() + buffer.size() - n_samples_vad_window, n_samples_vad_window * sizeof(float));
		return _vad_simple(pcmf32_window, WHISPER_SAMPLE_RATE, vad_last_ms, vad_thold, freq_thold, false);
	}
	return false;
}

Array SpeechToText::transcribe(PackedFloat32Array buffer) {
	Array return_value;
	whisper_full_params full_params = _get_whisper_params();

	// Keep the last 0.5s of an iteration to the next one for better
	// transcription at begin/end.
	const int n_samples_keep_iter = WHISPER_SAMPLE_RATE * 0.5;

	if (!context_instance) {
		ERR_PRINT("Context instance is null");
		return Array();
	}

	float time_started = Time::get_singleton()->get_ticks_msec();
	full_params.duration_ms = buffer.size() * 1000.0f / WHISPER_SAMPLE_RATE;
	int ret = whisper_full(context_instance, full_params, buffer.ptr(), buffer.size());
	if (ret != 0) {
		ERR_PRINT("Failed to process audio, returned " + rtos(ret));
		return Array();
	}
	const int n_segments = whisper_full_n_segments(context_instance);
	for (int i = 0; i < n_segments; ++i) {
		const int n_tokens = whisper_full_n_tokens(context_instance, i);
		for (int j = 0; j < n_tokens; j++) {
			auto token = whisper_full_get_token_data(context_instance, i, j);
			auto text = whisper_full_get_token_text(context_instance, i, j);
			Dictionary dict;
			dict["text"] = String::utf8(text);
			dict["id"] = token.id;
			dict["p"] = token.p;
			dict["plog"] = token.plog;
			dict["pt"] = token.pt;
			dict["ptsum"] = token.ptsum;
			dict["t0"] = token.t0;
			dict["t1"] = token.t1;
			dict["tid"] = token.tid;
			dict["vlen"] = token.vlen;
			return_value.push_back(dict);
		}
	}

	return return_value;
}

void SpeechToText::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_language"), &SpeechToText::get_language);
	ClassDB::bind_method(D_METHOD("set_language", "language"), &SpeechToText::set_language);
	ClassDB::bind_method(D_METHOD("get_language_model"), &SpeechToText::get_language_model);
	ClassDB::bind_method(D_METHOD("set_language_model", "model"), &SpeechToText::set_language_model);
	ClassDB::bind_method(D_METHOD("transcribe", "buffer"), &SpeechToText::transcribe);
	ClassDB::bind_method(D_METHOD("voice_activity_detection", "buffer"), &SpeechToText::voice_activity_detection);
	ClassDB::bind_method(D_METHOD("resample", "buffer"), &SpeechToText::resample);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "language", PROPERTY_HINT_ENUM, "Auto,English,Chinese,German,Spanish,Russian,Korean,French,Japanese,Portuguese,Turkish,Polish,Catalan,Dutch,Arabic,Swedish,Italian,Indonesian,Hindi,Finnish,Vietnamese,Hebrew,Ukrainian,Greek,Malay,Czech,Romanian,Danish,Hungarian,Tamil,Norwegian,Thai,Urdu,Croatian,Bulgarian,Lithuanian,Latin,Maori,Malayalam,Welsh,Slovak,Telugu,Persian,Latvian,Bengali,Serbian,Azerbaijani,Slovenian,Kannada,Estonian,Macedonian,Breton,Basque,Icelandic,Armenian,Nepali,Mongolian,Bosnian,Kazakh,Albanian,Swahili,Galician,Marathi,Punjabi,Sinhala,Khmer,Shona,Yoruba,Somali,Afrikaans,Occitan,Georgian,Belarusian,Tajik,Sindhi,Gujarati,Amharic,Yiddish,Lao,Uzbek,Faroese,Haitian_Creole,Pashto,Turkmen,Nynorsk,Maltese,Sanskrit,Luxembourgish,Myanmar,Tibetan,Tagalog,Malagasy,Assamese,Tatar,Hawaiian,Lingala,Hausa,Bashkir,Javanese,Sundanese,Cantonese"), "set_language", "get_language");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "language_model", PROPERTY_HINT_RESOURCE_TYPE, "WhisperResource"), "set_language_model", "get_language_model");
}
