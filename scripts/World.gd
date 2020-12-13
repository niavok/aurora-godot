extends Node2D
class_name GameWorld


enum WorldState {
	WORLD_STARTING,
	WORLD_LOADING_LEVEL,
	WORLD_INIT_LEVEL,
	WORLD_IN_EXPLORATION,
	WORLD_PAUSE,
	}

var state = WorldState.WORLD_STARTING
var game : Game
var player
var current_level : Node2D = null
var current_level_key : String = ""

var target_level_key : String = ""
var target_level_entry_key : String = ""
var target_level_path : String = ""

func _ready():
	GameGlobals.world = self
	game = get_parent()
	player = get_node("PlayerLayer/Player")


func _process(delta):
	while(state_machine()):
		pass

func state_machine() -> bool:
	match state:
		WorldState.WORLD_STARTING:
			if GameGlobals.current_level == "":
				# In transition
				load_level(GameGlobals.target_level, GameGlobals.target_level_entry)
				return true
			else:
				assert(false)
		WorldState.WORLD_LOADING_LEVEL:
			if not current_level and game.queue.is_ready(target_level_path):
				print("World: Level ",target_level_key," ready")
				current_level = game.queue.get_resource(target_level_path).instance()
			if current_level:
				print("World: Level ",target_level_key, " loaded")
				add_child(current_level)
				current_level_key = target_level_key
				GameGlobals.current_level = current_level_key
				target_level_key = ""

				target_level_path = ""
				print("World: level ", current_level_key," added")
				# Player placement
				var level_entries : Node = current_level.get_node("LevelEntries")
				var level_entry_node : Node2D = null
				if level_entries:
					level_entry_node = level_entries.get_node(target_level_entry_key)
					if not level_entry_node:
						print("World: No level entry named ", target_level_entry_key, " found in level ", current_level_key)
						if level_entries.get_child_count():
							level_entry_node = level_entries.get_child(0)
				else:
					print("World: No LevelEntries found in level ", current_level_key)

				var initial_player_position = Vector2()
				if level_entry_node:
					initial_player_position = level_entry_node.position

				player.enter_level(initial_player_position)
				# TODO flip ?,  etc
				target_level_entry_key = ""
				state = WorldState.WORLD_INIT_LEVEL
				return false
		WorldState.WORLD_INIT_LEVEL:
			if not player.order_enter_level:
				game.loading_screen.fade_out(0.15, 0.2)
				state = WorldState.WORLD_IN_EXPLORATION
				return true

	return false

func load_level(level_key : String, level_entry_key : String) -> void:
	game.loading_screen.activate(true)
	target_level_key = level_key
	target_level_entry_key = level_entry_key
	target_level_path = "res://scenes/levels/"+target_level_key+".tscn"
	game.queue.queue_resource(target_level_path)
	if current_level:
		remove_child(current_level)
		current_level = null
	state = WorldState.WORLD_LOADING_LEVEL

func can_explore() -> bool:
	return state == WorldState.WORLD_IN_EXPLORATION
