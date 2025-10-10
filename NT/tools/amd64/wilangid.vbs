' Windows Installer utility to report the language and codepage for a package
' For use with Windows Scripting Host, CScript.exe or WScript.exe
' Copyright (c) Microsoft Corporation. All rights reserved.
' Demonstrates the access of language and codepage values                 
'
Option Explicit

Const msiOpenDatabaseModeReadOnly     = 0
Const msiOpenDatabaseModeTransact     = 1
Const ForReading = 1
Const ForWriting = 2
Const TristateFalse = 0

Const msiViewModifyInsert         = 1
Const msiViewModifyUpdate         = 2
Const msiViewModifyAssign         = 3
Const msiViewModifyReplace        = 4
Const msiViewModifyDelete         = 6

Dim argCount:argCount = Wscript.Arguments.Count
If argCount > 0 Then If InStr(1, Wscript.Arguments(0), "?", vbTextCompare) > 0 Then argCount = 0
If (argCount = 0) Then
	message = "Windows Installer utility to manage language and codepage values for a package." &_
		vbNewLine & "The package language is a summary information property that designates the" &_
		vbNewLine & " primary language and any language transforms that are available, comma delim." &_
		vbNewLine & "The ProductLanguage in the database Property table is the language that is" &_
		vbNewLine & " registered for the product and determines the language used to load resources." &_
		vbNewLine & "The codepage is the ANSI codepage of the database strings, 0 if all ASCII data," &_
		vbNewLine & " and must represent the text data to avoid loss when persisting the database." &_
		vbNewLine & "The 1st argument is the path to MSI database (installer package)" &_
		vbNewLine & "To update a value, the 2nd argument contains the keyword and the 3rd the value:" &_
		vbNewLine & "   Package  {base LangId optionally followed by list of language transforms}" &_
		vbNewLine & "   Product  {LangId of the product (could be updated by language transforms)}" &_
		vbNewLine & "   Codepage {ANSI codepage of text data (use with caution when text exists!)}" &_
		vbNewLine &_
		vbNewLine & "Copyright (C) Microsoft Corporation.  All rights reserved."
	Wscript.Echo message
	Wscript.Quit 1
End If

' Connect to Windows Installer object
On Error Resume Next
Dim installer : Set installer = Nothing
Set installer = Wscript.CreateObject("WindowsInstaller.Installer") : CheckError


' Open database
Dim databasePath:databasePath = Wscript.Arguments(0)
Dim openMode : If argCount >= 3 Then openMode = msiOpenDatabaseModeTransact Else openMode = msiOpenDatabaseModeReadOnly
Dim database : Set database = installer.OpenDatabase(databasePath, openMode) : CheckError

' Update value if supplied
If argCount >= 3 Then
	Dim value:value = Wscript.Arguments(2)
	Select Case UCase(Wscript.Arguments(1))
		Case "PACKAGE"  : SetPackageLanguage database, value
		Case "PRODUCT"  : SetProductLanguage database, value
		Case "CODEPAGE" : SetDatabaseCodepage database, value
		Case Else       : Fail "Invalid value keyword"
	End Select
	CheckError
End If

' Extract language info and compose report message
Dim message:message = "Package language = "         & PackageLanguage(database) &_
					", ProductLanguage = " & ProductLanguage(database) &_
					", Database codepage = "        & DatabaseCodepage(database)
database.Commit : CheckError  ' no effect if opened ReadOnly
Set database = nothing
Wscript.Echo message
Wscript.Quit 0

' Get language list from summary information
Function PackageLanguage(database)
	On Error Resume Next
	Dim sumInfo  : Set sumInfo = database.SummaryInformation(0) : CheckError
	Dim template : template = sumInfo.Property(7) : CheckError
	Dim iDelim:iDelim = InStr(1, template, ";", vbTextCompare)
	If iDelim = 0 Then template = "Not specified!"
	PackageLanguage = Right(template, Len(template) - iDelim)
	If Len(PackageLanguage) = 0 Then PackageLanguage = "0"
End Function

' Get ProductLanguge property from Property table
Function ProductLanguage(database)
	On Error Resume Next
	Dim view : Set view = database.OpenView("SELECT `Value` FROM `Property` WHERE `Property` = 'ProductLanguage'")
	view.Execute : CheckError
	Dim record : Set record = view.Fetch : CheckError
	If record Is Nothing Then ProductLanguage = "Not specified!" Else ProductLanguage = record.IntegerData(1)
End Function

' Get ANSI codepage of database text data
Function DatabaseCodepage(database)
	On Error Resume Next
	Dim WshShell : Set WshShell = Wscript.CreateObject("Wscript.Shell") : CheckError
	Dim tempPath:tempPath = WshShell.ExpandEnvironmentStrings("%TEMP%") : CheckError
	database.Export "_ForceCodepage", tempPath, "codepage.idt" : CheckError
	Dim fileSys : Set fileSys = CreateObject("Scripting.FileSystemObject") : CheckError
	Dim file : Set file = fileSys.OpenTextFile(tempPath & "\codepage.idt", ForReading, False, TristateFalse) : CheckError
	file.ReadLine ' skip column name record
	file.ReadLine ' skip column defn record
	DatabaseCodepage = file.ReadLine
	file.Close
	Dim iDelim:iDelim = InStr(1, DatabaseCodepage, vbTab, vbTextCompare)
	If iDelim = 0 Then Fail "Failure in codepage export file"
	DatabaseCodepage = Left(DatabaseCodepage, iDelim - 1)
	fileSys.DeleteFile(tempPath & "\codepage.idt")
End Function

' Set ProductLanguge property in Property table
Sub SetProductLanguage(database, language)
	On Error Resume Next
	If Not IsNumeric(language) Then Fail "ProductLanguage must be numeric"
	Dim view : Set view = database.OpenView("SELECT `Property`,`Value` FROM `Property`")
	view.Execute : CheckError
	Dim record : Set record = installer.CreateRecord(2)
	record.StringData(1) = "ProductLanguage"
	record.StringData(2) = CStr(language)
	view.Modify msiViewModifyAssign, record : CheckError
End Sub

' Set ANSI codepage of database text data
Sub SetDatabaseCodepage(database, codepage)
	On Error Resume Next
	If Not IsNumeric(codepage) Then Fail "Codepage must be numeric"
	Dim WshShell : Set WshShell = Wscript.CreateObject("Wscript.Shell") : CheckError
	Dim tempPath:tempPath = WshShell.ExpandEnvironmentStrings("%TEMP%") : CheckError
	Dim fileSys : Set fileSys = CreateObject("Scripting.FileSystemObject") : CheckError
	Dim file : Set file = fileSys.OpenTextFile(tempPath & "\codepage.idt", ForWriting, True, TristateFalse) : CheckError
	file.WriteLine ' dummy column name record
	file.WriteLine ' dummy column defn record
	file.WriteLine codepage & vbTab & "_ForceCodepage"
	file.Close : CheckError
	database.Import tempPath, "codepage.idt" : CheckError
	fileSys.DeleteFile(tempPath & "\codepage.idt")
End Sub     

' Set language list in summary information
Sub SetPackageLanguage(database, language)
	On Error Resume Next
	Dim sumInfo  : Set sumInfo = database.SummaryInformation(1) : CheckError
	Dim template : template = sumInfo.Property(7) : CheckError
	Dim iDelim:iDelim = InStr(1, template, ";", vbTextCompare)
	Dim platform : If iDelim = 0 Then platform = ";" Else platform = Left(template, iDelim)
	sumInfo.Property(7) = platform & language
	sumInfo.Persist : CheckError
End Sub

Sub CheckError
	Dim message, errRec
	If Err = 0 Then Exit Sub
	message = Err.Source & " " & Hex(Err) & ": " & Err.Description
	If Not installer Is Nothing Then
		Set errRec = installer.LastErrorRecord
		If Not errRec Is Nothing Then message = message & vbNewLine & errRec.FormatText
	End If
	Fail message
End Sub

Sub Fail(message)
	Wscript.Echo message
	Wscript.Quit 2
End Sub

'' SIG '' Begin signature block
'' SIG '' MIImTQYJKoZIhvcNAQcCoIImPjCCJjoCAQExDzANBglg
'' SIG '' hkgBZQMEAgEFADB3BgorBgEEAYI3AgEEoGkwZzAyBgor
'' SIG '' BgEEAYI3AgEeMCQCAQEEEE7wKRaZJ7VNj+Ws4Q8X66sC
'' SIG '' AQACAQACAQACAQACAQAwMTANBglghkgBZQMEAgEFAAQg
'' SIG '' P5ZR+tRLXw+tvFB7cXDc0jFoO6HhZPDQciZh+dfNY5qg
'' SIG '' ggtnMIIE7zCCA9egAwIBAgITMwAABae4j/uXXTWE7AAA
'' SIG '' AAAFpzANBgkqhkiG9w0BAQsFADB+MQswCQYDVQQGEwJV
'' SIG '' UzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMH
'' SIG '' UmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBv
'' SIG '' cmF0aW9uMSgwJgYDVQQDEx9NaWNyb3NvZnQgQ29kZSBT
'' SIG '' aWduaW5nIFBDQSAyMDEwMB4XDTI0MDgyMjE5MjU1N1oX
'' SIG '' DTI1MDcwNTE5MjU1N1owdDELMAkGA1UEBhMCVVMxEzAR
'' SIG '' BgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1v
'' SIG '' bmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlv
'' SIG '' bjEeMBwGA1UEAxMVTWljcm9zb2Z0IENvcnBvcmF0aW9u
'' SIG '' MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA
'' SIG '' lhpUyo2LetKwfKDcj1iVBkFdjRsJUVyiYN+POKtpyYr/
'' SIG '' fha8/enxqF5W5SHid8akMRKAhA2I422ApYMd9TGXKEai
'' SIG '' Q9LCozbWAygNDYknTiULrd/hzdK0se+MqqGwwT/ACgMl
'' SIG '' gDWYrVEB5zx9RJE1zHUZZyZw9UbRZYzGDiZ68X6qwHbT
'' SIG '' TcVNaSGGJmOqA/HcnvNYvUR8UiRhIHzJxCKZ/9ckpIpE
'' SIG '' 1fjThxY9UB+/kh2VmDXHYC+PEEmtYwt9AIujCi4fdRTr
'' SIG '' ArLjVNHEwus+kJD+dZfXVPAfsZ72Wtv1s7yYQcmZ410v
'' SIG '' rVXjdigeRKdLHjrbhcLyOiqDl6xEygg4OwIDAQABo4IB
'' SIG '' bjCCAWowHwYDVR0lBBgwFgYKKwYBBAGCNz0GAQYIKwYB
'' SIG '' BQUHAwMwHQYDVR0OBBYEFFIVus1TcIc5i27HKM2KacHf
'' SIG '' HysSMEUGA1UdEQQ+MDykOjA4MR4wHAYDVQQLExVNaWNy
'' SIG '' b3NvZnQgQ29ycG9yYXRpb24xFjAUBgNVBAUTDTIzMDg2
'' SIG '' NSs1MDI3MDMwHwYDVR0jBBgwFoAU5vxfe7siAFjkck61
'' SIG '' 9CF0IzLm76wwVgYDVR0fBE8wTTBLoEmgR4ZFaHR0cDov
'' SIG '' L2NybC5taWNyb3NvZnQuY29tL3BraS9jcmwvcHJvZHVj
'' SIG '' dHMvTWljQ29kU2lnUENBXzIwMTAtMDctMDYuY3JsMFoG
'' SIG '' CCsGAQUFBwEBBE4wTDBKBggrBgEFBQcwAoY+aHR0cDov
'' SIG '' L3d3dy5taWNyb3NvZnQuY29tL3BraS9jZXJ0cy9NaWND
'' SIG '' b2RTaWdQQ0FfMjAxMC0wNy0wNi5jcnQwDAYDVR0TAQH/
'' SIG '' BAIwADANBgkqhkiG9w0BAQsFAAOCAQEAJdXECEPhQ/7m
'' SIG '' 2liIjIPMELRMd0pLEOa+qgIH3qznuk2eW5k3DI9lVJBy
'' SIG '' 675oUnKEXvaUPwqsGeu+mLjPdLYqj6zA41zvJCwgPpE3
'' SIG '' g2aCkC9DCNkoWw4V6wyLLovYRjYXfD8Bk1kJLJ6DuB8a
'' SIG '' hhtjH4qrJzoDKPR4ppkxdvx9Vy3P4Nkz6RfBslwHKO5I
'' SIG '' XIeJdYSCKZlRGTemRQpNv5Dn+5trApfIefgVkA5kmhAr
'' SIG '' SNsXOUi26qLdYrFrxYhEbWsPcUG99TFmGrNpdv13XGx+
'' SIG '' 0BWKSqRuHQ2YSiHZUVZmKVMkWjTmVjcDXOxum8yiAtxw
'' SIG '' BhTiBHOGg0Ltsk/6tMie1jCCBnAwggRYoAMCAQICCmEM
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
'' SIG '' jgd7JXFEqwZq5tTG3yOalnXFMYIaPjCCGjoCAQEwgZUw
'' SIG '' fjELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0
'' SIG '' b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1p
'' SIG '' Y3Jvc29mdCBDb3Jwb3JhdGlvbjEoMCYGA1UEAxMfTWlj
'' SIG '' cm9zb2Z0IENvZGUgU2lnbmluZyBQQ0EgMjAxMAITMwAA
'' SIG '' Bae4j/uXXTWE7AAAAAAFpzANBglghkgBZQMEAgEFAKCB
'' SIG '' xjAZBgkqhkiG9w0BCQMxDAYKKwYBBAGCNwIBBDAcBgor
'' SIG '' BgEEAYI3AgELMQ4wDAYKKwYBBAGCNwIBFTAvBgkqhkiG
'' SIG '' 9w0BCQQxIgQgAJ8cRSOltvo4yxAzz3Dva5kV2Rg0xRqh
'' SIG '' 52euzNRpebMwWgYKKwYBBAGCNwIBDDFMMEqgJIAiAE0A
'' SIG '' aQBjAHIAbwBzAG8AZgB0ACAAVwBpAG4AZABvAHcAc6Ei
'' SIG '' gCBodHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vd2luZG93
'' SIG '' czANBgkqhkiG9w0BAQEFAASCAQCUOtCivHCgchuPfgtR
'' SIG '' nFU3mP0nWj8QSawFUUul9hNiO688ePLk/j5monP1yD0s
'' SIG '' 7MnKZV/2mHK2BbbBNLXsnaNkOS4b7WDUjPgJKZiEU5en
'' SIG '' DTXecKbvHIt0sNZo3lyH83g3VdX3KAU0zPrcDHaqHCoC
'' SIG '' iqft1Q7/zgHWBh4fGdl3j3M7KYBPI/D+0thaFbevlIBf
'' SIG '' K5WnDWidTXSmT6bPBajrIjyJKUIl4QK7cQUMEb0q+ZWB
'' SIG '' W/Q5VFShxsEvsY8HNrKFRQAX1I84j9QUIW3K/uxz0n6A
'' SIG '' aAuvTeBIWikFb+txv2S+wOOES2uKj177J5tOXqRwXZ2P
'' SIG '' N5Lk+JEuhKNttgkOoYIXsDCCF6wGCisGAQQBgjcDAwEx
'' SIG '' ghecMIIXmAYJKoZIhvcNAQcCoIIXiTCCF4UCAQMxDzAN
'' SIG '' BglghkgBZQMEAgEFADCCAVoGCyqGSIb3DQEJEAEEoIIB
'' SIG '' SQSCAUUwggFBAgEBBgorBgEEAYRZCgMBMDEwDQYJYIZI
'' SIG '' AWUDBAIBBQAEIOnKPPmTRsqNEMejIUNKy4+v2cKj86mi
'' SIG '' bns4WX7giTIJAgZm60Q3OEAYEzIwMjQxMTE2MDkxNjQ2
'' SIG '' LjA4OVowBIACAfSggdmkgdYwgdMxCzAJBgNVBAYTAlVT
'' SIG '' MRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdS
'' SIG '' ZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9y
'' SIG '' YXRpb24xLTArBgNVBAsTJE1pY3Jvc29mdCBJcmVsYW5k
'' SIG '' IE9wZXJhdGlvbnMgTGltaXRlZDEnMCUGA1UECxMeblNo
'' SIG '' aWVsZCBUU1MgRVNOOjQwMUEtMDVFMC1EOTQ3MSUwIwYD
'' SIG '' VQQDExxNaWNyb3NvZnQgVGltZS1TdGFtcCBTZXJ2aWNl
'' SIG '' oIIR/jCCBygwggUQoAMCAQICEzMAAAH+0KjCezQhCwEA
'' SIG '' AQAAAf4wDQYJKoZIhvcNAQELBQAwfDELMAkGA1UEBhMC
'' SIG '' VVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcT
'' SIG '' B1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jw
'' SIG '' b3JhdGlvbjEmMCQGA1UEAxMdTWljcm9zb2Z0IFRpbWUt
'' SIG '' U3RhbXAgUENBIDIwMTAwHhcNMjQwNzI1MTgzMTE4WhcN
'' SIG '' MjUxMDIyMTgzMTE4WjCB0zELMAkGA1UEBhMCVVMxEzAR
'' SIG '' BgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1v
'' SIG '' bmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlv
'' SIG '' bjEtMCsGA1UECxMkTWljcm9zb2Z0IElyZWxhbmQgT3Bl
'' SIG '' cmF0aW9ucyBMaW1pdGVkMScwJQYDVQQLEx5uU2hpZWxk
'' SIG '' IFRTUyBFU046NDAxQS0wNUUwLUQ5NDcxJTAjBgNVBAMT
'' SIG '' HE1pY3Jvc29mdCBUaW1lLVN0YW1wIFNlcnZpY2UwggIi
'' SIG '' MA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoICAQC8vCEX
'' SIG '' FaWoDjeiWwTg8J6BniZJ+wfZhNIoRi/wafffrYGZrJx1
'' SIG '' /lPe1DGk/c1abrZgdSJ4hBfD7S7iqVrLgA3ciicj7js2
'' SIG '' mL1+jnbF0BcxfSkatzR6pbxFY3dt/Nc8q/Ts7XyLYeMP
'' SIG '' Iu7LBjIoD0WZZt4+NqF/0zB3xCDKCQ+3AOtVAYvI6TIz
'' SIG '' dVOIcqqEa70EIZVF0db2WY8yutSU9aJhX0tUIHlVh34A
'' SIG '' RS11+oB2qXNXEDncSDFKqGnolt8QqdN1x8/pPwyKvQev
'' SIG '' BNO1XaHbIMG2NdtAhqrJwo5vrfcZ9GSfbXos4MGDfs//
'' SIG '' HCGh1dPzVkLZoc3t7EQOaZuJayyMa8UmSWLaDp23TV5K
'' SIG '' E6IaaFuievSpddwF6o1vpCgXyNf+4NW1j2m8viPxoRZL
'' SIG '' j2EpQfSbOwK5wivBRL7Hwy5PS5/tVcIU0VuIJQ1FOh/E
'' SIG '' ncHjnh4YmEvR/BRNFuDIJukuAowoOIJG5vrkOFp4O9QA
'' SIG '' AlP3cpIKh4UKiSU9q9uBDJqEZkMv+9YBWNflvwnOGXL2
'' SIG '' AYJ0r+qLqL5zFnRLzHoHbKM9tl90FV8f80Gn/UufvFt4
'' SIG '' 4RMA6fs5P0PdQa3Sr4qJaBjjYecuPKGXVsC7kd+CvIA7
'' SIG '' cMJoh1Xa2O+QlrLao6cXsOCPxrrQpBP1CB8l/Beevdkq
'' SIG '' JtgyNpRsI0gOfHPbKQIDAQABo4IBSTCCAUUwHQYDVR0O
'' SIG '' BBYEFG/Oe4n1JTaDmX/n9v7kIGJtscXdMB8GA1UdIwQY
'' SIG '' MBaAFJ+nFV0AXmJdg/Tl0mWnG1M1GelyMF8GA1UdHwRY
'' SIG '' MFYwVKBSoFCGTmh0dHA6Ly93d3cubWljcm9zb2Z0LmNv
'' SIG '' bS9wa2lvcHMvY3JsL01pY3Jvc29mdCUyMFRpbWUtU3Rh
'' SIG '' bXAlMjBQQ0ElMjAyMDEwKDEpLmNybDBsBggrBgEFBQcB
'' SIG '' AQRgMF4wXAYIKwYBBQUHMAKGUGh0dHA6Ly93d3cubWlj
'' SIG '' cm9zb2Z0LmNvbS9wa2lvcHMvY2VydHMvTWljcm9zb2Z0
'' SIG '' JTIwVGltZS1TdGFtcCUyMFBDQSUyMDIwMTAoMSkuY3J0
'' SIG '' MAwGA1UdEwEB/wQCMAAwFgYDVR0lAQH/BAwwCgYIKwYB
'' SIG '' BQUHAwgwDgYDVR0PAQH/BAQDAgeAMA0GCSqGSIb3DQEB
'' SIG '' CwUAA4ICAQCJPg1S87aXD7R0My2wG7GZfajeVVCqdCS4
'' SIG '' hMAYgYKAj6yXtmpk5MN3wurGwfgZYI9PO1ze2vwCAG+x
'' SIG '' gjNaMXCKgKMJA4OrWgExY3MSwrNyQfEDNijlLN7s4+Qw
'' SIG '' DcMDFWrhJJHzL5NELYZw53QlF5nWU+WGU+X1cj7Pw6C0
'' SIG '' 4+ZCcsuI/2rOlMfAXN76xupKfxx6R24xl0vIcmTc2LDc
'' SIG '' CeCVT9ZPMaxAB1yH1JVXgseJ9SebBN/SLTuIq1OU2Srd
'' SIG '' vHWLJaDs3uMZkAFFZPaZf5gBUeUrbu32f5a1hufpw4k1
'' SIG '' fouwfzE9UFFgAhFWRawzIQB2g/12p9pnPBcaaO5VD3fU
'' SIG '' 2HMeOMb4R/DXXwNeOTdWrepQjWt7fjMwxNHNlkTDzYW6
'' SIG '' kXe+Jc1HcNU6VL0kfjHl6Z8g1rW65JpzoXgJ4kIPUZqR
'' SIG '' 9LsPlrI2xpnZ76wFSHrYpVOWESxBEdlHAJPFuLHVjiIn
'' SIG '' D48M0tzQd/X2pfZeJfS7ZIz0JZNOOzP1K8KMgpLEJkUI
'' SIG '' 2//OkoiWwfHuFA1AdIxsqHT/DCfzq6IgAsSNrNSzMTT5
'' SIG '' fqtw5sN9TiH87/S+ZsXcExH7jmsBkwARMmxEM/EckKj/
'' SIG '' lcaFZ2D8ugnldYGs4Mvjhg2s3sVGccQACvTqx+Wpnx55
'' SIG '' XcW4Mp0/mHX1ZScZbA7Uf9mNTM6hUaJXeDCCB3EwggVZ
'' SIG '' oAMCAQICEzMAAAAVxedrngKbSZkAAAAAABUwDQYJKoZI
'' SIG '' hvcNAQELBQAwgYgxCzAJBgNVBAYTAlVTMRMwEQYDVQQI
'' SIG '' EwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4w
'' SIG '' HAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xMjAw
'' SIG '' BgNVBAMTKU1pY3Jvc29mdCBSb290IENlcnRpZmljYXRl
'' SIG '' IEF1dGhvcml0eSAyMDEwMB4XDTIxMDkzMDE4MjIyNVoX
'' SIG '' DTMwMDkzMDE4MzIyNVowfDELMAkGA1UEBhMCVVMxEzAR
'' SIG '' BgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1v
'' SIG '' bmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlv
'' SIG '' bjEmMCQGA1UEAxMdTWljcm9zb2Z0IFRpbWUtU3RhbXAg
'' SIG '' UENBIDIwMTAwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAw
'' SIG '' ggIKAoICAQDk4aZM57RyIQt5osvXJHm9DtWC0/3unAcH
'' SIG '' 0qlsTnXIyjVX9gF/bErg4r25PhdgM/9cT8dm95VTcVri
'' SIG '' fkpa/rg2Z4VGIwy1jRPPdzLAEBjoYH1qUoNEt6aORmsH
'' SIG '' FPPFdvWGUNzBRMhxXFExN6AKOG6N7dcP2CZTfDlhAnrE
'' SIG '' qv1yaa8dq6z2Nr41JmTamDu6GnszrYBbfowQHJ1S/rbo
'' SIG '' YiXcag/PXfT+jlPP1uyFVk3v3byNpOORj7I5LFGc6XBp
'' SIG '' Dco2LXCOMcg1KL3jtIckw+DJj361VI/c+gVVmG1oO5pG
'' SIG '' ve2krnopN6zL64NF50ZuyjLVwIYwXE8s4mKyzbnijYjk
'' SIG '' lqwBSru+cakXW2dg3viSkR4dPf0gz3N9QZpGdc3EXzTd
'' SIG '' EonW/aUgfX782Z5F37ZyL9t9X4C626p+Nuw2TPYrbqgS
'' SIG '' Uei/BQOj0XOmTTd0lBw0gg/wEPK3Rxjtp+iZfD9M269e
'' SIG '' wvPV2HM9Q07BMzlMjgK8QmguEOqEUUbi0b1qGFphAXPK
'' SIG '' Z6Je1yh2AuIzGHLXpyDwwvoSCtdjbwzJNmSLW6CmgyFd
'' SIG '' XzB0kZSU2LlQ+QuJYfM2BjUYhEfb3BvR/bLUHMVr9lxS
'' SIG '' UV0S2yW6r1AFemzFER1y7435UsSFF5PAPBXbGjfHCBUY
'' SIG '' P3irRbb1Hode2o+eFnJpxq57t7c+auIurQIDAQABo4IB
'' SIG '' 3TCCAdkwEgYJKwYBBAGCNxUBBAUCAwEAATAjBgkrBgEE
'' SIG '' AYI3FQIEFgQUKqdS/mTEmr6CkTxGNSnPEP8vBO4wHQYD
'' SIG '' VR0OBBYEFJ+nFV0AXmJdg/Tl0mWnG1M1GelyMFwGA1Ud
'' SIG '' IARVMFMwUQYMKwYBBAGCN0yDfQEBMEEwPwYIKwYBBQUH
'' SIG '' AgEWM2h0dHA6Ly93d3cubWljcm9zb2Z0LmNvbS9wa2lv
'' SIG '' cHMvRG9jcy9SZXBvc2l0b3J5Lmh0bTATBgNVHSUEDDAK
'' SIG '' BggrBgEFBQcDCDAZBgkrBgEEAYI3FAIEDB4KAFMAdQBi
'' SIG '' AEMAQTALBgNVHQ8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB
'' SIG '' /zAfBgNVHSMEGDAWgBTV9lbLj+iiXGJo0T2UkFvXzpoY
'' SIG '' xDBWBgNVHR8ETzBNMEugSaBHhkVodHRwOi8vY3JsLm1p
'' SIG '' Y3Jvc29mdC5jb20vcGtpL2NybC9wcm9kdWN0cy9NaWNS
'' SIG '' b29DZXJBdXRfMjAxMC0wNi0yMy5jcmwwWgYIKwYBBQUH
'' SIG '' AQEETjBMMEoGCCsGAQUFBzAChj5odHRwOi8vd3d3Lm1p
'' SIG '' Y3Jvc29mdC5jb20vcGtpL2NlcnRzL01pY1Jvb0NlckF1
'' SIG '' dF8yMDEwLTA2LTIzLmNydDANBgkqhkiG9w0BAQsFAAOC
'' SIG '' AgEAnVV9/Cqt4SwfZwExJFvhnnJL/Klv6lwUtj5OR2R4
'' SIG '' sQaTlz0xM7U518JxNj/aZGx80HU5bbsPMeTCj/ts0aGU
'' SIG '' GCLu6WZnOlNN3Zi6th542DYunKmCVgADsAW+iehp4LoJ
'' SIG '' 7nvfam++Kctu2D9IdQHZGN5tggz1bSNU5HhTdSRXud2f
'' SIG '' 8449xvNo32X2pFaq95W2KFUn0CS9QKC/GbYSEhFdPSfg
'' SIG '' QJY4rPf5KYnDvBewVIVCs/wMnosZiefwC2qBwoEZQhlS
'' SIG '' dYo2wh3DYXMuLGt7bj8sCXgU6ZGyqVvfSaN0DLzskYDS
'' SIG '' PeZKPmY7T7uG+jIa2Zb0j/aRAfbOxnT99kxybxCrdTDF
'' SIG '' NLB62FD+CljdQDzHVG2dY3RILLFORy3BFARxv2T5JL5z
'' SIG '' bcqOCb2zAVdJVGTZc9d/HltEAY5aGZFrDZ+kKNxnGSgk
'' SIG '' ujhLmm77IVRrakURR6nxt67I6IleT53S0Ex2tVdUCbFp
'' SIG '' AUR+fKFhbHP+CrvsQWY9af3LwUFJfn6Tvsv4O+S3Fb+0
'' SIG '' zj6lMVGEvL8CwYKiexcdFYmNcP7ntdAoGokLjzbaukz5
'' SIG '' m/8K6TT4JDVnK+ANuOaMmdbhIurwJ0I9JZTmdHRbatGe
'' SIG '' Pu1+oDEzfbzL6Xu/OHBE0ZDxyKs6ijoIYn/ZcGNTTY3u
'' SIG '' gm2lBRDBcQZqELQdVTNYs6FwZvKhggNZMIICQQIBATCC
'' SIG '' AQGhgdmkgdYwgdMxCzAJBgNVBAYTAlVTMRMwEQYDVQQI
'' SIG '' EwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4w
'' SIG '' HAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xLTAr
'' SIG '' BgNVBAsTJE1pY3Jvc29mdCBJcmVsYW5kIE9wZXJhdGlv
'' SIG '' bnMgTGltaXRlZDEnMCUGA1UECxMeblNoaWVsZCBUU1Mg
'' SIG '' RVNOOjQwMUEtMDVFMC1EOTQ3MSUwIwYDVQQDExxNaWNy
'' SIG '' b3NvZnQgVGltZS1TdGFtcCBTZXJ2aWNloiMKAQEwBwYF
'' SIG '' Kw4DAhoDFQCEY0cP9rtDRtAtZUb0m4bGAtFex6CBgzCB
'' SIG '' gKR+MHwxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNo
'' SIG '' aW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQK
'' SIG '' ExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xJjAkBgNVBAMT
'' SIG '' HU1pY3Jvc29mdCBUaW1lLVN0YW1wIFBDQSAyMDEwMA0G
'' SIG '' CSqGSIb3DQEBCwUAAgUA6uLeiDAiGA8yMDI0MTExNjA5
'' SIG '' MDQwOFoYDzIwMjQxMTE3MDkwNDA4WjB3MD0GCisGAQQB
'' SIG '' hFkKBAExLzAtMAoCBQDq4t6IAgEAMAoCAQACAgHbAgH/
'' SIG '' MAcCAQACAhMSMAoCBQDq5DAIAgEAMDYGCisGAQQBhFkK
'' SIG '' BAIxKDAmMAwGCisGAQQBhFkKAwKgCjAIAgEAAgMHoSCh
'' SIG '' CjAIAgEAAgMBhqAwDQYJKoZIhvcNAQELBQADggEBAKh+
'' SIG '' IDrKdkAMtyb0I9NcqHe71Pxs4fRaQXGrzA5o4wbrOLmh
'' SIG '' zrBNmLuDurf58g5bHvu2/sKKvjnGo8MWusDno6IuGyhn
'' SIG '' mDMxmwrK4zTZhLvM1esb+f2MWD0BBI74qfwBAfegixW/
'' SIG '' t3Jk9tm6bFvm8NCj0rGPY6OCU/ys3wEUGWcrZJuZXsGK
'' SIG '' LmPbin2+QHjrJ+AmjfegQUrV6JSxERlZASzwZ+LmisQM
'' SIG '' vFu01dR2QERMm4rRxuNLJnToRnyX1MMc7dHi8pk5qG42
'' SIG '' adpI9xgE4w90wO8Dy/jcY7S1Kg3/pz5gH7uCP4cdBnB0
'' SIG '' o1Vkn1MBKWbXr7kG7imnrsoQ/fLAShoxggQNMIIECQIB
'' SIG '' ATCBkzB8MQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2Fz
'' SIG '' aGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UE
'' SIG '' ChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMSYwJAYDVQQD
'' SIG '' Ex1NaWNyb3NvZnQgVGltZS1TdGFtcCBQQ0EgMjAxMAIT
'' SIG '' MwAAAf7QqMJ7NCELAQABAAAB/jANBglghkgBZQMEAgEF
'' SIG '' AKCCAUowGgYJKoZIhvcNAQkDMQ0GCyqGSIb3DQEJEAEE
'' SIG '' MC8GCSqGSIb3DQEJBDEiBCBPq/nFQmmhVib2h7JSrkT0
'' SIG '' P2ngHWtkHbzxKICd2f2MADCB+gYLKoZIhvcNAQkQAi8x
'' SIG '' geowgecwgeQwgb0EIBGFzN38U8ifGNH3abaE9apz68Y4
'' SIG '' bX78jRa2QKy3KHR5MIGYMIGApH4wfDELMAkGA1UEBhMC
'' SIG '' VVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcT
'' SIG '' B1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jw
'' SIG '' b3JhdGlvbjEmMCQGA1UEAxMdTWljcm9zb2Z0IFRpbWUt
'' SIG '' U3RhbXAgUENBIDIwMTACEzMAAAH+0KjCezQhCwEAAQAA
'' SIG '' Af4wIgQgvYCeql4H3suUXn/y91PgGwzQbO0Fv+nID/8p
'' SIG '' 1CxMjnowDQYJKoZIhvcNAQELBQAEggIApj1Mn2FboR7o
'' SIG '' yUj2VrhFxgGFUsSMPLdqRTE/sOIEq30jn/hlUVSl4+6b
'' SIG '' X5o4UNO//N0lgGQ+sru6L4GqycKHOMfgewYGI8T9jKEZ
'' SIG '' ecMKZ33FWVW8bUBbgZq5QJuBDNxJufIAl2OcmN4epIc/
'' SIG '' 9zEHUzpGII+YpRYynzIi0oYQ3Z/rWxRlXvqSUUPtCmmd
'' SIG '' GtlQaMrWdCHUlvee8ozTFgwvqPyGJGkX5XSjkcwMb1HK
'' SIG '' Cpf7tG7vt4HKzDPLiqOGUrugVbfDu7D+dVit3GcmFYhv
'' SIG '' rhFD20ldDx6mB2NFoHGyGOzaRlHj/S4QmB6QHRGiSMjA
'' SIG '' YNnksqGu3b6LTmYPAzI0TzgGdaimYIhaP273grVScbvG
'' SIG '' wGhet5RHNqOGebbvjzbBlbZKWgp5QGaW9ZVP/vUo5eWm
'' SIG '' PYnDBmvq7hlLE5aaqN2xphd+UwsSg2IteJ1pyAULm+OK
'' SIG '' wZyT5nOVFodmmwIMrZNAX8G1DnFt+e79QYaKL44QCAcX
'' SIG '' MDy62jn33V5rwO8+q67K6h4sWQmmwCb4fbaPCwljfwCU
'' SIG '' tBq6YfsXhNor0GwSPWWvBJVHmzCcTXySs7r8l+ZGM4h7
'' SIG '' l8NEpZZRSk4L+CDj+7LpiQRPa4uAHl1zpTNMn8f0h5HH
'' SIG '' GFVniJTCRjnoCnjXrbKj2FhoAIvR0NwY0FjkeSz6jWkX
'' SIG '' RYeYrz3fKKA=
'' SIG '' End signature block
