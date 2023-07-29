/**************************************************************************/
/*  speech.cpp                                                            */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "core/error/error_macros.h"
#include "core/string/print_string.h"
#include "core/variant/variant.h"
#include "scene/2d/audio_stream_player_2d.h"
#include "scene/3d/audio_stream_player_3d.h"

#include "speech.h"
#include "speech_processor.h"

void SpeechToText::preallocate_buffers() {
	input_byte_array.resize(SpeechToTextProcessor::SPEECH_SETTING_PCM_BUFFER_SIZE);
	input_byte_array.fill(0);
	compression_output_byte_array.resize(
			SpeechToTextProcessor::SPEECH_SETTING_PCM_BUFFER_SIZE);
	compression_output_byte_array.fill(0);
	for (int i = 0; i < MAX_AUDIO_BUFFER_ARRAY_SIZE; i++) {
		input_audio_buffer_array[i].compressed_byte_array.resize(
				SpeechToTextProcessor::SPEECH_SETTING_PCM_BUFFER_SIZE);
		input_audio_buffer_array[i].compressed_byte_array.fill(0);
	}
}

void SpeechToText::setup_connections() {
	if (speech_processor) {
		speech_processor->register_speech_processed(
				std::function<void(SpeechToTextProcessor::SpeechInput *)>(std::bind(
						&SpeechToText::speech_processed, this, std::placeholders::_1)));
	}
}

SpeechToText::InputPacket *SpeechToText::get_next_valid_input_packet() {
	if (current_input_size < MAX_AUDIO_BUFFER_ARRAY_SIZE) {
		InputPacket *input_packet = &input_audio_buffer_array[current_input_size];
		current_input_size++;
		return input_packet;
	} else {
		for (int i = MAX_AUDIO_BUFFER_ARRAY_SIZE - 1; i > 0; i--) {
			memcpy(input_audio_buffer_array[i - 1].compressed_byte_array.ptrw(),
					input_audio_buffer_array[i].compressed_byte_array.ptr(),
					SpeechToTextProcessor::SPEECH_SETTING_PCM_BUFFER_SIZE);

			input_audio_buffer_array[i - 1].buffer_size =
					input_audio_buffer_array[i].buffer_size;
			input_audio_buffer_array[i - 1].loudness =
					input_audio_buffer_array[i].loudness;
		}
		skipped_audio_packets++;
		return &input_audio_buffer_array[MAX_AUDIO_BUFFER_ARRAY_SIZE - 1];
	}
}

void SpeechToText::speech_processed(SpeechToTextProcessor::SpeechInput *p_mic_input) {
	// Copy the raw PCM data from the SpeechInput packet to the input byte array
	PackedByteArray *mic_input_byte_array = p_mic_input->pcm_byte_array;
	memcpy(input_byte_array.ptrw(), mic_input_byte_array->ptr(),
			SpeechToTextProcessor::SPEECH_SETTING_PCM_BUFFER_SIZE);

	// Copy the raw PCM data from the SpeechInput packet to the input byte array
	PackedVector2Array write_vec2_array;
	bool ok = SpeechToTextProcessor::_16_pcm_mono_to_real_stereo(&input_byte_array, &write_vec2_array);
	ERR_FAIL_COND(!ok);
	{
		MutexLock mutex_lock(audio_mutex);

		whisper_params params;
		std::vector<whisper_token> prompt_tokens;
		whisper_context *ctx;

		params.n_threads = std::min(4, (int32_t)std::thread::hardware_concurrency());
		params.step_ms = 3000;
		params.keep_ms = 200;
		params.capture_id = -1;
		params.max_tokens = 32;
		params.audio_ctx = 0;
		params.vad_thold = 0.6f;
		params.freq_thold = 100.0f;
		params.speed_up = false;
		params.translate = false;
		params.no_fallback = false;
		params.print_special = false;
		params.no_context = true;
		params.no_timestamps = false;
		params.language = "en";
		params.model = "models/ggml-base.en.bin";

		ctx = whisper_init_from_file(params.model.c_str());

		if (!ctx) {
			ERR_PRINT("Failed to initialize Whisper context");
			return;
		}

		whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
		wparams.print_progress = false;
		wparams.print_special = params.print_special;
		wparams.print_realtime = false;
		wparams.print_timestamps = !params.no_timestamps;
		wparams.translate = params.translate;
		wparams.single_segment = true;
		wparams.max_tokens = params.max_tokens;
		wparams.language = params.language.c_str();
		wparams.n_threads = params.n_threads;
		wparams.audio_ctx = params.audio_ctx;
		wparams.speed_up = params.speed_up;
		wparams.prompt_tokens = params.no_context ? nullptr : prompt_tokens.data();
		wparams.prompt_n_tokens = params.no_context ? 0 : prompt_tokens.size();

		PackedFloat32Array float_array;
		for (const Vector2 &vector : write_vec2_array) {
			float_array.push_back(vector.x);
			float_array.push_back(vector.y);
		}
		if (whisper_full(ctx, wparams, float_array.ptr(), float_array.size()) != 0) {
			ERR_PRINT("Failed to process audio");
			return;
		}

		const int n_segments = whisper_full_n_segments(ctx);
		for (int i = 0; i < n_segments; ++i) {
			const char *text = whisper_full_get_segment_text(ctx, i);
			print_line(vformat("%s", text));
			add_text(text);
		}
	}
}

int SpeechToText::get_jitter_buffer_speedup() const {
	return JITTER_BUFFER_SPEEDUP;
}

void SpeechToText::set_jitter_buffer_speedup(int p_jitter_buffer_speedup) {
	JITTER_BUFFER_SPEEDUP = p_jitter_buffer_speedup;
}

int SpeechToText::get_jitter_buffer_slowdown() const {
	return JITTER_BUFFER_SLOWDOWN;
}

void SpeechToText::set_jitter_buffer_slowdown(int p_jitter_buffer_slowdown) {
	JITTER_BUFFER_SLOWDOWN = p_jitter_buffer_slowdown;
}

float SpeechToText::get_stream_speedup_pitch() const {
	return STREAM_SPEEDUP_PITCH;
}

void SpeechToText::set_stream_speedup_pitch(float p_stream_speedup_pitch) {
	STREAM_SPEEDUP_PITCH = p_stream_speedup_pitch;
}

int SpeechToText::get_max_jitter_buffer_size() const {
	return MAX_JITTER_BUFFER_SIZE;
}

void SpeechToText::set_max_jitter_buffer_size(int p_max_jitter_buffer_size) {
	MAX_JITTER_BUFFER_SIZE = p_max_jitter_buffer_size;
}

float SpeechToText::get_buffer_delay_threshold() const {
	return BUFFER_DELAY_THRESHOLD;
}

void SpeechToText::set_buffer_delay_threshold(float p_buffer_delay_threshold) {
	BUFFER_DELAY_THRESHOLD = p_buffer_delay_threshold;
}

float SpeechToText::get_stream_standard_pitch() const {
	return STREAM_STANDARD_PITCH;
}

void SpeechToText::set_stream_standard_pitch(float p_stream_standard_pitch) {
	STREAM_STANDARD_PITCH = p_stream_standard_pitch;
}

bool SpeechToText::get_debug() const {
	return DEBUG;
}

void SpeechToText::set_debug(bool val) {
	DEBUG = val;
}

bool SpeechToText::get_use_sample_stretching() const {
	return use_sample_stretching;
}

void SpeechToText::set_use_sample_stretching(bool val) {
	use_sample_stretching = val;
}

PackedVector2Array SpeechToText::get_uncompressed_audio() const {
	return uncompressed_audio;
}

void SpeechToText::set_uncompressed_audio(PackedVector2Array val) {
	uncompressed_audio = val;
}

int SpeechToText::get_packets_received_this_frame() const {
	return packets_received_this_frame;
}

void SpeechToText::set_packets_received_this_frame(int val) {
	packets_received_this_frame = val;
}

int SpeechToText::get_playback_ring_buffer_length() const {
	return playback_ring_buffer_length;
}

void SpeechToText::set_playback_ring_buffer_length(int val) {
	playback_ring_buffer_length = val;
}

PackedVector2Array SpeechToText::get_blank_packet() const {
	return blank_packet;
}

void SpeechToText::set_blank_packet(PackedVector2Array val) {
	blank_packet = val;
}

Dictionary SpeechToText::get_player_audio() {
	return player_audio;
}

void SpeechToText::set_player_audio(Dictionary val) {
	player_audio = val;
}

int SpeechToText::nearest_shift(int p_number) {
	for (int32_t i = 30; i-- > 0;) {
		if (p_number & (1 << i)) {
			return i + 1;
		}
	}
	return 0;
}

int SpeechToText::calc_playback_ring_buffer_length(Ref<AudioStreamGenerator> audio_stream_generator) {
	int target_buffer_size = int(audio_stream_generator->get_mix_rate() * audio_stream_generator->get_buffer_length());
	return (1 << nearest_shift(target_buffer_size));
}

void SpeechToText::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_skipped_audio_packets"),
			&SpeechToText::get_skipped_audio_packets);
	ClassDB::bind_method(D_METHOD("clear_skipped_audio_packets"),
			&SpeechToText::clear_skipped_audio_packets);
	ClassDB::bind_method(D_METHOD("copy_and_clear_buffers"),
			&SpeechToText::copy_and_clear_buffers);
	ClassDB::bind_method(D_METHOD("get_stats"), &SpeechToText::get_stats);

	ClassDB::bind_method(D_METHOD("start_recording"), &SpeechToText::start_recording);
	ClassDB::bind_method(D_METHOD("end_recording"), &SpeechToText::end_recording);

	ClassDB::bind_method(D_METHOD("set_streaming_bus", "bus"),
			&SpeechToText::set_streaming_bus);
	ClassDB::bind_method(D_METHOD("set_audio_input_stream_player", "player"),
			&SpeechToText::set_audio_input_stream_player);
	ClassDB::bind_method(D_METHOD("set_buffer_delay_threshold", "buffer_delay_threshold"),
			&SpeechToText::set_buffer_delay_threshold);
	ClassDB::bind_method(D_METHOD("get_buffer_delay_threshold"),
			&SpeechToText::get_buffer_delay_threshold);
	ClassDB::bind_method(D_METHOD("get_stream_standard_pitch"),
			&SpeechToText::get_stream_standard_pitch);
	ClassDB::bind_method(D_METHOD("set_stream_standard_pitch", "stream_standard_pitch"),
			&SpeechToText::set_stream_standard_pitch);
	ClassDB::bind_method(D_METHOD("get_stream_speedup_pitch"),
			&SpeechToText::get_stream_standard_pitch);
	ClassDB::bind_method(D_METHOD("set_stream_speedup_pitch", "stream_speedup_pitch"),
			&SpeechToText::set_stream_standard_pitch);
	ClassDB::bind_method(D_METHOD("get_max_jitter_buffer_size"),
			&SpeechToText::get_max_jitter_buffer_size);
	ClassDB::bind_method(D_METHOD("set_max_jitter_buffer_size", "max_jitter_buffer_size"),
			&SpeechToText::set_max_jitter_buffer_size);
	ClassDB::bind_method(D_METHOD("get_jitter_buffer_speedup"),
			&SpeechToText::get_jitter_buffer_speedup);
	ClassDB::bind_method(D_METHOD("set_jitter_buffer_speedup", "jitter_buffer_speedup"),
			&SpeechToText::set_jitter_buffer_speedup);
	ClassDB::bind_method(D_METHOD("get_jitter_buffer_slowdown"),
			&SpeechToText::get_jitter_buffer_slowdown);
	ClassDB::bind_method(D_METHOD("set_jitter_buffer_slowdown", "jitter_buffer_slowdown"),
			&SpeechToText::set_jitter_buffer_slowdown);
	ClassDB::bind_method(D_METHOD("get_debug"),
			&SpeechToText::get_debug);
	ClassDB::bind_method(D_METHOD("set_debug", "debug"),
			&SpeechToText::set_debug);
	ClassDB::bind_method(D_METHOD("get_uncompressed_audio"),
			&SpeechToText::get_uncompressed_audio);
	ClassDB::bind_method(D_METHOD("set_uncompressed_audio", "uncompressed_audio"),
			&SpeechToText::set_uncompressed_audio);
	ClassDB::bind_method(D_METHOD("get_packets_received_this_frame"),
			&SpeechToText::get_packets_received_this_frame);
	ClassDB::bind_method(D_METHOD("set_packets_received_this_frame", "packets_received_this_frame"),
			&SpeechToText::set_packets_received_this_frame);
	ClassDB::bind_method(D_METHOD("get_playback_ring_buffer_length"),
			&SpeechToText::get_playback_ring_buffer_length);
	ClassDB::bind_method(D_METHOD("set_playback_ring_buffer_length", "playback_ring_buffer_length"),
			&SpeechToText::set_playback_ring_buffer_length);
	ClassDB::bind_method(D_METHOD("get_blank_packet"),
			&SpeechToText::get_blank_packet);
	ClassDB::bind_method(D_METHOD("set_blank_packet", "blank_packet"),
			&SpeechToText::set_blank_packet);
	ClassDB::bind_method(D_METHOD("get_player_audio"),
			&SpeechToText::get_player_audio);
	ClassDB::bind_method(D_METHOD("set_player_audio", "player_audio"),
			&SpeechToText::set_player_audio);
	ClassDB::bind_method(D_METHOD("get_use_sample_stretching"),
			&SpeechToText::get_use_sample_stretching);
	ClassDB::bind_method(D_METHOD("set_use_sample_stretching", "use_sample_stretching"),
			&SpeechToText::set_use_sample_stretching);
	ClassDB::bind_method(D_METHOD("calc_playback_ring_buffer_length", "generator"),
			&SpeechToText::calc_playback_ring_buffer_length);
	ClassDB::bind_method(D_METHOD("add_player_audio", "player_id", "audio_stream_player"),
			&SpeechToText::add_player_audio);
	ClassDB::bind_method(D_METHOD("on_received_audio_packet", "peer_id", "sequence_id", "packet"),
			&SpeechToText::on_received_audio_packet);
	ClassDB::bind_method(D_METHOD("get_playback_stats", "speech_stat"),
			&SpeechToText::get_playback_stats);
	ClassDB::bind_method(D_METHOD("remove_player_audio", "player_id"),
			&SpeechToText::remove_player_audio);
	ClassDB::bind_method(D_METHOD("clear_all_player_audio"),
			&SpeechToText::clear_all_player_audio);
	ClassDB::bind_method(D_METHOD("set_error_cancellation_bus", "name"),
			&SpeechToText::set_error_cancellation_bus);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "BUFFER_DELAY_THRESHOLD"), "set_buffer_delay_threshold",
			"get_buffer_delay_threshold");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "STREAM_STANDARD_PITCH"), "set_stream_standard_pitch",
			"get_stream_standard_pitch");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "MAX_JITTER_BUFFER_SIZE"), "set_max_jitter_buffer_size",
			"get_max_jitter_buffer_size");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "STREAM_SPEEDUP_PITCH"), "set_stream_speedup_pitch",
			"get_stream_speedup_pitch");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "JITTER_BUFFER_SLOWDOWN"), "set_jitter_buffer_slowdown",
			"get_jitter_buffer_slowdown");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "JITTER_BUFFER_SPEEDUP"), "set_jitter_buffer_speedup",
			"get_jitter_buffer_speedup");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "DEBUG"), "set_debug",
			"get_debug");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_sample_stretching"), "set_use_sample_stretching",
			"get_use_sample_stretching");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR2_ARRAY, "uncompressed_audio", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_uncompressed_audio",
			"get_uncompressed_audio");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "packets_received_this_frame"), "set_packets_received_this_frame",
			"get_packets_received_this_frame");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "playback_ring_buffer_length"), "set_playback_ring_buffer_length",
			"get_playback_ring_buffer_length");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR2_ARRAY, "blank_packet", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_blank_packet",
			"get_blank_packet");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "player_audio", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_player_audio",
			"get_player_audio");
}

int SpeechToText::get_skipped_audio_packets() {
	return skipped_audio_packets;
}

void SpeechToText::clear_skipped_audio_packets() {
	skipped_audio_packets = 0;
}

PackedVector2Array SpeechToText::decompress_buffer(void *p_speech_decoder, PackedByteArray p_read_byte_array, const int p_read_size, PackedVector2Array p_write_vec2_array) {
	if (p_read_byte_array.size() < p_read_size) {
		ERR_PRINT("SpeechDecoder: read byte_array size!");
		return PackedVector2Array();
	}

	if (speech_processor->decompress_buffer_internal(
				p_speech_decoder, &p_read_byte_array, p_read_size,
				&p_write_vec2_array)) {
		return p_write_vec2_array;
	}

	return PackedVector2Array();
}

Array SpeechToText::copy_and_clear_buffers() {
	MutexLock mutex_lock(audio_mutex);

	Array output_array;
	output_array.resize(current_input_size);

	for (int i = 0; i < current_input_size; i++) {
		Dictionary dict;

		dict["byte_array"] = input_audio_buffer_array[i].compressed_byte_array;
		dict["buffer_size"] = input_audio_buffer_array[i].buffer_size;
		dict["loudness"] = input_audio_buffer_array[i].loudness;

		output_array[i] = dict;
	}
	current_input_size = 0;

	return output_array;
}

bool SpeechToText::start_recording() {
	if (speech_processor) {
		speech_processor->start();
		skipped_audio_packets = 0;
		return true;
	}

	return false;
}

bool SpeechToText::end_recording() {
	bool result = true;
	if (speech_processor) {
		speech_processor->stop();
	} else {
		result = false;
	}
	if (has_method("clear_all_player_audio")) {
		call("clear_all_player_audio");
	}
	return result;
}

void SpeechToText::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			setup_connections();
			if (speech_processor) {
				add_child(speech_processor, true);
				speech_processor->set_owner(get_owner());
			}
			uncompressed_audio.resize(
					SpeechToTextProcessor::SPEECH_SETTING_BUFFER_FRAME_COUNT);
			uncompressed_audio.fill(Vector2());
			set_process_internal(true);
			break;
		}
		case NOTIFICATION_EXIT_TREE: {
			if (speech_processor) {
				remove_child(speech_processor);
			}
			break;
		}
		case NOTIFICATION_POSTINITIALIZE: {
			blank_packet.resize(SpeechToTextProcessor::SPEECH_SETTING_BUFFER_FRAME_COUNT);
			blank_packet.fill(Vector2());
			for (int32_t i = 0; i < SpeechToTextProcessor::SPEECH_SETTING_BUFFER_FRAME_COUNT; i++) {
				blank_packet.write[i] = Vector2();
			}
			break;
		}
		case NOTIFICATION_INTERNAL_PROCESS: {
			Array keys = player_audio.keys();
			for (int32_t i = 0; i < keys.size(); i++) {
				Variant key = keys[i];
				if (!player_audio.has(key)) {
					continue;
				}
				Dictionary elem = player_audio[key];
				if (!elem.has("speech_decoder")) {
					continue;
				}
				if (!elem.has("audio_stream_player")) {
					continue;
				}
				if (!elem.has("jitter_buffer")) {
					continue;
				}
				Array jitter_buffer = elem["jitter_buffer"];
				if (!elem.has("playback_stats")) {
					continue;
				}
				Ref<SpeechToTextPlaybackStats> playback_stats = elem["playback_stats"];
				Dictionary dict = player_audio[key];
				dict["packets_received_this_frame"] = 0;
				player_audio[key] = dict;
			}
			packets_received_this_frame = 0;
			break;
		}
		default: {
			break;
		}
	}
}

void SpeechToText::set_streaming_bus(const String &p_name) {
	if (speech_processor) {
		speech_processor->set_streaming_bus(p_name);
	}
}

void SpeechToText::set_error_cancellation_bus(const String &p_name) {
	if (speech_processor) {
		speech_processor->set_error_cancellation_bus(p_name);
	}
}

bool SpeechToText::set_audio_input_stream_player(Node *p_audio_stream) {
	AudioStreamPlayer *player = cast_to<AudioStreamPlayer>(p_audio_stream);
	ERR_FAIL_NULL_V(player, false);
	if (!speech_processor) {
		return false;
	}
	speech_processor->set_audio_input_stream_player(player);
	return true;
}

Dictionary SpeechToText::get_stats() {
	if (speech_processor) {
		return speech_processor->get_stats();
	}
	return Dictionary();
}

SpeechToText::SpeechToText() {
	speech_processor = memnew(SpeechToTextProcessor);
	preallocate_buffers();
}

SpeechToText::~SpeechToText() {
	memdelete(speech_processor);
}

void SpeechToText::add_player_audio(int p_player_id, Node *p_audio_stream_player) {
	if (cast_to<AudioStreamPlayer>(p_audio_stream_player) || cast_to<AudioStreamPlayer2D>(p_audio_stream_player) || cast_to<AudioStreamPlayer3D>(p_audio_stream_player)) {
		if (!player_audio.has(p_player_id)) {
			Ref<AudioStreamGenerator> new_generator;
			new_generator.instantiate();
			new_generator->set_mix_rate(SpeechToTextProcessor::SPEECH_SETTING_VOICE_PACKET_SAMPLE_RATE);
			new_generator->set_buffer_length(BUFFER_DELAY_THRESHOLD);
			playback_ring_buffer_length = calc_playback_ring_buffer_length(new_generator);
			p_audio_stream_player->call("set_stream", new_generator);
			p_audio_stream_player->call("set_bus", "VoiceOutput");
			p_audio_stream_player->call("set_autoplay", true);
			p_audio_stream_player->call("play");
			Ref<SpeechToTextPlaybackStats> pstats = memnew(SpeechToTextPlaybackStats);
			pstats->playback_ring_buffer_length = playback_ring_buffer_length;
			pstats->buffer_frame_count = SpeechToTextProcessor::SPEECH_SETTING_BUFFER_FRAME_COUNT;
			Dictionary dict;
			dict["playback_last_skips"] = 0;
			dict["audio_stream_player"] = p_audio_stream_player;
			dict["jitter_buffer"] = Array();
			dict["sequence_id"] = -1;
			dict["last_update"] = OS::get_singleton()->get_ticks_msec();
			dict["packets_received_this_frame"] = 0;
			dict["excess_packets"] = 0;
			dict["playback_stats"] = pstats;
			dict["playback_start_time"] = 0;
			dict["playback_prev_time"] = -1;
			player_audio[p_player_id] = dict;
		} else {
			print_error(vformat("Attempted to duplicate player_audio entry (%s)!", p_player_id));
		}
	}
}

void SpeechToText::vc_debug_print(String p_str) const {
	if (!DEBUG) {
		return;
	}
	print_line(p_str);
}

void SpeechToText::vc_debug_printerr(String p_str) const {
	if (!DEBUG) {
		return;
	}
	print_error(p_str);
}

void SpeechToText::on_received_audio_packet(int p_peer_id, int p_sequence_id, PackedByteArray p_packet) {
	vc_debug_print(
			vformat("Received_audio_packet: peer_id: {%s} sequence_id: {%s}", itos(p_peer_id), itos(p_sequence_id)));
	if (!player_audio.has(p_peer_id)) {
		return;
	}
	Dictionary elem = player_audio[p_peer_id];
	// Detects if no audio packets have been received from this player yet.
	if (int64_t(elem["sequence_id"]) == -1) {
		elem["sequence_id"] = p_sequence_id - 1;
	}

	elem["packets_received_this_frame"] = int64_t(elem["packets_received_this_frame"]) + 1;
	packets_received_this_frame += 1;
	int64_t current_sequence_id = elem["sequence_id"];
	Array jitter_buffer = elem["jitter_buffer"];
	int64_t sequence_id_offset = p_sequence_id - current_sequence_id;
	if (sequence_id_offset > 0) {
		// For skipped buffers, add empty packets.
		int64_t skipped_packets = sequence_id_offset - 1;
		if (skipped_packets) {
			Variant fill_packets;
			// If using stretching, fill with last received packet.
			if (use_sample_stretching && jitter_buffer.size() > 0) {
				Dictionary new_jitter_buffer = jitter_buffer.back();
				fill_packets = new_jitter_buffer["packet"];
			}
			for (int32_t _i = 0; _i < skipped_packets; _i++) {
				Dictionary dict;
				dict["packet"] = fill_packets;
				dict["valid"] = false;
				jitter_buffer.push_back(dict);
			}
		}
		{
			// Add the new valid buffer.
			Dictionary dict;
			dict["packet"] = p_packet;
			dict["valid"] = true;
			jitter_buffer.push_back(dict);
		}
		int64_t excess_packet_count = jitter_buffer.size() - MAX_JITTER_BUFFER_SIZE;
		if (excess_packet_count > 0) {
			for (int32_t _i = 0; _i < excess_packet_count; _i++) {
				elem["excess_packets"] = (int64_t)elem["excess_packets"] + 1;
				jitter_buffer.pop_front();
			}
		}
		elem["sequence_id"] = int64_t(elem["sequence_id"]) + sequence_id_offset;
	} else {
		int64_t sequence_id = jitter_buffer.size() - 1 + sequence_id_offset;
		vc_debug_print(vformat("Updating existing sequence_id: %s", itos(sequence_id)));
		if (sequence_id >= 0) {
			// Update the existing buffer.
			if (use_sample_stretching) {
				int32_t jitter_buffer_size = jitter_buffer.size();
				for (int32_t i = sequence_id; i < jitter_buffer_size - 1; i++) {
					Dictionary buffer = jitter_buffer[i];
					if (buffer["valid"]) {
						break;
					}
					Dictionary dict;
					dict["packet"] = p_packet;
					dict["valid"] = false;
					jitter_buffer[i] = dict;
				}
			}
			Dictionary dict;
			dict["packet"] = p_packet;
			dict["valid"] = true;
			jitter_buffer[sequence_id] = dict;
		} else {
			vc_debug_printerr("Invalid repair sequence_id.");
		}
	}
	elem["jitter_buffer"] = jitter_buffer;
	player_audio[p_peer_id] = elem;
}

Dictionary SpeechToText::get_playback_stats(Dictionary speech_stat_dict) {
	Dictionary stat_dict = speech_stat_dict.duplicate(true);
	stat_dict["capture_get_percent"] = 0;
	stat_dict["capture_discard_percent"] = 0;
	if (double(stat_dict["capture_pushed_s"]) > 0) {
		stat_dict["capture_get_percent"] = 100.0 * double(stat_dict["capture_get_s"]) / double(stat_dict["capture_pushed_s"]);
		stat_dict["capture_discard_percent"] = 100.0 * double(stat_dict["capture_discarded_s"]) / double(stat_dict["capture_pushed_s"]);
	}

	Array keys = player_audio.keys();
	for (int32_t key_i = 0; key_i < keys.size(); key_i++) {
		Variant key = keys[key_i];
		Dictionary elem = player_audio[key];
		Ref<SpeechToTextPlaybackStats> playback_stats = elem["playback_stats"];
		if (playback_stats.is_null()) {
			continue;
		}
		Dictionary stats = playback_stats->get_playback_stats();
		stats["playback_total_time"] = (OS::get_singleton()->get_ticks_msec() - int64_t(elem["playback_start_time"])) / double(SpeechToTextProcessor::SPEECH_SETTING_MILLISECONDS_PER_SECOND);
		stats["excess_packets"] = elem["excess_packets"];
		stats["excess_s"] = int64_t(elem["excess_packets"]) * SpeechToTextProcessor::SPEECH_SETTING_PACKET_DELTA_TIME;
		stat_dict[key] = stats;
	}
	return stat_dict;
}

void SpeechToText::remove_player_audio(int p_player_id) {
	if (player_audio.has(p_player_id)) {
		if (player_audio.erase(p_player_id)) {
			return;
		}
	}
	print_error(vformat("Attempted to remove a non-existant player_audio entry (%s)", p_player_id));
}

void SpeechToText::clear_all_player_audio() {
	Array keys = player_audio.keys();
	for (int32_t i = 0; i < keys.size(); i++) {
		Variant key = keys[i];
		Variant element = player_audio[key];
		if (element.get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary elem = element;
		if (!elem.has("audio_stream_player")) {
			continue;
		}
		Dictionary dict = player_audio[key];
		Node *node = cast_to<Node>(dict["audio_stream_player"]);
		if (!node) {
			continue;
		}
		node->queue_free();
	}

	player_audio = Dictionary();
}

Dictionary SpeechToTextPlaybackStats::get_playback_stats() {
	double playback_pushed_frames = playback_pushed_calls * (buffer_frame_count * 1.0);
	double playback_discarded_frames = playback_discarded_calls * (buffer_frame_count * 1.0);
	Dictionary dict;
	dict["playback_ring_limit_s"] = playback_ring_buffer_length / double(SpeechToTextProcessor::SPEECH_SETTING_VOICE_PACKET_SAMPLE_RATE);
	dict["playback_ring_current_size_s"] = playback_ring_current_size / double(SpeechToTextProcessor::SPEECH_SETTING_VOICE_PACKET_SAMPLE_RATE);
	dict["playback_ring_max_size_s"] = playback_ring_max_size / double(SpeechToTextProcessor::SPEECH_SETTING_VOICE_PACKET_SAMPLE_RATE);
	dict["playback_ring_mean_size_s"] = 0;
	if (playback_push_buffer_calls > 0) {
		dict["playback_ring_mean_size_s"] = playback_ring_size_sum / playback_push_buffer_calls / double(SpeechToTextProcessor::SPEECH_SETTING_VOICE_PACKET_SAMPLE_RATE);
	} else {
		dict["playback_ring_mean_size_s"] = 0;
	}
	dict["jitter_buffer_current_size_s"] = float(jitter_buffer_current_size) * SpeechToTextProcessor::SPEECH_SETTING_PACKET_DELTA_TIME;
	dict["jitter_buffer_max_size_s"] = float(jitter_buffer_max_size) * SpeechToTextProcessor::SPEECH_SETTING_PACKET_DELTA_TIME;
	dict["jitter_buffer_mean_size_s"] = 0;
	if (jitter_buffer_calls > 0) {
		dict["jitter_buffer_mean_size_s"] = float(jitter_buffer_size_sum) / jitter_buffer_calls * SpeechToTextProcessor::SPEECH_SETTING_PACKET_DELTA_TIME;
	}
	dict["jitter_buffer_calls"] = jitter_buffer_calls;
	dict["playback_position_s"] = playback_position;
	dict["playback_get_percent"] = 0;
	dict["playback_discard_percent"] = 0;
	if (playback_pushed_frames > 0) {
		dict["playback_get_percent"] = 100.0 * playback_get_frames / playback_pushed_frames;
		dict["playback_discard_percent"] = 100.0 * playback_discarded_frames / playback_pushed_frames;
	}
	dict["playback_get_s"] = playback_get_frames / double(SpeechToTextProcessor::SPEECH_SETTING_VOICE_PACKET_SAMPLE_RATE);
	dict["playback_pushed_s"] = playback_pushed_frames / double(SpeechToTextProcessor::SPEECH_SETTING_VOICE_PACKET_SAMPLE_RATE);
	dict["playback_discarded_s"] = playback_discarded_frames / double(SpeechToTextProcessor::SPEECH_SETTING_VOICE_PACKET_SAMPLE_RATE);
	dict["playback_push_buffer_calls"] = floor(playback_push_buffer_calls);
	dict["playback_blank_s"] = playback_blank_push_calls * SpeechToTextProcessor::SPEECH_SETTING_PACKET_DELTA_TIME;
	dict["playback_blank_percent"] = 0;
	if (playback_push_buffer_calls > 0) {
		dict["playback_blank_percent"] = 100.0 * playback_blank_push_calls / playback_push_buffer_calls;
	}
	dict["playback_skips"] = floor(playback_skips);
	return dict;
}

void SpeechToTextPlaybackStats::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_playback_stats"),
			&SpeechToTextPlaybackStats::get_playback_stats);
}
