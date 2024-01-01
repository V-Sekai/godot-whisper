extends RichTextLabel


func update_text():
	text = completed_text + "[color=green]" + partial_text + "[/color]"

func _process(delta):
	update_text()

var completed_text = ""
var partial_text = ""

func _on_capture_stream_to_text_update_transcribed_msg(index, is_partial, new_text):
	if is_partial == false:
		completed_text += new_text
		partial_text = ""
	else:
		if new_text!="":
			partial_text = new_text
