tool
extends StaticBody2D

func get_class(): return "NotStickyStaticBody2D"

func _ready():
	set_meta("is_not_sticky", true)

func _enter_tree():
	pass


func _exit_tree():
	 pass
