/*
  xdrv_snfled.ino - sonoff led support for Sonoff-Tasmota

  Copyright (C) 2017  Theo Arends

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*********************************************************************************************\
 * Sonoff B1, Led and BN-SZ01
\*********************************************************************************************/

uint8_t ledTable[] = {
    0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  4,  4,
    4,  4,  4,  5,  5,  5,  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,
    8,  8,  9,  9,  9, 10, 10, 10, 11, 11, 12, 12, 12, 13, 13, 14,
   14, 15, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 22,
   22, 23, 23, 24, 25, 25, 26, 26, 27, 28, 28, 29, 30, 30, 31, 32,
   33, 33, 34, 35, 36, 36, 37, 38, 39, 40, 40, 41, 42, 43, 44, 45,
   46, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
   61, 62, 63, 64, 65, 67, 68, 69, 70, 71, 72, 73, 75, 76, 77, 78,
   80, 81, 82, 83, 85, 86, 87, 89, 90, 91, 93, 94, 95, 97, 98, 99,
  101,102,104,105,107,108,110,111,113,114,116,117,119,121,122,124,
  125,127,129,130,132,134,135,137,139,141,142,144,146,148,150,151,
  153,155,157,159,161,163,165,166,168,170,172,174,176,178,180,182,
  184,186,189,191,193,195,197,199,201,204,206,208,210,212,215,217,
  219,221,224,226,228,231,233,235,238,240,243,245,248,250,253,255 };

uint8_t sl_dcolor[5];
uint8_t sl_tcolor[5];
uint8_t sl_lcolor[5];

uint8_t sl_power;
uint8_t sl_any;
uint8_t sl_wakeupActive = 0;
uint8_t sl_wakeupDimmer = 0;
uint16_t sl_wakeupCntr = 0;

/*********************************************************************************************\
 * Sonoff B1 based on OpenLight https://github.com/icamgo/noduino-sdk
\*********************************************************************************************/

uint8_t sl_last_command;

void sl_di_pulse(byte times)
{
  for (byte i = 0; i < times; i++) {
    digitalWrite(pin[GPIO_DI], HIGH);
    digitalWrite(pin[GPIO_DI], LOW);
  }
}

void sl_dcki_pulse(byte times)
{
  for (byte i = 0; i < times; i++) {
    digitalWrite(pin[GPIO_DCKI], HIGH);
    digitalWrite(pin[GPIO_DCKI], LOW);
  }
}

void sl_send_command(uint8_t command)
{
  uint8_t command_data;

  sl_last_command = command;

//  ets_intr_lock();
  delayMicroseconds(12);     // TStop > 12us.
  // Send 12 DI pulse, after 6 pulse's falling edge store duty data, and 12
  // pulse's rising edge convert to command mode.
  sl_di_pulse(12);
  delayMicroseconds(12);    // Delay >12us, begin send CMD data

  for (byte n = 0; n < 2; n++) {    // Send CMD data
    command_data = command;

    for (byte i = 0; i < 4; i++) {  // Send byte
      digitalWrite(pin[GPIO_DCKI], LOW);
      if (command_data & 0x80) {
        digitalWrite(pin[GPIO_DI], HIGH);
      } else {
        digitalWrite(pin[GPIO_DI], LOW);
      }

//      digitalWrite(pin[GPIO_DI], (command_data & 0x80));
      
      digitalWrite(pin[GPIO_DCKI], HIGH);
      command_data = command_data << 1;
      if (command_data & 0x80) {
        digitalWrite(pin[GPIO_DI], HIGH);
      } else {
        digitalWrite(pin[GPIO_DI], LOW);
      }
      digitalWrite(pin[GPIO_DCKI], LOW);
      digitalWrite(pin[GPIO_DI], LOW);
      command_data = command_data << 1;
    }
  }

  delayMicroseconds(12);    // TStart > 12us. Delay 12 us.
  // Send 16 DI pulse, at 14 pulse's falling edge store CMD data, and
  // at 16 pulse's falling edge convert to duty mode.
  sl_di_pulse(16);
  delayMicroseconds(12);    // TStop > 12us.
//  ets_intr_unlock();
}

void sl_send_duty(uint16_t duty_r, uint16_t duty_g, uint16_t duty_b, uint16_t duty_w, uint16_t duty_c)
{
  uint8_t bit_length = 8;
  uint16_t duty_current = 0;

  uint16_t duty[8] = { duty_r, duty_g, duty_b, 0, duty_w, duty_c, 0, 0 };  // Definition for RGBWC channels

//  ets_intr_lock();
  delayMicroseconds(12);    // TStop > 12us.

  for (byte channel = 0; channel < 8; channel++) {    // RGB0WC00 8CH
    duty_current = duty[channel];                     // RGBWC Channel
    for (byte i = 0; i < bit_length / 2; i++) {       // Send 8bit/12bit/14bit/16bit Data
      digitalWrite(pin[GPIO_DCKI], LOW);
      if (duty_current & (0x01 << (bit_length - 1))) {
        digitalWrite(pin[GPIO_DI], HIGH);
      } else {
        digitalWrite(pin[GPIO_DI], LOW);
      }
      digitalWrite(pin[GPIO_DCKI], HIGH);
      duty_current = duty_current << 1;
      if (duty_current & (0x01 << (bit_length - 1))) {
        digitalWrite(pin[GPIO_DI], HIGH);
      } else {
        digitalWrite(pin[GPIO_DI], LOW);
      }
      digitalWrite(pin[GPIO_DCKI], LOW);
      digitalWrite(pin[GPIO_DI], LOW);
      duty_current = duty_current << 1;
    }
  }

  delayMicroseconds(12);  // TStart > 12us. Ready for send DI pulse.
  sl_di_pulse(8);      // Send 8 DI pulse. After 8 pulse falling edge, store old data.
  delayMicroseconds(12);  // TStop > 12us.
//  ets_intr_unlock();
}

/********************************************************************************************/

void sl_init(void)
{
  pin[GPIO_WS2812] = 99;    // I do not allow both Sonoff Led AND WS2812 led
  if (sfl_flg < 5) {
    if (!my_module.gp.io[4]) {
      pinMode(4, OUTPUT);     // Stop floating outputs
      digitalWrite(4, LOW);
    }
    if (!my_module.gp.io[5]) {
      pinMode(5, OUTPUT);     // Stop floating outputs
      digitalWrite(5, LOW);
    }
    if (!my_module.gp.io[14]) {
      pinMode(14, OUTPUT);    // Stop floating outputs
      digitalWrite(14, LOW);
    }
    sysCfg.pwmvalue[0] = 0;    // We use dimmer / led_color
    if (2 == sfl_flg) {
      sysCfg.pwmvalue[1] = 0;  // We use led_color
    }
  } else {
    pinMode(pin[GPIO_DI], OUTPUT);
    pinMode(pin[GPIO_DCKI], OUTPUT);
    digitalWrite(pin[GPIO_DI], LOW);
    digitalWrite(pin[GPIO_DCKI], LOW);

    // Clear all duty register 
    sl_dcki_pulse(64);
    sl_send_command(0x18);  // ONE_SHOT_DISABLE, REACTION_FAST, BIT_WIDTH_8, FREQUENCY_DIVIDE_1, SCATTER_APDM

    // Test
    sl_send_duty(16, 0, 0, 0, 0);  // Red
  }
  
  sl_power = 0;
  sl_any = 0;
  sl_wakeupActive = 0;
}

void sl_setDim(uint8_t myDimmer)
{
  float temp;
  
  if ((1 == sfl_flg) && (100 == myDimmer)) {
    myDimmer = 99;  // BN-SZ01 starts flickering at dimmer = 100
  }
  float newDim = 100 / (float)myDimmer;
  for (byte i = 0; i < sfl_flg; i++) {
    temp = (float)sysCfg.led_color[i] / newDim;
    sl_dcolor[i] = (uint8_t)temp;
  }
}

void sl_setColor()
{
  uint8_t highest = 0;
  float temp;

  for (byte i = 0; i < sfl_flg; i++) {
    if (highest < sl_dcolor[i]) {
      highest = sl_dcolor[i];
    }
  }
  float mDim = (float)highest / 2.55;
  sysCfg.led_dimmer[0] = (uint8_t)mDim;
  float newDim = 100 / mDim;
  for (byte i = 0; i < sfl_flg; i++) {
    temp = (float)sl_dcolor[i] * newDim;
    sysCfg.led_color[i] = (uint8_t)temp;
  }
}

char* sl_getColor(char* scolor)
{
  sl_setDim(sysCfg.led_dimmer[0]);
  scolor[0] = '\0';
  for (byte i = 0; i < sfl_flg; i++) {
    snprintf_P(scolor, 11, PSTR("%s%02X"), scolor, sl_dcolor[i]);
  }
  return scolor;
}

void sl_prepPower(char *svalue, uint16_t ssvalue)
{
  char scolor[11];
  
//  do_cmnd_power(index, (sysCfg.led_dimmer[0]>0));
  if (sysCfg.led_dimmer[0] && !(power&1)) {
    do_cmnd_power(1, 7);  // No publishPowerState
  }
  else if (!sysCfg.led_dimmer[0] && (power&1)) {
    do_cmnd_power(1, 6);  // No publishPowerState
  }
#ifdef USE_DOMOTICZ
  mqtt_publishDomoticzPowerState(1);
#endif  // USE_DOMOTICZ
  if (sfl_flg > 1) {
    snprintf_P(svalue, ssvalue, PSTR("{\"POWER\":\"%s\", \"Dimmer\":%d, \"Color\":\"%s\"}"),
      getStateText(power &1), sysCfg.led_dimmer[0], sl_getColor(scolor));
  } else {
    snprintf_P(svalue, ssvalue, PSTR("{\"POWER\":\"%s\", \"Dimmer\":%d}"),
      getStateText(power &1), sysCfg.led_dimmer[0]);
  }
}

void sl_setPower(uint8_t power)
{
  sl_power = power &1;
  if (sl_wakeupActive) {
    sl_wakeupActive--;
  }
  sl_animate();
}

void sl_animate()
{
// {"Wakeup":"Done"}
  char svalue[32];  // was MESSZ
  uint8_t fadeValue;
  uint8_t cur_col[5];
  
  if (0 == sl_power) {  // Power Off
    for (byte i = 0; i < sfl_flg; i++) {
      sl_tcolor[i] = 0;
    }
  }
  else {
    if (!sl_wakeupActive) {  // Power On
      sl_setDim(sysCfg.led_dimmer[0]);
      if (0 == sysCfg.led_fade) {
        for (byte i = 0; i < sfl_flg; i++) {
          sl_tcolor[i] = sl_dcolor[i];
        }
      } else {
        for (byte i = 0; i < sfl_flg; i++) {
          if (sl_tcolor[i] != sl_dcolor[i]) {
            if (sl_tcolor[i] < sl_dcolor[i]) {
              sl_tcolor[i] += ((sl_dcolor[i] - sl_tcolor[i]) >> sysCfg.led_speed) +1;
            }
            if (sl_tcolor[i] > sl_dcolor[i]) {
              sl_tcolor[i] -= ((sl_tcolor[i] - sl_dcolor[i]) >> sysCfg.led_speed) +1;
            }
          }
        }
      }
    } else {   // Power On using wake up duration
      if (2 == sl_wakeupActive) {
        sl_wakeupActive = 1;
        for (byte i = 0; i < sfl_flg; i++) {
          sl_tcolor[i] = 0;
        }
        sl_wakeupCntr = 0;
        sl_wakeupDimmer = 0;
      }
      sl_wakeupCntr++;
      if (sl_wakeupCntr > ((sysCfg.led_wakeup * STATES) / sysCfg.led_dimmer[0])) {
        sl_wakeupCntr = 0;
        sl_wakeupDimmer++;
        if (sl_wakeupDimmer <= sysCfg.led_dimmer[0]) {
          sl_setDim(sl_wakeupDimmer);
          for (byte i = 0; i < sfl_flg; i++) {
            sl_tcolor[i] = sl_dcolor[i];
          }
        } else {
          snprintf_P(svalue, sizeof(svalue), PSTR("{\"Wakeup\":\"Done\"}"));
          mqtt_publish_topic_P(2, PSTR("WAKEUP"), svalue);
          sl_wakeupActive = 0;
        }
      }
    }
  }
  for (byte i = 0; i < sfl_flg; i++) {
    if (sl_lcolor[i] != sl_tcolor[i]) {
      sl_any = 1;
    }
  }
  if (sl_any) {
    sl_any = 0;
    for (byte i = 0; i < sfl_flg; i++) {
      sl_lcolor[i] = sl_tcolor[i];
      cur_col[i] = (sysCfg.led_table) ? ledTable[sl_lcolor[i]] : sl_lcolor[i];
      if (sfl_flg < 5) {
        if (pin[GPIO_PWM1 +i] < 99) {
          analogWrite(pin[GPIO_PWM1 +i], cur_col[i] * (PWM_RANGE / 255));
        }
      }
    }
    if (5 == sfl_flg) {
      sl_send_duty(cur_col[0], cur_col[1], cur_col[2], cur_col[3], cur_col[4]);
    }
  }
}

/*********************************************************************************************\
 * Hue support
\*********************************************************************************************/

void sl_rgb2hsb(float *hue, float *sat, float *bri)
{
  sl_setDim(sysCfg.led_dimmer[0]);
  
  float r = (float)(sl_dcolor[0] / 255.0f);
  float g = (float)(sl_dcolor[1] / 255.0f);
  float b = (float)(sl_dcolor[2] / 255.0f);

  float max = fmax(fmax(r, g), b);
  float min = fmin(fmin(r, g), b);

  *bri = (max + min) / 2.0f;

  if (max == min) {
    *hue = *sat = 0.0f;
  } else {
    float d = max - min;
    *sat = (*bri > 0.5f) ? d / (2.0f - max - min) : d / (max + min);

    if (r > g && r > b) {
      *hue = (g - b) / d + (g < b ? 6.0f : 0.0f);
    }
    else if (g > b) {
      *hue = (b - r) / d + 2.0f;
    }
    else {
      *hue = (r - g) / d + 4.0f;
    }
    *hue /= 6.0f;
  }
}

float sl_hue2rgb(float p, float q, float t)
{
  if (t < 0.0f) {
    t += 1.0f;
  }
  if (t > 1.0f) {
    t -= 1.0f;
  }
  if (t < 1.0f / 6.0f) {
    return p + (q - p) * 6.0f * t;
  }
  if (t < 1.0f / 2.0f) {
    return q;
  }
  if (t < 2.0f / 3.0f) {
    return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
  }
  return p;
}

void sl_hsb2rgb(float hue, float sat, float bri)
{
  float r;
  float g;
  float b;

  if (sat == 0.0f) {
    r = g = b = bri;
  } else {
    float q = bri < 0.5f ? bri * (1.0f + sat) : bri + sat - bri * sat;
    float p = 2.0f * bri - q;
    r = sl_hue2rgb(p, q, hue + 1.0f / 3.0f);
    g = sl_hue2rgb(p, q, hue);
    b = sl_hue2rgb(p, q, hue - 1.0f / 3.0f);
  }
  sl_dcolor[0] = (uint8_t)(r * 255 + 0.5f);
  sl_dcolor[1] = (uint8_t)(g * 255 + 0.5f);
  sl_dcolor[2] = (uint8_t)(b * 255 + 0.5f);
  sl_setColor();
}

/********************************************************************************************/

void sl_replaceHSB(String *response)
{
  float hue;
  float sat;
  float bri;
  
  if (5 == sfl_flg) {
    sl_rgb2hsb(&hue, &sat, &bri);
    response->replace("{h}", String((uint16_t)(65535.0f * hue)));
    response->replace("{s}", String((uint8_t)(254.0f * sat)));
    response->replace("{b}", String((uint8_t)(254.0f * bri)));
  } else {
    response->replace("{h}", "0");
    response->replace("{s}", "0");
    response->replace("{b}", String((uint8_t)(2.54f * (float)sysCfg.led_dimmer[0])));
  }
}

void sl_getHSB(float *hue, float *sat, float *bri)
{
  if (5 == sfl_flg) {
    sl_rgb2hsb(hue, sat, bri);
  } else {
    *hue = 0;
    *sat = 0;
    *bri = (2.54f * (float)sysCfg.led_dimmer[0]);
  }
}

void sl_setHSB(float hue, float sat, float bri)
{
  char svalue[MESSZ];

/*
  char log[LOGSZ];
  char stemp1[10];
  char stemp2[10];
  char stemp3[10];
  dtostrf(hue, 1, 3, stemp1);
  dtostrf(sat, 1, 3, stemp2);
  dtostrf(bri, 1, 3, stemp3);
  snprintf_P(log, sizeof(log), PSTR("LED: Hue %s, Sat %s, Bri %s"), stemp1, stemp2, stemp3);
  addLog(LOG_LEVEL_DEBUG, log);
*/
  if (5 == sfl_flg) {
    sl_hsb2rgb(hue, sat, bri);
    sl_prepPower(svalue, sizeof(svalue));
    mqtt_publish_topic_P(5, "COLOR", svalue);
  } else {
    uint8_t tmp = (uint8_t)(bri * 100);
    sysCfg.led_dimmer[0] = tmp;
    sl_prepPower(svalue, sizeof(svalue));
    mqtt_publish_topic_P(5, "DIMMER", svalue);
  }
}

/*********************************************************************************************\
 * Commands
\*********************************************************************************************/

boolean sl_command(char *type, uint16_t index, char *dataBufUc, uint16_t data_len, int16_t payload, char *svalue, uint16_t ssvalue)
{
  boolean serviced = true;
  boolean coldim = false;
  char scolor[11];
  char *p;

  if ((sfl_flg > 1) && !strcmp_P(type,PSTR("COLOR"))) {
    if ((2 * sfl_flg) == data_len) {
      for (byte i = 0; i < sfl_flg; i++) {
        strlcpy(scolor, dataBufUc + (i *2), 3);
        sl_dcolor[i] = (uint8_t)strtol(scolor, &p, 16);
      }
      sl_setColor();
      coldim = true;
    } else {
      snprintf_P(svalue, ssvalue, PSTR("{\"Color\":\"%s\"}"), sl_getColor(scolor));
    }
  }
  else if (!strcmp_P(type,PSTR("DIMMER"))) {
    if ((payload >= 0) && (payload <= 100)) {
      sysCfg.led_dimmer[0] = payload;
      coldim = true;
    } else {
      snprintf_P(svalue, ssvalue, PSTR("{\"Dimmer\":%d}"), sysCfg.led_dimmer[0]);
    }
  }
  else if (!strcmp_P(type,PSTR("LEDTABLE"))) {
    if ((payload >= 0) && (payload <= 2)) {
      switch (payload) {
      case 0: // Off
      case 1: // On
        sysCfg.led_table = payload;
        break;
      case 2: // Toggle
        sysCfg.led_table ^= 1;
        break;
      }
      sl_any = 1;
    }
    snprintf_P(svalue, ssvalue, PSTR("{\"LedTable\":\"%s\"}"), getStateText(sysCfg.led_table));
  }
  else if (!strcmp_P(type,PSTR("FADE"))) {
    switch (payload) {
    case 0: // Off
    case 1: // On
      sysCfg.led_fade = payload;
      break;
    case 2: // Toggle
      sysCfg.led_fade ^= 1;
      break;
    }
    snprintf_P(svalue, ssvalue, PSTR("{\"Fade\":\"%s\"}"), getStateText(sysCfg.led_fade));
  }
  else if (!strcmp_P(type,PSTR("SPEED"))) {  // 1 - fast, 8 - slow
    if ((payload > 0) && (payload <= 8)) {
      sysCfg.led_speed = payload;
    }
    snprintf_P(svalue, ssvalue, PSTR("{\"Speed\":%d}"), sysCfg.led_speed);
  }
  else if (!strcmp_P(type,PSTR("WAKEUPDURATION"))) {
    if ((payload > 0) && (payload < 3001)) {
      sysCfg.led_wakeup = payload;
      sl_wakeupActive = 0;
    }
    snprintf_P(svalue, ssvalue, PSTR("{\"WakeUpDuration\":%d}"), sysCfg.led_wakeup);
  }
  else if (!strcmp_P(type,PSTR("WAKEUP"))) {
    sl_wakeupActive = 3;
    do_cmnd_power(1, 1);
    snprintf_P(svalue, ssvalue, PSTR("{\"Wakeup\":\"Started\"}"));
  }
  else {
    serviced = false;  // Unknown command
  }
  if (coldim) {
    sl_prepPower(svalue, ssvalue);
  }
  return serviced;
}

