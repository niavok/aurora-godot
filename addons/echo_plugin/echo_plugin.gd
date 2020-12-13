tool
extends EditorPlugin


func _enter_tree():
	add_custom_type("NotStickyStaticBody2D", "StaticBody2D", preload("not_sticky_static_body_2d.gd"), preload("NotStickyStaticBody2D.svg"))


func _exit_tree():
	remove_custom_type("NotStickyStaticBody2D")


