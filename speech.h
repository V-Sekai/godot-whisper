/*************************************************************************/
/*  speech.h                                                             */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef SPEECH_H
#define SPEECH_H

#include "core/error/error_macros.h"
#include "core/variant/variant.h"
#include "thirdparty/libsamplerate/src/samplerate.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/os/mutex.h"
#include "core/variant/array.h"
#include "core/variant/dictionary.h"
#include "servers/audio_server.h"

#include "servers/audio/effects/audio_stream_generator.h"
#include "speech_processor.h"
#include "thirdparty/whisper.cpp/whisper.h"

class SpeechToTextPlaybackStats : public RefCounted {
	GDCLASS(SpeechToTextPlaybackStats, RefCounted);

protected:
	static void _bind_methods();

public:
	int64_t playback_ring_current_size = 0;
	int64_t playback_ring_max_size = 0;
	int64_t playback_ring_size_sum = 0;
	double playback_get_frames = 0.0;
	int64_t playback_pushed_calls = 0;
	int64_t playback_discarded_calls = 0;
	int64_t playback_push_buffer_calls = 0;
	int64_t playback_blank_push_calls = 0;
	double playback_position = 0.0;
	double playback_skips = 0.0;

	double jitter_buffer_size_sum = 0.0;
	int64_t jitter_buffer_calls = 0;
	int64_t jitter_buffer_max_size = 0;
	int64_t jitter_buffer_current_size = 0;

	int64_t playback_ring_buffer_length = 0;
	int64_t buffer_frame_count = 0;
	Dictionary get_playback_stats();
};

class SpeechToText : public Node {
	GDCLASS(SpeechToText, Node);

	static const int MAX_AUDIO_BUFFER_ARRAY_SIZE = 10;

	PackedByteArray input_byte_array;
	float volume = 0.0;

	Mutex audio_mutex;

	int skipped_audio_packets = 0;

	SpeechToTextProcessor *speech_processor = nullptr;

	struct InputPacket {
		PackedByteArray compressed_byte_array;
		int buffer_size = 0;
		float loudness = 0.0;
	};

	int current_input_size = 0;
	PackedByteArray compression_output_byte_array;
	InputPacket input_audio_buffer_array[MAX_AUDIO_BUFFER_ARRAY_SIZE];

	struct whisper_params {
		int32_t n_threads  = std::min(4, (int32_t) OS::get_singleton()->get_processor_count());
		int32_t step_ms    = 3000;
		int32_t keep_ms    = 200;
		int32_t capture_id = -1;
		int32_t max_tokens = 32;
		int32_t audio_ctx  = 0;

		float vad_thold    = 0.6f;
		float freq_thold   = 100.0f;

		bool speed_up      = false;
		bool translate     = false;
		bool no_fallback   = false;
		bool print_special = false;
		bool no_context    = true;
		bool no_timestamps = false;

		std::string language  = "en";
		std::string model     = "models/ggml-base.en.bin";
		std::string fname_out;
	};

	whisper_params params;
	std::vector<whisper_token> prompt_tokens;
	whisper_context *whisper_context = nullptr;

private:
	// Assigns the memory to the fixed audio buffer arrays
	void preallocate_buffers();

	// Assigns a callback from the speech_processor to this object.
	void setup_connections();

	// Returns a pointer to the first valid input packet
	// If the current_input_size has exceeded MAX_AUDIO_BUFFER_ARRAY_SIZE,
	// The front packet will be popped from the queue back recursively
	// copying from the back.
	InputPacket *get_next_valid_input_packet();

	// Is responsible for recieving packets from the SpeechToTextProcessor and then
	// compressing them
	void speech_processed(SpeechToTextProcessor::SpeechInput *p_mic_input);

private:
	float BUFFER_DELAY_THRESHOLD = 0.1;
	float STREAM_STANDARD_PITCH = 1.0;
	float STREAM_SPEEDUP_PITCH = 1.5;
	int MAX_JITTER_BUFFER_SIZE = 16;
	int JITTER_BUFFER_SPEEDUP = 12;
	int JITTER_BUFFER_SLOWDOWN = 6;

	bool DEBUG = false;
	bool use_sample_stretching = true;
	PackedFloat32Array uncompressed_audio;

	int packets_received_this_frame = 0;
	int playback_ring_buffer_length = 0;

	PackedVector2Array blank_packet;
	Dictionary player_audio;
	int nearest_shift(int p_number);

public:
	NodePath get_speech_processor() {
		return get_path_to(speech_processor);
	}
	int get_jitter_buffer_speedup() const;
	void set_jitter_buffer_speedup(int p_jitter_buffer_speedup);
	int get_jitter_buffer_slowdown() const;
	void set_jitter_buffer_slowdown(int p_jitter_buffer_slowdown);
	float get_stream_speedup_pitch() const;
	void set_stream_speedup_pitch(float p_stream_speedup_pitch);
	int get_max_jitter_buffer_size() const;
	void set_max_jitter_buffer_size(int p_max_jitter_buffer_size);
	float get_buffer_delay_threshold() const;
	void set_buffer_delay_threshold(float p_buffer_delay_threshold);
	float get_stream_standard_pitch() const;
	void set_stream_standard_pitch(float p_stream_standard_pitch);
	bool get_debug() const;
	void set_debug(bool val);
	bool get_use_sample_stretching() const;
	void set_use_sample_stretching(bool val);
	PackedFloat32Array get_uncompressed_audio() const;
	void set_uncompressed_audio(PackedFloat32Array val);
	int get_packets_received_this_frame() const;
	void set_packets_received_this_frame(int val);
	int get_playback_ring_buffer_length() const;
	void set_playback_ring_buffer_length(int val);
	PackedVector2Array get_blank_packet() const;
	void set_blank_packet(PackedVector2Array val);
	Dictionary get_player_audio();
	void set_player_audio(Dictionary val);
	int calc_playback_ring_buffer_length(Ref<AudioStreamGenerator> audio_stream_generator);

protected:
	static void _bind_methods();

	int get_skipped_audio_packets();

	void clear_skipped_audio_packets();

	virtual PackedVector2Array
	decompress_buffer(void *p_speech_decoder,
			PackedByteArray p_read_byte_array, const int p_read_size,
			PackedVector2Array p_write_vec2_array);
	Array copy_and_clear_buffers();
	bool start_recording();
	bool end_recording();
	void _notification(int p_what);
	void set_streaming_bus(const String &p_name);
	void set_error_cancellation_bus(const String &p_name);
	bool set_audio_input_stream_player(Node *p_audio_stream);
	Dictionary get_stats();
	SpeechToText();
	~SpeechToText();

public:
	void add_player_audio(int p_player_id, Node *p_audio_stream_player);
	void vc_debug_print(String p_str) const;
	void vc_debug_printerr(String p_str) const;
	void on_received_audio_packet(int p_peer_id, int p_sequence_id, PackedByteArray p_packet);
	Dictionary get_playback_stats(Dictionary speech_stat_dict);
	void remove_player_audio(int p_player_id);
	void clear_all_player_audio();
};

#endif // SPEECH_H
