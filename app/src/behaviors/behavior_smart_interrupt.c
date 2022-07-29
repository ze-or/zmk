/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_smart_interrupt

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>
#include <zmk/behavior.h>
#include <zmk/behavior_queue.h>
#include <zmk/keymap.h>
#include <zmk/matrix.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/hid.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define ZMK_BHV_MAX_ACTIVE_SMART_INTERRUPTS 10

struct behavior_smart_interrupt_config {
    int32_t ignored_key_positions_len;
    int32_t ignored_layers_len;
    struct zmk_behavior_binding start_behavior;
    struct zmk_behavior_binding continue_behavior;
    struct zmk_behavior_binding end_behavior;
    int32_t ignored_layers;
    int32_t timeout_ms;
    int tap_ms;
    uint8_t ignored_key_positions[];
};

struct active_smart_interrupt {
    bool is_active;
    bool is_pressed;
    bool first_press;
    uint32_t position;
    const struct behavior_smart_interrupt_config *config;
    struct k_work_delayable release_timer;
    int64_t release_at;
    bool timer_started;
    bool timer_cancelled;
};

static int stop_timer(struct active_smart_interrupt *smart_interrupt) {
    int timer_cancel_result = k_work_cancel_delayable(&smart_interrupt->release_timer);
    if (timer_cancel_result == -EINPROGRESS) {
        // too late to cancel, we'll let the timer handler clear up.
        smart_interrupt->timer_cancelled = true;
    }
    return timer_cancel_result;
}

static void reset_timer(int32_t timestamp, struct active_smart_interrupt *smart_interrupt) {
    smart_interrupt->release_at = timestamp + smart_interrupt->config->timeout_ms;
    int32_t ms_left = smart_interrupt->release_at - k_uptime_get();
    if (ms_left > 0) {
        k_work_schedule(&smart_interrupt->release_timer, K_MSEC(ms_left));
        LOG_DBG("Successfully reset smart interrupt timer");
    }
}

void trigger_end_behavior(struct active_smart_interrupt *si) {
    zmk_behavior_queue_add(si->position, si->config->end_behavior, true, si->config->tap_ms);
    zmk_behavior_queue_add(si->position, si->config->end_behavior, false, 0);
}

void behavior_smart_interrupt_timer_handler(struct k_work *item) {
    struct active_smart_interrupt *smart_interrupt =
        CONTAINER_OF(item, struct active_smart_interrupt, release_timer);
    if (!smart_interrupt->is_active || smart_interrupt->timer_cancelled ||
        smart_interrupt->is_pressed) {
        return;
    }
    LOG_DBG("Smart interrupted deactivated due to timer");
    smart_interrupt->is_active = false;
    trigger_end_behavior(smart_interrupt);
}

static void clear_smart_interrupt(struct active_smart_interrupt *smart_interrupt) {
    smart_interrupt->is_active = false;
}

struct active_smart_interrupt active_smart_interrupts[ZMK_BHV_MAX_ACTIVE_SMART_INTERRUPTS] = {};

static struct active_smart_interrupt *find_smart_interrupt(uint32_t position) {
    for (int i = 0; i < ZMK_BHV_MAX_ACTIVE_SMART_INTERRUPTS; i++) {
        if (active_smart_interrupts[i].position == position &&
            active_smart_interrupts[i].is_active) {
            return &active_smart_interrupts[i];
        }
    }
    return NULL;
}

static int new_smart_interrupt(uint32_t position,
                               const struct behavior_smart_interrupt_config *config,
                               struct active_smart_interrupt **smart_interrupt) {
    for (int i = 0; i < ZMK_BHV_MAX_ACTIVE_SMART_INTERRUPTS; i++) {
        struct active_smart_interrupt *const ref_smart_interrupt = &active_smart_interrupts[i];
        if (!ref_smart_interrupt->is_active) {
            ref_smart_interrupt->position = position;
            ref_smart_interrupt->config = config;
            ref_smart_interrupt->is_active = true;
            ref_smart_interrupt->is_pressed = false;
            ref_smart_interrupt->first_press = true;
            *smart_interrupt = ref_smart_interrupt;
            return 0;
        }
    }
    return -ENOMEM;
}

static bool is_other_key_ignored(struct active_smart_interrupt *smart_interrupt, int32_t position) {
    for (int i = 0; i < smart_interrupt->config->ignored_key_positions_len; i++) {
        if (smart_interrupt->config->ignored_key_positions[i] == position) {
            return true;
        }
    }
    return false;
}

static bool is_layer_ignored(struct active_smart_interrupt *smart_interrupt, int32_t layer) {
    if ((BIT(layer) & smart_interrupt->config->ignored_layers) != 0U) {
        return true;
    }
    return false;
}

static int on_smart_interrupt_binding_pressed(struct zmk_behavior_binding *binding,
                                              struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_smart_interrupt_config *cfg = dev->config;
    struct active_smart_interrupt *smart_interrupt;
    smart_interrupt = find_smart_interrupt(event.position);
    if (smart_interrupt == NULL) {
        if (new_smart_interrupt(event.position, cfg, &smart_interrupt) == -ENOMEM) {
            LOG_ERR("Unable to create new smart_interrupt. Insufficient space in "
                    "active_smart_interrupts[].");
            return ZMK_BEHAVIOR_OPAQUE;
        }
        LOG_DBG("%d created new smart_interrupt", event.position);
    }
    LOG_DBG("%d smart_interrupt pressed", event.position);
    smart_interrupt->is_pressed = true;
    if (smart_interrupt->first_press) {
        behavior_keymap_binding_pressed((struct zmk_behavior_binding *)&cfg->start_behavior, event);
        behavior_keymap_binding_released((struct zmk_behavior_binding *)&cfg->start_behavior,
                                         event);
        smart_interrupt->first_press = false;
    }
    behavior_keymap_binding_pressed((struct zmk_behavior_binding *)&cfg->continue_behavior, event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static void release_smart_interrupt(struct zmk_behavior_binding_event event,
                                    struct zmk_behavior_binding *continue_behavior) {
    struct active_smart_interrupt *smart_interrupt = find_smart_interrupt(event.position);
    if (smart_interrupt == NULL) {
        return;
    }
    smart_interrupt->is_pressed = false;
    behavior_keymap_binding_released(continue_behavior, event);
    reset_timer(k_uptime_get(), smart_interrupt);
}

static int on_smart_interrupt_binding_released(struct zmk_behavior_binding *binding,
                                               struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_smart_interrupt_config *cfg = dev->config;
    LOG_DBG("%d smart_interrupt keybind released", event.position);
    release_smart_interrupt(event, (struct zmk_behavior_binding *)&cfg->continue_behavior);
    return ZMK_BEHAVIOR_OPAQUE;
}

static int behavior_smart_interrupt_init(const struct device *dev) {
    static bool init_first_run = true;
    if (init_first_run) {
        for (int i = 0; i < ZMK_BHV_MAX_ACTIVE_SMART_INTERRUPTS; i++) {
            k_work_init_delayable(&active_smart_interrupts[i].release_timer,
                                  behavior_smart_interrupt_timer_handler);
            clear_smart_interrupt(&active_smart_interrupts[i]);
        }
    }
    init_first_run = false;
    return 0;
}

static const struct behavior_driver_api behavior_smart_interrupt_driver_api = {
    .binding_pressed = on_smart_interrupt_binding_pressed,
    .binding_released = on_smart_interrupt_binding_released,
};

static int smart_interrupt_listener(const zmk_event_t *eh);
static int smart_interrupt_position_state_changed_listener(const zmk_event_t *eh);
static int smart_interrupt_layer_state_changed_listener(const zmk_event_t *eh);

ZMK_LISTENER(behavior_smart_interrupt, smart_interrupt_listener);
ZMK_SUBSCRIPTION(behavior_smart_interrupt, zmk_position_state_changed);
ZMK_SUBSCRIPTION(behavior_smart_interrupt, zmk_layer_state_changed);

static int smart_interrupt_listener(const zmk_event_t *eh) {
    if (as_zmk_position_state_changed(eh) != NULL) {
        return smart_interrupt_position_state_changed_listener(eh);
    } else if (as_zmk_layer_state_changed(eh) != NULL) {
        return smart_interrupt_layer_state_changed_listener(eh);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

static int smart_interrupt_position_state_changed_listener(const zmk_event_t *eh) {
    struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    for (int i = 0; i < ZMK_BHV_MAX_ACTIVE_SMART_INTERRUPTS; i++) {
        struct active_smart_interrupt *smart_interrupt = &active_smart_interrupts[i];
        if (!smart_interrupt->is_active) {
            continue;
        }
        if (smart_interrupt->position == ev->position) {
            continue;
        }
        if (!is_other_key_ignored(smart_interrupt, ev->position)) {
            LOG_DBG("Smart Interrupt interrupted, ending at %d %d", smart_interrupt->position,
                    ev->position);
            smart_interrupt->is_active = false;
            struct zmk_behavior_binding_event event = {.position = smart_interrupt->position,
                                                       .timestamp = k_uptime_get()};
            if (smart_interrupt->is_pressed) {
                behavior_keymap_binding_released(
                    (struct zmk_behavior_binding *)&smart_interrupt->config->continue_behavior,
                    event);
            }
            trigger_end_behavior(smart_interrupt);
            return ZMK_EV_EVENT_BUBBLE;
        }
        if (ev->state) {
            stop_timer(smart_interrupt);
        } else {
            reset_timer(ev->timestamp, smart_interrupt);
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}

static int smart_interrupt_layer_state_changed_listener(const zmk_event_t *eh) {
    struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    if (!ev->state) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    for (int i = 0; i < ZMK_BHV_MAX_ACTIVE_SMART_INTERRUPTS; i++) {
        struct active_smart_interrupt *smart_interrupt = &active_smart_interrupts[i];
        if (!smart_interrupt->is_active) {
            continue;
        }
        if (!is_layer_ignored(smart_interrupt, ev->layer)) {
            LOG_DBG("Smart Interrupt layer changed, ending at %d %d", smart_interrupt->position,
                    ev->layer);
            smart_interrupt->is_active = false;
            struct zmk_behavior_binding_event event = {.position = smart_interrupt->position,
                                                       .timestamp = k_uptime_get()};
            if (smart_interrupt->is_pressed) {
                behavior_keymap_binding_released(
                    (struct zmk_behavior_binding *)&smart_interrupt->config->continue_behavior,
                    event);
            }
            behavior_keymap_binding_pressed(
                (struct zmk_behavior_binding *)&smart_interrupt->config->end_behavior, event);
            behavior_keymap_binding_released(
                (struct zmk_behavior_binding *)&smart_interrupt->config->end_behavior, event);
            return ZMK_EV_EVENT_BUBBLE;
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}

#define _TRANSFORM_ENTRY(idx, node)                                                                \
    {                                                                                              \
        .behavior_dev = DT_LABEL(DT_INST_PHANDLE_BY_IDX(node, bindings, idx)),                     \
        .param1 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param1), (0),       \
                              (DT_INST_PHA_BY_IDX(node, bindings, idx, param1))),                  \
        .param2 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param2), (0),       \
                              (DT_INST_PHA_BY_IDX(node, bindings, idx, param2))),                  \
    }

#define IF_BIT(n, prop, i) BIT(DT_PROP_BY_IDX(n, prop, i)) |

#define SMART_INTERRUPT_INST(n)                                                                    \
    static struct behavior_smart_interrupt_config behavior_smart_interrupt_config_##n = {          \
        .ignored_key_positions = DT_INST_PROP(n, ignored_key_positions),                           \
        .ignored_key_positions_len = DT_INST_PROP_LEN(n, ignored_key_positions),                   \
        .ignored_layers = DT_INST_FOREACH_PROP_ELEM(n, ignored_layers, IF_BIT) 0,                  \
        .ignored_layers_len = DT_INST_PROP_LEN(n, ignored_layers),                                 \
        .timeout_ms = DT_INST_PROP(n, timeout_ms),                                                 \
        .tap_ms = DT_INST_PROP(n, tap_ms),                                                         \
        .start_behavior = _TRANSFORM_ENTRY(0, n),                                                  \
        .continue_behavior = _TRANSFORM_ENTRY(1, n),                                               \
        .end_behavior = _TRANSFORM_ENTRY(2, n)};                                                   \
    DEVICE_DT_INST_DEFINE(                                                                         \
        n, behavior_smart_interrupt_init, NULL, NULL, &behavior_smart_interrupt_config_##n,        \
        APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_smart_interrupt_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SMART_INTERRUPT_INST)
