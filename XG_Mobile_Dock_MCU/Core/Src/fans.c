#include "main.h"
#include <stdio.h>
#include "stm32f0xx_ll_adc.h"

// built from https://aterlux.ru/article/ntcresistor-en

// Value when sum of ADC values is more than first value in table
#define TEMPERATURE_UNDER 0
// Value when sum of ADC values is less than last value in table 
#define TEMPERATURE_OVER 800
// Value corresponds to first entry in table
#define TEMPERATURE_TABLE_START 0
// Table step
#define TEMPERATURE_TABLE_STEP 10

// Type of each table item. If sum fits into 16 bits - uint16_t, else - uint32_t
typedef uint16_t temperature_table_entry_type;
// Type of table index. If table has more than 255 items, then uint16_t, else - uint8_t
typedef uint8_t temperature_table_index_type;
// Access method to table entry. Should correspond to temperature_table_entry_type
#define TEMPERATURE_TABLE_READ(i) termo_table[i]

/* Table of ADC sum value, corresponding to temperature. Starting from higher value to lower.
   Next parameters had been used to build table:
     R1(T1): 100kOhm(25°С)
     B25/50: 4250
     Scheme: A
     Ra: 100kOhm
     U0/Uref: 3.3V/3.3V
*/
static const temperature_table_entry_type termo_table[] = {
    3222, 3182, 3142, 3100, 3058, 3014, 2970, 2925,
    2880, 2833, 2787, 2739, 2691, 2643, 2594, 2544,
    2495, 2445, 2396, 2346, 2296, 2246, 2196, 2147,
    2097, 2048, 1999, 1951, 1903, 1855, 1808, 1762,
    1716, 1671, 1626, 1583, 1539, 1497, 1456, 1415,
    1375, 1336, 1297, 1260, 1223, 1187, 1152, 1118,
    1085, 1052, 1021, 990, 960, 931, 903, 875,
    848, 822, 797, 772, 749, 726, 703, 682,
    661, 640, 620, 601, 583, 565, 547, 530,
    514, 498, 483, 468, 454, 440, 427, 414,
    401
};

// This function is calculating temperature in tenth of degree of Celsius
// depending on ADC sum value as input parameter.
static int16_t calc_temperature(temperature_table_entry_type adcsum) {
    temperature_table_index_type l = 0;
    temperature_table_index_type r = (sizeof(termo_table) / sizeof(termo_table[0])) - 1;
    temperature_table_entry_type thigh = TEMPERATURE_TABLE_READ(r);
    
    // Checking for bound values
    if (adcsum <= thigh) {
        #ifdef TEMPERATURE_UNDER
            if (adcsum < thigh) 
                return TEMPERATURE_UNDER;
        #endif
        return TEMPERATURE_TABLE_STEP * r + TEMPERATURE_TABLE_START;
    }
    temperature_table_entry_type tlow = TEMPERATURE_TABLE_READ(0);
    if (adcsum >= tlow) {
        #ifdef TEMPERATURE_OVER
            if (adcsum > tlow)
                return TEMPERATURE_OVER;
        #endif
        return TEMPERATURE_TABLE_START;
    }

    // Table lookup using binary search 
    while ((r - l) > 1) {
        temperature_table_index_type m = (l + r) >> 1;
        temperature_table_entry_type mid = TEMPERATURE_TABLE_READ(m);
        if (adcsum > mid) {
            r = m;
        } else {
            l = m;
        }
    }
    temperature_table_entry_type vl = TEMPERATURE_TABLE_READ(l);
    if (adcsum >= vl) {
        return l * TEMPERATURE_TABLE_STEP + TEMPERATURE_TABLE_START;
    }
    temperature_table_entry_type vr = TEMPERATURE_TABLE_READ(r);
    temperature_table_entry_type vd = vl - vr;
    int16_t res = TEMPERATURE_TABLE_START + r * TEMPERATURE_TABLE_STEP; 
    if (vd) {
        // Linear interpolation
        res -= ((TEMPERATURE_TABLE_STEP * (int32_t)(adcsum - vr) + (vd >> 1)) / vd);
    }
    return res;
}

// PWM generation

static const int fan_curve[][2] = {
    {0, 0},
    {200, 20},
    {800, 80},
    {850, 100},
};

static uint16_t adc_res[2];

#define NUM_SAMPLES (5)
static uint16_t adc_samples[NUM_SAMPLES];
static int adc_sample_index = 0;
static volatile int needs_temp_processing = 0, needs_temp_update = 0;

extern ADC_HandleTypeDef hadc;
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim6;

static int fans_get_speed(int temp) {
    int num_rows = sizeof(fan_curve) / sizeof(fan_curve[0]);
    int i;

    for (i = 0; i < num_rows; i++) {
        if (temp < fan_curve[i][0]) {
            break;
        }
    }

    // return speed at both ends of the table
    if (i == num_rows || i == 1) {
        return fan_curve[i-1][1];
    }

    // linearly extrapolate from the table
    int t1 = fan_curve[i-1][0], s1 = fan_curve[i-1][1];
    int t2 = fan_curve[i][0], s2 = fan_curve[i][1];
    return s1 + (s2 - s1) * (temp - t1) / (t2 - t1);
}

static void fans_configure(uint32_t duty_cycle) {
    TIM_OC_InitTypeDef sConfigOC = {0};

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = duty_cycle;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1);
}

static void fans_reconfigure(int off) {
    static int is_pwm_running = 0;
    static int last_speed = 0;
    uint16_t adc_value;
    uint32_t adc_average;
    int16_t temp;
    int fan_speed;

    // adjust for any difference in VREF
    adc_value = adc_res[0] + adc_res[1] - *VREFINT_CAL_ADDR;

    // create rolling average
    do {
        adc_samples[adc_sample_index] = adc_value;
        adc_sample_index = (adc_sample_index + 1) % NUM_SAMPLES;
    } while (adc_samples[adc_sample_index] == 0);
    adc_average = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        adc_average += adc_samples[i];
    }
    adc_average /= NUM_SAMPLES;
    temp = calc_temperature(adc_average);
    fan_speed = fans_get_speed(temp);

    if (last_speed != fan_speed) {
        last_speed = fan_speed;
        fans_configure(fan_speed);
        printf("Fans: ADC = %d, Avg = %ld, T = %d, S = %d\n", adc_value, adc_average, temp, fan_speed);
    }

    if (fan_speed == 0 || fan_speed == 100 || off) {
        if (is_pwm_running) {
            HAL_TIM_PWM_Stop_IT(&htim1, TIM_CHANNEL_1);
            HAL_TIM_Base_Stop_IT(&htim1);
            is_pwm_running = 0;
        }
    }

    if (fan_speed == 0 || off) {
        HAL_GPIO_WritePin(FAN_PWM_GPIO_Port, FAN_PWM_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(FAN_ON_GPIO_Port, FAN_ON_Pin, GPIO_PIN_RESET);
    } else if (fan_speed == 100) {
        HAL_GPIO_WritePin(FAN_PWM_GPIO_Port, FAN_PWM_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(FAN_ON_GPIO_Port, FAN_ON_Pin, GPIO_PIN_SET);
    } else {
        if (!is_pwm_running) {
            HAL_TIM_Base_Start_IT(&htim1);
            HAL_TIM_PWM_Start_IT(&htim1, TIM_CHANNEL_1);
            HAL_GPIO_WritePin(FAN_ON_GPIO_Port, FAN_ON_Pin, GPIO_PIN_SET);
            is_pwm_running = 1;
        }
    }
}

void fans_start(void) {
    HAL_ADCEx_Calibration_Start(&hadc);
    HAL_TIM_Base_Start_IT(&htim6);
}

void fans_stop(void) {
    HAL_TIM_Base_Stop_IT(&htim6);
    fans_reconfigure(1);
}

void fans_update(void) {
    if (needs_temp_update) {
        HAL_ADC_Start_DMA(&hadc, (uint32_t *)adc_res, 2);
        needs_temp_update = 0;
    }
    if (needs_temp_processing) {
        HAL_ADC_Stop_DMA(&hadc);
        fans_reconfigure(0);
        needs_temp_processing = 0;
    }
}

extern void HAL_TIM3_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim == &htim1) {
        HAL_GPIO_WritePin(FAN_PWM_GPIO_Port, FAN_PWM_Pin, GPIO_PIN_SET);
    } else if (htim == &htim6) {
        needs_temp_update = 1;
    } else {
        HAL_TIM3_PeriodElapsedCallback(htim);
    }
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
    HAL_GPIO_WritePin(FAN_PWM_GPIO_Port, FAN_PWM_Pin, GPIO_PIN_RESET);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    needs_temp_processing = 1;
}
