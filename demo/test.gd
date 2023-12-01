extends Node2D

@onready var idx = AudioServer.get_bus_index("Record")
@onready var effect_capture := AudioServer.get_bus_effect(idx, 0) as AudioEffectCapture
@onready var speech_to_text : SpeechToText = $SpeechToText
@onready var audio_stream_player :AudioStreamPlayer

func _process(_delta):
	var buffer: PackedVector2Array = effect_capture.get_buffer(effect_capture.get_frames_available())
	print("text " + speech_to_text.transcribe(buffer))
	#print(effect_transcribe.is_transcribing_active())
