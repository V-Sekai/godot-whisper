extends Control

signal host_requested(p_player_name, p_port, p_server_only)
@export_file var const_error_dialog_scene : String = "res://vsk_speech/scenes/error_dialog.tscn"
var error_dialog : Node = load(const_error_dialog_scene).instantiate()

func _on_host_pressed() -> void:
	if get_node("connect/name").text == "":
		get_node("connect/error_label").text = "Invalid name!"
		return
	get_node("connect").hide()
	get_node("players").show()
	get_node("connect/error_label").text = ""
	var player_name : String = get_node("connect/name").text
	var port : int = get_node("connect/port").value
	emit_signal("host_requested", player_name, port, true)


func _on_join_pressed() -> void:
	if get_node("connect/name").text == "":
		get_node("connect/error_label").text = "Invalid name!"
		return
	var port : int = get_node("connect/port").value
	var ip : String = get_node("connect/ip").text
	if not ip.is_valid_ip_address():
		get_node("connect/error_label").text = "Invalid IPv4 address!"
		return
	get_node("connect/error_label").text=""
	get_node("connect/host").disabled = true
	get_node("connect/join").disabled = true
	var player_name : String = get_node("connect/name").text
	network_layer.join_game(ip, port, player_name)


func on_connection_success() -> void:
	get_node("connect").hide()
	get_node("players").show()


func on_connection_failed() -> void:
	get_node("connect/host").disabled = false
	get_node("connect/join").disabled = false
	get_node("connect/error_label").set_text("Connection failed.")


func on_game_ended() -> void:
	show()
	get_node("connect").show()
	get_node("players").hide()
	get_node("connect/host").disabled = false


func on_game_error(p_errtxt : String) -> void:
	if error_dialog.get_parent() == null:
		error_dialog.set_name("error")
		add_child(error_dialog, true)
	get_node("error").dialog_text = p_errtxt
	get_node("error").popup_centered()


func refresh_lobby(p_player_names : Array) -> void:
	get_node("players/list").clear()

	for p in p_player_names:
		get_node("players/list").add_item(p)
