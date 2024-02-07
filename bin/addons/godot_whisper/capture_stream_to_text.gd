@tool
## Node that does transcribing of real time audio. It requires a bus with a [AudioEffectCapture] and a [WhisperResource] language model.
class_name CaptureStreamToText
extends StreamToText

## The interval at which transcribing is done.
@export var transcribe_interval := 1.0
## The record bus has to have a AudioEffectCapture at index specified by [member audio_effect_capture_index]
@export var record_bus := "Record"
## The index where the [AudioEffectCapture] is located at in the [member record_bus]
@export var audio_effect_capture_index := 0
## Download the model specified in the [member language_model_to_download]

@onready var _idx = AudioServer.get_bus_index(record_bus)
@onready var _effect_capture := AudioServer.get_bus_effect(_idx, audio_effect_capture_index) as AudioEffectCapture

func _ready():
	if Engine.is_editor_hint():
		return
	_add_timer()

func _add_timer():
	var timer_node = Timer.new()
	timer_node.one_shot = false
	timer_node.autostart = true
	timer_node.wait_time = transcribe_interval
	add_child(timer_node)
	timer_node.timeout.connect(_on_timer_timeout)

var _accumulated_frames: PackedVector2Array

func _on_timer_timeout():
	if Engine.is_editor_hint():
		return
	var start_time = Time.get_ticks_msec()
	var cur_frames_available = _effect_capture.get_frames_available()
	if cur_frames_available > 0:
		_accumulated_frames.append_array(_effect_capture.get_buffer(cur_frames_available))
		_effect_capture.clear_buffer()
	else:
		return
	var resampled = resample(_accumulated_frames, SpeechToText.SRC_SINC_FASTEST)
	var tokens = transcribe(resampled)
	var total_time = resampled.size() / SpeechToText.SPEECH_SETTING_SAMPLE_RATE
	if total_time > 15:
		_accumulated_frames.clear()
	#var no_activity = voice_activity_detection(resampled)
	#print(no_activity)
	var text : String
	for token in tokens:
		text += token["text"]
	print(text)
	print("Transcribe " + str((Time.get_ticks_msec() - start_time)/ 1000.0))

func _remove_special_characters(message: String, is_partial: bool):
	var special_characters = [ \
		{ "start": "[", "end": "]" }, \
		{ "start": "<", "end": ">" }]
	for special_character in special_characters:
		while(message.find(special_character["start"]) != -1):
			var begin_character := message.find(special_character["start"])
			var end_character := message.find(special_character["end"])
			if end_character != -1:
				message = message.substr(0, begin_character) + message.substr(end_character + 1)

	message = message.trim_suffix("{SPLIT}")
	
	var hallucinatory_character = [". you."]
	for special_character in hallucinatory_character:
		while(message.find(special_character) != -1):
			var begin_character := message.find(special_character)
			var end_character = begin_character + len(special_character)
			message = message.substr(0, begin_character) + message.substr(end_character + 1)
	return message
