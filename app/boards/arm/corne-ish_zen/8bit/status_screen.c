#include <zmk/display/status_screen.h>
#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/usb.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_selection_changed.h>
#else
#include <zmk/split/bluetooth/peripheral.h>
#include <zmk/events/split_peripheral_status_changed.h>
#endif

#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

LV_IMG_DECLARE(jack_full);
LV_IMG_DECLARE(queen_full);
LV_IMG_DECLARE(spade);
LV_IMG_DECLARE(heart);
LV_IMG_DECLARE(diamond);
LV_IMG_DECLARE(club);
LV_IMG_DECLARE(club_r);
LV_IMG_DECLARE(usb);
LV_IMG_DECLARE(usb_r);
LV_IMG_DECLARE(excl);
LV_IMG_DECLARE(excl_r);
LV_IMG_DECLARE(question);
LV_IMG_DECLARE(question_r);

lv_obj_t *suit_tl, *suit_br, *rank_tl, *rank_br;

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
struct output_status_state {
    enum zmk_endpoint selected_endpoint;
    bool active_profile_connected;
    bool active_profile_bonded;
    uint8_t active_profile_index;
};

static struct output_status_state get_state(const zmk_event_t *_eh) {
    return (struct output_status_state){.selected_endpoint = zmk_endpoints_selected(),
                                        .active_profile_connected =
                                            zmk_ble_active_profile_is_connected(),
                                        .active_profile_bonded = !zmk_ble_active_profile_is_open(),
                                        .active_profile_index = zmk_ble_active_profile_index()};
    ;
}

static void output_status_update_cb(struct output_status_state state) {
    switch (state.selected_endpoint) {
    case ZMK_ENDPOINT_USB:
        lv_img_set_src(rank_tl, &usb);
        lv_img_set_src(rank_br, &usb_r);
        break;
    case ZMK_ENDPOINT_BLE:
        if (state.active_profile_bonded && state.active_profile_connected) {
            lv_obj_set_hidden(rank_tl, true);
            lv_obj_set_hidden(rank_br, true);
        } else {
            if (state.active_profile_bonded) {
                lv_img_set_src(rank_tl, &excl);
                lv_img_set_src(rank_br, &excl_r);
            } else {
                lv_img_set_src(rank_tl, &question);
                lv_img_set_src(rank_br, &question_r);
            }
            lv_obj_set_hidden(rank_tl, false);
            lv_obj_set_hidden(rank_br, false);
        }
    }

    const lv_img_dsc_t *profile_suit, *profile_suit_r;
    switch (state.active_profile_index) {
    case 1:
        profile_suit = &heart;
        profile_suit_r = &spade;
        break;
    case 2:
        profile_suit = &diamond;
        profile_suit_r = &diamond;
        break;
    case 3:
        profile_suit = &club;
        profile_suit_r = &club_r;
        break;
    default:
        profile_suit = &spade;
        profile_suit_r = &heart;
        break;
    }
    lv_img_set_src(suit_tl, profile_suit);
    lv_img_set_src(suit_br, profile_suit_r);
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_output_status, struct output_status_state,
                            output_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_output_status, zmk_endpoint_selection_changed);
ZMK_SUBSCRIPTION(widget_output_status, zmk_usb_conn_state_changed);
ZMK_SUBSCRIPTION(widget_output_status, zmk_ble_active_profile_changed);
#else // peripheral
struct peripheral_status_state {
    bool connected;
};

static struct peripheral_status_state get_state(const zmk_event_t *_eh) {
    return (struct peripheral_status_state){.connected = zmk_split_bt_peripheral_is_connected()};
}

static void output_status_update_cb(struct peripheral_status_state state) {
    lv_obj_set_hidden(rank_tl, state.connected);
    lv_obj_set_hidden(rank_br, state.connected);
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_peripheral_status, struct peripheral_status_state,
                            output_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_peripheral_status, zmk_split_peripheral_status_changed);
#endif

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen = lv_obj_create(NULL, NULL);

    lv_obj_t *card_img = lv_img_create(screen, NULL);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    lv_img_set_src(card_img, &jack_full);

    suit_tl = lv_img_create(screen, NULL);
    suit_br = lv_img_create(screen, NULL);
    lv_obj_align(suit_tl, NULL, LV_ALIGN_IN_TOP_LEFT, 8, 27);
    lv_obj_align(suit_br, NULL, LV_ALIGN_IN_TOP_LEFT, 62, 90);
#else
    lv_img_set_src(card_img, &queen_full);
#endif

    rank_tl = lv_img_create(screen, NULL);
    rank_br = lv_img_create(screen, NULL);
    lv_obj_align(rank_tl, NULL, LV_ALIGN_IN_TOP_LEFT, 8, 12);
    lv_obj_align(rank_br, NULL, LV_ALIGN_IN_TOP_LEFT, 62, 105);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    widget_output_status_init();
#else
    lv_img_set_src(rank_tl, &excl);
    lv_img_set_src(rank_br, &excl_r);

    widget_peripheral_status_init();
#endif

    return screen;
}
