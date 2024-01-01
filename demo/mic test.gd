@tool
extends VBoxContainer
signal update_transcribed_msg(index: int, is_partial:bool, text: String)

func _get_configuration_warnings():
	if speech_to_text_singleton.language_model == null:
		return ["You need a language model."]
	else:
		return []

func do_download():
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

@export var download := false:
	set(x):
		do_download()
		pass
	get:
		return false
@export_enum("tiny.en", "tiny", "base.en", "base", "small.en", "small", "medium.en", "medium", "large-v1", "large-v2", "large-v3") var language_model_to_download = "tiny.en"
@onready var idx = AudioServer.get_bus_index("Record")
@onready var effect_capture := AudioServer.get_bus_effect(idx, 0) as AudioEffectCapture
var buffer_full : PackedVector2Array
var url := "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.bin?download=true"

var last_index = 0

var speech_to_text_singleton:
	get:
		return Engine.get_singleton("SpeechToText")

@export_enum("Auto","English","Chinese","German","Spanish","Russian","Korean","French","Japanese","Portuguese","Turkish","Polish","Catalan","Dutch","Arabic","Swedish","Italian","Indonesian","Hindi","Finnish","Vietnamese","Hebrew","Ukrainian","Greek","Malay","Czech","Romanian","Danish","Hungarian","Tamil","Norwegian","Thai","Urdu","Croatian","Bulgarian","Lithuanian","Latin","Maori","Malayalam","Welsh","Slovak","Telugu","Persian","Latvian","Bengali","Serbian","Azerbaijani","Slovenian","Kannada","Estonian","Macedonian","Breton","Basque","Icelandic","Armenian","Nepali","Mongolian","Bosnian","Kazakh","Albanian","Swahili","Galician","Marathi","Punjabi","Sinhala","Khmer","Shona","Yoruba","Somali","Afrikaans","Occitan","Georgian","Belarusian","Tajik","Sindhi","Gujarati","Amharic","Yiddish","Lao","Uzbek","Faroese","Haitian_Creole","Pashto","Turkmen","Nynorsk","Maltese","Sanskrit","Luxembourgish","Myanmar","Tibetan","Tagalog","Malagasy","Assamese","Tatar","Hawaiian","Lingala","Hausa","Bashkir","Javanese","Sundanese","Cantonese") var language:int = 1:
	get:
		return speech_to_text_singleton.get_language()
	set(val):
		speech_to_text_singleton.set_language(val)

@export var language_model: WhisperResource:
	get:
		return speech_to_text_singleton.get_language_model()
	set(val):
		speech_to_text_singleton.set_language_model(val)
		
@export var use_gpu: bool:
	get:
		return speech_to_text_singleton.is_use_gpu()
	set(val):
		speech_to_text_singleton.set_use_gpu(val)

func _ready():
	if Engine.is_editor_hint():
		return
	speech_to_text_singleton.connect("update_transcribed_msgs", self.update_transcribed_msgs_func)

func _on_timer_timeout():
	if Engine.is_editor_hint():
		return
	var buffer: PackedVector2Array = effect_capture.get_buffer(effect_capture.get_frames_available())
	if is_running:
		speech_to_text_singleton.add_audio_buffer(buffer)

func update_transcribed_msgs_func(transcribed_msgs):
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
		emit_signal("update_transcribed_msg", last_index, transcribed_msg["is_partial"], cur_text)
		if transcribed_msg["is_partial"]==false:
			last_index+=1


var is_running = false
func _on_start_button_pressed():
	speech_to_text_singleton.start_listen()
	is_running = true

func _on_stop_button_pressed():
	is_running = false
	speech_to_text_singleton.stop_listen()
	

