extends Node

const lobby_scene_const = preload("lobby.tscn")
const PACKET_TICK_TIMESLICE = 10
const MIC_BUS_NAME = "Mic"
const AEC_BUS_NAME = "Master"
var lobby_scene : Node = null
var debug_output : Label = null
@onready var godot_speech : Node = get_node("GodotSpeech")
var is_connected : bool = false
var audio_players : Dictionary
var audio_mutex : Mutex = Mutex.new()
const MAX_VOICE_BUFFERS = 16
var voice_buffers : Array = []
var audio_start_tick: int = 0
var voice_buffer_overrun_count: int = 0
var voice_id: int = 0
var voice_timeslice: int = 0
var voice_recording_started: bool = false

func _init() -> void:
	if godot_speech == null:
		return
	var nodes : Array = godot_speech.get_children()
	if nodes != null and nodes.size():
		for n in nodes:
			n.queue_free()


func _exit_tree() -> void:
	if godot_speech == null:
		return
	var nodes : Array = godot_speech.get_children()
	if nodes != null and nodes.size():
		for n in nodes:
			n.queue_free()


func get_voice_timeslice() -> int:
	return voice_timeslice


func reset_voice_timeslice() -> void:
	audio_start_tick = Time.get_ticks_msec()
	voice_timeslice = 0


func get_current_voice_id() -> int:
	return voice_id


func reset_voice_id() -> void:
	voice_id = 0


func started() -> void:
	if not godot_speech:
		return
	godot_speech.start_recording()
	voice_recording_started = true
	reset_voice_id()
	reset_voice_timeslice()


func ended() -> void:
	if not godot_speech:
		return
	godot_speech.end_recording()
	voice_recording_started = false


func host(p_player_name : String, p_port : int, p_server_only : bool) -> void:
	if network_layer.host_game(p_player_name, p_port, p_server_only):
		if lobby_scene:
			if network_layer.is_active_player():
				started()
			lobby_scene.refresh_lobby(network_layer.get_full_player_list())
		confirm_connection()


func confirm_connection() -> void:
	is_connected = true
	voice_id = 0


func _on_connection_success() -> void:
	if network_layer.is_active_player():
		started()
	if lobby_scene:
		lobby_scene.on_connection_success()
		lobby_scene.refresh_lobby(network_layer.get_full_player_list())
	confirm_connection()


func _on_connection_failed() -> void:
	if lobby_scene:
		lobby_scene.on_connection_failed()


func _player_list_changed() -> void:
	if lobby_scene:
		lobby_scene.refresh_lobby(network_layer.get_full_player_list())


func _on_game_ended() -> void:
	if network_layer.is_active_player():
		ended()

	if lobby_scene:
		lobby_scene.on_game_ended()


func _on_game_error(p_errtxt : String) -> void:
	if not lobby_scene:
		return
	lobby_scene.on_game_error(p_errtxt)


func _on_received_audio_packet(p_id : int, p_index : int, p_packet : PackedByteArray) -> void:
	if network_layer.is_active_player():
		godot_speech.on_received_audio_packet(p_id, p_index, p_packet)


func get_ticks_since_recording_started() -> int:
	return (Time.get_ticks_msec() - audio_start_tick)


func add_player_audio(p_id) -> void:
	var audio_stream_player : AudioStreamPlayer = AudioStreamPlayer.new()
	audio_players[p_id] = audio_stream_player
	audio_stream_player.set_name(str(p_id))
	add_child(audio_stream_player, true)
	audio_stream_player.owner = owner
	godot_speech.add_player_audio(p_id, audio_stream_player)

func remove_player_audio(p_id) -> void:
	if not godot_speech.get_property_list().has("voice_controller"):
		return
	godot_speech.voice_controller.remove_player_audio(p_id)
	var audio_stream_player : AudioStreamPlayer = audio_players[p_id]
	audio_stream_player.queue_free()
	audio_players.erase(p_id)

func setup_connections() -> void:
	if network_layer.connect("connection_failed", self._on_connection_failed) != OK:
		printerr("connection_failed could not be connected!")
	if network_layer.connect("connection_succeeded", self._on_connection_success) != OK:
		printerr("connection_succeeded could not be connected!")
	if network_layer.connect("player_list_changed", self._player_list_changed) != OK:
		printerr("player_list_changed could not be connected!")
	if network_layer.connect("game_ended", self._on_game_ended) != OK:
		printerr("game_ended could not be connected!")
	if network_layer.connect("game_error", self._on_game_error) != OK:
		printerr("game_error could not be connected!")
	if network_layer.connect("received_audio_packet", self._on_received_audio_packet) != OK:
		printerr("received_audio_packet could not be connected!")
	if network_layer.connect("peer_connected", self.add_player_audio):
		printerr("peer_connected could not be connected!")
	if network_layer.connect("peer_disconnected", self.remove_player_audio):
		printerr("peer_disconnected could not be connected!")
	if not lobby_scene:
		return
	if lobby_scene.connect("host_requested", self.host) != OK:
		printerr("audio_packet_processed could not be connected!")


func process_input_audio(_delta : float) -> void:
	if not godot_speech:
		return
	var copied_voice_buffers : Array = godot_speech.copy_and_clear_buffers()
	var current_skipped: int = godot_speech.get_skipped_audio_packets()
	godot_speech.clear_skipped_audio_packets()
	voice_id += current_skipped
	voice_timeslice = int(float(get_ticks_since_recording_started()) / PACKET_TICK_TIMESLICE)\
	- (copied_voice_buffers.size() + int(current_skipped))
	if copied_voice_buffers.size() > 0:
		for voice_buffer in copied_voice_buffers:
			voice_buffers.push_back(voice_buffer)
			if voice_buffers.size() > MAX_VOICE_BUFFERS:
				printerr("Voice buffer overrun!")
				voice_buffers.pop_front()
				voice_buffer_overrun_count += 1


# This function increments the internal voice_id.
# Make sure to get it before calling it.
func get_voice_buffers() -> Array:
	# Increment the internal voice id.
	voice_id += voice_buffers.size()

	var copied_voice_buffers : Array = voice_buffers
	voice_buffers = []
	return copied_voice_buffers


func _process(p_delta) -> void:
	if not voice_recording_started:
		return
	process_input_audio(p_delta)
	var index : int = get_current_voice_id()
	var buffers: Array = get_voice_buffers()
	for buffer in buffers:
		network_layer.send_audio_packet(index, buffer["byte_array"].slice(0, buffer["buffer_size"]))
		index += 1
	var speech_stat_dict : Dictionary = godot_speech.get_stats()
	var stat_dict : Dictionary = godot_speech.get_playback_stats(speech_stat_dict)
	var json : JSON = JSON.new()
	debug_output.set_text(json.stringify(stat_dict, "\t"))


func _ready() -> void:
	lobby_scene = lobby_scene_const.instantiate()
	add_child(lobby_scene, true)
	debug_output = lobby_scene.get_node("debug_output")
	debug_output.set_text("Ready.")
	var microphone_stream : AudioStreamPlayer = get_node("MicrophoneStreamAudio")
	godot_speech.set_audio_input_stream_player(microphone_stream)
	godot_speech.set_streaming_bus(MIC_BUS_NAME)
	godot_speech.set_error_cancellation_bus(AEC_BUS_NAME)
	setup_connections()
	set_process(true)
