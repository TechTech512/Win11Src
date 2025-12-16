#include <windows.h>
#include <mi.h>

MI_Result SsCoreExtMiApplicationClose(MI_Application* application)
{
    if (application == NULL || application->ft == NULL) {
        return MI_RESULT_INVALID_PARAMETER;
    }
    return application->ft->Close(application);
}

MI_Result SsCoreExtMiOperationClose(MI_Application* application)
{
    if (application == NULL || application->ft == NULL) {
        return MI_RESULT_INVALID_PARAMETER;
    }
    return application->ft->Close(application);
}

MI_Result SsCoreExtMiApplicationInitialize(
    MI_Uint32 flags,
    const MI_Char* applicationID,
    MI_Instance* extendedError,
    MI_Application* application)
{
    return MI_Application_InitializeV1(flags, applicationID, &extendedError, application);
}

MI_Result SsCoreExtMiApplicationNewOperationOptions(
    MI_Application* application,
    MI_Boolean mustUnderstand,
    MI_OperationOptions* options)
{
    if (application == NULL || application->ft == NULL) {
        if (options != NULL) {
            options->reserved1 = 0;
            options->reserved2 = 0;
            options->ft = NULL;
        }
        return MI_RESULT_INVALID_PARAMETER;
    }
    return application->ft->NewOperationOptions(application, mustUnderstand, options);
}

MI_Result SsCoreExtMiApplicationNewParameterSet(
    MI_Application* application,
    const MI_ClassDecl* classDecl,
    MI_Instance** parameters)
{
    if (application == NULL || application->ft == NULL) {
        if (parameters != NULL) {
            *parameters = NULL;
        }
        return MI_RESULT_INVALID_PARAMETER;
    }
    return application->ft->NewInstance(application, MI_T("Parameters"), classDecl, parameters);
}

MI_Result SsCoreExtMiApplicationNewSession(
    MI_Application* application,
    const MI_Char* protocol,
    const MI_Char* destination,
    MI_DestinationOptions* options,
    MI_SessionCallbacks* callbacks,
    MI_Instance* extendedError,
    MI_Session* session)
{
    if (application == NULL || application->ft == NULL) {
        if (session != NULL) {
            session->reserved1 = 0;
            session->reserved2 = 0;
            session->ft = NULL;
        }
        return MI_RESULT_INVALID_PARAMETER;
    }
    return application->ft->NewSession(application, protocol, destination, options, callbacks, &extendedError, session);
}

MI_Result SsCoreExtMiInstanceAddElement(
    MI_Instance* instance,
    const MI_Char* name,
    const MI_Value* value,
    MI_Type type,
    MI_Uint32 flags)
{
    if (instance == NULL || instance->ft == NULL) {
        return MI_RESULT_INVALID_PARAMETER;
    }
    return instance->ft->AddElement(instance, name, value, type, flags);
}

MI_Result SsCoreExtMiInstanceDelete(MI_Instance* instance)
{
    if (instance == NULL || instance->ft == NULL) {
        return MI_RESULT_INVALID_PARAMETER;
    }
    return instance->ft->Delete(instance);
}

MI_Result SsCoreExtMiOperationGetInstance(
    MI_Operation* operation,
    MI_Instance** instance,
    MI_Boolean* moreResults,
    MI_Result* result,
    const MI_Char** errorMessage,
    MI_Instance** errorDetails)
{
    if (operation == NULL || operation->ft == NULL) {
        if (result != NULL) {
            *result = MI_RESULT_INVALID_PARAMETER;
        }
        if (moreResults != NULL) {
            *moreResults = MI_FALSE;
        }
        return MI_RESULT_OK;
    }
    operation->ft->GetInstance(operation, instance, moreResults, result, errorMessage, errorDetails);
    return MI_RESULT_OK;
}

MI_Result SsCoreExtMiOperationOptionsDelete(MI_OperationOptions* options)
{
    if (options != NULL && options->ft != NULL) {
        options->ft->Delete(options);
    }
    return MI_RESULT_OK;
}

MI_Result SsCoreExtMiOperationOptionsSetResourceUriPrefix(
    MI_OperationOptions* options,
    const MI_Char* prefix)
{
    if (options == NULL || options->ft == NULL) {
        return MI_RESULT_INVALID_PARAMETER;
    }
    return options->ft->SetString(options, MI_T("__MI_OPERATIONOPTIONS_RESOURCE_URI_PREFIX"), prefix, 0);
}

typedef void (MI_CALL *MI_Session_CloseCallback)(void* context);

MI_Result SsCoreExtMiSessionClose(
    MI_Session* session,
    void* callbackContext,
    MI_Session_CloseCallback callback)
{
    if (session == NULL || session->ft == NULL) {
        if (callback == NULL) {
            return MI_RESULT_INVALID_PARAMETER;
        }
        callback(callbackContext);
        return MI_RESULT_OK;
    }
    return session->ft->Close(session, callbackContext, callback);
}

MI_Result SsCoreExtMiSessionInvoke(
    MI_Session* session,
    MI_Uint32 flags,
    MI_OperationOptions* options,
    const MI_Char* namespaceName,
    const MI_Char* className,
    const MI_Char* methodName,
    const MI_Instance* instance,
    const MI_Instance* parameters,
    MI_OperationCallbacks* callbacks,
    MI_Operation* operation)
{
    if (session == NULL || session->ft == NULL) {
        if (operation != NULL) {
            operation->reserved1 = 0;
            operation->reserved2 = 0;
            operation->ft = NULL;
        }
        if (callbacks != NULL && callbacks->instanceResult != NULL) {
            callbacks->instanceResult(
                NULL,
                callbacks->callbackContext,
                NULL,
                MI_FALSE,
                MI_RESULT_INVALID_PARAMETER,
                NULL,
                NULL,
                NULL);
        }
        return MI_RESULT_OK;
    }
    session->ft->Invoke(session, flags, options, namespaceName, className, methodName, instance, parameters, callbacks, operation);
    return MI_RESULT_OK;
}

