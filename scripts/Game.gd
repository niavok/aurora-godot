extends Node2D
class_name Game
var queue

const main_menu_path = "res://scenes/menus/MainMenu.tscn"
const settings_menu_path = "res://scenes/menus/SettingsMenu.tscn"
const world_path = "res://scenes/World.tscn"
const game_start_level = "TestHome"
#const game_start_level = "TestLevel"
const game_start_level_entry = "start_game"

var main_menu : Node
var settings_menu : Node
var loading_screen : Node
var pause_menu : Node
var world : Node
var is_in_settings_menu = false

enum GameState {
	GAME_STARTING,
	GAME_LOADING_MAIN_MENU,
	GAME_MAIN_MENU,
	#GAME_LOADING_SETTING_MENU,
	#GAME_SETTING_MENU,
	GAME_LOADING_NEW_GAME,
	GAME_IN_GAME,
	GAME_IN_PAUSE,
	}

var state = GameState.GAME_STARTING

func _ready():
	loading_screen = get_node("LoadingScreen")
	pause_menu = get_node("PauseMenu")

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	if(Input.is_action_just_pressed("ui_cancel")):
		back()

	while(state_machine()):
		pass

func state_machine() -> bool:

	if is_in_settings_menu:
		if not settings_menu and queue.is_ready(settings_menu_path):
			print("Game: Settings menu ready")
			settings_menu = queue.get_resource(settings_menu_path).instance()

			if settings_menu:
				print("Game: Settings menu loaded")
				add_child(settings_menu)
				loading_screen.visible = false
				print("Game: Settings menu added")
				return true
	else:
		match state:
			GameState.GAME_STARTING:
				queue = preload("res://scripts/resource_queue.gd").new()
				queue.start()
				queue.queue_resource(main_menu_path)
				state = GameState.GAME_LOADING_MAIN_MENU
				return true
			GameState.GAME_LOADING_MAIN_MENU:
				if not main_menu and queue.is_ready(main_menu_path):
					print("Game: Main menu ready")
					main_menu = queue.get_resource(main_menu_path).instance()
				if main_menu:
					print("Game: Main menu loaded")
					add_child(main_menu)
					loading_screen.visible = false
					print("Game: Main menu added")
					state = GameState.GAME_MAIN_MENU
					return true
			GameState.GAME_LOADING_NEW_GAME:
				if queue.is_ready(world_path):
					print("Game: World ready")
					world = queue.get_resource(world_path).instance()
					print("Game: Main menu loaded")
					add_child(world)
					move_child(loading_screen, get_child_count() -1)
					print("Game: Main menu added")
					state = GameState.GAME_IN_GAME
					get_tree().paused = false
					return true
	return false

func open_settings_menu() -> void:
	if not settings_menu:
		queue.queue_resource(settings_menu_path)
	#state = GameState.GAME_LOADING_SETTING_MENU
	loading_screen.activate(false)
	is_in_settings_menu = true
	#remove_child(main_menu)

func close_settings_menu() -> void:
	remove_child(settings_menu)
	settings_menu = null
	is_in_settings_menu = false
#	if not settings_menu:
#		queue.queue_resource(settings_menu_path)
#	state = GameState.GAME_LOADING_SETTING_MENU
#	loading_screen.activate(false)

#func open_main_menu() -> void:
#	state = GameState.GAME_LOADING_MAIN_MENU
#	loading_screen.activate(false)
#	remove_child(settings_menu)
#	settings_menu = null

func new_game() -> void:
	world = queue.queue_resource(world_path)
	state = GameState.GAME_LOADING_NEW_GAME
	GameGlobals.target_level = game_start_level
	GameGlobals.target_level_entry = game_start_level_entry
	GameGlobals.current_level = ""
	loading_screen.activate(false)
	remove_child(main_menu)

func back() -> void:
	if is_in_settings_menu:
		close_settings_menu()
	else:
		match state:
			GameState.GAME_IN_GAME:
				pause_game()
			GameState.GAME_IN_PAUSE:
				unpause_game()


func quit_world() -> void:
	state = GameState.GAME_LOADING_MAIN_MENU
	loading_screen.activate(false)
	remove_child(world)
	world = null
	pause_menu.visible = false


func pause_game() -> void:
	print("Pause game")
	get_tree().paused = true
	state = GameState.GAME_IN_PAUSE
	pause_menu.visible = true

func unpause_game() -> void:
	print("Unpause game")
	get_tree().paused = false
	state = GameState.GAME_IN_GAME
	pause_menu.visible = false

func quit_game():
	get_tree().quit()

