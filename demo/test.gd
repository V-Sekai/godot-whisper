extends Node2D

@onready var idx = AudioServer.get_bus_index("Record")
@onready var effect_capture := AudioServer.get_bus_effect(idx, 0) as AudioEffectCapture
@onready var speech_to_text : SpeechToText = $SpeechToText
@onready var audio_stream_player :AudioStreamPlayer
var buffer_full : PackedVector2Array

func _process(_delta):
	var buffer: PackedVector2Array = effect_capture.get_buffer(effect_capture.get_frames_available())
	buffer_full.append_array(buffer)
	#buffer_full.resize(speech_to_text.step_ms)
	#print(effect_transcribe.is_transcribing_active())

func ellapsed(time: float):
	return Time.get_unix_time_from_system() - time

func transcribe():
	var start = Time.get_unix_time_from_system()
	buffer_full.resize(ProjectSettings.get_setting("audio/driver/mix_rate") * effect_capture.buffer_length)
	print(ellapsed(start))
	print("text " + speech_to_text.transcribe(buffer_full))
	print(ellapsed(start))
	buffer_full.clear()
