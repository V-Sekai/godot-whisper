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
// #include <mutex>
#include <string>
// #include <thread>
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

		float entropy_threshold = 2.8f;
		int32_t max_context_size = 224;
	};
	Language language = English;
	Ref<WhisperResource> model;
	whisper_params params;
	whisper_full_params full_params;
	whisper_context_params context_parameters{ true };
	whisper_context *context_instance = nullptr;

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
	void set_use_gpu(bool use_gpu);
	_FORCE_INLINE_ bool is_use_gpu() { return context_parameters.use_gpu; }
	SpeechToText();
	~SpeechToText();

	std::atomic<bool> is_running;
	std::vector<float> s_queued_pcmf32;
	std::vector<transcribed_msg> s_transcribed_msgs;
	Mutex s_mutex;
	Thread worker;
	static void run(void *p_ud);
	int t_last_iter;

	_FORCE_INLINE_ void set_entropy_threshold(float entropy_threshold) { params.entropy_threshold = entropy_threshold; }
	_FORCE_INLINE_ float get_entropy_threshold() { return params.entropy_threshold; }

	_FORCE_INLINE_ void set_max_context_size(int32_t max_context_size) { params.max_context_size = max_context_size; }
	_FORCE_INLINE_ int32_t get_max_context_size() { return params.max_context_size; }
	void add_audio_buffer(PackedVector2Array buffer);
	std::vector<transcribed_msg> get_transcribed();
	void start_listen();
	void stop_listen();
	void load_model();
};

#endif // SPEECH_TO_TEXT_H
