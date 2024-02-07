@tool
## Node that handles downloading [WhisperResource] resources needed for [SpeechToText]
class_name StreamToText
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

## Download the model specified in the [member language_model_to_download]
@export var download := false:
	set(x):
		_do_download()
	get:
		return false
## What language model to download.
@export_enum("tiny.en", "tiny", "base.en", "base", "small.en", "small", "medium.en", "medium", "large-v1", "large-v2", "large-v3") var language_model_to_download = "tiny.en"
