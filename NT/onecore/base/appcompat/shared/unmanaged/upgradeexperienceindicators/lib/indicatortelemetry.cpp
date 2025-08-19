#include <windows.h>
#include <strsafe.h>
#include <evntprov.h>
#include <traceloggingprovider.h>
#include <TraceLoggingCorrelationVector.hpp>
#include <combaseapi.h>
#include <string>
#include <vector>
#include <initguid.h>  // Needed for TRACELOGGING_DEFINE_PROVIDER
#include <winmeta.h>   // For TRACE_LEVEL_VERBOSE and other trace levels
#include <sal.h>       // For annotation macros

// Add typedef for uint if not already defined
typedef unsigned int uint;

#define TRACE_LEVEL_VERBOSE 0

namespace Windows {
namespace Compat {
namespace Shared {

// Forward declare RegistryKey with needed methods
class RegistryKey {
public:
    HKEY Key = nullptr;
    
    uint GetKeyStringValues(std::vector<std::wstring>* values);
    uint IsKeyValuePresent(bool* present, const wchar_t* valueName);
    bool IsKeyValueStringDataExpected(const wchar_t* valueName, const wchar_t* expectedData);
    uint ReadKeyValueString(wchar_t* buffer, const wchar_t* valueName, unsigned long bufferSize);
    uint WriteKeyValueString(const wchar_t* valueName, const wchar_t* value);
};

// Define the telemetry provider
TRACELOGGING_DEFINE_PROVIDER(
    g_provider,
    "Microsoft.Windows.Compat.IndicatorTelemetry",
    (0x9a668b3d, 0x7f47, 0x4e9e, 0x8b, 0x3c, 0x1e, 0x5d, 0x8f, 0x1a, 0x3e, 0x6d));

class IndicatorTelemetry {
public:
    bool IsSendingNonReducedTelemetry = false;
    bool IsFullSync = false;
    uint EventCount = 0;
    TraceLoggingCorrelationVector cv;  // Changed to simple declaration
    wchar_t UtcSyncId[39] = {0};
    wchar_t CensusId[39] = {0};

    uint Initialize(bool sendNonReducedTelemetry, bool isFullSync, const char* syncId, 
                   const wchar_t* eventName, RegistryKey* syncKey, const wchar_t* valueName) {
        IsSendingNonReducedTelemetry = sendNonReducedTelemetry;
        IsFullSync = isFullSync;
        GUID guid;

        // Register telemetry provider
        TraceLoggingRegister(g_provider);

        // Create correlation vector
        if (CoCreateGuid(&guid) != S_OK) {
            return 0x8007000E; // E_OUTOFMEMORY
        }
        cv.CreateCvFromGuid(guid);

        // Populate UTC sync ID
        uint result = PopulateAndSaveUtcSyncId(syncKey, valueName);
        if (result != 0) {
            return result;
        }

        // Generate census ID
        if (CoCreateGuid(&guid) != S_OK) {
            return 0x8007000E; // E_OUTOFMEMORY
        }

        if (StringFromGUID2(guid, CensusId, _countof(CensusId)) == 0) {
            return 0x8007000E; // E_OUTOFMEMORY
        }

        EventCount = 0;

        if (IsFullSync) {
            SendEventStartSync();
        }

        return 0;
    }

    uint PopulateAndSaveUtcSyncId(RegistryKey* key, const wchar_t* valueName) {
        if (!IsFullSync) {
            // Try to read existing sync ID
            if (key->ReadKeyValueString(UtcSyncId, valueName, _countof(UtcSyncId)) != 0) {
                IsFullSync = true;
            } else {
                return 0;
            }
        }

        // Generate new sync ID
        GUID guid;
        if (CoCreateGuid(&guid) != S_OK) {
            return 0x8007000E; // E_OUTOFMEMORY
        }

        if (StringFromGUID2(guid, UtcSyncId, _countof(UtcSyncId)) == 0) {
            return 0x8007000E; // E_OUTOFMEMORY
        }

        // Save to registry
        return key->WriteKeyValueString(valueName, UtcSyncId);
    }

    void SendEventAdd(const wchar_t* name, const wchar_t* value) {
        if (TraceLoggingProviderEnabled(g_provider, TRACE_LEVEL_VERBOSE, 0)) {
            char cvStr[136] = {0};
            if (!cv.ToString(cvStr)) {
                cvStr[0] = '\0'; // Ensure null termination if conversion fails
            }

            TraceLoggingWrite(
                g_provider,
                "IndicatorAdded",
                TraceLoggingBool(IsFullSync, "IsFullSync"),
                TraceLoggingWideString(name, "Name"),
                TraceLoggingWideString(value, "Value"),
                TraceLoggingString(cvStr, "CV"),
                TraceLoggingWideString(CensusId, "CensusId"),
                TraceLoggingWideString(UtcSyncId, "UtcSyncId")
            );
        }

        EventCount++;
    }

    uint SendEventChecksum() {
        if (TraceLoggingProviderEnabled(g_provider, TRACE_LEVEL_VERBOSE, 0)) {
            char cvStr[136] = {0};
            if (!cv.ToString(cvStr)) {
                cvStr[0] = '\0'; // Ensure null termination if conversion fails
            }

            wchar_t countStr[11];
            StringCchPrintfW(countStr, _countof(countStr), L"%d", EventCount);

            TraceLoggingWrite(
                g_provider,
                "IndicatorChecksum",
                TraceLoggingString(cvStr, "CV"),
                TraceLoggingWideString(countStr, "EventCount"),
                TraceLoggingWideString(CensusId, "CensusId"),
                TraceLoggingWideString(UtcSyncId, "UtcSyncId")
            );
        }

        return 0;
    }

    void SendEventEndSync() {
        if (TraceLoggingProviderEnabled(g_provider, TRACE_LEVEL_VERBOSE, 0)) {
            char cvStr[136] = {0};
            if (!cv.ToString(cvStr)) {
                cvStr[0] = '\0'; // Ensure null termination if conversion fails
            }

            TraceLoggingWrite(
                g_provider,
                "IndicatorSyncEnd",
                TraceLoggingString(cvStr, "CV"),
                TraceLoggingWideString(CensusId, "CensusId"),
                TraceLoggingWideString(UtcSyncId, "UtcSyncId")
            );
        }

        EventCount++;
    }

    void SendEventRemove(const wchar_t* name) {
        if (TraceLoggingProviderEnabled(g_provider, TRACE_LEVEL_VERBOSE, 0)) {
            char cvStr[136] = {0};
            if (!cv.ToString(cvStr)) {
                cvStr[0] = '\0'; // Ensure null termination if conversion fails
            }

            TraceLoggingWrite(
                g_provider,
                "IndicatorRemoved",
                TraceLoggingBool(IsFullSync, "IsFullSync"),
                TraceLoggingWideString(name, "Name"),
                TraceLoggingString(cvStr, "CV"),
                TraceLoggingWideString(CensusId, "CensusId"),
                TraceLoggingWideString(UtcSyncId, "UtcSyncId")
            );
        }

        EventCount++;
    }

    void SendEventStartSync() {
        if (TraceLoggingProviderEnabled(g_provider, TRACE_LEVEL_VERBOSE, 0)) {
            char cvStr[136] = {0};
            if (!cv.ToString(cvStr)) {
                cvStr[0] = '\0'; // Ensure null termination if conversion fails
            }

            TraceLoggingWrite(
                g_provider,
                "IndicatorSyncStart",
                TraceLoggingString(cvStr, "CV"),
                TraceLoggingWideString(CensusId, "CensusId"),
                TraceLoggingWideString(UtcSyncId, "UtcSyncId")
            );
        }

        EventCount++;
    }

    uint SendIndicator(const wchar_t* name, const wchar_t* value, bool isReduced, RegistryKey* key) {
        if (!isReduced && !IsSendingNonReducedTelemetry) {
            return 0;
        }

        if (!IsFullSync && key && key->Key) {
            bool exists = false;
            if (key->IsKeyValueStringDataExpected(name, value) && exists) {
                return 0;
            }
        }

        SendEventAdd(name, value);
        return 0;
    }

    uint SendIndicatorsRemoved(RegistryKey* currentKey, RegistryKey* key, bool isFullSync) {
        if (!IsFullSync && (!IsSendingNonReducedTelemetry && !isFullSync)) {
            return 0;
        }

        if (!currentKey || !currentKey->Key) {
            return 0;
        }

        std::vector<std::wstring> values;
        uint result = currentKey->GetKeyStringValues(&values);
        if (result != 0) {
            return result;
        }

        for (const auto& value : values) {
            bool present = false;
            result = key->IsKeyValuePresent(&present, value.c_str());
            if (result != 0) {
                return result;
            }

            if (!present) {
                SendEventRemove(value.c_str());
            }
        }

        return 0;
    }

    uint UnInitialize() {
        if (IsFullSync) {
            SendEventEndSync();
        }

        uint result = SendEventChecksum();
        if (result != 0) {
            return result;
        }

        // Replace the g_provider.RegHandle check with TraceLoggingUnregister
        TraceLoggingUnregister(g_provider);

        return 0;
   }
};

} // namespace Shared
} // namespace Compat
} // namespace Windows

