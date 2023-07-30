/*************************************************************************/
/*  speech_processor.h                                                   */
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

#ifndef SPEECH_PROCESSOR_H
#define SPEECH_PROCESSOR_H

#include "scene/main/node.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/object/class_db.h"
#include "core/object/ref_counted.h"
#include "core/os/mutex.h"
#include "scene/audio/audio_stream_player.h"
#include "servers/audio/audio_stream.h"
#include "servers/audio/effects/audio_effect_capture.h"
#include "servers/audio_server.h"

#include <stdlib.h>
#include <functional>

#include "thirdparty/libsamplerate/src/samplerate.h"

#include "thirdparty/AEC3/api/echo_canceller3_config.h"
#include "thirdparty/AEC3/api/echo_canceller3_factory.h"
#include "thirdparty/AEC3/audio_processing/audio_buffer.h"
#include "thirdparty/AEC3/audio_processing/high_pass_filter.h"
#include "thirdparty/AEC3/audio_processing/include/audio_processing.h"

#include <stdint.h>

class SpeechToTextProcessor : public Node {
	GDCLASS(SpeechToTextProcessor, Node);
	Mutex mutex;

public:
	enum {
		SPEECH_SETTING_CHANNEL_COUNT = 1,
		SPEECH_SETTING_MILLISECONDS_PER_SECOND = 1000,
		SPEECH_SETTING_MILLISECONDS_PER_PACKET = 100,
		SPEECH_SETTING_BUFFER_BYTE_COUNT = sizeof(int16_t),
		SPEECH_SETTING_SAMPLE_RATE = 16000,
		SPEECH_SETTING_BUFFER_FRAME_COUNT = SPEECH_SETTING_SAMPLE_RATE / SPEECH_SETTING_MILLISECONDS_PER_PACKET,
		SPEECH_SETTING_INTERNAL_BUFFER_SIZE = 25 * 3 * 1276,
		SPEECH_SETTING_VOICE_SAMPLE_RATE = SPEECH_SETTING_SAMPLE_RATE,
		SPEECH_SETTING_VOICE_BUFFER_FRAME_COUNT = SPEECH_SETTING_BUFFER_FRAME_COUNT,
		SPEECH_SETTING_PCM_BUFFER_SIZE = SPEECH_SETTING_BUFFER_FRAME_COUNT * SPEECH_SETTING_BUFFER_BYTE_COUNT * SPEECH_SETTING_CHANNEL_COUNT,
		SPEECH_SETTING_VOICE_PACKET_SAMPLE_RATE = SPEECH_SETTING_VOICE_SAMPLE_RATE,
	};

	inline static float SPEECH_SETTING_PACKET_DELTA_TIME = float(SpeechToTextProcessor::SPEECH_SETTING_MILLISECONDS_PER_PACKET) / float(SpeechToTextProcessor::SPEECH_SETTING_MILLISECONDS_PER_SECOND);

protected:
	static void _bind_methods();

private:
	unsigned char internal_buffer[(size_t)SPEECH_SETTING_INTERNAL_BUFFER_SIZE];

public:
	bool decode_buffer(void *p_speech_decoder,
			const PackedByteArray *p_compressed_buffer,
			PackedByteArray *p_pcm_output_buffer,
			const int p_compressed_buffer_size,
			const int p_pcm_output_buffer_size) {
		if (p_pcm_output_buffer->size() != p_pcm_output_buffer_size) {
			ERR_PRINT("OpusCodec: decode_buffer output_buffer_size mismatch!");
			return false;
		}

		return false; // TODO RETURN data.
	}

private:
	int32_t record_mix_frames_processed = 0;

	void *encoder = nullptr;
	AudioServer *audio_server = nullptr;
	AudioStreamPlayer *audio_input_stream_player = nullptr;
	Ref<AudioEffectCapture> audio_effect_capture;
	Ref<AudioEffectCapture> audio_effect_error_cancellation_capture;
	uint32_t mix_rate = 0;
	PackedByteArray mix_byte_array;
	Vector<int16_t> mix_reference_buffer;
	Vector<int16_t> mix_capture_buffer;

	PackedFloat32Array mono_capture_real_array;
	PackedFloat32Array mono_reference_real_array;
	PackedFloat32Array capture_real_array;
	PackedFloat32Array reference_real_array;
	uint32_t capture_real_array_offset = 0;

	PackedByteArray pcm_byte_array_cache;

	// LibResample
	SRC_STATE *libresample_state = nullptr;
	int libresample_error = 0;

	int64_t capture_discarded_frames = 0;
	int64_t capture_pushed_frames = 0;
	int32_t capture_ring_limit = 0;
	int32_t capture_ring_current_size = 0;
	int32_t capture_ring_max_size = 0;
	int64_t capture_ring_size_sum = 0;
	int32_t capture_get_calls = 0;
	int64_t capture_get_frames = 0;

	int64_t capture_error_cancellation_discarded_frames = 0;
	int64_t capture_error_cancellation_pushed_frames = 0;
	int32_t capture_error_cancellation_ring_limit = 0;
	int32_t capture_error_cancellation_ring_current_size = 0;
	int32_t capture_error_cancellation_ring_max_size = 0;
	int64_t capture_error_cancellation_ring_size_sum = 0;
	int32_t capture_error_cancellation_get_calls = 0;
	int64_t capture_error_cancellation_get_frames = 0;

	webrtc::EchoCanceller3Config aec_config;
	std::unique_ptr<webrtc::AudioBuffer> reference_audio;
	std::unique_ptr<webrtc::AudioBuffer> capture_audio;
	const int kLinearOutputRateHz = 16000;
	std::unique_ptr<webrtc::EchoControl> echo_controller;
	std::unique_ptr<webrtc::HighPassFilter> hp_filter;
	webrtc::AudioFrame ref_frame, capture_frame;

public:
	struct SpeechInput {
		PackedByteArray *pcm_byte_array = nullptr;
		float volume = 0.0;
	};

	struct CompressedSpeechBuffer {
		PackedByteArray *compressed_byte_array = nullptr;
		int buffer_size = 0;
	};

	std::function<void(SpeechInput *)> speech_processed;
	void register_speech_processed(
			const std::function<void(SpeechInput *)> &callback) {
		speech_processed = callback;
	}

	void set_error_cancellation_bus(const String &p_name) {
		if (!audio_server) {
			return;
		}

		int index = audio_server->get_bus_index(p_name);
		if (index != -1) {
			int effect_count = audio_server->get_bus_effect_count(index);
			for (int i = 0; i < effect_count; i++) {
				audio_effect_error_cancellation_capture = audio_server->get_bus_effect(index, i);
			}
		}
	}

	uint32_t _resample_audio_buffer(const float *p_src,
			const uint32_t p_src_frame_count,
			const uint32_t p_src_samplerate,
			const uint32_t p_target_samplerate,
			float *p_dst);

	void start();
	void stop();

	static void _get_capture_block(AudioServer *p_audio_server,
			const uint32_t &p_mix_frame_count,
			const Vector2 *p_process_buffer_in,
			float *p_process_buffer_out);

	void _mix_audio(const Vector2 *p_process_buffer_in, const Vector2 *p_error_cancellation_buffer);

	static bool _16_pcm_mono_to_real_stereo(const PackedByteArray *p_src_buffer,
			PackedVector2Array *p_dst_buffer);

	static bool _16_pcm_mono_to_real_mono(
			const PackedByteArray *p_src_buffer, PackedFloat32Array *p_dst_buffer) {
		uint32_t buffer_size = p_src_buffer->size();

		ERR_FAIL_COND_V(buffer_size % 2, false);

		uint32_t frame_count = buffer_size / 2;

		const int16_t *src_buffer_ptr =
				reinterpret_cast<const int16_t *>(p_src_buffer->ptr());
		real_t *real_buffer_ptr = reinterpret_cast<real_t *>(p_dst_buffer->ptrw());

		for (uint32_t i = 0; i < frame_count; i++) {
			float value = ((float)*src_buffer_ptr) / 32768.0f;

			*(real_buffer_ptr) = value;

			real_buffer_ptr++;
			src_buffer_ptr++;
		}

		return true;
	}

	virtual bool decompress_buffer_internal(
			void *speech_decoder, const PackedByteArray *p_read_byte_array,
			const int p_read_size, PackedVector2Array *p_write_vec2_array) {
		if (decode_buffer(speech_decoder, p_read_byte_array, &pcm_byte_array_cache,
					p_read_size, SPEECH_SETTING_PCM_BUFFER_SIZE)) {
			if (_16_pcm_mono_to_real_stereo(&pcm_byte_array_cache,
						p_write_vec2_array)) {
				return true;
			}
		}
		return true;
	}

	virtual Dictionary compress_buffer(const PackedByteArray &p_pcm_byte_array,
			Dictionary p_output_buffer);

	virtual PackedVector2Array
	decompress_buffer(void *p_speech_decoder,
			const PackedByteArray &p_read_byte_array,
			const int p_read_size,
			PackedVector2Array p_write_vec2_array);

	void set_streaming_bus(const String &p_name);
	bool set_audio_input_stream_player(Node *p_audio_input_stream_player);

	void set_process_all(bool p_active);

	Dictionary get_stats() const;

	void _setup();
	void _update_stats();

	void _notification(int p_what);

	SpeechToTextProcessor();
	~SpeechToTextProcessor();
};
#endif // SPEECH_PROCESSOR_H
