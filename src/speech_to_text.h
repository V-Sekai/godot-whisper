#ifndef SPEECH_TO_TEXT_H
#define SPEECH_TO_TEXT_H

#include "resource_whisper.h"

#include <godot_cpp/classes/mutex.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/thread.hpp>
#include <godot_cpp/core/mutex_lock.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/callable.hpp>

#include <libsamplerate/src/samplerate.h>
#include <whisper.cpp/whisper.h>

#include <atomic>
#include <string>
#include <vector>

using namespace godot;

struct transcribed_msg {
	std::string text;
	bool is_partial;
};

class SpeechToText : public Node {
public:
	enum Language {
		Auto,
		English,
		Chinese,
		German,
		Spanish,
		Russian,
		Korean,
		French,
		Japanese,
		Portuguese,
		Turkish,
		Polish,
		Catalan,
		Dutch,
		Arabic,
		Swedish,
		Italian,
		Indonesian,
		Hindi,
		Finnish,
		Vietnamese,
		Hebrew,
		Ukrainian,
		Greek,
		Malay,
		Czech,
		Romanian,
		Danish,
		Hungarian,
		Tamil,
		Norwegian,
		Thai,
		Urdu,
		Croatian,
		Bulgarian,
		Lithuanian,
		Latin,
		Maori,
		Malayalam,
		Welsh,
		Slovak,
		Telugu,
		Persian,
		Latvian,
		Bengali,
		Serbian,
		Azerbaijani,
		Slovenian,
		Kannada,
		Estonian,
		Macedonian,
		Breton,
		Basque,
		Icelandic,
		Armenian,
		Nepali,
		Mongolian,
		Bosnian,
		Kazakh,
		Albanian,
		Swahili,
		Galician,
		Marathi,
		Punjabi,
		Sinhala,
		Khmer,
		Shona,
		Yoruba,
		Somali,
		Afrikaans,
		Occitan,
		Georgian,
		Belarusian,
		Tajik,
		Sindhi,
		Gujarati,
		Amharic,
		Yiddish,
		Lao,
		Uzbek,
		Faroese,
		Haitian_Creole,
		Pashto,
		Turkmen,
		Nynorsk,
		Maltese,
		Sanskrit,
		Luxembourgish,
		Myanmar,
		Tibetan,
		Tagalog,
		Malagasy,
		Assamese,
		Tatar,
		Hawaiian,
		Lingala,
		Hausa,
		Bashkir,
		Javanese,
		Sundanese,
		Cantonese
	};

	static SpeechToText *singleton;

private:
	GDCLASS(SpeechToText, Node);

	struct whisper_params {
		int32_t n_threads = MIN(4, (int32_t)OS::get_singleton()->get_processor_count());
		int32_t duration_ms = 5000;
		int32_t max_tokens = 32;

		float vad_thold = 0.3f;
		float freq_thold = 200.0f;

		bool speed_up = false;
		bool translate = false;
		bool no_fallback = false;
		bool no_timestamps = false;

		std::string language = "en";
		std::string model = "./addons/godot_whisper/models/ggml-tiny.en.bin";

		float entropy_threshold = 2.8f;
	};
	Language language = English;
	Ref<WhisperResource> model;
	whisper_params params;
	whisper_full_params full_params;
	whisper_context_params context_parameters{ true };
	whisper_context *context_instance = nullptr;
	int t_last_iter;

protected:
	static void _bind_methods();

public:
	enum {
		SPEECH_SETTING_SAMPLE_RATE = 16000,
	};
	static SpeechToText *get_singleton();
	std::string language_to_code(Language language);
	void set_language(int p_language);
	int get_language();
	void set_language_model(Ref<WhisperResource> p_model);
	_FORCE_INLINE_ Ref<WhisperResource> get_language_model() { return model; }
	_FORCE_INLINE_ void set_use_gpu(bool use_gpu);
	_FORCE_INLINE_ bool is_use_gpu() { return context_parameters.use_gpu; }
	SpeechToText();
	~SpeechToText();

	std::atomic<bool> is_running;
	std::vector<float> s_queued_pcmf32;
	std::vector<transcribed_msg> s_transcribed_msgs;
	Mutex s_mutex; // for accessing shared variables from both main thread and worker thread
	Thread worker;
	void run();

	_FORCE_INLINE_ void set_entropy_threshold(float entropy_threshold) { params.entropy_threshold = entropy_threshold; }
	_FORCE_INLINE_ float get_entropy_threshold() { return params.entropy_threshold; }

	_FORCE_INLINE_ void set_no_timestamps(bool no_timestamps) { params.no_timestamps = no_timestamps; }
	_FORCE_INLINE_ bool is_no_timestamps() { return params.no_timestamps; }

	_FORCE_INLINE_ void set_translate(bool translate) { params.translate = translate; }
	_FORCE_INLINE_ bool is_translate() { return params.translate; }

	_FORCE_INLINE_ void set_speed_up(bool speed_up) { params.speed_up = speed_up; }
	_FORCE_INLINE_ bool is_speed_up() { return params.speed_up; }

	_FORCE_INLINE_ void set_freq_thold(float freq_thold) { params.freq_thold = freq_thold; }
	_FORCE_INLINE_ float get_freq_thold() { return params.freq_thold; }

	_FORCE_INLINE_ void set_vad_thold(float vad_thold) { params.vad_thold = vad_thold; }
	_FORCE_INLINE_ float get_vad_thold() { return params.vad_thold; }

	_FORCE_INLINE_ void set_max_tokens(int max_tokens) { params.max_tokens = max_tokens; }
	_FORCE_INLINE_ int get_max_tokens() { return params.max_tokens; }

	_FORCE_INLINE_ void set_duration_ms(int duration_ms) { params.duration_ms = duration_ms; }
	_FORCE_INLINE_ int get_duration_ms() { return params.duration_ms; }

	_FORCE_INLINE_ void set_n_threads(int n_threads) { params.n_threads = n_threads; }
	_FORCE_INLINE_ int get_n_threads() { return params.n_threads; }

	void add_audio_buffer(PackedVector2Array buffer);
	void start_listen();
	void stop_listen();
	void load_model();
};

#endif // SPEECH_TO_TEXT_H
