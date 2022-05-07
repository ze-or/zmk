#include <zmk/event_manager.h>
#include <zmk/events/wpm_state_changed.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display/widgets/ffkb_fox.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

enum anim_state {
    anim_state_none,
    anim_state_idle,
    anim_state_slow,
    anim_state_fast
} current_anim_state;

LV_IMG_DECLARE(fox_mid);
LV_IMG_DECLARE(fox_down);
LV_IMG_DECLARE(fox_bot);
LV_IMG_DECLARE(fox_up);

const void *images[] = {&fox_mid, &fox_down, &fox_bot, &fox_up};

void set_img_src(void *var, lv_anim_value_t val) {
    lv_obj_t *img = (lv_obj_t *)var;
    lv_img_set_src(img, images[val]);
}

void update_fox_wpm(struct zmk_widget_fox *widget, int wpm) {
    LOG_DBG("anim state %d", current_anim_state);
    if (wpm == 0) {
        if (current_anim_state != anim_state_idle) {
            LOG_DBG("anim state idle");
            lv_anim_del(widget->obj, set_img_src);
            lv_img_set_src(widget->obj, &fox_mid);
            current_anim_state = anim_state_idle;
        }
    } else if (wpm < CONFIG_ZMK_WIDGET_FOX_SLOW_LIMIT) {
        if (current_anim_state != anim_state_slow) {
            LOG_DBG("anim state slow");
            lv_anim_init(&widget->anim);
            lv_anim_set_time(&widget->anim, 1000);
            lv_anim_set_repeat_delay(&widget->anim, 250);
            lv_anim_set_var(&widget->anim, widget->obj);
            lv_anim_set_values(&widget->anim, 0, 3);
            lv_anim_set_exec_cb(&widget->anim, set_img_src);
            lv_anim_set_repeat_count(&widget->anim, LV_ANIM_REPEAT_INFINITE);

            lv_anim_start(&widget->anim);
            current_anim_state = anim_state_slow;
        }
    } else if (current_anim_state != anim_state_fast) {
        LOG_DBG("anim state fast");
        lv_anim_init(&widget->anim);
        lv_anim_set_time(&widget->anim, 500);
        lv_anim_set_repeat_delay(&widget->anim, 125);
        lv_anim_set_var(&widget->anim, widget->obj);
        lv_anim_set_values(&widget->anim, 0, 3);
        lv_anim_set_exec_cb(&widget->anim, set_img_src);
        lv_anim_set_repeat_count(&widget->anim, LV_ANIM_REPEAT_INFINITE);

        lv_anim_start(&widget->anim);
        current_anim_state = anim_state_fast;
    }
}

int zmk_widget_fox_init(struct zmk_widget_fox *widget, lv_obj_t *parent) {
    widget->obj = lv_img_create(parent, NULL);
    lv_img_set_auto_size(widget->obj, true);

    update_fox_wpm(widget, 0);

    sys_slist_append(&widgets, &widget->node);

    return 0;
}

lv_obj_t *zmk_widget_fox_obj(struct zmk_widget_fox *widget) { return widget->obj; }

int fox_listener(const zmk_event_t *eh) {
    struct zmk_widget_fox *widget;
    struct zmk_wpm_state_changed *ev = as_zmk_wpm_state_changed(eh);
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        LOG_DBG("Set the WPM %d", ev->state);
        update_fox_wpm(widget, ev->state);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(zmk_widget_fox, fox_listener)
ZMK_SUBSCRIPTION(zmk_widget_fox, zmk_wpm_state_changed);
