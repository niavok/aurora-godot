extends Control


var root_control: Control

func _ready():
	root_control = get_node("CanvasLayer/Control")

func _process(delta):
	root_control.visible = visible


func _on_QuitGameButton_pressed():
	get_parent().quit_game()


func _on_MainMenuButton_pressed():
	get_parent().quit_world()

func _on_SettingsButton_pressed():
	get_parent().open_settings_menu()

