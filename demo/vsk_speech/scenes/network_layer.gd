extends Node

const MAX_PEERS = 32

var blocking_sending_audio_packets : bool = false

var is_server_only : bool = false
var player_name : String = "Player"
var players : Dictionary

@export var debug_output_path : NodePath
var debug_output: Label = null

signal peer_connected(p_id)
signal peer_disconnected(p_id)
signal player_list_changed()
signal connection_failed()
signal connection_succeeded()
signal game_ended()
signal game_error(what)
signal received_audio_packet(p_id, p_index, p_packet)

static func bitand(a : int, b : int) -> int:
	var c : int = 0
	for x in range(64):
		c += c
		if (a < 0):
			if (b < 0):
				c += 1
		a += a
		b += b
	return c


static func encode_16_bit_value(p_value : int) -> PackedByteArray:
	return PackedByteArray([bitand(p_value, 0x000000ff), bitand(p_value, 0x0000ff00) >> 8])


static func decode_16_bit_value(p_buffer : PackedByteArray) -> int:
	var integer : int = 0
	integer = bitand(p_buffer[0], 0x000000ff) | bitand((p_buffer[1] << 8), 0x0000ff00)
	return integer


static func encode_24_bit_value(p_value : int) -> PackedByteArray:
	return PackedByteArray([bitand(p_value, 0x000000ff), bitand(p_value, 0x0000ff00) >> 8, bitand(p_value, 0x00ff0000) >> 16])


static func decode_24_bit_value(p_buffer : PackedByteArray) -> int:
	var integer : int = 0
	integer = bitand(p_buffer[0], 0x000000ff) | bitand((p_buffer[1] << 8), 0x0000ff00) | bitand((p_buffer[2] << 16), 0x00ff0000)
	return integer


func is_active_player() -> bool:
	if get_tree().get_multiplayer().is_server():
		if !is_server_only:
			return true
		else:
			return false
	else:
		return true


func _player_connected(p_id : int) -> void:
	print(str(p_id) + " connected!")
	emit_signal("peer_connected", p_id)


func _player_disconnected(p_id : int) -> void:
	if get_tree().get_multiplayer().is_server():
		unregister_player(p_id)
		for id in players:
			# Erase in the server.
			rpc_id(id, StringName("unregister_player"), p_id)
	emit_signal("peer_disconnected", p_id)


func _connected_ok() -> void:
	rpc(StringName("register_player"), get_tree().get_multiplayer().get_unique_id(), player_name)
	emit_signal("connection_succeeded")


# Callback from the SceneTree, only for the clients (not the server).
func _server_disconnected() -> void:
	emit_signal("game_error", "Server disconnected")
	end_game()


# Callback from the SceneTree, only for the clients (not the server).
func _connected_fail() -> void:
	get_tree().set_multiplayer(null) # Remove the peer.
	emit_signal("connection_failed")


func _network_peer_packet(p_id : int, packet : PackedByteArray) -> void:
	var result : Array = decode_voice_packet(packet)
	emit_signal("received_audio_packet", p_id, result[0], result[1])


# Functions to manage the lobby.
@rpc("any_peer")
func register_player(id : int, new_player_name : String) -> void:
	if get_tree().get_multiplayer().is_server():
		var remote_id: int = get_tree().get_multiplayer().get_remote_sender_id()
		if is_server_only == false:
			rpc_id(remote_id, StringName("register_player"), 1, player_name)
		for p_id in players:
			rpc_id(remote_id, StringName("register_player"), p_id, players[p_id])
			rpc_id(p_id, StringName("register_player"), remote_id, new_player_name)
	players[id] = new_player_name
	emit_signal("player_list_changed")


@rpc("any_peer")
func unregister_player(p_id : int) -> void:
	var remote_id: int = p_id
	if get_tree().get_multiplayer().is_server():
		remote_id = get_tree().get_multiplayer().get_remote_sender_id()
	if players.erase(remote_id):
		emit_signal("player_list_changed")
	else:
		printerr("unregister_player: invalid id " + str(remote_id))


func is_network_server() -> bool:
	return get_tree().get_multiplayer().is_server()


func host_game(new_player_name : String, port : int, p_is_server_only : bool) -> bool:
	player_name = new_player_name
	is_server_only = p_is_server_only
	var host : ENetMultiplayerPeer = ENetMultiplayerPeer.new()
	if host.create_server(port, MAX_PEERS) == OK:
		get_tree().get_multiplayer().multiplayer_peer = host
		host.host.compress(ENetConnection.COMPRESS_RANGE_CODER)
		return true
	return false


func join_game(ip : String, port : int, new_player_name : String) -> void:
	player_name = new_player_name
	var host : ENetMultiplayerPeer = ENetMultiplayerPeer.new()
	if host.create_client(ip, port) == OK:
		get_tree().get_multiplayer().multiplayer_peer = host
		host.host.compress(ENetConnection.COMPRESS_RANGE_CODER)


func get_player_list() -> Array:
	return players.values()


func get_player_ids() -> Array:
	return players.keys()


func get_player_name() -> String:
	return player_name


func end_game() -> void:
	emit_signal("game_ended")
	players.clear()
	get_tree().get_multiplayer().multiplayer_peer = null


func encode_voice_packet(p_index : int, p_voice_buffer : PackedByteArray) -> PackedByteArray:
	var encoded_index : PackedByteArray = encode_24_bit_value(p_index)
	var encoded_size : PackedByteArray = encode_16_bit_value(p_voice_buffer.size())
	var new_bytes : PackedByteArray
	new_bytes.append_array(encoded_index)
	new_bytes.append_array(encoded_size)
	new_bytes.append_array(p_voice_buffer)
	return new_bytes


func decode_voice_packet(p_voice_buffer : PackedByteArray) -> Array:
	var new_bytes : PackedByteArray = PackedByteArray()
	var encoded_id : int = -1
	if p_voice_buffer.size() > 5:
		var index : int = 0
		encoded_id = decode_24_bit_value(PackedByteArray([p_voice_buffer[index + 0], p_voice_buffer[index + 1], p_voice_buffer[index + 2]]))
		index += 3
		var encoded_size : int = decode_16_bit_value(PackedByteArray([p_voice_buffer[index + 0], p_voice_buffer[index + 1]]))
		index += 2
		new_bytes = p_voice_buffer.slice(index, index + (encoded_size))
	return [encoded_id, new_bytes]


func send_audio_packet(p_index : int, p_data : PackedByteArray) -> void:
	if not blocking_sending_audio_packets:
		var compressed_audio_packet : PackedByteArray = encode_voice_packet(p_index , p_data)
		var e = get_tree().get_multiplayer().send_bytes(compressed_audio_packet, 0, MultiplayerPeer.TRANSFER_MODE_UNRELIABLE, 1)
		if (e & 0xffffffff) != OK:
			printerr("send_audio_packet: send_bytes failed! %s" % e)


func get_full_player_list() -> Array:
	var player_list : Array = get_player_list()
	player_list.sort()
	if is_active_player():
		player_list.push_front(get_player_name() + " (You)")
	return player_list


func _input(p_event : InputEvent) -> void:
	if not p_event is InputEventKey:
		return
	if p_event.keycode != KEY_X:
		return
	if p_event.pressed:
		blocking_sending_audio_packets = true
	else:
		blocking_sending_audio_packets = false


func _ready() -> void:
	var connect_result : int = OK
	if get_tree().get_multiplayer().connect("peer_connected", self._player_connected) != OK:
		printerr("could not connect network_peer_connected!")
	if get_tree().get_multiplayer().connect("peer_disconnected", self._player_disconnected) != OK:
		printerr("could not connect network_peer_disconnected!")
	if get_tree().get_multiplayer().connect("connected_to_server", self._connected_ok) != OK:
		printerr("could not connect connected_to_server!")
	if get_tree().get_multiplayer().connect("connection_failed", self._connected_fail) != OK:
		printerr("could not connect connection_failed!")
	if get_tree().get_multiplayer().connect("server_disconnected", self._server_disconnected) != OK:
		printerr("could not connect server_disconnected!")
	connect_result = get_tree().get_multiplayer().connect("peer_packet", self._network_peer_packet)
	if connect_result == OK:
		return
	printerr("NetworkManager: network_peer_packet could not be connected!")


func _init() -> void:
	players = Dictionary()
