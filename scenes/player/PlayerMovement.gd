extends Node
class_name PlayerMovement



const FEET_DISTANCE = 70
const MAX_WALK_ANGLE = PI / 4

const GRAVITY = 1200
const AIR_RESISTANCE_COEF = 0.002
const FLYING_AIR_RESISTANCE_COEF = 0.003


const WALK_VELOCITY = 400 * 0.25
const SPRINT_VELOCITY = 1000 * 0.25
const FLY_ACCELERATION = 1200
const FALL_ACCELERATION = 400
const MAX_JUMP_DURATION = 0.2
const INITIAL_JUMP_VELOCITY = 400
const JUMP_VELOCITY_PER_SEC = 2000




const MAX_STAMINA = 100.0
const STAMINA_REGEN_PER_SEC = 150.0
const STAMINA_SPRINT_PER_SEC = 30
const STAMINA_FLY_PER_SEC = 25
const STAMINA_FLY_ACCELERATION_PER_SEC = 5
const STAMINA_JUMP_PER_SEC = 80


var stamina = MAX_STAMINA
var velocity : Vector2

var target_rotation : float
var jump_target_rotation
var jump_direction
var jump_duration
var time_in_no_fly_zone = 0
var distance_angular_velocity = 0
var local_feet_position = Vector2(0, FEET_DISTANCE)
var stick_normal := Vector2()
var stick_position := Vector2()
var left_local_movement = 0
var right_local_movement = 0
var sticking_velocity: Vector2
var last_traveled_distance : float

# TODO STATE instead of tons of boolean
var is_on_ladder = false
var is_sticked_to_ground = false
var is_sprinting = false

var is_flying = false
var is_jumping = false
var is_touching_wall = false

var can_take_ladder = true
var can_fly = false


var wall_normal


var rng = RandomNumberGenerator.new()

var  player
var ladder_detector : Area2D
var no_fly_zone_detector: Area2D

var is_facing_right = true



func _ready():
	player = get_parent()
	ladder_detector = player.get_node("LadderDetectorArea2D")
	no_fly_zone_detector = player.get_node("NoFlyZoneDetectorArea2D3")


func enter_level(order_enter_level_position : Vector2):
	player.position = order_enter_level_position
	player.rotation = 0
	stamina = MAX_STAMINA
	velocity = Vector2()

	stamina = MAX_STAMINA
	target_rotation = 0.0
	is_on_ladder = false
	is_sticked_to_ground = false
	is_sprinting = false


	is_sticked_to_ground = false
	is_jumping = false
	is_sprinting = false
	can_fly = false
	is_flying = false
	is_touching_wall = false
	left_local_movement = 0
	right_local_movement = 0
	is_on_ladder = false
	can_take_ladder = true
	is_facing_right = false


func update(delta: float) -> void:
	if GameGlobals.world.can_explore():
		movement_exploration(delta)

func movement_exploration(delta : float):
	stamina = min(MAX_STAMINA, stamina + STAMINA_REGEN_PER_SEC * delta)

	if !Input.is_action_pressed("player_sprint"):
		is_sprinting = false
	elif Input.is_action_just_pressed("player_sprint"):
		is_sprinting = true

	target_rotation = 0
	is_on_ladder = ladder_detector.get_overlapping_areas().size() > 0
	if not is_on_ladder:
		can_take_ladder = true

	var traveled_distance = 0

	if is_sticked_to_ground:
		if(Input.is_action_pressed("player_down") or Input.is_action_pressed("player_up")) and is_on_ladder:
			ladder_movement(delta)
		traveled_distance = sticked_movement(delta)
	else:
		if Input.is_action_just_pressed("player_jump") and is_jump_possible():
			traveled_distance = start_jump(delta)
		else:
			if can_take_ladder and is_on_ladder:
				ladder_movement(delta)
			else:
				traveled_distance = free_movement(delta)

	update_rotation_position(delta, traveled_distance)
	last_traveled_distance = traveled_distance
	#update_debug_stick()
	#render.update()
	#update_shadow(traveled_distance)


func update_rotation_position(delta, traveled_distance) -> void:
	if is_sticked_to_ground:
		update_rotation_position_sticked(delta, traveled_distance)
	else:
		# Rotate around center
		player.rotation = lerp_angle(player.rotation, target_rotation, delta)

func sticked_movement(delta) -> float:

	# if want jump, un-stick
	if Input.is_action_just_pressed("player_jump"):
		start_jump(delta)
		return 0.0
	else:
		return follow_shapes(delta)

func ladder_movement(delta: float) -> void:
	velocity = Vector2()
	is_sticked_to_ground = false
	is_flying = false
	is_jumping = false

	var target_movement = Vector2()
	if  Input.is_action_pressed("player_left"):
		target_movement.x += -1
		wall_normal = Vector2(-1,0)
	if Input.is_action_pressed("player_right"):
		target_movement.x += 1
		wall_normal = Vector2(1,0)
	if  Input.is_action_pressed("player_down"):
		target_movement.y += 1
	if Input.is_action_pressed("player_up"):
		target_movement.y += -1

	target_movement = target_movement.normalized()
	if (target_movement.x > 0 and is_facing_right) or (target_movement.x < 0 and not is_facing_right):
			is_facing_right = !is_facing_right


	if target_movement != Vector2():
		var stamina_consumption = 0

		if is_sprinting:
			stamina_consumption = STAMINA_SPRINT_PER_SEC * delta
			if stamina_consumption > stamina:
				is_sprinting = false

		var movement_velocity = SPRINT_VELOCITY if is_sprinting else WALK_VELOCITY

		if stamina_consumption > stamina:
			var stamina_remaining_ratio = stamina / stamina_consumption
			movement_velocity *= stamina_remaining_ratio
			stamina_consumption = stamina

		var initial_position = player.position


		var target_velocity = target_movement * movement_velocity
		velocity = player.move_and_slide(target_velocity)

		if stamina_consumption > 0:
			var projected_movement_distance = movement_velocity * delta
			var actual_movement_distance = (player.position - initial_position).length()

			var movement_distance_ratio = actual_movement_distance / projected_movement_distance
			stamina -= stamina_consumption * movement_distance_ratio

func is_jump_possible():
	if is_touching_wall:
		return true
	elif is_on_ladder:
		return true
	else:

		var space_state = player.get_world_2d().direct_space_state
		var result = space_state.intersect_ray(player.position, player.position - wall_normal * (FEET_DISTANCE * 1.1), [], player.collision_mask)
		if result:
			return true
		else:
			return false

func start_jump(delta) -> float:
	is_sticked_to_ground = false
	is_jumping = true
	can_take_ladder = not can_take_ladder

	jump_target_rotation = player.rotation
	jump_direction = wall_normal.slerp(Vector2(0, -1).rotated(player.rotation), 0.5)
	jump_duration = 0

	return free_movement(delta)


func free_movement(delta) -> float:
	var movement_direction = Vector2()

	if is_jumping:
		can_fly = false

	if Input.is_action_just_released("player_jump"):
		is_jumping = false
		can_fly = true

	if can_fly:
		is_flying = Input.is_action_pressed("player_jump")

	if  Input.is_action_pressed("player_left"):
		movement_direction.x -= 1
		if !is_facing_right:
			is_facing_right = true
	if Input.is_action_pressed("player_right"):
		movement_direction.x += 1
		if is_facing_right:
			is_facing_right = false
	if is_flying:
		if Input.is_action_pressed("player_up"):
			movement_direction.y -= 1
		if Input.is_action_pressed("player_down"):
			movement_direction.y += 1

	movement_direction = movement_direction.normalized()

	if is_flying:
			var stamina_consumption = STAMINA_FLY_PER_SEC * delta

			if movement_direction != Vector2():
				stamina_consumption += (STAMINA_FLY_ACCELERATION_PER_SEC * (abs(movement_direction.x) - movement_direction.y*2))  * delta
			if stamina_consumption > stamina:
				is_flying = false
				can_fly = false
			else:
				stamina -= stamina_consumption

	if is_jumping:
			var stamina_consumption = STAMINA_JUMP_PER_SEC * delta

			if stamina_consumption > stamina:
				is_jumping = false
			else:
				stamina -= stamina_consumption

	if is_flying:
		velocity += movement_direction * FLY_ACCELERATION * delta
	else:
		velocity += movement_direction * FALL_ACCELERATION * delta


	if is_jumping:
		jump_duration += delta

		target_rotation = jump_target_rotation;

		if jump_duration > MAX_JUMP_DURATION:
			jump_duration =  MAX_JUMP_DURATION
			is_jumping = false

		var velocity_in_jump_direction  = velocity.dot(jump_direction)
		var jump_velocity =  INITIAL_JUMP_VELOCITY + jump_duration * JUMP_VELOCITY_PER_SEC

		if velocity_in_jump_direction < jump_velocity:
			var not_jump_direction = Vector2(jump_direction.y, -jump_direction.x)
			var velocity_in_not_jump_direction  = velocity.dot(not_jump_direction)
			velocity = jump_direction  * jump_velocity + not_jump_direction * velocity_in_not_jump_direction

	if not is_flying and not is_jumping:
		# fall with gravity
		velocity.y += GRAVITY * delta

	if not is_jumping:
		var air_resistance_coef = AIR_RESISTANCE_COEF if not is_flying else FLYING_AIR_RESISTANCE_COEF
		var air_resistance =  air_resistance_coef * velocity.length_squared() * delta
		var velocity_len = velocity.length()
		if(velocity_len > 1e-6):
			air_resistance = min(air_resistance, velocity_len)
			velocity = velocity * (1 - air_resistance / velocity_len)

	if no_fly_zone_detector.get_overlapping_areas().size() > 0:
		time_in_no_fly_zone += delta
		# no fly zone
#		velocity.y +=  FLY_ACCELERATION * time_in_no_fly_zone * 0.05
#		velocity.x +=  FLY_ACCELERATION * time_in_no_fly_zone * rng.randfn(0, 0.2)
	else:
		time_in_no_fly_zone = 0


	return apply_velocity(delta)


func update_rotation_position_sticked(delta, traveled_distance):
	if traveled_distance == 0:
		return

	var angle_correction = Helpers.angle_to_angle(player.rotation, target_rotation)
	if abs(angle_correction) > 1e-4 or abs(distance_angular_velocity) > 1e4:
		#var distance_angular_acceleration = pow(abs(angle_correction),1.5) * 0.00005
		var distance_angular_acceleration = 0.000004

		var angle_correction_sign = sign(angle_correction)
		var angle_correction_cd = angle_correction_sign * angle_correction # cd for correction_direction
		var distance_angular_velocity_cd  = angle_correction_sign * distance_angular_velocity

		var angle_correction_if_brake_now_cd = distance_angular_velocity_cd * sign(distance_angular_velocity_cd) * distance_angular_velocity_cd / distance_angular_acceleration

		if angle_correction_if_brake_now_cd > angle_correction_cd:
			# overshoot need full brake now
			var acceleration = - angle_correction_sign * distance_angular_acceleration
			distance_angular_velocity += acceleration * traveled_distance
		else:
			# need to accelerate
			var b = traveled_distance * distance_angular_acceleration
			var c = - angle_correction_cd * distance_angular_acceleration
			var disc = b *b - 4 *c
			assert(disc > 0)

			var target_angular_velocity_cd = (- b + sqrt(disc))/2
			var need_acceleration = target_angular_velocity_cd - distance_angular_velocity_cd
			if need_acceleration < distance_angular_acceleration * traveled_distance:
				distance_angular_velocity = angle_correction_sign * target_angular_velocity_cd
			else:
				# full_acceleration
				var delta_v = angle_correction_sign * distance_angular_acceleration * traveled_distance
				distance_angular_velocity += delta_v * traveled_distance


		player.rotation += distance_angular_velocity * traveled_distance

	var feet_position = player.to_global(local_feet_position)
	var position_correction = (stick_position - feet_position).project(stick_normal) * 0.1
	var correction_ratio = min(1, traveled_distance * 0.1)
	player.position += position_correction * correction_ratio

func follow_shapes(delta) -> float:
	# first, move normally

	var traveled_distance = 0

	if  (left_local_movement == 0
		or ((Input.is_action_just_pressed("player_left") or (Input.is_action_just_pressed("player_right"))
			and abs(stick_normal.y) > 0.5))):
		if stick_normal.y > 0.5:
			left_local_movement = 1
		elif stick_normal.y < -0.5:
			left_local_movement = -1
		elif stick_normal.x > 0:
			left_local_movement = -1
		else:
			left_local_movement = -1
		right_local_movement = -left_local_movement

	var local_movement = 0
	if  Input.is_action_pressed("player_left"):
		local_movement += left_local_movement
	if Input.is_action_pressed("player_right"):
		local_movement += right_local_movement

	if (local_movement > 0 and is_facing_right) or (local_movement < 0 and not is_facing_right):
			is_facing_right = !is_facing_right

	if local_movement != 0:
		var right_direction = stick_normal.rotated(PI/2)
		var target_direction = right_direction * local_movement

		var stamina_consumption = 0

		if is_sprinting:
			stamina_consumption = STAMINA_SPRINT_PER_SEC * delta
			if stamina_consumption > stamina:
				is_sprinting = false

		var movement_velocity = SPRINT_VELOCITY if is_sprinting else WALK_VELOCITY

		if stamina_consumption > stamina:
			var stamina_remaining_ratio = stamina / stamina_consumption
			movement_velocity *= stamina_remaining_ratio
			stamina_consumption = stamina

		var initial_position = player.position


		var target_velocity = target_direction * movement_velocity
		velocity = player.move_and_slide(target_velocity, Vector2(), false, 4, MAX_WALK_ANGLE / 2)

		var actual_movement_distance = (player.position - initial_position).length()

		if stamina_consumption > 0:
			var projected_movement_distance = movement_velocity * delta


			var movement_distance_ratio = actual_movement_distance / projected_movement_distance
			stamina -= stamina_consumption * movement_distance_ratio

		traveled_distance = actual_movement_distance


		if abs(stick_normal.x) > 0.5 and target_direction.y > 0.8 and player.get_slide_count() > 0:
			# if contact on ground while on a wall, stick to the ground
			var last_collision = player.get_slide_collision(player.get_slide_count() - 1)
			if last_collision.normal.x < 0.5:
				stick_normal = last_collision.normal
				stick_position = last_collision.position

		# secondly, find new sticking point
		var space_state = player.get_world_2d().direct_space_state
		var result = space_state.intersect_ray(player.position, player.position - stick_normal * (FEET_DISTANCE * 2), [], player.collision_mask)

		var too_high_angle = false

		if result:
			is_touching_wall = true
			wall_normal = result.normal
			if stick_normal.angle_to(result.normal) > MAX_WALK_ANGLE:
				too_high_angle = true
		else:
			is_touching_wall = false

		if too_high_angle:
			player.position = initial_position
			velocity = Vector2()
			traveled_distance = 0
		elif result and !result.collider.has_meta("is_not_sticky"):
			# sticking area found, update stick point and normal
			var new_stick_normal = result.normal
			var new_stick_position = result.position
			var delta_angle = stick_normal.angle_to(new_stick_normal)

			stick_position = new_stick_position
			stick_normal = new_stick_normal
		else:
			# no sticking area found, falling
			is_sticked_to_ground = false
	else:
		velocity = Vector2()

	target_rotation = stick_normal.angle() + PI/2
	return traveled_distance

func apply_velocity(delta) -> float:
	var previous_position = player.position
	velocity = player.move_and_slide (velocity)

	if player.get_slide_count() > 0:

		var collision = player.get_slide_collision(player.get_slide_count()-1)
		is_touching_wall = true
		wall_normal = collision.normal
	else:
		is_touching_wall = false

	# test stickyness
	var currentUp = Vector2(0,-1).rotated(player.rotation)
	var space_state = player.get_world_2d().direct_space_state
	var result = space_state.intersect_ray(player.position, player.position - currentUp * (FEET_DISTANCE * 1), [], player.collision_mask)

	var too_high_angle

	if result:
		if currentUp.angle_to(result.normal) > MAX_WALK_ANGLE:
			too_high_angle = true
		if velocity.dot(result.normal) > 0:
			too_high_angle = true

	if result and !result.collider.has_meta("is_not_sticky") and !too_high_angle:
		# collision, stick except if still jumping
		sticking_velocity = velocity
		velocity = Vector2()
		is_sticked_to_ground = true
		is_flying = false
		is_jumping = false

		stick_position = result.position
		stick_normal = result.normal
		is_touching_wall = true
		wall_normal = result.normal

		target_rotation = stick_normal.angle() + PI/2;
		left_local_movement = 0
		right_local_movement = 0

	return (player.position - previous_position).length()
