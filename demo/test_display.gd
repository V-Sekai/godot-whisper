extends RichTextLabel


func _on_audio_stream_to_text_text_updated(p_text, p_text_new):
	text = p_text + " -> " + p_text_new
