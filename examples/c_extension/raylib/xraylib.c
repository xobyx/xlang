#include "xlang.h"
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static XExtensionContext* s_ctx = NULL;

/* -------------------------------------------------------------------------
 * Color Helpers
 * ------------------------------------------------------------------------- */
static inline Color int_to_color(int c)
{
	Color col;
	col.r = (unsigned char)(c & 0xFF);
	col.g = (unsigned char)((c >> 8) & 0xFF);
	col.b = (unsigned char)((c >> 16) & 0xFF);
	col.a = (unsigned char)((c >> 24) & 0xFF);
	return col;
}

static inline int color_to_int(Color c)
{
	return (int)((uint32_t)c.r | ((uint32_t)c.g << 8) | ((uint32_t)c.b << 16) | ((uint32_t)c.a << 24));
}

/* -------------------------------------------------------------------------
 * Direct C Functions (C ABI exported for AOT Native LLVM and C consumers)
 * ------------------------------------------------------------------------- */

/* Window Lifecycle */
XLANG_EXPORT void raylib_init_window(int width, int height, const char* title)
{
	InitWindow(width, height, title ? title : "xlang raylib");
}

XLANG_EXPORT void raylib_close_window(void)
{
	CloseWindow();
}

XLANG_EXPORT int raylib_window_should_close(void)
{
	return WindowShouldClose() ? 1 : 0;
}

XLANG_EXPORT int raylib_is_window_ready(void)
{
	return IsWindowReady() ? 1 : 0;
}

XLANG_EXPORT void raylib_set_target_fps(int fps)
{
	SetTargetFPS(fps);
}

XLANG_EXPORT int raylib_get_fps(void)
{
	return GetFPS();
}

XLANG_EXPORT double raylib_get_frame_time(void)
{
	return (double)GetFrameTime();
}

XLANG_EXPORT double raylib_get_time(void)
{
	return (double)GetTime();
}

XLANG_EXPORT void raylib_set_config_flags(int flags)
{
	SetConfigFlags((unsigned int)flags);
}

XLANG_EXPORT int raylib_get_screen_width(void)
{
	return GetScreenWidth();
}

XLANG_EXPORT int raylib_get_screen_height(void)
{
	return GetScreenHeight();
}

/* Drawing Loop */
XLANG_EXPORT void raylib_begin_drawing(void)
{
	BeginDrawing();
}

XLANG_EXPORT void raylib_end_drawing(void)
{
	EndDrawing();
}

XLANG_EXPORT void raylib_clear_background(int color)
{
	ClearBackground(int_to_color(color));
}

/* 2D Shapes & Text */
XLANG_EXPORT void raylib_draw_text(const char* text, int posX, int posY, int fontSize, int color)
{
	DrawText(text ? text : "", posX, posY, fontSize, int_to_color(color));
}

XLANG_EXPORT void raylib_draw_fps(int posX, int posY)
{
	DrawFPS(posX, posY);
}

XLANG_EXPORT void raylib_draw_pixel(int posX, int posY, int color)
{
	DrawPixel(posX, posY, int_to_color(color));
}

XLANG_EXPORT void raylib_draw_line(int startPosX, int startPosY, int endPosX, int endPosY, int color)
{
	DrawLine(startPosX, startPosY, endPosX, endPosY, int_to_color(color));
}

XLANG_EXPORT void raylib_draw_circle(int centerX, int centerY, double radius, int color)
{
	DrawCircle(centerX, centerY, (float)radius, int_to_color(color));
}

XLANG_EXPORT void raylib_draw_circle_lines(int centerX, int centerY, double radius, int color)
{
	DrawCircleLines(centerX, centerY, (float)radius, int_to_color(color));
}

XLANG_EXPORT void raylib_draw_rectangle(int posX, int posY, int width, int height, int color)
{
	DrawRectangle(posX, posY, width, height, int_to_color(color));
}

XLANG_EXPORT void raylib_draw_rectangle_lines(int posX, int posY, int width, int height, int color)
{
	DrawRectangleLines(posX, posY, width, height, int_to_color(color));
}

/* Palette & Color Constructors */
XLANG_EXPORT int raylib_color(int r, int g, int b, int a)
{
	Color c;
	c.r = (unsigned char)r;
	c.g = (unsigned char)g;
	c.b = (unsigned char)b;
	c.a = (unsigned char)a;
	return color_to_int(c);
}

XLANG_EXPORT int raylib_white(void)     { return color_to_int(WHITE); }
XLANG_EXPORT int raylib_black(void)     { return color_to_int(BLACK); }
XLANG_EXPORT int raylib_red(void)       { return color_to_int(RED); }
XLANG_EXPORT int raylib_green(void)     { return color_to_int(GREEN); }
XLANG_EXPORT int raylib_blue(void)      { return color_to_int(BLUE); }
XLANG_EXPORT int raylib_yellow(void)    { return color_to_int(YELLOW); }
XLANG_EXPORT int raylib_gold(void)      { return color_to_int(GOLD); }
XLANG_EXPORT int raylib_raywhite(void)  { return color_to_int(RAYWHITE); }
XLANG_EXPORT int raylib_darkgray(void)  { return color_to_int(DARKGRAY); }
XLANG_EXPORT int raylib_lightgray(void) { return color_to_int(LIGHTGRAY); }
XLANG_EXPORT int raylib_skyblue(void)   { return color_to_int(SKYBLUE); }

/* Input Queries */
XLANG_EXPORT int raylib_get_mouse_x(void)
{
	return GetMouseX();
}

XLANG_EXPORT int raylib_get_mouse_y(void)
{
	return GetMouseY();
}

XLANG_EXPORT int raylib_is_mouse_button_pressed(int button)
{
	return IsMouseButtonPressed(button) ? 1 : 0;
}

XLANG_EXPORT int raylib_is_mouse_button_down(int button)
{
	return IsMouseButtonDown(button) ? 1 : 0;
}

XLANG_EXPORT int raylib_is_key_pressed(int key)
{
	return IsKeyPressed(key) ? 1 : 0;
}

XLANG_EXPORT int raylib_is_key_down(int key)
{
	return IsKeyDown(key) ? 1 : 0;
}

/* Image Manipulation & Headless Export */
XLANG_EXPORT void* raylib_gen_image_color(int width, int height, int color)
{
	Image* img = (Image*)malloc(sizeof(Image));
	if (img)
	{
		*img = GenImageColor(width, height, int_to_color(color));
	}
	return (void*)img;
}

XLANG_EXPORT void raylib_image_draw_rectangle(void* img, int posX, int posY, int width, int height, int color)
{
	if (img)
	{
		ImageDrawRectangle((Image*)img, posX, posY, width, height, int_to_color(color));
	}
}

XLANG_EXPORT void raylib_image_draw_circle(void* img, int centerX, int centerY, double radius, int color)
{
	if (img)
	{
		ImageDrawCircle((Image*)img, centerX, centerY, (int)radius, int_to_color(color));
	}
}

XLANG_EXPORT void raylib_image_draw_line(void* img, int startPosX, int startPosY, int endPosX, int endPosY, int color)
{
	if (img)
	{
		ImageDrawLine((Image*)img, startPosX, startPosY, endPosX, endPosY, int_to_color(color));
	}
}

XLANG_EXPORT void raylib_image_draw_text(void* img, const char* text, int posX, int posY, int fontSize, int color)
{
	if (img && text)
	{
		ImageDrawText((Image*)img, text, posX, posY, fontSize, int_to_color(color));
	}
}

XLANG_EXPORT int raylib_export_image(void* img, const char* fileName)
{
	if (!img || !fileName) return 0;
	return ExportImage(*(Image*)img, fileName) ? 1 : 0;
}

XLANG_EXPORT void raylib_take_screenshot(const char* fileName)
{
	if (fileName) TakeScreenshot(fileName);
}

XLANG_EXPORT void raylib_unload_image(void* img)
{
	if (img)
	{
		UnloadImage(*(Image*)img);
		free(img);
	}
}

/* Constant flags & keycodes */
XLANG_EXPORT int raylib_flag_window_hidden(void) { return 128; }
XLANG_EXPORT int raylib_flag_msaa_4x(void)        { return 32; }
XLANG_EXPORT int raylib_flag_vsync(void)          { return 64; }
XLANG_EXPORT int raylib_mouse_button_left(void)   { return 0; }
XLANG_EXPORT int raylib_mouse_button_right(void)  { return 1; }
XLANG_EXPORT int raylib_key_space(void)           { return 32; }
XLANG_EXPORT int raylib_key_escape(void)          { return 256; }
XLANG_EXPORT int raylib_key_enter(void)           { return 257; }
XLANG_EXPORT int raylib_key_right(void)           { return 262; }
XLANG_EXPORT int raylib_key_left(void)            { return 263; }
XLANG_EXPORT int raylib_key_down(void)            { return 264; }
XLANG_EXPORT int raylib_key_up(void)              { return 265; }

/* -------------------------------------------------------------------------
 * Vectorcall Wrappers (for VM and JIT invocation)
 * ------------------------------------------------------------------------- */
static inline int arg_as_int(int argc, const XValue* args, int index, int def)
{
	if (index < argc)
	{
		if (args[index].type == XLANG_VAL_INT) return (int)args[index].as.ival;
		if (args[index].type == XLANG_VAL_FLOAT) return (int)args[index].as.fval;
	}
	return def;
}

static inline double arg_as_float(int argc, const XValue* args, int index, double def)
{
	if (index < argc)
	{
		if (args[index].type == XLANG_VAL_FLOAT) return args[index].as.fval;
		if (args[index].type == XLANG_VAL_INT) return (double)args[index].as.ival;
	}
	return def;
}

static inline const char* arg_as_str(int argc, const XValue* args, int index, const char* def)
{
	if (index < argc && args[index].type == XLANG_VAL_STRING)
		return args[index].as.sval;
	return def;
}

static inline void* arg_as_ptr(int argc, const XValue* args, int index)
{
	if (index < argc)
	{
		return xval_as_pointer(args[index]);
	}
	return NULL;
}

/* Vectorcall functions */
static XValue vec_init_window(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	int w = arg_as_int(argc, args, 0, 800);
	int h = arg_as_int(argc, args, 1, 600);
	const char* t = arg_as_str(argc, args, 2, "xlang raylib");
	raylib_init_window(w, h, t);
	return xval_null();
}

static XValue vec_close_window(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver; (void)argc; (void)args;
	raylib_close_window();
	return xval_null();
}

static XValue vec_window_should_close(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver; (void)argc; (void)args;
	return xval_int(raylib_window_should_close());
}

static XValue vec_is_window_ready(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver; (void)argc; (void)args;
	return xval_int(raylib_is_window_ready());
}

static XValue vec_set_target_fps(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	int fps = arg_as_int(argc, args, 0, 60);
	raylib_set_target_fps(fps);
	return xval_null();
}

static XValue vec_get_fps(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver; (void)argc; (void)args;
	return xval_int(raylib_get_fps());
}

static XValue vec_get_frame_time(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver; (void)argc; (void)args;
	return xval_float(raylib_get_frame_time());
}

static XValue vec_get_time(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver; (void)argc; (void)args;
	return xval_float(raylib_get_time());
}

static XValue vec_set_config_flags(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	int flags = arg_as_int(argc, args, 0, 0);
	raylib_set_config_flags(flags);
	return xval_null();
}

static XValue vec_get_screen_width(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver; (void)argc; (void)args;
	return xval_int(raylib_get_screen_width());
}

static XValue vec_get_screen_height(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver; (void)argc; (void)args;
	return xval_int(raylib_get_screen_height());
}

static XValue vec_begin_drawing(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver; (void)argc; (void)args;
	raylib_begin_drawing();
	return xval_null();
}

static XValue vec_end_drawing(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver; (void)argc; (void)args;
	raylib_end_drawing();
	return xval_null();
}

static XValue vec_clear_background(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	int c = arg_as_int(argc, args, 0, 0);
	raylib_clear_background(c);
	return xval_null();
}

static XValue vec_draw_text(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	const char* text = arg_as_str(argc, args, 0, "");
	int x = arg_as_int(argc, args, 1, 0);
	int y = arg_as_int(argc, args, 2, 0);
	int sz = arg_as_int(argc, args, 3, 20);
	int col = arg_as_int(argc, args, 4, 0);
	raylib_draw_text(text, x, y, sz, col);
	return xval_null();
}

static XValue vec_draw_fps(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	int x = arg_as_int(argc, args, 0, 10);
	int y = arg_as_int(argc, args, 1, 10);
	raylib_draw_fps(x, y);
	return xval_null();
}

static XValue vec_draw_pixel(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	int x = arg_as_int(argc, args, 0, 0);
	int y = arg_as_int(argc, args, 1, 0);
	int col = arg_as_int(argc, args, 2, 0);
	raylib_draw_pixel(x, y, col);
	return xval_null();
}

static XValue vec_draw_line(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	int x1 = arg_as_int(argc, args, 0, 0);
	int y1 = arg_as_int(argc, args, 1, 0);
	int x2 = arg_as_int(argc, args, 2, 0);
	int y2 = arg_as_int(argc, args, 3, 0);
	int col = arg_as_int(argc, args, 4, 0);
	raylib_draw_line(x1, y1, x2, y2, col);
	return xval_null();
}

static XValue vec_draw_circle(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	int cx = arg_as_int(argc, args, 0, 0);
	int cy = arg_as_int(argc, args, 1, 0);
	double r = arg_as_float(argc, args, 2, 10.0);
	int col = arg_as_int(argc, args, 3, 0);
	raylib_draw_circle(cx, cy, r, col);
	return xval_null();
}

static XValue vec_draw_circle_lines(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	int cx = arg_as_int(argc, args, 0, 0);
	int cy = arg_as_int(argc, args, 1, 0);
	double r = arg_as_float(argc, args, 2, 10.0);
	int col = arg_as_int(argc, args, 3, 0);
	raylib_draw_circle_lines(cx, cy, r, col);
	return xval_null();
}

static XValue vec_draw_rectangle(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	int x = arg_as_int(argc, args, 0, 0);
	int y = arg_as_int(argc, args, 1, 0);
	int w = arg_as_int(argc, args, 2, 0);
	int h = arg_as_int(argc, args, 3, 0);
	int col = arg_as_int(argc, args, 4, 0);
	raylib_draw_rectangle(x, y, w, h, col);
	return xval_null();
}

static XValue vec_draw_rectangle_lines(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	int x = arg_as_int(argc, args, 0, 0);
	int y = arg_as_int(argc, args, 1, 0);
	int w = arg_as_int(argc, args, 2, 0);
	int h = arg_as_int(argc, args, 3, 0);
	int col = arg_as_int(argc, args, 4, 0);
	raylib_draw_rectangle_lines(x, y, w, h, col);
	return xval_null();
}

static XValue vec_color(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	int r = arg_as_int(argc, args, 0, 0);
	int g = arg_as_int(argc, args, 1, 0);
	int b = arg_as_int(argc, args, 2, 0);
	int a = arg_as_int(argc, args, 3, 255);
	return xval_int(raylib_color(r, g, b, a));
}

static XValue vec_white(XVm* vm, XValue receiver, int argc, const XValue* args)     { (void)vm; (void)receiver; (void)argc; (void)args; return xval_int(raylib_white()); }
static XValue vec_black(XVm* vm, XValue receiver, int argc, const XValue* args)     { (void)vm; (void)receiver; (void)argc; (void)args; return xval_int(raylib_black()); }
static XValue vec_red(XVm* vm, XValue receiver, int argc, const XValue* args)       { (void)vm; (void)receiver; (void)argc; (void)args; return xval_int(raylib_red()); }
static XValue vec_green(XVm* vm, XValue receiver, int argc, const XValue* args)     { (void)vm; (void)receiver; (void)argc; (void)args; return xval_int(raylib_green()); }
static XValue vec_blue(XVm* vm, XValue receiver, int argc, const XValue* args)      { (void)vm; (void)receiver; (void)argc; (void)args; return xval_int(raylib_blue()); }
static XValue vec_yellow(XVm* vm, XValue receiver, int argc, const XValue* args)    { (void)vm; (void)receiver; (void)argc; (void)args; return xval_int(raylib_yellow()); }
static XValue vec_gold(XVm* vm, XValue receiver, int argc, const XValue* args)      { (void)vm; (void)receiver; (void)argc; (void)args; return xval_int(raylib_gold()); }
static XValue vec_raywhite(XVm* vm, XValue receiver, int argc, const XValue* args)  { (void)vm; (void)receiver; (void)argc; (void)args; return xval_int(raylib_raywhite()); }
static XValue vec_darkgray(XVm* vm, XValue receiver, int argc, const XValue* args)  { (void)vm; (void)receiver; (void)argc; (void)args; return xval_int(raylib_darkgray()); }
static XValue vec_lightgray(XVm* vm, XValue receiver, int argc, const XValue* args) { (void)vm; (void)receiver; (void)argc; (void)args; return xval_int(raylib_lightgray()); }
static XValue vec_skyblue(XVm* vm, XValue receiver, int argc, const XValue* args)   { (void)vm; (void)receiver; (void)argc; (void)args; return xval_int(raylib_skyblue()); }

static XValue vec_get_mouse_x(XVm* vm, XValue receiver, int argc, const XValue* args) { (void)vm; (void)receiver; (void)argc; (void)args; return xval_int(raylib_get_mouse_x()); }
static XValue vec_get_mouse_y(XVm* vm, XValue receiver, int argc, const XValue* args) { (void)vm; (void)receiver; (void)argc; (void)args; return xval_int(raylib_get_mouse_y()); }
static XValue vec_is_mouse_button_pressed(XVm* vm, XValue receiver, int argc, const XValue* args) { (void)vm; (void)receiver; return xval_int(raylib_is_mouse_button_pressed(arg_as_int(argc, args, 0, 0))); }
static XValue vec_is_mouse_button_down(XVm* vm, XValue receiver, int argc, const XValue* args)    { (void)vm; (void)receiver; return xval_int(raylib_is_mouse_button_down(arg_as_int(argc, args, 0, 0))); }
static XValue vec_is_key_pressed(XVm* vm, XValue receiver, int argc, const XValue* args)          { (void)vm; (void)receiver; return xval_int(raylib_is_key_pressed(arg_as_int(argc, args, 0, 0))); }
static XValue vec_is_key_down(XVm* vm, XValue receiver, int argc, const XValue* args)             { (void)vm; (void)receiver; return xval_int(raylib_is_key_down(arg_as_int(argc, args, 0, 0))); }

static XValue vec_gen_image_color(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	int w = arg_as_int(argc, args, 0, 100);
	int h = arg_as_int(argc, args, 1, 100);
	int c = arg_as_int(argc, args, 2, 0);
	void* img = raylib_gen_image_color(w, h, c);
	return xval_pointer(img);
}

static XValue vec_image_draw_rectangle(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	void* img = arg_as_ptr(argc, args, 0);
	int x = arg_as_int(argc, args, 1, 0);
	int y = arg_as_int(argc, args, 2, 0);
	int w = arg_as_int(argc, args, 3, 0);
	int h = arg_as_int(argc, args, 4, 0);
	int c = arg_as_int(argc, args, 5, 0);
	raylib_image_draw_rectangle(img, x, y, w, h, c);
	return xval_null();
}

static XValue vec_image_draw_circle(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	void* img = arg_as_ptr(argc, args, 0);
	int cx = arg_as_int(argc, args, 1, 0);
	int cy = arg_as_int(argc, args, 2, 0);
	double r = arg_as_float(argc, args, 3, 10.0);
	int c = arg_as_int(argc, args, 4, 0);
	raylib_image_draw_circle(img, cx, cy, r, c);
	return xval_null();
}

static XValue vec_image_draw_line(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	void* img = arg_as_ptr(argc, args, 0);
	int x1 = arg_as_int(argc, args, 1, 0);
	int y1 = arg_as_int(argc, args, 2, 0);
	int x2 = arg_as_int(argc, args, 3, 0);
	int y2 = arg_as_int(argc, args, 4, 0);
	int c = arg_as_int(argc, args, 5, 0);
	raylib_image_draw_line(img, x1, y1, x2, y2, c);
	return xval_null();
}

static XValue vec_image_draw_text(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	void* img = arg_as_ptr(argc, args, 0);
	const char* text = arg_as_str(argc, args, 1, "");
	int x = arg_as_int(argc, args, 2, 0);
	int y = arg_as_int(argc, args, 3, 0);
	int sz = arg_as_int(argc, args, 4, 20);
	int col = arg_as_int(argc, args, 5, 0);
	raylib_image_draw_text(img, text, x, y, sz, col);
	return xval_null();
}

static XValue vec_export_image(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	void* img = arg_as_ptr(argc, args, 0);
	const char* fn = arg_as_str(argc, args, 1, "output.png");
	return xval_int(raylib_export_image(img, fn));
}

static XValue vec_take_screenshot(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	const char* fn = arg_as_str(argc, args, 0, "screenshot.png");
	raylib_take_screenshot(fn);
	return xval_null();
}

static XValue vec_unload_image(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	void* img = arg_as_ptr(argc, args, 0);
	raylib_unload_image(img);
	return xval_null();
}

static XValue vec_flag_window_hidden(XVm* vm, XValue receiver, int argc, const XValue* args) { (void)vm; (void)receiver; (void)argc; (void)args; return xval_int(raylib_flag_window_hidden()); }
static XValue vec_flag_msaa_4x(XVm* vm, XValue receiver, int argc, const XValue* args)        { (void)vm; (void)receiver; (void)argc; (void)args; return xval_int(raylib_flag_msaa_4x()); }
static XValue vec_flag_vsync(XVm* vm, XValue receiver, int argc, const XValue* args)          { (void)vm; (void)receiver; (void)argc; (void)args; return xval_int(raylib_flag_vsync()); }
static XValue vec_mouse_button_left(XVm* vm, XValue receiver, int argc, const XValue* args)   { (void)vm; (void)receiver; (void)argc; (void)args; return xval_int(raylib_mouse_button_left()); }
static XValue vec_mouse_button_right(XVm* vm, XValue receiver, int argc, const XValue* args)  { (void)vm; (void)receiver; (void)argc; (void)args; return xval_int(raylib_mouse_button_right()); }
static XValue vec_key_space(XVm* vm, XValue receiver, int argc, const XValue* args)           { (void)vm; (void)receiver; (void)argc; (void)args; return xval_int(raylib_key_space()); }
static XValue vec_key_escape(XVm* vm, XValue receiver, int argc, const XValue* args)          { (void)vm; (void)receiver; (void)argc; (void)args; return xval_int(raylib_key_escape()); }
static XValue vec_key_enter(XVm* vm, XValue receiver, int argc, const XValue* args)           { (void)vm; (void)receiver; (void)argc; (void)args; return xval_int(raylib_key_enter()); }
static XValue vec_key_right(XVm* vm, XValue receiver, int argc, const XValue* args)           { (void)vm; (void)receiver; (void)argc; (void)args; return xval_int(raylib_key_right()); }
static XValue vec_key_left(XVm* vm, XValue receiver, int argc, const XValue* args)            { (void)vm; (void)receiver; (void)argc; (void)args; return xval_int(raylib_key_left()); }
static XValue vec_key_down(XVm* vm, XValue receiver, int argc, const XValue* args)            { (void)vm; (void)receiver; (void)argc; (void)args; return xval_int(raylib_key_down()); }
static XValue vec_key_up(XVm* vm, XValue receiver, int argc, const XValue* args)              { (void)vm; (void)receiver; (void)argc; (void)args; return xval_int(raylib_key_up()); }

/* -------------------------------------------------------------------------
 * Function Table Registration
 * ------------------------------------------------------------------------- */
static const XExtensionFunc s_raylib_funcs[] = {
	/* Window lifecycle */
	{ "init_window",          vec_init_window,          3, XLANG_TYPE_VOID,   { XLANG_TYPE_INT, XLANG_TYPE_INT, XLANG_TYPE_STRING } },
	{ "close_window",         vec_close_window,         0, XLANG_TYPE_VOID,   { XLANG_TYPE_VOID } },
	{ "window_should_close",  vec_window_should_close,  0, XLANG_TYPE_INT,    { XLANG_TYPE_VOID } },
	{ "is_window_ready",      vec_is_window_ready,      0, XLANG_TYPE_INT,    { XLANG_TYPE_VOID } },
	{ "set_target_fps",       vec_set_target_fps,       1, XLANG_TYPE_VOID,   { XLANG_TYPE_INT } },
	{ "get_fps",              vec_get_fps,              0, XLANG_TYPE_INT,    { XLANG_TYPE_VOID } },
	{ "get_frame_time",       vec_get_frame_time,       0, XLANG_TYPE_FLOAT,  { XLANG_TYPE_VOID } },
	{ "get_time",             vec_get_time,             0, XLANG_TYPE_FLOAT,  { XLANG_TYPE_VOID } },
	{ "set_config_flags",     vec_set_config_flags,     1, XLANG_TYPE_VOID,   { XLANG_TYPE_INT } },
	{ "get_screen_width",     vec_get_screen_width,     0, XLANG_TYPE_INT,    { XLANG_TYPE_VOID } },
	{ "get_screen_height",    vec_get_screen_height,    0, XLANG_TYPE_INT,    { XLANG_TYPE_VOID } },

	/* Drawing loop */
	{ "begin_drawing",        vec_begin_drawing,        0, XLANG_TYPE_VOID,   { XLANG_TYPE_VOID } },
	{ "end_drawing",          vec_end_drawing,          0, XLANG_TYPE_VOID,   { XLANG_TYPE_VOID } },
	{ "clear_background",     vec_clear_background,     1, XLANG_TYPE_VOID,   { XLANG_TYPE_INT } },

	/* 2D primitives & text */
	{ "draw_text",            vec_draw_text,            5, XLANG_TYPE_VOID,   { XLANG_TYPE_STRING, XLANG_TYPE_INT, XLANG_TYPE_INT, XLANG_TYPE_INT, XLANG_TYPE_INT } },
	{ "draw_fps",             vec_draw_fps,             2, XLANG_TYPE_VOID,   { XLANG_TYPE_INT, XLANG_TYPE_INT } },
	{ "draw_pixel",           vec_draw_pixel,           3, XLANG_TYPE_VOID,   { XLANG_TYPE_INT, XLANG_TYPE_INT, XLANG_TYPE_INT } },
	{ "draw_line",            vec_draw_line,            5, XLANG_TYPE_VOID,   { XLANG_TYPE_INT, XLANG_TYPE_INT, XLANG_TYPE_INT, XLANG_TYPE_INT, XLANG_TYPE_INT } },
	{ "draw_circle",          vec_draw_circle,          4, XLANG_TYPE_VOID,   { XLANG_TYPE_INT, XLANG_TYPE_INT, XLANG_TYPE_FLOAT, XLANG_TYPE_INT } },
	{ "draw_circle_lines",    vec_draw_circle_lines,    4, XLANG_TYPE_VOID,   { XLANG_TYPE_INT, XLANG_TYPE_INT, XLANG_TYPE_FLOAT, XLANG_TYPE_INT } },
	{ "draw_rectangle",       vec_draw_rectangle,       5, XLANG_TYPE_VOID,   { XLANG_TYPE_INT, XLANG_TYPE_INT, XLANG_TYPE_INT, XLANG_TYPE_INT, XLANG_TYPE_INT } },
	{ "draw_rectangle_lines", vec_draw_rectangle_lines, 5, XLANG_TYPE_VOID,   { XLANG_TYPE_INT, XLANG_TYPE_INT, XLANG_TYPE_INT, XLANG_TYPE_INT, XLANG_TYPE_INT } },

	/* Palette & Colors */
	{ "color",                vec_color,                4, XLANG_TYPE_INT,    { XLANG_TYPE_INT, XLANG_TYPE_INT, XLANG_TYPE_INT, XLANG_TYPE_INT } },
	{ "white",                vec_white,                0, XLANG_TYPE_INT,    { XLANG_TYPE_VOID } },
	{ "black",                vec_black,                0, XLANG_TYPE_INT,    { XLANG_TYPE_VOID } },
	{ "red",                  vec_red,                  0, XLANG_TYPE_INT,    { XLANG_TYPE_VOID } },
	{ "green",                vec_green,                0, XLANG_TYPE_INT,    { XLANG_TYPE_VOID } },
	{ "blue",                 vec_blue,                 0, XLANG_TYPE_INT,    { XLANG_TYPE_VOID } },
	{ "yellow",               vec_yellow,               0, XLANG_TYPE_INT,    { XLANG_TYPE_VOID } },
	{ "gold",                 vec_gold,                 0, XLANG_TYPE_INT,    { XLANG_TYPE_VOID } },
	{ "raywhite",             vec_raywhite,             0, XLANG_TYPE_INT,    { XLANG_TYPE_VOID } },
	{ "darkgray",             vec_darkgray,             0, XLANG_TYPE_INT,    { XLANG_TYPE_VOID } },
	{ "lightgray",            vec_lightgray,            0, XLANG_TYPE_INT,    { XLANG_TYPE_VOID } },
	{ "skyblue",              vec_skyblue,              0, XLANG_TYPE_INT,    { XLANG_TYPE_VOID } },

	/* Input */
	{ "get_mouse_x",              vec_get_mouse_x,              0, XLANG_TYPE_INT, { XLANG_TYPE_VOID } },
	{ "get_mouse_y",              vec_get_mouse_y,              0, XLANG_TYPE_INT, { XLANG_TYPE_VOID } },
	{ "is_mouse_button_pressed",  vec_is_mouse_button_pressed,  1, XLANG_TYPE_INT, { XLANG_TYPE_INT } },
	{ "is_mouse_button_down",     vec_is_mouse_button_down,     1, XLANG_TYPE_INT, { XLANG_TYPE_INT } },
	{ "is_key_pressed",           vec_is_key_pressed,           1, XLANG_TYPE_INT, { XLANG_TYPE_INT } },
	{ "is_key_down",              vec_is_key_down,              1, XLANG_TYPE_INT, { XLANG_TYPE_INT } },

	/* Image ops */
	{ "gen_image_color",        vec_gen_image_color,        3, XLANG_TYPE_OBJECT, { XLANG_TYPE_INT, XLANG_TYPE_INT, XLANG_TYPE_INT } },
	{ "image_draw_rectangle",   vec_image_draw_rectangle,   6, XLANG_TYPE_VOID,   { XLANG_TYPE_OBJECT, XLANG_TYPE_INT, XLANG_TYPE_INT, XLANG_TYPE_INT, XLANG_TYPE_INT, XLANG_TYPE_INT } },
	{ "image_draw_circle",      vec_image_draw_circle,      5, XLANG_TYPE_VOID,   { XLANG_TYPE_OBJECT, XLANG_TYPE_INT, XLANG_TYPE_INT, XLANG_TYPE_FLOAT, XLANG_TYPE_INT } },
	{ "image_draw_line",        vec_image_draw_line,        6, XLANG_TYPE_VOID,   { XLANG_TYPE_OBJECT, XLANG_TYPE_INT, XLANG_TYPE_INT, XLANG_TYPE_INT, XLANG_TYPE_INT, XLANG_TYPE_INT } },
	{ "image_draw_text",        vec_image_draw_text,        6, XLANG_TYPE_VOID,   { XLANG_TYPE_OBJECT, XLANG_TYPE_STRING, XLANG_TYPE_INT, XLANG_TYPE_INT, XLANG_TYPE_INT, XLANG_TYPE_INT } },
	{ "export_image",           vec_export_image,           2, XLANG_TYPE_INT,    { XLANG_TYPE_OBJECT, XLANG_TYPE_STRING } },
	{ "take_screenshot",        vec_take_screenshot,        1, XLANG_TYPE_VOID,   { XLANG_TYPE_STRING } },
	{ "unload_image",           vec_unload_image,           1, XLANG_TYPE_VOID,   { XLANG_TYPE_OBJECT } },

	/* Flags & keys */
	{ "flag_window_hidden",   vec_flag_window_hidden,   0, XLANG_TYPE_INT, { XLANG_TYPE_VOID } },
	{ "flag_msaa_4x",         vec_flag_msaa_4x,         0, XLANG_TYPE_INT, { XLANG_TYPE_VOID } },
	{ "flag_vsync",           vec_flag_vsync,           0, XLANG_TYPE_INT, { XLANG_TYPE_VOID } },
	{ "mouse_button_left",    vec_mouse_button_left,    0, XLANG_TYPE_INT, { XLANG_TYPE_VOID } },
	{ "mouse_button_right",   vec_mouse_button_right,   0, XLANG_TYPE_INT, { XLANG_TYPE_VOID } },
	{ "key_space",            vec_key_space,            0, XLANG_TYPE_INT, { XLANG_TYPE_VOID } },
	{ "key_escape",           vec_key_escape,           0, XLANG_TYPE_INT, { XLANG_TYPE_VOID } },
	{ "key_enter",            vec_key_enter,            0, XLANG_TYPE_INT, { XLANG_TYPE_VOID } },
	{ "key_right",            vec_key_right,            0, XLANG_TYPE_INT, { XLANG_TYPE_VOID } },
	{ "key_left",             vec_key_left,             0, XLANG_TYPE_INT, { XLANG_TYPE_VOID } },
	{ "key_down",             vec_key_down,             0, XLANG_TYPE_INT, { XLANG_TYPE_VOID } },
	{ "key_up",               vec_key_up,               0, XLANG_TYPE_INT, { XLANG_TYPE_VOID } },

	{ NULL, NULL, 0, XLANG_TYPE_VOID, { XLANG_TYPE_VOID } }
};

/* Module entry point */
XLANG_EXPORT XLANG_EXTENSION_ENTRY
{
	s_ctx = ctx;
	return ctx->register_module(ctx, "raylib", s_raylib_funcs);
}
