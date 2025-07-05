' Windows Installer utility to manage the summary information stream
' For use with Windows Scripting Host, CScript.exe or WScript.exe
' Copyright (c) Microsoft Corporation. All rights reserved.
' Demonstrates the use of the database summary information methods

Option Explicit

Const msiOpenDatabaseModeReadOnly     = 0
Const msiOpenDatabaseModeTransact     = 1
Const msiOpenDatabaseModeCreate       = 3

Dim propList(19, 1)
propList( 1,0) = "Codepage"    : propList( 1,1) = "ANSI codepage of text strings in summary information only"
propList( 2,0) = "Title"       : propList( 2,1) = "Package type, e.g. Installation Database"
propList( 3,0) = "Subject"     : propList( 3,1) = "Product full name or description"
propList( 4,0) = "Author"      : propList( 4,1) = "Creator, typically vendor name"
propList( 5,0) = "Keywords"    : propList( 5,1) = "List of keywords for use by file browsers"
propList( 6,0) = "Comments"    : propList( 6,1) = "Description of purpose or use of package"
propList( 7,0) = "Template"    : propList( 7,1) = "Target system: Platform(s);Language(s)"
propList( 8,0) = "LastAuthor"  : propList( 8,1) = "Used for transforms only: New target: Platform(s);Language(s)"
propList( 9,0) = "Revision"    : propList( 9,1) = "Package code GUID, for transforms contains old and new info"
propList(11,0) = "Printed"     : propList(11,1) = "Date and time of installation image, same as Created if CD"
propList(12,0) = "Created"     : propList(12,1) = "Date and time of package creation"
propList(13,0) = "Saved"       : propList(13,1) = "Date and time of last package modification"
propList(14,0) = "Pages"       : propList(14,1) = "Minimum Windows Installer version required: Major * 100 + Minor"
propList(15,0) = "Words"       : propList(15,1) = "Source and Elevation flags: 1=short names, 2=compressed, 4=network image, 8=LUA package"
propList(16,0) = "Characters"  : propList(16,1) = "Used for transforms only: validation and error flags"
propList(18,0) = "Application" : propList(18,1) = "Application associated with file, ""Windows Installer"" for MSI"
propList(19,0) = "Security"    : propList(19,1) = "0=Read/write 2=Readonly recommended 4=Readonly enforced"

Dim iArg, iProp, property, value, message
Dim argCount:argCount = Wscript.Arguments.Count
If argCount > 0 Then If InStr(1, Wscript.Arguments(0), "?", vbTextCompare) > 0 Then argCount = 0
If (argCount = 0) Then
	message = "Windows Installer utility to manage summary information stream" &_
		vbNewLine & " 1st argument is the path to the storage file (installer package)" &_
		vbNewLine & " If no other arguments are supplied, summary properties will be listed" &_
		vbNewLine & " Subsequent arguments are property=value pairs to be updated" &_
		vbNewLine & " Either the numeric or the names below may be used for the property" &_
		vbNewLine & " Date and time fields use current locale format, or ""Now"" or ""Date""" &_
		vbNewLine & " Some properties have specific meaning for installer packages"
	For iProp = 1 To UBound(propList)
		property = propList(iProp, 0)
		If Not IsEmpty(property) Then
			message = message & vbNewLine & Right(" " & iProp, 2) & "  " & property & " - " & propLIst(iProp, 1)
		End If
	Next
	message = message & vbNewLine & vbNewLine & "Copyright (C) Microsoft Corporation.  All rights reserved."

	Wscript.Echo message
	Wscript.Quit 1
End If

' Connect to Windows Installer object
On Error Resume Next
Dim installer : Set installer = Nothing
Set installer = Wscript.CreateObject("WindowsInstaller.Installer") : If CheckError("MSI.DLL not registered") Then Wscript.Quit 2

' Evaluate command-line arguments and open summary information
Dim cUpdate:cUpdate = 0 : If argCount > 1 Then cUpdate = 20
Dim sumInfo  : Set sumInfo = installer.SummaryInformation(Wscript.Arguments(0), cUpdate) : If CheckError(Empty) Then Wscript.Quit 2

' If only package name supplied, then list all properties in summary information stream
If argCount = 1 Then
	For iProp = 1 to UBound(propList)
		value = sumInfo.Property(iProp) : CheckError(Empty)
		If Not IsEmpty(value) Then message = message & vbNewLine & Right(" " & iProp, 2) & "  " &  propList(iProp, 0) & " = " & value
	Next
	Wscript.Echo message
	Wscript.Quit 0
End If

' Process property settings, combining arguments if equal sign has spaces before or after it
For iArg = 1 To argCount - 1
	property = property & Wscript.Arguments(iArg)
	Dim iEquals:iEquals = InStr(1, property, "=", vbTextCompare) 'Must contain an equals sign followed by a value
	If iEquals > 0 And iEquals <> Len(property) Then
		value = Right(property, Len(property) - iEquals)
		property = Left(property, iEquals - 1)
		If IsNumeric(property) Then
			iProp = CLng(property)
		Else  ' Lookup property name if numeric property ID not supplied
			For iProp = 1 To UBound(propList)
				If propList(iProp, 0) = property Then Exit For
			Next
		End If
		If iProp > UBound(propList) Then
			Wscript.Echo "Unknown summary property name: " & property
			sumInfo.Persist ' Note! must write even if error, else entire stream will be deleted
			Wscript.Quit 2
		End If
		If iProp = 11 Or iProp = 12 Or iProp = 13 Then
			If UCase(value) = "NOW"  Then value = Now
			If UCase(value) = "DATE" Then value = Date
			value = CDate(value)
		End If
		If iProp = 1 Or iProp = 14 Or iProp = 15 Or iProp = 16 Or iProp = 19 Then value = CLng(value)
		sumInfo.Property(iProp) = value : CheckError("Bad format for property value " & iProp)
		property = Empty
	End If
Next
If Not IsEmpty(property) Then
	Wscript.Echo "Arguments must be in the form: property=value  " & property
	sumInfo.Persist ' Note! must write even if error, else entire stream will be deleted
	Wscript.Quit 2
End If

' Write new property set. Note! must write even if error, else entire stream will be deleted
sumInfo.Persist : If CheckError("Error persisting summary property stream") Then Wscript.Quit 2
Wscript.Quit 0


Function CheckError(message)
	If Err = 0 Then Exit Function
	If IsEmpty(message) Then message = Err.Source & " " & Hex(Err) & ": " & Err.Description
	If Not installer Is Nothing Then
		Dim errRec : Set errRec = installer.LastErrorRecord
		If Not errRec Is Nothing Then message = message & vbNewLine & errRec.FormatText
	End If
	Wscript.Echo message
	CheckError = True
	Err.Clear
End Function

'' SIG '' Begin signature block
'' SIG '' MIImMwYJKoZIhvcNAQcCoIImJDCCJiACAQExDzANBglg
'' SIG '' hkgBZQMEAgEFADB3BgorBgEEAYI3AgEEoGkwZzAyBgor
'' SIG '' BgEEAYI3AgEeMCQCAQEEEE7wKRaZJ7VNj+Ws4Q8X66sC
'' SIG '' AQACAQACAQACAQACAQAwMTANBglghkgBZQMEAgEFAAQg
'' SIG '' bn8llKyfjYiHNwaF/UnnU74Wl84HND+puok0mU7lHYug
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
'' SIG '' jgd7JXFEqwZq5tTG3yOalnXFMYIaJDCCGiACAQEwgZUw
'' SIG '' fjELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0
'' SIG '' b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1p
'' SIG '' Y3Jvc29mdCBDb3Jwb3JhdGlvbjEoMCYGA1UEAxMfTWlj
'' SIG '' cm9zb2Z0IENvZGUgU2lnbmluZyBQQ0EgMjAxMAITMwAA
'' SIG '' BVfPkN3H0cCIjAAAAAAFVzANBglghkgBZQMEAgEFAKCB
'' SIG '' xjAZBgkqhkiG9w0BCQMxDAYKKwYBBAGCNwIBBDAcBgor
'' SIG '' BgEEAYI3AgELMQ4wDAYKKwYBBAGCNwIBFTAvBgkqhkiG
'' SIG '' 9w0BCQQxIgQglzkLrttgD3vlcRipTn+ZUDOD3R+L+Hfv
'' SIG '' DDHGuJMwz2wwWgYKKwYBBAGCNwIBDDFMMEqgJIAiAE0A
'' SIG '' aQBjAHIAbwBzAG8AZgB0ACAAVwBpAG4AZABvAHcAc6Ei
'' SIG '' gCBodHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vd2luZG93
'' SIG '' czANBgkqhkiG9w0BAQEFAASCAQBc9VEuUBi2rZtWl2xm
'' SIG '' c6dEItSCae3LFVe24dx0jDWrlk43xH9kEm7Msmxmr0Zm
'' SIG '' 6XAV6e7ONxV6oMykkn6FhGi0IIAG3h5x+EDLyB5xKnoS
'' SIG '' 6RCgZSG3Ts5jxJs3LUUzsasWLPUin2aB4U/3lEzH1eHH
'' SIG '' S+XZwLq8tHkbDMCZCje9OwKRjcISNXMXu+UCPib7oZQG
'' SIG '' hDKG9YONroqzJA/rE3pRyAaX3w+m4pHmxKNdR+YjrlMd
'' SIG '' Fb7THxWBQ29HYFmTSjtmIx3F+g7UAI6xsAqgRkZaEr9T
'' SIG '' CsmHSEmqqsaR3osleOWRKsSKXbmnTdmuRp09MJ60tI0Q
'' SIG '' k8Sj9dHBBP+yhsL+oYIXljCCF5IGCisGAQQBgjcDAwEx
'' SIG '' gheCMIIXfgYJKoZIhvcNAQcCoIIXbzCCF2sCAQMxDzAN
'' SIG '' BglghkgBZQMEAgEFADCCAVEGCyqGSIb3DQEJEAEEoIIB
'' SIG '' QASCATwwggE4AgEBBgorBgEEAYRZCgMBMDEwDQYJYIZI
'' SIG '' AWUDBAIBBQAEINkCX/M0tgG+aH2B3+8bRS2DunVEpN3P
'' SIG '' 0dappaAMVB2PAgZmvi0DuE8YEjIwMjQwOTA1MDkxNzMz
'' SIG '' LjUzWjAEgAIB9KCB0aSBzjCByzELMAkGA1UEBhMCVVMx
'' SIG '' EzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1Jl
'' SIG '' ZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3Jh
'' SIG '' dGlvbjElMCMGA1UECxMcTWljcm9zb2Z0IEFtZXJpY2Eg
'' SIG '' T3BlcmF0aW9uczEnMCUGA1UECxMeblNoaWVsZCBUU1Mg
'' SIG '' RVNOOjk2MDAtMDVFMC1EOTQ3MSUwIwYDVQQDExxNaWNy
'' SIG '' b3NvZnQgVGltZS1TdGFtcCBTZXJ2aWNloIIR7TCCByAw
'' SIG '' ggUIoAMCAQICEzMAAAHviT9WoVjMqNoAAQAAAe8wDQYJ
'' SIG '' KoZIhvcNAQELBQAwfDELMAkGA1UEBhMCVVMxEzARBgNV
'' SIG '' BAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQx
'' SIG '' HjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEm
'' SIG '' MCQGA1UEAxMdTWljcm9zb2Z0IFRpbWUtU3RhbXAgUENB
'' SIG '' IDIwMTAwHhcNMjMxMjA2MTg0NTQ4WhcNMjUwMzA1MTg0
'' SIG '' NTQ4WjCByzELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldh
'' SIG '' c2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNV
'' SIG '' BAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjElMCMGA1UE
'' SIG '' CxMcTWljcm9zb2Z0IEFtZXJpY2EgT3BlcmF0aW9uczEn
'' SIG '' MCUGA1UECxMeblNoaWVsZCBUU1MgRVNOOjk2MDAtMDVF
'' SIG '' MC1EOTQ3MSUwIwYDVQQDExxNaWNyb3NvZnQgVGltZS1T
'' SIG '' dGFtcCBTZXJ2aWNlMIICIjANBgkqhkiG9w0BAQEFAAOC
'' SIG '' Ag8AMIICCgKCAgEAowtY4p8M4B8ITmpGaste6BOASASr
'' SIG '' JuZF+A1JggViNJRVaRIiuZmdioefbKC+J7OdqYRTEGBh
'' SIG '' uZMqQoqbp4MD/TaG+FRlROmqDKOYWfTcrV0eWUYG/WfD
'' SIG '' UehJiyiAkYQ+LKIzzIP0ZxkU3HX+/02L8jNdIy45i8ih
'' SIG '' HoDB37yMD5jPgD+4c0C3xMQ3agidruuBneV5Z6xTpLuV
'' SIG '' PYyzipNcu9HPk8LdOP0S6q7r9Xxj/C5mJrR76weE3AbA
'' SIG '' A10pnBY4dFYEJF+M1xcKpyBvK4GPsw6iWEDWT/DtWKOJ
'' SIG '' EnJB0+N1wtKDONMntvvZf602IgxTN55WXto4bTpBgjuh
'' SIG '' qok6edMSPSE6SV4tLxHpPAHo0+DyjBDtmz8VOt6et7mW
'' SIG '' 43TeS/pYCHAjTAjSNEiKKUuIGlUeEsvyKA79bw1qXviN
'' SIG '' vPysvI1k3nndDtx8TyTGal+EAdyOg58Gax4ip+qBN/LY
'' SIG '' AUwggCrxKGDk4O69pRdCLm7f9/lT7yrUwlG2TxThvI2b
'' SIG '' faugBaHZb0J7YqJWCGLakqy8lwECJVxoWeIDXL+Hb9WA
'' SIG '' IpZ21gPQrJ2IfjihBa/+MODOvZSPsmqGdy/7f1H16U//
'' SIG '' snO4UvxaJXJqxhSUwWJUuJxNXLim5cGf1Dhtuki4QzjV
'' SIG '' lxmQyjCSjed6Di0kpOJXUdB5bG0+IXi5VpThJSUCAwEA
'' SIG '' AaOCAUkwggFFMB0GA1UdDgQWBBTtTFqihcKwm7a8PT/A
'' SIG '' Ot2wFUicyzAfBgNVHSMEGDAWgBSfpxVdAF5iXYP05dJl
'' SIG '' pxtTNRnpcjBfBgNVHR8EWDBWMFSgUqBQhk5odHRwOi8v
'' SIG '' d3d3Lm1pY3Jvc29mdC5jb20vcGtpb3BzL2NybC9NaWNy
'' SIG '' b3NvZnQlMjBUaW1lLVN0YW1wJTIwUENBJTIwMjAxMCgx
'' SIG '' KS5jcmwwbAYIKwYBBQUHAQEEYDBeMFwGCCsGAQUFBzAC
'' SIG '' hlBodHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vcGtpb3Bz
'' SIG '' L2NlcnRzL01pY3Jvc29mdCUyMFRpbWUtU3RhbXAlMjBQ
'' SIG '' Q0ElMjAyMDEwKDEpLmNydDAMBgNVHRMBAf8EAjAAMBYG
'' SIG '' A1UdJQEB/wQMMAoGCCsGAQUFBwMIMA4GA1UdDwEB/wQE
'' SIG '' AwIHgDANBgkqhkiG9w0BAQsFAAOCAgEAGBmWt2gg7nW5
'' SIG '' PRFXZD/MXEBmbiACD0cfStQgO7kcwbfNHwtGlpLmGIUD
'' SIG '' LxxyUR1KG0jOFMN8ze3xxDfIYWgQ2/TUWhpxVnbR8Zif
'' SIG '' XjM+iaZ+ioiMovVOToO0Ak2TJde59sOHnXaub7ZOK0Vj
'' SIG '' lb6YgwRiQESol1gfbtosdFh9hDBRh6oyIY1lF4T4EeAu
'' SIG '' jShTVx71r13nCdll6yZ770BlwHzSRhEyWRqUeNZ1Dd4o
'' SIG '' 34gkoxQ8Wphj7MuYmLvdOB7/brkl2HeZtCcX9ljSUl5D
'' SIG '' xpTYaztu6T8YE9ddZsgEetUt0toXOe9szfcqCRDmxPfF
'' SIG '' cuShDN2V+d3C3nzfNRdQvaf3ACpBOrvVeq8spf6koMbt
'' SIG '' VKnjmQrRv4mh0ijKMTOzKuEjBbD0//InjncApWKXMNAo
'' SIG '' 2XuSgcdsS2uAdZ3hYm/CfP4EqLIzHRd5x4sh8dWHnWQ7
'' SIG '' cUkoHoHibItH21IHc7FTCWL6lcOdlqkDbtBkQu/Wbla3
'' SIG '' lFSnQiZlDARwaU6elRaKS9CX+Eq4IPs0Q/YsG3Pbma5/
'' SIG '' vPaHaSJ2852K5zyh4jtuqntXpDcJf3e66NiLT/5YIc9A
'' SIG '' 6A+5BBnopCiVh3baO3lSaCYZK1HGp07lB9PIPjWMBukv
'' SIG '' j4wUgfzcjRemx2v8UfnHgGIXI8dIgYr/dDJ9CYhn5wNv
'' SIG '' 4S4+Xr4U3AIwggdxMIIFWaADAgECAhMzAAAAFcXna54C
'' SIG '' m0mZAAAAAAAVMA0GCSqGSIb3DQEBCwUAMIGIMQswCQYD
'' SIG '' VQQGEwJVUzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4G
'' SIG '' A1UEBxMHUmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0
'' SIG '' IENvcnBvcmF0aW9uMTIwMAYDVQQDEylNaWNyb3NvZnQg
'' SIG '' Um9vdCBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkgMjAxMDAe
'' SIG '' Fw0yMTA5MzAxODIyMjVaFw0zMDA5MzAxODMyMjVaMHwx
'' SIG '' CzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9u
'' SIG '' MRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNy
'' SIG '' b3NvZnQgQ29ycG9yYXRpb24xJjAkBgNVBAMTHU1pY3Jv
'' SIG '' c29mdCBUaW1lLVN0YW1wIFBDQSAyMDEwMIICIjANBgkq
'' SIG '' hkiG9w0BAQEFAAOCAg8AMIICCgKCAgEA5OGmTOe0ciEL
'' SIG '' eaLL1yR5vQ7VgtP97pwHB9KpbE51yMo1V/YBf2xK4OK9
'' SIG '' uT4XYDP/XE/HZveVU3Fa4n5KWv64NmeFRiMMtY0Tz3cy
'' SIG '' wBAY6GB9alKDRLemjkZrBxTzxXb1hlDcwUTIcVxRMTeg
'' SIG '' Cjhuje3XD9gmU3w5YQJ6xKr9cmmvHaus9ja+NSZk2pg7
'' SIG '' uhp7M62AW36MEBydUv626GIl3GoPz130/o5Tz9bshVZN
'' SIG '' 7928jaTjkY+yOSxRnOlwaQ3KNi1wjjHINSi947SHJMPg
'' SIG '' yY9+tVSP3PoFVZhtaDuaRr3tpK56KTesy+uDRedGbsoy
'' SIG '' 1cCGMFxPLOJiss254o2I5JasAUq7vnGpF1tnYN74kpEe
'' SIG '' HT39IM9zfUGaRnXNxF803RKJ1v2lIH1+/NmeRd+2ci/b
'' SIG '' fV+AutuqfjbsNkz2K26oElHovwUDo9Fzpk03dJQcNIIP
'' SIG '' 8BDyt0cY7afomXw/TNuvXsLz1dhzPUNOwTM5TI4CvEJo
'' SIG '' LhDqhFFG4tG9ahhaYQFzymeiXtcodgLiMxhy16cg8ML6
'' SIG '' EgrXY28MyTZki1ugpoMhXV8wdJGUlNi5UPkLiWHzNgY1
'' SIG '' GIRH29wb0f2y1BzFa/ZcUlFdEtsluq9QBXpsxREdcu+N
'' SIG '' +VLEhReTwDwV2xo3xwgVGD94q0W29R6HXtqPnhZyacau
'' SIG '' e7e3PmriLq0CAwEAAaOCAd0wggHZMBIGCSsGAQQBgjcV
'' SIG '' AQQFAgMBAAEwIwYJKwYBBAGCNxUCBBYEFCqnUv5kxJq+
'' SIG '' gpE8RjUpzxD/LwTuMB0GA1UdDgQWBBSfpxVdAF5iXYP0
'' SIG '' 5dJlpxtTNRnpcjBcBgNVHSAEVTBTMFEGDCsGAQQBgjdM
'' SIG '' g30BATBBMD8GCCsGAQUFBwIBFjNodHRwOi8vd3d3Lm1p
'' SIG '' Y3Jvc29mdC5jb20vcGtpb3BzL0RvY3MvUmVwb3NpdG9y
'' SIG '' eS5odG0wEwYDVR0lBAwwCgYIKwYBBQUHAwgwGQYJKwYB
'' SIG '' BAGCNxQCBAweCgBTAHUAYgBDAEEwCwYDVR0PBAQDAgGG
'' SIG '' MA8GA1UdEwEB/wQFMAMBAf8wHwYDVR0jBBgwFoAU1fZW
'' SIG '' y4/oolxiaNE9lJBb186aGMQwVgYDVR0fBE8wTTBLoEmg
'' SIG '' R4ZFaHR0cDovL2NybC5taWNyb3NvZnQuY29tL3BraS9j
'' SIG '' cmwvcHJvZHVjdHMvTWljUm9vQ2VyQXV0XzIwMTAtMDYt
'' SIG '' MjMuY3JsMFoGCCsGAQUFBwEBBE4wTDBKBggrBgEFBQcw
'' SIG '' AoY+aHR0cDovL3d3dy5taWNyb3NvZnQuY29tL3BraS9j
'' SIG '' ZXJ0cy9NaWNSb29DZXJBdXRfMjAxMC0wNi0yMy5jcnQw
'' SIG '' DQYJKoZIhvcNAQELBQADggIBAJ1VffwqreEsH2cBMSRb
'' SIG '' 4Z5yS/ypb+pcFLY+TkdkeLEGk5c9MTO1OdfCcTY/2mRs
'' SIG '' fNB1OW27DzHkwo/7bNGhlBgi7ulmZzpTTd2YurYeeNg2
'' SIG '' LpypglYAA7AFvonoaeC6Ce5732pvvinLbtg/SHUB2Rje
'' SIG '' bYIM9W0jVOR4U3UkV7ndn/OOPcbzaN9l9qRWqveVtihV
'' SIG '' J9AkvUCgvxm2EhIRXT0n4ECWOKz3+SmJw7wXsFSFQrP8
'' SIG '' DJ6LGYnn8AtqgcKBGUIZUnWKNsIdw2FzLixre24/LAl4
'' SIG '' FOmRsqlb30mjdAy87JGA0j3mSj5mO0+7hvoyGtmW9I/2
'' SIG '' kQH2zsZ0/fZMcm8Qq3UwxTSwethQ/gpY3UA8x1RtnWN0
'' SIG '' SCyxTkctwRQEcb9k+SS+c23Kjgm9swFXSVRk2XPXfx5b
'' SIG '' RAGOWhmRaw2fpCjcZxkoJLo4S5pu+yFUa2pFEUep8beu
'' SIG '' yOiJXk+d0tBMdrVXVAmxaQFEfnyhYWxz/gq77EFmPWn9
'' SIG '' y8FBSX5+k77L+DvktxW/tM4+pTFRhLy/AsGConsXHRWJ
'' SIG '' jXD+57XQKBqJC4822rpM+Zv/Cuk0+CQ1ZyvgDbjmjJnW
'' SIG '' 4SLq8CdCPSWU5nR0W2rRnj7tfqAxM328y+l7vzhwRNGQ
'' SIG '' 8cirOoo6CGJ/2XBjU02N7oJtpQUQwXEGahC0HVUzWLOh
'' SIG '' cGbyoYIDUDCCAjgCAQEwgfmhgdGkgc4wgcsxCzAJBgNV
'' SIG '' BAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYD
'' SIG '' VQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQg
'' SIG '' Q29ycG9yYXRpb24xJTAjBgNVBAsTHE1pY3Jvc29mdCBB
'' SIG '' bWVyaWNhIE9wZXJhdGlvbnMxJzAlBgNVBAsTHm5TaGll
'' SIG '' bGQgVFNTIEVTTjo5NjAwLTA1RTAtRDk0NzElMCMGA1UE
'' SIG '' AxMcTWljcm9zb2Z0IFRpbWUtU3RhbXAgU2VydmljZaIj
'' SIG '' CgEBMAcGBSsOAwIaAxUAS3CPNYMW3mtRMdphW18e3JPt
'' SIG '' IP+ggYMwgYCkfjB8MQswCQYDVQQGEwJVUzETMBEGA1UE
'' SIG '' CBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEe
'' SIG '' MBwGA1UEChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMSYw
'' SIG '' JAYDVQQDEx1NaWNyb3NvZnQgVGltZS1TdGFtcCBQQ0Eg
'' SIG '' MjAxMDANBgkqhkiG9w0BAQsFAAIFAOqDsFwwIhgPMjAy
'' SIG '' NDA5MDUwNDIxNDhaGA8yMDI0MDkwNjA0MjE0OFowdzA9
'' SIG '' BgorBgEEAYRZCgQBMS8wLTAKAgUA6oOwXAIBADAKAgEA
'' SIG '' AgIMmwIB/zAHAgEAAgIUfDAKAgUA6oUB3AIBADA2Bgor
'' SIG '' BgEEAYRZCgQCMSgwJjAMBgorBgEEAYRZCgMCoAowCAIB
'' SIG '' AAIDB6EgoQowCAIBAAIDAYagMA0GCSqGSIb3DQEBCwUA
'' SIG '' A4IBAQAj4rLTPeaUtC3bSLsAxftIN8L0wxslcGvbDz9x
'' SIG '' aGOyoEII0gOTehP7KnmhzakoEYTmU2w4o2pSm/5P/eBN
'' SIG '' 3JWPPZwqRAIsTssierkEueeBLa/NWrj3eedY3IUUQsZZ
'' SIG '' dl+3XbfdYWlSCiEdcCTa3vWMtgvoQPq9r8rwEb+vPvbk
'' SIG '' 6oXlAxHBjgw5EVUVGIKS+xo2jCf8h8J0Qg5C12cm+JGh
'' SIG '' dz+5ANKc57fLx6O8zzgKOc9vgzVzzLCRR3n0FH1xGxAu
'' SIG '' HcDSIWX7l+iAipanzSWXo47TsIDfp2W8/w6perIcVj65
'' SIG '' fgIY+uG/euANEdT0nxdRkImc2baB5ppnlAm+7LF8MYIE
'' SIG '' DTCCBAkCAQEwgZMwfDELMAkGA1UEBhMCVVMxEzARBgNV
'' SIG '' BAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQx
'' SIG '' HjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEm
'' SIG '' MCQGA1UEAxMdTWljcm9zb2Z0IFRpbWUtU3RhbXAgUENB
'' SIG '' IDIwMTACEzMAAAHviT9WoVjMqNoAAQAAAe8wDQYJYIZI
'' SIG '' AWUDBAIBBQCgggFKMBoGCSqGSIb3DQEJAzENBgsqhkiG
'' SIG '' 9w0BCRABBDAvBgkqhkiG9w0BCQQxIgQgiP2Yo9K/mfiO
'' SIG '' 5lFuvHDK3Bt5CCMygyrYo6dy7RB1Yx4wgfoGCyqGSIb3
'' SIG '' DQEJEAIvMYHqMIHnMIHkMIG9BCDwYShFuBaN8FM9PTUM
'' SIG '' dmtA23HbF/I6LzOS4sx5p8l/ozCBmDCBgKR+MHwxCzAJ
'' SIG '' BgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAw
'' SIG '' DgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3Nv
'' SIG '' ZnQgQ29ycG9yYXRpb24xJjAkBgNVBAMTHU1pY3Jvc29m
'' SIG '' dCBUaW1lLVN0YW1wIFBDQSAyMDEwAhMzAAAB74k/VqFY
'' SIG '' zKjaAAEAAAHvMCIEIMxIFLGZrYHVbh6svGKe12PuZNxh
'' SIG '' n/Mq4m3WXxcKUVpGMA0GCSqGSIb3DQEBCwUABIICAJoq
'' SIG '' 4J8y0Y5LcUN5TZCibQwDt99eK66+8vt7GLk+gowc5cqc
'' SIG '' Ao501SecH8201QvglVxBo13OTjzdIv2rsVUc4+eNFW31
'' SIG '' 57e9crbMCOFGFD8ELU214gdSR8P0CkTGacWJV/5gnOLt
'' SIG '' isEG6thMyU7g/KLG4jNYPofKaKWP/4qhTKt2xebwHB8k
'' SIG '' omWBbx6YTJhmCKK0uY9xWKzDG2BIrsoPTvnFv7MHrHpM
'' SIG '' FEyVu7t6dTu3aVL4qGG3FHyu/oLd1LkW2cyOsTWU60b3
'' SIG '' UVXSBU5Nv/1tlnMOp/dc6Cqz00DH/2CVqHZ4MkVclteq
'' SIG '' LIpwUS5kcqLxTh6l5WerCSYuNgsoXMU/Oco2hj3jwdP6
'' SIG '' hoNzalp2CRf3WYXiO7RHQyMLMXvZfZTNMu9PtoZLOIEX
'' SIG '' o1dGJGdxIpSZ7Sbdm3j5Ckc/4Y20pYw+npTz7KiuVvu4
'' SIG '' u2BOuk7MADSRSL+e7vqRByUNEYOeOdiEIUbgVkiUZVd/
'' SIG '' WoJSEy8pOpQrIddNUYQYYWGlaO4Vu9/hoEQ6Td05TUV+
'' SIG '' 6E0oJEE/Lgl0bQibONwYFAFSbSAVdaLPC1UPPxomi+62
'' SIG '' DYtAlwGPsK1lYa0kO3071xxtg81xMnBdN0a2R8FEE9Gb
'' SIG '' p9bNRNaTE6HxcU0L5VHT5zA4w3hd/pGj7il+8Hx3QyFL
'' SIG '' DX//XBYESFgTv+7d1wPi
'' SIG '' End signature block
