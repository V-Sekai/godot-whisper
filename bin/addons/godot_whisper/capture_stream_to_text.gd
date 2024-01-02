@tool
## Node that does transcribing of real time audio. It requires a bus with a [AudioEffectCapture] and a [WhisperResource] language model.
class_name CaptureStreamToText
extends Node
signal update_transcribed_msg(index: int, is_partial:bool, text: String)

func _get_configuration_warnings():
	if _speech_to_text_singleton.language_model == null:
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

var _last_index = 0

var _speech_to_text_singleton:
	get:
		return Engine.get_singleton("SpeechToText")

## What language you would want to transcribe from.
@export_enum("Auto","English","Chinese","German","Spanish","Russian","Korean","French","Japanese","Portuguese","Turkish","Polish","Catalan","Dutch","Arabic","Swedish","Italian","Indonesian","Hindi","Finnish","Vietnamese","Hebrew","Ukrainian","Greek","Malay","Czech","Romanian","Danish","Hungarian","Tamil","Norwegian","Thai","Urdu","Croatian","Bulgarian","Lithuanian","Latin","Maori","Malayalam","Welsh","Slovak","Telugu","Persian","Latvian","Bengali","Serbian","Azerbaijani","Slovenian","Kannada","Estonian","Macedonian","Breton","Basque","Icelandic","Armenian","Nepali","Mongolian","Bosnian","Kazakh","Albanian","Swahili","Galician","Marathi","Punjabi","Sinhala","Khmer","Shona","Yoruba","Somali","Afrikaans","Occitan","Georgian","Belarusian","Tajik","Sindhi","Gujarati","Amharic","Yiddish","Lao","Uzbek","Faroese","Haitian_Creole","Pashto","Turkmen","Nynorsk","Maltese","Sanskrit","Luxembourgish","Myanmar","Tibetan","Tagalog","Malagasy","Assamese","Tatar","Hawaiian","Lingala","Hausa","Bashkir","Javanese","Sundanese","Cantonese") var language:int = 1:
	get:
		return _speech_to_text_singleton.get_language()
	set(val):
		_speech_to_text_singleton.set_language(val)

## The language model downloaded.
@export var language_model: WhisperResource:
	get:
		return _speech_to_text_singleton.get_language_model()
	set(val):
		_speech_to_text_singleton.set_language_model(val)

## If we should use gpu for transcribing.
@export var use_gpu := true :
	get:
		return _speech_to_text_singleton.is_use_gpu()
	set(val):
		_speech_to_text_singleton.set_use_gpu(val)

func _ready():
	if Engine.is_editor_hint():
		return
	_add_timer()
	_speech_to_text_singleton.connect("update_transcribed_msgs", self._update_transcribed_msgs_func)

func _add_timer():
	var timer_node = Timer.new()
	timer_node.one_shot = false
	timer_node.autostart = true
	timer_node.wait_time = 1
	add_child(timer_node)
	timer_node.connect("timeout",self._on_timer_timeout)

func _on_timer_timeout():
	if Engine.is_editor_hint():
		return
	var buffer: PackedVector2Array = _effect_capture.get_buffer(_effect_capture.get_frames_available())
	if is_running:
		_speech_to_text_singleton.add_audio_buffer(buffer)

func _update_transcribed_msgs_func(transcribed_msgs):
	for transcribed_msg  in transcribed_msgs:
		var cur_text = transcribed_msg["text"]
		var token_index = cur_text.rfind("]")
		if token_index!=-1:
			cur_text = cur_text.substr(token_index+1)
		
		token_index = cur_text.find("<")
		if token_index!=-1:
			cur_text = cur_text.substr(0,token_index)
		
		if transcribed_msg["is_partial"]==false:
			if cur_text.ends_with("?") or cur_text.ends_with(",") or cur_text.ends_with("."):
				pass
			else:
				cur_text = cur_text + "."
		emit_signal("update_transcribed_msg", _last_index, transcribed_msg["is_partial"], cur_text)
		if transcribed_msg["is_partial"]==false:
			_last_index+=1

## Is the transcribing thread running? Start it with [method start_listen]
var is_running = false

## Starts listening to audio and transcribing.
func start_listen():
	_speech_to_text_singleton.start_listen()
	is_running = true
	
## Stop listening to audio and transcribing.
func stop_listen():
	is_running = false
	_speech_to_text_singleton.stop_listen()
	

