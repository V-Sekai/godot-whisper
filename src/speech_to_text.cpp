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
		float *p_dst,
		SpeechToText::InterpolatorType interpolator_type) {
	if (p_src_samplerate != p_target_samplerate) {
		SRC_DATA src_data;

		src_data.data_in = p_src;
		src_data.data_out = p_dst;

		src_data.input_frames = p_src_frame_count;
		src_data.src_ratio = (double)p_target_samplerate / (double)p_src_samplerate;
		src_data.output_frames = int(p_src_frame_count * src_data.src_ratio);

		src_data.end_of_input = 0;
		int error = src_simple(&src_data, interpolator_type, 1);
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
		UtilityFunctions::print(rtos(energy_all), " ", rtos(energy_last), " ",rtos(vad_thold), " ",rtos(freq_thold));
	}

	if (!(energy_all < 0.0001f && energy_last < 0.0001f) || energy_last > vad_thold * energy_all) {
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

PackedFloat32Array SpeechToText::resample(PackedVector2Array buffer, SpeechToText::InterpolatorType interpolator_type) {
	int64_t buffer_len = buffer.size();
	float *buffer_float = (float *)memalloc(sizeof(float) * buffer_len);
	uint32_t expected_size = buffer_len * WHISPER_SAMPLE_RATE / AudioServer::get_singleton()->get_mix_rate();
	float *resampled_float = (float *)memalloc(sizeof(float) * expected_size);
	_vector2_array_to_float_array(buffer_len, buffer.ptr(), buffer_float);
	// Speaker frame.
	uint32_t result_size = _resample_audio_buffer(
			buffer_float, // Pointer to source buffer
			buffer_len, // Size of source buffer * sizeof(float)
			AudioServer::get_singleton()->get_mix_rate(), // Source sample rate
			WHISPER_SAMPLE_RATE, // Target sample rate
			resampled_float,
			interpolator_type);
	if (result_size != expected_size) {
		ERR_PRINT("size differ exp: " + rtos(expected_size) + " res: " + rtos(result_size));
	}
	PackedFloat32Array array;
	array.resize(result_size);
	std::memcpy(array.ptrw(), resampled_float, result_size * sizeof(float));
	memfree(buffer_float);
	memfree(resampled_float);
	return array;
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
	whisper_full_params whisper_params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
	/**
	 * Experimental optimization: Reduce audio_ctx to 15s (half of the chunk
	 * size whisper is designed for) to speed up 2x.
	 * https://github.com/ggerganov/whisper.cpp/issues/137#issuecomment-1318412267
	 */
	whisper_params.language = _language_to_code(language).c_str();
	//whisper_params.audio_ctx = 768;
	whisper_params.split_on_word = true;
	whisper_params.token_timestamps = true;
	whisper_params.suppress_non_speech_tokens = true;
	whisper_params.single_segment = true;
	whisper_params.max_tokens = _get_max_tokens();
	whisper_params.entropy_thold = _get_entropy_threshold();

	if (!context_instance) {
		ERR_PRINT("Context instance is null");
		return Array();
	}
	//whisper_params.duration_ms = buffer.size() * 1000.0f / WHISPER_SAMPLE_RATE;
	int ret = whisper_full(context_instance, whisper_params, buffer.ptr(), buffer.size());
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

	BIND_ENUM_CONSTANT(SRC_SINC_BEST_QUALITY);
	BIND_ENUM_CONSTANT(SRC_SINC_MEDIUM_QUALITY);
	BIND_ENUM_CONSTANT(SRC_SINC_FASTEST);
	BIND_ENUM_CONSTANT(SRC_ZERO_ORDER_HOLD);
	BIND_ENUM_CONSTANT(SRC_LINEAR);

	BIND_ENUM_CONSTANT(Auto);
	BIND_ENUM_CONSTANT(English);
	BIND_ENUM_CONSTANT(Chinese);
	BIND_ENUM_CONSTANT(German);
	BIND_ENUM_CONSTANT(Spanish);
	BIND_ENUM_CONSTANT(Russian);
	BIND_ENUM_CONSTANT(Korean);
	BIND_ENUM_CONSTANT(French);
	BIND_ENUM_CONSTANT(Japanese);
	BIND_ENUM_CONSTANT(Portuguese);
	BIND_ENUM_CONSTANT(Turkish);
	BIND_ENUM_CONSTANT(Polish);
	BIND_ENUM_CONSTANT(Catalan);
	BIND_ENUM_CONSTANT(Dutch);
	BIND_ENUM_CONSTANT(Arabic);
	BIND_ENUM_CONSTANT(Swedish);
	BIND_ENUM_CONSTANT(Italian);
	BIND_ENUM_CONSTANT(Indonesian);
	BIND_ENUM_CONSTANT(Hindi);
	BIND_ENUM_CONSTANT(Finnish);
	BIND_ENUM_CONSTANT(Vietnamese);
	BIND_ENUM_CONSTANT(Hebrew);
	BIND_ENUM_CONSTANT(Ukrainian);
	BIND_ENUM_CONSTANT(Greek);
	BIND_ENUM_CONSTANT(Malay);
	BIND_ENUM_CONSTANT(Czech);
	BIND_ENUM_CONSTANT(Romanian);
	BIND_ENUM_CONSTANT(Danish);
	BIND_ENUM_CONSTANT(Hungarian);
	BIND_ENUM_CONSTANT(Tamil);
	BIND_ENUM_CONSTANT(Norwegian);
	BIND_ENUM_CONSTANT(Thai);
	BIND_ENUM_CONSTANT(Urdu);
	BIND_ENUM_CONSTANT(Croatian);
	BIND_ENUM_CONSTANT(Bulgarian);
	BIND_ENUM_CONSTANT(Lithuanian);
	BIND_ENUM_CONSTANT(Latin);
	BIND_ENUM_CONSTANT(Maori);
	BIND_ENUM_CONSTANT(Malayalam);
	BIND_ENUM_CONSTANT(Welsh);
	BIND_ENUM_CONSTANT(Slovak);
	BIND_ENUM_CONSTANT(Telugu);
	BIND_ENUM_CONSTANT(Persian);
	BIND_ENUM_CONSTANT(Latvian);
	BIND_ENUM_CONSTANT(Bengali);
	BIND_ENUM_CONSTANT(Serbian);
	BIND_ENUM_CONSTANT(Azerbaijani);
	BIND_ENUM_CONSTANT(Slovenian);
	BIND_ENUM_CONSTANT(Kannada);
	BIND_ENUM_CONSTANT(Estonian);
	BIND_ENUM_CONSTANT(Macedonian);
	BIND_ENUM_CONSTANT(Breton);
	BIND_ENUM_CONSTANT(Basque);
	BIND_ENUM_CONSTANT(Icelandic);
	BIND_ENUM_CONSTANT(Armenian);
	BIND_ENUM_CONSTANT(Nepali);
	BIND_ENUM_CONSTANT(Mongolian);
	BIND_ENUM_CONSTANT(Bosnian);
	BIND_ENUM_CONSTANT(Kazakh);
	BIND_ENUM_CONSTANT(Albanian);
	BIND_ENUM_CONSTANT(Swahili);
	BIND_ENUM_CONSTANT(Galician);
	BIND_ENUM_CONSTANT(Marathi);
	BIND_ENUM_CONSTANT(Punjabi);
	BIND_ENUM_CONSTANT(Sinhala);
	BIND_ENUM_CONSTANT(Khmer);
	BIND_ENUM_CONSTANT(Shona);
	BIND_ENUM_CONSTANT(Yoruba);
	BIND_ENUM_CONSTANT(Somali);
	BIND_ENUM_CONSTANT(Afrikaans);
	BIND_ENUM_CONSTANT(Occitan);
	BIND_ENUM_CONSTANT(Georgian);
	BIND_ENUM_CONSTANT(Belarusian);
	BIND_ENUM_CONSTANT(Tajik);
	BIND_ENUM_CONSTANT(Sindhi);
	BIND_ENUM_CONSTANT(Gujarati);
	BIND_ENUM_CONSTANT(Amharic);
	BIND_ENUM_CONSTANT(Yiddish);
	BIND_ENUM_CONSTANT(Lao);
	BIND_ENUM_CONSTANT(Uzbek);
	BIND_ENUM_CONSTANT(Faroese);
	BIND_ENUM_CONSTANT(Haitian_Creole);
	BIND_ENUM_CONSTANT(Pashto);
	BIND_ENUM_CONSTANT(Turkmen);
	BIND_ENUM_CONSTANT(Nynorsk);
	BIND_ENUM_CONSTANT(Maltese);
	BIND_ENUM_CONSTANT(Sanskrit);
	BIND_ENUM_CONSTANT(Luxembourgish);
	BIND_ENUM_CONSTANT(Myanmar);
	BIND_ENUM_CONSTANT(Tibetan);
	BIND_ENUM_CONSTANT(Tagalog);
	BIND_ENUM_CONSTANT(Malagasy);
	BIND_ENUM_CONSTANT(Assamese);
	BIND_ENUM_CONSTANT(Tatar);
	BIND_ENUM_CONSTANT(Hawaiian);
	BIND_ENUM_CONSTANT(Lingala);
	BIND_ENUM_CONSTANT(Hausa);
	BIND_ENUM_CONSTANT(Bashkir);
	BIND_ENUM_CONSTANT(Javanese);
	BIND_ENUM_CONSTANT(Sundanese);
	BIND_ENUM_CONSTANT(Cantonese);
	
	BIND_ENUM_CONSTANT(SPEECH_SETTING_SAMPLE_RATE);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "language", PROPERTY_HINT_ENUM, "Auto,English,Chinese,German,Spanish,Russian,Korean,French,Japanese,Portuguese,Turkish,Polish,Catalan,Dutch,Arabic,Swedish,Italian,Indonesian,Hindi,Finnish,Vietnamese,Hebrew,Ukrainian,Greek,Malay,Czech,Romanian,Danish,Hungarian,Tamil,Norwegian,Thai,Urdu,Croatian,Bulgarian,Lithuanian,Latin,Maori,Malayalam,Welsh,Slovak,Telugu,Persian,Latvian,Bengali,Serbian,Azerbaijani,Slovenian,Kannada,Estonian,Macedonian,Breton,Basque,Icelandic,Armenian,Nepali,Mongolian,Bosnian,Kazakh,Albanian,Swahili,Galician,Marathi,Punjabi,Sinhala,Khmer,Shona,Yoruba,Somali,Afrikaans,Occitan,Georgian,Belarusian,Tajik,Sindhi,Gujarati,Amharic,Yiddish,Lao,Uzbek,Faroese,Haitian_Creole,Pashto,Turkmen,Nynorsk,Maltese,Sanskrit,Luxembourgish,Myanmar,Tibetan,Tagalog,Malagasy,Assamese,Tatar,Hawaiian,Lingala,Hausa,Bashkir,Javanese,Sundanese,Cantonese"), "set_language", "get_language");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "language_model", PROPERTY_HINT_RESOURCE_TYPE, "WhisperResource"), "set_language_model", "get_language_model");
}
