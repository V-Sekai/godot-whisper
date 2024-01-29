extends RichTextLabel


func update_text():
	text = str(last_process_time) + "\n" + completed_text + "[color=green]" + partial_text + "[/color]"

func _process(_delta):
	update_text()

var completed_text = ""
var partial_text = ""
var last_process_time = 0.0

func _on_capture_stream_to_text_update_transcribed_msg(index, is_partial, new_text, process_time):
	last_process_time = process_time / 1000.0
	if is_partial == false:
		completed_text += new_text
		partial_text = ""
	else:
		if new_text!="":
			partial_text = new_text
