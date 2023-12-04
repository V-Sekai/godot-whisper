extends RichTextLabel


func _on_speech_to_text_text_updated(total_time, p_text, new_text):
	text = str(total_time) + ":" + p_text + "[color=orange]" + new_text + "[/color]"
