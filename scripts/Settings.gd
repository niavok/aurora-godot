extends Node

const settings_path = "user://config/settings.json"
const settings_dir_path = "user://config"

const section_key_game = "game"
const section_key_display = "display"

# Game keys
const setting_key_continue_on_start = "continue_on_start"


# Display keys
const setting_key_window_fullscren = "window_fullscren"
const setting_key_preferred_screen = "preferred_screen"


var has_pending_change = true


# Global settings
var continue_on_start = false

var settings_dict = {}

# Called when the node enters the scene tree for the first time.
func _ready():
	load_settings()

func load_settings() -> void:
	var settings_file = File.new()
	var err = settings_file.open(settings_path, settings_file.READ)
	if err == OK:
		var settings_text = settings_file.get_as_text()
		settings_file.close()
		var parsed_dict = parse_json(settings_text)
		if typeof(parsed_dict) == TYPE_DICTIONARY:
			settings_dict = parsed_dict
			apply_settings(settings_dict)
		else:
			print("Error: invalid config file format")

func save_settings_if_necessary() -> void:
	if has_pending_change:
		save_settings()

func save_settings() -> void:
	var settings_dir = Directory.new()
	settings_dir.make_dir_recursive(settings_dir_path)

	has_pending_change = false
	var settings_text = JSON.print(settings_dict, "\t", true)
	var settings_file = File.new()
	settings_file.open(settings_path, settings_file.WRITE)
	settings_file.store_string(settings_text)
	settings_file.close()

func configure_setting(section_key: String, key: String, value) -> void:
	if not section_key in settings_dict:
		settings_dict[section_key] = {}
	var section : Dictionary = settings_dict[section_key]
	section[key] = value
	apply_settings({ section_key : {key : value}})
	has_pending_change = true

func apply_settings(settings_dict: Dictionary) -> void:

	# Game section
	if section_key_game in settings_dict:
		var section : Dictionary = settings_dict[section_key_game]

		if setting_key_continue_on_start in section:
			continue_on_start = section[setting_key_continue_on_start]

	# Display section
	if section_key_display in settings_dict:
		var section : Dictionary = settings_dict[section_key_display]

		if setting_key_preferred_screen in section:
			var preferred_screen : int = section[setting_key_preferred_screen]
			print("set_current_screen: ", preferred_screen)
			if preferred_screen != OS.current_screen:
				var window_fullscreen = OS.window_fullscreen
				if window_fullscreen:
					OS.window_fullscreen = false
				OS.set_current_screen( preferred_screen)
				if window_fullscreen:
					OS.window_fullscreen = true
		if setting_key_window_fullscren in section:
			OS.window_fullscreen = section[setting_key_window_fullscren]

func configure_display_window_fullscreen(value: bool) -> void:
	configure_setting(section_key_display, setting_key_window_fullscren, value)

func configure_display_preferred_screen(value: int) -> void:
	configure_setting(section_key_display, setting_key_preferred_screen, value)

func configure_game_continue_on_start(value: bool) -> void:
	configure_setting(section_key_game, setting_key_continue_on_start, value)

func get_display_window_fullscreen() -> bool:
	return OS.window_fullscreen

func get_display_preferred_screen() -> int:
	return OS.current_screen

func get_game_continue_on_start() -> bool:
	return continue_on_start
