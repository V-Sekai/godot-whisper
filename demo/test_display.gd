extends RichTextLabel


func _on_speech_to_text_text_updated(total_time, p_text, new_text):
	transcribed = "Time to process: " + str(total_time) + "s\n" + p_text + "[color=orange]" + new_text + "[/color]"

var transcribed := "..."

func update_text():
	text = "Time: " + str(Time.get_ticks_msec() / 1000) + "s\n" + transcribed

func _process(delta):
	update_text()
