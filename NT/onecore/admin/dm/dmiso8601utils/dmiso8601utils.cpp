#include <windows.h>
#include <wchar.h>

int __cdecl ParseGetNumberEx(wchar_t* maxDigits, unsigned int outputValue, unsigned int digitsParsed, unsigned short* param_4, unsigned long* param_5);
unsigned long __cdecl HandleTimeZone(wchar_t* timeFormat, int timezoneSign, int timeData, int precision, _SYSTEMTIME* timeValues);
unsigned long __cdecl SystemTimeToISO8601String(_SYSTEMTIME* systemTime, unsigned long format, wchar_t** outputString);
unsigned long __cdecl ISO8601StringToSystemTime(wchar_t* isoString, unsigned long format, _SYSTEMTIME* systemTime, unsigned long* parseInfo);

unsigned long __cdecl ExtractDate(wchar_t* dateString, _SYSTEMTIME* systemTime, int* parseData, unsigned long* dateValues)
{
    unsigned short digitsParsed = 0;
    unsigned long* currentPosition = dateValues;
    int yearLength = 4;
    
    int parseResult = ParseGetNumberEx((wchar_t*)yearLength, (unsigned int)currentPosition, (unsigned int)&digitsParsed, (unsigned short*)parseData, (unsigned long*)systemTime);
    
    if ((parseResult != 0) && (*currentPosition > 99)) {
        bool hasDash = *(short*)((int)dateString + 8) == L'-';
        int currentIndex = 4;
        
        if (hasDash) {
            currentIndex = 5;
        }
        
        unsigned long* monthPosition = (unsigned long*)((int)currentPosition + 2);
        digitsParsed = 0;
        unsigned short fieldLength = 2;
        
        int monthResult = ParseGetNumberEx((wchar_t*)fieldLength, (unsigned int)monthPosition, (unsigned int)&digitsParsed, (unsigned short*)&yearLength, currentPosition);
        
        if ((monthResult != 0) && (*monthPosition != 0) && (*monthPosition < 13)) {
            int dayIndex = currentIndex + 2;
            
            if (hasDash) {
                if (*(short*)((int)dateString + dayIndex * 2) != L'-') {
                    return 0x80070057; // E_INVALIDARG
                }
                dayIndex = currentIndex + 3;
            }
            
            unsigned long* dayPosition = (unsigned long*)0x0;
            digitsParsed = (unsigned short)((int)currentPosition + 6);
            int dayResult = ParseGetNumberEx((wchar_t*)fieldLength, (unsigned int)digitsParsed, (unsigned int)&dayPosition, (unsigned short*)&fieldLength, monthPosition);
            
            if ((dayResult != 0) && (digitsParsed != 0) && (digitsParsed < 32)) {
                *(unsigned int*)dateString = (unsigned int)hasDash;
                *(int*)systemTime = dayIndex + 2;
                return 0; // S_OK
            }
        }
    }
    return 0x80070057; // E_INVALIDARG
}

unsigned long __cdecl ExtractTime(wchar_t* timeString, int timeData, _SYSTEMTIME* systemTime, int* timeComponents, unsigned long* timeValues)
{
    wchar_t* timeFormat = *(wchar_t**)timeData;
    int totalOffset = 0;
    wchar_t* hourPosition = timeString + 4;
    unsigned short fieldLength = 2;
    unsigned short digitsParsed = 0;
    _SYSTEMTIME* parseData = (_SYSTEMTIME*)0x0;
    wchar_t* currentPosition = hourPosition;
    int baseOffset = (int)timeValues;
    
    int hourResult = ParseGetNumberEx((wchar_t*)fieldLength, (unsigned int)hourPosition, (unsigned int)&parseData, (unsigned short*)timeComponents, (unsigned long*)systemTime);
    
    if (hourResult == 0) {
        return 0x80070057; // E_INVALIDARG
    }
    
    if (*hourPosition > 24) {
        return 0x80070057; // E_INVALIDARG
    }
    
    if ((*hourPosition == 24) && (baseOffset == 0)) {
        *hourPosition = 0;
        timeString[3] = timeString[3] + 1;
    }
    
    int minuteIndex = 2;
    
    if (timeFormat == (wchar_t*)0x0) {
        if (*(short*)((int)timeString + 4) == L':') {
            timeFormat = (wchar_t*)0x1;
            goto MINUTE_PARSE;
        }
    } else {
        if (*(short*)((int)timeString + 4) != L':') {
            return 0x80070057; // E_INVALIDARG
        }
MINUTE_PARSE:
        minuteIndex = 3;
    }
    
    parseData = (_SYSTEMTIME*)0x0;
    wchar_t* minutePosition = timeString + 5;
    unsigned short* lengthPtr = (unsigned short*)0x2;
    wchar_t* minuteCheck = minutePosition;
    
    int minuteResult = ParseGetNumberEx((wchar_t*)fieldLength, (unsigned int)minutePosition, (unsigned int)&parseData, (unsigned short*)&fieldLength, (unsigned long*)hourPosition);
    
    if ((minuteResult == 0) || (*minuteCheck > 59)) {
        return 0x80070057; // E_INVALIDARG
    }
    
    int secondIndex = minuteIndex + 2;
    
    if (timeFormat != (wchar_t*)0x0) {
        if (*(short*)((int)timeString + secondIndex * 2) != L':') {
            return 0x80070057; // E_INVALIDARG
        }
        secondIndex = minuteIndex + 3;
    }
    
    _SYSTEMTIME* secondPosition = (_SYSTEMTIME*)(timeString + 6);
    wchar_t* secondData = (wchar_t*)0x0;
    unsigned short* secondLength = (unsigned short*)0x2;
    _SYSTEMTIME* secondCheck = secondPosition;
    
    int secondResult = ParseGetNumberEx((wchar_t*)fieldLength, (unsigned int)secondPosition, (unsigned int)&secondData, (unsigned short*)lengthPtr, (unsigned long*)minutePosition);
    
    if (secondResult == 0) {
        return 0x80070057; // E_INVALIDARG
    }
    
    if (secondCheck->wYear > 60) {
        return 0x80070057; // E_INVALIDARG
    }
    
    int millisecondIndex = secondIndex + 2;
    
    if (*(short*)((int)timeString + millisecondIndex * 2) == L'.') {
        _SYSTEMTIME* millisecondPosition = (_SYSTEMTIME*)(timeString + 7);
        int precision = 3;
        _SYSTEMTIME* millisecondCheck = millisecondPosition;
        
        int millisecondResult = ParseGetNumberEx((wchar_t*)0x3, (unsigned int)millisecondPosition, (unsigned int)&digitsParsed, (unsigned short*)&fieldLength, (unsigned long*)secondPosition);
        
        if (millisecondResult == 0) {
            return 0x80070057; // E_INVALIDARG
        }
        
        if (millisecondCheck->wYear > 999) {
            return 0x80070057; // E_INVALIDARG
        }
        
        short multiplier = 1;
        if (digitsParsed == 1) {
            multiplier = 100;
        } else if (digitsParsed == 2) {
            multiplier = 10;
        }
        
        millisecondIndex = secondIndex + 3 + digitsParsed;
        millisecondCheck->wYear = millisecondCheck->wYear * multiplier;
    }
    
    short timezoneChar = *(short*)((int)timeString + millisecondIndex * 2);
    int timezoneSign = 0;
    
    if (timezoneChar == L'+') {
        timezoneSign = -1;
    } else {
        if (timezoneChar != L'-') {
            if (timezoneChar != L'Z') {
                return 0x80070057; // E_INVALIDARG
            }
            goto TIMEZONE_COMPLETE;
        }
        timezoneSign = 1;
    }
    
    millisecondIndex = millisecondIndex + 1;
    int timezoneResult = HandleTimeZone(timeFormat, timezoneSign, (int)timeString, digitsParsed, (_SYSTEMTIME*)minutePosition);
    
    if (timezoneResult < 0) {
        return timezoneResult;
    }

TIMEZONE_COMPLETE:
    *(wchar_t**)timeData = timeFormat;
    systemTime->wYear = (short)millisecondIndex;
    systemTime->wMonth = (short)((unsigned int)millisecondIndex >> 0x10);
    return timezoneResult;
}

unsigned long __cdecl FileTimeToISO8601String(_FILETIME* fileTime, unsigned long format, wchar_t** outputString)
{
    unsigned long result = 0;
    unsigned short lowPart = 0;
    unsigned short highPart = 0;
    
    if ((fileTime == NULL) || (outputString == NULL)) {
        result = 0x80070057; // E_INVALIDARG
    } else {
        SYSTEMTIME systemTime;
        int conversionResult = FileTimeToSystemTime(fileTime, &systemTime);
        
        if (conversionResult == 0) {
            result = GetLastError();
            if (result > 0) {
                result = result & 0xFFFF | 0x80070000;
            }
        } else {
            result = SystemTimeToISO8601String(&systemTime, format, outputString);
        }
    }
    
    return result;
}

unsigned long __cdecl HandleTimeZone(wchar_t* timeFormat, int timezoneSign, int timeData, int precision, _SYSTEMTIME* timeValues)
{
    unsigned long result = 0;
    unsigned short timezoneHours = 0;
    unsigned long fileTimeLow = 0;
    unsigned long fileTimeHigh = 0;
    unsigned long timezoneMinutes = 0;
    unsigned long calculatedTime = 0;
    SYSTEMTIME currentTime;
    
    unsigned short fieldLength = 2;
    unsigned long parseData = 0;
    int baseOffset = (int)timeValues;
    
    int hourResult = ParseGetNumberEx((wchar_t*)fieldLength, (unsigned int)&timezoneHours, (unsigned int)&parseData, (unsigned short*)timeFormat, (unsigned long*)&timezoneHours);
    
    if ((hourResult != 0) && (timezoneHours < 25)) {
        int spaceCheck = iswspace(*(wchar_t*)((int)timeData + 4));
        
        if ((spaceCheck != 0) || (*(wchar_t*)((int)timeData + 4) == 0)) {
TIMEZONE_CALC:
            if (baseOffset == 0) {
                currentTime.wYear = *(unsigned short*)timeData;
                currentTime.wMonth = *(unsigned short*)(timeData + 2);
                currentTime.wDay = *(unsigned short*)(timeData + 4);
                currentTime.wDayOfWeek = *(unsigned short*)(timeData + 6);
            } else {
                GetSystemTime(&currentTime);
            }
            
            currentTime.wHour = *(unsigned short*)(timeData + 8);
            currentTime.wMinute = *(unsigned short*)(timeData + 10);
            currentTime.wSecond = *(unsigned short*)(timeData + 12);
            currentTime.wMilliseconds = *(unsigned short*)(timeData + 14);
            
            int timeResult = SystemTimeToFileTime(&currentTime, (FILETIME*)&fileTimeLow);
            
            if (timeResult == 0) {
                result = GetLastError();
                if (result > 0) {
                    result = result & 0xFFFF | 0x80070000;
                }
                if ((int)result < 0) goto CLEANUP;
            }
            
            unsigned long timezoneSeconds = ((timezoneMinutes & 0xFFFF) + timezoneHours * 60) * 60;
            unsigned long long timeAdjustment = (unsigned long long)timezoneSeconds * 10000000ULL;
            
            if (timezoneSign < 0) {
                bool borrow = fileTimeLow < (unsigned long)(timeAdjustment & 0xFFFFFFFF);
                fileTimeHigh = (fileTimeHigh - (unsigned long)(timeAdjustment >> 32)) - (unsigned long)borrow;
                fileTimeLow = fileTimeLow - (unsigned long)(timeAdjustment & 0xFFFFFFFF);
            } else {
                fileTimeLow = fileTimeLow + (unsigned long)(timeAdjustment & 0xFFFFFFFF);
                fileTimeHigh = fileTimeHigh + (unsigned long)(timeAdjustment >> 32) + (fileTimeLow < (unsigned long)(timeAdjustment & 0xFFFFFFFF) ? 1 : 0);
            }
            
            int finalResult = FileTimeToSystemTime((FILETIME*)&fileTimeLow, &currentTime);
            
            if (finalResult == 0) {
                result = GetLastError();
                if (result > 0) {
                    result = result & 0xFFFF | 0x80070000;
                }
                if ((int)result < 0) goto CLEANUP;
            }
            
            if (baseOffset == 0) {
                *(unsigned short*)timeData = currentTime.wYear;
                *(unsigned short*)(timeData + 2) = currentTime.wMonth;
                *(unsigned short*)(timeData + 4) = currentTime.wDay;
                *(unsigned short*)(timeData + 6) = currentTime.wDayOfWeek;
            }
            
            *(unsigned short*)(timeData + 8) = currentTime.wHour;
            *(unsigned short*)(timeData + 10) = currentTime.wMinute;
            *(unsigned short*)(timeData + 12) = currentTime.wSecond;
            *(unsigned short*)(timeData + 14) = currentTime.wMilliseconds;
            
            goto CLEANUP;
        }
        
        if ((timeFormat == NULL) || (*(wchar_t*)((int)timeData + 4) == L':')) {
            parseData = 0;
            int minuteResult = ParseGetNumberEx((wchar_t*)fieldLength, (unsigned int)&timezoneMinutes, (unsigned int)&parseData, (unsigned short*)&fieldLength, (unsigned long*)&timezoneHours);
            
            if (minuteResult != 0) {
                timezoneMinutes = (unsigned long)minuteResult;
                if ((unsigned short)timezoneMinutes < 60) {
                    goto TIMEZONE_CALC;
                }
            }
        }
    }
    
    result = 0x80070057; // E_INVALIDARG

CLEANUP:
    return result;
}

unsigned long __cdecl ISO8601StringToFileTime(wchar_t* isoString, unsigned long format, _FILETIME* fileTime, unsigned long* parseInfo)
{
    unsigned long result = 0;
    SYSTEMTIME systemTime;
    
    systemTime.wYear = 0;
    systemTime.wMonth = 0;
    systemTime.wDayOfWeek = 0;
    systemTime.wDay = 0;
    systemTime.wHour = 0;
    systemTime.wMinute = 0;
    systemTime.wSecond = 0;
    systemTime.wMilliseconds = 0;
    
    if ((isoString == NULL) || (fileTime == NULL) || (parseInfo == NULL)) {
        result = 0x80070057; // E_INVALIDARG
    } else {
        result = ISO8601StringToSystemTime(isoString, format, &systemTime, parseInfo);
        
        if (((int)result >= 0) && (SystemTimeToFileTime(&systemTime, fileTime) == 0)) {
            result = GetLastError();
            if (result > 0) {
                result = result & 0xFFFF | 0x80070000;
            }
        }
    }
    
    return result;
}

unsigned long __cdecl ISO8601StringToSystemTime(wchar_t* isoString, unsigned long format, _SYSTEMTIME* systemTime, unsigned long* parseInfo)
{
    int dateResult;
    int timeResult;
    int dateOffset = 0;
    wchar_t* timeFormat = NULL;
    int hasDateError = 0;
    wchar_t* timePosition;
    
    if (isoString == NULL) {
        return 0x80070057; // E_INVALIDARG
    }
    
    if (systemTime == NULL) {
        return 0x80070057; // E_INVALIDARG
    }
    
    if (parseInfo != NULL) {
        systemTime->wYear = 0;
        systemTime->wMonth = 0;
        systemTime->wDayOfWeek = 0;
        systemTime->wDay = 0;
        systemTime->wHour = 0;
        systemTime->wMinute = 0;
        systemTime->wSecond = 0;
        systemTime->wMilliseconds = 0;
        *parseInfo = 0;
        
        dateResult = ExtractDate(isoString, systemTime, (int*)&timeFormat, parseInfo);
        
        if (dateResult < 0) {
            if ((format & 0x20) == 0) {
                return dateResult;
            }
            
            hasDateError = 1;
            systemTime->wYear = 0;
            systemTime->wMonth = 0;
            dateOffset = 0;
            systemTime->wDayOfWeek = 0;
            systemTime->wDay = 0;
            timeFormat = NULL;
            systemTime->wHour = 0;
            systemTime->wMinute = 0;
            systemTime->wSecond = 0;
            systemTime->wMilliseconds = 0;
            *parseInfo = *parseInfo | 0x80;
        } else {
            timePosition = isoString + dateOffset;
            
            if (*timePosition != L'T') {
                if ((format & 0x10) == 0) {
                    return 0x80070057; // E_INVALIDARG
                }
                
                int spaceCheck = iswspace(*timePosition);
                if ((spaceCheck == 0) && (*timePosition != L'\0')) {
                    return 0x80070057; // E_INVALIDARG
                }
                
                *parseInfo = *parseInfo | 0x40;
                return dateResult;
            }
            
            dateOffset = dateOffset + 1;
        }
        
        wchar_t* timeData = timeFormat;
        timeResult = ExtractTime(isoString, (int)&timeData, systemTime, (int*)&timeFormat, (unsigned long*)&dateOffset);
        
        if (timeResult < 0) {
            if ((format & 0x10) == 0) {
                return timeResult;
            }
            
            if (hasDateError == 0) {
                *parseInfo = *parseInfo | 0x40;
                systemTime->wHour = 0;
                systemTime->wMinute = 0;
                systemTime->wSecond = 0;
                systemTime->wMilliseconds = 0;
                return 0;
            }
        } else {
            if (hasDateError != 0) {
                return timeResult;
            }
            
            if (timeData == timeFormat) {
                return timeResult;
            }
        }
    }
    
    return 0x80070057; // E_INVALIDARG
}

int __cdecl ParseGetNumberEx(wchar_t* maxDigits, unsigned int outputValue, unsigned int digitsParsed, unsigned short* param_4, unsigned long* param_5)
{
    int digitCount = 0;
    wchar_t* currentDigit = NULL;
    unsigned short currentValue = 0;
    
    *(unsigned short*)outputValue = 0;
    *(unsigned int*)digitsParsed = 0;
    
    if (maxDigits != NULL) {
        do {
            int isDigit = iswdigit(*(wchar_t*)((int)param_5 + (int)currentDigit * 2));
            if (isDigit == 0) break;
            
            digitCount = digitCount + 1;
            int charOffset = (int)currentDigit * 2;
            currentDigit = (wchar_t*)((int)currentDigit + 1);
            
            currentValue = currentValue * 10 + (*(wchar_t*)((int)param_5 + charOffset) - L'0');
        } while (currentDigit < maxDigits);
    }
    
    *(unsigned int*)digitsParsed = digitCount;
    return (unsigned int)param_4 <= (unsigned int)digitCount;
}

unsigned long __cdecl SystemTimeToISO8601String(_SYSTEMTIME* systemTime, unsigned long format, wchar_t** outputString)
{
    int formatResult;
    unsigned long result = 0;
    wchar_t* formatString;
    
    if ((systemTime == NULL) || (outputString == NULL)) {
        result = 0x80070057; // E_INVALIDARG
    } else {
        if ((format & 0xF) == 2) {
            formatString = L"%1!04d!-%2!02d!-%3!02d!T%4!02d!:%5!02d!:%6!02d!.%7!03d!Z";
        } else {
            switch (format & 0xF) {
            case 0:
                formatString = L"%1!04d!-%2!02d!-%3!02d!T%4!02d!:%5!02d!:%6!02d!Z";
                break;
            case 1:
                formatString = L"%1!04d!%2!02d!%3!02d!T%4!02d!%5!02d!%6!02d!Z";
                break;
            case 4:
                formatString = L"%1!04d!-%2!02d!-%3!02d!";
                break;
            case 5:
                formatString = L"%1!04d!%2!02d!%3!02d!";
                break;
            case 8:
                formatString = L"%4!02d!:%5!02d!:%6!02d!Z";
                break;
            case 9:
                formatString = L"%4!02d!%5!02d!%6!02d!Z";
                break;
            case 10:
                formatString = L"%4!02d!:%5!02d!:%6!02d!.%7!03d!Z";
                break;
            default:
                result = 0x80070057; // E_INVALIDARG
                goto EXIT;
            }
        }
        
        formatResult = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER, 
                                    formatString, 0, 0, (LPWSTR)outputString, 0, NULL);
        
        if (formatResult == 0) {
            result = GetLastError();
            if (result > 0) {
                result = result & 0xFFFF | 0x80070000;
            }
        } else {
            result = 0;
        }
    }

EXIT:
    return result;
}

