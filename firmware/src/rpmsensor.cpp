#include "RPMSensor.h"
#include <driver/pcnt.h>

#define RPM_PIN 25
#define PCNT_UNIT PCNT_UNIT_0
#define PCNT_CHANNEL PCNT_CHANNEL_0

RPMSensor::RPMSensor()
    : latestRPM(0), lastPoll(0) {}

void RPMSensor::begin()
{
    setupPCNT();
}

void RPMSensor::poll()
{
    unsigned long now = millis();
    unsigned long dt = now - lastPoll;
    if (dt < 500)
        return;
    int16_t count;
    pcnt_get_counter_value(PCNT_UNIT, &count);
    pcnt_counter_clear(PCNT_UNIT);
    latestRPM = count * 1000.0f / dt;
    lastPoll = now;
}

void RPMSensor::setupPCNT()
{
    pcnt_config_t cfg = {};
    cfg.pulse_gpio_num = RPM_PIN;
    cfg.ctrl_gpio_num = PCNT_PIN_NOT_USED;
    cfg.unit = PCNT_UNIT;
    cfg.channel = PCNT_CHANNEL;
    cfg.pos_mode = PCNT_COUNT_INC;
    cfg.neg_mode = PCNT_COUNT_DIS;
    cfg.lctrl_mode = PCNT_MODE_KEEP;
    cfg.hctrl_mode = PCNT_MODE_KEEP;
    cfg.counter_h_lim = 32767;
    cfg.counter_l_lim = 0;
    pcnt_unit_config(&cfg);
    pcnt_set_filter_value(PCNT_UNIT, 100);
    pcnt_filter_enable(PCNT_UNIT);
    pcnt_counter_pause(PCNT_UNIT);
    pcnt_counter_clear(PCNT_UNIT);
    pcnt_counter_resume(PCNT_UNIT);
}

float RPMSensor::getHz()
{
    return latestRPM;
}
