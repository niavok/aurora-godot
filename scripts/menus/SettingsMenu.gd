extends Control

func _ready():
	var full_screen_button : CheckButton = get_node("CanvasLayer/ScrollContainer/VBoxContainer/FullScreenButton")
	var continue_on_start_button : CheckButton = get_node("CanvasLayer/ScrollContainer/VBoxContainer/ContinueOnStartButton")

	full_screen_button.pressed = Settings.get_display_window_fullscreen()
	continue_on_start_button.pressed = Settings.get_game_continue_on_start()

	update_preferred_screen_buttons()


func _on_BackButton_pressed():
	Settings.save_settings_if_necessary()
	#get_parent().open_main_menu()
	get_parent().close_settings_menu()

func _on_FullScreenButton_toggled(button_pressed):
	Settings.configure_display_window_fullscreen(button_pressed)

func _on_ContinueOnStartButton_toggled(button_pressed):
	Settings.configure_game_continue_on_start(button_pressed)

func update_preferred_screen_buttons() -> void :
	var preferred_screen : int = Settings.get_display_preferred_screen()
	var screen_count : int = OS.get_screen_count()
	update_preferred_screen_button(0, preferred_screen, screen_count, get_node("CanvasLayer/ScrollContainer/VBoxContainer/HBoxContainer/DisplayScreenButton1"))
	update_preferred_screen_button(1, preferred_screen, screen_count, get_node("CanvasLayer/ScrollContainer/VBoxContainer/HBoxContainer/DisplayScreenButton2"))
	update_preferred_screen_button(2, preferred_screen, screen_count, get_node("CanvasLayer/ScrollContainer/VBoxContainer/HBoxContainer/DisplayScreenButton3"))
	update_preferred_screen_button(3, preferred_screen, screen_count, get_node("CanvasLayer/ScrollContainer/VBoxContainer/HBoxContainer/DisplayScreenButton4"))


func update_preferred_screen_button(button_index : int, preferred_screen : int, screen_count, button : Button) -> void:
	button.visible = (button_index < screen_count)
	button.pressed = (button_index == preferred_screen)

func _on_DisplayScreenButton_toggled(button_pressed, button_index):
	if button_pressed:
		Settings.configure_display_preferred_screen(button_index)
		update_preferred_screen_buttons()
