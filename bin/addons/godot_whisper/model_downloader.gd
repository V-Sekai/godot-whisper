@tool
extends Node

@export var option_button: OptionButton

# Called when the HTTP request is completed.
func _http_request_completed(result, response_code, headers, body, file_path):
	if result != HTTPRequest.RESULT_SUCCESS || response_code != 200:
		push_error("Can't downloaded.")
		return
	EditorInterface.get_resource_filesystem().scan()
	ResourceLoader.load(file_path, "WhisperResource", 2)
	print("Download successful. Check " + file_path)

func _on_button_pressed():
	var http_request = HTTPRequest.new()
	add_child(http_request)
	http_request.use_threads = true
	DirAccess.make_dir_recursive_absolute("res://addons/godot_whisper/models")
	var model = option_button.get_item_text(option_button.get_selected_id())
	var file_path : String = "res://addons/godot_whisper/models/gglm-" + model + ".bin"
	http_request.request_completed.connect(self._http_request_completed.bind(file_path))
	http_request.download_file = file_path
	var url : String = "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-" + model + ".bin?download=true"
	print("Downloading file from " + url)
	# Perform a GET request. The URL below returns JSON as of writing.
	var error = http_request.request(url)
	if error != OK:
		push_error("An error occurred in the HTTP request.")
