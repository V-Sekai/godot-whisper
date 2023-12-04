extends RichTextLabel


func _on_speech_to_text_text_updated(p_text, new_text):
	text = p_text + "[color=orange]" + new_text + "[/color]"
