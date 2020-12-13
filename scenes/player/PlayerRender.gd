extends Node
class_name  PlayerRender

var  player

const SHADOW_MAX_DISTANCE = 350
const SHADOW_MIN_DISTANCE = 70

var shadow_sprite : Sprite
var shadow_sprite_base_scale : Vector2
var main_sprite : Sprite


var flip_sprite = false

func _ready():
	player = get_parent()
	main_sprite = player.get_node("MainSprite")
	shadow_sprite = player.get_node("ShadowSprite")
	shadow_sprite_base_scale = shadow_sprite.scale

func update() -> void:
	update_flip_facing(player.movement.is_facing_right)


	update_shadow(player.movement.last_traveled_distance)


func flip_sprite(sprite: Sprite, val : bool):
	sprite.offset.x = -sprite.offset.x
	sprite.position.x = -sprite.position.x
	sprite.flip_h = val

func update_flip_facing(val):
	if flip_sprite != val:
		flip_sprite(shadow_sprite, val)
		flip_sprite(main_sprite, val)
		var render_root : Node2D = player.get_node("RenderRoot")
		render_root.scale.x *= -1
		flip_sprite = val



func update_shadow(traveled_distance : float) -> void:
	var currentUp = Vector2(0,-1).rotated(player.rotation)
	var space_state = player.get_world_2d().direct_space_state
	var result = space_state.intersect_ray(player.position, player.position - currentUp * (SHADOW_MAX_DISTANCE), [], player.collision_mask)
	if not result:
		shadow_sprite.visible = false
	else:
		shadow_sprite.visible = true
		var ground_distance = (player.position- result.position).length()
		shadow_sprite.position.y = ground_distance
		#shadow_sprite.rotation = lerp(shadow_sprite.rotation, currentUp.angle_to(result.normal), 0.2)


		var angle_correction = currentUp.angle_to(result.normal) - shadow_sprite.rotation
		var angle_correction_ratio = min(1, traveled_distance * 0.005)
		shadow_sprite.rotation += angle_correction * angle_correction_ratio


		var distance_ratio = 1 - (ground_distance - SHADOW_MIN_DISTANCE)  / (SHADOW_MAX_DISTANCE - SHADOW_MIN_DISTANCE)

		shadow_sprite.modulate = Color(1,1,1, distance_ratio * 1)
		shadow_sprite.scale =  shadow_sprite_base_scale * distance_ratio

