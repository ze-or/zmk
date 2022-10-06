zephyr_library()

if(CONFIG_ZMK_DISPLAY)
  target_sources_ifdef(CONFIG_ZEN_STATUS_SCREEN_STOCK app PRIVATE custom_status_screen.c)
  add_subdirectory_ifdef(CONFIG_ZEN_STATUS_SCREEN_STOCK widgets)

  add_subdirectory_ifdef(CONFIG_ZEN_STATUS_SCREEN_8BIT 8bit)
endif()

zephyr_library_include_directories(${ZEPHYR_LVGL_MODULE_DIR})
zephyr_library_include_directories(${ZEPHYR_BASE}/lib/gui/lvgl/)
zephyr_library_sources(${ZEPHYR_BASE}/misc/empty_file.c)
zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)
