; ModuleID = 'examples/raylib/bouncing_balls.xb'
source_filename = "examples/raylib/bouncing_balls.xb"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.Ball = type { double, double, double, double, double, i32 }
%struct.List = type { i32 }
%struct.Map = type { i32 }
%struct.ProcessResult = type { i8*, i32 }

@.str.0 = private unnamed_addr constant [36 x i8] c"xlang + Raylib: Bouncing Balls Demo\00", align 1
@.str.1 = private unnamed_addr constant [54 x i8] c"Starting Raylib animation loop (800x600 @ 60 FPS)...\0A\00", align 1
@.str.2 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@.str.3 = private unnamed_addr constant [31 x i8] c"xlang + Raylib 2D Physics Demo\00", align 1
@.str.4 = private unnamed_addr constant [56 x i8] c"Elastic Collisions: Balls bounce off walls & each other\00", align 1
@.str.5 = private unnamed_addr constant [44 x i8] c"Left Click: Teleport the red ball to cursor\00", align 1
@.str.6 = private unnamed_addr constant [34 x i8] c"Press ESC or Close Window to exit\00", align 1
@.str.7 = private unnamed_addr constant [38 x i8] c"Window closed. Raylib demo complete!\0A\00", align 1

declare i32 @printf(i8*, ...)
declare i32 @puts(i8*)
declare void @exit(i32)
declare i8* @malloc(i64)
declare i8* @calloc(i64, i64)
declare void @free(i8*)
declare i8* @gc_malloc(i64, i32)
declare i8* @gc_calloc(i64, i64, i32)
declare i32 @strcmp(i8*, i8*)
declare void @xllvm_rt_init(i32, i8**)

; String Helpers
declare i8* @_str_concat(i8*, i8*)
declare i8* @_str_from_int(i32)
declare i8* @_str_from_float(double)
declare i32 @_len(i8*)
declare i8* @_trim(i8*)
declare i8* @_lower(i8*)
declare i8* @_upper(i8*)
declare i8* @substr(i8*, i32, i32)
declare i8* @chr(i32)
declare i8* @x_readline()
declare i32 @index_of(i8*, i8*)
declare i32 @str_eq(i8*, i8*)

; Map Operations
declare i32 @map_new()
declare i32 @map_put(i32, i8*, i8*)
declare i32 @map_put_int(i32, i8*, i32)
declare i32 @map_put_float(i32, i8*, double)
declare i8* @map_get(i32, i8*)
declare i32 @map_get_int(i32, i8*)
declare double @map_get_float(i32, i8*)
declare i32 @map_has(i32, i8*)
declare i32 @map_remove(i32, i8*)
declare i32 @map_size(i32)
declare i32 @map_clear(i32)
declare i8* @map_keys(i32)
declare i8* @map_values(i32)
declare i8* @map_to_string(i32)
declare i32 @map_free(i32)
declare i32 @map_keys_list(i32)

; List Operations
declare i32 @list_new()
declare i32 @list_add(i32, i8*)
declare i32 @list_add_int(i32, i32)
declare i32 @list_add_float(i32, double)
declare i8* @list_get(i32, i32)
declare i32 @list_get_int(i32, i32)
declare double @list_get_float(i32, i32)
declare i32 @list_set(i32, i32, i8*)
declare i32 @list_set_int(i32, i32, i32)
declare i32 @list_set_float(i32, i32, double)
declare i32 @list_remove_at(i32, i32)
declare i32 @list_size(i32)
declare i32 @list_clear(i32)
declare i32 @list_contains(i32, i8*)
declare i32 @list_contains_int(i32, i32)
declare i32 @list_index_of(i32, i8*)
declare i32 @list_index_of_int(i32, i32)
declare i8* @list_pop(i32)
declare i32 @list_pop_int(i32)
declare i8* @list_join(i32, i8*)
declare i8* @list_to_string(i32)
declare i32 @list_free(i32)

; Socket Operations
declare i32 @socket_create(i8*)
declare i32 @socket_connect(i32, i8*, i32)
declare i32 @socket_bind(i32, i8*, i32)
declare i32 @socket_listen(i32, i32)
declare i32 @socket_accept(i32)
declare i32 @socket_send(i32, i8*)
declare i8* @socket_recv(i32, i32)
declare i32 @socket_close(i32)
declare i32 @socket_set_timeout(i32, i32)
declare i32 @socket_set_reuseaddr(i32, i32)
declare i32 @socket_sendto(i32, i8*, i8*, i32)
declare i8* @socket_recvfrom(i32, i32)

; System Operations
declare i32 @clock_ms()
declare i32 @get_argc()
declare i8* @get_arg(i32)
declare i32 @system_exec(i8*)
declare i8* @system_getenv(i8*)
declare i32 @system_setenv(i8*, i8*)

; Directory Operations
declare i32 @dir_create(i8*)
declare i32 @dir_exists(i8*)
declare i32 @dir_remove(i8*)
declare %struct.List* @dir_list(i8*)

; File Operations
declare i8* @file_read_all(i8*)
declare i8* @file_read(i32, i32)
declare i32 @file_write_all(i8*, i8*)
declare i32 @file_append(i8*, i8*)
declare i32 @file_create(i8*)
declare i32 @file_exists(i8*)
declare i32 @file_size(i8*)
declare i32 @file_remove(i8*)
declare i32 @file_open(i8*)
declare i32 @file_write(i32, i8*)
declare i32 @file_close(i32)
declare %struct.List* @file_list(i8*)

; Math Operations
declare double @math_abs(double)
declare double @math_sqrt(double)
declare double @math_pow(double, double)
declare double @math_min(double, double)
declare double @math_max(double, double)
declare double @math_floor(double)
declare double @math_ceil(double)
declare double @math_round(double)
declare double @math_sin(double)
declare double @math_cos(double)
declare double @math_tan(double)
declare double @math_log(double)

; GC Operations
declare i32 @gc_collect()
declare i32 @gc_allocated_bytes()
declare i32 @gc_total_objects()
declare i32 @gc_enable()
declare i32 @gc_disable()
declare i32 @gc_set_threshold(i32)
declare i32 @gc_dump()

; Process Operations
declare i8* @proc_capture(i8*)
declare %struct.ProcessResult* @proc_run(i8*)

; Regex & String Operations
declare i32 @starts_with(i8*, i8*)
declare i32 @ends_with(i8*, i8*)
declare i32 @regex_match(i8*, i8*)
declare i8* @regex_find(i8*, i8*)
declare i8* @regex_replace(i8*, i8*, i8*)
declare i32 @str_split(i8*, i8*)

; DateTime Operations
declare i32 @datetime_now()
declare i32 @datetime_year(i32)
declare i32 @datetime_month(i32)
declare i32 @datetime_day(i32)
declare i32 @datetime_hour(i32)
declare i32 @datetime_minute(i32)
declare i32 @datetime_second(i32)
declare i8* @datetime_format(i32, i8*)
declare i32 @datetime_clock_ms()

; JSON Operations
declare i32 @json_is_valid(i8*)
declare %struct.Map* @json_parse(i8*)
declare i8* @json_stringify(%struct.Map*)

; HTTP Operations
declare i8* @http_get(i8*)


; Native Extension Module Declarations
declare void @raylib_init_window(i32, i32, i8*)
declare void @raylib_close_window()
declare i32 @raylib_window_should_close()
declare i32 @raylib_is_window_ready()
declare void @raylib_set_target_fps(i32)
declare i32 @raylib_get_fps()
declare double @raylib_get_frame_time()
declare double @raylib_get_time()
declare void @raylib_set_config_flags(i32)
declare i32 @raylib_get_screen_width()
declare i32 @raylib_get_screen_height()
declare void @raylib_begin_drawing()
declare void @raylib_end_drawing()
declare void @raylib_clear_background(i32)
declare void @raylib_draw_text(i8*, i32, i32, i32, i32)
declare void @raylib_draw_fps(i32, i32)
declare void @raylib_draw_pixel(i32, i32, i32)
declare void @raylib_draw_line(i32, i32, i32, i32, i32)
declare void @raylib_draw_circle(i32, i32, double, i32)
declare void @raylib_draw_circle_lines(i32, i32, double, i32)
declare void @raylib_draw_rectangle(i32, i32, i32, i32, i32)
declare void @raylib_draw_rectangle_lines(i32, i32, i32, i32, i32)
declare i32 @raylib_color(i32, i32, i32, i32)
declare i32 @raylib_white()
declare i32 @raylib_black()
declare i32 @raylib_red()
declare i32 @raylib_green()
declare i32 @raylib_blue()
declare i32 @raylib_yellow()
declare i32 @raylib_gold()
declare i32 @raylib_raywhite()
declare i32 @raylib_darkgray()
declare i32 @raylib_lightgray()
declare i32 @raylib_skyblue()
declare i32 @raylib_get_mouse_x()
declare i32 @raylib_get_mouse_y()
declare i32 @raylib_is_mouse_button_pressed(i32)
declare i32 @raylib_is_mouse_button_down(i32)
declare i32 @raylib_is_key_pressed(i32)
declare i32 @raylib_is_key_down(i32)
declare i8* @raylib_gen_image_color(i32, i32, i32)
declare void @raylib_image_draw_rectangle(i8*, i32, i32, i32, i32, i32)
declare void @raylib_image_draw_circle(i8*, i32, i32, double, i32)
declare void @raylib_image_draw_line(i8*, i32, i32, i32, i32, i32)
declare void @raylib_image_draw_text(i8*, i8*, i32, i32, i32, i32)
declare i32 @raylib_export_image(i8*, i8*)
declare void @raylib_unload_image(i8*)
declare i32 @raylib_flag_window_hidden()
declare i32 @raylib_flag_msaa_4x()
declare i32 @raylib_flag_vsync()
declare i32 @raylib_mouse_button_left()
declare i32 @raylib_mouse_button_right()
declare i32 @raylib_key_space()
declare i32 @raylib_key_escape()
declare i32 @raylib_key_enter()
declare i32 @raylib_key_right()
declare i32 @raylib_key_left()
declare i32 @raylib_key_down()
declare i32 @raylib_key_up()
define void @Ball_Ball(%struct.Ball* %this) {
entry:
  %t0 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 0
  store double 4.000000e+02, double* %t0, align 8
  %t1 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 1
  store double 3.000000e+02, double* %t1, align 8
  %t2 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 2
  store double 4.000000e+00, double* %t2, align 8
  %t3 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 3
  store double 3.000000e+00, double* %t3, align 8
  %t4 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 4
  store double 2.400000e+01, double* %t4, align 8
  %t5 = call i32 @raylib_red()
  %t6 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 5
  store i32 %t5, i32* %t6, align 4
  ret void
}

define void @Ball_set(%struct.Ball* %this, double %init_x, double %init_y, double %init_vx, double %init_vy, double %r, i32 %c) {
entry:
  %init_x.addr = alloca double, align 8
  store double %init_x, double* %init_x.addr, align 8
  %init_y.addr = alloca double, align 8
  store double %init_y, double* %init_y.addr, align 8
  %init_vx.addr = alloca double, align 8
  store double %init_vx, double* %init_vx.addr, align 8
  %init_vy.addr = alloca double, align 8
  store double %init_vy, double* %init_vy.addr, align 8
  %r.addr = alloca double, align 8
  store double %r, double* %r.addr, align 8
  %c.addr = alloca i32, align 8
  store i32 %c, i32* %c.addr, align 8
  %t7 = load double, double* %init_x.addr, align 8
  %t8 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 0
  store double %t7, double* %t8, align 8
  %t9 = load double, double* %init_y.addr, align 8
  %t10 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 1
  store double %t9, double* %t10, align 8
  %t11 = load double, double* %init_vx.addr, align 8
  %t12 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 2
  store double %t11, double* %t12, align 8
  %t13 = load double, double* %init_vy.addr, align 8
  %t14 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 3
  store double %t13, double* %t14, align 8
  %t15 = load double, double* %r.addr, align 8
  %t16 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 4
  store double %t15, double* %t16, align 8
  %t17 = load i32, i32* %c.addr, align 4
  %t18 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 5
  store i32 %t17, i32* %t18, align 4
  ret void
}

define void @Ball_update(%struct.Ball* %this, double %screen_w, double %screen_h) {
entry:
  %screen_w.addr = alloca double, align 8
  store double %screen_w, double* %screen_w.addr, align 8
  %screen_h.addr = alloca double, align 8
  store double %screen_h, double* %screen_h.addr, align 8
  %t19 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 0
  %t20 = load double, double* %t19, align 8
  %t21 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 2
  %t22 = load double, double* %t21, align 8
  %t23 = fadd double %t20, %t22
  %t24 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 0
  store double %t23, double* %t24, align 8
  %t25 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 1
  %t26 = load double, double* %t25, align 8
  %t27 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 3
  %t28 = load double, double* %t27, align 8
  %t29 = fadd double %t26, %t28
  %t30 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 1
  store double %t29, double* %t30, align 8
  %t31 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 0
  %t32 = load double, double* %t31, align 8
  %t33 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 4
  %t34 = load double, double* %t33, align 8
  %t35 = fsub double %t32, %t34
  %t36 = fcmp ole double %t35, 0.000000e+00
  br i1 %t36, label %then.0, label %merge.0

then.0:
  %t37 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 4
  %t38 = load double, double* %t37, align 8
  %t39 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 0
  store double %t38, double* %t39, align 8
  %t40 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 2
  %t41 = load double, double* %t40, align 8
  %t42 = fsub double 0.000000e+00, %t41
  %t43 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 2
  store double %t42, double* %t43, align 8
  br label %merge.0

merge.0:
  %t44 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 0
  %t45 = load double, double* %t44, align 8
  %t46 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 4
  %t47 = load double, double* %t46, align 8
  %t48 = fadd double %t45, %t47
  %t49 = load double, double* %screen_w.addr, align 8
  %t50 = fcmp oge double %t48, %t49
  br i1 %t50, label %then.1, label %merge.1

then.1:
  %t51 = load double, double* %screen_w.addr, align 8
  %t52 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 4
  %t53 = load double, double* %t52, align 8
  %t54 = fsub double %t51, %t53
  %t55 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 0
  store double %t54, double* %t55, align 8
  %t56 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 2
  %t57 = load double, double* %t56, align 8
  %t58 = fsub double 0.000000e+00, %t57
  %t59 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 2
  store double %t58, double* %t59, align 8
  br label %merge.1

merge.1:
  %t60 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 1
  %t61 = load double, double* %t60, align 8
  %t62 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 4
  %t63 = load double, double* %t62, align 8
  %t64 = fsub double %t61, %t63
  %t65 = fcmp ole double %t64, 0.000000e+00
  br i1 %t65, label %then.2, label %merge.2

then.2:
  %t66 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 4
  %t67 = load double, double* %t66, align 8
  %t68 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 1
  store double %t67, double* %t68, align 8
  %t69 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 3
  %t70 = load double, double* %t69, align 8
  %t71 = fsub double 0.000000e+00, %t70
  %t72 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 3
  store double %t71, double* %t72, align 8
  br label %merge.2

merge.2:
  %t73 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 1
  %t74 = load double, double* %t73, align 8
  %t75 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 4
  %t76 = load double, double* %t75, align 8
  %t77 = fadd double %t74, %t76
  %t78 = load double, double* %screen_h.addr, align 8
  %t79 = fcmp oge double %t77, %t78
  br i1 %t79, label %then.3, label %merge.3

then.3:
  %t80 = load double, double* %screen_h.addr, align 8
  %t81 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 4
  %t82 = load double, double* %t81, align 8
  %t83 = fsub double %t80, %t82
  %t84 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 1
  store double %t83, double* %t84, align 8
  %t85 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 3
  %t86 = load double, double* %t85, align 8
  %t87 = fsub double 0.000000e+00, %t86
  %t88 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 3
  store double %t87, double* %t88, align 8
  br label %merge.3

merge.3:
  ret void
}

define void @Ball_resolve_collision(%struct.Ball* %this, %struct.Ball* %other) {
entry:
  %other.addr = alloca %struct.Ball*, align 8
  store %struct.Ball* %other, %struct.Ball** %other.addr, align 8
  %dx.89 = alloca double, align 8
  %dy.90 = alloca double, align 8
  %dist2.91 = alloca double, align 8
  %min_dist.92 = alloca double, align 8
  %dist.93 = alloca double, align 8
  %nx.94 = alloca double, align 8
  %ny.95 = alloca double, align 8
  %rvx.96 = alloca double, align 8
  %rvy.97 = alloca double, align 8
  %vel_along_normal.98 = alloca double, align 8
  %m1.99 = alloca double, align 8
  %m2.100 = alloca double, align 8
  %impulse.101 = alloca double, align 8
  %overlap.102 = alloca double, align 8
  %t103 = load %struct.Ball*, %struct.Ball** %other.addr, align 8
  %t104 = getelementptr inbounds %struct.Ball, %struct.Ball* %t103, i32 0, i32 0
  %t105 = load double, double* %t104, align 8
  %t106 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 0
  %t107 = load double, double* %t106, align 8
  %t108 = fsub double %t105, %t107
  store double %t108, double* %dx.89, align 8
  %t109 = load %struct.Ball*, %struct.Ball** %other.addr, align 8
  %t110 = getelementptr inbounds %struct.Ball, %struct.Ball* %t109, i32 0, i32 1
  %t111 = load double, double* %t110, align 8
  %t112 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 1
  %t113 = load double, double* %t112, align 8
  %t114 = fsub double %t111, %t113
  store double %t114, double* %dy.90, align 8
  %t115 = load double, double* %dx.89, align 8
  %t116 = load double, double* %dx.89, align 8
  %t117 = fmul double %t115, %t116
  %t118 = load double, double* %dy.90, align 8
  %t119 = load double, double* %dy.90, align 8
  %t120 = fmul double %t118, %t119
  %t121 = fadd double %t117, %t120
  store double %t121, double* %dist2.91, align 8
  %t122 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 4
  %t123 = load double, double* %t122, align 8
  %t124 = load %struct.Ball*, %struct.Ball** %other.addr, align 8
  %t125 = getelementptr inbounds %struct.Ball, %struct.Ball* %t124, i32 0, i32 4
  %t126 = load double, double* %t125, align 8
  %t127 = fadd double %t123, %t126
  store double %t127, double* %min_dist.92, align 8
  %t128 = load double, double* %dist2.91, align 8
  %t129 = load double, double* %min_dist.92, align 8
  %t130 = load double, double* %min_dist.92, align 8
  %t131 = fmul double %t129, %t130
  %t132 = fcmp olt double %t128, %t131
  br i1 %t132, label %then.4, label %merge.4

then.4:
  %t133 = load double, double* %dist2.91, align 8
  %t134 = fcmp ogt double %t133, 1.000000e-04
  br i1 %t134, label %then.5, label %merge.5

then.5:
  %t135 = load double, double* %dist2.91, align 8
  %t136 = call double @math_sqrt(double %t135)
  store double %t136, double* %dist.93, align 8
  %t137 = load double, double* %dx.89, align 8
  %t138 = load double, double* %dist.93, align 8
  %t139 = fdiv double %t137, %t138
  store double %t139, double* %nx.94, align 8
  %t140 = load double, double* %dy.90, align 8
  %t141 = load double, double* %dist.93, align 8
  %t142 = fdiv double %t140, %t141
  store double %t142, double* %ny.95, align 8
  %t143 = load %struct.Ball*, %struct.Ball** %other.addr, align 8
  %t144 = getelementptr inbounds %struct.Ball, %struct.Ball* %t143, i32 0, i32 2
  %t145 = load double, double* %t144, align 8
  %t146 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 2
  %t147 = load double, double* %t146, align 8
  %t148 = fsub double %t145, %t147
  store double %t148, double* %rvx.96, align 8
  %t149 = load %struct.Ball*, %struct.Ball** %other.addr, align 8
  %t150 = getelementptr inbounds %struct.Ball, %struct.Ball* %t149, i32 0, i32 3
  %t151 = load double, double* %t150, align 8
  %t152 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 3
  %t153 = load double, double* %t152, align 8
  %t154 = fsub double %t151, %t153
  store double %t154, double* %rvy.97, align 8
  %t155 = load double, double* %rvx.96, align 8
  %t156 = load double, double* %nx.94, align 8
  %t157 = fmul double %t155, %t156
  %t158 = load double, double* %rvy.97, align 8
  %t159 = load double, double* %ny.95, align 8
  %t160 = fmul double %t158, %t159
  %t161 = fadd double %t157, %t160
  store double %t161, double* %vel_along_normal.98, align 8
  %t162 = load double, double* %vel_along_normal.98, align 8
  %t163 = fcmp olt double %t162, 0.000000e+00
  br i1 %t163, label %then.6, label %merge.6

then.6:
  %t164 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 4
  %t165 = load double, double* %t164, align 8
  %t166 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 4
  %t167 = load double, double* %t166, align 8
  %t168 = fmul double %t165, %t167
  store double %t168, double* %m1.99, align 8
  %t169 = load %struct.Ball*, %struct.Ball** %other.addr, align 8
  %t170 = getelementptr inbounds %struct.Ball, %struct.Ball* %t169, i32 0, i32 4
  %t171 = load double, double* %t170, align 8
  %t172 = load %struct.Ball*, %struct.Ball** %other.addr, align 8
  %t173 = getelementptr inbounds %struct.Ball, %struct.Ball* %t172, i32 0, i32 4
  %t174 = load double, double* %t173, align 8
  %t175 = fmul double %t171, %t174
  store double %t175, double* %m2.100, align 8
  %t176 = load double, double* %vel_along_normal.98, align 8
  %t177 = fmul double 2.000000e+00, %t176
  %t178 = fsub double 0.000000e+00, %t177
  %t179 = load double, double* %m1.99, align 8
  %t180 = load double, double* %m2.100, align 8
  %t181 = fadd double %t179, %t180
  %t182 = fdiv double %t178, %t181
  store double %t182, double* %impulse.101, align 8
  %t183 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 2
  %t184 = load double, double* %t183, align 8
  %t185 = load double, double* %impulse.101, align 8
  %t186 = load double, double* %m2.100, align 8
  %t187 = fmul double %t185, %t186
  %t188 = load double, double* %nx.94, align 8
  %t189 = fmul double %t187, %t188
  %t190 = fsub double %t184, %t189
  %t191 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 2
  store double %t190, double* %t191, align 8
  %t192 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 3
  %t193 = load double, double* %t192, align 8
  %t194 = load double, double* %impulse.101, align 8
  %t195 = load double, double* %m2.100, align 8
  %t196 = fmul double %t194, %t195
  %t197 = load double, double* %ny.95, align 8
  %t198 = fmul double %t196, %t197
  %t199 = fsub double %t193, %t198
  %t200 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 3
  store double %t199, double* %t200, align 8
  %t201 = load %struct.Ball*, %struct.Ball** %other.addr, align 8
  %t202 = getelementptr inbounds %struct.Ball, %struct.Ball* %t201, i32 0, i32 2
  %t203 = load double, double* %t202, align 8
  %t204 = load double, double* %impulse.101, align 8
  %t205 = load double, double* %m1.99, align 8
  %t206 = fmul double %t204, %t205
  %t207 = load double, double* %nx.94, align 8
  %t208 = fmul double %t206, %t207
  %t209 = fadd double %t203, %t208
  %t210 = load %struct.Ball*, %struct.Ball** %other.addr, align 8
  %t211 = getelementptr inbounds %struct.Ball, %struct.Ball* %t210, i32 0, i32 2
  store double %t209, double* %t211, align 8
  %t212 = load %struct.Ball*, %struct.Ball** %other.addr, align 8
  %t213 = getelementptr inbounds %struct.Ball, %struct.Ball* %t212, i32 0, i32 3
  %t214 = load double, double* %t213, align 8
  %t215 = load double, double* %impulse.101, align 8
  %t216 = load double, double* %m1.99, align 8
  %t217 = fmul double %t215, %t216
  %t218 = load double, double* %ny.95, align 8
  %t219 = fmul double %t217, %t218
  %t220 = fadd double %t214, %t219
  %t221 = load %struct.Ball*, %struct.Ball** %other.addr, align 8
  %t222 = getelementptr inbounds %struct.Ball, %struct.Ball* %t221, i32 0, i32 3
  store double %t220, double* %t222, align 8
  %t223 = load double, double* %min_dist.92, align 8
  %t224 = load double, double* %dist.93, align 8
  %t225 = fsub double %t223, %t224
  %t226 = fmul double 5.000000e-01, %t225
  store double %t226, double* %overlap.102, align 8
  %t227 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 0
  %t228 = load double, double* %t227, align 8
  %t229 = load double, double* %overlap.102, align 8
  %t230 = load double, double* %nx.94, align 8
  %t231 = fmul double %t229, %t230
  %t232 = fsub double %t228, %t231
  %t233 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 0
  store double %t232, double* %t233, align 8
  %t234 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 1
  %t235 = load double, double* %t234, align 8
  %t236 = load double, double* %overlap.102, align 8
  %t237 = load double, double* %ny.95, align 8
  %t238 = fmul double %t236, %t237
  %t239 = fsub double %t235, %t238
  %t240 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 1
  store double %t239, double* %t240, align 8
  %t241 = load %struct.Ball*, %struct.Ball** %other.addr, align 8
  %t242 = getelementptr inbounds %struct.Ball, %struct.Ball* %t241, i32 0, i32 0
  %t243 = load double, double* %t242, align 8
  %t244 = load double, double* %overlap.102, align 8
  %t245 = load double, double* %nx.94, align 8
  %t246 = fmul double %t244, %t245
  %t247 = fadd double %t243, %t246
  %t248 = load %struct.Ball*, %struct.Ball** %other.addr, align 8
  %t249 = getelementptr inbounds %struct.Ball, %struct.Ball* %t248, i32 0, i32 0
  store double %t247, double* %t249, align 8
  %t250 = load %struct.Ball*, %struct.Ball** %other.addr, align 8
  %t251 = getelementptr inbounds %struct.Ball, %struct.Ball* %t250, i32 0, i32 1
  %t252 = load double, double* %t251, align 8
  %t253 = load double, double* %overlap.102, align 8
  %t254 = load double, double* %ny.95, align 8
  %t255 = fmul double %t253, %t254
  %t256 = fadd double %t252, %t255
  %t257 = load %struct.Ball*, %struct.Ball** %other.addr, align 8
  %t258 = getelementptr inbounds %struct.Ball, %struct.Ball* %t257, i32 0, i32 1
  store double %t256, double* %t258, align 8
  br label %merge.6

merge.6:
  br label %merge.5

merge.5:
  br label %merge.4

merge.4:
  ret void
}

define void @Ball_draw(%struct.Ball* %this) {
entry:
  %ix.259 = alloca i32, align 8
  %iy.260 = alloca i32, align 8
  %t261 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 0
  %t262 = load double, double* %t261, align 8
  %t263 = fptosi double %t262 to i32
  store i32 %t263, i32* %ix.259, align 8
  %t264 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 1
  %t265 = load double, double* %t264, align 8
  %t266 = fptosi double %t265 to i32
  store i32 %t266, i32* %iy.260, align 8
  %t267 = load i32, i32* %ix.259, align 4
  %t268 = load i32, i32* %iy.260, align 4
  %t269 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 4
  %t270 = load double, double* %t269, align 8
  %t271 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 5
  %t272 = load i32, i32* %t271, align 4
  call void @raylib_draw_circle(i32 %t267, i32 %t268, double %t270, i32 %t272)
  %t273 = load i32, i32* %ix.259, align 4
  %t274 = load i32, i32* %iy.260, align 4
  %t275 = getelementptr inbounds %struct.Ball, %struct.Ball* %this, i32 0, i32 4
  %t276 = load double, double* %t275, align 8
  %t277 = call i32 @raylib_white()
  call void @raylib_draw_circle_lines(i32 %t273, i32 %t274, double %t276, i32 %t277)
  ret void
}

define i32 @main(i32 %argc, i8** %argv) {
entry:
  call void @xllvm_rt_init(i32 %argc, i8** %argv)
  %screen_w.278 = alloca i32, align 8
  store i32 800, i32* %screen_w.278, align 8
  %screen_h.279 = alloca i32, align 8
  store i32 600, i32* %screen_h.279, align 8
  %t280 = call i32 @raylib_flag_msaa_4x()
  %t281 = call i32 @raylib_flag_vsync()
  %t282 = add nsw i32 %t280, %t281
  call void @raylib_set_config_flags(i32 %t282)
  %t283 = load i32, i32* %screen_w.278, align 4
  %t284 = load i32, i32* %screen_h.279, align 4
  %t285 = getelementptr inbounds [36 x i8], [36 x i8]* @.str.0, i64 0, i64 0
  call void @raylib_init_window(i32 %t283, i32 %t284, i8* %t285)
  call void @raylib_set_target_fps(i32 60)
  %b1.286 = alloca %struct.Ball*, align 8
  %t287 = getelementptr %struct.Ball, %struct.Ball* null, i32 1
  %t288 = ptrtoint %struct.Ball* %t287 to i64
  %t289 = call i8* @calloc(i64 1, i64 %t288)
  %t290 = bitcast i8* %t289 to %struct.Ball*
  call void @Ball_Ball(%struct.Ball* %t290)
  store %struct.Ball* %t290, %struct.Ball** %b1.286, align 8
  %t291 = load %struct.Ball*, %struct.Ball** %b1.286, align 8
  %t292 = call i32 @raylib_red()
  call void @Ball_set(%struct.Ball* %t291, double 1.500000e+02, double 1.500000e+02, double 4.500000e+00, double 3.200000e+00, double 2.800000e+01, i32 %t292)
  %b2.293 = alloca %struct.Ball*, align 8
  %t294 = getelementptr %struct.Ball, %struct.Ball* null, i32 1
  %t295 = ptrtoint %struct.Ball* %t294 to i64
  %t296 = call i8* @calloc(i64 1, i64 %t295)
  %t297 = bitcast i8* %t296 to %struct.Ball*
  call void @Ball_Ball(%struct.Ball* %t297)
  store %struct.Ball* %t297, %struct.Ball** %b2.293, align 8
  %t298 = load %struct.Ball*, %struct.Ball** %b2.293, align 8
  %t299 = fneg double 3.800000e+00
  %t300 = call i32 @raylib_skyblue()
  call void @Ball_set(%struct.Ball* %t298, double 4.000000e+02, double 2.500000e+02, double %t299, double 4.800000e+00, double 2.200000e+01, i32 %t300)
  %b3.301 = alloca %struct.Ball*, align 8
  %t302 = getelementptr %struct.Ball, %struct.Ball* null, i32 1
  %t303 = ptrtoint %struct.Ball* %t302 to i64
  %t304 = call i8* @calloc(i64 1, i64 %t303)
  %t305 = bitcast i8* %t304 to %struct.Ball*
  call void @Ball_Ball(%struct.Ball* %t305)
  store %struct.Ball* %t305, %struct.Ball** %b3.301, align 8
  %t306 = load %struct.Ball*, %struct.Ball** %b3.301, align 8
  %t307 = fneg double 3.900000e+00
  %t308 = call i32 @raylib_gold()
  call void @Ball_set(%struct.Ball* %t306, double 6.500000e+02, double 4.000000e+02, double 5.200000e+00, double %t307, double 3.400000e+01, i32 %t308)
  %b4.309 = alloca %struct.Ball*, align 8
  %t310 = getelementptr %struct.Ball, %struct.Ball* null, i32 1
  %t311 = ptrtoint %struct.Ball* %t310 to i64
  %t312 = call i8* @calloc(i64 1, i64 %t311)
  %t313 = bitcast i8* %t312 to %struct.Ball*
  call void @Ball_Ball(%struct.Ball* %t313)
  store %struct.Ball* %t313, %struct.Ball** %b4.309, align 8
  %t314 = load %struct.Ball*, %struct.Ball** %b4.309, align 8
  %t315 = fneg double 4.200000e+00
  %t316 = fneg double 3.600000e+00
  %t317 = call i32 @raylib_green()
  call void @Ball_set(%struct.Ball* %t314, double 3.000000e+02, double 5.000000e+02, double %t315, double %t316, double 1.800000e+01, i32 %t317)
  %b5.318 = alloca %struct.Ball*, align 8
  %t319 = getelementptr %struct.Ball, %struct.Ball* null, i32 1
  %t320 = ptrtoint %struct.Ball* %t319 to i64
  %t321 = call i8* @calloc(i64 1, i64 %t320)
  %t322 = bitcast i8* %t321 to %struct.Ball*
  call void @Ball_Ball(%struct.Ball* %t322)
  store %struct.Ball* %t322, %struct.Ball** %b5.318, align 8
  %t323 = load %struct.Ball*, %struct.Ball** %b5.318, align 8
  %t324 = fneg double 3.500000e+00
  %t325 = call i32 @raylib_color(i32 200, i32 80, i32 240, i32 255)
  call void @Ball_set(%struct.Ball* %t323, double 5.200000e+02, double 1.800000e+02, double %t324, double 4.100000e+00, double 2.600000e+01, i32 %t325)
  %bg_color.326 = alloca i32, align 8
  %t327 = call i32 @raylib_color(i32 24, i32 28, i32 36, i32 255)
  store i32 %t327, i32* %bg_color.326, align 8
  %panel_color.328 = alloca i32, align 8
  %t329 = call i32 @raylib_color(i32 38, i32 43, i32 56, i32 230)
  store i32 %t329, i32* %panel_color.328, align 8
  %grid_color.330 = alloca i32, align 8
  %t331 = call i32 @raylib_color(i32 35, i32 41, i32 54, i32 255)
  store i32 %t331, i32* %grid_color.330, align 8
  %t332 = getelementptr inbounds [54 x i8], [54 x i8]* @.str.1, i64 0, i64 0
  call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str.2, i64 0, i64 0), i8* %t332)
  br label %while.cond.7

while.cond.7:
  %t333 = call i32 @raylib_window_should_close()
  %t334 = icmp eq i32 %t333, 0
  br i1 %t334, label %while.body.7, label %while.end.7

while.body.7:
  %sw.335 = alloca double, align 8
  store double 8.000000e+02, double* %sw.335, align 8
  %sh.336 = alloca double, align 8
  store double 6.000000e+02, double* %sh.336, align 8
  %t337 = load %struct.Ball*, %struct.Ball** %b1.286, align 8
  %t338 = load double, double* %sw.335, align 8
  %t339 = load double, double* %sh.336, align 8
  call void @Ball_update(%struct.Ball* %t337, double %t338, double %t339)
  %t340 = load %struct.Ball*, %struct.Ball** %b2.293, align 8
  %t341 = load double, double* %sw.335, align 8
  %t342 = load double, double* %sh.336, align 8
  call void @Ball_update(%struct.Ball* %t340, double %t341, double %t342)
  %t343 = load %struct.Ball*, %struct.Ball** %b3.301, align 8
  %t344 = load double, double* %sw.335, align 8
  %t345 = load double, double* %sh.336, align 8
  call void @Ball_update(%struct.Ball* %t343, double %t344, double %t345)
  %t346 = load %struct.Ball*, %struct.Ball** %b4.309, align 8
  %t347 = load double, double* %sw.335, align 8
  %t348 = load double, double* %sh.336, align 8
  call void @Ball_update(%struct.Ball* %t346, double %t347, double %t348)
  %t349 = load %struct.Ball*, %struct.Ball** %b5.318, align 8
  %t350 = load double, double* %sw.335, align 8
  %t351 = load double, double* %sh.336, align 8
  call void @Ball_update(%struct.Ball* %t349, double %t350, double %t351)
  %t352 = load %struct.Ball*, %struct.Ball** %b1.286, align 8
  %t353 = load %struct.Ball*, %struct.Ball** %b2.293, align 8
  call void @Ball_resolve_collision(%struct.Ball* %t352, %struct.Ball* %t353)
  %t354 = load %struct.Ball*, %struct.Ball** %b1.286, align 8
  %t355 = load %struct.Ball*, %struct.Ball** %b3.301, align 8
  call void @Ball_resolve_collision(%struct.Ball* %t354, %struct.Ball* %t355)
  %t356 = load %struct.Ball*, %struct.Ball** %b1.286, align 8
  %t357 = load %struct.Ball*, %struct.Ball** %b4.309, align 8
  call void @Ball_resolve_collision(%struct.Ball* %t356, %struct.Ball* %t357)
  %t358 = load %struct.Ball*, %struct.Ball** %b1.286, align 8
  %t359 = load %struct.Ball*, %struct.Ball** %b5.318, align 8
  call void @Ball_resolve_collision(%struct.Ball* %t358, %struct.Ball* %t359)
  %t360 = load %struct.Ball*, %struct.Ball** %b2.293, align 8
  %t361 = load %struct.Ball*, %struct.Ball** %b3.301, align 8
  call void @Ball_resolve_collision(%struct.Ball* %t360, %struct.Ball* %t361)
  %t362 = load %struct.Ball*, %struct.Ball** %b2.293, align 8
  %t363 = load %struct.Ball*, %struct.Ball** %b4.309, align 8
  call void @Ball_resolve_collision(%struct.Ball* %t362, %struct.Ball* %t363)
  %t364 = load %struct.Ball*, %struct.Ball** %b2.293, align 8
  %t365 = load %struct.Ball*, %struct.Ball** %b5.318, align 8
  call void @Ball_resolve_collision(%struct.Ball* %t364, %struct.Ball* %t365)
  %t366 = load %struct.Ball*, %struct.Ball** %b3.301, align 8
  %t367 = load %struct.Ball*, %struct.Ball** %b4.309, align 8
  call void @Ball_resolve_collision(%struct.Ball* %t366, %struct.Ball* %t367)
  %t368 = load %struct.Ball*, %struct.Ball** %b3.301, align 8
  %t369 = load %struct.Ball*, %struct.Ball** %b5.318, align 8
  call void @Ball_resolve_collision(%struct.Ball* %t368, %struct.Ball* %t369)
  %t370 = load %struct.Ball*, %struct.Ball** %b4.309, align 8
  %t371 = load %struct.Ball*, %struct.Ball** %b5.318, align 8
  call void @Ball_resolve_collision(%struct.Ball* %t370, %struct.Ball* %t371)
  %t372 = call i32 @raylib_mouse_button_left()
  %t373 = call i32 @raylib_is_mouse_button_pressed(i32 %t372)
  %t374 = icmp ne i32 %t373, 0
  br i1 %t374, label %then.8, label %merge.8

then.8:
  %mx.375 = alloca i32, align 8
  %t376 = call i32 @raylib_get_mouse_x()
  store i32 %t376, i32* %mx.375, align 8
  %my.377 = alloca i32, align 8
  %t378 = call i32 @raylib_get_mouse_y()
  store i32 %t378, i32* %my.377, align 8
  %t379 = load i32, i32* %mx.375, align 4
  %t380 = load %struct.Ball*, %struct.Ball** %b1.286, align 8
  %t382 = sitofp i32 %t379 to double
  %t381 = getelementptr inbounds %struct.Ball, %struct.Ball* %t380, i32 0, i32 0
  store double %t382, double* %t381, align 8
  %t383 = load i32, i32* %my.377, align 4
  %t384 = load %struct.Ball*, %struct.Ball** %b1.286, align 8
  %t386 = sitofp i32 %t383 to double
  %t385 = getelementptr inbounds %struct.Ball, %struct.Ball* %t384, i32 0, i32 1
  store double %t386, double* %t385, align 8
  br label %merge.8

merge.8:
  call void @raylib_begin_drawing()
  %t387 = load i32, i32* %bg_color.326, align 4
  call void @raylib_clear_background(i32 %t387)
  %gx.388 = alloca i32, align 8
  store i32 0, i32* %gx.388, align 8
  br label %while.cond.9

while.cond.9:
  %t389 = load i32, i32* %gx.388, align 4
  %t390 = load i32, i32* %screen_w.278, align 4
  %t391 = icmp slt i32 %t389, %t390
  br i1 %t391, label %while.body.9, label %while.end.9

while.body.9:
  %t392 = load i32, i32* %gx.388, align 4
  %t393 = load i32, i32* %gx.388, align 4
  %t394 = load i32, i32* %screen_h.279, align 4
  %t395 = load i32, i32* %grid_color.330, align 4
  call void @raylib_draw_line(i32 %t392, i32 0, i32 %t393, i32 %t394, i32 %t395)
  %t396 = load i32, i32* %gx.388, align 4
  %t397 = add nsw i32 %t396, 40
  store i32 %t397, i32* %gx.388, align 4
  br label %while.cond.9

while.end.9:
  %gy.398 = alloca i32, align 8
  store i32 0, i32* %gy.398, align 8
  br label %while.cond.10

while.cond.10:
  %t399 = load i32, i32* %gy.398, align 4
  %t400 = load i32, i32* %screen_h.279, align 4
  %t401 = icmp slt i32 %t399, %t400
  br i1 %t401, label %while.body.10, label %while.end.10

while.body.10:
  %t402 = load i32, i32* %gy.398, align 4
  %t403 = load i32, i32* %screen_w.278, align 4
  %t404 = load i32, i32* %gy.398, align 4
  %t405 = load i32, i32* %grid_color.330, align 4
  call void @raylib_draw_line(i32 0, i32 %t402, i32 %t403, i32 %t404, i32 %t405)
  %t406 = load i32, i32* %gy.398, align 4
  %t407 = add nsw i32 %t406, 40
  store i32 %t407, i32* %gy.398, align 4
  br label %while.cond.10

while.end.10:
  %t408 = load %struct.Ball*, %struct.Ball** %b1.286, align 8
  call void @Ball_draw(%struct.Ball* %t408)
  %t409 = load %struct.Ball*, %struct.Ball** %b2.293, align 8
  call void @Ball_draw(%struct.Ball* %t409)
  %t410 = load %struct.Ball*, %struct.Ball** %b3.301, align 8
  call void @Ball_draw(%struct.Ball* %t410)
  %t411 = load %struct.Ball*, %struct.Ball** %b4.309, align 8
  call void @Ball_draw(%struct.Ball* %t411)
  %t412 = load %struct.Ball*, %struct.Ball** %b5.318, align 8
  call void @Ball_draw(%struct.Ball* %t412)
  %t413 = load i32, i32* %panel_color.328, align 4
  call void @raylib_draw_rectangle(i32 20, i32 20, i32 430, i32 130, i32 %t413)
  %t414 = call i32 @raylib_color(i32 80, i32 95, i32 120, i32 255)
  call void @raylib_draw_rectangle_lines(i32 20, i32 20, i32 430, i32 130, i32 %t414)
  %t415 = getelementptr inbounds [31 x i8], [31 x i8]* @.str.3, i64 0, i64 0
  %t416 = call i32 @raylib_raywhite()
  call void @raylib_draw_text(i8* %t415, i32 35, i32 32, i32 20, i32 %t416)
  %t417 = getelementptr inbounds [56 x i8], [56 x i8]* @.str.4, i64 0, i64 0
  %t418 = call i32 @raylib_lightgray()
  call void @raylib_draw_text(i8* %t417, i32 35, i32 60, i32 13, i32 %t418)
  %t419 = getelementptr inbounds [44 x i8], [44 x i8]* @.str.5, i64 0, i64 0
  %t420 = call i32 @raylib_lightgray()
  call void @raylib_draw_text(i8* %t419, i32 35, i32 80, i32 13, i32 %t420)
  %t421 = getelementptr inbounds [34 x i8], [34 x i8]* @.str.6, i64 0, i64 0
  %t422 = call i32 @raylib_lightgray()
  call void @raylib_draw_text(i8* %t421, i32 35, i32 100, i32 13, i32 %t422)
  call void @raylib_draw_fps(i32 35, i32 122)
  call void @raylib_end_drawing()
  br label %while.cond.7

while.end.7:
  call void @raylib_close_window()
  %t423 = getelementptr inbounds [38 x i8], [38 x i8]* @.str.7, i64 0, i64 0
  call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str.2, i64 0, i64 0), i8* %t423)
  ret i32 0
}
