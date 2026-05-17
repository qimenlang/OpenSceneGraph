cmake_minimum_required(VERSION 3.7)
project(freeglut)

set (HEADER_FILES 
./src/fg_gl2.h
./src/fg_init.h
./src/fg_internal.h
./src/fg_teapot_data.h
./src/fg_version.h
./src/mswin/fg_internal_mswin.h
./src/util/xparsegeometry_repl.h)


set (SOURCE_FILES 
./src/fg_callbacks.c
./src/fg_cursor.c
./src/fg_display.c
./src/fg_ext.c
./src/fg_font.c
./src/fg_font_data.c
./src/fg_gamemode.c
./src/fg_geometry.c
./src/fg_gl2.c
./src/fg_init.c
./src/fg_input_devices.c
./src/fg_joystick.c
./src/fg_main.c
./src/fg_menu.c
./src/fg_misc.c
./src/fg_overlay.c
./src/fg_spaceball.c
./src/fg_state.c
./src/fg_stroke_mono_roman.c
./src/fg_stroke_roman.c
./src/fg_structure.c
./src/fg_teapot.c
./src/fg_videoresize.c
./src/fg_window.c
./src/mswin/fg_cursor_mswin.c
./src/mswin/fg_display_mswin.c
./src/mswin/fg_ext_mswin.c
./src/mswin/fg_gamemode_mswin.c
./src/mswin/fg_init_mswin.c
./src/mswin/fg_input_devices_mswin.c
./src/mswin/fg_joystick_mswin.c
./src/mswin/fg_main_mswin.c
./src/mswin/fg_menu_mswin.c
./src/mswin/fg_spaceball_mswin.c
./src/mswin/fg_state_mswin.c
./src/mswin/fg_structure_mswin.c
./src/mswin/fg_window_mswin.c
./src/util/xparsegeometry_repl.c
)

add_definitions(-DFREEGLUT_EXPORTS)

include_directories (./include)
include_directories (./src/util)

add_library(freeglut SHARED ${HEADER_FILES} ${SOURCE_FILES})


