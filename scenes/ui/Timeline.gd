extends ColorRect


# Declare member variables here. Examples:
# var a = 2
# var b = "text"


# Called when the node enters the scene tree for the first time.
func _ready():
	pass # Replace with function body.


# Called every frame. 'delta' is the elapsed time since the previous frame.
#func _process(delta):
#	pass
func _draw():

	for x in range(0, rect_size.x, 100):
		draw_line(Vector2(x,0), Vector2(x,rect_size.y), Color(0.6, 0.6, 0.6), 1)

