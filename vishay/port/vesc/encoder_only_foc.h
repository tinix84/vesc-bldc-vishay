/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ENCODER_ONLY_FOC_H
#define ENCODER_ONLY_FOC_H

#include "datatypes.h"

bool encoder_only_foc_config_valid(const volatile mc_configuration *conf);
void encoder_only_foc_restrict_config(volatile mc_configuration *conf);
bool encoder_only_foc_control_allowed(mc_control_mode mode);
bool encoder_only_foc_estimate_voltage(float mod_alpha, float mod_beta,
                                      float bus_voltage, bool driven,
                                      float *v_alpha, float *v_beta);
bool encoder_only_foc_offsets_valid(const float offsets[3], float max_error);

#endif
