#include <windows.h>
#include <comdef.h>
#include <taskschd.h>
#include <string.h>

#pragma comment(lib, "taskschd.lib")

namespace OSCleanup {
    class CleanupEnvironment {
    public:
        HRESULT StopCleanupTaskInternal();
        DWORD StopCleanupTaskInMain();

    private:
        HANDLE m_hMutex;
        bool m_bUseApartment;
    };

    DWORD CleanupEnvironment::StopCleanupTaskInMain()
    {
        HANDLE mutexHandle;
        DWORD result = 0;
        DWORD lastError;
        int attemptCount;

        if (this->m_hMutex == nullptr)
        {
            mutexHandle = CreateMutexW(nullptr, FALSE, L"Global\\Microsoft.Windows.Setup.Cleanup");
            this->m_hMutex = mutexHandle;
            lastError = GetLastError();

            if (lastError == ERROR_ALREADY_EXISTS)
            {
                result = CoInitializeEx(nullptr, (this->m_bUseApartment != 0) ? COINIT_APARTMENTTHREADED : COINIT_MULTITHREADED);
                
                if (SUCCEEDED(result))
                {
                    attemptCount = 0;
                    do
                    {
                        result = StopCleanupTaskInternal();
                        if (SUCCEEDED(result))
                            break;
                        attemptCount++;
                    } while (attemptCount < 3);
                    
                    CoUninitialize();
                }
            }
            else if (lastError != ERROR_SUCCESS && result > 0)
            {
                result = lastError | 0x80070000;
            }
        }

        return result;
    }

    HRESULT CleanupEnvironment::StopCleanupTaskInternal()
    {
        ITaskService* pTaskService = nullptr;
        ITaskFolder* pRootFolder = nullptr;
        IRegisteredTaskCollection* pTaskCollection = nullptr;
        IRegisteredTask* pRegisteredTask = nullptr;
        BSTR taskName = nullptr;
        HRESULT hr = E_FAIL;
        long taskCount = 0;
        long index = 1;

        hr = CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskService, reinterpret_cast<void**>(&pTaskService));
        
        if (SUCCEEDED(hr))
        {
            hr = pTaskService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
            
            if (SUCCEEDED(hr))
            {
                hr = pTaskService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
                
                if (SUCCEEDED(hr))
                {
                    hr = pRootFolder->GetTasks(TASK_ENUM_HIDDEN, &pTaskCollection);
                    
                    if (SUCCEEDED(hr))
                    {
                        hr = pTaskCollection->get_Count(&taskCount);
                        
                        if (SUCCEEDED(hr) && taskCount > 0)
                        {
                            for (index = 1; index <= taskCount; index++)
                            {
                                pRegisteredTask = nullptr;
                                hr = pTaskCollection->get_Item(_variant_t(index), &pRegisteredTask);
                                
                                if (FAILED(hr))
                                    break;

                                if (taskName != nullptr)
                                {
                                    SysFreeString(taskName);
                                    taskName = nullptr;
                                }

                                hr = pRegisteredTask->get_Name(&taskName);
                                
                                if (FAILED(hr))
                                {
                                    pRegisteredTask->Release();
                                    break;
                                }

                                if (_wcsicmp(taskName, L"SetupCleanupTask") == 0)
                                {
                                    hr = pRegisteredTask->Stop(0);
                                    pRegisteredTask->Release();
                                    break;
                                }

                                pRegisteredTask->Release();
                            }
                        }
                    }
                }
            }
        }

        if (taskName != nullptr)
            SysFreeString(taskName);

        if (pTaskCollection != nullptr)
            pTaskCollection->Release();

        if (pRootFolder != nullptr)
            pRootFolder->Release();

        if (pTaskService != nullptr)
            pTaskService->Release();

        return hr;
    }
}

