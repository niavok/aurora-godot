extends Area2D
class_name LevelExit
export (String) var target_level_key = ""

export (String) var target_level_entry_key = ""

var display_time : float
var label : Label

func _ready():
	label = get_node("Label")
	label.visible = false
	label.text = target_level_entry_key + "@" +target_level_key

func _process(delta):

	if display_time > 0:

		update_scale()
		display_time -= delta
		if display_time <= 0:
			label.visible = false

func ping() -> void:
	display_time = 0.5
	update_scale()
	label.visible = true

func update_scale() -> void:
	var camera_transform = get_viewport().canvas_transform
	var scale = 1 / camera_transform.get_scale().x
	label.rect_position = -label.rect_size/ (2 * camera_transform.get_scale().x)
	label.rect_scale = Vector2(scale, scale)
