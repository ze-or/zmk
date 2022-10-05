#
# Copyright (c) 2021 Darryl deHaan
# SPDX-License-Identifier: MIT
#
target_sources_ifdef(CONFIG_CUSTOM_WIDGET_BATTERY_STATUS app PRIVATE battery_status.c)
target_sources_ifdef(CONFIG_CUSTOM_WIDGET_OUTPUT_STATUS app PRIVATE output_status.c)
target_sources_ifdef(CONFIG_CUSTOM_WIDGET_LAYER_STATUS app PRIVATE layer_status.c)
target_sources_ifdef(CONFIG_CUSTOM_WIDGET_PERIPHERAL_STATUS app PRIVATE peripheral_status.c)
add_subdirectory(icons)
