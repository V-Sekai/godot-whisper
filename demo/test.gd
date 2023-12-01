extends Node2D

@onready var idx = AudioServer.get_bus_index("Record")
@onready var effect_capture := AudioServer.get_bus_effect(idx, 1) as AudioEffectCapture
@onready var speech_to_text : SpeechToText = $SpeechToText

func _ready():
	pass

func _process(_delta):
	var buffer = effect_capture.get_buffer(effect_capture.get_frames_available())
	print("text " + speech_to_text.transcribe(buffer))
	#print(effect_transcribe.is_transcribing_active())
