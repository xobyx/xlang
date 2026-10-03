; ModuleID = 'examples/raylib/dino_run.xb'
source_filename = "examples/raylib/dino_run.xb"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.Cloud = type { double, double, double }
%struct.GroundTile = type { double, i32, i32 }
%struct.Obstacle = type { double, double, double, i32, i32, i32, i32 }
%struct.Dino = type { double, double, double, double, double, double, i1, i1, i32, i32, i32 }
%struct.List = type { i32 }
%struct.Map = type { i32 }
%struct.ProcessResult = type { i8*, i32 }

@.str.0 = private unnamed_addr constant [37 x i8] c"xlang Dino Run - Google Chrome Clone\00", align 1
@.str.1 = private unnamed_addr constant [51 x i8] c"==================================================\00", align 1
@.str.2 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@.str.3 = private unnamed_addr constant [51 x i8] c"  xlang Dino Run: Game Launched Successfully!     \00", align 1
@.str.4 = private unnamed_addr constant [1 x i8] c"\00", align 1
@.str.5 = private unnamed_addr constant [4 x i8] c"HI \00", align 1
@.str.6 = private unnamed_addr constant [38 x i8] c"P R E S S   S P A C E   T O   J U M P\00", align 1
@.str.7 = private unnamed_addr constant [55 x i8] c"Controls: [SPACE / UP] Jump   [DOWN] Duck   [ESC] Exit\00", align 1
@.str.8 = private unnamed_addr constant [26 x i8] c"G  A  M  E     O  V  E  R\00", align 1
@.str.9 = private unnamed_addr constant [32 x i8] c"Press SPACE or CLICK to Restart\00", align 1
@.str.10 = private unnamed_addr constant [40 x i8] c"Dino Run session ended. High score: %d\0A\00", align 1

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
declare void @raylib_take_screenshot(i8*)
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
define void @Cloud_Cloud(%struct.Cloud* %this) {
entry:
  %t0 = getelementptr inbounds %struct.Cloud, %struct.Cloud* %this, i32 0, i32 0
  store double 0.000000e+00, double* %t0, align 8
  %t1 = getelementptr inbounds %struct.Cloud, %struct.Cloud* %this, i32 0, i32 1
  store double 0.000000e+00, double* %t1, align 8
  %t2 = getelementptr inbounds %struct.Cloud, %struct.Cloud* %this, i32 0, i32 2
  store double 1.000000e+00, double* %t2, align 8
  ret void
}

define void @Cloud_reset(%struct.Cloud* %this, double %start_x, double %start_y, double %spd) {
entry:
  %start_x.addr = alloca double, align 8
  store double %start_x, double* %start_x.addr, align 8
  %start_y.addr = alloca double, align 8
  store double %start_y, double* %start_y.addr, align 8
  %spd.addr = alloca double, align 8
  store double %spd, double* %spd.addr, align 8
  %t3 = load double, double* %start_x.addr, align 8
  %t4 = getelementptr inbounds %struct.Cloud, %struct.Cloud* %this, i32 0, i32 0
  store double %t3, double* %t4, align 8
  %t5 = load double, double* %start_y.addr, align 8
  %t6 = getelementptr inbounds %struct.Cloud, %struct.Cloud* %this, i32 0, i32 1
  store double %t5, double* %t6, align 8
  %t7 = load double, double* %spd.addr, align 8
  %t8 = getelementptr inbounds %struct.Cloud, %struct.Cloud* %this, i32 0, i32 2
  store double %t7, double* %t8, align 8
  ret void
}

define void @Cloud_update(%struct.Cloud* %this) {
entry:
  %t9 = getelementptr inbounds %struct.Cloud, %struct.Cloud* %this, i32 0, i32 0
  %t10 = load double, double* %t9, align 8
  %t11 = getelementptr inbounds %struct.Cloud, %struct.Cloud* %this, i32 0, i32 2
  %t12 = load double, double* %t11, align 8
  %t13 = fsub double %t10, %t12
  %t14 = getelementptr inbounds %struct.Cloud, %struct.Cloud* %this, i32 0, i32 0
  store double %t13, double* %t14, align 8
  %t15 = getelementptr inbounds %struct.Cloud, %struct.Cloud* %this, i32 0, i32 0
  %t16 = load double, double* %t15, align 8
  %t17 = fneg double 8.000000e+01
  %t18 = fcmp olt double %t16, %t17
  br i1 %t18, label %then.0, label %merge.0

then.0:
  %t19 = getelementptr inbounds %struct.Cloud, %struct.Cloud* %this, i32 0, i32 0
  store double 8.500000e+02, double* %t19, align 8
  br label %merge.0

merge.0:
  ret void
}

define void @Cloud_draw(%struct.Cloud* %this, i32 %col) {
entry:
  %col.addr = alloca i32, align 8
  store i32 %col, i32* %col.addr, align 8
  %ix.20 = alloca i32, align 8
  %iy.21 = alloca i32, align 8
  %t22 = getelementptr inbounds %struct.Cloud, %struct.Cloud* %this, i32 0, i32 0
  %t23 = load double, double* %t22, align 8
  %t24 = fptosi double %t23 to i32
  store i32 %t24, i32* %ix.20, align 8
  %t25 = getelementptr inbounds %struct.Cloud, %struct.Cloud* %this, i32 0, i32 1
  %t26 = load double, double* %t25, align 8
  %t27 = fptosi double %t26 to i32
  store i32 %t27, i32* %iy.21, align 8
  %t28 = load i32, i32* %ix.20, align 4
  %t29 = add nsw i32 %t28, 16
  %t30 = load i32, i32* %iy.21, align 4
  %t31 = add nsw i32 %t30, 10
  %t32 = load i32, i32* %col.addr, align 4
  call void @raylib_draw_circle(i32 %t29, i32 %t31, double 1.000000e+01, i32 %t32)
  %t33 = load i32, i32* %ix.20, align 4
  %t34 = add nsw i32 %t33, 28
  %t35 = load i32, i32* %iy.21, align 4
  %t36 = add nsw i32 %t35, 6
  %t37 = load i32, i32* %col.addr, align 4
  call void @raylib_draw_circle(i32 %t34, i32 %t36, double 1.300000e+01, i32 %t37)
  %t38 = load i32, i32* %ix.20, align 4
  %t39 = add nsw i32 %t38, 42
  %t40 = load i32, i32* %iy.21, align 4
  %t41 = add nsw i32 %t40, 10
  %t42 = load i32, i32* %col.addr, align 4
  call void @raylib_draw_circle(i32 %t39, i32 %t41, double 9.000000e+00, i32 %t42)
  %t43 = load i32, i32* %ix.20, align 4
  %t44 = add nsw i32 %t43, 12
  %t45 = load i32, i32* %iy.21, align 4
  %t46 = add nsw i32 %t45, 8
  %t47 = load i32, i32* %col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t44, i32 %t46, i32 36, i32 10, i32 %t47)
  ret void
}

define void @GroundTile_GroundTile(%struct.GroundTile* %this) {
entry:
  %t48 = getelementptr inbounds %struct.GroundTile, %struct.GroundTile* %this, i32 0, i32 0
  store double 0.000000e+00, double* %t48, align 8
  %t49 = getelementptr inbounds %struct.GroundTile, %struct.GroundTile* %this, i32 0, i32 1
  store i32 12, i32* %t49, align 4
  %t50 = getelementptr inbounds %struct.GroundTile, %struct.GroundTile* %this, i32 0, i32 2
  store i32 4, i32* %t50, align 4
  ret void
}

define void @GroundTile_init(%struct.GroundTile* %this, double %init_x, i32 %len, i32 %oy) {
entry:
  %init_x.addr = alloca double, align 8
  store double %init_x, double* %init_x.addr, align 8
  %len.addr = alloca i32, align 8
  store i32 %len, i32* %len.addr, align 8
  %oy.addr = alloca i32, align 8
  store i32 %oy, i32* %oy.addr, align 8
  %t51 = load double, double* %init_x.addr, align 8
  %t52 = getelementptr inbounds %struct.GroundTile, %struct.GroundTile* %this, i32 0, i32 0
  store double %t51, double* %t52, align 8
  %t53 = load i32, i32* %len.addr, align 4
  %t54 = getelementptr inbounds %struct.GroundTile, %struct.GroundTile* %this, i32 0, i32 1
  store i32 %t53, i32* %t54, align 4
  %t55 = load i32, i32* %oy.addr, align 4
  %t56 = getelementptr inbounds %struct.GroundTile, %struct.GroundTile* %this, i32 0, i32 2
  store i32 %t55, i32* %t56, align 4
  ret void
}

define void @GroundTile_update(%struct.GroundTile* %this, double %speed) {
entry:
  %speed.addr = alloca double, align 8
  store double %speed, double* %speed.addr, align 8
  %t57 = getelementptr inbounds %struct.GroundTile, %struct.GroundTile* %this, i32 0, i32 0
  %t58 = load double, double* %t57, align 8
  %t59 = load double, double* %speed.addr, align 8
  %t60 = fsub double %t58, %t59
  %t61 = getelementptr inbounds %struct.GroundTile, %struct.GroundTile* %this, i32 0, i32 0
  store double %t60, double* %t61, align 8
  %t62 = getelementptr inbounds %struct.GroundTile, %struct.GroundTile* %this, i32 0, i32 0
  %t63 = load double, double* %t62, align 8
  %t64 = fneg double 5.000000e+01
  %t65 = fcmp olt double %t63, %t64
  br i1 %t65, label %then.1, label %merge.1

then.1:
  %t66 = getelementptr inbounds %struct.GroundTile, %struct.GroundTile* %this, i32 0, i32 0
  %t67 = load double, double* %t66, align 8
  %t68 = fadd double %t67, 8.500000e+02
  %t69 = getelementptr inbounds %struct.GroundTile, %struct.GroundTile* %this, i32 0, i32 0
  store double %t68, double* %t69, align 8
  br label %merge.1

merge.1:
  ret void
}

define void @GroundTile_draw(%struct.GroundTile* %this, i32 %gy, i32 %col) {
entry:
  %gy.addr = alloca i32, align 8
  store i32 %gy, i32* %gy.addr, align 8
  %col.addr = alloca i32, align 8
  store i32 %col, i32* %col.addr, align 8
  %ix.70 = alloca i32, align 8
  %t71 = getelementptr inbounds %struct.GroundTile, %struct.GroundTile* %this, i32 0, i32 0
  %t72 = load double, double* %t71, align 8
  %t73 = fptosi double %t72 to i32
  store i32 %t73, i32* %ix.70, align 8
  %t74 = load i32, i32* %ix.70, align 4
  %t75 = load i32, i32* %gy.addr, align 4
  %t76 = getelementptr inbounds %struct.GroundTile, %struct.GroundTile* %this, i32 0, i32 2
  %t77 = load i32, i32* %t76, align 4
  %t78 = add nsw i32 %t75, %t77
  %t79 = load i32, i32* %ix.70, align 4
  %t80 = getelementptr inbounds %struct.GroundTile, %struct.GroundTile* %this, i32 0, i32 1
  %t81 = load i32, i32* %t80, align 4
  %t82 = add nsw i32 %t79, %t81
  %t83 = load i32, i32* %gy.addr, align 4
  %t84 = getelementptr inbounds %struct.GroundTile, %struct.GroundTile* %this, i32 0, i32 2
  %t85 = load i32, i32* %t84, align 4
  %t86 = add nsw i32 %t83, %t85
  %t87 = load i32, i32* %col.addr, align 4
  call void @raylib_draw_line(i32 %t74, i32 %t78, i32 %t82, i32 %t86, i32 %t87)
  ret void
}

define void @Obstacle_Obstacle(%struct.Obstacle* %this) {
entry:
  %t88 = fneg double 1.000000e+02
  %t89 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 0
  store double %t88, double* %t89, align 8
  %t90 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 2
  store double 2.700000e+02, double* %t90, align 8
  %t91 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 1
  store double 0.000000e+00, double* %t91, align 8
  %t92 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 3
  store i32 0, i32* %t92, align 4
  %t93 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 4
  store i32 20, i32* %t93, align 4
  %t94 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 5
  store i32 36, i32* %t94, align 4
  %t95 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 6
  store i32 0, i32* %t95, align 4
  ret void
}

define void @Obstacle_spawn(%struct.Obstacle* %this, double %start_x, i32 %type_id) {
entry:
  %start_x.addr = alloca double, align 8
  store double %start_x, double* %start_x.addr, align 8
  %type_id.addr = alloca i32, align 8
  store i32 %type_id, i32* %type_id.addr, align 8
  %t96 = load double, double* %start_x.addr, align 8
  %t97 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 0
  store double %t96, double* %t97, align 8
  %t98 = load i32, i32* %type_id.addr, align 4
  %t99 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 3
  store i32 %t98, i32* %t99, align 4
  %t100 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 6
  store i32 0, i32* %t100, align 4
  %t101 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 3
  %t102 = load i32, i32* %t101, align 4
  %t103 = icmp eq i32 %t102, 0
  br i1 %t103, label %then.2, label %merge.2

then.2:
  %t104 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 4
  store i32 18, i32* %t104, align 4
  %t105 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 5
  store i32 36, i32* %t105, align 4
  %t106 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 2
  %t107 = load double, double* %t106, align 8
  %t108 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 5
  %t109 = load i32, i32* %t108, align 4
  %t110 = sitofp i32 %t109 to double
  %t111 = fsub double %t107, %t110
  %t112 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 1
  store double %t111, double* %t112, align 8
  br label %merge.2

merge.2:
  %t113 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 3
  %t114 = load i32, i32* %t113, align 4
  %t115 = icmp eq i32 %t114, 1
  br i1 %t115, label %then.3, label %merge.3

then.3:
  %t116 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 4
  store i32 24, i32* %t116, align 4
  %t117 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 5
  store i32 50, i32* %t117, align 4
  %t118 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 2
  %t119 = load double, double* %t118, align 8
  %t120 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 5
  %t121 = load i32, i32* %t120, align 4
  %t122 = sitofp i32 %t121 to double
  %t123 = fsub double %t119, %t122
  %t124 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 1
  store double %t123, double* %t124, align 8
  br label %merge.3

merge.3:
  %t125 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 3
  %t126 = load i32, i32* %t125, align 4
  %t127 = icmp eq i32 %t126, 2
  br i1 %t127, label %then.4, label %merge.4

then.4:
  %t128 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 4
  store i32 38, i32* %t128, align 4
  %t129 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 5
  store i32 40, i32* %t129, align 4
  %t130 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 2
  %t131 = load double, double* %t130, align 8
  %t132 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 5
  %t133 = load i32, i32* %t132, align 4
  %t134 = sitofp i32 %t133 to double
  %t135 = fsub double %t131, %t134
  %t136 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 1
  store double %t135, double* %t136, align 8
  br label %merge.4

merge.4:
  %t137 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 3
  %t138 = load i32, i32* %t137, align 4
  %t139 = icmp eq i32 %t138, 3
  br i1 %t139, label %then.5, label %merge.5

then.5:
  %t140 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 4
  store i32 38, i32* %t140, align 4
  %t141 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 5
  store i32 26, i32* %t141, align 4
  %t142 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 2
  %t143 = load double, double* %t142, align 8
  %t144 = fsub double %t143, 5.600000e+01
  %t145 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 1
  store double %t144, double* %t145, align 8
  br label %merge.5

merge.5:
  %t146 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 3
  %t147 = load i32, i32* %t146, align 4
  %t148 = icmp eq i32 %t147, 4
  br i1 %t148, label %then.6, label %merge.6

then.6:
  %t149 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 4
  store i32 38, i32* %t149, align 4
  %t150 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 5
  store i32 26, i32* %t150, align 4
  %t151 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 2
  %t152 = load double, double* %t151, align 8
  %t153 = fsub double %t152, 3.200000e+01
  %t154 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 1
  store double %t153, double* %t154, align 8
  br label %merge.6

merge.6:
  ret void
}

define void @Obstacle_update(%struct.Obstacle* %this, double %speed) {
entry:
  %speed.addr = alloca double, align 8
  store double %speed, double* %speed.addr, align 8
  %t155 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 0
  %t156 = load double, double* %t155, align 8
  %t157 = load double, double* %speed.addr, align 8
  %t158 = fsub double %t156, %t157
  %t159 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 0
  store double %t158, double* %t159, align 8
  %t160 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 6
  %t161 = load i32, i32* %t160, align 4
  %t162 = add nsw i32 %t161, 1
  %t163 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 6
  store i32 %t162, i32* %t163, align 4
  ret void
}

define void @Obstacle_draw(%struct.Obstacle* %this, i32 %col) {
entry:
  %col.addr = alloca i32, align 8
  store i32 %col, i32* %col.addr, align 8
  %ix.164 = alloca i32, align 8
  %iy.165 = alloca i32, align 8
  %flap.166 = alloca i32, align 8
  %t167 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 0
  %t168 = load double, double* %t167, align 8
  %t169 = fptosi double %t168 to i32
  store i32 %t169, i32* %ix.164, align 8
  %t170 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 1
  %t171 = load double, double* %t170, align 8
  %t172 = fptosi double %t171 to i32
  store i32 %t172, i32* %iy.165, align 8
  %t173 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 3
  %t174 = load i32, i32* %t173, align 4
  %t175 = icmp eq i32 %t174, 0
  br i1 %t175, label %then.7, label %merge.7

then.7:
  %t176 = load i32, i32* %ix.164, align 4
  %t177 = add nsw i32 %t176, 5
  %t178 = load i32, i32* %iy.165, align 4
  %t179 = load i32, i32* %col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t177, i32 %t178, i32 8, i32 36, i32 %t179)
  %t180 = load i32, i32* %ix.164, align 4
  %t181 = load i32, i32* %iy.165, align 4
  %t182 = add nsw i32 %t181, 10
  %t183 = load i32, i32* %col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t180, i32 %t182, i32 5, i32 4, i32 %t183)
  %t184 = load i32, i32* %ix.164, align 4
  %t185 = load i32, i32* %iy.165, align 4
  %t186 = add nsw i32 %t185, 6
  %t187 = load i32, i32* %col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t184, i32 %t186, i32 4, i32 8, i32 %t187)
  %t188 = load i32, i32* %ix.164, align 4
  %t189 = add nsw i32 %t188, 13
  %t190 = load i32, i32* %iy.165, align 4
  %t191 = add nsw i32 %t190, 14
  %t192 = load i32, i32* %col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t189, i32 %t191, i32 5, i32 4, i32 %t192)
  %t193 = load i32, i32* %ix.164, align 4
  %t194 = add nsw i32 %t193, 14
  %t195 = load i32, i32* %iy.165, align 4
  %t196 = add nsw i32 %t195, 8
  %t197 = load i32, i32* %col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t194, i32 %t196, i32 4, i32 10, i32 %t197)
  br label %merge.7

merge.7:
  %t198 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 3
  %t199 = load i32, i32* %t198, align 4
  %t200 = icmp eq i32 %t199, 1
  br i1 %t200, label %then.8, label %merge.8

then.8:
  %t201 = load i32, i32* %ix.164, align 4
  %t202 = add nsw i32 %t201, 7
  %t203 = load i32, i32* %iy.165, align 4
  %t204 = load i32, i32* %col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t202, i32 %t203, i32 10, i32 50, i32 %t204)
  %t205 = load i32, i32* %ix.164, align 4
  %t206 = load i32, i32* %iy.165, align 4
  %t207 = add nsw i32 %t206, 14
  %t208 = load i32, i32* %col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t205, i32 %t207, i32 7, i32 5, i32 %t208)
  %t209 = load i32, i32* %ix.164, align 4
  %t210 = load i32, i32* %iy.165, align 4
  %t211 = add nsw i32 %t210, 8
  %t212 = load i32, i32* %col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t209, i32 %t211, i32 5, i32 12, i32 %t212)
  %t213 = load i32, i32* %ix.164, align 4
  %t214 = add nsw i32 %t213, 17
  %t215 = load i32, i32* %iy.165, align 4
  %t216 = add nsw i32 %t215, 18
  %t217 = load i32, i32* %col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t214, i32 %t216, i32 7, i32 5, i32 %t217)
  %t218 = load i32, i32* %ix.164, align 4
  %t219 = add nsw i32 %t218, 19
  %t220 = load i32, i32* %iy.165, align 4
  %t221 = add nsw i32 %t220, 10
  %t222 = load i32, i32* %col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t219, i32 %t221, i32 5, i32 14, i32 %t222)
  br label %merge.8

merge.8:
  %t223 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 3
  %t224 = load i32, i32* %t223, align 4
  %t225 = icmp eq i32 %t224, 2
  br i1 %t225, label %then.9, label %merge.9

then.9:
  %t226 = load i32, i32* %ix.164, align 4
  %t227 = add nsw i32 %t226, 4
  %t228 = load i32, i32* %iy.165, align 4
  %t229 = add nsw i32 %t228, 6
  %t230 = load i32, i32* %col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t227, i32 %t229, i32 8, i32 34, i32 %t230)
  %t231 = load i32, i32* %ix.164, align 4
  %t232 = load i32, i32* %iy.165, align 4
  %t233 = add nsw i32 %t232, 14
  %t234 = load i32, i32* %col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t231, i32 %t233, i32 4, i32 10, i32 %t234)
  %t235 = load i32, i32* %ix.164, align 4
  %t236 = add nsw i32 %t235, 20
  %t237 = load i32, i32* %iy.165, align 4
  %t238 = load i32, i32* %col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t236, i32 %t237, i32 10, i32 40, i32 %t238)
  %t239 = load i32, i32* %ix.164, align 4
  %t240 = add nsw i32 %t239, 30
  %t241 = load i32, i32* %iy.165, align 4
  %t242 = add nsw i32 %t241, 12
  %t243 = load i32, i32* %col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t240, i32 %t242, i32 6, i32 12, i32 %t243)
  br label %merge.9

merge.9:
  %t244 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 3
  %t245 = load i32, i32* %t244, align 4
  %t246 = icmp eq i32 %t245, 3
  %t247 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 3
  %t248 = load i32, i32* %t247, align 4
  %t249 = icmp eq i32 %t248, 4
  %t250 = or i1 %t246, %t249
  br i1 %t250, label %then.10, label %merge.10

then.10:
  %t251 = load i32, i32* %ix.164, align 4
  %t252 = add nsw i32 %t251, 10
  %t253 = load i32, i32* %iy.165, align 4
  %t254 = add nsw i32 %t253, 8
  %t255 = load i32, i32* %col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t252, i32 %t254, i32 20, i32 8, i32 %t255)
  %t256 = load i32, i32* %ix.164, align 4
  %t257 = add nsw i32 %t256, 4
  %t258 = load i32, i32* %iy.165, align 4
  %t259 = add nsw i32 %t258, 6
  %t260 = load i32, i32* %col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t257, i32 %t259, i32 8, i32 7, i32 %t260)
  %t261 = load i32, i32* %ix.164, align 4
  %t262 = load i32, i32* %iy.165, align 4
  %t263 = add nsw i32 %t262, 8
  %t264 = load i32, i32* %col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t261, i32 %t263, i32 5, i32 3, i32 %t264)
  %t265 = load i32, i32* %ix.164, align 4
  %t266 = add nsw i32 %t265, 30
  %t267 = load i32, i32* %iy.165, align 4
  %t268 = add nsw i32 %t267, 10
  %t269 = load i32, i32* %col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t266, i32 %t268, i32 8, i32 4, i32 %t269)
  %flap.270 = alloca i32, align 8
  %t271 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %this, i32 0, i32 6
  %t272 = load i32, i32* %t271, align 4
  %t273 = sdiv i32 %t272, 10
  %t274 = srem i32 %t273, 2
  store i32 %t274, i32* %flap.270, align 8
  %t275 = load i32, i32* %flap.270, align 4
  %t276 = icmp eq i32 %t275, 0
  br i1 %t276, label %then.11, label %else.11

then.11:
  %t277 = load i32, i32* %ix.164, align 4
  %t278 = add nsw i32 %t277, 14
  %t279 = load i32, i32* %iy.165, align 4
  %t280 = sub nsw i32 %t279, 6
  %t281 = load i32, i32* %col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t278, i32 %t280, i32 7, i32 14, i32 %t281)
  %t282 = load i32, i32* %ix.164, align 4
  %t283 = add nsw i32 %t282, 16
  %t284 = load i32, i32* %iy.165, align 4
  %t285 = sub nsw i32 %t284, 10
  %t286 = load i32, i32* %col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t283, i32 %t285, i32 4, i32 5, i32 %t286)
  br label %merge.11

else.11:
  %t287 = load i32, i32* %ix.164, align 4
  %t288 = add nsw i32 %t287, 14
  %t289 = load i32, i32* %iy.165, align 4
  %t290 = add nsw i32 %t289, 12
  %t291 = load i32, i32* %col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t288, i32 %t290, i32 7, i32 14, i32 %t291)
  %t292 = load i32, i32* %ix.164, align 4
  %t293 = add nsw i32 %t292, 16
  %t294 = load i32, i32* %iy.165, align 4
  %t295 = add nsw i32 %t294, 24
  %t296 = load i32, i32* %col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t293, i32 %t295, i32 4, i32 5, i32 %t296)
  br label %merge.11

merge.11:
  br label %merge.10

merge.10:
  ret void
}

define void @Dino_Dino(%struct.Dino* %this) {
entry:
  %t297 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 0
  store double 6.000000e+01, double* %t297, align 8
  %t298 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 2
  store double 2.700000e+02, double* %t298, align 8
  %t299 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 2
  %t300 = load double, double* %t299, align 8
  %t301 = fsub double %t300, 4.800000e+01
  %t302 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 1
  store double %t301, double* %t302, align 8
  %t303 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 3
  store double 0.000000e+00, double* %t303, align 8
  %t304 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 4
  store double 8.200000e-01, double* %t304, align 8
  %t305 = fneg double 1.420000e+01
  %t306 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 5
  store double %t305, double* %t306, align 8
  %t307 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 6
  store i1 false, i1* %t307, align 4
  %t308 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 7
  store i1 false, i1* %t308, align 4
  %t309 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 8
  store i32 0, i32* %t309, align 4
  %t310 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 9
  store i32 44, i32* %t310, align 4
  %t311 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 10
  store i32 48, i32* %t311, align 4
  ret void
}

define void @Dino_reset(%struct.Dino* %this) {
entry:
  %t312 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 2
  %t313 = load double, double* %t312, align 8
  %t314 = fsub double %t313, 4.800000e+01
  %t315 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 1
  store double %t314, double* %t315, align 8
  %t316 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 3
  store double 0.000000e+00, double* %t316, align 8
  %t317 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 6
  store i1 false, i1* %t317, align 4
  %t318 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 7
  store i1 false, i1* %t318, align 4
  %t319 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 8
  store i32 0, i32* %t319, align 4
  %t320 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 9
  store i32 44, i32* %t320, align 4
  %t321 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 10
  store i32 48, i32* %t321, align 4
  ret void
}

define void @Dino_jump(%struct.Dino* %this) {
entry:
  %t322 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 6
  %t323 = load i1, i1* %t322, align 4
  %t324 = xor i1 %t323, true
  br i1 %t324, label %then.12, label %merge.12

then.12:
  %t325 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 5
  %t326 = load double, double* %t325, align 8
  %t327 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 3
  store double %t326, double* %t327, align 8
  %t328 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 6
  store i1 true, i1* %t328, align 4
  br label %merge.12

merge.12:
  ret void
}

define void @Dino_update(%struct.Dino* %this, i1 %duck_key) {
entry:
  %duck_key.addr = alloca i1, align 8
  store i1 %duck_key, i1* %duck_key.addr, align 8
  %t329 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 8
  %t330 = load i32, i32* %t329, align 4
  %t331 = add nsw i32 %t330, 1
  %t332 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 8
  store i32 %t331, i32* %t332, align 4
  %t333 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 6
  %t334 = load i1, i1* %t333, align 4
  br i1 %t334, label %then.13, label %else.13

then.13:
  %t335 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 3
  %t336 = load double, double* %t335, align 8
  %t337 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 4
  %t338 = load double, double* %t337, align 8
  %t339 = fadd double %t336, %t338
  %t340 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 3
  store double %t339, double* %t340, align 8
  %t341 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 1
  %t342 = load double, double* %t341, align 8
  %t343 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 3
  %t344 = load double, double* %t343, align 8
  %t345 = fadd double %t342, %t344
  %t346 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 1
  store double %t345, double* %t346, align 8
  %t347 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 1
  %t348 = load double, double* %t347, align 8
  %t349 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 2
  %t350 = load double, double* %t349, align 8
  %t351 = fsub double %t350, 4.800000e+01
  %t352 = fcmp oge double %t348, %t351
  br i1 %t352, label %then.14, label %merge.14

then.14:
  %t353 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 2
  %t354 = load double, double* %t353, align 8
  %t355 = fsub double %t354, 4.800000e+01
  %t356 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 1
  store double %t355, double* %t356, align 8
  %t357 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 3
  store double 0.000000e+00, double* %t357, align 8
  %t358 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 6
  store i1 false, i1* %t358, align 4
  br label %merge.14

merge.14:
  br label %merge.13

else.13:
  %t359 = load i1, i1* %duck_key.addr, align 4
  br i1 %t359, label %then.15, label %else.15

then.15:
  %t360 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 7
  store i1 true, i1* %t360, align 4
  %t361 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 9
  store i32 56, i32* %t361, align 4
  %t362 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 10
  store i32 30, i32* %t362, align 4
  %t363 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 2
  %t364 = load double, double* %t363, align 8
  %t365 = fsub double %t364, 3.000000e+01
  %t366 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 1
  store double %t365, double* %t366, align 8
  br label %merge.15

else.15:
  %t367 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 7
  store i1 false, i1* %t367, align 4
  %t368 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 9
  store i32 44, i32* %t368, align 4
  %t369 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 10
  store i32 48, i32* %t369, align 4
  %t370 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 2
  %t371 = load double, double* %t370, align 8
  %t372 = fsub double %t371, 4.800000e+01
  %t373 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 1
  store double %t372, double* %t373, align 8
  br label %merge.15

merge.15:
  br label %merge.13

merge.13:
  ret void
}

define void @Dino_draw(%struct.Dino* %this, i32 %dino_col, i32 %eye_col, i1 %is_dead) {
entry:
  %dino_col.addr = alloca i32, align 8
  store i32 %dino_col, i32* %dino_col.addr, align 8
  %eye_col.addr = alloca i32, align 8
  store i32 %eye_col, i32* %eye_col.addr, align 8
  %is_dead.addr = alloca i1, align 8
  store i1 %is_dead, i1* %is_dead.addr, align 8
  %ix.374 = alloca i32, align 8
  %iy.375 = alloca i32, align 8
  %step.376 = alloca i32, align 8
  %t377 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 0
  %t378 = load double, double* %t377, align 8
  %t379 = fptosi double %t378 to i32
  store i32 %t379, i32* %ix.374, align 8
  %t380 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 1
  %t381 = load double, double* %t380, align 8
  %t382 = fptosi double %t381 to i32
  store i32 %t382, i32* %iy.375, align 8
  %t383 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 7
  %t384 = load i1, i1* %t383, align 4
  %t385 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 6
  %t386 = load i1, i1* %t385, align 4
  %t387 = xor i1 %t386, true
  %t388 = and i1 %t384, %t387
  br i1 %t388, label %then.16, label %else.16

then.16:
  %t389 = load i32, i32* %ix.374, align 4
  %t390 = add nsw i32 %t389, 10
  %t391 = load i32, i32* %iy.375, align 4
  %t392 = add nsw i32 %t391, 6
  %t393 = load i32, i32* %dino_col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t390, i32 %t392, i32 32, i32 16, i32 %t393)
  %t394 = load i32, i32* %ix.374, align 4
  %t395 = add nsw i32 %t394, 40
  %t396 = load i32, i32* %iy.375, align 4
  %t397 = add nsw i32 %t396, 4
  %t398 = load i32, i32* %dino_col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t395, i32 %t397, i32 16, i32 12, i32 %t398)
  %t399 = load i32, i32* %ix.374, align 4
  %t400 = add nsw i32 %t399, 48
  %t401 = load i32, i32* %iy.375, align 4
  %t402 = add nsw i32 %t401, 10
  %t403 = load i32, i32* %dino_col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t400, i32 %t402, i32 8, i32 6, i32 %t403)
  %t404 = load i32, i32* %ix.374, align 4
  %t405 = add nsw i32 %t404, 44
  %t406 = load i32, i32* %iy.375, align 4
  %t407 = add nsw i32 %t406, 6
  %t408 = load i32, i32* %eye_col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t405, i32 %t407, i32 3, i32 3, i32 %t408)
  %t409 = load i32, i32* %ix.374, align 4
  %t410 = load i32, i32* %iy.375, align 4
  %t411 = add nsw i32 %t410, 8
  %t412 = load i32, i32* %dino_col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t409, i32 %t411, i32 10, i32 8, i32 %t412)
  %t413 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 8
  %t414 = load i32, i32* %t413, align 4
  %t415 = sdiv i32 %t414, 6
  %t416 = srem i32 %t415, 2
  store i32 %t416, i32* %step.376, align 8
  %t417 = load i32, i32* %step.376, align 4
  %t418 = icmp eq i32 %t417, 0
  br i1 %t418, label %then.17, label %else.17

then.17:
  %t419 = load i32, i32* %ix.374, align 4
  %t420 = add nsw i32 %t419, 16
  %t421 = load i32, i32* %iy.375, align 4
  %t422 = add nsw i32 %t421, 22
  %t423 = load i32, i32* %dino_col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t420, i32 %t422, i32 6, i32 8, i32 %t423)
  %t424 = load i32, i32* %ix.374, align 4
  %t425 = add nsw i32 %t424, 28
  %t426 = load i32, i32* %iy.375, align 4
  %t427 = add nsw i32 %t426, 22
  %t428 = load i32, i32* %dino_col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t425, i32 %t427, i32 6, i32 5, i32 %t428)
  br label %merge.17

else.17:
  %t429 = load i32, i32* %ix.374, align 4
  %t430 = add nsw i32 %t429, 16
  %t431 = load i32, i32* %iy.375, align 4
  %t432 = add nsw i32 %t431, 22
  %t433 = load i32, i32* %dino_col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t430, i32 %t432, i32 6, i32 5, i32 %t433)
  %t434 = load i32, i32* %ix.374, align 4
  %t435 = add nsw i32 %t434, 28
  %t436 = load i32, i32* %iy.375, align 4
  %t437 = add nsw i32 %t436, 22
  %t438 = load i32, i32* %dino_col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t435, i32 %t437, i32 6, i32 8, i32 %t438)
  br label %merge.17

merge.17:
  br label %merge.16

else.16:
  %t439 = load i32, i32* %ix.374, align 4
  %t440 = add nsw i32 %t439, 20
  %t441 = load i32, i32* %iy.375, align 4
  %t442 = load i32, i32* %dino_col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t440, i32 %t441, i32 22, i32 16, i32 %t442)
  %t443 = load i32, i32* %ix.374, align 4
  %t444 = add nsw i32 %t443, 34
  %t445 = load i32, i32* %iy.375, align 4
  %t446 = add nsw i32 %t445, 8
  %t447 = load i32, i32* %dino_col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t444, i32 %t446, i32 10, i32 8, i32 %t447)
  %t448 = load i32, i32* %ix.374, align 4
  %t449 = add nsw i32 %t448, 32
  %t450 = load i32, i32* %iy.375, align 4
  %t451 = add nsw i32 %t450, 12
  %t452 = load i32, i32* %eye_col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t449, i32 %t451, i32 8, i32 2, i32 %t452)
  %t453 = load i1, i1* %is_dead.addr, align 4
  br i1 %t453, label %then.18, label %else.18

then.18:
  %t454 = load i32, i32* %ix.374, align 4
  %t455 = add nsw i32 %t454, 26
  %t456 = load i32, i32* %iy.375, align 4
  %t457 = add nsw i32 %t456, 4
  %t458 = load i32, i32* %eye_col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t455, i32 %t457, i32 4, i32 4, i32 %t458)
  %t459 = load i32, i32* %ix.374, align 4
  %t460 = add nsw i32 %t459, 25
  %t461 = load i32, i32* %iy.375, align 4
  %t462 = add nsw i32 %t461, 3
  %t463 = load i32, i32* %ix.374, align 4
  %t464 = add nsw i32 %t463, 30
  %t465 = load i32, i32* %iy.375, align 4
  %t466 = add nsw i32 %t465, 8
  %t467 = load i32, i32* %dino_col.addr, align 4
  call void @raylib_draw_line(i32 %t460, i32 %t462, i32 %t464, i32 %t466, i32 %t467)
  %t468 = load i32, i32* %ix.374, align 4
  %t469 = add nsw i32 %t468, 25
  %t470 = load i32, i32* %iy.375, align 4
  %t471 = add nsw i32 %t470, 8
  %t472 = load i32, i32* %ix.374, align 4
  %t473 = add nsw i32 %t472, 30
  %t474 = load i32, i32* %iy.375, align 4
  %t475 = add nsw i32 %t474, 3
  %t476 = load i32, i32* %dino_col.addr, align 4
  call void @raylib_draw_line(i32 %t469, i32 %t471, i32 %t473, i32 %t475, i32 %t476)
  br label %merge.18

else.18:
  %t477 = load i32, i32* %ix.374, align 4
  %t478 = add nsw i32 %t477, 26
  %t479 = load i32, i32* %iy.375, align 4
  %t480 = add nsw i32 %t479, 4
  %t481 = load i32, i32* %eye_col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t478, i32 %t480, i32 3, i32 3, i32 %t481)
  br label %merge.18

merge.18:
  %t482 = load i32, i32* %ix.374, align 4
  %t483 = add nsw i32 %t482, 10
  %t484 = load i32, i32* %iy.375, align 4
  %t485 = add nsw i32 %t484, 16
  %t486 = load i32, i32* %dino_col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t483, i32 %t485, i32 24, i32 20, i32 %t486)
  %t487 = load i32, i32* %ix.374, align 4
  %t488 = add nsw i32 %t487, 32
  %t489 = load i32, i32* %iy.375, align 4
  %t490 = add nsw i32 %t489, 22
  %t491 = load i32, i32* %dino_col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t488, i32 %t490, i32 6, i32 4, i32 %t491)
  %t492 = load i32, i32* %ix.374, align 4
  %t493 = add nsw i32 %t492, 36
  %t494 = load i32, i32* %iy.375, align 4
  %t495 = add nsw i32 %t494, 24
  %t496 = load i32, i32* %dino_col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t493, i32 %t495, i32 2, i32 4, i32 %t496)
  %t497 = load i32, i32* %ix.374, align 4
  %t498 = load i32, i32* %iy.375, align 4
  %t499 = add nsw i32 %t498, 18
  %t500 = load i32, i32* %dino_col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t497, i32 %t499, i32 10, i32 10, i32 %t500)
  %t501 = load i32, i32* %ix.374, align 4
  %t502 = add nsw i32 %t501, 2
  %t503 = load i32, i32* %iy.375, align 4
  %t504 = add nsw i32 %t503, 26
  %t505 = load i32, i32* %dino_col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t502, i32 %t504, i32 8, i32 6, i32 %t505)
  %t506 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 6
  %t507 = load i1, i1* %t506, align 4
  br i1 %t507, label %then.19, label %else.19

then.19:
  %t508 = load i32, i32* %ix.374, align 4
  %t509 = add nsw i32 %t508, 14
  %t510 = load i32, i32* %iy.375, align 4
  %t511 = add nsw i32 %t510, 36
  %t512 = load i32, i32* %dino_col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t509, i32 %t511, i32 6, i32 6, i32 %t512)
  %t513 = load i32, i32* %ix.374, align 4
  %t514 = add nsw i32 %t513, 24
  %t515 = load i32, i32* %iy.375, align 4
  %t516 = add nsw i32 %t515, 36
  %t517 = load i32, i32* %dino_col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t514, i32 %t516, i32 6, i32 6, i32 %t517)
  br label %merge.19

else.19:
  %step.518 = alloca i32, align 8
  %t519 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 8
  %t520 = load i32, i32* %t519, align 4
  %t521 = sdiv i32 %t520, 6
  %t522 = srem i32 %t521, 2
  store i32 %t522, i32* %step.518, align 8
  %t523 = load i32, i32* %step.518, align 4
  %t524 = icmp eq i32 %t523, 0
  br i1 %t524, label %then.20, label %else.20

then.20:
  %t525 = load i32, i32* %ix.374, align 4
  %t526 = add nsw i32 %t525, 14
  %t527 = load i32, i32* %iy.375, align 4
  %t528 = add nsw i32 %t527, 36
  %t529 = load i32, i32* %dino_col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t526, i32 %t528, i32 4, i32 12, i32 %t529)
  %t530 = load i32, i32* %ix.374, align 4
  %t531 = add nsw i32 %t530, 18
  %t532 = load i32, i32* %iy.375, align 4
  %t533 = add nsw i32 %t532, 45
  %t534 = load i32, i32* %dino_col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t531, i32 %t533, i32 4, i32 3, i32 %t534)
  %t535 = load i32, i32* %ix.374, align 4
  %t536 = add nsw i32 %t535, 24
  %t537 = load i32, i32* %iy.375, align 4
  %t538 = add nsw i32 %t537, 36
  %t539 = load i32, i32* %dino_col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t536, i32 %t538, i32 4, i32 7, i32 %t539)
  br label %merge.20

else.20:
  %t540 = load i32, i32* %ix.374, align 4
  %t541 = add nsw i32 %t540, 14
  %t542 = load i32, i32* %iy.375, align 4
  %t543 = add nsw i32 %t542, 36
  %t544 = load i32, i32* %dino_col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t541, i32 %t543, i32 4, i32 7, i32 %t544)
  %t545 = load i32, i32* %ix.374, align 4
  %t546 = add nsw i32 %t545, 24
  %t547 = load i32, i32* %iy.375, align 4
  %t548 = add nsw i32 %t547, 36
  %t549 = load i32, i32* %dino_col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t546, i32 %t548, i32 4, i32 12, i32 %t549)
  %t550 = load i32, i32* %ix.374, align 4
  %t551 = add nsw i32 %t550, 28
  %t552 = load i32, i32* %iy.375, align 4
  %t553 = add nsw i32 %t552, 45
  %t554 = load i32, i32* %dino_col.addr, align 4
  call void @raylib_draw_rectangle(i32 %t551, i32 %t553, i32 4, i32 3, i32 %t554)
  br label %merge.20

merge.20:
  br label %merge.19

merge.19:
  br label %merge.16

merge.16:
  ret void
}

define i1 @Dino_check_collision(%struct.Dino* %this, %struct.Obstacle* %obs) {
entry:
  %obs.addr = alloca %struct.Obstacle*, align 8
  store %struct.Obstacle* %obs, %struct.Obstacle** %obs.addr, align 8
  %dino_left.555 = alloca double, align 8
  %dino_right.556 = alloca double, align 8
  %dino_top.557 = alloca double, align 8
  %dino_bottom.558 = alloca double, align 8
  %obs_left.559 = alloca double, align 8
  %obs_right.560 = alloca double, align 8
  %obs_top.561 = alloca double, align 8
  %obs_bottom.562 = alloca double, align 8
  %t563 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 0
  %t564 = load double, double* %t563, align 8
  %t565 = fadd double %t564, 4.000000e+00
  store double %t565, double* %dino_left.555, align 8
  %t566 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 0
  %t567 = load double, double* %t566, align 8
  %t568 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 9
  %t569 = load i32, i32* %t568, align 4
  %t570 = sitofp i32 %t569 to double
  %t571 = fadd double %t567, %t570
  %t572 = fsub double %t571, 4.000000e+00
  store double %t572, double* %dino_right.556, align 8
  %t573 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 1
  %t574 = load double, double* %t573, align 8
  %t575 = fadd double %t574, 4.000000e+00
  store double %t575, double* %dino_top.557, align 8
  %t576 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 1
  %t577 = load double, double* %t576, align 8
  %t578 = getelementptr inbounds %struct.Dino, %struct.Dino* %this, i32 0, i32 10
  %t579 = load i32, i32* %t578, align 4
  %t580 = sitofp i32 %t579 to double
  %t581 = fadd double %t577, %t580
  store double %t581, double* %dino_bottom.558, align 8
  %t582 = load %struct.Obstacle*, %struct.Obstacle** %obs.addr, align 8
  %t583 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %t582, i32 0, i32 0
  %t584 = load double, double* %t583, align 8
  %t585 = fadd double %t584, 3.000000e+00
  store double %t585, double* %obs_left.559, align 8
  %t586 = load %struct.Obstacle*, %struct.Obstacle** %obs.addr, align 8
  %t587 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %t586, i32 0, i32 0
  %t588 = load double, double* %t587, align 8
  %t589 = load %struct.Obstacle*, %struct.Obstacle** %obs.addr, align 8
  %t590 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %t589, i32 0, i32 4
  %t591 = load i32, i32* %t590, align 4
  %t592 = sitofp i32 %t591 to double
  %t593 = fadd double %t588, %t592
  %t594 = fsub double %t593, 3.000000e+00
  store double %t594, double* %obs_right.560, align 8
  %t595 = load %struct.Obstacle*, %struct.Obstacle** %obs.addr, align 8
  %t596 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %t595, i32 0, i32 1
  %t597 = load double, double* %t596, align 8
  %t598 = fadd double %t597, 3.000000e+00
  store double %t598, double* %obs_top.561, align 8
  %t599 = load %struct.Obstacle*, %struct.Obstacle** %obs.addr, align 8
  %t600 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %t599, i32 0, i32 1
  %t601 = load double, double* %t600, align 8
  %t602 = load %struct.Obstacle*, %struct.Obstacle** %obs.addr, align 8
  %t603 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %t602, i32 0, i32 5
  %t604 = load i32, i32* %t603, align 4
  %t605 = sitofp i32 %t604 to double
  %t606 = fadd double %t601, %t605
  store double %t606, double* %obs_bottom.562, align 8
  %t607 = load double, double* %dino_right.556, align 8
  %t608 = load double, double* %obs_left.559, align 8
  %t609 = fcmp ogt double %t607, %t608
  %t610 = load double, double* %dino_left.555, align 8
  %t611 = load double, double* %obs_right.560, align 8
  %t612 = fcmp olt double %t610, %t611
  %t613 = and i1 %t609, %t612
  br i1 %t613, label %then.21, label %merge.21

then.21:
  %t614 = load double, double* %dino_bottom.558, align 8
  %t615 = load double, double* %obs_top.561, align 8
  %t616 = fcmp ogt double %t614, %t615
  %t617 = load double, double* %dino_top.557, align 8
  %t618 = load double, double* %obs_bottom.562, align 8
  %t619 = fcmp olt double %t617, %t618
  %t620 = and i1 %t616, %t619
  br i1 %t620, label %then.22, label %merge.22

then.22:
  ret i1 true
merge.22:
  br label %merge.21

merge.21:
  ret i1 false
}

define i32 @main(i32 %argc, i8** %argv) {
entry:
  call void @xllvm_rt_init(i32 %argc, i8** %argv)
  %SCREEN_W.621 = alloca i32, align 8
  store i32 800, i32* %SCREEN_W.621, align 8
  %SCREEN_H.622 = alloca i32, align 8
  store i32 340, i32* %SCREEN_H.622, align 8
  %ground_y_pos.623 = alloca i32, align 8
  store i32 270, i32* %ground_y_pos.623, align 8
  %STATE_START.624 = alloca i32, align 8
  store i32 0, i32* %STATE_START.624, align 8
  %STATE_PLAYING.625 = alloca i32, align 8
  store i32 1, i32* %STATE_PLAYING.625, align 8
  %STATE_GAMEOVER.626 = alloca i32, align 8
  store i32 2, i32* %STATE_GAMEOVER.626, align 8
  %t627 = load i32, i32* %SCREEN_W.621, align 4
  %t628 = load i32, i32* %SCREEN_H.622, align 4
  %t629 = getelementptr inbounds [37 x i8], [37 x i8]* @.str.0, i64 0, i64 0
  call void @raylib_init_window(i32 %t627, i32 %t628, i8* %t629)
  call void @raylib_set_target_fps(i32 60)
  %dino.630 = alloca %struct.Dino*, align 8
  %t631 = getelementptr %struct.Dino, %struct.Dino* null, i32 1
  %t632 = ptrtoint %struct.Dino* %t631 to i64
  %t633 = call i8* @calloc(i64 1, i64 %t632)
  %t634 = bitcast i8* %t633 to %struct.Dino*
  call void @Dino_Dino(%struct.Dino* %t634)
  store %struct.Dino* %t634, %struct.Dino** %dino.630, align 8
  %obs1.635 = alloca %struct.Obstacle*, align 8
  %t636 = getelementptr %struct.Obstacle, %struct.Obstacle* null, i32 1
  %t637 = ptrtoint %struct.Obstacle* %t636 to i64
  %t638 = call i8* @calloc(i64 1, i64 %t637)
  %t639 = bitcast i8* %t638 to %struct.Obstacle*
  call void @Obstacle_Obstacle(%struct.Obstacle* %t639)
  store %struct.Obstacle* %t639, %struct.Obstacle** %obs1.635, align 8
  %obs2.640 = alloca %struct.Obstacle*, align 8
  %t641 = getelementptr %struct.Obstacle, %struct.Obstacle* null, i32 1
  %t642 = ptrtoint %struct.Obstacle* %t641 to i64
  %t643 = call i8* @calloc(i64 1, i64 %t642)
  %t644 = bitcast i8* %t643 to %struct.Obstacle*
  call void @Obstacle_Obstacle(%struct.Obstacle* %t644)
  store %struct.Obstacle* %t644, %struct.Obstacle** %obs2.640, align 8
  %obs3.645 = alloca %struct.Obstacle*, align 8
  %t646 = getelementptr %struct.Obstacle, %struct.Obstacle* null, i32 1
  %t647 = ptrtoint %struct.Obstacle* %t646 to i64
  %t648 = call i8* @calloc(i64 1, i64 %t647)
  %t649 = bitcast i8* %t648 to %struct.Obstacle*
  call void @Obstacle_Obstacle(%struct.Obstacle* %t649)
  store %struct.Obstacle* %t649, %struct.Obstacle** %obs3.645, align 8
  %t650 = load %struct.Obstacle*, %struct.Obstacle** %obs1.635, align 8
  call void @Obstacle_spawn(%struct.Obstacle* %t650, double 7.500000e+02, i32 0)
  %t651 = load %struct.Obstacle*, %struct.Obstacle** %obs2.640, align 8
  call void @Obstacle_spawn(%struct.Obstacle* %t651, double 1.100000e+03, i32 1)
  %t652 = load %struct.Obstacle*, %struct.Obstacle** %obs3.645, align 8
  call void @Obstacle_spawn(%struct.Obstacle* %t652, double 1.450000e+03, i32 3)
  %c1.653 = alloca %struct.Cloud*, align 8
  %t654 = getelementptr %struct.Cloud, %struct.Cloud* null, i32 1
  %t655 = ptrtoint %struct.Cloud* %t654 to i64
  %t656 = call i8* @calloc(i64 1, i64 %t655)
  %t657 = bitcast i8* %t656 to %struct.Cloud*
  call void @Cloud_Cloud(%struct.Cloud* %t657)
  store %struct.Cloud* %t657, %struct.Cloud** %c1.653, align 8
  %c2.658 = alloca %struct.Cloud*, align 8
  %t659 = getelementptr %struct.Cloud, %struct.Cloud* null, i32 1
  %t660 = ptrtoint %struct.Cloud* %t659 to i64
  %t661 = call i8* @calloc(i64 1, i64 %t660)
  %t662 = bitcast i8* %t661 to %struct.Cloud*
  call void @Cloud_Cloud(%struct.Cloud* %t662)
  store %struct.Cloud* %t662, %struct.Cloud** %c2.658, align 8
  %c3.663 = alloca %struct.Cloud*, align 8
  %t664 = getelementptr %struct.Cloud, %struct.Cloud* null, i32 1
  %t665 = ptrtoint %struct.Cloud* %t664 to i64
  %t666 = call i8* @calloc(i64 1, i64 %t665)
  %t667 = bitcast i8* %t666 to %struct.Cloud*
  call void @Cloud_Cloud(%struct.Cloud* %t667)
  store %struct.Cloud* %t667, %struct.Cloud** %c3.663, align 8
  %t668 = load %struct.Cloud*, %struct.Cloud** %c1.653, align 8
  call void @Cloud_reset(%struct.Cloud* %t668, double 1.200000e+02, double 5.000000e+01, double 8.000000e-01)
  %t669 = load %struct.Cloud*, %struct.Cloud** %c2.658, align 8
  call void @Cloud_reset(%struct.Cloud* %t669, double 4.200000e+02, double 7.500000e+01, double 6.000000e-01)
  %t670 = load %struct.Cloud*, %struct.Cloud** %c3.663, align 8
  call void @Cloud_reset(%struct.Cloud* %t670, double 7.200000e+02, double 4.000000e+01, double 1.000000e+00)
  %g1.671 = alloca %struct.GroundTile*, align 8
  %t672 = getelementptr %struct.GroundTile, %struct.GroundTile* null, i32 1
  %t673 = ptrtoint %struct.GroundTile* %t672 to i64
  %t674 = call i8* @calloc(i64 1, i64 %t673)
  %t675 = bitcast i8* %t674 to %struct.GroundTile*
  call void @GroundTile_GroundTile(%struct.GroundTile* %t675)
  store %struct.GroundTile* %t675, %struct.GroundTile** %g1.671, align 8
  %g2.676 = alloca %struct.GroundTile*, align 8
  %t677 = getelementptr %struct.GroundTile, %struct.GroundTile* null, i32 1
  %t678 = ptrtoint %struct.GroundTile* %t677 to i64
  %t679 = call i8* @calloc(i64 1, i64 %t678)
  %t680 = bitcast i8* %t679 to %struct.GroundTile*
  call void @GroundTile_GroundTile(%struct.GroundTile* %t680)
  store %struct.GroundTile* %t680, %struct.GroundTile** %g2.676, align 8
  %g3.681 = alloca %struct.GroundTile*, align 8
  %t682 = getelementptr %struct.GroundTile, %struct.GroundTile* null, i32 1
  %t683 = ptrtoint %struct.GroundTile* %t682 to i64
  %t684 = call i8* @calloc(i64 1, i64 %t683)
  %t685 = bitcast i8* %t684 to %struct.GroundTile*
  call void @GroundTile_GroundTile(%struct.GroundTile* %t685)
  store %struct.GroundTile* %t685, %struct.GroundTile** %g3.681, align 8
  %g4.686 = alloca %struct.GroundTile*, align 8
  %t687 = getelementptr %struct.GroundTile, %struct.GroundTile* null, i32 1
  %t688 = ptrtoint %struct.GroundTile* %t687 to i64
  %t689 = call i8* @calloc(i64 1, i64 %t688)
  %t690 = bitcast i8* %t689 to %struct.GroundTile*
  call void @GroundTile_GroundTile(%struct.GroundTile* %t690)
  store %struct.GroundTile* %t690, %struct.GroundTile** %g4.686, align 8
  %g5.691 = alloca %struct.GroundTile*, align 8
  %t692 = getelementptr %struct.GroundTile, %struct.GroundTile* null, i32 1
  %t693 = ptrtoint %struct.GroundTile* %t692 to i64
  %t694 = call i8* @calloc(i64 1, i64 %t693)
  %t695 = bitcast i8* %t694 to %struct.GroundTile*
  call void @GroundTile_GroundTile(%struct.GroundTile* %t695)
  store %struct.GroundTile* %t695, %struct.GroundTile** %g5.691, align 8
  %t696 = load %struct.GroundTile*, %struct.GroundTile** %g1.671, align 8
  call void @GroundTile_init(%struct.GroundTile* %t696, double 5.000000e+01, i32 16, i32 4)
  %t697 = load %struct.GroundTile*, %struct.GroundTile** %g2.676, align 8
  call void @GroundTile_init(%struct.GroundTile* %t697, double 2.100000e+02, i32 24, i32 6)
  %t698 = load %struct.GroundTile*, %struct.GroundTile** %g3.681, align 8
  call void @GroundTile_init(%struct.GroundTile* %t698, double 3.900000e+02, i32 12, i32 3)
  %t699 = load %struct.GroundTile*, %struct.GroundTile** %g4.686, align 8
  call void @GroundTile_init(%struct.GroundTile* %t699, double 5.800000e+02, i32 30, i32 7)
  %t700 = load %struct.GroundTile*, %struct.GroundTile** %g5.691, align 8
  call void @GroundTile_init(%struct.GroundTile* %t700, double 7.400000e+02, i32 18, i32 5)
  %game_state.701 = alloca i32, align 8
  %t702 = load i32, i32* %STATE_START.624, align 4
  store i32 %t702, i32* %game_state.701, align 8
  %game_speed.703 = alloca double, align 8
  store double 6.500000e+00, double* %game_speed.703, align 8
  %score.704 = alloca i32, align 8
  store i32 0, i32* %score.704, align 8
  %high_score.705 = alloca i32, align 8
  store i32 0, i32* %high_score.705, align 8
  %score_timer.706 = alloca i32, align 8
  store i32 0, i32* %score_timer.706, align 8
  %night_mode_timer.707 = alloca i32, align 8
  store i32 0, i32* %night_mode_timer.707, align 8
  %flash_timer.708 = alloca i32, align 8
  store i32 0, i32* %flash_timer.708, align 8
  %t709 = getelementptr inbounds [51 x i8], [51 x i8]* @.str.1, i64 0, i64 0
  call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str.2, i64 0, i64 0), i8* %t709)
  %t710 = getelementptr inbounds [51 x i8], [51 x i8]* @.str.3, i64 0, i64 0
  call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str.2, i64 0, i64 0), i8* %t710)
  %t711 = getelementptr inbounds [51 x i8], [51 x i8]* @.str.1, i64 0, i64 0
  call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str.2, i64 0, i64 0), i8* %t711)
  br label %while.cond.23

while.cond.23:
  %t712 = call i32 @raylib_window_should_close()
  %t713 = icmp eq i32 %t712, 0
  br i1 %t713, label %while.body.23, label %while.end.23

while.body.23:
  %space_key.714 = alloca i1, align 8
  %t715 = call i32 @raylib_key_space()
  %t716 = call i32 @raylib_is_key_pressed(i32 %t715)
  %t717 = icmp ne i32 %t716, 0
  store i1 %t717, i1* %space_key.714, align 8
  %up_key.718 = alloca i1, align 8
  %t719 = call i32 @raylib_key_up()
  %t720 = call i32 @raylib_is_key_pressed(i32 %t719)
  %t721 = icmp ne i32 %t720, 0
  store i1 %t721, i1* %up_key.718, align 8
  %click.722 = alloca i1, align 8
  %t723 = call i32 @raylib_mouse_button_left()
  %t724 = call i32 @raylib_is_mouse_button_pressed(i32 %t723)
  %t725 = icmp ne i32 %t724, 0
  store i1 %t725, i1* %click.722, align 8
  %jump_requested.726 = alloca i1, align 8
  %t727 = load i1, i1* %space_key.714, align 4
  %t728 = load i1, i1* %up_key.718, align 4
  %t729 = or i1 %t727, %t728
  %t730 = load i1, i1* %click.722, align 4
  %t731 = or i1 %t729, %t730
  store i1 %t731, i1* %jump_requested.726, align 8
  %duck_held.732 = alloca i1, align 8
  %t733 = call i32 @raylib_key_down()
  %t734 = call i32 @raylib_is_key_down(i32 %t733)
  %t735 = icmp ne i32 %t734, 0
  store i1 %t735, i1* %duck_held.732, align 8
  %t736 = call i32 @raylib_key_escape()
  %t737 = call i32 @raylib_is_key_pressed(i32 %t736)
  %t738 = icmp ne i32 %t737, 0
  br i1 %t738, label %then.24, label %merge.24

then.24:
  br label %while.end.23
merge.24:
  %t739 = load i32, i32* %game_state.701, align 4
  %t740 = load i32, i32* %STATE_START.624, align 4
  %t741 = icmp eq i32 %t739, %t740
  br i1 %t741, label %then.25, label %merge.25

then.25:
  %t742 = load i1, i1* %jump_requested.726, align 4
  br i1 %t742, label %then.26, label %merge.26

then.26:
  %t743 = load i32, i32* %STATE_PLAYING.625, align 4
  store i32 %t743, i32* %game_state.701, align 4
  %t744 = load %struct.Dino*, %struct.Dino** %dino.630, align 8
  call void @Dino_jump(%struct.Dino* %t744)
  br label %merge.26

merge.26:
  br label %merge.25

merge.25:
  %t745 = load i32, i32* %game_state.701, align 4
  %t746 = load i32, i32* %STATE_PLAYING.625, align 4
  %t747 = icmp eq i32 %t745, %t746
  br i1 %t747, label %then.27, label %merge.27

then.27:
  %t748 = load i1, i1* %jump_requested.726, align 4
  br i1 %t748, label %then.28, label %merge.28

then.28:
  %t749 = load %struct.Dino*, %struct.Dino** %dino.630, align 8
  call void @Dino_jump(%struct.Dino* %t749)
  br label %merge.28

merge.28:
  %t750 = load %struct.Dino*, %struct.Dino** %dino.630, align 8
  %t751 = load i1, i1* %duck_held.732, align 4
  call void @Dino_update(%struct.Dino* %t750, i1 %t751)
  %t752 = load %struct.Obstacle*, %struct.Obstacle** %obs1.635, align 8
  %t753 = load double, double* %game_speed.703, align 8
  call void @Obstacle_update(%struct.Obstacle* %t752, double %t753)
  %t754 = load %struct.Obstacle*, %struct.Obstacle** %obs2.640, align 8
  %t755 = load double, double* %game_speed.703, align 8
  call void @Obstacle_update(%struct.Obstacle* %t754, double %t755)
  %t756 = load %struct.Obstacle*, %struct.Obstacle** %obs3.645, align 8
  %t757 = load double, double* %game_speed.703, align 8
  call void @Obstacle_update(%struct.Obstacle* %t756, double %t757)
  %t758 = load %struct.Obstacle*, %struct.Obstacle** %obs1.635, align 8
  %t759 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %t758, i32 0, i32 0
  %t760 = load double, double* %t759, align 8
  %t761 = fneg double 6.000000e+01
  %t762 = fcmp olt double %t760, %t761
  br i1 %t762, label %then.29, label %merge.29

then.29:
  %nxt_type.763 = alloca i32, align 8
  %t764 = load i32, i32* %score.704, align 4
  %t765 = srem i32 %t764, 3
  store i32 %t765, i32* %nxt_type.763, align 8
  %t766 = load i32, i32* %score.704, align 4
  %t767 = icmp sgt i32 %t766, 300
  %t768 = load i32, i32* %score.704, align 4
  %t769 = srem i32 %t768, 3
  %t770 = icmp eq i32 %t769, 0
  %t771 = and i1 %t767, %t770
  br i1 %t771, label %then.30, label %merge.30

then.30:
  store i32 3, i32* %nxt_type.763, align 4
  br label %merge.30

merge.30:
  %t772 = load %struct.Obstacle*, %struct.Obstacle** %obs1.635, align 8
  %t773 = load %struct.Obstacle*, %struct.Obstacle** %obs3.645, align 8
  %t774 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %t773, i32 0, i32 0
  %t775 = load double, double* %t774, align 8
  %t776 = fadd double %t775, 3.200000e+02
  %t777 = load i32, i32* %nxt_type.763, align 4
  call void @Obstacle_spawn(%struct.Obstacle* %t772, double %t776, i32 %t777)
  br label %merge.29

merge.29:
  %t778 = load %struct.Obstacle*, %struct.Obstacle** %obs2.640, align 8
  %t779 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %t778, i32 0, i32 0
  %t780 = load double, double* %t779, align 8
  %t781 = fneg double 6.000000e+01
  %t782 = fcmp olt double %t780, %t781
  br i1 %t782, label %then.31, label %merge.31

then.31:
  %nxt_type.783 = alloca i32, align 8
  %t784 = load i32, i32* %score.704, align 4
  %t785 = add nsw i32 %t784, 1
  %t786 = srem i32 %t785, 3
  store i32 %t786, i32* %nxt_type.783, align 8
  %t787 = load i32, i32* %score.704, align 4
  %t788 = icmp sgt i32 %t787, 450
  %t789 = load i32, i32* %score.704, align 4
  %t790 = srem i32 %t789, 4
  %t791 = icmp eq i32 %t790, 0
  %t792 = and i1 %t788, %t791
  br i1 %t792, label %then.32, label %merge.32

then.32:
  store i32 4, i32* %nxt_type.783, align 4
  br label %merge.32

merge.32:
  %t793 = load %struct.Obstacle*, %struct.Obstacle** %obs2.640, align 8
  %t794 = load %struct.Obstacle*, %struct.Obstacle** %obs1.635, align 8
  %t795 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %t794, i32 0, i32 0
  %t796 = load double, double* %t795, align 8
  %t797 = fadd double %t796, 3.400000e+02
  %t798 = load i32, i32* %nxt_type.783, align 4
  call void @Obstacle_spawn(%struct.Obstacle* %t793, double %t797, i32 %t798)
  br label %merge.31

merge.31:
  %t799 = load %struct.Obstacle*, %struct.Obstacle** %obs3.645, align 8
  %t800 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %t799, i32 0, i32 0
  %t801 = load double, double* %t800, align 8
  %t802 = fneg double 6.000000e+01
  %t803 = fcmp olt double %t801, %t802
  br i1 %t803, label %then.33, label %merge.33

then.33:
  %nxt_type.804 = alloca i32, align 8
  store i32 0, i32* %nxt_type.804, align 8
  %t805 = load i32, i32* %score.704, align 4
  %t806 = srem i32 %t805, 2
  %t807 = icmp eq i32 %t806, 0
  br i1 %t807, label %then.34, label %merge.34

then.34:
  store i32 1, i32* %nxt_type.804, align 4
  br label %merge.34

merge.34:
  %t808 = load %struct.Obstacle*, %struct.Obstacle** %obs3.645, align 8
  %t809 = load %struct.Obstacle*, %struct.Obstacle** %obs2.640, align 8
  %t810 = getelementptr inbounds %struct.Obstacle, %struct.Obstacle* %t809, i32 0, i32 0
  %t811 = load double, double* %t810, align 8
  %t812 = fadd double %t811, 3.300000e+02
  %t813 = load i32, i32* %nxt_type.804, align 4
  call void @Obstacle_spawn(%struct.Obstacle* %t808, double %t812, i32 %t813)
  br label %merge.33

merge.33:
  %t814 = load %struct.Cloud*, %struct.Cloud** %c1.653, align 8
  call void @Cloud_update(%struct.Cloud* %t814)
  %t815 = load %struct.Cloud*, %struct.Cloud** %c2.658, align 8
  call void @Cloud_update(%struct.Cloud* %t815)
  %t816 = load %struct.Cloud*, %struct.Cloud** %c3.663, align 8
  call void @Cloud_update(%struct.Cloud* %t816)
  %t817 = load %struct.GroundTile*, %struct.GroundTile** %g1.671, align 8
  %t818 = load double, double* %game_speed.703, align 8
  call void @GroundTile_update(%struct.GroundTile* %t817, double %t818)
  %t819 = load %struct.GroundTile*, %struct.GroundTile** %g2.676, align 8
  %t820 = load double, double* %game_speed.703, align 8
  call void @GroundTile_update(%struct.GroundTile* %t819, double %t820)
  %t821 = load %struct.GroundTile*, %struct.GroundTile** %g3.681, align 8
  %t822 = load double, double* %game_speed.703, align 8
  call void @GroundTile_update(%struct.GroundTile* %t821, double %t822)
  %t823 = load %struct.GroundTile*, %struct.GroundTile** %g4.686, align 8
  %t824 = load double, double* %game_speed.703, align 8
  call void @GroundTile_update(%struct.GroundTile* %t823, double %t824)
  %t825 = load %struct.GroundTile*, %struct.GroundTile** %g5.691, align 8
  %t826 = load double, double* %game_speed.703, align 8
  call void @GroundTile_update(%struct.GroundTile* %t825, double %t826)
  %t827 = load i32, i32* %score_timer.706, align 4
  %t828 = add nsw i32 %t827, 1
  store i32 %t828, i32* %score_timer.706, align 4
  %t829 = load i32, i32* %score_timer.706, align 4
  %t830 = icmp sge i32 %t829, 4
  br i1 %t830, label %then.35, label %merge.35

then.35:
  %t831 = load i32, i32* %score.704, align 4
  %t832 = add nsw i32 %t831, 1
  store i32 %t832, i32* %score.704, align 4
  store i32 0, i32* %score_timer.706, align 4
  %t833 = load i32, i32* %score.704, align 4
  %t834 = icmp sgt i32 %t833, 0
  %t835 = load i32, i32* %score.704, align 4
  %t836 = srem i32 %t835, 100
  %t837 = icmp eq i32 %t836, 0
  %t838 = and i1 %t834, %t837
  br i1 %t838, label %then.36, label %merge.36

then.36:
  store i32 30, i32* %flash_timer.708, align 4
  br label %merge.36

merge.36:
  %t839 = load i32, i32* %score.704, align 4
  %t840 = srem i32 %t839, 100
  %t841 = icmp eq i32 %t840, 0
  %t842 = load double, double* %game_speed.703, align 8
  %t843 = fcmp olt double %t842, 1.350000e+01
  %t844 = and i1 %t841, %t843
  br i1 %t844, label %then.37, label %merge.37

then.37:
  %t845 = load double, double* %game_speed.703, align 8
  %t846 = fadd double %t845, 3.500000e-01
  store double %t846, double* %game_speed.703, align 8
  br label %merge.37

merge.37:
  br label %merge.35

merge.35:
  %t847 = load %struct.Dino*, %struct.Dino** %dino.630, align 8
  %t848 = load %struct.Obstacle*, %struct.Obstacle** %obs1.635, align 8
  %t849 = call i1 @Dino_check_collision(%struct.Dino* %t847, %struct.Obstacle* %t848)
  %t850 = load %struct.Dino*, %struct.Dino** %dino.630, align 8
  %t851 = load %struct.Obstacle*, %struct.Obstacle** %obs2.640, align 8
  %t852 = call i1 @Dino_check_collision(%struct.Dino* %t850, %struct.Obstacle* %t851)
  %t853 = or i1 %t849, %t852
  %t854 = load %struct.Dino*, %struct.Dino** %dino.630, align 8
  %t855 = load %struct.Obstacle*, %struct.Obstacle** %obs3.645, align 8
  %t856 = call i1 @Dino_check_collision(%struct.Dino* %t854, %struct.Obstacle* %t855)
  %t857 = or i1 %t853, %t856
  br i1 %t857, label %then.38, label %merge.38

then.38:
  %t858 = load i32, i32* %STATE_GAMEOVER.626, align 4
  store i32 %t858, i32* %game_state.701, align 4
  %t859 = load i32, i32* %score.704, align 4
  %t860 = load i32, i32* %high_score.705, align 4
  %t861 = icmp sgt i32 %t859, %t860
  br i1 %t861, label %then.39, label %merge.39

then.39:
  %t862 = load i32, i32* %score.704, align 4
  store i32 %t862, i32* %high_score.705, align 4
  br label %merge.39

merge.39:
  br label %merge.38

merge.38:
  br label %merge.27

merge.27:
  %t863 = load i32, i32* %game_state.701, align 4
  %t864 = load i32, i32* %STATE_GAMEOVER.626, align 4
  %t865 = icmp eq i32 %t863, %t864
  br i1 %t865, label %then.40, label %merge.40

then.40:
  %t866 = load i1, i1* %jump_requested.726, align 4
  br i1 %t866, label %then.41, label %merge.41

then.41:
  %t867 = load %struct.Dino*, %struct.Dino** %dino.630, align 8
  call void @Dino_reset(%struct.Dino* %t867)
  %t868 = load %struct.Obstacle*, %struct.Obstacle** %obs1.635, align 8
  call void @Obstacle_spawn(%struct.Obstacle* %t868, double 7.500000e+02, i32 0)
  %t869 = load %struct.Obstacle*, %struct.Obstacle** %obs2.640, align 8
  call void @Obstacle_spawn(%struct.Obstacle* %t869, double 1.100000e+03, i32 1)
  %t870 = load %struct.Obstacle*, %struct.Obstacle** %obs3.645, align 8
  call void @Obstacle_spawn(%struct.Obstacle* %t870, double 1.450000e+03, i32 3)
  store i32 0, i32* %score.704, align 4
  store i32 0, i32* %score_timer.706, align 4
  store double 6.500000e+00, double* %game_speed.703, align 8
  %t871 = load i32, i32* %STATE_PLAYING.625, align 4
  store i32 %t871, i32* %game_state.701, align 4
  %t872 = load %struct.Dino*, %struct.Dino** %dino.630, align 8
  call void @Dino_jump(%struct.Dino* %t872)
  br label %merge.41

merge.41:
  br label %merge.40

merge.40:
  %is_night.873 = alloca i1, align 8
  store i1 false, i1* %is_night.873, align 8
  %t874 = load i32, i32* %score.704, align 4
  %t875 = sdiv i32 %t874, 500
  %t876 = srem i32 %t875, 2
  %t877 = icmp eq i32 %t876, 1
  br i1 %t877, label %then.42, label %merge.42

then.42:
  store i1 true, i1* %is_night.873, align 4
  br label %merge.42

merge.42:
  %col_bg.878 = alloca i32, align 8
  store i32 0, i32* %col_bg.878, align 8
  %col_fg.879 = alloca i32, align 8
  store i32 0, i32* %col_fg.879, align 8
  %col_cloud.880 = alloca i32, align 8
  store i32 0, i32* %col_cloud.880, align 8
  %col_eye.881 = alloca i32, align 8
  store i32 0, i32* %col_eye.881, align 8
  %col_ground.882 = alloca i32, align 8
  store i32 0, i32* %col_ground.882, align 8
  %t883 = load i1, i1* %is_night.873, align 4
  br i1 %t883, label %then.43, label %else.43

then.43:
  %t884 = call i32 @raylib_color(i32 32, i32 33, i32 36, i32 255)
  store i32 %t884, i32* %col_bg.878, align 4
  %t885 = call i32 @raylib_color(i32 230, i32 230, i32 230, i32 255)
  store i32 %t885, i32* %col_fg.879, align 4
  %t886 = call i32 @raylib_color(i32 55, i32 58, i32 64, i32 255)
  store i32 %t886, i32* %col_cloud.880, align 4
  %t887 = load i32, i32* %col_bg.878, align 4
  store i32 %t887, i32* %col_eye.881, align 4
  %t888 = call i32 @raylib_color(i32 100, i32 105, i32 115, i32 255)
  store i32 %t888, i32* %col_ground.882, align 4
  br label %merge.43

else.43:
  %t889 = call i32 @raylib_color(i32 247, i32 247, i32 247, i32 255)
  store i32 %t889, i32* %col_bg.878, align 4
  %t890 = call i32 @raylib_color(i32 83, i32 83, i32 83, i32 255)
  store i32 %t890, i32* %col_fg.879, align 4
  %t891 = call i32 @raylib_color(i32 218, i32 220, i32 224, i32 255)
  store i32 %t891, i32* %col_cloud.880, align 4
  %t892 = call i32 @raylib_white()
  store i32 %t892, i32* %col_eye.881, align 4
  %t893 = call i32 @raylib_color(i32 180, i32 180, i32 180, i32 255)
  store i32 %t893, i32* %col_ground.882, align 4
  br label %merge.43

merge.43:
  call void @raylib_begin_drawing()
  %t894 = load i32, i32* %col_bg.878, align 4
  call void @raylib_clear_background(i32 %t894)
  %t895 = load %struct.Cloud*, %struct.Cloud** %c1.653, align 8
  %t896 = load i32, i32* %col_cloud.880, align 4
  call void @Cloud_draw(%struct.Cloud* %t895, i32 %t896)
  %t897 = load %struct.Cloud*, %struct.Cloud** %c2.658, align 8
  %t898 = load i32, i32* %col_cloud.880, align 4
  call void @Cloud_draw(%struct.Cloud* %t897, i32 %t898)
  %t899 = load %struct.Cloud*, %struct.Cloud** %c3.663, align 8
  %t900 = load i32, i32* %col_cloud.880, align 4
  call void @Cloud_draw(%struct.Cloud* %t899, i32 %t900)
  %t901 = load i32, i32* %ground_y_pos.623, align 4
  %t902 = load i32, i32* %SCREEN_W.621, align 4
  %t903 = load i32, i32* %ground_y_pos.623, align 4
  %t904 = load i32, i32* %col_fg.879, align 4
  call void @raylib_draw_line(i32 0, i32 %t901, i32 %t902, i32 %t903, i32 %t904)
  %t905 = load %struct.GroundTile*, %struct.GroundTile** %g1.671, align 8
  %t906 = load i32, i32* %ground_y_pos.623, align 4
  %t907 = load i32, i32* %col_ground.882, align 4
  call void @GroundTile_draw(%struct.GroundTile* %t905, i32 %t906, i32 %t907)
  %t908 = load %struct.GroundTile*, %struct.GroundTile** %g2.676, align 8
  %t909 = load i32, i32* %ground_y_pos.623, align 4
  %t910 = load i32, i32* %col_ground.882, align 4
  call void @GroundTile_draw(%struct.GroundTile* %t908, i32 %t909, i32 %t910)
  %t911 = load %struct.GroundTile*, %struct.GroundTile** %g3.681, align 8
  %t912 = load i32, i32* %ground_y_pos.623, align 4
  %t913 = load i32, i32* %col_ground.882, align 4
  call void @GroundTile_draw(%struct.GroundTile* %t911, i32 %t912, i32 %t913)
  %t914 = load %struct.GroundTile*, %struct.GroundTile** %g4.686, align 8
  %t915 = load i32, i32* %ground_y_pos.623, align 4
  %t916 = load i32, i32* %col_ground.882, align 4
  call void @GroundTile_draw(%struct.GroundTile* %t914, i32 %t915, i32 %t916)
  %t917 = load %struct.GroundTile*, %struct.GroundTile** %g5.691, align 8
  %t918 = load i32, i32* %ground_y_pos.623, align 4
  %t919 = load i32, i32* %col_ground.882, align 4
  call void @GroundTile_draw(%struct.GroundTile* %t917, i32 %t918, i32 %t919)
  %t920 = load %struct.Obstacle*, %struct.Obstacle** %obs1.635, align 8
  %t921 = load i32, i32* %col_fg.879, align 4
  call void @Obstacle_draw(%struct.Obstacle* %t920, i32 %t921)
  %t922 = load %struct.Obstacle*, %struct.Obstacle** %obs2.640, align 8
  %t923 = load i32, i32* %col_fg.879, align 4
  call void @Obstacle_draw(%struct.Obstacle* %t922, i32 %t923)
  %t924 = load %struct.Obstacle*, %struct.Obstacle** %obs3.645, align 8
  %t925 = load i32, i32* %col_fg.879, align 4
  call void @Obstacle_draw(%struct.Obstacle* %t924, i32 %t925)
  %is_dead.926 = alloca i1, align 8
  %t927 = load i32, i32* %game_state.701, align 4
  %t928 = load i32, i32* %STATE_GAMEOVER.626, align 4
  %t929 = icmp eq i32 %t927, %t928
  store i1 %t929, i1* %is_dead.926, align 8
  %t930 = load %struct.Dino*, %struct.Dino** %dino.630, align 8
  %t931 = load i32, i32* %col_fg.879, align 4
  %t932 = load i32, i32* %col_eye.881, align 4
  %t933 = load i1, i1* %is_dead.926, align 4
  call void @Dino_draw(%struct.Dino* %t930, i32 %t931, i32 %t932, i1 %t933)
  %display_score.934 = alloca i32, align 8
  %t935 = load i32, i32* %score.704, align 4
  store i32 %t935, i32* %display_score.934, align 8
  %t936 = load i32, i32* %flash_timer.708, align 4
  %t937 = icmp sgt i32 %t936, 0
  br i1 %t937, label %then.44, label %merge.44

then.44:
  %t938 = load i32, i32* %flash_timer.708, align 4
  %t939 = sub nsw i32 %t938, 1
  store i32 %t939, i32* %flash_timer.708, align 4
  %t940 = load i32, i32* %flash_timer.708, align 4
  %t941 = sdiv i32 %t940, 4
  %t942 = srem i32 %t941, 2
  %t943 = icmp eq i32 %t942, 1
  br i1 %t943, label %then.45, label %merge.45

then.45:
  %t944 = load i32, i32* %score.704, align 4
  %t945 = sdiv i32 %t944, 100
  %t946 = mul nsw i32 %t945, 100
  store i32 %t946, i32* %display_score.934, align 4
  br label %merge.45

merge.45:
  br label %merge.44

merge.44:
  %cur_ten_thousands.947 = alloca i32, align 8
  %t948 = load i32, i32* %display_score.934, align 4
  %t949 = sdiv i32 %t948, 10000
  %t950 = srem i32 %t949, 10
  store i32 %t950, i32* %cur_ten_thousands.947, align 8
  %cur_thousands.951 = alloca i32, align 8
  %t952 = load i32, i32* %display_score.934, align 4
  %t953 = sdiv i32 %t952, 1000
  %t954 = srem i32 %t953, 10
  store i32 %t954, i32* %cur_thousands.951, align 8
  %cur_hundreds.955 = alloca i32, align 8
  %t956 = load i32, i32* %display_score.934, align 4
  %t957 = sdiv i32 %t956, 100
  %t958 = srem i32 %t957, 10
  store i32 %t958, i32* %cur_hundreds.955, align 8
  %cur_tens.959 = alloca i32, align 8
  %t960 = load i32, i32* %display_score.934, align 4
  %t961 = sdiv i32 %t960, 10
  %t962 = srem i32 %t961, 10
  store i32 %t962, i32* %cur_tens.959, align 8
  %cur_ones.963 = alloca i32, align 8
  %t964 = load i32, i32* %display_score.934, align 4
  %t965 = srem i32 %t964, 10
  store i32 %t965, i32* %cur_ones.963, align 8
  %hi_ten_thousands.966 = alloca i32, align 8
  %t967 = load i32, i32* %high_score.705, align 4
  %t968 = sdiv i32 %t967, 10000
  %t969 = srem i32 %t968, 10
  store i32 %t969, i32* %hi_ten_thousands.966, align 8
  %hi_thousands.970 = alloca i32, align 8
  %t971 = load i32, i32* %high_score.705, align 4
  %t972 = sdiv i32 %t971, 1000
  %t973 = srem i32 %t972, 10
  store i32 %t973, i32* %hi_thousands.970, align 8
  %hi_hundreds.974 = alloca i32, align 8
  %t975 = load i32, i32* %high_score.705, align 4
  %t976 = sdiv i32 %t975, 100
  %t977 = srem i32 %t976, 10
  store i32 %t977, i32* %hi_hundreds.974, align 8
  %hi_tens.978 = alloca i32, align 8
  %t979 = load i32, i32* %high_score.705, align 4
  %t980 = sdiv i32 %t979, 10
  %t981 = srem i32 %t980, 10
  store i32 %t981, i32* %hi_tens.978, align 8
  %hi_ones.982 = alloca i32, align 8
  %t983 = load i32, i32* %high_score.705, align 4
  %t984 = srem i32 %t983, 10
  store i32 %t984, i32* %hi_ones.982, align 8
  %s_buf.985 = alloca i8*, align 8
  %t986 = getelementptr inbounds [1 x i8], [1 x i8]* @.str.4, i64 0, i64 0
  %t987 = load i32, i32* %cur_ten_thousands.947, align 4
  %t988 = call i8* @_str_from_int(i32 %t987)
  %t989 = call i8* @_str_concat(i8* %t986, i8* %t988)
  %t990 = load i32, i32* %cur_thousands.951, align 4
  %t991 = call i8* @_str_from_int(i32 %t990)
  %t992 = call i8* @_str_concat(i8* %t989, i8* %t991)
  %t993 = load i32, i32* %cur_hundreds.955, align 4
  %t994 = call i8* @_str_from_int(i32 %t993)
  %t995 = call i8* @_str_concat(i8* %t992, i8* %t994)
  %t996 = load i32, i32* %cur_tens.959, align 4
  %t997 = call i8* @_str_from_int(i32 %t996)
  %t998 = call i8* @_str_concat(i8* %t995, i8* %t997)
  %t999 = load i32, i32* %cur_ones.963, align 4
  %t1000 = call i8* @_str_from_int(i32 %t999)
  %t1001 = call i8* @_str_concat(i8* %t998, i8* %t1000)
  store i8* %t1001, i8** %s_buf.985, align 8
  %h_buf.1002 = alloca i8*, align 8
  %t1003 = getelementptr inbounds [4 x i8], [4 x i8]* @.str.5, i64 0, i64 0
  %t1004 = load i32, i32* %hi_ten_thousands.966, align 4
  %t1005 = call i8* @_str_from_int(i32 %t1004)
  %t1006 = call i8* @_str_concat(i8* %t1003, i8* %t1005)
  %t1007 = load i32, i32* %hi_thousands.970, align 4
  %t1008 = call i8* @_str_from_int(i32 %t1007)
  %t1009 = call i8* @_str_concat(i8* %t1006, i8* %t1008)
  %t1010 = load i32, i32* %hi_hundreds.974, align 4
  %t1011 = call i8* @_str_from_int(i32 %t1010)
  %t1012 = call i8* @_str_concat(i8* %t1009, i8* %t1011)
  %t1013 = load i32, i32* %hi_tens.978, align 4
  %t1014 = call i8* @_str_from_int(i32 %t1013)
  %t1015 = call i8* @_str_concat(i8* %t1012, i8* %t1014)
  %t1016 = load i32, i32* %hi_ones.982, align 4
  %t1017 = call i8* @_str_from_int(i32 %t1016)
  %t1018 = call i8* @_str_concat(i8* %t1015, i8* %t1017)
  store i8* %t1018, i8** %h_buf.1002, align 8
  %t1019 = load i8*, i8** %h_buf.1002, align 8
  %t1020 = load i32, i32* %SCREEN_W.621, align 4
  %t1021 = sub nsw i32 %t1020, 220
  %t1022 = load i32, i32* %col_ground.882, align 4
  call void @raylib_draw_text(i8* %t1019, i32 %t1021, i32 22, i32 18, i32 %t1022)
  %t1023 = load i8*, i8** %s_buf.985, align 8
  %t1024 = load i32, i32* %SCREEN_W.621, align 4
  %t1025 = sub nsw i32 %t1024, 95
  %t1026 = load i32, i32* %col_fg.879, align 4
  call void @raylib_draw_text(i8* %t1023, i32 %t1025, i32 22, i32 18, i32 %t1026)
  %t1027 = load i32, i32* %game_state.701, align 4
  %t1028 = load i32, i32* %STATE_START.624, align 4
  %t1029 = icmp eq i32 %t1027, %t1028
  br i1 %t1029, label %then.46, label %merge.46

then.46:
  %t1030 = getelementptr inbounds [38 x i8], [38 x i8]* @.str.6, i64 0, i64 0
  %t1031 = load i32, i32* %SCREEN_W.621, align 4
  %t1032 = sdiv i32 %t1031, 2
  %t1033 = sub nsw i32 %t1032, 210
  %t1034 = load i32, i32* %col_fg.879, align 4
  call void @raylib_draw_text(i8* %t1030, i32 %t1033, i32 130, i32 20, i32 %t1034)
  %t1035 = getelementptr inbounds [55 x i8], [55 x i8]* @.str.7, i64 0, i64 0
  %t1036 = load i32, i32* %SCREEN_W.621, align 4
  %t1037 = sdiv i32 %t1036, 2
  %t1038 = sub nsw i32 %t1037, 195
  %t1039 = load i32, i32* %col_ground.882, align 4
  call void @raylib_draw_text(i8* %t1035, i32 %t1038, i32 170, i32 14, i32 %t1039)
  br label %merge.46

merge.46:
  %t1040 = load i32, i32* %game_state.701, align 4
  %t1041 = load i32, i32* %STATE_GAMEOVER.626, align 4
  %t1042 = icmp eq i32 %t1040, %t1041
  br i1 %t1042, label %then.47, label %merge.47

then.47:
  %t1043 = getelementptr inbounds [26 x i8], [26 x i8]* @.str.8, i64 0, i64 0
  %t1044 = load i32, i32* %SCREEN_W.621, align 4
  %t1045 = sdiv i32 %t1044, 2
  %t1046 = sub nsw i32 %t1045, 145
  %t1047 = load i32, i32* %col_fg.879, align 4
  call void @raylib_draw_text(i8* %t1043, i32 %t1046, i32 110, i32 22, i32 %t1047)
  %rx.1048 = alloca i32, align 8
  %t1049 = load i32, i32* %SCREEN_W.621, align 4
  %t1050 = sdiv i32 %t1049, 2
  store i32 %t1050, i32* %rx.1048, align 8
  %ry.1051 = alloca i32, align 8
  store i32 160, i32* %ry.1051, align 8
  %t1052 = load i32, i32* %rx.1048, align 4
  %t1053 = load i32, i32* %ry.1051, align 4
  %t1054 = load i32, i32* %col_fg.879, align 4
  call void @raylib_draw_circle_lines(i32 %t1052, i32 %t1053, double 1.600000e+01, i32 %t1054)
  %t1055 = load i32, i32* %rx.1048, align 4
  %t1056 = load i32, i32* %ry.1051, align 4
  %t1057 = load i32, i32* %col_fg.879, align 4
  call void @raylib_draw_circle_lines(i32 %t1055, i32 %t1056, double 1.700000e+01, i32 %t1057)
  %t1058 = load i32, i32* %rx.1048, align 4
  %t1059 = sub nsw i32 %t1058, 8
  %t1060 = load i32, i32* %ry.1051, align 4
  %t1061 = sub nsw i32 %t1060, 18
  %t1062 = load i32, i32* %col_bg.878, align 4
  call void @raylib_draw_rectangle(i32 %t1059, i32 %t1061, i32 16, i32 8, i32 %t1062)
  %t1063 = load i32, i32* %rx.1048, align 4
  %t1064 = add nsw i32 %t1063, 2
  %t1065 = load i32, i32* %ry.1051, align 4
  %t1066 = sub nsw i32 %t1065, 18
  %t1067 = load i32, i32* %rx.1048, align 4
  %t1068 = add nsw i32 %t1067, 10
  %t1069 = load i32, i32* %ry.1051, align 4
  %t1070 = sub nsw i32 %t1069, 14
  %t1071 = load i32, i32* %col_fg.879, align 4
  call void @raylib_draw_line(i32 %t1064, i32 %t1066, i32 %t1068, i32 %t1070, i32 %t1071)
  %t1072 = load i32, i32* %rx.1048, align 4
  %t1073 = add nsw i32 %t1072, 10
  %t1074 = load i32, i32* %ry.1051, align 4
  %t1075 = sub nsw i32 %t1074, 14
  %t1076 = load i32, i32* %rx.1048, align 4
  %t1077 = add nsw i32 %t1076, 4
  %t1078 = load i32, i32* %ry.1051, align 4
  %t1079 = sub nsw i32 %t1078, 10
  %t1080 = load i32, i32* %col_fg.879, align 4
  call void @raylib_draw_line(i32 %t1073, i32 %t1075, i32 %t1077, i32 %t1079, i32 %t1080)
  %t1081 = getelementptr inbounds [32 x i8], [32 x i8]* @.str.9, i64 0, i64 0
  %t1082 = load i32, i32* %SCREEN_W.621, align 4
  %t1083 = sdiv i32 %t1082, 2
  %t1084 = sub nsw i32 %t1083, 120
  %t1085 = load i32, i32* %col_ground.882, align 4
  call void @raylib_draw_text(i8* %t1081, i32 %t1084, i32 195, i32 14, i32 %t1085)
  br label %merge.47

merge.47:
  call void @raylib_end_drawing()
  br label %while.cond.23

while.end.23:
  call void @raylib_close_window()
  %t1086 = load i32, i32* %high_score.705, align 4
  call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([40 x i8], [40 x i8]* @.str.10, i64 0, i64 0), i32 %t1086)
  ret i32 0
}
