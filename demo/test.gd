extends AudioStreamPlayer

func _process(delta):
	var idx = AudioServer.get_bus_index("Record")
	var effect_capture := AudioServer.get_bus_effect(idx, 1) as AudioEffectCapture
	var frames = effect_capture.get_frames_available()
	var buffer := effect_capture.get_buffer(frames)
	print(buffer)
