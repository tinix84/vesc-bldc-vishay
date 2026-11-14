/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "encoder_only_foc.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static mc_configuration valid_config(void) {
    mc_configuration conf;
    memset(&conf, 0, sizeof(conf));
    conf.foc_encoder_ratio = 7.0f;
    conf.foc_encoder_offset = 12.0f;
    conf.foc_motor_r = 0.05f;
    conf.foc_motor_l = 0.00002f;
    encoder_only_foc_restrict_config(&conf);
    return conf;
}

static void test_config(void) {
    mc_configuration conf = valid_config();
    assert(encoder_only_foc_config_valid(&conf));
    conf.motor_type = MOTOR_TYPE_BLDC;
    assert(!encoder_only_foc_config_valid(&conf));
    conf.motor_type = MOTOR_TYPE_DC;
    assert(!encoder_only_foc_config_valid(&conf));
    conf = valid_config(); conf.foc_sensor_mode = FOC_SENSOR_MODE_SENSORLESS;
    assert(!encoder_only_foc_config_valid(&conf));
    conf.foc_sensor_mode = FOC_SENSOR_MODE_HFI;
    assert(!encoder_only_foc_config_valid(&conf));
    conf.foc_sensor_mode = FOC_SENSOR_MODE_ENCODER_AB;
    assert(!encoder_only_foc_config_valid(&conf));
    conf = valid_config(); conf.m_sensor_port_mode = SENSOR_PORT_MODE_HALL;
    assert(!encoder_only_foc_config_valid(&conf));
    conf = valid_config(); conf.foc_speed_soure = FOC_SPEED_SRC_OBSERVER;
    assert(!encoder_only_foc_config_valid(&conf));
    conf = valid_config(); conf.foc_offsets_cal_mode = 7;
    assert(!encoder_only_foc_config_valid(&conf));
    conf = valid_config(); conf.foc_phase_filter_enable = true;
    assert(!encoder_only_foc_config_valid(&conf));
    conf = valid_config(); conf.foc_fw_current_max = 1.0f;
    assert(!encoder_only_foc_config_valid(&conf));
    conf = valid_config(); conf.foc_cc_decoupling = FOC_CC_DECOUPLING_BEMF;
    assert(!encoder_only_foc_config_valid(&conf));
    conf = valid_config(); conf.foc_encoder_ratio = NAN;
    assert(!encoder_only_foc_config_valid(&conf));
    conf = valid_config(); conf.foc_encoder_offset = NAN;
    assert(!encoder_only_foc_config_valid(&conf));
    conf = valid_config(); conf.foc_motor_r = 0.0f;
    assert(!encoder_only_foc_config_valid(&conf));
}

int main(void) {
    test_config();
    assert(encoder_only_foc_control_allowed(CONTROL_MODE_CURRENT));
    assert(encoder_only_foc_control_allowed(CONTROL_MODE_SPEED));
    assert(encoder_only_foc_control_allowed(CONTROL_MODE_POS));
    assert(!encoder_only_foc_control_allowed(CONTROL_MODE_OPENLOOP));
    assert(!encoder_only_foc_control_allowed(CONTROL_MODE_OPENLOOP_PHASE));
    assert(!encoder_only_foc_control_allowed(CONTROL_MODE_HANDBRAKE));
    float a, b;
    assert(encoder_only_foc_estimate_voltage(0.3f, -0.6f, 48.0f, true, &a, &b));
    assert(fabsf(a - 9.6f) < 0.0001f && fabsf(b + 19.2f) < 0.0001f);
    assert(!encoder_only_foc_estimate_voltage(0.3f, 0.6f, 48.0f, false, &a, &b));
    assert(a == 0.0f && b == 0.0f);
    assert(!encoder_only_foc_estimate_voltage(0.3f, 0.6f, 0.0f, true, &a, &b));
    assert(!encoder_only_foc_estimate_voltage(NAN, 0.6f, 48.0f, true, &a, &b));
    assert(!encoder_only_foc_estimate_voltage(FLT_MAX, 1.0f, FLT_MAX, true, &a, &b));
    const float good[] = {2050.0f, 2040.0f, 2049.0f};
    const float clipped[] = {0.0f, 2040.0f, 2049.0f};
    const float invalid[] = {NAN, 2040.0f, 2049.0f};
    assert(encoder_only_foc_offsets_valid(good, 620.0f));
    assert(!encoder_only_foc_offsets_valid(clipped, 620.0f));
    assert(!encoder_only_foc_offsets_valid(invalid, 620.0f));
    puts("Encoder-only FOC configuration, mode restrictions, voltage model and current-offset tests passed");
    return 0;
}
