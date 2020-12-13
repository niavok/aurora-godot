extends Control


# Declare member variables here. Examples:
# var a = 2
# var b = "text"


# Called when the node enters the scene tree for the first time.
func _ready():
	pass # Replace with function body.

func _on_QuitGameButton_pressed():
	get_parent().quit_game()

func _on_SettingsButton_pressed():
	get_parent().open_settings_menu()


func _on_NewGameButton_pressed():
	get_parent().new_game()
