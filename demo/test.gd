extends Node2D

@export var interval := 5.0
@export var text := ""
@onready var idx = AudioServer.get_bus_index("Record")
@onready var effect_capture := AudioServer.get_bus_effect(idx, 0) as AudioEffectCapture
@onready var speech_to_text : SpeechToText = $SpeechToText
var buffer_full : PackedVector2Array

func _ready():
	speech_to_text.duration_ms = int(interval * 1000)

func _process(_delta):
	var buffer: PackedVector2Array = effect_capture.get_buffer(effect_capture.get_frames_available())
	buffer_full.append_array(buffer)
	var mix_rate : int = ProjectSettings.get_setting("audio/driver/mix_rate")
	var total_len : int = mix_rate * interval
	var keep_len : int = mix_rate * 0.2
	if buffer_full.size() > total_len:
		buffer_full = buffer_full.slice(buffer_full.size() - total_len)
		text += speech_to_text.transcribe(buffer_full) + " "
		print(text)
		buffer_full = buffer_full.slice(buffer_full.size() - keep_len, buffer_full.size())

