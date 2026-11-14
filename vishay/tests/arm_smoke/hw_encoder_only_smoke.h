/* Compile-only reference target, never a Vishay board configuration. */
#ifndef HW_ENCODER_ONLY_SMOKE_H
#define HW_ENCODER_ONLY_SMOKE_H
#include "../../../hwconf/trampa/vesc6/hw_60_mk3.h"
#define HW_ENCODER_ONLY_FOC
#undef HW_NAME
#define HW_NAME "COMPILE_TEST_NO_FLASH"
#undef ENABLE_GATE
#define ENABLE_GATE() DISABLE_GATE()
#undef HW_HAS_PHASE_FILTERS
/* Poison every phase measurement. Any unguarded dependency must fail compilation. */
#undef ADC_V_L1
#undef ADC_V_L2
#undef ADC_V_L3
#undef ADC_IND_SENS1
#undef ADC_IND_SENS2
#undef ADC_IND_SENS3
#define ADC_V_L1 PHASE_VOLTAGE_INPUT_DOES_NOT_EXIST
#define ADC_V_L2 PHASE_VOLTAGE_INPUT_DOES_NOT_EXIST
#define ADC_V_L3 PHASE_VOLTAGE_INPUT_DOES_NOT_EXIST
#define ADC_IND_SENS1 PHASE_VOLTAGE_INPUT_DOES_NOT_EXIST
#define ADC_IND_SENS2 PHASE_VOLTAGE_INPUT_DOES_NOT_EXIST
#define ADC_IND_SENS3 PHASE_VOLTAGE_INPUT_DOES_NOT_EXIST
#endif
