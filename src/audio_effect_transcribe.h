#ifndef SPEECH_H
#define SPEECH_H

#include <godot_cpp/classes/audio_effect.hpp>
#include <godot_cpp/classes/audio_effect_instance.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/thread.hpp>
#include <godot_cpp/templates/vector.hpp>

#include <libsamplerate/src/samplerate.h>
#include <whisper.cpp/whisper.h>

using namespace godot;

class AudioEffectTranscribe;
class AudioEffectTranscribeInstance;

class AudioEffectTranscribeInstance : public AudioEffectInstance {
	GDCLASS(AudioEffectTranscribeInstance, AudioEffectInstance);
	friend class AudioEffectTranscribe;
	Ref<AudioEffectTranscribe> base;

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
	
	String text;

	whisper_params params;
	whisper_context_params context_parameters;
	Vector<whisper_token> prompt_tokens;
	whisper_context *context_instance = nullptr;

	bool is_transcribing;
	Thread io_thread;

	Vector<AudioFrame> ring_buffer;

	unsigned int ring_buffer_pos;
	unsigned int ring_buffer_mask;
	unsigned int ring_buffer_read_pos;

	void _io_thread_process();
	void _io_store_buffer();
	static void _thread_callback(void *_instance);
	void _init_recording();
	void _update_buffer();
protected:
	static void _bind_methods() {}
public:
	void init();
	void finish();
	virtual void _process(const void *src_buffer, AudioFrame *dst_buffer, int32_t frame_count) override;
	virtual bool _process_silence() const override;
};

class AudioEffectTranscribe : public AudioEffect {
	GDCLASS(AudioEffectTranscribe, AudioEffect);
	enum {
		IO_BUFFER_SIZE_MS = 1500
	};

	Ref<AudioEffectTranscribeInstance> current_instance;

	void ensure_thread_stopped();
protected:
	static void _bind_methods();

public:
	Ref<AudioEffectInstance> _instantiate() override;
	void set_transcribing_active(bool p_transcribe);
	bool is_transcribing_active() const;
	String get_text() const;

	AudioEffectTranscribe();
	~AudioEffectTranscribe();
};

#endif // SPEECH_H
