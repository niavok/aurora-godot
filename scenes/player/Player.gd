extends KinematicBody2D
class_name Player

#var velocity := Vector2()

var status : PlayerStatus
var animation : PlayerAnimation
var render : PlayerRender
var movement : PlayerMovement

var exit_detector : Area2D
var order_enter_level_position : Vector2
var order_enter_level = false

func _ready():
	status = get_node("PlayerStatus")
	animation = get_node("PlayerAnimation")
	render = get_node("PlayerRender")
	movement = get_node("PlayerMovement")

	exit_detector = get_node("ExitDetectorArea2D2")

func _process(delta):
	var is_on_exit = exit_detector.get_overlapping_areas().size() > 0
	if is_on_exit:
		var level_exit : LevelExit = exit_detector.get_overlapping_areas()[0]
		level_exit.ping()
		if Input.is_action_just_pressed("player_use"):
			GameGlobals.world.load_level(level_exit.target_level_key, level_exit.target_level_entry_key)

func _physics_process(delta):
	if order_enter_level: # thread safe ?
		apply_enter_level()

	movement.update(delta)
	animation.update(delta)
	render.update()


func enter_level(entry_position : Vector2) -> void:
	order_enter_level_position = entry_position
	order_enter_level = true

func apply_enter_level() -> void:
	order_enter_level = false

	movement.enter_level(order_enter_level_position)



