#include <lvgl.h>
#include <kernel.h>

struct zmk_widget_fox {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_anim_t anim;
};

int zmk_widget_fox_init(struct zmk_widget_fox *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_fox_obj(struct zmk_widget_fox *widget);
