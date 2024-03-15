@tool
## Node used to quick test single audio stream to text.
class_name AudioStreamToText
extends SpeechToText

# For Traditional Chinese "以下是普通話的句子。"
# For Simplified Chinese "以下是普通话的句子。"
@export var initial_prompt: String

func _get_configuration_warnings():
	if language_model == null:
		return ["You need a language model."]
	else:
		return []
		
@export var audio_stream: AudioStreamWAV :
	set(value):
		audio_stream = value
		text = get_text()
	get:
		return audio_stream
@export var text: String

## Download the model specified in the [member language_model_to_download]
@export var start_transcribe := false:
	set(x):
		text = get_text()
	get:
		return false


func get_text():
	var start_time := Time.get_ticks_msec()
	var data := audio_stream.data
	var data_float : PackedFloat32Array
	match audio_stream.format:
		AudioStreamWAV.FORMAT_8_BITS:
			for i in data.size() / 2:
				data_float.append((data.decode_s8(i * 2) * 1.0/128.0))
		AudioStreamWAV.FORMAT_16_BITS:
			for i in data.size() / 2:
				data_float.append((data.decode_s16(i * 2) / 32768.0))
	var tokens = transcribe(data_float, initial_prompt, 0)
	if tokens.is_empty():
		return ""
	var full_text : String = tokens.pop_front()
	var text := ""
	for token in tokens:
		if token["plog"] > 0:
			continue
		text += token["text"]
	text = full_text
	print("Transcribe: " + str((Time.get_ticks_msec() - start_time)/ 1000.0))
	print(text)
	return _remove_special_characters(text)

func _remove_special_characters(message: String):
	var special_characters := [ \
		{ "start": "[", "end": "]" }, \
		{ "start": "<", "end": ">" }]
	for special_character in special_characters:
		while(message.find(special_character["start"]) != -1):
			var begin_character := message.find(special_character["start"])
			var end_character := message.find(special_character["end"])
			if end_character != -1:
				message = message.substr(0, begin_character) + message.substr(end_character + 1)

	var hallucinatory_character := [". you."]
	for special_character in hallucinatory_character:
		while(message.find(special_character) != -1):
			var begin_character := message.find(special_character)
			var end_character := begin_character + len(special_character)
			message = message.substr(0, begin_character) + message.substr(end_character + 1)
	return message
