import math
import statistics

# https://www.cairographics.org/samples/
from cairocffi import Context, ImageSurface, SurfacePattern, OPERATOR_HSL_SATURATION, OPERATOR_OVER, FONT_SLANT_NORMAL, FONT_WEIGHT_BOLD


def draw_rounded_rectangle(ctx, x, y, width, height, radius):
	degrees = math.pi / 180

	ctx.new_sub_path()
	ctx.arc(x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees)
	ctx.arc(x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees)
	ctx.arc(x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees)
	ctx.arc(x + radius, y + radius, radius, 180 * degrees, 270 * degrees)
	ctx.close_path()


## DATA

FONT_SIZE = 14
label_margin = (-15, 0)
label_pad = (10, 6)
label_radius = 4

LEFT = 0
RIGHT = 1

IN = 0
OUT = 1
POWER = 2
SYSTEM = 3

boards = {
	"F042F6": {
		"input": "tssop20.png",
		"pad_sides": 0.8,
		"y_offset": 0,
		"pin_defs": {
			"PA0": (55/367, 273/500, RIGHT),
			"PA1": (55/367, 313/500, RIGHT),
			"PA4": (55/367, 439/500, RIGHT),
			"PA6": (303/367, 397/500, LEFT),
			"PA7": (303/367, 358/500, LEFT),
			"GND": (303/367, 166/300, LEFT),
			"3v3": (303/367, 142/300, LEFT),
			"USB_DM": (303/367, 116/300, LEFT),
			"USB_DP": (303/367, 92/300, LEFT),
			"BOOT": (37/220, 44/300, RIGHT)
		},
		"variants": {
			"_base": [
				("PA4", OUT, "PWM A"),
				("PA7", OUT, "PWM B"),
				("BOOT", SYSTEM, "short to GND"),
				("USB_DM", SYSTEM, ""),
				("USB_DP", SYSTEM, ""),
				("GND", POWER, ""),
				("3v3", POWER, ""),
			],
			"pulse_count": [
				("PA0", IN, "signal input"),
			],
			"period": [
				("PA0", IN, "signal input"),
			],
			"pwm": [
				("PA0+PA1", IN, "signal input"),
			],
			"freq_ratio": [
				("PA0", IN, "input A"),
				("PA6", IN, "input B"),
			],
		}
	},
	"F042_Nucleo32": {
		"input": "nucleo-f042k6-medium.png",
		"pad_sides": 0.8,
		"y_offset": 0.3,
		"pin_defs": {
			"A0": (196/250, 362/500, LEFT),
			"A1": (196/250, 339/500, LEFT),
			"A3": (196/250, 291/500, LEFT),
			"A5": (196/250, 242/500, LEFT),
			"A6": (196/250, 221/500, LEFT),
		},
		"variants": {
			"_base": [
				("A3", OUT, "PWM A"),
				("A6", OUT, "PWM B"),
			],
			"pulse_count": [
				("A0", IN, "signal input"),
			],
			"period": [
				("A0", IN, "signal input"),
			],
			"pwm": [
				("A0+A1", IN, "signal input"),
			],
			"freq_ratio": [
				("A0", IN, "input A"),
				("A5", IN, "input B"),
			],
		}
	},
	"F303_Nucleo64": {
		"input": "nucleo-f303re-medium.png",
		"pad_sides": 0.3,
		"y_offset": 300/1000,
		"pin_defs": {
			"A0": (173/872, 772/1000, RIGHT),
			"A1": (173/872, 803/1000, RIGHT),
			"D5": (706/872, 773/1000, LEFT),
			"D11": (706/872, 589/1000, LEFT),
			"D12": (706/872, 558/1000, LEFT),
		},
		"variants": {
			"_base": [
				("D11", OUT, "PWM B"),
				("D12", OUT, "PWM A"),
			],
			"pulse_count": [
				("A0", IN, "signal input"),
			],
			"period": [
				("A0", IN, "signal input"),
			],
			"pwm": [
				("A0+A1", IN, "signal input"),
			],
			"freq_ratio": [
				("A0", IN, "input A"),
				("D5", IN, "input B"),
			],
		}
	},
}

## CODE

for board_name, board_info in boards.items():
	pad_sides = board_info["pad_sides"]
	pin_defs = board_info["pin_defs"]
	y_offset = board_info["y_offset"]

	bg = ImageSurface.create_from_png(board_info["input"])
	w, h_bg = bg.get_width(), bg.get_height()
	h = int(h_bg * (1 - y_offset))

	base_labels = board_info["variants"]["_base"]

	for variant_name, variant_info in board_info["variants"].items():
		if variant_name.startswith("_"):
			# skip "_base"
			continue

		# create canvas
		w_p = int(w * (1 + 2 * pad_sides))
		canvas = ImageSurface(bg.get_format(), w_p, h)
		ctx = Context(canvas)

		ctx.set_source_rgb(1, 1, 1)
		ctx.rectangle(0, 0, w_p, h)
		ctx.fill()

		ctx.set_source_surface(bg, (w_p - w) // 2, -h_bg * y_offset)
		ctx.rectangle((w_p - w) // 2, 0, w, h_bg)
		ctx.fill()

		# make B&W

		ctx.set_source_rgb(0.5, 0.5, 0.5)
		ctx.set_operator(OPERATOR_HSL_SATURATION)
		ctx.rectangle(0, 0, w_p, h)
		ctx.fill()

		if True:
			canvas.write_to_png(f"{board_name}_bg.png")
			canvas = None

			from PIL import Image
			Image.open(f"{board_name}_bg.png").save(f"{board_name}_bg.jpg")

			canvas = ImageSurface(bg.get_format(), w_p, h)
			ctx = Context(canvas)

		# add labels
		ctx.set_operator(OPERATOR_OVER)

		ctx.select_font_face("Roboto", FONT_SLANT_NORMAL, FONT_WEIGHT_BOLD)
		ctx.set_font_size(FONT_SIZE)

		labels = base_labels + variant_info

		for pass_ in range(2):
			for pin_list, type, note in labels:
				text = f"{pin_list}: {note}" if note else pin_list

				pin_names = pin_list.split("+")

				# calculate label size
				(x_bearing, y_bearing, width, height, x_advance, y_advance) = ctx.text_extents(text)

				label_width = width + 2 * label_pad[0]
				label_height = height + 2 * label_pad[1]

				_, _, align = pin_defs[pin_names[0]]		# HACK

				if align == RIGHT:
					x = w * pad_sides - label_margin[0] - label_width
					line_origin_x = x + label_width
				else:
					x = w * pad_sides + w + label_margin[0]
					line_origin_x = x

				# https://lospec.com/palette-list/dawnbringer-16
				color_table = {
					IN: 	"30346d",
					OUT:	"d27d2c",
					POWER:	"d04648",
					SYSTEM:	"757161",
				}
				# # https://lospec.com/palette-list/soft-rainbow
				# color_table = {
				# 	IN: 	"257d8f",
				# 	OUT:	"a085ab",
				# 	POWER:	"c7453e",
				# 	SYSTEM:	"383431",
				# }

				r, g, b = color_table[type][0:2], color_table[type][2:4], color_table[type][4:6]
				color = (int(r, 16) / 255, int(g, 16) / 255, int(b, 16) / 255)

				mean_pin_y = statistics.mean([pin_defs[pin_name][1] for pin_name in pin_names])
				mean_pin_y = mean_pin_y * h_bg - h_bg * y_offset

				y = mean_pin_y - label_height // 2
				print(f"{text=}\t{y=}")

				if pass_ == 0:
					ctx.set_source_rgba(*color, 1)
					draw_rounded_rectangle(ctx, x, y, label_width, label_height, label_radius)
					ctx.fill()

					for pin_name in pin_names:
						pin_x, pin_y, align = pin_defs[pin_name]
						pin_x = pin_x * w
						pin_y = pin_y * h_bg - h_bg * y_offset

						ctx.move_to(line_origin_x, y + label_height // 2)
						ctx.line_to(w * pad_sides + pin_x, pin_y)
						ctx.set_line_width(4)
						ctx.stroke()
				else:
					ctx.move_to(x + label_pad[0] - x_bearing, y - y_bearing + label_pad[1])
					ctx.set_source_rgb(1, 1, 1)
					ctx.show_text(text)

		# save to file
		canvas.write_to_png(f"{board_name}_{variant_name}.png")

		# stride = ImageSurface.format_stride_for_width(format, width)
		# data = bytearray(stride * height)
		# surface = ImageSurface(format, width, height, data, stride)
