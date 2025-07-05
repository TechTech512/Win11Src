' Windows Installer utility to manage installer policy settings
' For use with Windows Scripting Host, CScript.exe or WScript.exe
' Copyright (c) Microsoft Corporation. All rights reserved.
' Demonstrates the use of the installer policy keys
' Policy can be configured by an administrator using the NT Group Policy Editor
'
Option Explicit

Dim policies(21, 4)
policies(1, 0)="LM" : policies(1, 1)="HKLM" : policies(1, 2)="Logging"              : policies(1, 3)="REG_SZ"    : policies(1, 4) = "Logging modes if not supplied by install, set of iwearucmpv"
policies(2, 0)="DO" : policies(2, 1)="HKLM" : policies(2, 2)="Debug"                : policies(2, 3)="REG_DWORD" : policies(2, 4) = "OutputDebugString: 1=debug output, 2=verbose debug output, 7=include command line"
policies(3, 0)="DI" : policies(3, 1)="HKLM" : policies(3, 2)="DisableMsi"           : policies(3, 3)="REG_DWORD" : policies(3, 4) = "1=Disable non-managed installs, 2=disable all installs"
policies(4, 0)="WT" : policies(4, 1)="HKLM" : policies(4, 2)="Timeout"              : policies(4, 3)="REG_DWORD" : policies(4, 4) = "Wait timeout in seconds in case of no activity"
policies(5, 0)="DB" : policies(5, 1)="HKLM" : policies(5, 2)="DisableBrowse"        : policies(5, 3)="REG_DWORD" : policies(5, 4) = "Disable user browsing of source locations if 1"
policies(6, 0)="AB" : policies(6, 1)="HKLM" : policies(6, 2)="AllowLockdownBrowse"  : policies(6, 3)="REG_DWORD" : policies(6, 4) = "Allow non-admin users to browse to new sources for managed applications if 1 - use is not recommended"
policies(7, 0)="AM" : policies(7, 1)="HKLM" : policies(7, 2)="AllowLockdownMedia"   : policies(7, 3)="REG_DWORD" : policies(7, 4) = "Allow non-admin users to browse to new media sources for managed applications if 1 - use is not recommended"
policies(8, 0)="AP" : policies(8, 1)="HKLM" : policies(8, 2)="AllowLockdownPatch"   : policies(8, 3)="REG_DWORD" : policies(8, 4) = "Allow non-admin users to apply small and minor update patches to managed applications if 1 - use is not recommended"
policies(9, 0)="DU" : policies(9, 1)="HKLM" : policies(9, 2)="DisableUserInstalls"  : policies(9, 3)="REG_DWORD" : policies(9, 4) = "Disable per-user installs if 1 - available on Windows Installer version 2.0 and later"
policies(10, 0)="DP" : policies(10, 1)="HKLM" : policies(10, 2)="DisablePatch"         : policies(10, 3)="REG_DWORD" : policies(10, 4) = "Disable patch application to all products if 1"
policies(11, 0)="UC" : policies(11, 1)="HKLM" : policies(11, 2)="EnableUserControl"    : policies(11, 3)="REG_DWORD" : policies(11, 4) = "All public properties sent to install service if 1"
policies(12, 0)="ER" : policies(12, 1)="HKLM" : policies(12, 2)="EnableAdminTSRemote"  : policies(12, 3)="REG_DWORD" : policies(12, 4) = "Allow admins to perform installs from terminal server client sessions if 1"
policies(13, 0)="LS" : policies(13, 1)="HKLM" : policies(13, 2)="LimitSystemRestoreCheckpointing" : policies(13, 3)="REG_DWORD" : policies(13, 4) = "Turn off creation of system restore check points on Windows XP if 1 - available on Windows Installer version 2.0 and later"
policies(14, 0)="SS" : policies(14, 1)="HKLM" : policies(14, 2)="SafeForScripting"     : policies(14, 3)="REG_DWORD" : policies(14, 4) = "Do not prompt when scripts within a webpage access Installer automation interface if 1 - use is not recommended"
policies(15, 0)="TP" : policies(15,1)="HKLM" : policies(15, 2)="TransformsSecure"     : policies(15, 3)="REG_DWORD" : policies(15, 4) = "Pin tranforms in secure location if 1 (only admin and system have write privileges to cache location)"
policies(16, 0)="EM" : policies(16, 1)="HKLM" : policies(16, 2)="AlwaysInstallElevated": policies(16, 3)="REG_DWORD" : policies(16, 4) = "System privileges if 1 and HKCU value also set - dangerous policy as non-admin users can install with elevated privileges if enabled"
policies(17, 0)="EU" : policies(17, 1)="HKCU" : policies(17, 2)="AlwaysInstallElevated": policies(17, 3)="REG_DWORD" : policies(17, 4) = "System privileges if 1 and HKLM value also set - dangerous policy as non-admin users can install with elevated privileges if enabled"
policies(18,0)="DR" : policies(18,1)="HKCU" : policies(18,2)="DisableRollback"      : policies(18,3)="REG_DWORD" : policies(18,4) = "Disable rollback if 1 - use is not recommended"
policies(19,0)="TS" : policies(19,1)="HKCU" : policies(19,2)="TransformsAtSource"   : policies(19,3)="REG_DWORD" : policies(19,4) = "Locate transforms at root of source image if 1"
policies(20,0)="SO" : policies(20,1)="HKCU" : policies(20,2)="SearchOrder"          : policies(20,3)="REG_SZ"    : policies(20,4) = "Search order of source types, set of n,m,u (default=nmu)"
policies(21,0)="DM" : policies(21,1)="HKCU" : policies(21,2)="DisableMedia"          : policies(21,3)="REG_DWORD"    : policies(21,4) = "Browsing to media sources is disabled"

Dim argCount:argCount = Wscript.Arguments.Count
Dim message, iPolicy, policyKey, policyValue, WshShell, policyCode
On Error Resume Next

' If no arguments supplied, then list all current policy settings
If argCount = 0 Then
	Set WshShell = WScript.CreateObject("WScript.Shell") : CheckError
	For iPolicy = 0 To UBound(policies)
		policyValue = ReadPolicyValue(iPolicy)
		If Not IsEmpty(policyValue) Then 'policy key present, else skip display
			If Not IsEmpty(message) Then message = message & vbLf
			message = message & policies(iPolicy,0) & ": " & policies(iPolicy,2) & "(" & policies(iPolicy,1) & ") = " & policyValue
		End If
	Next
	If IsEmpty(message) Then message = "No installer policies set"
	Wscript.Echo message
	Wscript.Quit 0
End If

' Check for ?, and show help message if found
policyCode = UCase(Wscript.Arguments(0))
If InStr(1, policyCode, "?", vbTextCompare) <> 0 Then
	message = "Windows Installer utility to manage installer policy settings" &_
		vbLf & " If no arguments are supplied, current policy settings in list will be reported" &_
		vbLf & " The 1st argument specifies the policy to set, using a code from the list below" &_
		vbLf & " The 2nd argument specifies the new policy setting, use """" to remove the policy" &_
		vbLf & " If the 2nd argument is not supplied, the current policy value will be reported"
	For iPolicy = 0 To UBound(policies)
		message = message & vbLf & policies(iPolicy,0) & ": " & policies(iPolicy,2) & "(" & policies(iPolicy,1) & ")  " & policies(iPolicy,4) & vbLf
	Next
	message = message & vblf & vblf & "Copyright (C) Microsoft Corporation.  All rights reserved."

	Wscript.Echo message
	Wscript.Quit 1
End If

' Policy code supplied, look up in array
For iPolicy = 0 To UBound(policies)
	If policies(iPolicy, 0) = policyCode Then Exit For
Next
If iPolicy > UBound(policies) Then Wscript.Echo "Unknown policy code: " & policyCode : Wscript.Quit 2
Set WshShell = WScript.CreateObject("WScript.Shell") : CheckError

' If no value supplied, then simply report current value
policyValue = ReadPolicyValue(iPolicy)
If IsEmpty(policyValue) Then policyValue = "Not present"
message = policies(iPolicy,0) & ": " & policies(iPolicy,2) & "(" & policies(iPolicy,1) & ") = " & policyValue
If argCount > 1 Then ' Value supplied, set policy
	policyValue = WritePolicyValue(iPolicy, Wscript.Arguments(1))
	If IsEmpty(policyValue) Then policyValue = "Not present"
	message = message & " --> " & policyValue
End If
Wscript.Echo message

Function ReadPolicyValue(iPolicy)
	On Error Resume Next
	Dim policyKey:policyKey = policies(iPolicy,1) & "\Software\Policies\Microsoft\Windows\Installer\" & policies(iPolicy,2)
	ReadPolicyValue = WshShell.RegRead(policyKey)
End Function

Function WritePolicyValue(iPolicy, policyValue)
	On Error Resume Next
	Dim policyKey:policyKey = policies(iPolicy,1) & "\Software\Policies\Microsoft\Windows\Installer\" & policies(iPolicy,2)
	If Len(policyValue) Then
		WshShell.RegWrite policyKey, policyValue, policies(iPolicy,3) : CheckError
		WritePolicyValue = policyValue
	ElseIf Not IsEmpty(ReadPolicyValue(iPolicy)) Then
		WshShell.RegDelete policyKey : CheckError
	End If
End Function

Sub CheckError
	Dim message, errRec
	If Err = 0 Then Exit Sub
	message = Err.Source & " " & Hex(Err) & ": " & Err.Description
	If Not installer Is Nothing Then
		Set errRec = installer.LastErrorRecord
		If Not errRec Is Nothing Then message = message & vbLf & errRec.FormatText
	End If
	Wscript.Echo message
	Wscript.Quit 2
End Sub

'' SIG '' Begin signature block
'' SIG '' MIImMQYJKoZIhvcNAQcCoIImIjCCJh4CAQExDzANBglg
'' SIG '' hkgBZQMEAgEFADB3BgorBgEEAYI3AgEEoGkwZzAyBgor
'' SIG '' BgEEAYI3AgEeMCQCAQEEEE7wKRaZJ7VNj+Ws4Q8X66sC
'' SIG '' AQACAQACAQACAQACAQAwMTANBglghkgBZQMEAgEFAAQg
'' SIG '' /P2vh2Oob6dK4LJgJX9q0NCUL4Tr2otbVtqd2lYQZG6g
'' SIG '' ggtnMIIE7zCCA9egAwIBAgITMwAABVfPkN3H0cCIjAAA
'' SIG '' AAAFVzANBgkqhkiG9w0BAQsFADB+MQswCQYDVQQGEwJV
'' SIG '' UzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMH
'' SIG '' UmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBv
'' SIG '' cmF0aW9uMSgwJgYDVQQDEx9NaWNyb3NvZnQgQ29kZSBT
'' SIG '' aWduaW5nIFBDQSAyMDEwMB4XDTIzMTAxOTE5NTExMloX
'' SIG '' DTI0MTAxNjE5NTExMlowdDELMAkGA1UEBhMCVVMxEzAR
'' SIG '' BgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1v
'' SIG '' bmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlv
'' SIG '' bjEeMBwGA1UEAxMVTWljcm9zb2Z0IENvcnBvcmF0aW9u
'' SIG '' MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA
'' SIG '' rNP5BRqxQTyYzc7lY4sbAK2Huz47DGso8p9wEvDxx+0J
'' SIG '' gngiIdoh+jhkos8Hcvx0lOW32XMWZ9uWBMn3+pgUKZad
'' SIG '' OuLXO3LnuVop+5akCowquXhMS3uzPTLONhyePNp74iWb
'' SIG '' 1StajQ3uGOx+fEw00mrTpNGoDeRj/cUHOqKb/TTx2TCt
'' SIG '' 7z32yj/OcNp5pk+8A5Gg1S6DMZhJjZ39s2LVGrsq8fs8
'' SIG '' y1RP3ZBb2irsMamIOUFSTar8asexaAgoNauVnQMqeAdE
'' SIG '' tNScUxT6m/cNfOZjrCItHZO7ieiaDk9ljrCS9QVLldjI
'' SIG '' JhadWdjiAa8JHXgeecBvJhe2s9XVho5OTQIDAQABo4IB
'' SIG '' bjCCAWowHwYDVR0lBBgwFgYKKwYBBAGCNz0GAQYIKwYB
'' SIG '' BQUHAwMwHQYDVR0OBBYEFGVIsKghPtVDZfZAsyDVZjTC
'' SIG '' rXm3MEUGA1UdEQQ+MDykOjA4MR4wHAYDVQQLExVNaWNy
'' SIG '' b3NvZnQgQ29ycG9yYXRpb24xFjAUBgNVBAUTDTIzMDg2
'' SIG '' NSs1MDE1OTcwHwYDVR0jBBgwFoAU5vxfe7siAFjkck61
'' SIG '' 9CF0IzLm76wwVgYDVR0fBE8wTTBLoEmgR4ZFaHR0cDov
'' SIG '' L2NybC5taWNyb3NvZnQuY29tL3BraS9jcmwvcHJvZHVj
'' SIG '' dHMvTWljQ29kU2lnUENBXzIwMTAtMDctMDYuY3JsMFoG
'' SIG '' CCsGAQUFBwEBBE4wTDBKBggrBgEFBQcwAoY+aHR0cDov
'' SIG '' L3d3dy5taWNyb3NvZnQuY29tL3BraS9jZXJ0cy9NaWND
'' SIG '' b2RTaWdQQ0FfMjAxMC0wNy0wNi5jcnQwDAYDVR0TAQH/
'' SIG '' BAIwADANBgkqhkiG9w0BAQsFAAOCAQEAyi7DQuZQIWdC
'' SIG '' y9r24eaW4WAzNYbRIN/nYv+fHw77U3E/qC8KvnkT7iJX
'' SIG '' lGit+3mhHspwiQO1r3SSvRY72QQuBW5KoS7upUqqZVFH
'' SIG '' ic8Z+ttKnH7pfqYXFLM0GA8gLIeH43U8ybcdoxnoiXA9
'' SIG '' fd8iKCM4za5ZVwrRlTEo68sto4lOKXM6dVjo1qwi/X89
'' SIG '' Gb0fNdWGQJ4cj+s7tVfKXWKngOuvISr3X2c1aetBfGZK
'' SIG '' p7nDqWtViokBGBMJBubzkHcaDsWVnPjCenJnDYAPu0ny
'' SIG '' W29F1/obCiMyu02/xPXRCxfPOe97LWPgLrgKb2SwLBu+
'' SIG '' mlP476pcq3lFl+TN7ltkoTCCBnAwggRYoAMCAQICCmEM
'' SIG '' UkwAAAAAAAMwDQYJKoZIhvcNAQELBQAwgYgxCzAJBgNV
'' SIG '' BAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYD
'' SIG '' VQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQg
'' SIG '' Q29ycG9yYXRpb24xMjAwBgNVBAMTKU1pY3Jvc29mdCBS
'' SIG '' b290IENlcnRpZmljYXRlIEF1dGhvcml0eSAyMDEwMB4X
'' SIG '' DTEwMDcwNjIwNDAxN1oXDTI1MDcwNjIwNTAxN1owfjEL
'' SIG '' MAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24x
'' SIG '' EDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jv
'' SIG '' c29mdCBDb3Jwb3JhdGlvbjEoMCYGA1UEAxMfTWljcm9z
'' SIG '' b2Z0IENvZGUgU2lnbmluZyBQQ0EgMjAxMDCCASIwDQYJ
'' SIG '' KoZIhvcNAQEBBQADggEPADCCAQoCggEBAOkOZFB5Z7XE
'' SIG '' 4/0JAEyelKz3VmjqRNjPxVhPqaV2fG1FutM5krSkHvn5
'' SIG '' ZYLkF9KP/UScCOhlk84sVYS/fQjjLiuoQSsYt6JLbklM
'' SIG '' axUH3tHSwokecZTNtX9LtK8I2MyI1msXlDqTziY/7Ob+
'' SIG '' NJhX1R1dSfayKi7VhbtZP/iQtCuDdMorsztG4/BGScEX
'' SIG '' ZlTJHL0dxFViV3L4Z7klIDTeXaallV6rKIDN1bKe5QO1
'' SIG '' Y9OyFMjByIomCll/B+z/Du2AEjVMEqa+Ulv1ptrgiwtI
'' SIG '' d9aFR9UQucboqu6Lai0FXGDGtCpbnCMcX0XjGhQebzfL
'' SIG '' GTOAaolNo2pmY3iT1TDPlR8CAwEAAaOCAeMwggHfMBAG
'' SIG '' CSsGAQQBgjcVAQQDAgEAMB0GA1UdDgQWBBTm/F97uyIA
'' SIG '' WORyTrX0IXQjMubvrDAZBgkrBgEEAYI3FAIEDB4KAFMA
'' SIG '' dQBiAEMAQTALBgNVHQ8EBAMCAYYwDwYDVR0TAQH/BAUw
'' SIG '' AwEB/zAfBgNVHSMEGDAWgBTV9lbLj+iiXGJo0T2UkFvX
'' SIG '' zpoYxDBWBgNVHR8ETzBNMEugSaBHhkVodHRwOi8vY3Js
'' SIG '' Lm1pY3Jvc29mdC5jb20vcGtpL2NybC9wcm9kdWN0cy9N
'' SIG '' aWNSb29DZXJBdXRfMjAxMC0wNi0yMy5jcmwwWgYIKwYB
'' SIG '' BQUHAQEETjBMMEoGCCsGAQUFBzAChj5odHRwOi8vd3d3
'' SIG '' Lm1pY3Jvc29mdC5jb20vcGtpL2NlcnRzL01pY1Jvb0Nl
'' SIG '' ckF1dF8yMDEwLTA2LTIzLmNydDCBnQYDVR0gBIGVMIGS
'' SIG '' MIGPBgkrBgEEAYI3LgMwgYEwPQYIKwYBBQUHAgEWMWh0
'' SIG '' dHA6Ly93d3cubWljcm9zb2Z0LmNvbS9QS0kvZG9jcy9D
'' SIG '' UFMvZGVmYXVsdC5odG0wQAYIKwYBBQUHAgIwNB4yIB0A
'' SIG '' TABlAGcAYQBsAF8AUABvAGwAaQBjAHkAXwBTAHQAYQB0
'' SIG '' AGUAbQBlAG4AdAAuIB0wDQYJKoZIhvcNAQELBQADggIB
'' SIG '' ABp071dPKXvEFoV4uFDTIvwJnayCl/g0/yosl5US5eS/
'' SIG '' z7+TyOM0qduBuNweAL7SNW+v5X95lXflAtTx69jNTh4b
'' SIG '' YaLCWiMa8IyoYlFFZwjjPzwek/gwhRfIOUCm1w6zISnl
'' SIG '' paFpjCKTzHSY56FHQ/JTrMAPMGl//tIlIG1vYdPfB9XZ
'' SIG '' cgAsaYZ2PVHbpjlIyTdhbQfdUxnLp9Zhwr/ig6sP4Gub
'' SIG '' ldZ9KFGwiUpRpJpsyLcfShoOaanX3MF+0Ulwqratu3JH
'' SIG '' Yxf6ptaipobsqBBEm2O2smmJBsdGhnoYP+jFHSHVe/kC
'' SIG '' Iy3FQcu/HUzIFu+xnH/8IktJim4V46Z/dlvRU3mRhZ3V
'' SIG '' 0ts9czXzPK5UslJHasCqE5XSjhHamWdeMoz7N4XR3HWF
'' SIG '' nIfGWleFwr/dDY+Mmy3rtO7PJ9O1Xmn6pBYEAackZ3PP
'' SIG '' TU+23gVWl3r36VJN9HcFT4XG2Avxju1CCdENduMjVngi
'' SIG '' Jja+yrGMbqod5IXaRzNij6TJkTNfcR5Ar5hlySLoQiEl
'' SIG '' ihwtYNk3iUGJKhYP12E8lGhgUu/WR5mggEDuFYF3Ppzg
'' SIG '' UxgaUB04lZseZjMTJzkXeIc2zk7DX7L1PUdTtuDl2wth
'' SIG '' PSrXkizON1o+QEIxpB8QCMJWnL8kXVECnWp50hfT2sGU
'' SIG '' jgd7JXFEqwZq5tTG3yOalnXFMYIaIjCCGh4CAQEwgZUw
'' SIG '' fjELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0
'' SIG '' b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1p
'' SIG '' Y3Jvc29mdCBDb3Jwb3JhdGlvbjEoMCYGA1UEAxMfTWlj
'' SIG '' cm9zb2Z0IENvZGUgU2lnbmluZyBQQ0EgMjAxMAITMwAA
'' SIG '' BVfPkN3H0cCIjAAAAAAFVzANBglghkgBZQMEAgEFAKCB
'' SIG '' xjAZBgkqhkiG9w0BCQMxDAYKKwYBBAGCNwIBBDAcBgor
'' SIG '' BgEEAYI3AgELMQ4wDAYKKwYBBAGCNwIBFTAvBgkqhkiG
'' SIG '' 9w0BCQQxIgQgG74cd1WwHhi9o902FBuqooOIxL5Kiicy
'' SIG '' nZhgW/cwOaswWgYKKwYBBAGCNwIBDDFMMEqgJIAiAE0A
'' SIG '' aQBjAHIAbwBzAG8AZgB0ACAAVwBpAG4AZABvAHcAc6Ei
'' SIG '' gCBodHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vd2luZG93
'' SIG '' czANBgkqhkiG9w0BAQEFAASCAQAkDhlpEPAjJpYNVAoq
'' SIG '' eyDK/OKWeK6cm2UufctahILOKbZnVh+JbuZlwZ5Iz8Xr
'' SIG '' VXLstO8d2MB+2TyvqnU2fYoIqmwp5resPFXMozesDv5d
'' SIG '' B5Os1QN75tURbuFbgZFMAbf8Dd0o99uwrkT6egrSVfAM
'' SIG '' FKNmfhgUkq27md7f+/5Kr7QzzXC/f8I/BsF+++L+4giX
'' SIG '' OuEUAJN9Igg9y3GPqhRmoGkRgA81Rkmf2AbmKeh0z8Uz
'' SIG '' FUsS/JgGNwWtoqDs+JnbRYeTBfQMFnFRVT9Jsohc+aFI
'' SIG '' HbabkeX7x+5zhNGNadb4k7wFCp2FbjyvBUsLW0yNso85
'' SIG '' 05DQPfL614UkgdlFoYIXlDCCF5AGCisGAQQBgjcDAwEx
'' SIG '' gheAMIIXfAYJKoZIhvcNAQcCoIIXbTCCF2kCAQMxDzAN
'' SIG '' BglghkgBZQMEAgEFADCCAVIGCyqGSIb3DQEJEAEEoIIB
'' SIG '' QQSCAT0wggE5AgEBBgorBgEEAYRZCgMBMDEwDQYJYIZI
'' SIG '' AWUDBAIBBQAEIJWuqriJI6F5GIdqxvwHnU7KN8qL4vqq
'' SIG '' kgexcwtd//XKAgZmvgePKicYEzIwMjQwOTA1MDkxNzA1
'' SIG '' LjQ5NVowBIACAfSggdGkgc4wgcsxCzAJBgNVBAYTAlVT
'' SIG '' MRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdS
'' SIG '' ZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9y
'' SIG '' YXRpb24xJTAjBgNVBAsTHE1pY3Jvc29mdCBBbWVyaWNh
'' SIG '' IE9wZXJhdGlvbnMxJzAlBgNVBAsTHm5TaGllbGQgVFNT
'' SIG '' IEVTTjo4NjAzLTA1RTAtRDk0NzElMCMGA1UEAxMcTWlj
'' SIG '' cm9zb2Z0IFRpbWUtU3RhbXAgU2VydmljZaCCEeowggcg
'' SIG '' MIIFCKADAgECAhMzAAAB8bNF9SfowBbWAAEAAAHxMA0G
'' SIG '' CSqGSIb3DQEBCwUAMHwxCzAJBgNVBAYTAlVTMRMwEQYD
'' SIG '' VQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25k
'' SIG '' MR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24x
'' SIG '' JjAkBgNVBAMTHU1pY3Jvc29mdCBUaW1lLVN0YW1wIFBD
'' SIG '' QSAyMDEwMB4XDTIzMTIwNjE4NDU1NVoXDTI1MDMwNTE4
'' SIG '' NDU1NVowgcsxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpX
'' SIG '' YXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYD
'' SIG '' VQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xJTAjBgNV
'' SIG '' BAsTHE1pY3Jvc29mdCBBbWVyaWNhIE9wZXJhdGlvbnMx
'' SIG '' JzAlBgNVBAsTHm5TaGllbGQgVFNTIEVTTjo4NjAzLTA1
'' SIG '' RTAtRDk0NzElMCMGA1UEAxMcTWljcm9zb2Z0IFRpbWUt
'' SIG '' U3RhbXAgU2VydmljZTCCAiIwDQYJKoZIhvcNAQEBBQAD
'' SIG '' ggIPADCCAgoCggIBALG6UJm20h/xf3utb38n5DhWD0+K
'' SIG '' 6AHXJrX8NHHEtbaHDLhCC1TePl9XvlkprpdNNCFbkKWQ
'' SIG '' aXqCnWd3lUGzHglv6hTg+wwDZ+h7yA/1tA09XEgcwm7p
'' SIG '' Nhyuuff0d1163bGR2pSHPPJJdo8WoUyTZWJ8R+P4dHom
'' SIG '' F42zYsvObwUMmb6kF108MtqD9H4A8hYfJ+2r2K3AzRY/
'' SIG '' lnR19DIjhaVV5RL6+i2w9tab5EqwfgVA2HNvS38PiK61
'' SIG '' x8Irf8sr7EuZLp2YCHsAwq4RSXyLaR1YENFxz4lZrbVI
'' SIG '' J5/HlI+EkQWBiF0Y8CincbWXxPfdyqtsu1wUmrDDhNCJ
'' SIG '' iIKR3KwJycgXRmpI0Adx8j1IC/eB+TLGpA0knexOyDkY
'' SIG '' 9EX3maqBt9BuQWdTXuJhtEg8mrCBIuHIHzfdkOCbPFsq
'' SIG '' YmZ0NptvNLTIaGeAdrr6DBVo5Spwd/3DqTDEyj46obdB
'' SIG '' khzB3nAcQKzmsAlno8jIUzsB3aFFQUdFOLfncjtXjESB
'' SIG '' ga5lvqoXHo9/jiLsCNdum1SiUNxXNgR2AtBJaK4VqNLp
'' SIG '' eDeTsLLxOIzkc9Qr0tkieWhPG5QtLEmYnudONSM6PnHB
'' SIG '' GYLvHZL+bGqXye8dII3U4QPb/AQI6i3owR71svefOgrA
'' SIG '' 7xM2URK2rmxx3bkYDSAxA76o1dX/FMM4FMnzMFwZAgMB
'' SIG '' AAGjggFJMIIBRTAdBgNVHQ4EFgQUvLbF7n2wITRKPJyo
'' SIG '' TkStvhitLWAwHwYDVR0jBBgwFoAUn6cVXQBeYl2D9OXS
'' SIG '' ZacbUzUZ6XIwXwYDVR0fBFgwVjBUoFKgUIZOaHR0cDov
'' SIG '' L3d3dy5taWNyb3NvZnQuY29tL3BraW9wcy9jcmwvTWlj
'' SIG '' cm9zb2Z0JTIwVGltZS1TdGFtcCUyMFBDQSUyMDIwMTAo
'' SIG '' MSkuY3JsMGwGCCsGAQUFBwEBBGAwXjBcBggrBgEFBQcw
'' SIG '' AoZQaHR0cDovL3d3dy5taWNyb3NvZnQuY29tL3BraW9w
'' SIG '' cy9jZXJ0cy9NaWNyb3NvZnQlMjBUaW1lLVN0YW1wJTIw
'' SIG '' UENBJTIwMjAxMCgxKS5jcnQwDAYDVR0TAQH/BAIwADAW
'' SIG '' BgNVHSUBAf8EDDAKBggrBgEFBQcDCDAOBgNVHQ8BAf8E
'' SIG '' BAMCB4AwDQYJKoZIhvcNAQELBQADggIBAOFISNIEVIJs
'' SIG '' nKXdT9CYUxbZ4s8GSeeWx8gP/uBMy8A0SeGrTwj0cdtu
'' SIG '' qLCoMQdK8BG8q0vuPTOcgJgFsytVKa+APFTyMAaozKIu
'' SIG '' gzzTvzxKjf5PohlX/9RlEmoGXigzdsIhCAUajRVN5DpH
'' SIG '' Ngv63XMJReaak+YzjFxJxUUBNePlPHsHLhKFZQLtWGbu
'' SIG '' mJwOJTmKAaO6K9GHE+9ul+VuH9uyITm3Hly44kQlIb65
'' SIG '' ZyoHJHtMLhwa+5q8dKOTWJFdP9CNo4R4mg6d96xs528m
'' SIG '' sl1ub6V5gtEjrs3dx3wH+y5TbW1F2DA6dOTaE65kqz+Q
'' SIG '' vBpfo2wBtTL2kqwOZPKhacabJNYE+JNvaunmiCjxjyEx
'' SIG '' TVhCzusdHmGqKUSrzyMX70fwpxxv/WKyYlMacGdEy/rx
'' SIG '' R3aXksWE5nidG2XiUeuL43UvwQGDtoTwS897wJr2DPyy
'' SIG '' HYXgI5Nh3U8dx7W6Au+9ZbX5o5Kl3w2fASJ3jOAPv1lD
'' SIG '' GKwmrI7iUxYzMCAR4WFSbjQWyG3Ne50CxfkugKKXists
'' SIG '' d/Bi0Y6nD0NVfeNcBX3S0b2JFtyqO23e+Fb1P4vd8BmU
'' SIG '' x6tpZ+Ht5SY+W0xTyURA4x6Wj/V6GQgY7thk4fFSp4qm
'' SIG '' YX1BpbwtdNPT3QAdniTqD612lkV8Iyi3Ib4Theo3pla0
'' SIG '' oQFCITfEvbsEMIIHcTCCBVmgAwIBAgITMwAAABXF52ue
'' SIG '' AptJmQAAAAAAFTANBgkqhkiG9w0BAQsFADCBiDELMAkG
'' SIG '' A1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAO
'' SIG '' BgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29m
'' SIG '' dCBDb3Jwb3JhdGlvbjEyMDAGA1UEAxMpTWljcm9zb2Z0
'' SIG '' IFJvb3QgQ2VydGlmaWNhdGUgQXV0aG9yaXR5IDIwMTAw
'' SIG '' HhcNMjEwOTMwMTgyMjI1WhcNMzAwOTMwMTgzMjI1WjB8
'' SIG '' MQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2FzaGluZ3Rv
'' SIG '' bjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UEChMVTWlj
'' SIG '' cm9zb2Z0IENvcnBvcmF0aW9uMSYwJAYDVQQDEx1NaWNy
'' SIG '' b3NvZnQgVGltZS1TdGFtcCBQQ0EgMjAxMDCCAiIwDQYJ
'' SIG '' KoZIhvcNAQEBBQADggIPADCCAgoCggIBAOThpkzntHIh
'' SIG '' C3miy9ckeb0O1YLT/e6cBwfSqWxOdcjKNVf2AX9sSuDi
'' SIG '' vbk+F2Az/1xPx2b3lVNxWuJ+Slr+uDZnhUYjDLWNE893
'' SIG '' MsAQGOhgfWpSg0S3po5GawcU88V29YZQ3MFEyHFcUTE3
'' SIG '' oAo4bo3t1w/YJlN8OWECesSq/XJprx2rrPY2vjUmZNqY
'' SIG '' O7oaezOtgFt+jBAcnVL+tuhiJdxqD89d9P6OU8/W7IVW
'' SIG '' Te/dvI2k45GPsjksUZzpcGkNyjYtcI4xyDUoveO0hyTD
'' SIG '' 4MmPfrVUj9z6BVWYbWg7mka97aSueik3rMvrg0XnRm7K
'' SIG '' MtXAhjBcTyziYrLNueKNiOSWrAFKu75xqRdbZ2De+JKR
'' SIG '' Hh09/SDPc31BmkZ1zcRfNN0Sidb9pSB9fvzZnkXftnIv
'' SIG '' 231fgLrbqn427DZM9ituqBJR6L8FA6PRc6ZNN3SUHDSC
'' SIG '' D/AQ8rdHGO2n6Jl8P0zbr17C89XYcz1DTsEzOUyOArxC
'' SIG '' aC4Q6oRRRuLRvWoYWmEBc8pnol7XKHYC4jMYctenIPDC
'' SIG '' +hIK12NvDMk2ZItboKaDIV1fMHSRlJTYuVD5C4lh8zYG
'' SIG '' NRiER9vcG9H9stQcxWv2XFJRXRLbJbqvUAV6bMURHXLv
'' SIG '' jflSxIUXk8A8FdsaN8cIFRg/eKtFtvUeh17aj54WcmnG
'' SIG '' rnu3tz5q4i6tAgMBAAGjggHdMIIB2TASBgkrBgEEAYI3
'' SIG '' FQEEBQIDAQABMCMGCSsGAQQBgjcVAgQWBBQqp1L+ZMSa
'' SIG '' voKRPEY1Kc8Q/y8E7jAdBgNVHQ4EFgQUn6cVXQBeYl2D
'' SIG '' 9OXSZacbUzUZ6XIwXAYDVR0gBFUwUzBRBgwrBgEEAYI3
'' SIG '' TIN9AQEwQTA/BggrBgEFBQcCARYzaHR0cDovL3d3dy5t
'' SIG '' aWNyb3NvZnQuY29tL3BraW9wcy9Eb2NzL1JlcG9zaXRv
'' SIG '' cnkuaHRtMBMGA1UdJQQMMAoGCCsGAQUFBwMIMBkGCSsG
'' SIG '' AQQBgjcUAgQMHgoAUwB1AGIAQwBBMAsGA1UdDwQEAwIB
'' SIG '' hjAPBgNVHRMBAf8EBTADAQH/MB8GA1UdIwQYMBaAFNX2
'' SIG '' VsuP6KJcYmjRPZSQW9fOmhjEMFYGA1UdHwRPME0wS6BJ
'' SIG '' oEeGRWh0dHA6Ly9jcmwubWljcm9zb2Z0LmNvbS9wa2kv
'' SIG '' Y3JsL3Byb2R1Y3RzL01pY1Jvb0NlckF1dF8yMDEwLTA2
'' SIG '' LTIzLmNybDBaBggrBgEFBQcBAQROMEwwSgYIKwYBBQUH
'' SIG '' MAKGPmh0dHA6Ly93d3cubWljcm9zb2Z0LmNvbS9wa2kv
'' SIG '' Y2VydHMvTWljUm9vQ2VyQXV0XzIwMTAtMDYtMjMuY3J0
'' SIG '' MA0GCSqGSIb3DQEBCwUAA4ICAQCdVX38Kq3hLB9nATEk
'' SIG '' W+Geckv8qW/qXBS2Pk5HZHixBpOXPTEztTnXwnE2P9pk
'' SIG '' bHzQdTltuw8x5MKP+2zRoZQYIu7pZmc6U03dmLq2HnjY
'' SIG '' Ni6cqYJWAAOwBb6J6Gngugnue99qb74py27YP0h1AdkY
'' SIG '' 3m2CDPVtI1TkeFN1JFe53Z/zjj3G82jfZfakVqr3lbYo
'' SIG '' VSfQJL1AoL8ZthISEV09J+BAljis9/kpicO8F7BUhUKz
'' SIG '' /AyeixmJ5/ALaoHCgRlCGVJ1ijbCHcNhcy4sa3tuPywJ
'' SIG '' eBTpkbKpW99Jo3QMvOyRgNI95ko+ZjtPu4b6MhrZlvSP
'' SIG '' 9pEB9s7GdP32THJvEKt1MMU0sHrYUP4KWN1APMdUbZ1j
'' SIG '' dEgssU5HLcEUBHG/ZPkkvnNtyo4JvbMBV0lUZNlz138e
'' SIG '' W0QBjloZkWsNn6Qo3GcZKCS6OEuabvshVGtqRRFHqfG3
'' SIG '' rsjoiV5PndLQTHa1V1QJsWkBRH58oWFsc/4Ku+xBZj1p
'' SIG '' /cvBQUl+fpO+y/g75LcVv7TOPqUxUYS8vwLBgqJ7Fx0V
'' SIG '' iY1w/ue10CgaiQuPNtq6TPmb/wrpNPgkNWcr4A245oyZ
'' SIG '' 1uEi6vAnQj0llOZ0dFtq0Z4+7X6gMTN9vMvpe784cETR
'' SIG '' kPHIqzqKOghif9lwY1NNje6CbaUFEMFxBmoQtB1VM1iz
'' SIG '' oXBm8qGCA00wggI1AgEBMIH5oYHRpIHOMIHLMQswCQYD
'' SIG '' VQQGEwJVUzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4G
'' SIG '' A1UEBxMHUmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0
'' SIG '' IENvcnBvcmF0aW9uMSUwIwYDVQQLExxNaWNyb3NvZnQg
'' SIG '' QW1lcmljYSBPcGVyYXRpb25zMScwJQYDVQQLEx5uU2hp
'' SIG '' ZWxkIFRTUyBFU046ODYwMy0wNUUwLUQ5NDcxJTAjBgNV
'' SIG '' BAMTHE1pY3Jvc29mdCBUaW1lLVN0YW1wIFNlcnZpY2Wi
'' SIG '' IwoBATAHBgUrDgMCGgMVAPufsGTiCwza1tT+L4zcG1Gc
'' SIG '' uPT3oIGDMIGApH4wfDELMAkGA1UEBhMCVVMxEzARBgNV
'' SIG '' BAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQx
'' SIG '' HjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEm
'' SIG '' MCQGA1UEAxMdTWljcm9zb2Z0IFRpbWUtU3RhbXAgUENB
'' SIG '' IDIwMTAwDQYJKoZIhvcNAQELBQACBQDqg4rmMCIYDzIw
'' SIG '' MjQwOTA1MDE0MTU4WhgPMjAyNDA5MDYwMTQxNThaMHQw
'' SIG '' OgYKKwYBBAGEWQoEATEsMCowCgIFAOqDiuYCAQAwBwIB
'' SIG '' AAICHtYwBwIBAAICE5MwCgIFAOqE3GYCAQAwNgYKKwYB
'' SIG '' BAGEWQoEAjEoMCYwDAYKKwYBBAGEWQoDAqAKMAgCAQAC
'' SIG '' AwehIKEKMAgCAQACAwGGoDANBgkqhkiG9w0BAQsFAAOC
'' SIG '' AQEA1Gs9xYKje43+LQfFCuatIlCFMPm8UyHHfqx7f7xA
'' SIG '' NpB1QVDlcaR0OE+i3Yg5SjQ+1v1DBBCmuQeFsDeFxCcb
'' SIG '' MZtBTlBScj9xwEggIn1FGdkyNXnLtrmQ07v3wuly+6PC
'' SIG '' oo+Lm0P4e0z/OGch8cMM3ywUkrWQvcALWCU1kCDAXyH9
'' SIG '' +9BGAf0v5vy5FYoElLnOIb372nk5L2Yd0eDmsX3ujOG7
'' SIG '' 0xSFtoB33FrfB6JiNpSyjrNc1DFtsHCrQ3TmwbfmLqzA
'' SIG '' 3ZPqjvlTSs5SZpWUh5dFTG/hVK1s3k/sY9YpwOaXjxEI
'' SIG '' J9in/oH2G8Kclrjy0rUCiBn1xlhWcH1y0yjfZDGCBA0w
'' SIG '' ggQJAgEBMIGTMHwxCzAJBgNVBAYTAlVTMRMwEQYDVQQI
'' SIG '' EwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4w
'' SIG '' HAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xJjAk
'' SIG '' BgNVBAMTHU1pY3Jvc29mdCBUaW1lLVN0YW1wIFBDQSAy
'' SIG '' MDEwAhMzAAAB8bNF9SfowBbWAAEAAAHxMA0GCWCGSAFl
'' SIG '' AwQCAQUAoIIBSjAaBgkqhkiG9w0BCQMxDQYLKoZIhvcN
'' SIG '' AQkQAQQwLwYJKoZIhvcNAQkEMSIEIPiQLAmQ6GIu8eX3
'' SIG '' ZZJhzPH+ItgEtbnnMw6qYZ+4+pAaMIH6BgsqhkiG9w0B
'' SIG '' CRACLzGB6jCB5zCB5DCBvQQg1Xf9PmFLuKPBqjjrpGiw
'' SIG '' HvDASJu3RrU/kSojASP2EXgwgZgwgYCkfjB8MQswCQYD
'' SIG '' VQQGEwJVUzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4G
'' SIG '' A1UEBxMHUmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0
'' SIG '' IENvcnBvcmF0aW9uMSYwJAYDVQQDEx1NaWNyb3NvZnQg
'' SIG '' VGltZS1TdGFtcCBQQ0EgMjAxMAITMwAAAfGzRfUn6MAW
'' SIG '' 1gABAAAB8TAiBCAQO+EVtww9RAEqOubnVoHyMlMhM/IE
'' SIG '' K12/dx7RbBSsfjANBgkqhkiG9w0BAQsFAASCAgB/A2Qz
'' SIG '' r0PTAK3WL0tAIT1MD3v+4YcsHwmutkClWIXs0TE0RS55
'' SIG '' excHMFlDnElOpWJnkfz0XtYUXefDpjkSiP1nNsU5GooA
'' SIG '' A7NwWvPw1qaE0a2PSnIsaxZTITdTdMI9d1abujaEDUI6
'' SIG '' 42YGoHp/PMiRawiT6BNCdO6gcfU9kikWvT09jdkLFrgR
'' SIG '' ljygd7zSWIOJ6Gd05WUGoz6j9pJZhd/Ped1U5M/rrIt3
'' SIG '' Z77DmBV4ww04I0bxzFfnLs5/lXMHVYyoXqIAFhzFaTIf
'' SIG '' qCKm/uiV81DQ+c/xZie5nojuc9aCS0akjgwqMbv2DfiL
'' SIG '' xCp/O2zm2hQQLJ1i9p6ae+ZMGQ7RW1euQimUfNyzB+bC
'' SIG '' gORQaxixA0bLMJsNLc7r9eixcp4ueuXJ03IwKoNPiSHY
'' SIG '' b/fxFj+G0JXv1OEvSGfTmOmhQKuqjxAv+1XkvGQbF7+S
'' SIG '' 3/ioXVaf4BATCh940xmShdVhn+vFa2mXxz6FtdDJwvVT
'' SIG '' ArPf49FS1/vpq54kmbpMQM90q4aC/5jrqIferwyFdHOH
'' SIG '' PuiCP3l99a+P4UcKvQ9FNRVZlXYapCba3rF4o0ncBchl
'' SIG '' NFmNhLOqCIGE2TkBclp+UV+5th6sRTrrVnbv6GTO2yau
'' SIG '' nLggf7+tf8upOQiYSyoTJGmDjS2NGmTyst30u4xuJ6oR
'' SIG '' bQY+Im9B7WRnZU1qBA==
'' SIG '' End signature block
