#include "ledcalendar.h"

/**
 * @brief Turn on the calendar with current date
 */
void turnOnCalendar()
{
    uint8_t dayOfWeek = ntp.getDayOfWeek();
    uint8_t dayOfMonth = ntp.getDayOfMonth();
    uint8_t month = ntp.getMonthNumber();
    
    dateLED.flushCalendar();
    dateLED.setDayOfWeek(dayOfWeek, calendar_color_dow);
    dateLED.setDayOfMonth(dayOfMonth, calendar_color_dom);
    dateLED.setMonth(month, calendar_color_month);
    dateLED.drawOnCalendarInstant();
}

/**
 * @brief Turn off the calendar
 */
void turnOffCalendar()
{
    dateLED.flushCalendar();
    dateLED.drawOnCalendarInstant();
}

/**
 * @brief Update the calendar display
 */
void updateCalendar()
{
    if(nightMode || !calendarActive)
    {
        turnOffCalendar();
        return;
    }

    turnOnCalendar();
}

/**
 * @brief Load calendar settings from EEPROM
 */
void loadCalendarSettingsFromEEPROM()
{
    uint8_t cal_dow_r = EEPROM.read(ADR_CAL_DOW_RED);
    uint8_t cal_dow_g = EEPROM.read(ADR_CAL_DOW_GREEN);
    uint8_t cal_dow_b = EEPROM.read(ADR_CAL_DOW_BLUE);
    calendar_color_dow = LEDCalendar::Color24bit(cal_dow_r, cal_dow_g, cal_dow_b);
    
    uint8_t cal_dom_r = EEPROM.read(ADR_CAL_DOM_RED);
    uint8_t cal_dom_g = EEPROM.read(ADR_CAL_DOM_GREEN);
    uint8_t cal_dom_b = EEPROM.read(ADR_CAL_DOM_BLUE);
    calendar_color_dom = LEDCalendar::Color24bit(cal_dom_r, cal_dom_g, cal_dom_b);
    
    uint8_t cal_month_r = EEPROM.read(ADR_CAL_MONTH_RED);
    uint8_t cal_month_g = EEPROM.read(ADR_CAL_MONTH_GREEN);
    uint8_t cal_month_b = EEPROM.read(ADR_CAL_MONTH_BLUE);
    calendar_color_month = LEDCalendar::Color24bit(cal_month_r, cal_month_g, cal_month_b);
    
    calendarActive = EEPROM.read(ADR_CAL_ACTIVE);
    
    logger.logString("Calendar Settings loaded from EEPROM");
    logger.logString("Calendar Active: " + String(calendarActive));
    logger.logString("Day of Week Color: " + String(cal_dow_r) + "-" + String(cal_dow_g) + "-" + String(cal_dow_b));
    logger.logString("Day of Month Color: " + String(cal_dom_r) + "-" + String(cal_dom_g) + "-" + String(cal_dom_b));
    logger.logString("Month Color: " + String(cal_month_r) + "-" + String(cal_month_g) + "-" + String(cal_month_b));
}
