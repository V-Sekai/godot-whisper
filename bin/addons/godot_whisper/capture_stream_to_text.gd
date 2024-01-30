@tool
## Node that does transcribing of real time audio. It requires a bus with a [AudioEffectCapture] and a [WhisperResource] language model.
class_name CaptureStreamToText
extends SpeechToText

func _get_configuration_warnings():
	if language_model == null:
		return ["You need a language model."]
	else:
		return []

func _do_download():
	var http_request = HTTPRequest.new()
	add_child(http_request)
	http_request.use_threads = true
	DirAccess.make_dir_recursive_absolute("res://addons/godot_whisper/models")
	var file_path = "res://addons/godot_whisper/models/gglm-" + language_model_to_download + ".bin"
	http_request.request_completed.connect(self._http_request_completed.bind(file_path))
	http_request.download_file = file_path
	var url = "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-" + language_model_to_download + ".bin?download=true"
	print("Downloading file from " + url)
	# Perform a GET request. The URL below returns JSON as of writing.
	var error = http_request.request(url)
	if error != OK:
		push_error("An error occurred in the HTTP request.")

# Called when the HTTP request is completed.
func _http_request_completed(result, response_code, headers, body, file_path):
	if result != HTTPRequest.RESULT_SUCCESS:
		push_error("Can't downloaded.")
		return
	ResourceLoader.load(file_path, "WhisperResource", 2)
	print("Download successful. Check " + file_path + ". If file is not there, alt tab or restart editor.")

## The record bus has to have a AudioEffectCapture at index specified by [member audio_effect_capture_index]
@export var record_bus := "Record"
## The index where the [AudioEffectCapture] is located at in the [member record_bus]
@export var audio_effect_capture_index := 0
## Download the model specified in the [member language_model_to_download]
@export var download := false:
	set(x):
		_do_download()
	get:
		return false
## What language model to download.
@export_enum("tiny.en", "tiny", "base.en", "base", "small.en", "small", "medium.en", "medium", "large-v1", "large-v2", "large-v3") var language_model_to_download = "tiny.en"

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
	timer_node.wait_time = 0.5
	add_child(timer_node)
	timer_node.connect("timeout",self._on_timer_timeout)

var _accumulated_frames: PackedVector2Array

func _on_timer_timeout():
	if Engine.is_editor_hint():
		return
	var cur_frames_available = _effect_capture.get_frames_available()
	if cur_frames_available > 0:
		_accumulated_frames.append_array(_effect_capture.get_buffer(cur_frames_available))
	else:
		return
	print(_accumulated_frames.size() / AudioServer.get_mix_rate())
	var start_time = Time.get_ticks_msec()
	var resampled = resample(_accumulated_frames)
	print("Resampled " + str((Time.get_ticks_msec() - start_time)/ 1000.0))
	start_time = Time.get_ticks_msec()
	var tokens = transcribe(resampled)
	var text : String
	for token in tokens:
		text += token["text"]
	print(text)
	print("Transcribe " + str((Time.get_ticks_msec() - start_time)/ 1000.0))
	#_effect_capture.clear_buffer()

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
