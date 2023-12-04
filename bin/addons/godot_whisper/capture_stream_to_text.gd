@tool
class_name CaptureStreamToText
extends SpeechToText

signal text_updated(total_time: float, text: String, new_text: String)

func _get_configuration_warnings():
	if language_model == null:
		return ["You need a language model."]
	else:
		return []

func do_download():
	var http_request = HTTPRequest.new()
	add_child(http_request)
	http_request.use_threads = true
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

@export var download := false:
	set(x):
		do_download()
		pass
	get:
		return false
@export_enum("tiny.en", "tiny", "base.en", "base", "small.en", "small", "medium.en", "medium", "large-v1", "large-v2", "large-v3") var language_model_to_download = "tiny.en"
@export var keep_interval := 0.1
@export var time_taken := 0.0
@export var text := ""
@export var new_text := ""
@export var max_tokens := 30
@export var tokens: Array
@onready var idx = AudioServer.get_bus_index("Record")
@onready var effect_capture := AudioServer.get_bus_effect(idx, 0) as AudioEffectCapture
var buffer_full : PackedVector2Array
var url := "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.bin?download=true"

func _ready():
	if Engine.is_editor_hint():
		return
	if effect_capture.buffer_length < audio_duration:
		push_warning("buffer_length smaller than duration_ms.")
		audio_duration = effect_capture.buffer_length


func tokens_to_text(speech_tokens):
	var computed_text = ""
	for token in speech_tokens:
		computed_text += token["text"]
	return computed_text

func _process(_delta):
	if Engine.is_editor_hint():
		return
	var buffer: PackedVector2Array = effect_capture.get_buffer(effect_capture.get_frames_available())
	buffer_full.append_array(buffer)
	var mix_rate : int = ProjectSettings.get_setting("audio/driver/mix_rate")
	var total_len := int(mix_rate * audio_duration)
	var keep_len := int(mix_rate * keep_interval)
	if buffer_full.size() > total_len:
		var buffer_copy = buffer_full.duplicate()
		buffer_full = buffer_full.slice(buffer_full.size() - keep_len, buffer_full.size())
		buffer_copy = buffer_copy.slice(buffer_copy.size() - total_len, buffer_copy.size())
		
		var start_time := Time.get_ticks_msec()
		var new_tokens : Array= transcribe(buffer_copy)
		time_taken = (Time.get_ticks_msec() - start_time)* 0.001
		new_tokens = new_tokens.filter(func (token): return !("[" in token["text"]) && !("<" in token["text"]))
		text = tokens_to_text(tokens)
		new_text = tokens_to_text(new_tokens)
		tokens.append_array(new_tokens)
		text_updated.emit(time_taken, text, new_text)

