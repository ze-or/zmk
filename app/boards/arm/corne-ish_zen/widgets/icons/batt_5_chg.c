/*
*
* Copyright (c) 2021 Darryl deHaan
* SPDX-License-Identifier: MIT
*
*/

#include <lvgl.h>


#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_IMG_BATT_5_CHG
#define LV_ATTRIBUTE_IMG_BATT_5_CHG
#endif

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_BATT_5_CHG uint8_t batt_5_chg_map[] = {
  0xff, 0xff, 0xff, 0xff, 	/*Color of index 0*/
  0x00, 0x00, 0x00, 0xff, 	/*Color of index 1*/

  0x00, 0x00, 0x00, 0x20, 0x00, 
  0x00, 0x00, 0x00, 0x60, 0x00, 
  0x00, 0x00, 0x00, 0xc0, 0x00, 
  0x00, 0x00, 0x01, 0xc0, 0x00, 
  0x00, 0x00, 0x03, 0x80, 0x00, 
  0x7f, 0xff, 0xf7, 0xbf, 0xf8, 
  0xff, 0xff, 0xef, 0xbf, 0xfc, 
  0xff, 0xff, 0xdf, 0x7f, 0xfc, 
  0xff, 0xff, 0xbf, 0x7f, 0xfc, 
  0xf0, 0x00, 0x7e, 0x00, 0x3f, 
  0xf0, 0x00, 0xfe, 0x00, 0x3f, 
  0xf3, 0x01, 0xfe, 0x00, 0x3f, 
  0xf3, 0x03, 0xfc, 0x00, 0x3f, 
  0xf3, 0x07, 0xfc, 0x00, 0x0f, 
  0xf3, 0x0f, 0xff, 0xe0, 0x0f, 
  0xf3, 0x1f, 0xff, 0xc0, 0x0f, 
  0xf3, 0x00, 0x7f, 0x80, 0x0f, 
  0xf3, 0x00, 0x7f, 0x00, 0x3f, 
  0xf3, 0x00, 0xfe, 0x00, 0x3f, 
  0xf0, 0x00, 0xfc, 0x00, 0x3f, 
  0xf0, 0x01, 0xf8, 0x00, 0x3f, 
  0xff, 0xfd, 0xf7, 0xff, 0xfc, 
  0xff, 0xfb, 0xef, 0xff, 0xfc, 
  0xff, 0xfb, 0xdf, 0xff, 0xfc, 
  0x7f, 0xfb, 0xbf, 0xff, 0xf8, 
  0x00, 0x07, 0x00, 0x00, 0x00, 
  0x00, 0x07, 0x00, 0x00, 0x00, 
  0x00, 0x0e, 0x00, 0x00, 0x00, 
  0x00, 0x0c, 0x00, 0x00, 0x00, 
  0x00, 0x18, 0x00, 0x00, 0x00, 
  0x00, 0x10, 0x00, 0x00, 0x00, 
};

const lv_img_dsc_t batt_5_chg = {
  .header.always_zero = 0,
  .header.w = 40,
  .header.h = 31,
  .data_size = 163,
  .header.cf = LV_IMG_CF_INDEXED_1BIT,
  .data = batt_5_chg_map,
};

