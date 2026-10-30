/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "encoder_only_foc.h"
#include <math.h>
#include <stddef.h>

bool encoder_only_foc_config_valid(const volatile mc_configuration *conf) {
    return conf != NULL && conf->motor_type == MOTOR_TYPE_FOC &&
           conf->foc_sensor_mode == FOC_SENSOR_MODE_ENCODER &&
           conf->m_sensor_port_mode == SENSOR_PORT_MODE_CUSTOM_ENCODER &&
           conf->foc_speed_soure == FOC_SPEED_SRC_CORRECTED &&
           conf->foc_offsets_cal_mode == 1 &&
           !conf->foc_phase_filter_enable && conf->foc_fw_current_max == 0.0f &&
           conf->foc_cc_decoupling == FOC_CC_DECOUPLING_DISABLED &&
           isfinite(conf->foc_encoder_ratio) && conf->foc_encoder_ratio > 0.0f &&
           isfinite(conf->foc_encoder_offset) &&
           isfinite(conf->foc_motor_r) && conf->foc_motor_r > 0.0f &&
           isfinite(conf->foc_motor_l) && conf->foc_motor_l > 0.0f;
}

void encoder_only_foc_restrict_config(volatile mc_configuration *conf) {
    conf->motor_type = MOTOR_TYPE_FOC;
    conf->foc_sensor_mode = FOC_SENSOR_MODE_ENCODER;
    conf->m_sensor_port_mode = SENSOR_PORT_MODE_CUSTOM_ENCODER;
    conf->foc_speed_soure = FOC_SPEED_SRC_CORRECTED;
    conf->foc_offsets_cal_mode = 1;
    conf->foc_phase_filter_enable = false;
    conf->foc_fw_current_max = 0.0f;
    conf->foc_cc_decoupling = FOC_CC_DECOUPLING_DISABLED;
}

bool encoder_only_foc_control_allowed(mc_control_mode mode) {
    return mode == CONTROL_MODE_NONE || mode == CONTROL_MODE_DUTY ||
           mode == CONTROL_MODE_CURRENT || mode == CONTROL_MODE_CURRENT_BRAKE ||
           mode == CONTROL_MODE_SPEED || mode == CONTROL_MODE_POS;
}

bool encoder_only_foc_estimate_voltage(float mod_alpha, float mod_beta,
                                      float bus_voltage, bool driven,
                                      float *v_alpha, float *v_beta) {
    if (v_alpha == NULL || v_beta == NULL) return false;
    *v_alpha = 0.0f;
    *v_beta = 0.0f;
    /* Zero is an inactive internal model state, never a measured phase voltage. */
    if (!driven || !isfinite(bus_voltage) || bus_voltage <= 0.0f ||
        !isfinite(mod_alpha) || !isfinite(mod_beta)) return false;
    const float scale = (2.0f / 3.0f) * bus_voltage;
    *v_alpha = mod_alpha * scale;
    *v_beta = mod_beta * scale;
    if (!isfinite(*v_alpha) || !isfinite(*v_beta)) {
        *v_alpha = 0.0f;
        *v_beta = 0.0f;
        return false;
    }
    return true;
}

bool encoder_only_foc_offsets_valid(const float offsets[3], float max_error) {
    if (offsets == NULL || !isfinite(max_error) || max_error < 0.0f) return false;
    for (int i = 0; i < 3; ++i) {
        if (!isfinite(offsets[i]) || fabsf(offsets[i] - 2048.0f) > max_error) return false;
    }
    return true;
}
