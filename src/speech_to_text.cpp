#include "speech_to_text.h"
#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
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
	ERR_FAIL_COND_V_MSG(buffer.size() > buffer_len, Array(), "Buffer size " + rtos(buffer.size()) + " bigger than max buffer size " + rtos(buffer_len) + ". Increase Duration (ms).");
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

	whisper_full_params whisper_params = whisper_full_default_params(WHISPER_SAMPLING_BEAM_SEARCH);
	whisper_params.max_len = 1;
	whisper_params.print_progress = false;
	whisper_params.print_special = params.print_special;
	whisper_params.print_realtime = false;
	whisper_params.duration_ms = params.duration_ms;
	whisper_params.print_timestamps = true;
	whisper_params.translate = params.translate;
	whisper_params.single_segment = true;
	whisper_params.no_timestamps = false;
	whisper_params.token_timestamps = true;
	whisper_params.max_tokens = params.max_tokens;
	whisper_params.language = params.language.c_str();
	whisper_params.n_threads = params.n_threads;
	whisper_params.audio_ctx = params.audio_ctx;
	whisper_params.speed_up = params.speed_up;
	whisper_params.prompt_tokens = nullptr;
	whisper_params.prompt_n_tokens = 0;
	whisper_params.suppress_non_speech_tokens = true;
	//whisper_params.suppress_blank = true;
	//whisper_params.entropy_thold = 2.8;

	//whisper_params.prompt_tokens = params.no_context ? nullptr : prompt_tokens.data();
	//whisper_params.prompt_n_tokens = params.no_context ? 0 : prompt_tokens.size();

	if (whisper_full(context_instance, whisper_params, resampled_float, result_size) != 0) {
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
	buffer_len = audio_duration * mix_rate;
	buffer_float = (float *)memalloc(sizeof(float) * buffer_len);
	resampled_float = (float *)memalloc(sizeof(float) * audio_duration * SPEECH_SETTING_SAMPLE_RATE);
}

void SpeechToText::set_language(int p_language) {
	language = (Language)p_language;
	params.language = language_to_code(language);
}
int SpeechToText::get_language() {
	return language;
}

std::string SpeechToText::language_to_code(Language language) {
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
	whisper_free(context_instance);
	if (p_model.is_null()) {
		return;
	}
	PackedByteArray data = model->get_content();
	if (data.is_empty()) {
		return;
	}
	context_instance = whisper_init_from_buffer_with_params((void *)(data.ptr()), data.size(), context_parameters);
	UtilityFunctions::print(whisper_print_system_info());
}

void SpeechToText::set_audio_duration(float p_audio_duration) {
	audio_duration = p_audio_duration;
	params.duration_ms = int32_t(audio_duration * 1000);
	int mix_rate = ProjectSettings::get_singleton()->get_setting("audio/driver/mix_rate");
	buffer_len = audio_duration * mix_rate;
	memfree(buffer_float);
	memfree(resampled_float);
	buffer_float = (float *)memalloc(sizeof(float) * buffer_len);
	resampled_float = (float *)memalloc(sizeof(float) * audio_duration * SPEECH_SETTING_SAMPLE_RATE);
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
	ClassDB::bind_method(D_METHOD("get_audio_duration"), &SpeechToText::get_audio_duration);
	ClassDB::bind_method(D_METHOD("set_audio_duration", "duration_ms"), &SpeechToText::set_audio_duration);
	ClassDB::bind_method(D_METHOD("is_use_gpu"), &SpeechToText::is_use_gpu);
	ClassDB::bind_method(D_METHOD("set_use_gpu", "use_gpu"), &SpeechToText::set_use_gpu);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "language", PROPERTY_HINT_ENUM, "Auto,English,Chinese,German,Spanish,Russian,Korean,French,Japanese,Portuguese,Turkish,Polish,Catalan,Dutch,Arabic,Swedish,Italian,Indonesian,Hindi,Finnish,Vietnamese,Hebrew,Ukrainian,Greek,Malay,Czech,Romanian,Danish,Hungarian,Tamil,Norwegian,Thai,Urdu,Croatian,Bulgarian,Lithuanian,Latin,Maori,Malayalam,Welsh,Slovak,Telugu,Persian,Latvian,Bengali,Serbian,Azerbaijani,Slovenian,Kannada,Estonian,Macedonian,Breton,Basque,Icelandic,Armenian,Nepali,Mongolian,Bosnian,Kazakh,Albanian,Swahili,Galician,Marathi,Punjabi,Sinhala,Khmer,Shona,Yoruba,Somali,Afrikaans,Occitan,Georgian,Belarusian,Tajik,Sindhi,Gujarati,Amharic,Yiddish,Lao,Uzbek,Faroese,Haitian_Creole,Pashto,Turkmen,Nynorsk,Maltese,Sanskrit,Luxembourgish,Myanmar,Tibetan,Tagalog,Malagasy,Assamese,Tatar,Hawaiian,Lingala,Hausa,Bashkir,Javanese,Sundanese,Cantonese"), "set_language", "get_language");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "language_model", PROPERTY_HINT_RESOURCE_TYPE, "WhisperResource"), "set_language_model", "get_language_model");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "audio_duration"), "set_audio_duration", "get_audio_duration");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_gpu"), "set_use_gpu", "is_use_gpu");
	BIND_CONSTANT(SPEECH_SETTING_SAMPLE_RATE);
}
