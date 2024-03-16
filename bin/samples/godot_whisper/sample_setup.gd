extends VBoxContainer

var bus_layout :AudioBusLayout = load("res://samples/godot_whisper/sample_bus_layout.tres")
@onready var audio_player: AudioStreamPlayer = $AudioPlayer
@onready var mic_player: AudioStreamPlayer = $MicPlayer
@onready var audio_to_text: CaptureStreamToText = $CaptureStreamToText
@onready var transcribe_label: RichTextLabel = $Panel/Label
# Called when the node enters the scene tree for the first time.
func _init():
	AudioServer.set_bus_layout(bus_layout)

func _ready():
	audio_player.bus = "Record"
	mic_player.bus = "Record"
	audio_to_text.transcribed_msg.connect(transcribe_label._on_capture_stream_to_text_transcribed_msg)
