extends Button

@onready var capture_stream_to_text: CaptureStreamToText = $"../../CaptureStreamToText"
@export var button_state := true

func _ready():
	toggled.connect(on_toggle)

func on_toggle(new_state):
	capture_stream_to_text.recording = new_state
