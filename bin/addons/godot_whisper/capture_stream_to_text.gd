## Node that does transcribing of real time audio. It requires a bus with a [AudioEffectCapture] and a [WhisperResource] language model.
class_name CaptureStreamToText
extends SpeechToText

signal transcribed_msg(is_partial, new_text)

# For Traditional Chinese "以下是普通話的句子。"
# For Simplified Chinese "以下是普通话的句子。"
@export var initial_prompt: String

func _get_configuration_warnings():
	if language_model == null:
		return ["You need a language model."]
	else:
		return []
		
@export var recording := true:
	set(value):
		recording = value
		if recording:
			_ready()
		else:
			thread.wait_to_finish()
	get:
		return recording
## The interval at which transcribing is done. Use a value bigger than the time it takes to transcribe (eg. depends on model).
@export var transcribe_interval := 0.3
## Using dynamic audio context speeds up transcribing but may result in mistakes.
@export var use_dynamic_audio_context := true
## How much time has to pass in seconds until we can consider a sentence.
@export var minimum_sentence_time := 3
## Maximum time a sentence can have in seconds.
@export var maximum_sentence_time := 15
## How many tokens it's allowed to halucinate. Can provide useful info as it talks, but too much can provide useless text.
@export var halucinating_count := 1
## The record bus has to have a AudioEffectCapture at index specified by [member audio_effect_capture_index]
@export var record_bus := "Record"
## The index where the [AudioEffectCapture] is located at in the [member record_bus]
@export var audio_effect_capture_index := 0

## Character to consider when ending a sentence.
@export var punctuation_characters := ".!?;。；？！"

@onready var _idx = AudioServer.get_bus_index(record_bus)
@onready var _effect_capture := AudioServer.get_bus_effect(_idx, audio_effect_capture_index) as AudioEffectCapture

var thread : Thread

func _ready():
	if Engine.is_editor_hint():
		return
	if thread && thread.is_alive():
		recording = false
		thread.wait_to_finish()
	thread = Thread.new()
	_effect_capture.clear_buffer()
	thread.start(transcribe_thread)

var _accumulated_frames: PackedVector2Array

func transcribe_thread():
	var last_token_count := 0
	while recording:
		var start_time = Time.get_ticks_msec()
		_accumulated_frames.append_array(_effect_capture.get_buffer(_effect_capture.get_frames_available()))		
		var resampled = resample(_accumulated_frames, SpeechToText.SRC_SINC_FASTEST)
		if resampled.size() <= 0:
			OS.delay_msec(transcribe_interval * 1000)
			continue
		var no_activity := voice_activity_detection(resampled)
		#if no_activity:
			#print("no activity")
			#continue
		var total_time : float= (resampled.size() as float) / SpeechToText.SPEECH_SETTING_SAMPLE_RATE
		var audio_ctx : int = total_time * 1500 / 30 + 128
		if !use_dynamic_audio_context:
			audio_ctx = 0
		var tokens := transcribe(resampled, initial_prompt, audio_ctx)
		if tokens.is_empty():
			push_warning("No tokens generated")
			return
		var full_text : String = tokens.pop_front()
		var mix_rate : int = ProjectSettings.get_setting("audio/driver/mix_rate")
		var finish_sentence = false
		if total_time > maximum_sentence_time:
			finish_sentence = true
		var text : String
		for token in tokens:
			text += token["text"]
		text = _remove_special_characters(text)
		if _has_terminating_characters(text, punctuation_characters) || no_activity:
			finish_sentence = true
		if total_time < minimum_sentence_time || abs(tokens.size() - last_token_count) > halucinating_count:
			finish_sentence = false
		var time_processing = (Time.get_ticks_msec() - start_time)
		if no_activity:
			#_accumulated_frames = []
			continue
		if finish_sentence:
			_accumulated_frames = _accumulated_frames.slice(_accumulated_frames.size() - (0.2 * mix_rate))
		#if !no_activity:
		call_deferred("emit_signal", "transcribed_msg", finish_sentence, full_text)
		last_token_count = tokens.size()
		#print(text)
		print(full_text)
		print("Transcribe " + str(time_processing/ 1000.0) + " s")
		# Sleep remaining time
		var interval_sleep = transcribe_interval * 1000 - time_processing
		if interval_sleep > 0:
			OS.delay_msec(interval_sleep)

func _has_terminating_characters(message: String, characters: String):
	for character in characters:
		if message.contains(character):
			return true
	return false

func _remove_special_characters(message: String):
	var special_characters = [ \
		{ "start": "[", "end": "]" }, \
		{ "start": "<", "end": ">" }, \
		{ "start": "♪", "end": "♪" }]
	for special_character in special_characters:
		while(message.find(special_character["start"]) != -1):
			var begin_character := message.find(special_character["start"])
			var end_character := message.find(special_character["end"])
			if end_character != -1:
				message = message.substr(0, begin_character) + message.substr(end_character + 1)

	var hallucinatory_character = [". you."]
	for special_character in hallucinatory_character:
		while(message.find(special_character) != -1):
			var begin_character := message.find(special_character)
			var end_character = begin_character + len(special_character)
			message = message.substr(0, begin_character) + message.substr(end_character + 1)
	return message

func _notification(what):
	if what == NOTIFICATION_WM_CLOSE_REQUEST:
		recording = false
		if thread.is_alive():
			thread.wait_to_finish()
