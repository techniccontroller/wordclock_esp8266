#include "ntp_client_plus.h"

/**
 * @brief Construct a new NTPClientPlus::NTPClientPlus object
 * 
 * @param udp   UDP client
 * @param poolServerName    time server name
 * @param utcx  UTC offset (in 1h)
 * @param _swChange should summer/winter time be considered
 */
NTPClientPlus::NTPClientPlus(UDP &udp, const char *poolServerName, int utcx, bool _swChange)
{
    this->_udp = &udp;
    this->_utcx = utcx;
    this->_timeOffset = this->secondperhour * this->_utcx;
    this->_poolServerName = poolServerName;
    this->_swChange = _swChange;
}

/**
 * @brief Starts the underlying UDP client, get first NTP timestamp and calc date
 * 
 */
void NTPClientPlus::setupNTPClient()
{
    this->_udp->begin(this->_port);
    this->_udpSetup = true;
    this->updateNTP();
    this->calcDate();
}

/**
 * @brief Get new update from NTP
 * 
 * @return 0     after successful update
 * @return -1    timeout after 1000 ms
 * @return 1     too much difference to previous received time (try again)
 */
int NTPClientPlus::updateNTP()
{

    // flush any existing packets
    while (this->_udp->parsePacket() != 0)
        this->_udp->flush();

    this->sendNTPPacket();

    // Wait till data is there or timeout...
    byte timeout = 0;
    int cb = 0;
    do
    {
        delay(10);
        cb = this->_udp->parsePacket();
        if (timeout > 100)
            return -1; // timeout after 1000 ms
        timeout++;
    } while (cb == 0);

    this->_udp->read(this->_packetBuffer, NTP_PACKET_SIZE);

    unsigned long highWord = word(this->_packetBuffer[40], this->_packetBuffer[41]);
    unsigned long lowWord = word(this->_packetBuffer[42], this->_packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long tempSecsSince1900 = highWord << 16 | lowWord;

    if(tempSecsSince1900 < SEVENZYYEARS){
        // NTP time is not valid
        return 2;
    }

    // check if time off last ntp update is roughly in the same range: 100sec apart (validation check)
    if(this->_lastSecsSince1900 == 0 || tempSecsSince1900 - this->_lastSecsSince1900 < 100000){
        // Only update time then
        this->_lastUpdate = millis() - (10 * (timeout + 1)); // Account for delay in reading the time

        this->_secsSince1900 = tempSecsSince1900;

        this->_currentEpoc = this->_secsSince1900 - SEVENZYYEARS;

        // Remember time of last update
        this->_lastSecsSince1900 = tempSecsSince1900;

        return 0; // return 0 after successful update
    }
    else{
        // Remember time of last update
        this->_lastSecsSince1900 = tempSecsSince1900;
        
        return 1;
    }
}

/**
 * @brief Stops the underlying UDP client
 * 
 */
void NTPClientPlus::end()
{
    this->_udp->stop();

    this->_udpSetup = false;
}

/**
 * @brief Setter TimeOffset
 * 
 * @param timeOffset offset from UTC in seconds
 */
void NTPClientPlus::setTimeOffset(int timeOffset)
{
    this->_timeOffset = timeOffset;
}

long NTPClientPlus::getTimeOffset()
{
    return this->_timeOffset;
}

/**
 * @brief Set time server name
 * 
 * @param poolServerName 
 */
void NTPClientPlus::setPoolServerName(const char *poolServerName)
{
    this->_poolServerName = poolServerName;
}

/**
 * @brief Calc seconds since 1. Jan. 1900
 * 
 * @return unsigned long seconds since 1. Jan. 1900
 */
unsigned long NTPClientPlus::getSecsSince1900() const
{
    return this->_timeOffset +                      // User offset
           this->_secsSince1900 +                   // seconds returned by the NTP server
           ((millis() - this->_lastUpdate) / 1000); // Time since last update
}

/**
 * @brief Get UNIX Epoch time since 1. Jan. 1970
 * 
 * @return unsigned long UNIX Epoch time since 1. Jan. 1970 in seconds
 */
unsigned long NTPClientPlus::getEpochTime() const
{
    return this->getSecsSince1900() - SEVENZYYEARS;
}

/**
 * @brief Get current hours in 24h format
 * 
 * @return int 
 */
int NTPClientPlus::getHours24() const
{
    int hours = ((this->getEpochTime() % 86400L) / 3600);
    return hours;
}

/**
 * @brief Get current hours in 12h format
 * 
 * @return int 
 */
int NTPClientPlus::getHours12() const
{
    int hours = this->getHours24();
    if (hours >= 12)
    {
        hours = hours - 12;
    }
    return hours;
}

/**
 * @brief Get current minutes
 * 
 * @return int 
 */
int NTPClientPlus::getMinutes() const
{
    return ((this->getEpochTime() % 3600) / 60);
}

/**
 * @brief Get current seconds
 * 
 * @return int 
 */
int NTPClientPlus::getSeconds() const
{
    return (this->getEpochTime() % 60);
}

/**
 * @brief 
 * 
 * @return String time formatted like `hh:mm:ss`
 */
String NTPClientPlus::getFormattedTime() const {
  unsigned long rawTime = this->getEpochTime();
  unsigned long hours = (rawTime % 86400L) / 3600;
  String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

  unsigned long minutes = (rawTime % 3600) / 60;
  String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

  unsigned long seconds = rawTime % 60;
  String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

  return hoursStr + ":" + minuteStr + ":" + secondStr;
}

/**
 * @brief 
 * 
 * @return String date formatted like `dd.mm.yyyy`
 */
String NTPClientPlus::getFormattedDate() {
    this->calcDate();
    unsigned int dateDay = this->_dateDay; 
    unsigned int dateMonth = this->_dateMonth;
    unsigned int dateYear = this->_dateYear;

    String dayStr = dateDay < 10 ? "0" + String(dateDay) : String(dateDay);
    String monthStr = dateMonth < 10 ? "0" + String(dateMonth) : String(dateMonth);
    String yearStr = dateYear < 10 ? "0" + String(dateYear) : String(dateYear);

    return dayStr + "." + monthStr + "." + yearStr;
}


/**
 * @brief Calc date from seconds since 1900
 * 
 */
void NTPClientPlus::calcDate()
{
    // Start: Calc date

    // get days since 1900
    unsigned long days1900 = this->getSecsSince1900() / secondperday;

    // calc current year
    this->_dateYear = this->getYear();

    // calc how many leap days since 1.Jan 1900
    int leapDays = 0;
    for (unsigned int i = 1900; i < this->_dateYear; i++)
    {
        // check if leap year
        if (this->isLeapYear(i))
        {
            leapDays++;
        }
    }
    leapDays = leapDays - 1;

    // check if current year is leap year
    if (this->isLeapYear(this->_dateYear))
    {
        daysInMonth[2] = 29;
    }
    else
    {
        daysInMonth[2] = 28;
    }

    unsigned int dayOfYear = (days1900 - ((this->_dateYear - 1900) * 365) - leapDays);

    // calc current month
    this->_dateMonth = this->getMonth(dayOfYear);

    this->_dateDay = 0;

    // calc day of month
    for (unsigned int i = 0; i < this->_dateMonth; i++)
    {
        this->_dateDay = this->_dateDay + daysInMonth[i];
    }
    this->_dateDay = dayOfYear - this->_dateDay;

    // calc day of week:
    // Monday = 1, Tuesday = 2, Wednesday = 3, Thursday = 4, Friday = 5, Saturday = 6, Sunday = 7
    // 1. Januar 1900 was a monday
    this->_dayOfWeek = 1;

    for (unsigned long i = 0; i < days1900; i++)
    {

        if (this->_dayOfWeek < 7)
        {
            this->_dayOfWeek = this->_dayOfWeek + 1;
        }
        else
        {
            this->_dayOfWeek = 1;
        }
    }

    // End: Calc date (dateDay, dateMonth, dateYear)

    // calc if summer time active

    this->updateSWChange();
}

/**
 * @brief Getter for day of the week
 * 
 * @return unsigned int 
 */
unsigned int NTPClientPlus::getDayOfWeek()
{
    return this->_dayOfWeek;
}

/**
 * @brief Function to calc current year
 * 
 * @return unsigned int 
 */
unsigned int NTPClientPlus::getYear()
{

    unsigned long sec1900 = this->getSecsSince1900();

    //NTP starts at 1. Jan 1900
    unsigned int result = 1900;
    unsigned int dayInYear = 0;
    unsigned int days = 0;
    unsigned int days1900 = 0;

    unsigned long for_i = 0;
    bool leapYear = LOW;

    days1900 = sec1900 / this->secondperday;

    for (for_i = 0; for_i < days1900; for_i++)
    {

        leapYear = this->isLeapYear(result);

        if (leapYear)
        {
            dayInYear = 366;
        }

        else
        {
            dayInYear = 365;
        }

        days++;

        if (days >= dayInYear)
        {
            result++;
            days = 0;
        }
    }

    return result;
}

/**
 * @brief Function to check if given year is leap year
 * 
 * @param year 
 * @return true 
 * @return false 
 */
bool NTPClientPlus::isLeapYear(unsigned int year)
{

    bool result = LOW;

    // check for leap year
    if ((year % 4) == 0)
    {

        result = HIGH;

        if ((year % 100) == 0)
        {

            result = LOW;

            if ((year % 400) == 0)
            {

                result = HIGH;
            }
        }
    }

    else
    {
        result = LOW;
    }

    return result;
}

/**
 * @brief Get Month of given day of year
 * 
 * @param dayOfYear 
 * @return int 
 */
int NTPClientPlus::getMonth(int dayOfYear)
{

    bool leapYear = this->isLeapYear(this->getYear());

    // Month beginnings
    int monthMin[13] = {0, 1, 32, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335};
    // Month endings
    int monthMax[13] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};

    int month = 0;

    int y = 0;

    // Calculation of the beginning and end of each month in the leap year
    if (leapYear == HIGH)
    {

        for (y = 3; y < 13; y++)
        {
            monthMin[y] = monthMin[y] + 1;
        }

        for (y = 2; y < 13; y++)
        {
            monthMax[y] = monthMax[y] + 1;
        }
    }

    // January
    if (dayOfYear >= monthMin[1] && dayOfYear <= monthMax[1])
    {
        month = 1;
    }

    // February
    if (dayOfYear >= monthMin[2] && dayOfYear <= monthMax[2])
    {
        month = 2;
    }

    // March
    if (dayOfYear >= monthMin[3] && dayOfYear <= monthMax[3])
    {
        month = 3;
    }

    // April
    if (dayOfYear >= monthMin[4] && dayOfYear <= monthMax[4])
    {
        month = 4;
    }

    // May
    if (dayOfYear >= monthMin[5] && dayOfYear <= monthMax[5])
    {
        month = 5;
    }

    // June
    if (dayOfYear >= monthMin[6] && dayOfYear <= monthMax[6])
    {
        month = 6;
    }

    // July
    if (dayOfYear >= monthMin[7] && dayOfYear <= monthMax[7])
    {
        month = 7;
    }

    // August
    if (dayOfYear >= monthMin[8] && dayOfYear <= monthMax[8])
    {
        month = 8;
    }

    // September
    if (dayOfYear >= monthMin[9] && dayOfYear <= monthMax[9])
    {
        month = 9;
    }

    // October
    if (dayOfYear >= monthMin[10] && dayOfYear <= monthMax[10])
    {
        month = 10;
    }

    // November
    if (dayOfYear >= monthMin[11] && dayOfYear <= monthMax[11])
    {
        month = 11;
    }

    // December
    if (dayOfYear >= monthMin[12] && dayOfYear <= monthMax[12])
    {
        month = 12;
    }

    return month;
}

/**
 * @brief (private) Send NTP Packet to NTP server
 * 
 */
void NTPClientPlus::sendNTPPacket()
{
    // set all bytes in the buffer to 0
    memset(this->_packetBuffer, 0, NTP_PACKET_SIZE);
    // Initialize values needed to form NTP request
    this->_packetBuffer[0] = 0b11100011; // LI, Version, Mode
    this->_packetBuffer[1] = 0;          // Stratum, or type of clock
    this->_packetBuffer[2] = 6;          // Polling Interval
    this->_packetBuffer[3] = 0xEC;       // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    this->_packetBuffer[12] = 49;
    this->_packetBuffer[13] = 0x4E;
    this->_packetBuffer[14] = 49;
    this->_packetBuffer[15] = 52;

    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    if (this->_poolServerName)
    {
        this->_udp->beginPacket(this->_poolServerName, 123);
    }
    else
    {
        this->_udp->beginPacket(this->_poolServerIP, 123);
    }
    this->_udp->write(this->_packetBuffer, NTP_PACKET_SIZE);
    this->_udp->endPacket();
}

/**
 * @brief (private) Set time offset accordance to summer time
 * 
 * @param summertime 
 */
void NTPClientPlus::setSummertime(bool summertime)
{
    if (summertime)
    {
        this->_timeOffset = this->secondperhour * (this->_utcx + 1);
    }
    else
    {
        this->_timeOffset = this->secondperhour * (this->_utcx);
    }
}

/**
 * @brief (private) Update Summer/Winter time change
 * 
 * @returns bool summertime active
 */
bool NTPClientPlus::updateSWChange()
{
    unsigned int dayOfWeek = this->_dayOfWeek; 
    unsigned int dateDay = this->_dateDay; 
    unsigned int dateMonth = this->_dateMonth;

    bool summertimeActive = false;
    
    if (this->_swChange)
    {
        //Start: Set summer-/ winter time

        // current month is march
        if (dateMonth == 3)
        {

            // it is last week in march
            if ((this->daysInMonth[3] - dateDay) < 7)
            {

                //Example year 2020: March 31 days; Check on March 26, 2020 (Thursday = weekday = 4); 5 days remaining; Last Sunday March 29, 2020
                //Calculation: 31 - 26 = 5; 5 + 4 = 9;
                //Result: Last day in March is a Tuesday. There follows another Sunday in October => set winter time

                //Example year 2021: March 31 days; Check on March 30, 2021 (Tuesday = weekday = 2); 1 days remaining; Last Sunday March 28, 2021
                //Calculation: 31 - 30 = 1; 1 + 2 = 3;
                //Result: Last day in March is a Wednesday. Changeover to summer time already done => set summer time

                // If today is Sunday (dayOfWeek == 7) then this is already the last sunday in march -> set summer time
                if(dayOfWeek == 7){
                    this->setSummertime(1);
                    summertimeActive = true;
                }

                //There follows within the last week in March one more Sunday => set winter time
                else if (((this->daysInMonth[3] - dateDay) + dayOfWeek) >= 7)
                {
                    this->setSummertime(0);
                    summertimeActive = false;
                }

                // last sunday in march already over -> set summer time
                else
                {
                    this->setSummertime(1);
                    summertimeActive = true;
                }
            }

            // Check in first three weeks of march -> winter time
            else
            {
                this->setSummertime(0);
                summertimeActive = false;
            }
        }

        // current month is october
        else if (dateMonth == 10)
        {

            // Check in last week of october
            if ((this->daysInMonth[10] - dateDay) < 7)
            {

                //Example year 2020: October 31 days; Check on October 26, 2020 (Monday = weekday = 1); 5 days remaining; last Sunday October 25, 2020
                //Calculation: 31 - 26 = 5; 5 + 1 = 6;
                //Result: Last day in October is a Saturday. Changeover to winter time already done => set winter time

                //Example year 2021: October 31 days; Check on 26. October 2021 (Tuesday = weekday = 2); 5 days remaining; Last Sunday 31. October 2021
                //Calculation: 31 - 26 = 5; 5 + 2 = 7;
                //Result: Last day in October is a Sunday. There follows another Sunday in October => set summer time
                
                // If today is Sunday (dayOfWeek == 7) then this is already the last sunday in october -> winter time
                if(dayOfWeek == 7){
                    this->setSummertime(0);
                    summertimeActive = false;
                }

                // There follows within the last week in October one more Sunday => summer time
                else if (((this->daysInMonth[10] - dateDay) + dayOfWeek) >= 7)
                {
                    this->setSummertime(1);
                    summertimeActive = true;
                }

                // last sunday in october already over -> winter time
                else
                {
                    this->setSummertime(0);
                    summertimeActive = false;
                }
            }

            // Check in first three weeks of october -> summer time
            else
            {
                this->setSummertime(1);
                summertimeActive = true;
            }
        }

        // Check in summer time
        else if (dateMonth > 3 && dateMonth < 10)
        {
            this->setSummertime(1);
            summertimeActive = true;
        }

        // Check in winter time
        else if (dateMonth < 3 || dateMonth > 10)
        {
            this->setSummertime(0);
            summertimeActive = false;
        }
    }

    return summertimeActive;
}