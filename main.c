#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define CANVAS_WIDTH 450
#define CANVAS_HEIGHT 450
#define BITS_PER_PIXEL 32

#define CANVAS_BORDER_WIDTH 10
#define CANVAS_SPACING 10
#define NEXT_COL(x, i) (x + i * (CANVAS_BORDER_WIDTH + CANVAS_SPACING))

static const char *getenv_default(const char *name, const char *dflt)
{
    return "/dev/fb0";
}

#if LV_USE_LINUX_FBDEV
static lv_display_t *lv_linux_disp_init(void)
{
    const char *device = getenv_default("LV_LINUX_FBDEV_DEVICE", "/dev/fb0");
    lv_display_t * disp = lv_linux_fbdev_create();

    lv_linux_fbdev_set_file(disp, device);
	lv_display_set_default(disp);
	return disp;
}
#elif LV_USE_LINUX_DRM
static void lv_linux_disp_init(void)
{
    const char *device = getenv_default("LV_LINUX_DRM_CARD", "/dev/dri/card0");
    lv_display_t * disp = lv_linux_drm_create();

    lv_linux_drm_set_file(disp, device, -1);
}
#elif LV_USE_SDL
static void lv_linux_disp_init(void)
{
    const int width = atoi(getenv("LV_SDL_VIDEO_WIDTH") ? : "800");
    const int height = atoi(getenv("LV_SDL_VIDEO_HEIGHT") ? : "480");

    lv_sdl_window_create(width, height);
}
#else
#error Unsupported configuration
#endif

static void btn_event_cb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED) {
       static uint8_t cnt = 0;
       cnt++;

       /*Get the first child of the button which is the label and change its text*/
       lv_obj_t * label = lv_obj_get_child(btn, 0);
       lv_label_set_text_fmt(label, "Button: %d", cnt);
    }
}

static void create_canvas(lv_image_dsc_t img_dsc, uint8_t cbuf[], const int x, const int y)
{
	lv_obj_t * canvas = lv_canvas_create(lv_screen_active());
    lv_canvas_set_buffer(canvas, cbuf, img_dsc.header.w, img_dsc.header.h, LV_COLOR_FORMAT_ARGB8888);

    lv_obj_set_style_border_color(canvas, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(canvas, CANVAS_BORDER_WIDTH, LV_PART_MAIN);
    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);
	lv_obj_set_pos(canvas, x, y);

	lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);

    lv_draw_image_dsc_t dsc;
    lv_draw_image_dsc_init(&dsc);

    dsc.src = &img_dsc;
	lv_area_t coords = {0, 0, img_dsc.header.w, img_dsc.header.h};

    lv_draw_image(&layer, &dsc, &coords);

    lv_canvas_finish_layer(canvas, &layer);
}

void create_home_screen()
{
	const int y0 = 200;

	const int x0 = 300;
	static uint8_t cbuf_evcharging[LV_CANVAS_BUF_SIZE(CANVAS_WIDTH, CANVAS_HEIGHT, 32, LV_DRAW_BUF_STRIDE_ALIGN)];
	LV_IMAGE_DECLARE(ti_demo_evcharging);
	create_canvas(ti_demo_evcharging, cbuf_evcharging, x0, y0);

	const int x1 = x0 + ti_demo_evcharging.header.w + 10 + 10;
	static uint8_t cbuf_homeauto[LV_CANVAS_BUF_SIZE(CANVAS_WIDTH, CANVAS_HEIGHT, 32, LV_DRAW_BUF_STRIDE_ALIGN)];
	LV_IMAGE_DECLARE(ti_demo_homeauto);
	create_canvas(ti_demo_homeauto, cbuf_homeauto, x1, y0);

	const int x2 = x1 + ti_demo_homeauto.header.w + 10 + 10;
	static uint8_t cbuf_tempsensor[LV_CANVAS_BUF_SIZE(CANVAS_WIDTH, CANVAS_HEIGHT, 32, LV_DRAW_BUF_STRIDE_ALIGN)];
	LV_IMAGE_DECLARE(ti_demo_tempsensor);
	create_canvas(ti_demo_tempsensor, cbuf_tempsensor, x2, y0);

	const int y1 = y0 + ti_demo_evcharging.header.h + 10 + 10;
	static uint8_t cbuf_emeter[LV_CANVAS_BUF_SIZE(CANVAS_WIDTH, CANVAS_HEIGHT, 32, LV_DRAW_BUF_STRIDE_ALIGN)];
	LV_IMAGE_DECLARE(ti_demo_emeter);
	create_canvas(ti_demo_emeter, cbuf_emeter, x0, y1);

	static uint8_t cbuf_security[LV_CANVAS_BUF_SIZE(CANVAS_WIDTH, CANVAS_HEIGHT, 32, LV_DRAW_BUF_STRIDE_ALIGN)];
	LV_IMAGE_DECLARE(ti_demo_security);
	create_canvas(ti_demo_security, cbuf_security, x1, y1);
}

int main(void)
{
	/* initialize lvgl */
    lv_init();

	/* initialize /dev/fb1 */
    lv_display_t *disp = lv_linux_disp_init();

	/* set background colour to blue */
	lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x003a57), LV_PART_MAIN);

	create_home_screen();

	/* initialize /dev/mouse0 for mouse input */
	lv_indev_t *ptr = lv_evdev_create(LV_INDEV_TYPE_POINTER, "/dev/input/event8");
	lv_indev_set_display(ptr, disp);
	/* put the image of a cursor on the evdev instance so that a cursor is visible */
	LV_IMAGE_DECLARE(mouse_cursor_icon);
	lv_obj_t *cursor_obj = lv_image_create(lv_screen_active());
	lv_image_set_src(cursor_obj, &mouse_cursor_icon);
	lv_indev_set_cursor(ptr, cursor_obj);

    /*Handle LVGL tasks*/
    while(1) {
        lv_timer_handler();
        usleep(5000);
    }

    return 0;
}
