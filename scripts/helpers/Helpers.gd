extends Node

func _ready():
	pass # Replace with function body.


func move_max_velocity(initial_position: float, target_position: float, max_velocity: float, delta : float) -> float:
		var max_delta_pos = max_velocity * delta
		var target_correction = target_position - initial_position

		if abs(target_correction) > max_delta_pos:
			return initial_position + sign(target_correction) * max_delta_pos
		else:
			return target_position

static func angle_to_angle(from, to):
	return fposmod(to-from + PI, PI*2) - PI
