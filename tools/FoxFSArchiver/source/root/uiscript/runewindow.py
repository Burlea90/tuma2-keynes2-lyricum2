import uiscriptlocale

BOARD_WIDTH = 376
BOARD_HEIGHT = 408
BG_WIDTH = 356
BG_HEIGHT = 368

window = {
	"name" : "RuneWindow",
	"x" : (SCREEN_WIDTH / 2) - (BOARD_WIDTH / 2),
	"y" : (SCREEN_HEIGHT / 2) - (BOARD_HEIGHT / 2),
	"style" : ("movable", "float",),
	"width" : BOARD_WIDTH,
	"height" : BOARD_HEIGHT,
	"children" :
	(
		{
			"name" : "board",
			"type" : "board",
			"x" : 0,
			"y" : 0,
			"width" : BOARD_WIDTH,
			"height" : BOARD_HEIGHT,
			"children" :
			(
				{
					"name" : "TitleBar",
					"type" : "titlebar",
					"style" : ("attach",),
					"x" : 6,
					"y" : 6,
					"width" : BOARD_WIDTH - 14,
					"children" :
					(
						{
							"name" : "TitleName",
							"type" : "text",
							"x" : 0,
							"y" : 3,
							"text" : uiscriptlocale.RUNE_TITLE,
							"horizontal_align" : "center",
							"text_horizontal_align" : "center"
						},
					),
				},
				{
					"name" : "Base",
					"type" : "expanded_image",
					"x" : 10,
					"y" : 25,
					"image" : "d:/ymir work/ui/rune/bg.tga",
					"children" :
					(
						{
							"name" : "eff1_rune_0",
							"type" : "expanded_image",
							"x" : 45,
							"y" : 86,
							"image" : "d:/ymir work/ui/rune/eff_rune_0.tga",
						},
						{
							"name" : "eff1_rune_1",
							"type" : "expanded_image",
							"x" : 129,
							"y" : 139,
							"image" : "d:/ymir work/ui/rune/eff_rune_3.tga",
						},
						{
							"name" : "eff1_rune_2",
							"type" : "expanded_image",
							"x" : 148,
							"y" : 85,
							"image" : "d:/ymir work/ui/rune/eff_rune_1.tga",
						},
						{
							"name" : "eff1_rune_3",
							"type" : "expanded_image",
							"x" : 136,
							"y" : 75,
							"image" : "d:/ymir work/ui/rune/eff_rune_2.tga",
						},
						{
							"name" : "eff1_rune_4",
							"type" : "expanded_image",
							"x" : 79,
							"y" : 131,
							"image" : "d:/ymir work/ui/rune/eff_rune_4.tga",
						},
						{
							"name" : "eff1_rune_5",
							"type" : "expanded_image",
							"x" : 108,
							"y" : 130,
							"image" : "d:/ymir work/ui/rune/eff_rune_5.tga",
						},
						{
							"name" : "eff1_rune_6",
							"type" : "expanded_image",
							"x" : 0,
							"y" : 0,
							"image" : "d:/ymir work/ui/rune/eff_rune_6.tga",
						},
						{
							"name" : "eff2_rune_0",
							"type" : "expanded_image",
							"x" : 25,
							"y" : 81,
							"image" : "d:/ymir work/ui/rune/eff_rune_circle.tga",
						},
						{
							"name" : "eff2_rune_1",
							"type" : "expanded_image",
							"x" : 148,
							"y" : 284,
							"image" : "d:/ymir work/ui/rune/eff_rune_circle.tga",
						},
						{
							"name" : "eff2_rune_2",
							"type" : "expanded_image",
							"x" : 273,
							"y" : 81,
							"image" : "d:/ymir work/ui/rune/eff_rune_circle.tga",
						},
						{
							"name" : "eff2_rune_3",
							"type" : "expanded_image",
							"x" : 149,
							"y" : 49,
							"image" : "d:/ymir work/ui/rune/eff_rune_circle.tga",
						},
						{
							"name" : "eff2_rune_4",
							"type" : "expanded_image",
							"x" : 63,
							"y" : 193,
							"image" : "d:/ymir work/ui/rune/eff_rune_circle.tga",
						},
						{
							"name" : "eff2_rune_5",
							"type" : "expanded_image",
							"x" : 235,
							"y" : 193,
							"image" : "d:/ymir work/ui/rune/eff_rune_circle.tga",
						},
						{
							"name" : "eff2_rune_6",
							"type" : "expanded_image",
							"x" : 150,
							"y" : 146,
							"image" : "d:/ymir work/ui/rune/eff_rune_circle_empty.tga",
						},
						{
							"name" : "eff_bonus",
							"type" : "ani_image",
							"x" : 128,
							"y" : 134,
							"width"  : 100,
							"height" : 100,
							"delay" : 6,
							"images" :
							(
								"d:/ymir work/ui/rune/frame/1.tga",
								"d:/ymir work/ui/rune/frame/2.tga",
								"d:/ymir work/ui/rune/frame/3.tga",
								"d:/ymir work/ui/rune/frame/4.tga",
								"d:/ymir work/ui/rune/frame/5.tga",
								"d:/ymir work/ui/rune/frame/6.tga",
								"d:/ymir work/ui/rune/frame/7.tga",
								"d:/ymir work/ui/rune/frame/8.tga",
								"d:/ymir work/ui/rune/frame/9.tga",
								"d:/ymir work/ui/rune/frame/10.tga",
								"d:/ymir work/ui/rune/frame/11.tga",
								"d:/ymir work/ui/rune/frame/12.tga",
								"d:/ymir work/ui/rune/frame/13.tga",
								"d:/ymir work/ui/rune/frame/14.tga",
								"d:/ymir work/ui/rune/frame/15.tga",
								"d:/ymir work/ui/rune/frame/16.tga",
								"d:/ymir work/ui/rune/frame/17.tga",
								"d:/ymir work/ui/rune/frame/18.tga",
							)
						},
					),
				},
				{
					"name" : "ItemSlot",
					"type" : "slot",
					"x" : 10,
					"y" : 25,
					"width" : BG_WIDTH,
					"height" : BG_HEIGHT,
					"slot" : (
								{"index" : RUNE_SLOT_START, "x" : 34, "y" : 90, "width" : 40, "height" : 40},
								{"index" : RUNE_SLOT_START + 1, "x" : 158, "y" : 292, "width" : 40, "height" : 40},
								{"index" : RUNE_SLOT_START + 2, "x" : 282, "y" : 90, "width" : 40, "height" : 40},
								{"index" : RUNE_SLOT_START + 3, "x" : 158, "y" : 58, "width" : 40, "height" : 40},
								{"index" : RUNE_SLOT_START + 4, "x" : 71, "y" : 202, "width" : 40, "height" : 40},
								{"index" : RUNE_SLOT_START + 5, "x" : 245, "y" : 202, "width" : 40, "height" : 40},
								{"index" : RUNE_SLOT_START + 6, "x" : 128, "y" : 125, "width" : 100, "height" : 100},
					),
				},
				{
					"name" : "shop",
					"type" : "button",
					"x" : BOARD_WIDTH - 130,
					"y" : BOARD_HEIGHT - 30,
					"text" : uiscriptlocale.RUNE_SHOP,
					"default_image" : "d:/ymir work/ui/public/middle_button_01.sub",
					"over_image" : "d:/ymir work/ui/public/middle_button_02.sub",
					"down_image" : "d:/ymir work/ui/public/middle_button_03.sub",
				},
				{
					"name" : "activate",
					"type" : "toggle_button",
					"x" : BOARD_WIDTH - 70,
					"y" : BOARD_HEIGHT - 30,
					"text" : "",
					"default_image" : "d:/ymir work/ui/public/middle_button_01.sub",
					"over_image" : "d:/ymir work/ui/public/middle_button_02.sub",
					"down_image" : "d:/ymir work/ui/public/middle_button_03.sub",
				},
				{
					"name" : "effect",
					"type" : "toggle_button",
					"x" : 9,
					"y" : BOARD_HEIGHT - 30,
					"text" : uiscriptlocale.RUNE_EFFECT,
					"tooltip_text" : uiscriptlocale.RUNE_EFFECT_DESC,
					"default_image" : "d:/ymir work/ui/public/middle_button_01.sub",
					"over_image" : "d:/ymir work/ui/public/middle_button_02.sub",
					"down_image" : "d:/ymir work/ui/public/middle_button_03.sub",
				},
			),
		},
	),
}