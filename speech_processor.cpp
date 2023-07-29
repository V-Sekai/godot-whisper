/*************************************************************************/
/*  speech_processor.cpp                                                 */
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

#include "speech_processor.h"

#include <algorithm>

#define STEREO_CHANNEL_COUNT 2

#define SIGNED_32_BIT_SIZE 2147483647
#define UNSIGNED_32_BIT_SIZE 4294967295
#define SIGNED_16_BIT_SIZE 32767
#define UNSIGNED_16_BIT_SIZE 65536

#define RECORD_MIX_FRAMES 1024 * 2
#define RESAMPLED_BUFFER_FACTOR sizeof(int)

void SpeechToTextProcessor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("start"), &SpeechToTextProcessor::start);
	ClassDB::bind_method(D_METHOD("stop"), &SpeechToTextProcessor::stop);
	ClassDB::bind_method(D_METHOD("compress_buffer", "pcm_byte_array", "output_buffer"),
			&SpeechToTextProcessor::compress_buffer);
	ClassDB::bind_method(D_METHOD("set_streaming_bus", "name"),
			&SpeechToTextProcessor::set_streaming_bus);
	ClassDB::bind_method(D_METHOD("set_error_cancellation_bus", "name"),
			&SpeechToTextProcessor::set_error_cancellation_bus);
	ClassDB::bind_method(D_METHOD("set_audio_input_stream_player", "stream_player"),
			&SpeechToTextProcessor::set_audio_input_stream_player);
	ADD_SIGNAL(MethodInfo("speech_processed",
			PropertyInfo(Variant::DICTIONARY, "packet")));

	BIND_CONSTANT(SPEECH_SETTING_CHANNEL_COUNT);
	BIND_CONSTANT(SPEECH_SETTING_MILLISECONDS_PER_PACKET);
	BIND_CONSTANT(SPEECH_SETTING_BUFFER_BYTE_COUNT);
	BIND_CONSTANT(SPEECH_SETTING_SAMPLE_RATE);
	BIND_CONSTANT(SPEECH_SETTING_BUFFER_FRAME_COUNT);
	BIND_CONSTANT(SPEECH_SETTING_INTERNAL_BUFFER_SIZE);
	BIND_CONSTANT(SPEECH_SETTING_VOICE_SAMPLE_RATE);
	BIND_CONSTANT(SPEECH_SETTING_VOICE_BUFFER_FRAME_COUNT);
	BIND_CONSTANT(SPEECH_SETTING_PCM_BUFFER_SIZE);
	BIND_CONSTANT(SPEECH_SETTING_MILLISECONDS_PER_SECOND);
	BIND_CONSTANT(SPEECH_SETTING_VOICE_PACKET_SAMPLE_RATE);
	BIND_CONSTANT(SPEECH_SETTING_PACKET_DELTA_TIME);
}

uint32_t SpeechToTextProcessor::_resample_audio_buffer(
		const float *p_src, const uint32_t p_src_frame_count,
		const uint32_t p_src_samplerate, const uint32_t p_target_samplerate,
		float *p_dst) {
	if (p_src_samplerate != p_target_samplerate) {
		SRC_DATA src_data;

		src_data.data_in = p_src;
		src_data.data_out = p_dst;

		src_data.input_frames = p_src_frame_count;
		src_data.output_frames = p_src_frame_count * RESAMPLED_BUFFER_FACTOR;

		src_data.src_ratio = (double)p_target_samplerate / (double)p_src_samplerate;
		src_data.end_of_input = 0;

		int error = src_process(libresample_state, &src_data);
		if (error != 0) {
			ERR_PRINT("resample_error!");
			return 0;
		}
		return src_data.output_frames_gen;
	} else {
		memcpy(p_dst, p_src,
				static_cast<size_t>(p_src_frame_count) * sizeof(float));
		return p_src_frame_count;
	}
}

void SpeechToTextProcessor::_get_capture_block(AudioServer *p_audio_server,
		const uint32_t &p_mix_frame_count,
		const Vector2 *p_process_buffer_in,
		float *p_process_buffer_out) {
	for (size_t i = 0; i < p_mix_frame_count; i++) {
		float mono =
				p_process_buffer_in[i].x * 0.5f + p_process_buffer_in[i].y * 0.5f;
		p_process_buffer_out[i] = mono;
	}
}

void SpeechToTextProcessor::_mix_audio(const Vector2 *p_capture_buffer, const Vector2 *p_reference_buffer) {
	if (audio_server) {
		_get_capture_block(audio_server, RECORD_MIX_FRAMES, p_reference_buffer, mono_reference_real_array.ptrw());
		_get_capture_block(audio_server, RECORD_MIX_FRAMES, p_capture_buffer, mono_capture_real_array.ptrw());
		// Speaker frame.
		_resample_audio_buffer(
				mono_reference_real_array.ptr(), // Pointer to source buffer
				RECORD_MIX_FRAMES, // Size of source buffer * sizeof(float)
				AudioServer::get_singleton()->get_mix_rate(), // Source sample rate
				SPEECH_SETTING_VOICE_SAMPLE_RATE, // Target sample rate
				reference_real_array.ptrw() +
						static_cast<size_t>(capture_real_array_offset));
		// Microphone frame.
		uint32_t resampled_frame_count =
				capture_real_array_offset +
				_resample_audio_buffer(
						mono_capture_real_array.ptr(), // Pointer to source buffer
						RECORD_MIX_FRAMES, // Size of source buffer * sizeof(float)
						mix_rate, // Source sample rate
						SPEECH_SETTING_VOICE_SAMPLE_RATE, // Target sample rate
						capture_real_array.ptrw() +
								static_cast<size_t>(capture_real_array_offset));
		capture_real_array_offset = 0;
		const float *capture_real_array_read_ptr = capture_real_array.ptr();
		const float *reference_real_array_read_ptr = reference_real_array.ptr();
		double_t sum = 0;
		while (capture_real_array_offset < resampled_frame_count - SPEECH_SETTING_BUFFER_FRAME_COUNT) {
			sum = 0.0;
			// Speaker frame.
			for (int64_t i = 0; i < SPEECH_SETTING_BUFFER_FRAME_COUNT; i++) {
				float frame_error_cancellation_float = reference_real_array_read_ptr[static_cast<size_t>(capture_real_array_offset) + i];
				mix_reference_buffer.write[i] = webrtc::FloatToS16(frame_error_cancellation_float);
			}
			ref_frame.UpdateFrame(0, mix_reference_buffer.ptr(), SPEECH_SETTING_BUFFER_FRAME_COUNT, SPEECH_SETTING_VOICE_SAMPLE_RATE, webrtc::AudioFrame::kNormalSpeech, webrtc::AudioFrame::kVadActive, 1);
			// Microphone frame.
			for (int64_t i = 0; i < SPEECH_SETTING_BUFFER_FRAME_COUNT; i++) {
				float frame_float = capture_real_array_read_ptr[static_cast<size_t>(capture_real_array_offset) + i];
				sum += fabsf(frame_float);
				mix_capture_buffer.write[i] = webrtc::FloatToS16(frame_float);
			}
			capture_frame.UpdateFrame(0, mix_capture_buffer.ptr(), SPEECH_SETTING_BUFFER_FRAME_COUNT, SPEECH_SETTING_VOICE_SAMPLE_RATE, webrtc::AudioFrame::kNormalSpeech, webrtc::AudioFrame::kVadActive, 1);
			capture_audio->CopyFrom(&capture_frame);
			reference_audio->CopyFrom(&ref_frame);
			reference_audio->SplitIntoFrequencyBands();
			echo_controller->AnalyzeRender(reference_audio.get());
			reference_audio->MergeFrequencyBands();
			echo_controller->AnalyzeCapture(capture_audio.get());
			capture_audio->SplitIntoFrequencyBands();
			hp_filter->Process(capture_audio.get(), true);
			echo_controller->ProcessCapture(capture_audio.get(), nullptr, false);
			capture_audio->MergeFrequencyBands();
			capture_audio->CopyTo(&capture_frame);
			memcpy(mix_byte_array.ptrw(), capture_frame.data(), mix_byte_array.size());
			Dictionary voice_data_packet;
			voice_data_packet["buffer"] = mix_byte_array;
			float average = (float)sum / (float)SPEECH_SETTING_BUFFER_FRAME_COUNT;
			voice_data_packet["loudness"] = average;

			emit_signal("speech_processed", voice_data_packet);

			if (speech_processed) {
				SpeechInput speech_input;
				speech_input.pcm_byte_array = &mix_byte_array;
				speech_input.volume = average;

				speech_processed(&speech_input);
			}

			capture_real_array_offset += SPEECH_SETTING_BUFFER_FRAME_COUNT;
		}

		{
			float *resampled_buffer_write_ptr = capture_real_array.ptrw();
			uint32_t remaining_resampled_buffer_frames =
					(resampled_frame_count - capture_real_array_offset);

			// Copy the remaining frames to the beginning of the buffer for the next
			// around
			if (remaining_resampled_buffer_frames > 0) {
				memmove(resampled_buffer_write_ptr,
						capture_real_array_read_ptr + capture_real_array_offset,
						static_cast<size_t>(remaining_resampled_buffer_frames) *
								sizeof(float));
			}
			capture_real_array_offset = remaining_resampled_buffer_frames;
		}
	}
}

void SpeechToTextProcessor::start() {
	if (!ProjectSettings::get_singleton()->get("audio/enable_audio_input")) {
		print_line("Need to enable Project settings > Audio > Enable Audio Input "
				   "option to use capturing.");
		return;
	}

	if (!audio_input_stream_player || !audio_effect_capture.is_valid() || !audio_effect_error_cancellation_capture.is_valid()) {
		return;
	}
	if (AudioDriver::get_singleton()) {
		mix_rate = AudioDriver::get_singleton()->get_mix_rate();
	}
	audio_input_stream_player->play();
	audio_effect_capture->clear_buffer();
	audio_effect_error_cancellation_capture->clear_buffer();
	webrtc::EchoCanceller3Factory aec_factory = webrtc::EchoCanceller3Factory(aec_config);
	echo_controller = aec_factory.Create(SPEECH_SETTING_VOICE_SAMPLE_RATE, SPEECH_SETTING_CHANNEL_COUNT, SPEECH_SETTING_CHANNEL_COUNT);
	hp_filter = std::make_unique<webrtc::HighPassFilter>(SPEECH_SETTING_VOICE_SAMPLE_RATE, SPEECH_SETTING_CHANNEL_COUNT);

	webrtc::StreamConfig config = webrtc::StreamConfig(SPEECH_SETTING_VOICE_SAMPLE_RATE, SPEECH_SETTING_CHANNEL_COUNT, false);

	reference_audio = std::make_unique<webrtc::AudioBuffer>(
			config.sample_rate_hz(), config.num_channels(),
			config.sample_rate_hz(), config.num_channels(),
			config.sample_rate_hz(), config.num_channels());
	capture_audio = std::make_unique<webrtc::AudioBuffer>(
			config.sample_rate_hz(), config.num_channels(),
			config.sample_rate_hz(), config.num_channels(),
			config.sample_rate_hz(), config.num_channels());
	echo_controller->SetAudioBufferDelay(AudioServer::get_singleton()->get_output_latency());
}

void SpeechToTextProcessor::stop() {
	if (!audio_input_stream_player) {
		return;
	}
	audio_input_stream_player->stop();
}

bool SpeechToTextProcessor::_16_pcm_mono_to_real_stereo(
		const PackedByteArray *p_src_buffer, PackedVector2Array *p_dst_buffer) {
	uint32_t buffer_size = p_src_buffer->size();

	ERR_FAIL_COND_V(buffer_size % 2, false);

	uint32_t frame_count = buffer_size / 2;

	const int16_t *src_buffer_ptr =
			reinterpret_cast<const int16_t *>(p_src_buffer->ptr());
	real_t *real_buffer_ptr = reinterpret_cast<real_t *>(p_dst_buffer->ptrw());

	for (uint32_t i = 0; i < frame_count; i++) {
		float value = ((float)*src_buffer_ptr) / 32768.0f;

		*(real_buffer_ptr + 0) = value;
		*(real_buffer_ptr + 1) = value;

		real_buffer_ptr += 2;
		src_buffer_ptr++;
	}

	return true;
}

Dictionary
SpeechToTextProcessor::compress_buffer(const PackedByteArray &p_pcm_byte_array,
		Dictionary p_output_buffer) {
	if (p_pcm_byte_array.size() != SPEECH_SETTING_PCM_BUFFER_SIZE) {
		ERR_PRINT("SpeechToTextProcessor: PCM buffer is incorrect size!");
		return p_output_buffer;
	}

	PackedByteArray *byte_array = nullptr;
	if (!p_output_buffer.has("byte_array")) {
		byte_array = (PackedByteArray *)&p_output_buffer["byte_array"];
	}

	if (!byte_array) {
		ERR_PRINT("SpeechToTextProcessor: did not provide valid 'byte_array' in "
				  "p_output_buffer argument!");
		return p_output_buffer;
	} else {
		if (byte_array->size() == SPEECH_SETTING_PCM_BUFFER_SIZE) {
			ERR_PRINT("SpeechToTextProcessor: output byte array is incorrect size!");
			return p_output_buffer;
		}
	}

	CompressedSpeechBuffer compressed_speech_buffer;
	compressed_speech_buffer.compressed_byte_array = byte_array;

	p_output_buffer["buffer_size"] = -1;

	p_output_buffer["byte_array"] =
			*compressed_speech_buffer.compressed_byte_array;

	return p_output_buffer;
}

PackedVector2Array
SpeechToTextProcessor::decompress_buffer(void *p_speech_decoder,
		const PackedByteArray &p_read_byte_array,
		const int p_read_size,
		PackedVector2Array p_write_vec2_array) {
	if (p_read_byte_array.size() < p_read_size) {
		ERR_PRINT("SpeechToTextProcessor: read byte_array size!");
		return PackedVector2Array();
	}

	if (decompress_buffer_internal(p_speech_decoder, &p_read_byte_array,
				p_read_size, &p_write_vec2_array)) {
		return p_write_vec2_array;
	}

	return PackedVector2Array();
}

void SpeechToTextProcessor::set_streaming_bus(const String &p_name) {
	if (!audio_server) {
		return;
	}

	int index = audio_server->get_bus_index(p_name);
	if (index != -1) {
		int effect_count = audio_server->get_bus_effect_count(index);
		for (int i = 0; i < effect_count; i++) {
			audio_effect_capture = audio_server->get_bus_effect(index, i);
		}
	}
}

bool SpeechToTextProcessor::set_audio_input_stream_player(
		Node *p_audio_input_stream_player) {
	AudioStreamPlayer *player =
			cast_to<AudioStreamPlayer>(p_audio_input_stream_player);
	ERR_FAIL_COND_V(!player, false);
	if (!audio_server) {
		return false;
	}

	audio_input_stream_player = player;
	return true;
}

void SpeechToTextProcessor::_setup() {}

void SpeechToTextProcessor::set_process_all(bool p_active) {
	set_process(p_active);
	set_physics_process(p_active);
	set_process_input(p_active);
}

void SpeechToTextProcessor::_update_stats() {}

void SpeechToTextProcessor::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY:
			_setup();
			set_process_all(true);
			break;
		case NOTIFICATION_ENTER_TREE:
			mix_byte_array.resize(SPEECH_SETTING_BUFFER_FRAME_COUNT *
					SPEECH_SETTING_BUFFER_BYTE_COUNT);
			mix_byte_array.fill(0);
			mix_reference_buffer.resize(SPEECH_SETTING_BUFFER_FRAME_COUNT);
			mix_reference_buffer.fill(0);
			mix_capture_buffer.resize(SPEECH_SETTING_BUFFER_FRAME_COUNT);
			mix_capture_buffer.fill(0);
			break;
		case NOTIFICATION_EXIT_TREE:
			stop();
			mix_byte_array.resize(0);

			audio_server = nullptr;
			break;
		case NOTIFICATION_PROCESS:
			if (audio_effect_capture.is_valid() && audio_input_stream_player &&
					audio_input_stream_player->is_playing() && audio_effect_error_cancellation_capture.is_valid()) {
				_update_stats();
				// This is pretty ugly, but needed to keep the audio from going out of
				// sync
				while (true) {
					PackedVector2Array audio_frames =
							audio_effect_capture->get_buffer(RECORD_MIX_FRAMES);
					if (audio_frames.size() == 0) {
						break;
					}
					capture_get_calls++;
					capture_get_frames += audio_frames.size();
					capture_pushed_frames = audio_effect_capture->get_pushed_frames();
					capture_discarded_frames = audio_effect_capture->get_discarded_frames();
					capture_ring_limit = audio_effect_capture->get_buffer_length_frames();
					capture_ring_current_size =
							audio_effect_capture->get_frames_available();
					capture_ring_size_sum += capture_ring_current_size;
					capture_ring_max_size =
							(capture_ring_current_size > capture_ring_max_size)
							? capture_ring_current_size
							: capture_ring_max_size;

					PackedVector2Array audio_error_cancellation_frames =
							audio_effect_error_cancellation_capture->get_buffer(RECORD_MIX_FRAMES);
					if (audio_error_cancellation_frames.size() != audio_frames.size()) {
						break;
					}
					capture_error_cancellation_get_calls++;
					capture_error_cancellation_get_frames += audio_error_cancellation_frames.size();
					capture_error_cancellation_pushed_frames = audio_effect_error_cancellation_capture->get_pushed_frames();
					capture_error_cancellation_discarded_frames = audio_effect_error_cancellation_capture->get_discarded_frames();
					capture_error_cancellation_ring_limit = audio_effect_error_cancellation_capture->get_buffer_length_frames();
					capture_error_cancellation_ring_current_size = audio_effect_error_cancellation_capture->get_frames_available();
					capture_error_cancellation_ring_size_sum += capture_error_cancellation_ring_current_size;
					capture_error_cancellation_ring_max_size =
							(capture_error_cancellation_ring_current_size > capture_error_cancellation_ring_max_size)
							? capture_error_cancellation_ring_current_size
							: capture_error_cancellation_ring_max_size;
					_mix_audio(audio_frames.ptrw(), audio_error_cancellation_frames.ptrw());
					record_mix_frames_processed++;
				}
			}
			break;
	}
}

Dictionary SpeechToTextProcessor::get_stats() const {
	Dictionary stats;
	stats["capture_discarded_s"] = 0;
	stats["capture_pushed_s"] = 0;
	stats["capture_ring_limit_s"] = 0;
	stats["capture_ring_current_size_s"] = 0;
	stats["capture_ring_max_size_s"] = 0;
	stats["capture_get_s"] = 0;
	if (mix_rate > 0) {
		stats["capture_discarded_s"] = capture_discarded_frames / (double)mix_rate;
		stats["capture_pushed_s"] = capture_pushed_frames / (double)mix_rate;
		stats["capture_ring_limit_s"] = capture_ring_limit / (double)mix_rate;
		stats["capture_ring_current_size_s"] = capture_ring_current_size / (double)mix_rate;
		stats["capture_ring_max_size_s"] = capture_ring_max_size / (double)mix_rate;
		stats["capture_get_s"] = capture_get_frames / (double)mix_rate;
	}
	stats["capture_ring_mean_size_s"] = 0;
	if (capture_get_calls > 0 && mix_rate > 0) {
		stats["capture_ring_mean_size_s"] = ((double)capture_ring_size_sum) /
				((double)capture_get_calls) /
				(double)mix_rate;
	}
	stats["capture_get_calls"] = capture_get_calls;
	stats["capture_mix_rate"] = mix_rate;
	return stats;
}

SpeechToTextProcessor::SpeechToTextProcessor() {
	capture_discarded_frames = 0;
	capture_pushed_frames = 0;
	capture_ring_limit = 0;
	capture_ring_current_size = 0;
	capture_ring_max_size = 0;
	capture_ring_size_sum = 0;
	capture_get_calls = 0;
	capture_get_frames = 0;

	mono_capture_real_array.resize(RECORD_MIX_FRAMES);
	mono_capture_real_array.fill(0);
	mono_reference_real_array.resize(RECORD_MIX_FRAMES);
	mono_reference_real_array.fill(0);
	capture_real_array.resize(RECORD_MIX_FRAMES * RESAMPLED_BUFFER_FACTOR);
	capture_real_array.fill(0);
	reference_real_array.resize(RECORD_MIX_FRAMES * RESAMPLED_BUFFER_FACTOR);
	reference_real_array.fill(0);
	pcm_byte_array_cache.resize(SPEECH_SETTING_PCM_BUFFER_SIZE);
	pcm_byte_array_cache.fill(0);
	libresample_state = src_new(SRC_SINC_BEST_QUALITY,
			SPEECH_SETTING_CHANNEL_COUNT, &libresample_error);
	audio_server = AudioServer::get_singleton();
}

SpeechToTextProcessor::~SpeechToTextProcessor() {
	libresample_state = src_delete(libresample_state);
}
