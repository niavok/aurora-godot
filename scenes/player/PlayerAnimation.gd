extends Node
class_name PlayerAnimation

var  player


func _ready():
	player = get_parent()
	walk_init()

func update(delta) -> void:
	update_walk(delta)
	update_arm_animation(delta)


var walk_ankor_position: Vector2
var walk_node_position: Vector2
var walk_main_leg_index = -1
var walk_render_root : Node2D
var WALK_WALK_MAX_LEG_ANGLE = 0.35
var WALK_SPRINT_MAX_LEG_ANGLE = 0.65
var WALK_WALK_MAX_BOTTOM_LEG_ANGLE = 1.15
var WALK_SPRINT_MAX_BOTTOM_LEG_ANGLE = 1.2

var walk_feet = [null, null]
var walk_hips = [null, null]
var walk_legs = [null, null]
var walk_leg_lengths = [0.0, 0.0]
var walk_initial_angles = [0.0, 0.0]

var walk_last_main_hip_rotation

var walk_target_hip_height_offset = 0.0

var walk_hip_target_angle = [0.0, 0.0]
var walk_leg_target_angle = [0.0, 0.0]

var walk_hip_absorb_angle = [0.0, 0.0]
var walk_leg_absorb_angle = [0.0, 0.0]


var walk_jump_reception_velocity : float
var walk_jump_reception_offset : float


func update_walk(delta : float) -> void:

	var walking = false

	if walk_jump_reception_offset > 0:
		for leg_index in range(0, 2):
			var hip = walk_hips[leg_index]
			var leg = walk_legs[leg_index]

			hip.rotation -= walk_hip_absorb_angle[leg_index]
			leg.rotation -= walk_leg_absorb_angle[leg_index]


	if not player.movement.is_sticked_to_ground:
		walk_main_leg_index = -1
		walk_jump_reception_offset = 0

	else:
		if walk_main_leg_index == -1:
			walk_jump_reception_velocity = player.movement.sticking_velocity.project(Vector2(0, -1).rotated(player.rotation)).length()
			walk_jump_reception_offset = 0

			walk_main_leg_index = 1
			walk_init_step()

		if player.movement.velocity != Vector2():
			walk_move_main_leg()
			walk_move_secondary_leg()
			walking = true;

		var main_hip = walk_hips[walk_main_leg_index]
		var main_hip_target_angle = walk_hip_target_angle[walk_main_leg_index]

		# Swap legs if necessary
		var angle_diff =  abs(main_hip_target_angle) - abs(walk_last_main_hip_rotation)
		if abs(main_hip_target_angle) > walk_get_max_leg_angle() and angle_diff > 1e-9:
			walk_main_leg_index = (walk_main_leg_index + 1) %2
			walk_init_step()

			walk_move_main_leg()
			walk_move_secondary_leg()

		else:
			walk_last_main_hip_rotation = main_hip_target_angle

	#print("update_walk velocity = ", velocity)

	if not walking:
		if player.movement.is_flying:
				walk_hip_target_angle[0] = PI / 4
				walk_leg_target_angle[0] = -PI / 1.5
				walk_hip_target_angle[1] = -PI / 4
				walk_leg_target_angle[1] = PI / 1.5
		else:
			for leg_index in range(0, 2):
				walk_hip_target_angle[leg_index] = 0
				walk_leg_target_angle[leg_index] = 0


	for leg_index in range(0, 2):
		var hip = walk_hips[leg_index]
		var leg = walk_legs[leg_index]

		var is_quick = walking and player.movement.is_sprinting

		var hip_angular_velocity = 5.0 if is_quick else 2.0
		var leg_hip_angular_velocity = 12.0 if is_quick else 6.0

		hip.rotation = Helpers.move_max_velocity(hip.rotation, walk_hip_target_angle[leg_index], hip_angular_velocity, delta)
		leg.rotation = Helpers.move_max_velocity(leg.rotation, walk_leg_target_angle[leg_index], leg_hip_angular_velocity, delta)

	# Update height
	var leg_length = walk_leg_lengths[0]
	var hip_height_offset = leg_length * (1 - cos(walk_hips[0].rotation))

	if walk_jump_reception_velocity > 0:

		var absorb_acceleration = 40000
		var time_to_absorb = walk_jump_reception_velocity / absorb_acceleration

		if time_to_absorb > delta:
			walk_jump_reception_velocity -= absorb_acceleration * delta
			walk_jump_reception_offset += walk_jump_reception_velocity * delta + absorb_acceleration * delta*delta
		else:
			walk_jump_reception_velocity = 0
			walk_jump_reception_offset += absorb_acceleration * time_to_absorb*time_to_absorb

		walk_jump_reception_velocity = 0

	walk_target_hip_height_offset = hip_height_offset + walk_jump_reception_offset
	walk_render_root.position.y = walk_target_hip_height_offset

	if walk_jump_reception_offset > 0:
		for leg_index in range(0, 2):
			var hip = walk_hips[leg_index]
			var leg = walk_legs[leg_index]

			var absorbsion_angle = acos(1-walk_jump_reception_offset / walk_leg_lengths[leg_index] * 2)

			#print("absorbsion_angle ", absorbsion_angle, " walk_jump_reception_offset ",walk_jump_reception_offset)

			if leg_index == 1:
				walk_hip_absorb_angle[leg_index] = -absorbsion_angle
				walk_leg_absorb_angle[leg_index] = absorbsion_angle * 2
			else:
				walk_hip_absorb_angle[leg_index] = -absorbsion_angle * 0.5
				walk_leg_absorb_angle[leg_index] = absorbsion_angle * 1.5

			hip.rotation += walk_hip_absorb_angle[leg_index]
			leg.rotation += walk_leg_absorb_angle[leg_index]

			if walk_jump_reception_velocity == 0:
				walk_jump_reception_offset -= 10 * delta
				if walk_jump_reception_offset < 0:
					walk_jump_reception_offset = 0

func walk_init():
	walk_main_leg_index = -1
	walk_init_leg(0, "RenderRoot/Body/LeftTopLeg/LeftBottomLeg/LeftFeet", "RenderRoot/Body/LeftTopLeg/LeftBottomLeg", "RenderRoot/Body/LeftTopLeg")
	walk_init_leg(1, "RenderRoot/Body/RightTopLeg/RightBottomLeg/RightFeet", "RenderRoot/Body/RightTopLeg/RightBottomLeg", "RenderRoot/Body/RightTopLeg")
	walk_render_root = player.get_node("RenderRoot")

func walk_init_leg(leg_index : int, feet_name : NodePath, leg_name : NodePath, hip_name : NodePath):
	var hip : Node2D = player.get_node(hip_name)
	var foot : Node2D = player.get_node(feet_name)
	var leg : Node2D = player.get_node(leg_name)

	hip.rotation = 0
	walk_hips[leg_index] = hip
	walk_feet[leg_index] = foot
	walk_legs[leg_index] = leg

	var hip_location = hip.to_global(Vector2())
	var foot_location = foot.to_global(Vector2())

	var hip_to_foot : Vector2 = foot_location - hip_location

	var leg_length = hip_to_foot.length()
	walk_leg_lengths[leg_index] = leg_length

	var bottom_direction : Vector2 = Vector2(0, 1).rotated(player.rotation)
	walk_initial_angles[leg_index] = bottom_direction.angle_to(hip_to_foot)


func walk_init_step():
	var leg = walk_legs[walk_main_leg_index]
	var hip = walk_hips[walk_main_leg_index]
	walk_leg_target_angle[walk_main_leg_index] = 0

	walk_ankor_position = walk_feet[walk_main_leg_index].to_global(Vector2())
	walk_node_position = player.position
	walk_last_main_hip_rotation = walk_hip_target_angle[walk_main_leg_index]


func walk_move_main_leg():
	var right_direction : Vector2 = Vector2(1, 0).rotated(player.rotation)

	var hip = walk_hips[walk_main_leg_index]
	var hip_location = hip.to_global(Vector2())

	var ankor_to_hip : Vector2 = hip_location - walk_ankor_position
	var min_distance = right_direction.dot(ankor_to_hip)

	if player.movement.is_facing_right:
		min_distance *= -1

	var leg_length = walk_leg_lengths[walk_main_leg_index]

	if abs(min_distance) > leg_length:
		min_distance = leg_length * sign(min_distance)

	var leg_angle = asin(min_distance / leg_length)
	var initial_angle = walk_initial_angles[walk_main_leg_index]

	walk_hip_target_angle[walk_main_leg_index] = leg_angle - initial_angle

func walk_get_max_leg_angle() -> float:
	if player.movement.is_sprinting:
		return WALK_SPRINT_MAX_LEG_ANGLE
	else:
		return WALK_WALK_MAX_LEG_ANGLE

func walk_get_max_bottom_leg_angle() -> float:
	if player.movement.is_sprinting:
		return WALK_SPRINT_MAX_BOTTOM_LEG_ANGLE
	else:
		return WALK_WALK_MAX_BOTTOM_LEG_ANGLE

func walk_move_secondary_leg() -> void:
	var main_hip = walk_hips[walk_main_leg_index]
	var secondary_index = (walk_main_leg_index + 1) % 2
	var hip = walk_hips[secondary_index]
	var leg = walk_legs[secondary_index]

	var target_hip_rotation = -walk_hip_target_angle[walk_main_leg_index]
	walk_hip_target_angle[secondary_index] = target_hip_rotation

	var max_angle = walk_get_max_leg_angle()
	walk_leg_target_angle[secondary_index] = walk_get_max_bottom_leg_angle()*(1 - abs(target_hip_rotation / max_angle))


var arm_animation_time_accumulator = 0.0
func update_arm_animation(delta) -> void:
	var left_arm = player.get_node("RenderRoot/Body/LeftArm")
	var right_arm = player.get_node("RenderRoot/Body/RightArm")
	var head = player.get_node("RenderRoot/Body/Head")



	arm_animation_time_accumulator += delta

	var arm_rotation_phase = sin(arm_animation_time_accumulator)
	left_arm.rotation = arm_rotation_phase * 0.1
	right_arm.rotation = arm_rotation_phase * 0.1
	head.rotation = arm_rotation_phase * -0.05

