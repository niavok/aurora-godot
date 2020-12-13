extends Control

var pulse_ts = 0
var pulse_range = 1.0
var pulse_frequency = 1 # pulse per second
var pulse_phase = PI
var enable_pulse = true

var order_fade_out = false
var fade_out_duration : float
var fade_out_wait_delay : float
var fade_out_time : float

var root_control: Control
var loading_label : Label

func _ready():
	root_control = get_node("CanvasLayer/Control")
	loading_label = get_node("CanvasLayer/Control/LoadingLabel")
	update_text_color()

func _process(delta):

	if order_fade_out:
		if fade_out_time > fade_out_duration + fade_out_wait_delay:
			# fadeout end, mask
			visible = false
			order_fade_out = false
		elif fade_out_time > fade_out_wait_delay:
			var time_in_fade = fade_out_time - fade_out_wait_delay
			var fade_alpha = 1 - time_in_fade / fade_out_duration
			root_control.modulate = Color(1, 1, 1, fade_alpha)
		fade_out_time += delta

	root_control.visible = visible

	if !visible or not enable_pulse:
		return


	update_text_color()

	pulse_ts += delta
	pulse_ts = fmod(pulse_ts, 2 * PI * pulse_frequency * 2)

func update_text_color():
	var loading_text_alpha = (1 - pulse_range) + (cos(pulse_phase + pulse_ts * pulse_frequency * 2) + 1) * pulse_range * 0.5
	var new_color : Color = loading_label.get_color ("font_color")
	new_color.a = loading_text_alpha
	loading_label.add_color_override("font_color", new_color)

func activate(i_enable_pulse: bool) -> void:
	visible = true
	pulse_ts = 0
	enable_pulse = i_enable_pulse
	root_control.modulate = Color(1, 1, 1, 1)
	update_text_color()

func fade_out(wait_delay : float, duration : float) -> void:
	order_fade_out = true
	fade_out_duration = duration
	fade_out_wait_delay = wait_delay
	fade_out_time = 0
