extends Node2D

const WORLD_BLOCK_SIZE = 1000
#const WORLD_BLOCK_DEPTH = 1000.0
const WORLD_BLOCK_DEPTH = 1.0
const WORLD_BLOCK_INTERFACE_SECTION = WORLD_BLOCK_SIZE * WORLD_BLOCK_DEPTH
const WORLD_BLOCK_COUNT_X = 80
const WORLD_BLOCK_COUNT_Y = 30
const WORLD_SIZE_X = WORLD_BLOCK_COUNT_X * WORLD_BLOCK_SIZE
const WORLD_SIZE_Y = WORLD_BLOCK_COUNT_Y * WORLD_BLOCK_SIZE
const WORLD_BLOCK_COUNT = WORLD_BLOCK_COUNT_X * WORLD_BLOCK_COUNT_Y
const WORLD_SIZE = Vector2(WORLD_BLOCK_SIZE * WORLD_BLOCK_COUNT_X, WORLD_BLOCK_COUNT_Y * WORLD_BLOCK_COUNT_Y)


const WORLD_RADIUS = 100000.0
const WORLD_BLOCK_SPHERICAL_OVERFLOW = WORLD_RADIUS - sqrt(WORLD_RADIUS*WORLD_RADIUS - WORLD_BLOCK_SIZE*WORLD_BLOCK_SIZE)
const WORLD_BLOCK_SPHERICAL_COEF = WORLD_BLOCK_SPHERICAL_OVERFLOW / WORLD_BLOCK_SIZE

const RENDER_DEBUG_SCALE = 800.0 / WORLD_SIZE_X
const RENDER_DEBUG_VELOCITY_SCALE = 15
const RENDER_DEBUG_OFFSET = Vector2(-2200, 750 - RENDER_DEBUG_SCALE * WORLD_BLOCK_SIZE * WORLD_BLOCK_COUNT_Y)

var DEBUG_BLOCK_X = 17
var DEBUG_BLOCK_Y = WORLD_BLOCK_COUNT_Y-1



const blocks = []

var rng = RandomNumberGenerator.new()

var world_processing_thread
var world_processing_thread_running

var world_enable_sun

class Block:

	# debug, clean
	var is_init = false

	# static
	var gas_volume
	var block_x
	var block_y
	var tl_render_block : Block
	var tr_render_block : Block
	var br_render_block : Block
	var bl_render_block : Block
	var l_render_block : Block
	var r_render_block : Block
	var t_render_block : Block
	var b_render_block : Block
	var is_opaque


	# dynamic
	var input_internal_energy = 0
	var output_internal_energy = 0
	var input_gas_composition_N = []
	var output_gas_composition_N = []
	var kinetic_energy = 0
	var velocity_direction = Vector2()
	var pending_input_internal_energy
	var pending_input_gas_composition_N = []
	var pending_kinetic_energy = []

	# Light energy, four direction, 3 energy range : IR, Visible, UV
	var light_energy_by_direction = []
	var light_energy_by_direction_pending = []

	# Cache
	var pressure
	var temperature
	var N
	var energy_per_k
	var mass
	var input_volume = 0.0
	var output_volume = 0.0
	var input_mass = 0.0
	var output_mass = 0.0
	var input_elastic_energy
	var output_elastic_energy
	var input_thermal_energy
	var output_thermal_energy
	var input_energy_per_k
	var output_energy_per_k
	var give_volume_orders = []
	var velocity_cache
	var velocity_amplitude
	var pressure_gradient
	var new_velocity_list = []
	var albedo = []
	var opacity = []
	var diffusion = []

	func init_empty():
		assert(!is_init)
		is_init = true
		input_internal_energy = 0
		output_internal_energy = 0
		kinetic_energy = 0
		velocity_direction = Vector2()
		velocity_cache = Vector2()
		input_gas_composition_N.resize(PhysicalConstants.Gas.Count)
		output_gas_composition_N.resize(PhysicalConstants.Gas.Count)
		pending_input_gas_composition_N.resize(PhysicalConstants.Gas.Count)
		pending_input_internal_energy = 0

		for gas in range(PhysicalConstants.Gas.Count):
			input_gas_composition_N[gas] = 0
			output_gas_composition_N[gas] = 0
			pending_input_gas_composition_N[gas] = 0

		light_energy_by_direction = []
		light_energy_by_direction_pending = []
		for light_direction in range(PhysicalConstants.LightDirection.Count):
			light_energy_by_direction.append([])
			light_energy_by_direction_pending.append([])
			var light_energy_direction = light_energy_by_direction[light_direction]
			var light_energy_direction_pending = light_energy_by_direction_pending[light_direction]
			for light_type in range(PhysicalConstants.LightType.Count):
				light_energy_direction.append(0.0)
				light_energy_direction_pending.append(0.0)

	func velocity_dumping(new_velocity : Vector2) -> Vector2:
		assert(false) # virtual pure
		return new_velocity

	func init_with_gas(gas : int, temperature : float, pressure : float):
		init_empty()

		var totalN = PhysicalConstants.estimate_gas_N(gas_volume, pressure, temperature)
		output_gas_composition_N[gas] = totalN
		output_internal_energy = PhysicalConstants.estimate_internal_energy(gas, totalN, temperature, gas_volume);

	func add_light(light_direction, light_type, light_energy : float):
		light_energy_by_direction[light_direction][light_type] += light_energy

	func update_cache():

		N = 0
		input_energy_per_k = 0
		output_energy_per_k = 0
		input_mass = 0
		output_mass = 0

		for gas in range(PhysicalConstants.Gas.Count):
			var input_N = input_gas_composition_N[gas]
			var output_N = output_gas_composition_N[gas]
			var gas_N = input_N + output_N

			if gas_N > 0:

				var gas_specific_heat_by_mole = PhysicalConstants.gas_specific_heat_by_mole[gas]
				var gas_mass_by_mole = PhysicalConstants.gas_mass_by_mole[gas]

				N += gas_N
				input_energy_per_k += input_N * gas_specific_heat_by_mole
				output_energy_per_k += output_N * gas_specific_heat_by_mole
				input_mass += input_N * gas_mass_by_mole;
				output_mass += output_N * gas_mass_by_mole;

		energy_per_k = input_energy_per_k + output_energy_per_k
		mass = input_mass + output_mass

		var internal_energy = input_internal_energy + output_internal_energy

		if N == 0:
			pressure = 0
			temperature = 0
			pressure_gradient = 0
		else:
			if input_mass == 0:
				input_thermal_energy = 0
			else:
				input_thermal_energy =  input_internal_energy / ( 1 + PhysicalConstants.gas_elastic_coef * PhysicalConstants.gas_constant * N / input_energy_per_k)
			if output_mass == 0:
				output_thermal_energy = 0
			else:
				output_thermal_energy =  output_internal_energy / ( 1 + PhysicalConstants.gas_elastic_coef * PhysicalConstants.gas_constant * N / output_energy_per_k)

			input_elastic_energy = input_internal_energy - input_thermal_energy
			output_elastic_energy = output_internal_energy - output_thermal_energy

			if input_elastic_energy > output_elastic_energy:
				input_volume = gas_volume / (1+ output_elastic_energy / input_elastic_energy)
				output_volume = gas_volume - input_volume
				pressure = input_elastic_energy / (PhysicalConstants.gas_elastic_coef * input_volume)
			else:
				output_volume = gas_volume / (1+ input_elastic_energy / output_elastic_energy)
				input_volume = gas_volume - output_volume
				pressure = output_elastic_energy / (PhysicalConstants.gas_elastic_coef * output_volume)

			var elastic_energy = input_elastic_energy + output_elastic_energy;
			var thermal_energy = input_thermal_energy + output_thermal_energy;

			temperature = thermal_energy / energy_per_k
			var density = mass / gas_volume
			pressure_gradient = density * PhysicalConstants.gravity

		if kinetic_energy > 0:
			velocity_amplitude = sqrt(kinetic_energy * 2 / mass)
			velocity_cache = velocity_direction * velocity_amplitude

		if is_opaque:
			opacity = [1.0 , 1.0, 1.0]
		else:
			opacity = [0.1e-5 * pressure, 0.005e-5 * pressure, 0.01e-5 * pressure]

	func add_give_volume_order(output_block : Block, volume : float, delta_height : float):
		give_volume_orders.append([output_block, volume, delta_height])


	func apply_gas_movement():

		#if block_x == DEBUG_BLOCK_X and block_y == DEBUG_BLOCK_Y:
		#	pass

		var new_velocity_sum = Vector2()
		var new_velocity_mass_sum = 0
		for new_velocity in new_velocity_list:
			new_velocity_sum += new_velocity[0] * new_velocity[1]
			new_velocity_mass_sum += new_velocity[1]
		new_velocity_list = []

		# todo check if better to divide by mass or mass sum
		var new_velocity = new_velocity_sum / new_velocity_mass_sum

		var new_velocity_after_damping = velocity_dumping(new_velocity)

		var new_kinetic_energy = 0.5 * mass * new_velocity_after_damping.length_squared()

		var kinetic_energy_diff = new_kinetic_energy - kinetic_energy

		var internal_energy = input_internal_energy + output_internal_energy

		kinetic_energy = new_kinetic_energy
		velocity_direction =new_velocity_after_damping.normalized()

		var remaining_internal_energy = internal_energy - kinetic_energy_diff

		var new_energy_ratio = remaining_internal_energy / internal_energy
		input_internal_energy *= new_energy_ratio
		output_internal_energy *= new_energy_ratio


		var volume_to_give = 0

		for give_order in give_volume_orders:
			volume_to_give += give_order[1]

		var total_given_mass = 0
		var given_mass_by_order = []

		if volume_to_give < output_volume:
			for give_order in give_volume_orders:
				var output_volume_ratio = give_order[1] /  output_volume
				var internal_energy_to_give = output_volume_ratio * output_internal_energy
				var output_block = give_order[0]
				output_block.pending_input_internal_energy += internal_energy_to_give

				var given_mass_for_order = 0

				for gas in range(PhysicalConstants.Gas.Count):
					var N_to_give = output_volume_ratio * output_gas_composition_N[gas]
					output_block.pending_input_gas_composition_N[gas] += N_to_give
					given_mass_for_order += N_to_give * PhysicalConstants.gas_mass_by_mole[gas]
				total_given_mass += given_mass_for_order
				given_mass_by_order.append(given_mass_for_order)

			# remove given_composition
			var remaining_volume_ratio = 1 - volume_to_give / output_volume
			output_internal_energy *= remaining_volume_ratio
			for gas in range(PhysicalConstants.Gas.Count):
				output_gas_composition_N[gas] *= remaining_volume_ratio
		else:
			# give all output, part between order

			for give_order in give_volume_orders:
				var output_volume_ratio = give_order[1] /  volume_to_give
				var internal_energy_to_give = output_volume_ratio * output_internal_energy
				var output_block = give_order[0]
				output_block.pending_input_internal_energy += internal_energy_to_give

				var given_mass_for_order = 0

				for gas in range(PhysicalConstants.Gas.Count):
					var N_to_give = output_volume_ratio * output_gas_composition_N[gas]
					output_block.pending_input_gas_composition_N[gas] += N_to_give
					given_mass_for_order += N_to_give * PhysicalConstants.gas_mass_by_mole[gas]
				total_given_mass += given_mass_for_order
				given_mass_by_order.append(given_mass_for_order)

			# swap output
			var remaining_volume_to_give = volume_to_give - output_volume
			var remaining_volume_to_give_ratio = remaining_volume_to_give / volume_to_give

			output_internal_energy = input_internal_energy
			input_internal_energy = 0
			for gas in range(PhysicalConstants.Gas.Count):
				output_gas_composition_N[gas] = input_gas_composition_N[gas]
				input_gas_composition_N[gas] = 0
			output_volume = input_volume

			# give missing
			var give_order_index = 0
			for give_order in give_volume_orders:
				var output_volume_ratio = remaining_volume_to_give_ratio * give_order[1]/ output_volume
				var internal_energy_to_give = output_volume_ratio * output_internal_energy
				var output_block = give_order[0]
				output_block.pending_input_internal_energy += internal_energy_to_give

				var given_mass_for_order = 0

				for gas in range(PhysicalConstants.Gas.Count):
					var N_to_give = output_volume_ratio * output_gas_composition_N[gas]
					output_block.pending_input_gas_composition_N[gas] += N_to_give
					given_mass_for_order += N_to_give * PhysicalConstants.gas_mass_by_mole[gas]
				total_given_mass += given_mass_for_order
				given_mass_by_order[give_order_index] += given_mass_for_order
				give_order_index += 1

			# remove given composition
			var remaining_volume_ratio = 1 - remaining_volume_to_give / output_volume
			output_internal_energy *= remaining_volume_ratio
			for gas in range(PhysicalConstants.Gas.Count):
				output_gas_composition_N[gas] *= remaining_volume_ratio


		# Transfert part of kinetic energy depending on the given mass

		for give_order_index in range(give_volume_orders.size()):
			var given_mass_ratio = given_mass_by_order[give_order_index] / mass
			var given_kinetic_energy = kinetic_energy * given_mass_ratio
			give_volume_orders[give_order_index][0].give_kinetic_energy(given_kinetic_energy, velocity_direction)

		if mass > 0:
			var remaining_kinetic_energy_ratio = 1 - total_given_mass / mass
			kinetic_energy *= remaining_kinetic_energy_ratio

		# compute potiential energydifference
		for give_order_index in range(give_volume_orders.size()):
			var delta_height = give_volume_orders[give_order_index][2]
			if delta_height != 0:
				var output_block = give_volume_orders[give_order_index][0]
				var potential_energy_diff = delta_height * given_mass_by_order[give_order_index] * PhysicalConstants.gravity
				var volume_ratio = give_volume_orders[give_order_index][1] / gas_volume
				output_block.pending_input_internal_energy += potential_energy_diff * volume_ratio
				output_internal_energy += potential_energy_diff * (1-volume_ratio)
		give_volume_orders = []

	func apply_light_movement(delta_time):

		# TODO update albedo

		var modified_light_by_direction = []

		var light_variation_by_direction = []

		var internal_energy_variation = 0

		# apply pending light
		if block_y == WORLD_BLOCK_COUNT_Y-1:
			for light_type in range(PhysicalConstants.LightType.Count):
				internal_energy_variation += light_energy_by_direction[PhysicalConstants.LightDirection.Down][light_type]

		for light_direction in range(PhysicalConstants.LightDirection.Count):
			light_energy_by_direction[light_direction] = light_energy_by_direction_pending[light_direction]
			light_energy_by_direction_pending[light_direction] = [0.0,0.0,0.0]

		# apply light spherical distortion ratio
		for light_type in range(PhysicalConstants.LightType.Count):
			var transfered_energy = 0.0
			transfered_energy += light_energy_by_direction[PhysicalConstants.LightDirection.Left][light_type] * WORLD_BLOCK_SPHERICAL_COEF
			transfered_energy += light_energy_by_direction[PhysicalConstants.LightDirection.Right][light_type] * WORLD_BLOCK_SPHERICAL_COEF

			light_energy_by_direction[PhysicalConstants.LightDirection.Up][light_type] += transfered_energy
			light_energy_by_direction[PhysicalConstants.LightDirection.Left][light_type] *= 1 - WORLD_BLOCK_SPHERICAL_COEF
			light_energy_by_direction[PhysicalConstants.LightDirection.Right][light_type] *= 1 - WORLD_BLOCK_SPHERICAL_COEF




		for light_direction in range(PhysicalConstants.LightDirection.Count):

			var light_energy_direction = light_energy_by_direction[light_direction]

			# test absorbsion
			modified_light_by_direction.append([])
			light_variation_by_direction.append([])

			var modified_light_by_type = modified_light_by_direction[light_direction]

			for light_type in range(PhysicalConstants.LightType.Count):
				var light_energy = light_energy_direction[light_type]

				var modified_light_energy = light_energy * opacity[light_type]
				light_variation_by_direction[light_direction].append(-modified_light_energy)
				modified_light_by_direction[light_direction].append(modified_light_energy)

		for light_direction in range(PhysicalConstants.LightDirection.Count):
			var modified_light_by_type = modified_light_by_direction[light_direction]
			for light_type in range(PhysicalConstants.LightType.Count):
				var modified_light_energy = modified_light_by_type[light_type]

				var returned_energy = modified_light_energy * albedo[light_type]
				var diffused_energy = returned_energy * diffusion[light_type]
				var reflected_energy = returned_energy - diffused_energy

				light_variation_by_direction[(light_direction + 2) % 4][light_type] += reflected_energy
				light_variation_by_direction[(light_direction + 1) % 4][light_type] += diffused_energy * 0.5
				light_variation_by_direction[(light_direction + 3) % 4][light_type] += diffused_energy * 0.5

				var absorbed_energy = modified_light_energy - returned_energy
				internal_energy_variation += absorbed_energy



		# todo emissivity/ black body radiation
		var T2 = temperature*temperature
		var T4 = T2*T2
		var opaque_radiated_energy = WORLD_BLOCK_INTERFACE_SECTION * T4 * PhysicalConstants.stephan_boltzmann_constant * delta_time
		if opaque_radiated_energy > 0:
			var wavelenght_peak = PhysicalConstants.wien_dispacement_constant / temperature

			var wavelenght_area = [0.0,0.0,0.0]
			var one_over_total_wavelenght_area = 1 / (1.5 * wavelenght_peak)

			if wavelenght_peak > PhysicalConstants.infrared_wavelength_thresold:
				# peak in infrared
				wavelenght_area[PhysicalConstants.LightType.Infrared] = 1.5 * wavelenght_peak - 0.5 * PhysicalConstants.infrared_wavelength_thresold * PhysicalConstants.infrared_wavelength_thresold / wavelenght_peak
				wavelenght_area[PhysicalConstants.LightType.Ultraviolet] = 0.5 * PhysicalConstants.ultraviolet_wavelength_thresold * PhysicalConstants.ultraviolet_wavelength_thresold / wavelenght_peak
				wavelenght_area[PhysicalConstants.LightType.Visible] = 0.5 * PhysicalConstants.infrared_wavelength_thresold * PhysicalConstants.infrared_wavelength_thresold / wavelenght_peak - wavelenght_area[PhysicalConstants.LightType.Ultraviolet]
			elif wavelenght_peak * 3  < PhysicalConstants.ultraviolet_wavelength_thresold:
				# all in ultraviolet
				wavelenght_area[PhysicalConstants.LightType.Infrared] = 0
				wavelenght_area[PhysicalConstants.LightType.Visible] = 00
				wavelenght_area[PhysicalConstants.LightType.Ultraviolet] = 1.5 * wavelenght_peak
			elif wavelenght_peak < PhysicalConstants.ultraviolet_wavelength_thresold:
				# peak in ultraviolet
				var I_dist_to_3peak = 3 * wavelenght_peak - PhysicalConstants.infrared_wavelength_thresold
				var hI = 0.5 * I_dist_to_3peak / wavelenght_peak

				var V_dist_to_3peak = 3 * wavelenght_peak - PhysicalConstants.infrared_wavelength_thresold
				var hV = 0.5 * V_dist_to_3peak / wavelenght_peak

				wavelenght_area[PhysicalConstants.LightType.Infrared] = 0.5 * I_dist_to_3peak * hI
				wavelenght_area[PhysicalConstants.LightType.Visible] = 0.5 * V_dist_to_3peak * hV - wavelenght_area[PhysicalConstants.LightType.Infrared]
				wavelenght_area[PhysicalConstants.LightType.Ultraviolet] = 1.5 * wavelenght_peak -  0.5 * V_dist_to_3peak * hV
			else:
				# peak in visible
				var I_dist_to_3peak = 3 * wavelenght_peak - PhysicalConstants.infrared_wavelength_thresold
				var hI = 0.5 * I_dist_to_3peak / wavelenght_peak

				wavelenght_area[PhysicalConstants.LightType.Infrared] = 0.5 * I_dist_to_3peak * hI
				wavelenght_area[PhysicalConstants.LightType.Ultraviolet] = 0.5 * PhysicalConstants.ultraviolet_wavelength_thresold * PhysicalConstants.ultraviolet_wavelength_thresold / wavelenght_peak
				wavelenght_area[PhysicalConstants.LightType.Visible] = 1.5 * wavelenght_peak - wavelenght_area[PhysicalConstants.LightType.Infrared] - wavelenght_area[PhysicalConstants.LightType.Ultraviolet]

			var wavelenght_ratio = [wavelenght_area[0] * one_over_total_wavelenght_area, wavelenght_area[1] * one_over_total_wavelenght_area, wavelenght_area[2] * one_over_total_wavelenght_area]



			for light_type in range(PhysicalConstants.LightType.Count):
				var radiated_energy =  opaque_radiated_energy * wavelenght_ratio[light_type] * opacity[light_type]
				internal_energy_variation += -4 * radiated_energy
				for light_direction in range(PhysicalConstants.LightDirection.Count):
					light_variation_by_direction[light_direction][light_type] += radiated_energy

		add_internal_energy(internal_energy_variation)

		# Apply light variation
		for light_direction in range(PhysicalConstants.LightDirection.Count):
			var light_variation_for_direction = light_variation_by_direction[light_direction]
			var light_energy_direction = light_energy_by_direction[light_direction]

			for light_type in range(PhysicalConstants.LightType.Count):
				light_energy_direction[light_type] += light_variation_for_direction[light_type]

	func integrate_pending():
		input_internal_energy += pending_input_internal_energy

		for gas in range(PhysicalConstants.Gas.Count):
			input_gas_composition_N[gas] += pending_input_gas_composition_N[gas]
			pending_input_gas_composition_N[gas] = 0

		pending_input_internal_energy = 0
		input_gas_composition_N

		if pending_kinetic_energy.size() > 0:
			var kinetic_energy_vector = kinetic_energy * velocity_direction
			var total_kinetic_energy = kinetic_energy
			for kinetic_energy_addition in pending_kinetic_energy:
				var impulse_vector = kinetic_energy_addition[0] * kinetic_energy_addition[1]
				total_kinetic_energy += kinetic_energy_addition[0]
				kinetic_energy_vector += impulse_vector

			var new_kinetic_energy = kinetic_energy_vector.length()
			velocity_direction = kinetic_energy_vector.normalized()

			var kinetic_energy_diff = total_kinetic_energy - new_kinetic_energy
			input_internal_energy += kinetic_energy_diff
			pending_kinetic_energy = []

	func add_internal_energy(energy : float):
		var internal_energy = input_internal_energy + output_internal_energy

		var new_internal_energy = internal_energy + energy

		if internal_energy == 0:
			input_internal_energy += new_internal_energy
		else:
			var new_energy_ratio = new_internal_energy / internal_energy
			input_internal_energy *= new_energy_ratio
			output_internal_energy *= new_energy_ratio

	func give_kinetic_energy(energy, direction):
		pending_kinetic_energy.append([energy, direction])

	func draw_velocity_debug(canvas : CanvasItem, pos : Vector2):
		var center_pos = pos + Vector2(WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE) * 0.5
		var velocity_pos = center_pos + velocity_cache * RENDER_DEBUG_VELOCITY_SCALE

		var velocity_angle = velocity_cache.angle()

		var red   = 0.5*(1+sin(velocity_angle));
		var green = 0.5*(1+sin(velocity_angle + 2*PI / 3.0)); # + 60°
		var blue  = 0.5*(1+sin(velocity_angle + 4*PI / 3.0)); # + 120°

		var color = Color(red, green, blue)

		canvas.draw_line(RENDER_DEBUG_OFFSET + center_pos * RENDER_DEBUG_SCALE, RENDER_DEBUG_OFFSET + velocity_pos * RENDER_DEBUG_SCALE, color, 2)


	var debug_light_directions = [ Vector2(1, 0), Vector2(0, 1), Vector2(-1, 0), Vector2(0, -1) ]
	var debug_light_type_color = [ Color(1.0, 0.0, 0.0, 0.3), Color(0.0, 1.0, 0.0, 0.3), Color(0.0, 0.0, 1.0, 0.3) ]

	func draw_light_direction_debug(canvas : CanvasItem, pos : Vector2, direction : Vector2, light_energy : Array):

		var ortho_dir = direction.rotated(PI/2)

		#var base_start = pos + ortho_dir * 300 + direction * 100
		#var end_start = base_start + ortho_dir * 200
		#canvas.draw_line(RENDER_DEBUG_OFFSET + base_start * RENDER_DEBUG_SCALE, RENDER_DEBUG_OFFSET + end_start * RENDER_DEBUG_SCALE, Color(1.0,1.0,1.0), 1)


		for light_type in range(PhysicalConstants.LightType.Count):
			var energy = light_energy[light_type]
			var start_pos = pos + ortho_dir * (300 + light_type * 100) + direction * 100
			var end_pos = start_pos + direction * 0.005 * energy / WORLD_BLOCK_INTERFACE_SECTION
			var color = debug_light_type_color[light_type]

			canvas.draw_line(RENDER_DEBUG_OFFSET + start_pos * RENDER_DEBUG_SCALE, RENDER_DEBUG_OFFSET + end_pos * RENDER_DEBUG_SCALE, color, 1)

	func draw_light_debug(canvas : CanvasItem, pos : Vector2):
		return

		var pos_positions = []
		var center_pos = pos + Vector2(WORLD_BLOCK_SIZE * 0.5, WORLD_BLOCK_SIZE * 0.5)
#		pos_positions.append(center_pos + Vector2(WORLD_BLOCK_SIZE * 0.2, 0))
#		pos_positions.append(center_pos + Vector2(0, WORLD_BLOCK_SIZE * 0.2))
#		pos_positions.append(center_pos + Vector2(0, -WORLD_BLOCK_SIZE * 0.2))
#		pos_positions.append(center_pos + Vector2(-WORLD_BLOCK_SIZE * 0.2, 0))

		for light_direction in range(PhysicalConstants.LightDirection.Count):
			draw_light_direction_debug(canvas, center_pos, debug_light_directions[light_direction], light_energy_by_direction[light_direction])



	var min_temperature = 0
	var max_temperature = 1500
	var temperature_range = max_temperature - min_temperature

	var temperature_buckets = [Color("#000000"),
		Color("#00015c"),
		Color("#9a009c"),
		Color("#f06500"),
		Color("#fec300"),
		Color("#ffffff")]
	var temperature_range_by_bucket = temperature_range / (temperature_buckets.size()-1)

	func temperature_pressure_to_color(temperature, pressure) -> Color:

		var color : Color

		var temperature_bucket_start_index = int((temperature - min_temperature) / temperature_range_by_bucket)
		if temperature_bucket_start_index >= temperature_buckets.size()-1:
			color = temperature_buckets[temperature_buckets.size()-1]
		else:
			var temperature_bucket_end_index = temperature_bucket_start_index + 1
			var temperature_in_bucket = temperature - min_temperature - temperature_bucket_start_index * temperature_range_by_bucket
			var temperature_lerp_alpha = temperature_in_bucket / temperature_range_by_bucket
			color = temperature_buckets[temperature_bucket_start_index].linear_interpolate(temperature_buckets[temperature_bucket_end_index], temperature_lerp_alpha)

		var pressure_opacity = clamp(pressure / (50000*1.0), 0.0, 1.0)
		#var temperature_red = clamp(temperature / 600.0, 0.0, 1.0)
		#var temperature_rb = clamp((temperature - 600) / 1000.0, 0.0, 1.0)

		color.a = pressure_opacity

		 #var color = Color(temperature_red, temperature_rb, temperature_rb, pressure_opacity)
		return color


	func draw_bilinear_sub_tile(canvas : CanvasItem,pos : Vector2, temperatures, pressures):


		var points = PoolVector2Array()
		points.append(RENDER_DEBUG_OFFSET + (pos + Vector2(0,0))  * RENDER_DEBUG_SCALE)
		points.append(RENDER_DEBUG_OFFSET + (pos + Vector2(WORLD_BLOCK_SIZE,0)* 0.5)  * RENDER_DEBUG_SCALE)
		points.append(RENDER_DEBUG_OFFSET + (pos + Vector2(WORLD_BLOCK_SIZE,WORLD_BLOCK_SIZE)* 0.5)  * RENDER_DEBUG_SCALE)
		points.append(RENDER_DEBUG_OFFSET + (pos + Vector2(0,WORLD_BLOCK_SIZE)* 0.5)  * RENDER_DEBUG_SCALE)

		var colors = PoolColorArray()
		for i in range(4):
			colors.append(temperature_pressure_to_color(temperatures[i], pressures[i]))

		canvas.draw_polygon(points, colors)



	func draw_debug_tile(canvas : CanvasItem, pos : Vector2):

		var top_pressure = get_pressure_at_relative_altitude(-WORLD_BLOCK_SIZE*0.5)
		var bottom_pressure = get_pressure_at_relative_altitude(WORLD_BLOCK_SIZE*0.5)

		var tl_temperature = (temperature + tl_render_block.temperature + t_render_block.temperature + l_render_block.temperature) / 4
		var tr_temperature = (temperature + tr_render_block.temperature + t_render_block.temperature + r_render_block.temperature) / 4
		var br_temperature = (temperature + br_render_block.temperature + b_render_block.temperature + r_render_block.temperature) / 4
		var bl_temperature = (temperature + bl_render_block.temperature + b_render_block.temperature + l_render_block.temperature) / 4

		var l_temperature = (temperature + l_render_block.temperature) / 2
		var r_temperature = (temperature + r_render_block.temperature) / 2
		var t_temperature = (temperature + t_render_block.temperature) / 2
		var b_temperature = (temperature + b_render_block.temperature) / 2

		var tl_pressure = (pressure + tl_render_block.get_pressure_at_relative_altitude(WORLD_BLOCK_SIZE*0.5) + t_render_block.get_pressure_at_relative_altitude(WORLD_BLOCK_SIZE*0.5) + l_render_block.get_pressure_at_relative_altitude(-WORLD_BLOCK_SIZE*0.5)) / 4
		var tr_pressure = (pressure + tr_render_block.get_pressure_at_relative_altitude(WORLD_BLOCK_SIZE*0.5) + t_render_block.get_pressure_at_relative_altitude(WORLD_BLOCK_SIZE*0.5) + r_render_block.get_pressure_at_relative_altitude(-WORLD_BLOCK_SIZE*0.5)) / 4
		var br_pressure = (pressure + br_render_block.get_pressure_at_relative_altitude(-WORLD_BLOCK_SIZE*0.5) + b_render_block.get_pressure_at_relative_altitude(-WORLD_BLOCK_SIZE*0.5) + r_render_block.get_pressure_at_relative_altitude(WORLD_BLOCK_SIZE*0.5)) / 4
		var bl_pressure = (pressure + bl_render_block.get_pressure_at_relative_altitude(-WORLD_BLOCK_SIZE*0.5) + b_render_block.get_pressure_at_relative_altitude(-WORLD_BLOCK_SIZE*0.5) + l_render_block.get_pressure_at_relative_altitude(WORLD_BLOCK_SIZE*0.5)) / 4

		var l_pressure = (pressure + l_render_block.pressure) / 2
		var r_pressure = (pressure + r_render_block.pressure) / 2
		var t_pressure = (pressure + t_render_block.pressure) / 2
		var b_pressure = (pressure + b_render_block.pressure) / 2


		draw_bilinear_sub_tile(canvas, pos, [tl_temperature, t_temperature, temperature, l_temperature], [tl_pressure, t_pressure, pressure, l_pressure])
		draw_bilinear_sub_tile(canvas, pos + Vector2(WORLD_BLOCK_SIZE*0.5, 0), [t_temperature, tr_temperature, r_temperature, temperature], [t_pressure, tr_pressure, r_pressure, pressure])
		draw_bilinear_sub_tile(canvas, pos + Vector2(WORLD_BLOCK_SIZE*0.5, WORLD_BLOCK_SIZE*0.5), [temperature, r_temperature, br_temperature, b_temperature], [pressure, r_pressure, br_pressure, b_pressure])
		draw_bilinear_sub_tile(canvas, pos + Vector2(0, WORLD_BLOCK_SIZE*0.5), [l_temperature, temperature, b_temperature, bl_temperature], [l_pressure, pressure, b_pressure, br_pressure])


#		var top_pressure_opacity = clamp(top_pressure / (100000*1.0), 0.0, 1.0)
#		var bottom_pressure_opacity = clamp(bottom_pressure / (100000*1.0), 0.0, 1.0)
#		var tl_temperature_red = clamp(tl_temperature / 300.0, 0.0, 1.0)
#		var tr_temperature_red = clamp(tr_temperature / 300.0, 0.0, 1.0)
#		var br_temperature_red = clamp(br_temperature / 300.0, 0.0, 1.0)
#		var bl_temperature_red = clamp(bl_temperature / 300.0, 0.0, 1.0)
#
#		var tl_color = Color(tl_temperature_red, 0.5, 1.0, top_pressure_opacity)
#		var tr_color = Color(tr_temperature_red, 0.5, 1.0, top_pressure_opacity)
#		var br_color = Color(br_temperature_red, 0.5, 1.0, bottom_pressure_opacity)
#		var bl_color = Color(bl_temperature_red, 0.5, 1.0, bottom_pressure_opacity)




		#canvas.draw_rect(Rect2(RENDER_DEBUG_OFFSET + pos * RENDER_DEBUG_SCALE, Vector2(WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE) * RENDER_DEBUG_SCALE), color, true)

		#var temp_text = String.format("%10.3f" % temperature)

		var temp_text = "%10.1f" % temperature
		#var pressure_text = "%10.1f" % ((pressure-100000))
		var pressure_text = "%10.2f" % (pressure/100000)




		#canvas.draw_string(canvas.font, RENDER_DEBUG_OFFSET + pos * RENDER_DEBUG_SCALE + Vector2(-20, 10), temp_text, Color(1.0,1.0,1.0,1.0))
		#canvas.draw_string(canvas.font, RENDER_DEBUG_OFFSET + pos * RENDER_DEBUG_SCALE + Vector2(-20, 25), pressure_text, Color(1.0,1.0,1.0,1.0))

#		if block_x == DEBUG_BLOCK_X and block_y == DEBUG_BLOCK_Y:
#			canvas.draw_rect(Rect2(RENDER_DEBUG_OFFSET + pos * RENDER_DEBUG_SCALE, Vector2(WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE) * RENDER_DEBUG_SCALE), Color(1.0,1.0,1.0), false)
	func draw_tile_highlight(canvas : CanvasItem, pos : Vector2):
		#if block_x == DEBUG_BLOCK_X and block_y == DEBUG_BLOCK_Y:
		canvas.draw_rect(Rect2(RENDER_DEBUG_OFFSET + pos * RENDER_DEBUG_SCALE, Vector2(WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE) * RENDER_DEBUG_SCALE), Color(1.0,0.0,1.0), false)

	func get_pressure_at_relative_altitude(relative_altitude : float):
		return pressure + pressure_gradient * relative_altitude

class AtmosphericBlock extends Block:

	func init():
		gas_volume = WORLD_BLOCK_DEPTH * WORLD_BLOCK_SIZE * WORLD_BLOCK_SIZE

		is_opaque = false

		opacity = [0.01, 0.005, 0.01]
		albedo = [0.5, 0.2, 0.01]
		diffusion = [0.7, 0.7, 0.7]

		#init_with_gas(PhysicalConstants.Gas.Nitrogen, 274, 1e5)
		init_with_gas(PhysicalConstants.Gas.Nitrogen, 200, 0.13e5)

	func draw(canvas : CanvasItem, pos : Vector2):
		draw_debug_tile(canvas, pos)

	func draw_debug(canvas : CanvasItem, pos : Vector2):
		draw_velocity_debug(canvas, pos)

		draw_light_debug(canvas, pos)

	func velocity_dumping(new_velocity : Vector2) -> Vector2:
		return new_velocity


class GroundBlock extends Block:
	func init():
		gas_volume = WORLD_BLOCK_DEPTH * WORLD_BLOCK_SIZE * WORLD_BLOCK_SIZE
		is_opaque = true
		opacity = [1.0, 1.0, 1.0]
		albedo = [0.0, 0.0, 0.0]
		diffusion = [0.0, 0.0, 0.0]

		init_with_gas(PhysicalConstants.Gas.Nitrogen, 200, 0.13e5)
		# TODO proportionnal

	func draw(canvas : CanvasItem, pos : Vector2):
		draw_debug_tile(canvas, pos)
		#canvas.draw_rect(Rect2(RENDER_DEBUG_OFFSET + pos * RENDER_DEBUG_SCALE, Vector2(WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE) * RENDER_DEBUG_SCALE), Color(1.0, 0.5, 0.5), true)
	func draw_debug(canvas : CanvasItem, pos : Vector2):
		draw_velocity_debug(canvas, pos)

		draw_light_debug(canvas, pos)

	func velocity_dumping(new_velocity : Vector2) -> Vector2:
#		if block_y == WORLD_BLOCK_COUNT_Y - 1 and new_velocity.y > 0:
#			return Vector2(new_velocity.x, 0)
#		else:
#			return new_velocity

		return Vector2()



class SurfaceBlock extends Block:

	var surface_local_left_altitude
	var surface_local_right_altitude
	var surface_local_normal

	func init():
		gas_volume = WORLD_BLOCK_DEPTH * WORLD_BLOCK_SIZE * WORLD_BLOCK_SIZE
		is_opaque = false
		opacity = [1.0, 1.0, 1.0]
		albedo = [0.1, 0.5, 0.01]
		diffusion = [0.5, 0.5, 0.5]

		surface_local_normal = Vector2(WORLD_BLOCK_SIZE, surface_local_right_altitude-surface_local_left_altitude).rotated(-PI/2.0).normalized()

		init_with_gas(PhysicalConstants.Gas.Nitrogen, 200, 0.13e5)
		# TODO depend on ground composition

	func draw(canvas : CanvasItem, pos : Vector2):
		#canvas.draw_rect(Rect2(RENDER_DEBUG_OFFSET + pos * RENDER_DEBUG_SCALE, Vector2(WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE) * RENDER_DEBUG_SCALE), Color(0.5, 1.0, 0.5), true)
		draw_debug_tile(canvas, pos)
		var left_pos = pos + Vector2(0, surface_local_left_altitude)
		var right_pos = pos + Vector2(WORLD_BLOCK_SIZE, surface_local_right_altitude)
		canvas.draw_line(RENDER_DEBUG_OFFSET + left_pos * RENDER_DEBUG_SCALE, RENDER_DEBUG_OFFSET + right_pos * RENDER_DEBUG_SCALE, Color(0.3,0.6,0.3), 2)

		#canvas.draw_rect(Rect2(RENDER_DEBUG_OFFSET + pos * RENDER_DEBUG_SCALE, Vector2(WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE) * RENDER_DEBUG_SCALE), Color(0.0,1.0,1.0), false)
		#canvas.draw_rect(Rect2(RENDER_DEBUG_OFFSET + left_pos * RENDER_DEBUG_SCALE, Vector2(WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE) * RENDER_DEBUG_SCALE), Color(1.0,0.0,0.0), false)
	func draw_debug(canvas : CanvasItem, pos : Vector2):

		draw_velocity_debug(canvas, pos)
		draw_light_debug(canvas, pos)


	func velocity_dumping(new_velocity : Vector2) -> Vector2:
#		if block_y == WORLD_BLOCK_COUNT_Y - 1 and new_velocity.y > 0:
#			return Vector2(new_velocity.x, 0)
#		else:
#			return new_velocity

		if new_velocity.dot(surface_local_normal) < 0:
			var det = new_velocity.cross(surface_local_normal)
			var dumped_velocity
			if det > 0:
				dumped_velocity = surface_local_normal.rotated(-PI / 2) * new_velocity.length()
			elif det < 0:
				dumped_velocity = surface_local_normal.rotated(PI / 2) * new_velocity.length()
			else:
				dumped_velocity = Vector2()
			dumped_velocity = dumped_velocity
			return dumped_velocity
		else:
			return new_velocity



var font

func _ready():
	print("init world model")
	#rng.randomize()

	#for i in range(WORLD_BLOCK_COUNT):
	#	blocks.append(Block.new())
	blocks.resize(WORLD_BLOCK_COUNT)

	var label = Label.new()
	font = label.get_font("")

	#font = load("res://fonts/DebugFont.tres")

#	font = DynamicFont.new()
#	font.font_data = load("res://fonts/RobotoMono-Regular.ttf")
#	font.size = 8

	var altitude = WORLD_SIZE_Y - 3500.0
	var sloop_rate = 0

	for x in range (WORLD_BLOCK_COUNT_X):
		sloop_rate += rng.randfn(0.0, 0.1) * WORLD_BLOCK_SIZE * 1

		sloop_rate = clamp(sloop_rate, -2 * WORLD_BLOCK_SIZE, 2 * WORLD_BLOCK_SIZE)
		var last_altitude = altitude
		altitude += sloop_rate

		if altitude < 200 or altitude > WORLD_BLOCK_COUNT_Y * WORLD_BLOCK_SIZE - 200:
			altitude = clamp(altitude, 200, WORLD_BLOCK_COUNT_Y * WORLD_BLOCK_SIZE - 200)
			sloop_rate /= 2

		var max_atm_block = int(altitude / WORLD_BLOCK_SIZE)

		for y in range(max_atm_block):
			var block = AtmosphericBlock.new()

			blocks[x + y  * WORLD_BLOCK_COUNT_X] = block

		for y in range(max_atm_block+1, WORLD_BLOCK_COUNT_Y):
			var block = GroundBlock.new()
			blocks[x + y  * WORLD_BLOCK_COUNT_X] = block

		var surface_block = SurfaceBlock.new()
		surface_block.surface_local_left_altitude = last_altitude - max_atm_block * WORLD_BLOCK_SIZE
		surface_block.surface_local_right_altitude = altitude - max_atm_block * WORLD_BLOCK_SIZE
		blocks[x + max_atm_block  * WORLD_BLOCK_COUNT_X] = surface_block

	for i in range (WORLD_BLOCK_COUNT):
		var block = blocks[i]
		block.init()
		block.update_cache()

	for x in range (WORLD_BLOCK_COUNT_X):
		for y in range(WORLD_BLOCK_COUNT_Y):
			var block = blocks[x + y  * WORLD_BLOCK_COUNT_X]
			block.block_x = x
			block.block_y = y

			var neighbour_block
			neighbour_block = get_block_at_with_wrapping(x-1, y-1)
			block.tl_render_block = neighbour_block if neighbour_block != null else block
			neighbour_block = get_block_at_with_wrapping(x+1, y-1)
			block.tr_render_block = neighbour_block if neighbour_block != null else block
			neighbour_block = get_block_at_with_wrapping(x+1, y+1)
			block.br_render_block = neighbour_block if neighbour_block != null else block
			neighbour_block = get_block_at_with_wrapping(x-1, y+1)
			block.bl_render_block = neighbour_block if neighbour_block != null else block
			neighbour_block = get_block_at_with_wrapping(x-1, y)
			block.l_render_block = neighbour_block if neighbour_block != null else block
			neighbour_block = get_block_at_with_wrapping(x+1, y)
			block.r_render_block = neighbour_block if neighbour_block != null else block
			neighbour_block = get_block_at_with_wrapping(x, y-1)
			block.t_render_block = neighbour_block if neighbour_block != null else block
			neighbour_block = get_block_at_with_wrapping(x, y+1)
			block.b_render_block = neighbour_block if neighbour_block != null else block


	print_world_state()


	print("init world done")

	world_processing_thread = Thread.new()
	world_processing_thread_running = true
	#world_processing_thread.start(self, "process_world")

func _exit_tree():
	world_processing_thread_running = false
	#world_processing_thread.wait_to_finish()

func _draw():
	for x in range (WORLD_BLOCK_COUNT_X):
		for y in range(WORLD_BLOCK_COUNT_Y):
			var block = blocks[x + y  * WORLD_BLOCK_COUNT_X]
			block.draw(self, Vector2(x * WORLD_BLOCK_SIZE, y * WORLD_BLOCK_SIZE))
	for x in range (WORLD_BLOCK_COUNT_X):
		for y in range(WORLD_BLOCK_COUNT_Y):
			var block = blocks[x + y  * WORLD_BLOCK_COUNT_X]
			block.draw_debug(self, Vector2(x * WORLD_BLOCK_SIZE, y * WORLD_BLOCK_SIZE))
			if x == DEBUG_BLOCK_X and y == DEBUG_BLOCK_Y:
				block.draw_tile_highlight(self, Vector2(x * WORLD_BLOCK_SIZE, y * WORLD_BLOCK_SIZE))


var paused  = true

func print_world_state():
	var total_internal_energy = 0
	var total_kinetic_energy = 0
	var total_elastic_energy = 0
	var total_thermal_energy = 0

	var total_gas_N = []
	for gas in range(PhysicalConstants.Gas.Count):
		total_gas_N.append(0)

	for block in blocks:
		total_internal_energy += block.input_internal_energy + block.output_internal_energy
		total_thermal_energy += block.input_thermal_energy + block.output_thermal_energy
		total_elastic_energy += block.input_elastic_energy + block.output_elastic_energy
		total_kinetic_energy += block.kinetic_energy
		for gas in range(PhysicalConstants.Gas.Count):
			total_gas_N[gas] += block.input_gas_composition_N[gas] + block.output_gas_composition_N[gas]

	var total_energy = total_internal_energy + total_kinetic_energy

	print("====")
	print("total_internal_energy %s TJ" % (total_internal_energy / 1e9))
	print("total_kinetic_energy %s MJ" % (total_kinetic_energy/ 1e6))
	print("total_thermal_energy %s TJ" % (total_thermal_energy/ 1e9))
	print("total_elastic_energy %s TJ" % (total_elastic_energy/ 1e9))
	print("total_gas N:")
	for gas in range(PhysicalConstants.Gas.Count):
		print("- %s Mmol" % (total_gas_N[gas] / 1e6) )
	print("total_energy %s TJ" % (total_energy/ 1e9))

var world_step_speed = 0.5

func _process(delta):

	if Input.is_action_just_pressed("world_debug_toogle_sun"):
		world_enable_sun = !world_enable_sun
		print("world_enable_sun=", world_enable_sun)

	if Input.is_action_just_pressed("world_debug_increase_simulation_speed"):
		world_step_speed *= 1.2
		print("world_step_speed=", world_step_speed)

	if Input.is_action_just_pressed("world_debug_decrease_simulation_speed"):
		world_step_speed /= 1.2
		print("world_step_speed=", world_step_speed)

	if Input.is_action_just_pressed("world_toogle_pause"):
		paused = !paused
		print("paused=", paused)

	if not paused or Input.is_action_just_pressed("world_step"):
		step_world(world_step_speed)
		update()

	if Input.is_action_just_pressed("world_toogle_pause") or Input.is_action_just_pressed("world_step"):
		print_world_state()

# Called every frame. 'delta' is the elapsed time since the previous frame.
func process_world(userdata):
	print("process_world")
	#var plop = 0
	#var plip = 2 / plop
	while world_processing_thread_running:
		 step_world(0.1)

func step_world(delta_time):

	var first_block = blocks[0]
	#print("step world model")

	# compute pressure again ?

	#for x in ran++--ge (WORLD_BLOCK_COUNT_X):
	#	for y in range (WORLD_BLOCK_COUNT_Y):
	#		compute_gas_block_movement(delta_time, x, y, get_block_at(x, y))

	for x in range (WORLD_BLOCK_COUNT_X):
		for y in range (WORLD_BLOCK_COUNT_Y - 1):
			var current_block = get_block_at_with_wrapping(x, y)
			var right_block = get_block_at_with_wrapping(x+1, y)
			var down_block = get_block_at_with_wrapping(x, y+1)

			compute_gas_transition(delta_time, current_block, right_block, 1, 0)
			compute_gas_transition(delta_time, current_block, down_block, 0, 1)

			compute_light_transition(delta_time, current_block, right_block, 1, 0)
			compute_light_transition(delta_time, current_block, down_block, 0, 1)

	for x in range (WORLD_BLOCK_COUNT_X):
		var current_block = get_block_at_with_wrapping(x, WORLD_BLOCK_COUNT_Y - 1)
		var right_block = get_block_at_with_wrapping(x+1, WORLD_BLOCK_COUNT_Y - 1)
		compute_gas_transition(delta_time, current_block, right_block, 1, 0)
		compute_light_transition(delta_time, current_block, right_block, 1, 0)


	for i in range (WORLD_BLOCK_COUNT):
		blocks[i].apply_gas_movement()
		blocks[i].apply_light_movement(delta_time)



# TODO diffusion, decantation

	# apply debug heat
	if world_enable_sun:
		for x in range (WORLD_BLOCK_COUNT_X):
			var heat_alpha = sin( PI * float(x) / WORLD_BLOCK_COUNT_X)
			#heat_alpha = 1 + heat_alpha * 0.001
			#heat_alpha = 1
			var surface = WORLD_BLOCK_INTERFACE_SECTION
			var power = heat_alpha * surface * 1361 * 1000
			var energy = delta_time * power
			var block = get_block_at(x, 0)

			block.add_light(PhysicalConstants.LightDirection.Down, PhysicalConstants.LightType.Infrared, 0.1 * energy)
			block.add_light(PhysicalConstants.LightDirection.Down, PhysicalConstants.LightType.Visible, 0.8 * energy)
			block.add_light(PhysicalConstants.LightDirection.Down, PhysicalConstants.LightType.Ultraviolet, 0.1 * energy)

	var max_velocity_block_id
	var max_velocity_sq = -1

	for i in range (WORLD_BLOCK_COUNT):
		var block : Block = blocks[i]
		block.integrate_pending()
		block.update_cache()
		if block.velocity_cache.length_squared() > max_velocity_sq:
			max_velocity_sq = block.velocity_cache.length_squared()
			max_velocity_block_id = i


	print("max vel="+str(sqrt(max_velocity_sq)))
	DEBUG_BLOCK_X = max_velocity_block_id % WORLD_BLOCK_COUNT_X
	DEBUG_BLOCK_Y = max_velocity_block_id / WORLD_BLOCK_COUNT_X


	#var debug_block = get_block_at(DEBUG_BLOCK_X, DEBUG_BLOCK_Y)
	#print("debug_block vel = ", str(debug_block.velocity_cache))



	# export molecules based one tempature -> velocity repartition -> movement on side
	# https://fr.wikipedia.org/wiki/Loi_de_distribution_des_vitesses_de_Maxwell
	# elastique energy

func get_relative_force(block_x : int, block_y : int, dx : int, dy: int, dir_and_coef : Vector2, local_block : Block):
	var neightbourg_block : Block = get_block_at_with_wrapping(block_x + dx, block_y + dy)
	if not neightbourg_block:
		return Vector2()

	var local_pressure_at_altitude = local_block.pressure + local_block.pressure_gradient * dy * WORLD_BLOCK_SIZE *0.5
	var neightbourg_pressure_at_altitude = neightbourg_block.pressure - neightbourg_block.pressure_gradient * dy * WORLD_BLOCK_SIZE *0.5

	var delta_pressure = neightbourg_pressure_at_altitude - local_pressure_at_altitude # Pa
	var force = delta_pressure * WORLD_BLOCK_INTERFACE_SECTION  # Pa * m2 = N
	var directional_force = force * dir_and_coef
	return directional_force

func compute_gas_transition(delta_time : float, block_a : Block, block_b : Block, dx : int, dy : int):

	if block_a.block_x == DEBUG_BLOCK_X and block_a.block_y == DEBUG_BLOCK_Y:
		pass
	if block_b.block_x == DEBUG_BLOCK_X and block_b.block_y == DEBUG_BLOCK_Y:
		pass

	var delta_pressure = block_a.get_pressure_at_relative_altitude(dy * WORLD_BLOCK_SIZE * 0.5) - block_b.get_pressure_at_relative_altitude(-dy * WORLD_BLOCK_SIZE * 0.5)


	var transition_direction = Vector2(dx, dy)

	var transition_mass = 0.5 * (block_a.mass + block_b.mass)

	var transition_velocity = 0.5 * (block_a.velocity_cache * block_a.mass + block_b.velocity_cache * block_b.mass) / transition_mass

	var force = transition_direction * delta_pressure * WORLD_BLOCK_INTERFACE_SECTION # Pa * m2 = N

	var acceleration = force / transition_mass

	#if dy != 0:
	#	acceleration += Vector2(0, 9.81)

	var new_transition_velocity = transition_velocity + acceleration * delta_time


	var linear_transition_velocity = new_transition_velocity.dot(transition_direction)

	var linear_translation = linear_transition_velocity * delta_time
	var linear_translation_clamped = clamp(linear_translation,-WORLD_BLOCK_SIZE * 0.5, WORLD_BLOCK_SIZE * 0.5)
	var volume_displaced = linear_translation_clamped * WORLD_BLOCK_INTERFACE_SECTION

	transfert_block_volume(block_a, block_b, volume_displaced, dy * WORLD_BLOCK_SIZE)

	block_a.new_velocity_list.append([new_transition_velocity, block_a.mass * 0.5])
	block_b.new_velocity_list.append([new_transition_velocity, block_a.mass * 0.5])

func compute_light_transition(delta_time : float, block_a : Block, block_b : Block, dx : int, dy : int):
	# Light movement, maybe move
	if dx > 0:
		block_b.light_energy_by_direction_pending[PhysicalConstants.LightDirection.Right] = block_a.light_energy_by_direction[PhysicalConstants.LightDirection.Right]
		block_a.light_energy_by_direction_pending[PhysicalConstants.LightDirection.Left] = block_b.light_energy_by_direction[PhysicalConstants.LightDirection.Left]
	elif dy > 0:
		block_b.light_energy_by_direction_pending[PhysicalConstants.LightDirection.Down] = block_a.light_energy_by_direction[PhysicalConstants.LightDirection.Down]
		block_a.light_energy_by_direction_pending[PhysicalConstants.LightDirection.Up] = block_b.light_energy_by_direction[PhysicalConstants.LightDirection.Up]
	elif dx < 0:
		block_b.light_energy_by_direction_pending[PhysicalConstants.LightDirection.Left] = block_a.light_energy_by_direction[PhysicalConstants.LightDirection.Left]
		block_a.light_energy_by_direction_pending[PhysicalConstants.LightDirection.Right] = block_b.light_energy_by_direction[PhysicalConstants.LightDirection.Right]
	elif dy < 0:
		block_b.light_energy_by_direction_pending[PhysicalConstants.LightDirection.Up] = block_a.light_energy_by_direction[PhysicalConstants.LightDirection.Up]
		block_a.light_energy_by_direction_pending[PhysicalConstants.LightDirection.Down] = block_b.light_energy_by_direction[PhysicalConstants.LightDirection.Down]

func get_block_at(block_x : int, block_y : int):
	assert(block_x > -1)
	assert(block_y > -1)
	assert(block_x < WORLD_BLOCK_COUNT_X)
	assert(block_y < WORLD_BLOCK_COUNT_Y)

	return blocks[block_x + block_y * WORLD_BLOCK_COUNT_X]

func get_block_at_with_wrapping(block_x : int, block_y : int):
	if block_y < 0 or block_y >= WORLD_BLOCK_COUNT_Y:
		return null

	if block_x < 0:
		block_x += WORLD_BLOCK_COUNT_X
	elif block_x >= WORLD_BLOCK_COUNT_X:
		 block_x -= WORLD_BLOCK_COUNT_X


	return get_block_at(block_x, block_y)

func transfert_block_volume(block_a, block_b, signed_volume, delta_height):

	var input_block
	var output_block
	var volume = abs(signed_volume)

	if volume == 0:
		return

	if signed_volume > 0:
		input_block = block_a
		output_block = block_b
	else:
		input_block = block_b
		output_block = block_a

	input_block.add_give_volume_order(output_block, volume, sign(signed_volume) * delta_height)




