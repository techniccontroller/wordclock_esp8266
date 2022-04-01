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
 * @return true     after successful update
 * @return false    timeout after 1000 ms
 */
bool NTPClientPlus::updateNTP()
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
            return false; // timeout after 1000 ms
        timeout++;
    } while (cb == 0);

    this->_lastUpdate = millis() - (10 * (timeout + 1)); // Account for delay in reading the time

    this->_udp->read(this->_packetBuffer, NTP_PACKET_SIZE);

    unsigned long highWord = word(this->_packetBuffer[40], this->_packetBuffer[41]);
    unsigned long lowWord = word(this->_packetBuffer[42], this->_packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    this->_secsSince1900 = highWord << 16 | lowWord;

    this->_currentEpoc = this->_secsSince1900 - SEVENZYYEARS;

    return true; // return true after successful update
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
    for (int i = 1900; i < this->_dateYear; i++)
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
    for (int i = 0; i < this->_dateMonth; i++)
    {
        this->_dateDay = this->_dateDay + daysInMonth[i];
    }
    this->_dateDay = dayOfYear - this->_dateDay;

    // calc day of week:
    // Monday = 1, Tuesday = 2, Wednesday = 3, Thursday = 4, Friday = 5, Saturday = 6, Sunday = 7
    // 1. Januar 1900 was a monday
    this->_dayOfWeek = 1;

    for (int i = 0; i < days1900; i++)
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

    //NTP beginnt am 1. Januar 1900
    unsigned int result = 1900;
    unsigned int tageimjahr = 0;
    unsigned int tage = 0;
    unsigned int days1900 = 0;

    int for_i = 0;
    bool schaltjahr = LOW;

    days1900 = sec1900 / this->secondperday;

    for (for_i = 0; for_i < days1900; for_i++)
    {

        schaltjahr = this->isLeapYear(result);

        if (schaltjahr)
        {
            tageimjahr = 366;
        }

        else
        {
            tageimjahr = 365;
        }

        tage++;

        if (tage >= tageimjahr)
        {
            result++;
            tage = 0;
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

    //Monatsanfänge
    int monatMin[13] = {0, 1, 32, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335};
    //Monatsenden
    int monatMax[13] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};

    int datum = 0;

    int y = 0;

    //Berechnung des Anfang und Ende jeden Montas im Schaltjahr
    if (leapYear == HIGH)
    {

        for (y = 3; y < 13; y++)
        {
            monatMin[y] = monatMin[y] + 1;
        }

        for (y = 2; y < 13; y++)
        {
            monatMax[y] = monatMax[y] + 1;
        }
    }

    //Monat Januar
    if (dayOfYear >= monatMin[1] && dayOfYear <= monatMax[1])
    {
        datum = 1;
    }

    //Monat Februar
    if (dayOfYear >= monatMin[2] && dayOfYear <= monatMax[2])
    {
        datum = 2;
    }

    //Monat März
    if (dayOfYear >= monatMin[3] && dayOfYear <= monatMax[3])
    {
        datum = 3;
    }

    //Monat April
    if (dayOfYear >= monatMin[4] && dayOfYear <= monatMax[4])
    {
        datum = 4;
    }

    //Monat Mai
    if (dayOfYear >= monatMin[5] && dayOfYear <= monatMax[5])
    {
        datum = 5;
    }

    //Monat Juni
    if (dayOfYear >= monatMin[6] && dayOfYear <= monatMax[6])
    {
        datum = 6;
    }

    //Monat Juli
    if (dayOfYear >= monatMin[7] && dayOfYear <= monatMax[7])
    {
        datum = 7;
    }

    //Monat August
    if (dayOfYear >= monatMin[8] && dayOfYear <= monatMax[8])
    {
        datum = 8;
    }

    //Monat September
    if (dayOfYear >= monatMin[9] && dayOfYear <= monatMax[9])
    {
        datum = 9;
    }

    //Monat Oktober
    if (dayOfYear >= monatMin[10] && dayOfYear <= monatMax[10])
    {
        datum = 10;
    }

    //Monat November
    if (dayOfYear >= monatMin[11] && dayOfYear <= monatMax[11])
    {
        datum = 11;
    }

    //Monat Dezember
    if (dayOfYear >= monatMin[12] && dayOfYear <= monatMax[12])
    {
        datum = 12;
    }

    return datum;
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
 */
void NTPClientPlus::updateSWChange()
{
    unsigned int dayOfWeek = this->_dayOfWeek; 
    unsigned int dateDay = this->_dateDay; 
    unsigned dateMonth = this->_dateMonth;
    
    if (this->_swChange)
    {
        //Start: Set summer-/ winter time

        static bool initSWChange = false;
        if (initSWChange == false)
        {
            // not initialized yet
            // restart in march
            if (dateMonth == 3)
            {

                //Neustart in der letzten Woche im März
                if ((this->daysInMonth[3] - dateDay) < 7)
                {

                    //Example year 2020: March 31 days; Restart March 26, 2020 (Thursday = weekday = 4); 5 days remaining; Last Sunday March 29, 2020
                    //Calculation: 31 - 26 = 5; 5 + 4 = 9;
                    //Result: Last day in March is a Tuesday. There follows another Sunday in October => set winter time

                    //Example year 2021: March 31 days; Restart March 30, 2021 (Tuesday = weekday = 2); 1 days remaining; Last Sunday March 28, 2021
                    //Calculation: 31 - 30 = 1; 1 + 2 = 3;
                    //Result: Last day in March is a Wednesday. Changeover to summer time already done => set summer time

                    //There follows within the last week in March one more Sunday => set winter time
                    if (((this->daysInMonth[3] - dateDay) + dayOfWeek) >= 7)
                    {
                        this->setSummertime(0);
                    }

                    // last sunday in march already over -> summer time
                    else
                    {
                        this->setSummertime(1);
                    }
                }

                // restart in first three weeks of march -> winter time
                else
                {
                    this->setSummertime(0);
                }
            }

            //Neustart im Monat Oktober
            if (dateMonth == 10)
            {

                // restart last week of october
                if ((this->daysInMonth[10] - dateDay) < 7)
                {

                    //Example year 2020: October 31 days; restart October 26, 2020 (Monday = weekday = 1); 5 days remaining; last Sunday October 25, 2020
                    //Calculation: 31 - 26 = 5; 5 + 1 = 6;
                    //Result: Last day in October is a Saturday. Changeover to winter time already done => set winter time

                    //Example year 2021: October 31 days; Restart 26. October 2021 (Tuesday = weekday = 2); 5 days remaining; Last Sunday 31. October 2021
                    //Calculation: 31 - 26 = 5; 5 + 2 = 7;
                    //Result: Last day in October is a Sunday. There follows another Sunday in October => set summer time

                    // There follows within the last week in October one more Sunday => summer time
                    if (((this->daysInMonth[10] - dateDay) + dayOfWeek) >= 7)
                    {
                        this->setSummertime(1);
                    }

                    // last sunday in october already over -> winter time
                    else
                    {
                        this->setSummertime(0);
                    }
                }

                // restart in first three weeks of october -> summer time
                else
                {
                    this->setSummertime(1);
                }
            }

            // restart in summer time
            if (dateMonth > 3 && dateMonth < 10)
            {
                this->setSummertime(1);
            }

            // restart in winter time
            if (dateMonth < 3 || dateMonth > 10)
            {
                this->setSummertime(0);
            }

            initSWChange = true;
        }

        //on the last Sunday in March (03) is changed from UTC+1 to UTC+2; 02:00 -> 03:00
        //on the last Sunday in October (10) is changed from UTC+2 to UTC+1; 03:00 -> 02:00

        // call every sunday of march
        if (dateMonth == 3 && dayOfWeek == 7)
        {

            // last sunday of march
            if ((this->daysInMonth[3] - dateDay) < 7)
            {

                // change to summer time
                this->setSummertime(1);
            }
        }

        // call every sunday in october
        if (dateMonth == 10 && dayOfWeek == 7)
        {

            // last sunday in october
            if ((this->daysInMonth[10] - dateDay) < 7)
            {

                // change to winter time
                this->setSummertime(0);
            }
        }
    }
}