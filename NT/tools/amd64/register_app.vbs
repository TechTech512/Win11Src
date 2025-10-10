'******************************************************************************
'Microsoft Confidential. © 2002-2003 Microsoft Corporation. All rights reserved.
'
' This file may contain preliminary information or inaccuracies, 
' and may not correctly represent any associated Microsoft 
' Product as commercially released. All Materials are provided entirely 
' “AS IS.” To the extent permitted by law, MICROSOFT MAKES NO 
' WARRANTY OF ANY KIND, DISCLAIMS ALL EXPRESS, IMPLIED AND STATUTORY 
' WARRANTIES, AND ASSUMES NO LIABILITY TO YOU FOR ANY DAMAGES OF 
' ANY TYPE IN CONNECTION WITH THESE MATERIALS OR ANY INTELLECTUAL PROPERTY IN THEM. 
'******************************************************************************

Option Explicit

Wscript.Echo "" 
Wscript.Echo "REGISTER_APP.VBS version 1.6 for Windows Server 2008"
Wscript.Echo "Copyright (C) Microsoft Corporation 2002-2003. All rights reserved."
Wscript.Echo "" 


'******************************************************************************
' Parse command line arguments
'******************************************************************************
Dim Args
Set Args = Wscript.Arguments
If Args.Count < 1 Then 
	PrintsUsage
End If

Dim ProviderName, ProviderDLL, ProviderDescription
If Args.Item(0) = "-register" Then 
	If Args.Count <> 4 Then PrintsUsage

	ProviderName = Args.Item(1)
	ProviderDLL = Args.Item(2)
	ProviderDescription = Args.Item(3)

	UninstallProvider
	InstallProvider
	Wscript.Quit 0
End If 

If Args.Item(0) = "-unregister" Then 
	If Not Args.Count = 2 Then PrintsUsage
	ProviderName = Args.Item(1)
	UninstallProvider
	Wscript.Quit 0
End If

' Wrong options?
PrintsUsage

Wscript.Quit 0

'******************************************************************************
' Prints the usage
'******************************************************************************
Sub PrintsUsage

	Wscript.Echo "Usage:" 
	Wscript.Echo "" 
	Wscript.Echo " 1) Registering a VSS/VDS Provider as a COM+ application:" 
	Wscript.Echo "      CScript.exe " & Wscript.ScriptName & " -register <Provider_Name> <Provider.DLL>  <Provider_Description>" 
	Wscript.Echo "" 
	Wscript.Echo " 2) Unregistering a COM+ application associated with a VSS/VDS provider:" 
	Wscript.Echo "      CScript.exe " & Wscript.ScriptName & " -unregister <Provider_Name>" 
	Wscript.Echo "" 
	Wscript.Quit 1

End Sub


'******************************************************************************
' Installs the Provider
'******************************************************************************
Sub InstallProvider
	On Error Resume Next

	Wscript.Echo "Creating a new COM+ application:" 

	Wscript.Echo "- Creating the catalog object "
	Dim cat
	Set cat = CreateObject("COMAdmin.COMAdminCatalog") 	
	CheckError 101

	wscript.echo "- Get the Applications collection"
	Dim collApps
	Set collApps = cat.GetCollection("Applications")
	CheckCollectionError 102, cat

	Wscript.Echo "- Populate..." 
	collApps.Populate 
	CheckCollectionError 103, collApps

	Wscript.Echo "- Add new application object" 
	Dim app
	Set app = collApps.Add 
	CheckCollectionError 104, collApps

	Wscript.Echo "- Set app name = " & ProviderName & " "
	app.Value("Name") = ProviderName
	CheckObjectError 105, collApps, app

	Wscript.Echo "- Set app description = " & ProviderDescription & " "
	app.Value("Description") = ProviderDescription 
	CheckObjectError 106, collApps, app

	' Only roles added below are allowed to call in.
	Wscript.Echo "- Set app access check = true "
	app.Value("ApplicationAccessChecksEnabled") = 1   
	CheckObjectError 107, collApps, app

	' Encrypting communication
	Wscript.Echo "- Set encrypted COM communication = true "
	app.Value("Authentication") = 6	                  
	CheckObjectError 108, collApps, app

	' Secure references
	Wscript.Echo "- Set secure references = true "
	app.Value("AuthenticationCapability") = 2         
	CheckObjectError 109, collApps, app

	' Do not allow impersonation
	Wscript.Echo "- Set impersonation = false "
	app.Value("ImpersonationLevel") = 2               
	CheckObjectError 110, collApps, app

	Wscript.Echo "- Save changes..."
	collApps.SaveChanges
	CheckCollectionError 111, collApps

	wscript.echo "- Create Windows service running as Local System"
	cat.CreateServiceForApplication ProviderName, ProviderName , "SERVICE_AUTO_START", "SERVICE_ERROR_NORMAL", "", ".\localsystem", "", 0
	CheckCollectionError 112, cat

	wscript.echo "- Add the DLL component"
	cat.InstallComponent ProviderName, ProviderDLL , "", ""
        CheckCollectionError 113, cat

	'
	' Add the new role for the Local SYSTEM account
	'

	wscript.echo "Secure the COM+ application:"
	wscript.echo "- Get roles collection"
	Dim collRoles
	Set collRoles = collApps.GetCollection("Roles", app.Key)
	CheckCollectionError 120, cat

	wscript.echo "- Populate..."
	collRoles.Populate
	CheckCollectionError 121, collRoles

	wscript.echo "- Add new role"
	Dim role
	Set role = collRoles.Add
	CheckCollectionError 122, collRoles

	wscript.echo "- Set name = Administrators "
	role.Value("Name") = "Administrators"
	CheckObjectError 123, collRoles, role

	wscript.echo "- Set description = Administrators group "
	role.Value("Description") = "Administrators group"
	CheckObjectError 124, collRoles, role

	wscript.echo "- Save changes ..."
	collRoles.SaveChanges
	CheckCollectionError 125, collRoles
	
	'
	' Add users into role
	'

	wscript.echo "Granting user permissions:"
	Dim collUsersInRole
	Set collUsersInRole = collRoles.GetCollection("UsersInRole", role.Key)
	CheckCollectionError 130, collRoles

	wscript.echo "- Populate..."
	collUsersInRole.Populate
	CheckCollectionError 131, collUsersInRole

	wscript.echo "- Add new user"
	Dim user
	Set user = collUsersInRole.Add
	CheckCollectionError 132, collUsersInRole

	wscript.echo "- Searching for the Administrators account using WMI..."

	' Get the Administrators account domain and name
	Dim strQuery
	strQuery = "select * from Win32_Account where SID='S-1-5-32-544' and localAccount=TRUE"
	Dim objSet
	set objSet = GetObject("winmgmts:").ExecQuery(strQuery)
	CheckError 133

	Dim obj, Account
	for each obj in objSet
	    set Account = obj
		exit for
	next

	wscript.echo "- Set user name = .\" & Account.Name & " "
	user.Value("User") = ".\" & Account.Name
	CheckObjectError 140, collUsersInRole, user

	wscript.echo "- Add new user"
	Set user = collUsersInRole.Add
	CheckCollectionError 141, collUsersInRole

	wscript.echo "- Set user name = Local SYSTEM "
	user.Value("User") = "NT AUTHORITY\SYSTEM"
	CheckObjectError 142, collUsersInRole, user

	wscript.echo "- Save changes..."
	collUsersInRole.SaveChanges
	CheckCollectionError 143, collUsersInRole
	
	Set app      = Nothing
	Set cat      = Nothing
	Set role     = Nothing
	Set user     = Nothing

	Set collApps = Nothing
	Set collRoles = Nothing
	Set collUsersInRole	= Nothing

	set objSet   = Nothing
	set obj      = Nothing

	Wscript.Echo "Done." 

	On Error GoTo 0
End Sub


'******************************************************************************
' Uninstalls the Provider
'******************************************************************************
Sub UninstallProvider
	On Error Resume Next

	Wscript.Echo "Unregistering the existing application..." 

	wscript.echo "- Create the catalog object"
	Dim cat
	Set cat = CreateObject("COMAdmin.COMAdminCatalog")
	CheckError 201
	
	wscript.echo "- Get the Applications collection"
	Dim collApps
	Set collApps = cat.GetCollection("Applications")
	CheckCollectionError 202, cat

	wscript.echo "- Populate..."
	collApps.Populate
	CheckCollectionError 203, collApps
	
	wscript.echo "- Search for " & ProviderName & " application..."
	Dim numApps
	numApps = collApps.Count
	Dim i
	For i = numApps - 1 To 0 Step -1
	    If collApps.Item(i).Value("Name") = ProviderName Then
	        collApps.Remove(i)
		CheckCollectionError 204, collApps
                WScript.echo "- Application " & ProviderName & " removed!"
	    End If
	Next
	
	wscript.echo "- Saving changes..."
	collApps.SaveChanges
	CheckCollectionError 205, collApps

	Set collApps = Nothing
	Set cat      = Nothing

	Wscript.Echo "Done." 

	On Error GoTo 0
End Sub



'******************************************************************************
' Sub CheckError
'******************************************************************************
Sub CheckError(exitCode)
    If Err = 0 Then Exit Sub
    DumpVBScriptError exitCode

    Wscript.Quit exitCode
End Sub


'******************************************************************************
' Sub CheckCollectionError
'******************************************************************************
Sub CheckCollectionError(exitCode, coll)
    If Err = 0 Then Exit Sub
    DumpVBScriptError exitCode

    DumpComPlusError(coll.GetCollection("ErrorInfo"))

    Wscript.Quit exitCode
End Sub


'******************************************************************************
' Sub CheckObjectError
'******************************************************************************
Sub CheckObjectError(exitCode, coll, object)
    If Err = 0 Then Exit Sub
    DumpVBScriptError exitCode

    ' DumpComPlusError(coll.GetCollection("ErrorInfo", object.Key))
    DumpComPlusError(coll.GetCollection("ErrorInfo"))

    Wscript.Quit exitCode
End Sub



'******************************************************************************
' Sub DumpVBScriptError
'******************************************************************************
Sub DumpVBScriptError(exitCode)
    WScript.Echo vbNewLine & "ERROR:"
    WScript.Echo "- Error code: " & Err & " [0x" & Hex(Err) & "]"
    WScript.Echo "- Exit code: " & exitCode
    WScript.Echo "- Description: " & Err.Description
    WScript.Echo "- Source: " & Err.Source
    WScript.Echo "- Help file: " & Err.Helpfile
    WScript.Echo "- Help context: " & Err.HelpContext
End Sub


'******************************************************************************
' Sub DumpComPlusError
'******************************************************************************
Sub DumpComPlusError(errors)
    errors.Populate
    WScript.Echo "- COM+ Errors detected: (" & errors.Count & ")"

    Dim error
    Dim I
    For I = 0 to errors.Count - 1
	Set error = errors.Item(I)
        WScript.Echo "   * (COM+ ERROR " & I & ") on " & error.Value("Name")
        WScript.Echo "       ErrorCode: " & error.Value("ErrorCode") & " [0x" & Hex(error.Value("ErrorCode")) & "]"
        WScript.Echo "       MajorRef: " & error.Value("MajorRef")
        WScript.Echo "       MinorRef: " & error.Value("MinorRef")
    Next
End Sub


'' SIG '' Begin signature block
'' SIG '' MIImXAYJKoZIhvcNAQcCoIImTTCCJkkCAQExDzANBglg
'' SIG '' hkgBZQMEAgEFADB3BgorBgEEAYI3AgEEoGkwZzAyBgor
'' SIG '' BgEEAYI3AgEeMCQCAQEEEE7wKRaZJ7VNj+Ws4Q8X66sC
'' SIG '' AQACAQACAQACAQACAQAwMTANBglghkgBZQMEAgEFAAQg
'' SIG '' t2OGjVuwrDi7m9eD1oGHZt1e8mT97G6PYHdAzoXpmRWg
'' SIG '' ggt2MIIE/jCCA+agAwIBAgITMwAABaZYEGdLPWx89gAA
'' SIG '' AAAFpjANBgkqhkiG9w0BAQsFADB+MQswCQYDVQQGEwJV
'' SIG '' UzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMH
'' SIG '' UmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBv
'' SIG '' cmF0aW9uMSgwJgYDVQQDEx9NaWNyb3NvZnQgQ29kZSBT
'' SIG '' aWduaW5nIFBDQSAyMDEwMB4XDTI0MDgyMjE5MjU1N1oX
'' SIG '' DTI1MDcwNTE5MjU1N1owdDELMAkGA1UEBhMCVVMxEzAR
'' SIG '' BgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1v
'' SIG '' bmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlv
'' SIG '' bjEeMBwGA1UEAxMVTWljcm9zb2Z0IENvcnBvcmF0aW9u
'' SIG '' MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA
'' SIG '' umNGOR6LgZFk1THk2vW3NrXGKPwTj9NWpe4hKbCXtraY
'' SIG '' afofK2h67UZgcaUTBp5DnWodrzw5nhwZPG9glhUmMj/e
'' SIG '' sHENG8/ya59HX9Nv6jQY2MLVPB+Mr/DwlcBtSyXzUbeV
'' SIG '' nmdmIPF+pxkGFEQLl8KY0bnMmJT5S+s6uuJ12SiACfdu
'' SIG '' OwvE0JOP44cvTsNjy8PCHNnWo3ejNQVmGUz5Nzn31Li3
'' SIG '' W8OWY5J7BKMU2c/lf34/VMjJdrPq7qYeDo2IJsYMkSPN
'' SIG '' ysvnyvokbaWA4oy8ANC7j4m+Ou1WL9JlpDFWr5gN7jfF
'' SIG '' pbUyqsrSK9rfiwNsdmEAA8yXpII+qEzHEwIDAQABo4IB
'' SIG '' fTCCAXkwHwYDVR0lBBgwFgYKKwYBBAGCNz0GAQYIKwYB
'' SIG '' BQUHAwMwHQYDVR0OBBYEFE6cI++DFsHzFTkxX8wNFC39
'' SIG '' UpYtMFQGA1UdEQRNMEukSTBHMS0wKwYDVQQLEyRNaWNy
'' SIG '' b3NvZnQgSXJlbGFuZCBPcGVyYXRpb25zIExpbWl0ZWQx
'' SIG '' FjAUBgNVBAUTDTIzMDg2NSs1MDI3MTIwHwYDVR0jBBgw
'' SIG '' FoAU5vxfe7siAFjkck619CF0IzLm76wwVgYDVR0fBE8w
'' SIG '' TTBLoEmgR4ZFaHR0cDovL2NybC5taWNyb3NvZnQuY29t
'' SIG '' L3BraS9jcmwvcHJvZHVjdHMvTWljQ29kU2lnUENBXzIw
'' SIG '' MTAtMDctMDYuY3JsMFoGCCsGAQUFBwEBBE4wTDBKBggr
'' SIG '' BgEFBQcwAoY+aHR0cDovL3d3dy5taWNyb3NvZnQuY29t
'' SIG '' L3BraS9jZXJ0cy9NaWNDb2RTaWdQQ0FfMjAxMC0wNy0w
'' SIG '' Ni5jcnQwDAYDVR0TAQH/BAIwADANBgkqhkiG9w0BAQsF
'' SIG '' AAOCAQEAtf5mqFjLeh0YfVmslPmBu6sVN3x8lLTdH2eY
'' SIG '' 1PWbITjY6IEh5YtAf6y2pmPjRij7ebZeJB1lLjdzDh7/
'' SIG '' 0Y/XG9vQUNyuczRPjMwAk1FNW+w1n8NG2XHMLy+YDqlF
'' SIG '' UjcuvxVSQXDqLO374g51sl75wnuPb+uoQyMC6s7BdfNm
'' SIG '' 376xWV+7cVq0PfJltFIciJPp8bxp6zjZ6od39acS/QWe
'' SIG '' 8710FpYi9ENiV845KUKBFTq4MX6f84Rtz8CDCa5/Yonk
'' SIG '' NVUerDLKo1b0s6m8b8zMaiu/2s3tj97VP5SvAGz6uhwF
'' SIG '' nnLZXUJR47uy7tNI6dzWNz9XBgm0DSVjkf1nvdX0QDCC
'' SIG '' BnAwggRYoAMCAQICCmEMUkwAAAAAAAMwDQYJKoZIhvcN
'' SIG '' AQELBQAwgYgxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpX
'' SIG '' YXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYD
'' SIG '' VQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xMjAwBgNV
'' SIG '' BAMTKU1pY3Jvc29mdCBSb290IENlcnRpZmljYXRlIEF1
'' SIG '' dGhvcml0eSAyMDEwMB4XDTEwMDcwNjIwNDAxN1oXDTI1
'' SIG '' MDcwNjIwNTAxN1owfjELMAkGA1UEBhMCVVMxEzARBgNV
'' SIG '' BAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQx
'' SIG '' HjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEo
'' SIG '' MCYGA1UEAxMfTWljcm9zb2Z0IENvZGUgU2lnbmluZyBQ
'' SIG '' Q0EgMjAxMDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCC
'' SIG '' AQoCggEBAOkOZFB5Z7XE4/0JAEyelKz3VmjqRNjPxVhP
'' SIG '' qaV2fG1FutM5krSkHvn5ZYLkF9KP/UScCOhlk84sVYS/
'' SIG '' fQjjLiuoQSsYt6JLbklMaxUH3tHSwokecZTNtX9LtK8I
'' SIG '' 2MyI1msXlDqTziY/7Ob+NJhX1R1dSfayKi7VhbtZP/iQ
'' SIG '' tCuDdMorsztG4/BGScEXZlTJHL0dxFViV3L4Z7klIDTe
'' SIG '' XaallV6rKIDN1bKe5QO1Y9OyFMjByIomCll/B+z/Du2A
'' SIG '' EjVMEqa+Ulv1ptrgiwtId9aFR9UQucboqu6Lai0FXGDG
'' SIG '' tCpbnCMcX0XjGhQebzfLGTOAaolNo2pmY3iT1TDPlR8C
'' SIG '' AwEAAaOCAeMwggHfMBAGCSsGAQQBgjcVAQQDAgEAMB0G
'' SIG '' A1UdDgQWBBTm/F97uyIAWORyTrX0IXQjMubvrDAZBgkr
'' SIG '' BgEEAYI3FAIEDB4KAFMAdQBiAEMAQTALBgNVHQ8EBAMC
'' SIG '' AYYwDwYDVR0TAQH/BAUwAwEB/zAfBgNVHSMEGDAWgBTV
'' SIG '' 9lbLj+iiXGJo0T2UkFvXzpoYxDBWBgNVHR8ETzBNMEug
'' SIG '' SaBHhkVodHRwOi8vY3JsLm1pY3Jvc29mdC5jb20vcGtp
'' SIG '' L2NybC9wcm9kdWN0cy9NaWNSb29DZXJBdXRfMjAxMC0w
'' SIG '' Ni0yMy5jcmwwWgYIKwYBBQUHAQEETjBMMEoGCCsGAQUF
'' SIG '' BzAChj5odHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vcGtp
'' SIG '' L2NlcnRzL01pY1Jvb0NlckF1dF8yMDEwLTA2LTIzLmNy
'' SIG '' dDCBnQYDVR0gBIGVMIGSMIGPBgkrBgEEAYI3LgMwgYEw
'' SIG '' PQYIKwYBBQUHAgEWMWh0dHA6Ly93d3cubWljcm9zb2Z0
'' SIG '' LmNvbS9QS0kvZG9jcy9DUFMvZGVmYXVsdC5odG0wQAYI
'' SIG '' KwYBBQUHAgIwNB4yIB0ATABlAGcAYQBsAF8AUABvAGwA
'' SIG '' aQBjAHkAXwBTAHQAYQB0AGUAbQBlAG4AdAAuIB0wDQYJ
'' SIG '' KoZIhvcNAQELBQADggIBABp071dPKXvEFoV4uFDTIvwJ
'' SIG '' nayCl/g0/yosl5US5eS/z7+TyOM0qduBuNweAL7SNW+v
'' SIG '' 5X95lXflAtTx69jNTh4bYaLCWiMa8IyoYlFFZwjjPzwe
'' SIG '' k/gwhRfIOUCm1w6zISnlpaFpjCKTzHSY56FHQ/JTrMAP
'' SIG '' MGl//tIlIG1vYdPfB9XZcgAsaYZ2PVHbpjlIyTdhbQfd
'' SIG '' UxnLp9Zhwr/ig6sP4GubldZ9KFGwiUpRpJpsyLcfShoO
'' SIG '' aanX3MF+0Ulwqratu3JHYxf6ptaipobsqBBEm2O2smmJ
'' SIG '' BsdGhnoYP+jFHSHVe/kCIy3FQcu/HUzIFu+xnH/8IktJ
'' SIG '' im4V46Z/dlvRU3mRhZ3V0ts9czXzPK5UslJHasCqE5XS
'' SIG '' jhHamWdeMoz7N4XR3HWFnIfGWleFwr/dDY+Mmy3rtO7P
'' SIG '' J9O1Xmn6pBYEAackZ3PPTU+23gVWl3r36VJN9HcFT4XG
'' SIG '' 2Avxju1CCdENduMjVngiJja+yrGMbqod5IXaRzNij6TJ
'' SIG '' kTNfcR5Ar5hlySLoQiElihwtYNk3iUGJKhYP12E8lGhg
'' SIG '' Uu/WR5mggEDuFYF3PpzgUxgaUB04lZseZjMTJzkXeIc2
'' SIG '' zk7DX7L1PUdTtuDl2wthPSrXkizON1o+QEIxpB8QCMJW
'' SIG '' nL8kXVECnWp50hfT2sGUjgd7JXFEqwZq5tTG3yOalnXF
'' SIG '' MYIaPjCCGjoCAQEwgZUwfjELMAkGA1UEBhMCVVMxEzAR
'' SIG '' BgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1v
'' SIG '' bmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlv
'' SIG '' bjEoMCYGA1UEAxMfTWljcm9zb2Z0IENvZGUgU2lnbmlu
'' SIG '' ZyBQQ0EgMjAxMAITMwAABaZYEGdLPWx89gAAAAAFpjAN
'' SIG '' BglghkgBZQMEAgEFAKCBxjAZBgkqhkiG9w0BCQMxDAYK
'' SIG '' KwYBBAGCNwIBBDAcBgorBgEEAYI3AgELMQ4wDAYKKwYB
'' SIG '' BAGCNwIBFTAvBgkqhkiG9w0BCQQxIgQgorqISAsetE22
'' SIG '' SO7QBE0kVhcpYE9Wme7Kh+uQRwMUI5IwWgYKKwYBBAGC
'' SIG '' NwIBDDFMMEqgJIAiAE0AaQBjAHIAbwBzAG8AZgB0ACAA
'' SIG '' VwBpAG4AZABvAHcAc6EigCBodHRwOi8vd3d3Lm1pY3Jv
'' SIG '' c29mdC5jb20vd2luZG93czANBgkqhkiG9w0BAQEFAASC
'' SIG '' AQAnBPNMirpYg3yzdG48S5jFfR1i8jIiZubMJtGfIoG7
'' SIG '' 2tvQrv54DJzLFFdUeVBIPcBGO7f1XBhR4tn52ShNER8R
'' SIG '' eJMsODKtGDPBdWHpIeNraNus8ENDWSKCnpRIvLjPnHju
'' SIG '' DOGu/vWlMMQjWJ1aGnBzH7qVIKnMYQGiSB1BJFUXSFez
'' SIG '' roO7vX6NV1CH2XZKErc7zPrUQOflryh1H5KADOSIJKcN
'' SIG '' 20YCtsN3jJpbdhzpVgijviYGMLmRr2K3DF1weJQg23Gn
'' SIG '' CkbqQK1x8dePFTPWTWNw5+7npQepzIcB3V22hYF1FikZ
'' SIG '' AK/TOkAQDr2W5jeawx5QSb8pYWMvb6YMxCitoYIXsDCC
'' SIG '' F6wGCisGAQQBgjcDAwExghecMIIXmAYJKoZIhvcNAQcC
'' SIG '' oIIXiTCCF4UCAQMxDzANBglghkgBZQMEAgEFADCCAVoG
'' SIG '' CyqGSIb3DQEJEAEEoIIBSQSCAUUwggFBAgEBBgorBgEE
'' SIG '' AYRZCgMBMDEwDQYJYIZIAWUDBAIBBQAEICdoHzm0Cnkm
'' SIG '' Yvy3c9PhjnhjHTMBNlqlZOFjLDEVOrr7AgZm6yrdHU4Y
'' SIG '' EzIwMjQxMTE2MDkxNjQ2Ljk5M1owBIACAfSggdmkgdYw
'' SIG '' gdMxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5n
'' SIG '' dG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVN
'' SIG '' aWNyb3NvZnQgQ29ycG9yYXRpb24xLTArBgNVBAsTJE1p
'' SIG '' Y3Jvc29mdCBJcmVsYW5kIE9wZXJhdGlvbnMgTGltaXRl
'' SIG '' ZDEnMCUGA1UECxMeblNoaWVsZCBUU1MgRVNOOjMyMUEt
'' SIG '' MDVFMC1EOTQ3MSUwIwYDVQQDExxNaWNyb3NvZnQgVGlt
'' SIG '' ZS1TdGFtcCBTZXJ2aWNloIIR/jCCBygwggUQoAMCAQIC
'' SIG '' EzMAAAH4o6EmDAxASP4AAQAAAfgwDQYJKoZIhvcNAQEL
'' SIG '' BQAwfDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hp
'' SIG '' bmd0b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoT
'' SIG '' FU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEmMCQGA1UEAxMd
'' SIG '' TWljcm9zb2Z0IFRpbWUtU3RhbXAgUENBIDIwMTAwHhcN
'' SIG '' MjQwNzI1MTgzMTA4WhcNMjUxMDIyMTgzMTA4WjCB0zEL
'' SIG '' MAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24x
'' SIG '' EDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jv
'' SIG '' c29mdCBDb3Jwb3JhdGlvbjEtMCsGA1UECxMkTWljcm9z
'' SIG '' b2Z0IElyZWxhbmQgT3BlcmF0aW9ucyBMaW1pdGVkMScw
'' SIG '' JQYDVQQLEx5uU2hpZWxkIFRTUyBFU046MzIxQS0wNUUw
'' SIG '' LUQ5NDcxJTAjBgNVBAMTHE1pY3Jvc29mdCBUaW1lLVN0
'' SIG '' YW1wIFNlcnZpY2UwggIiMA0GCSqGSIb3DQEBAQUAA4IC
'' SIG '' DwAwggIKAoICAQDFHbeldicPYG44N15ezYK79PmQoj5s
'' SIG '' DDxxu03nQKb8UCuNfIvhFOox7qVpD8Kp4xPGByS9mvUm
'' SIG '' tbQyLgXXmvH9W94aEoGahvjkOY5xXnHLHuH1OTn00CXk
'' SIG '' 80wBYoAhZ/bvRJYABbFBulUiGE9YKdVXei1W9qERp3yk
'' SIG '' yahJetPlns2TVGcHvQDZur0eTzAh4Le8G7ERfYTxfnQi
'' SIG '' AAezJpH2ugWrcSvNQQeVLxidKrfe6Lm4FysU5wU4Jkgu
'' SIG '' 5UVVOASpKtfhSJfR62qLuNS0rKmAh+VplxXlwjlcj94L
'' SIG '' FjzAM2YGmuFgw2VjF2ZD1otENxMpa111amcm3KXl7eAe
'' SIG '' 5iiPzG4NDRdk3LsRJHAkgrTf6tNmp9pjIzhdIrWzRpr6
'' SIG '' Y7r2+j82YnhH9/X4q5wE8njJR1uolYzfEy8HAtjJy+KA
'' SIG '' j9YriSA+iDRQE1zNpDANVelxT5Mxw69Y/wcFaZYlAiZN
'' SIG '' kicAWK9epRoFujfAB881uxCm800a7/XamDQXw78J1F+A
'' SIG '' 8d86EhZDQPwAsJj4uyLBvNx6NutWXg31+fbA6DawNrxF
'' SIG '' 82gPrXgjSkWPL+WrU2wGj1XgZkGKTNftmNYJGB3UUIFc
'' SIG '' al+kOKQeNDTlg6QBqR1YNPZsZJpRkkZVi16kik9MCzWB
'' SIG '' 3+9SiBx2IvnWjuyG4ciUHpBJSJDbhdiFFttAIQIDAQAB
'' SIG '' o4IBSTCCAUUwHQYDVR0OBBYEFL3OxnPPntCVPmeu3+iK
'' SIG '' 0u/U5Du2MB8GA1UdIwQYMBaAFJ+nFV0AXmJdg/Tl0mWn
'' SIG '' G1M1GelyMF8GA1UdHwRYMFYwVKBSoFCGTmh0dHA6Ly93
'' SIG '' d3cubWljcm9zb2Z0LmNvbS9wa2lvcHMvY3JsL01pY3Jv
'' SIG '' c29mdCUyMFRpbWUtU3RhbXAlMjBQQ0ElMjAyMDEwKDEp
'' SIG '' LmNybDBsBggrBgEFBQcBAQRgMF4wXAYIKwYBBQUHMAKG
'' SIG '' UGh0dHA6Ly93d3cubWljcm9zb2Z0LmNvbS9wa2lvcHMv
'' SIG '' Y2VydHMvTWljcm9zb2Z0JTIwVGltZS1TdGFtcCUyMFBD
'' SIG '' QSUyMDIwMTAoMSkuY3J0MAwGA1UdEwEB/wQCMAAwFgYD
'' SIG '' VR0lAQH/BAwwCgYIKwYBBQUHAwgwDgYDVR0PAQH/BAQD
'' SIG '' AgeAMA0GCSqGSIb3DQEBCwUAA4ICAQBh+TwbPOkRWcaX
'' SIG '' vLqhejK0JvjYfHpM4DT52RoEjfp+0MT20u5tRr/ExscH
'' SIG '' mtw2JGEUdn3dF590+lzj4UXQMCXmU/zEoA77b3dFY8oM
'' SIG '' U4UjGC1ljTy3wP1xJCmAZTPLDeURNl5s0sQDXsD8JOkD
'' SIG '' YX26HyPzgrKB4RuP5uJ1YOIR9rKgfYDn/nLAknEi4vMV
'' SIG '' Udpy9bFIIqgX2GVKtlIbl9dZLedqZ/i23r3RRPoAbJYs
'' SIG '' VZ7z3lygU/Gb+bRQgyOOn1VEUfudvc2DZDiA9L0TllMx
'' SIG '' nqcCWZSJwOPQ1cCzbBC5CudidtEAn8NBbfmoujsNrD0C
'' SIG '' wi2qMWFsxwbryANziPvgvYph7/aCgEcvDNKflQN+1LUd
'' SIG '' kjRlGyqY0cjRNm+9RZf1qObpJ8sFMS2hOjqAs5fRQP/2
'' SIG '' uuEaN2SILDhLBTmiwKWCqCI0wrmd2TaDEWUNccLIunmo
'' SIG '' HoGg+lzzZGE7TILOg/2C/vO/YShwBYSyoTn7Raa7m5qu
'' SIG '' Z+9zOIt9TVJjbjQ5lbyV3ixLx+fJuf+MMyYUCFrNXXMf
'' SIG '' RARFYSx8tKnCQ5doiZY0UnmWZyd/VVObpyZ9qxJxi0SW
'' SIG '' mOpn0aigKaTVcUCk5E+z887jchwWY9HBqC3TSJBLD6sF
'' SIG '' 4gfTQpCr4UlP/rZIHvSD2D9HxNLqTpv/C3ZRaGqtb5Dy
'' SIG '' XDpfOB7H9jCCB3EwggVZoAMCAQICEzMAAAAVxedrngKb
'' SIG '' SZkAAAAAABUwDQYJKoZIhvcNAQELBQAwgYgxCzAJBgNV
'' SIG '' BAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYD
'' SIG '' VQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQg
'' SIG '' Q29ycG9yYXRpb24xMjAwBgNVBAMTKU1pY3Jvc29mdCBS
'' SIG '' b290IENlcnRpZmljYXRlIEF1dGhvcml0eSAyMDEwMB4X
'' SIG '' DTIxMDkzMDE4MjIyNVoXDTMwMDkzMDE4MzIyNVowfDEL
'' SIG '' MAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24x
'' SIG '' EDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jv
'' SIG '' c29mdCBDb3Jwb3JhdGlvbjEmMCQGA1UEAxMdTWljcm9z
'' SIG '' b2Z0IFRpbWUtU3RhbXAgUENBIDIwMTAwggIiMA0GCSqG
'' SIG '' SIb3DQEBAQUAA4ICDwAwggIKAoICAQDk4aZM57RyIQt5
'' SIG '' osvXJHm9DtWC0/3unAcH0qlsTnXIyjVX9gF/bErg4r25
'' SIG '' PhdgM/9cT8dm95VTcVrifkpa/rg2Z4VGIwy1jRPPdzLA
'' SIG '' EBjoYH1qUoNEt6aORmsHFPPFdvWGUNzBRMhxXFExN6AK
'' SIG '' OG6N7dcP2CZTfDlhAnrEqv1yaa8dq6z2Nr41JmTamDu6
'' SIG '' GnszrYBbfowQHJ1S/rboYiXcag/PXfT+jlPP1uyFVk3v
'' SIG '' 3byNpOORj7I5LFGc6XBpDco2LXCOMcg1KL3jtIckw+DJ
'' SIG '' j361VI/c+gVVmG1oO5pGve2krnopN6zL64NF50ZuyjLV
'' SIG '' wIYwXE8s4mKyzbnijYjklqwBSru+cakXW2dg3viSkR4d
'' SIG '' Pf0gz3N9QZpGdc3EXzTdEonW/aUgfX782Z5F37ZyL9t9
'' SIG '' X4C626p+Nuw2TPYrbqgSUei/BQOj0XOmTTd0lBw0gg/w
'' SIG '' EPK3Rxjtp+iZfD9M269ewvPV2HM9Q07BMzlMjgK8Qmgu
'' SIG '' EOqEUUbi0b1qGFphAXPKZ6Je1yh2AuIzGHLXpyDwwvoS
'' SIG '' CtdjbwzJNmSLW6CmgyFdXzB0kZSU2LlQ+QuJYfM2BjUY
'' SIG '' hEfb3BvR/bLUHMVr9lxSUV0S2yW6r1AFemzFER1y7435
'' SIG '' UsSFF5PAPBXbGjfHCBUYP3irRbb1Hode2o+eFnJpxq57
'' SIG '' t7c+auIurQIDAQABo4IB3TCCAdkwEgYJKwYBBAGCNxUB
'' SIG '' BAUCAwEAATAjBgkrBgEEAYI3FQIEFgQUKqdS/mTEmr6C
'' SIG '' kTxGNSnPEP8vBO4wHQYDVR0OBBYEFJ+nFV0AXmJdg/Tl
'' SIG '' 0mWnG1M1GelyMFwGA1UdIARVMFMwUQYMKwYBBAGCN0yD
'' SIG '' fQEBMEEwPwYIKwYBBQUHAgEWM2h0dHA6Ly93d3cubWlj
'' SIG '' cm9zb2Z0LmNvbS9wa2lvcHMvRG9jcy9SZXBvc2l0b3J5
'' SIG '' Lmh0bTATBgNVHSUEDDAKBggrBgEFBQcDCDAZBgkrBgEE
'' SIG '' AYI3FAIEDB4KAFMAdQBiAEMAQTALBgNVHQ8EBAMCAYYw
'' SIG '' DwYDVR0TAQH/BAUwAwEB/zAfBgNVHSMEGDAWgBTV9lbL
'' SIG '' j+iiXGJo0T2UkFvXzpoYxDBWBgNVHR8ETzBNMEugSaBH
'' SIG '' hkVodHRwOi8vY3JsLm1pY3Jvc29mdC5jb20vcGtpL2Ny
'' SIG '' bC9wcm9kdWN0cy9NaWNSb29DZXJBdXRfMjAxMC0wNi0y
'' SIG '' My5jcmwwWgYIKwYBBQUHAQEETjBMMEoGCCsGAQUFBzAC
'' SIG '' hj5odHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vcGtpL2Nl
'' SIG '' cnRzL01pY1Jvb0NlckF1dF8yMDEwLTA2LTIzLmNydDAN
'' SIG '' BgkqhkiG9w0BAQsFAAOCAgEAnVV9/Cqt4SwfZwExJFvh
'' SIG '' nnJL/Klv6lwUtj5OR2R4sQaTlz0xM7U518JxNj/aZGx8
'' SIG '' 0HU5bbsPMeTCj/ts0aGUGCLu6WZnOlNN3Zi6th542DYu
'' SIG '' nKmCVgADsAW+iehp4LoJ7nvfam++Kctu2D9IdQHZGN5t
'' SIG '' ggz1bSNU5HhTdSRXud2f8449xvNo32X2pFaq95W2KFUn
'' SIG '' 0CS9QKC/GbYSEhFdPSfgQJY4rPf5KYnDvBewVIVCs/wM
'' SIG '' nosZiefwC2qBwoEZQhlSdYo2wh3DYXMuLGt7bj8sCXgU
'' SIG '' 6ZGyqVvfSaN0DLzskYDSPeZKPmY7T7uG+jIa2Zb0j/aR
'' SIG '' AfbOxnT99kxybxCrdTDFNLB62FD+CljdQDzHVG2dY3RI
'' SIG '' LLFORy3BFARxv2T5JL5zbcqOCb2zAVdJVGTZc9d/HltE
'' SIG '' AY5aGZFrDZ+kKNxnGSgkujhLmm77IVRrakURR6nxt67I
'' SIG '' 6IleT53S0Ex2tVdUCbFpAUR+fKFhbHP+CrvsQWY9af3L
'' SIG '' wUFJfn6Tvsv4O+S3Fb+0zj6lMVGEvL8CwYKiexcdFYmN
'' SIG '' cP7ntdAoGokLjzbaukz5m/8K6TT4JDVnK+ANuOaMmdbh
'' SIG '' IurwJ0I9JZTmdHRbatGePu1+oDEzfbzL6Xu/OHBE0ZDx
'' SIG '' yKs6ijoIYn/ZcGNTTY3ugm2lBRDBcQZqELQdVTNYs6Fw
'' SIG '' ZvKhggNZMIICQQIBATCCAQGhgdmkgdYwgdMxCzAJBgNV
'' SIG '' BAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYD
'' SIG '' VQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQg
'' SIG '' Q29ycG9yYXRpb24xLTArBgNVBAsTJE1pY3Jvc29mdCBJ
'' SIG '' cmVsYW5kIE9wZXJhdGlvbnMgTGltaXRlZDEnMCUGA1UE
'' SIG '' CxMeblNoaWVsZCBUU1MgRVNOOjMyMUEtMDVFMC1EOTQ3
'' SIG '' MSUwIwYDVQQDExxNaWNyb3NvZnQgVGltZS1TdGFtcCBT
'' SIG '' ZXJ2aWNloiMKAQEwBwYFKw4DAhoDFQC2RC395tZJDkOc
'' SIG '' b5opHM8QsIUT0aCBgzCBgKR+MHwxCzAJBgNVBAYTAlVT
'' SIG '' MRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdS
'' SIG '' ZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9y
'' SIG '' YXRpb24xJjAkBgNVBAMTHU1pY3Jvc29mdCBUaW1lLVN0
'' SIG '' YW1wIFBDQSAyMDEwMA0GCSqGSIb3DQEBCwUAAgUA6uLF
'' SIG '' LDAiGA8yMDI0MTExNjA3MTU1NloYDzIwMjQxMTE3MDcx
'' SIG '' NTU2WjB3MD0GCisGAQQBhFkKBAExLzAtMAoCBQDq4sUs
'' SIG '' AgEAMAoCAQACAhMqAgH/MAcCAQACAhSDMAoCBQDq5Bas
'' SIG '' AgEAMDYGCisGAQQBhFkKBAIxKDAmMAwGCisGAQQBhFkK
'' SIG '' AwKgCjAIAgEAAgMHoSChCjAIAgEAAgMBhqAwDQYJKoZI
'' SIG '' hvcNAQELBQADggEBAH6/TcuZuOnlsWG4xkxUSngaHyBN
'' SIG '' qRw8ZhXc54vl0bU9/b0M4Rj68R+zHCqnn0rRc2X6mavS
'' SIG '' Qwtdq1pMdnhVwhiepHM7z7q+1AbkuNg4iCrG6N5NT8i4
'' SIG '' TI2UVMkkQZihdvFFOKQg1dpLVLb9ZU1sXKHNZM/f9Alq
'' SIG '' AmMa7QJfKdDWAbnyqPVGMAsbKztCqo1j5ROLi5IEz7U3
'' SIG '' 91UA9d/9J8/6BThFkRZsPw49ENFeptGWfsTbCkhCVdyN
'' SIG '' lSfdsmk2r55qCJ64/Bi7a5vjDa5z+vOqyjqGMb1tqEjQ
'' SIG '' rl+PvMu0lTdQlK0x4oQrA4lpKE1Gb4iFowf0Cbnw0oO6
'' SIG '' HOp1qE8xggQNMIIECQIBATCBkzB8MQswCQYDVQQGEwJV
'' SIG '' UzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMH
'' SIG '' UmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBv
'' SIG '' cmF0aW9uMSYwJAYDVQQDEx1NaWNyb3NvZnQgVGltZS1T
'' SIG '' dGFtcCBQQ0EgMjAxMAITMwAAAfijoSYMDEBI/gABAAAB
'' SIG '' +DANBglghkgBZQMEAgEFAKCCAUowGgYJKoZIhvcNAQkD
'' SIG '' MQ0GCyqGSIb3DQEJEAEEMC8GCSqGSIb3DQEJBDEiBCB+
'' SIG '' m3iY3VF6sRF/kQb50Tu0+eB1uuFQbWlHYBH98XNIIDCB
'' SIG '' +gYLKoZIhvcNAQkQAi8xgeowgecwgeQwgb0EIO/MM/Jf
'' SIG '' DVSQBQVi3xtHhR2Mz3RC/nGdVqIoPcjRnPdaMIGYMIGA
'' SIG '' pH4wfDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hp
'' SIG '' bmd0b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoT
'' SIG '' FU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEmMCQGA1UEAxMd
'' SIG '' TWljcm9zb2Z0IFRpbWUtU3RhbXAgUENBIDIwMTACEzMA
'' SIG '' AAH4o6EmDAxASP4AAQAAAfgwIgQg1yyDR63xa5MhPTFX
'' SIG '' zEZU7lwW5PvfDtFT+/RqqBTj6qAwDQYJKoZIhvcNAQEL
'' SIG '' BQAEggIAD+9hk92q2/oMCqrMHG5g8hjZayZqalRhTPw2
'' SIG '' Ueu+hp45GZ1bld1GpNskDKT6tOMNYsI5I0e1NpBkX6E0
'' SIG '' Y6IBRuIQKvc6+oOg/YWGiMiCA2u/yzD4X1Mhtu4BP63B
'' SIG '' mfM2wiy+UcW40DheKjYsUsn1KY5n69scmPzmxA9G4B6J
'' SIG '' kyVtjZRFWccJbxO76zUBd2eEw181WbGDJXz4lBhGeC6w
'' SIG '' EbRbKWc71I/VAc7f9rVxs6LV8o1Qi+p35aV64mJBbfZT
'' SIG '' ex3Xu/5/aRhgY/4bBCEw218h8EgpcYYTQhoCu9OUQnBe
'' SIG '' NAVVHGS8zi4t8ei9t7iu/A4SJNu9P6iP80CPJMkuCvuv
'' SIG '' t8Q50SOKrxNNHZBGsSXp1+XE9zraj99/sk/9ws+Jfrsh
'' SIG '' BJfjbjv7okNV/lRUI+ofitSZzmA2dqm1+3LGkyBbUv31
'' SIG '' FkGW2WLklC/oyM59QA9yN8lYIFk2GOVkmCh2e9fEsDRP
'' SIG '' 9c9TAhPdY+deObJDV/KRN+rUufHGPADGWhhAXlOc3ZqA
'' SIG '' irv6sl+Rk6mvDfgiayl8aJviEMToK1bVklRxKB6UTqFb
'' SIG '' WZKw9NqjezHz1ayZBGCLXwyb0pey8mlsrt1YkuLFw+cu
'' SIG '' Lvip6GstyT5JGueaiU0X6UVvTB2dpfnfbCsHdr9flbaR
'' SIG '' ShTTA8ScKC71kAKoBwI1VeObwuuSkPU=
'' SIG '' End signature block
