#include <lvgl.h>
#include <kernel.h>

struct ffkb_fox_widget {
    sys_snode_t node;
    lv_obj_t *obj;
    // lv_anim_t anim;
};

int ffkb_fox_widget_init(struct ffkb_fox_widget *widget, lv_obj_t *parent);
lv_obj_t *ffkb_fox_widget_obj(struct ffkb_fox_widget *widget);
