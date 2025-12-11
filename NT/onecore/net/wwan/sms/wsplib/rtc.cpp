#include <stdlib.h>
#include <string.h>

/* Structure definitions */
typedef struct s_tm {
    int tm_sec;      /* seconds (0-59) */
    int tm_min;      /* minutes (0-59) */
    int tm_hour;     /* hours (0-23) */
    int tm_mday;     /* day of month (1-31) */
    int tm_mon;      /* month (0-11) */
    int tm_year;     /* year - 1900 */
    int tm_wday;     /* day of week (0-6, Sunday = 0) */
    int tm_yday;     /* day of year (0-365) */
    int tm_isdst;    /* daylight saving time */
} s_tm;

typedef struct ZoneInfo {
    char *iZoneName;
    long iZoneOffset;
} ZoneInfo;

/* Global variables */

char *lDays[7] = {
    "Sun", "Mon", "Tue", "Wed", 
    "Thu", "Fri", "Sat"
};

char *lMonths[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

ZoneInfo ZoneInfo[0x24] = {
    {"UT", 0},
    {"UTC", 0},
    {"GMT", 0},
    {"EST", -0x1450},
    {"EDT", -0xE39},
    {"CST", -0x1554},
    {"CDT", -0x1450},
    {"MST", -0x166C},
    {"MDT", -0x1554},
    {"PST", -0x1780},
    {"PDT", -0x166C},
    {"BST", 0xE10},
    {"A", -0xE10},
    {"B", -0x1C20},
    {"C", -0x2A30},
    {"D", -0x3839},
    {"E", -0x4650},
    {"F", -0x5454},
    {"G", -0x6263},
    {"H", -0x7070},
    {"I", -0x7E7F},
    {"K", -0x9C8D},
    {"L", -0xAA9B},
    {"M", -0xB8A9},
    {"N", 0xE10},
    {"O", 0x1C20},
    {"P", 0x2A30},
    {"Q", 0x3838},
    {"R", 0x4646},
    {"S", 0x5454},
    {"T", 0x6262},
    {"U", 0x7070},
    {"V", 0x7E7E},
    {"W", 0x8C8C},
    {"X", 0x9A9A},
    {"Y", 0xA8A8}
};

int _days[13] = {
    -1, 30, 58, 89,
    119, 150, 180, 211,
    242, 272, 303, 333,
    364
};

int lFindEntry(char **stringArray, char *searchString, unsigned long arraySize)
{
    int i;
    
    if (stringArray == NULL || searchString == NULL) {
        return -1;
    }
    
    for (i = 0; i < (int)arraySize; i++) {
        if (stringArray[i] != NULL) {
            if (_strnicmp(stringArray[i], searchString, strlen(searchString)) == 0) {
                return i;
            }
        }
    }
    
    return -1;
}

int lGetNumA(char *str, char **endPtr)
{
    int result = 0;
    char ch;
    
    if (str == NULL) {
        if (endPtr != NULL) {
            *endPtr = NULL;
        }
        return -1;
    }
    
    /* Parse decimal number */
    while ((ch = *str) >= '0' && ch <= '9') {
        result = result * 10 + (ch - '0');
        str++;
    }
    
    if (endPtr != NULL) {
        *endPtr = str;
    }
    
    return result;
}

long Rtc_DecodeZoneString(char *zoneStr, long *offsetResult)
{
    char *current;
    char ch;
    long offset = 0;
    long result = -1;
    unsigned int i;
    int cmpResult;
    
    if (zoneStr == NULL || offsetResult == NULL) {
        return -1;
    }
    
    /* Skip leading whitespace */
    current = zoneStr;
    while (*current == ' ' || (*current >= '\t' && *current <= '\r')) {
        current++;
    }
    
    ch = *current;
    
    /* Check for numeric offset (+/-HHMM or +/-HHMMSS) */
    if (ch == '-' || ch == '+') {
        /* Check if followed by 4 or 6 digits */
        if (current[1] >= '0' && current[1] <= '9' &&
            current[2] >= '0' && current[2] <= '9' &&
            current[3] >= '0' && current[3] <= '9' &&
            current[4] >= '0' && current[4] <= '9') {
            
            /* Parse HHMM format */
            int hours = (current[1] - '0') * 10 + (current[2] - '0');
            int minutes = (current[3] - '0') * 10 + (current[4] - '0');
            int seconds = 0;
            
            offset = hours * 3600 + minutes * 60;
            
            /* Check for optional seconds (HHMMSS format) */
            if (current[5] >= '0' && current[5] <= '9' &&
                current[6] >= '0' && current[6] <= '9') {
                seconds = (current[5] - '0') * 10 + (current[6] - '0');
                offset += seconds;
            }
            
            if (ch == '-') {
                offset = -offset;
            }
            
            result = 0;
        }
    } 
    else {
        /* Look up timezone name */
        for (i = 0; i < 0x24; i++) {  /* 36 timezones */
            if (ZoneInfo[i].iZoneName != NULL) {
                cmpResult = _stricmp(ZoneInfo[i].iZoneName, current);
                if (cmpResult == 0) {
                    offset = ZoneInfo[i].iZoneOffset;
                    result = 0;
                    break;
                }
            }
        }
    }
    
    *offsetResult = offset;
    return result;
}

long Rtc_MkTime(s_tm *tm)
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    long totalSeconds;
    int daysInMonth;
    int isLeapYear;
    long temp;
    
    if (tm == NULL) {
        return -1;
    }
    
    year = tm->tm_year;
    month = tm->tm_mon;
    day = tm->tm_mday;
    hour = tm->tm_hour;
    minute = tm->tm_min;
    second = tm->tm_sec;
    
    /* Validate year range (1900 + 69 = 1969 to 1900 + 69 + 70 = 2039) */
    if (year < 69 || year > 69 + 70) {
        return -1;
    }
    
    /* Adjust month if out of range */
    if (month < 0 || month > 11) {
        year += month / 12;
        month = month % 12;
        if (month < 0) {
            year -= 1;
            month += 12;
        }
        
        /* Re-check year range after adjustment */
        if (year < 69 || year > 69 + 70) {
            return -1;
        }
    }
    
    /* Get days in month */
    daysInMonth = _days[month];
    
    /* Adjust for leap year in February */
    isLeapYear = ((year + 1900) % 4 == 0);
    if (isLeapYear && month > 1) {  /* February is month 1 (0-based) */
        daysInMonth += 1;
    }
    
    /* Calculate days since epoch (1970-01-01) */
    /* First, days from years */
    totalSeconds = (long)(year - 70) * 365L;
    
    /* Add leap days (excluding current year) */
    totalSeconds += (long)((year - 1) / 4 - 69 / 4);
    
    /* Add days from months */
    totalSeconds += (long)daysInMonth;
    
    /* Add days in current month (1-based to 0-based) */
    totalSeconds += (long)(day - 1);
    
    /* Check for overflow in days calculation */
    if (totalSeconds < 0) {
        return -1;
    }
    
    /* Convert days to hours */
    temp = totalSeconds * 24L;
    if (totalSeconds != 0 && temp / totalSeconds != 24) {
        return -1;
    }
    
    totalSeconds = temp + (long)hour;
    
    /* Check for overflow in hours calculation */
    if ((temp < 0 && hour < 0 && totalSeconds >= 0) ||
        (temp >= 0 && hour >= 0 && totalSeconds < 0)) {
        return -1;
    }
    
    /* Convert hours to minutes */
    temp = totalSeconds * 60L;
    if (totalSeconds != 0 && temp / totalSeconds != 60) {
        return -1;
    }
    
    totalSeconds = temp + (long)minute;
    
    /* Check for overflow in minutes calculation */
    if ((temp < 0 && minute < 0 && totalSeconds >= 0) ||
        (temp >= 0 && minute >= 0 && totalSeconds < 0)) {
        return -1;
    }
    
    /* Convert minutes to seconds */
    temp = totalSeconds * 60L;
    if (totalSeconds != 0 && temp / totalSeconds != 60) {
        return -1;
    }
    
    totalSeconds = temp + (long)second;
    
    /* Check for overflow in seconds calculation */
    if ((temp < 0 && second < 0 && totalSeconds >= 0) ||
        (temp >= 0 && second >= 0 && totalSeconds < 0)) {
        return -1;
    }
    
    /* Adjust for daylight saving time */
    if (tm->tm_isdst != 0) {
        totalSeconds -= 3600L;  /* Subtract one hour for DST */
    }
    
    return totalSeconds;
}

long Rtc_ProcessInetDate(char *dateStr)
{
    char *current;
    char *dayNameEnd;
    int dayIndex;
    int day;
    int month;
    int year;
    int hour;
    int minute;
    int second;
    long timezoneOffset;
    long timestamp;
    s_tm tm;
    
    if (dateStr == NULL) {
        return -1;
    }
    
    current = dateStr;
    
    /* Skip optional day name (e.g., "Mon, ") */
    dayIndex = lFindEntry(lDays, current, 7);
    if (dayIndex != -1) {
        dayNameEnd = (char*)strchr(current, ',');
        if (dayNameEnd != NULL) {
            current = dayNameEnd + 2;  /* Skip ", " */
        }
    }
    
    /* Parse day of month */
    day = lGetNumA(current, &current);
    if (day <= 0 || day > 31) {
        return -1;
    }
    
    /* Skip space */
    while (*current == ' ') {
        current++;
    }
    
    /* Parse month name */
    month = lFindEntry(lMonths, current, 12);
    if (month == -1) {
        return -1;
    }
    
    /* Skip month name */
    while (*current != ' ' && *current != '\0') {
        current++;
    }
    
    /* Skip space */
    while (*current == ' ') {
        current++;
    }
    
    /* Parse year */
    year = lGetNumA(current, &current);
    if (year == -1) {
        return -1;
    }
    
    /* Skip space */
    while (*current == ' ') {
        current++;
    }
    
    /* Parse hour */
    hour = lGetNumA(current, &current);
    if (hour == -1 || hour < 0 || hour > 23) {
        return -1;
    }
    
    /* Skip colon */
    if (*current == ':') {
        current++;
    }
    
    /* Parse minute */
    minute = lGetNumA(current, &current);
    if (minute == -1 || minute < 0 || minute > 59) {
        return -1;
    }
    
    /* Skip colon */
    if (*current == ':') {
        current++;
    }
    
    /* Parse second */
    second = lGetNumA(current, &current);
    if (second == -1 || second < 0 || second > 59) {
        return -1;
    }
    
    /* Skip space */
    while (*current == ' ') {
        current++;
    }
    
    /* Parse timezone */
    if (Rtc_DecodeZoneString(current, &timezoneOffset) != 0) {
        return -1;
    }
    
    /* Fill tm structure */
    tm.tm_sec = second;
    tm.tm_min = minute;
    tm.tm_hour = hour;
    tm.tm_mday = day;
    tm.tm_mon = month;
    tm.tm_year = (year >= 1900) ? (year - 1900) : year;
    tm.tm_wday = 0;  /* Not used */
    tm.tm_yday = 0;  /* Not used */
    tm.tm_isdst = 0; /* Assume no DST */
    
    /* Convert to timestamp */
    timestamp = Rtc_MkTime(&tm);
    if (timestamp == -1) {
        return -1;
    }
    
    /* Adjust for timezone */
    timestamp -= timezoneOffset;
    
    return timestamp;
}

