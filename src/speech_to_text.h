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
#include <whisper.cpp/include/whisper.h>
#include <godot_cpp/classes/project_settings.hpp>

#include <atomic>
#include <string>
#include <vector>

using namespace godot;

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

private:
	GDCLASS(SpeechToText, Node);
	Language language = English;
	Ref<WhisperResource> model;
	whisper_context *context_instance = nullptr;

	_FORCE_INLINE_ bool _is_use_gpu() { return ProjectSettings::get_singleton()->get("audio/input/transcribe/use_gpu"); }
	_FORCE_INLINE_ float _get_entropy_threshold() { return ProjectSettings::get_singleton()->get("audio/input/transcribe/entropy_treshold"); }
	_FORCE_INLINE_ float _get_freq_thold() { return ProjectSettings::get_singleton()->get("audio/input/transcribe/freq_treshold"); }
	_FORCE_INLINE_ float _get_vad_thold() { return ProjectSettings::get_singleton()->get("audio/input/transcribe/vad_treshold"); }
	_FORCE_INLINE_ int _get_max_tokens() { return ProjectSettings::get_singleton()->get("audio/input/transcribe/max_tokens"); }
	_FORCE_INLINE_ bool _get_speed_up() { return ProjectSettings::get_singleton()->get("audio/input/transcribe/speed_up_2x"); }
	void _load_model();
	std::vector<float> _add_audio_buffer(PackedVector2Array buffer);
	const char *_language_to_code(Language language);

protected:
	static void _bind_methods();

public:
	enum InterpolatorType {
		SRC_SINC_BEST_QUALITY = 0,
		SRC_SINC_MEDIUM_QUALITY = 1,
		SRC_SINC_FASTEST = 2,
		SRC_ZERO_ORDER_HOLD = 3,
		SRC_LINEAR = 4,
	};
	enum SpeechSamplingRate {
		SPEECH_SETTING_SAMPLE_RATE = WHISPER_SAMPLE_RATE
	};
	bool voice_activity_detection(PackedFloat32Array buffer);
	PackedFloat32Array resample(PackedVector2Array buffer, SpeechToText::InterpolatorType interpolator_type);
	Array transcribe(PackedFloat32Array buffer, String initial_prompt, int audio_ctx);
	void set_language(int p_language);
	int get_language();
	void set_language_model(Ref<WhisperResource> p_model);
	_FORCE_INLINE_ Ref<WhisperResource> get_language_model() { return model; }
	SpeechToText();
	~SpeechToText();
};

VARIANT_ENUM_CAST(SpeechToText::SpeechSamplingRate);
VARIANT_ENUM_CAST(SpeechToText::InterpolatorType);
VARIANT_ENUM_CAST(SpeechToText::Language);

#endif // SPEECH_TO_TEXT_H
