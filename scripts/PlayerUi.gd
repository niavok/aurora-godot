extends Control

export(NodePath) var player_node_path
var player: Player

var stamina_progress_bar

# Called when the node enters the scene tree for the first time.
func _ready():
	player = get_node(player_node_path)
	stamina_progress_bar = get_node("StaminaProgressBar")


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	stamina_progress_bar.value = player.movement.stamina
	stamina_progress_bar.max_value = player.movement.MAX_STAMINA
