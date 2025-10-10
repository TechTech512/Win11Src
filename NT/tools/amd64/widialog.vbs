' Windows Installer utility to preview dialogs from a install database
' For use with Windows Scripting Host, CScript.exe or WScript.exe
' Copyright (c) Microsoft Corporation. All rights reserved.
' Demonstrates the use of preview APIs
'
Option Explicit

Const msiOpenDatabaseModeReadOnly = 0

' Show help if no arguments or if argument contains ?
Dim argCount : argCount = Wscript.Arguments.Count
If argCount > 0 Then If InStr(1, Wscript.Arguments(0), "?", vbTextCompare) > 0 Then argCount = 0
If argCount = 0 Then
	Wscript.Echo "Windows Installer utility to preview dialogs from an install database." &_
		vbLf & " The 1st argument is the path to an install database, relative or complete path" &_
		vbLf & " Subsequent arguments are dialogs to display (primary key of Dialog table)" &_
		vbLf & " To show a billboard, append the Control name (Control table key) and Billboard" &_
		vbLf & "       name (Billboard table key) to the Dialog name, separated with colons." &_
		vbLf & " If no dialogs specified, all dialogs in Dialog table are displayed sequentially" &_
		vbLf & " Note: The name of the dialog, if provided,  is case-sensitive" &_
		vblf &_
		vblf & "Copyright (C) Microsoft Corporation.  All rights reserved."
	Wscript.Quit 1
End If

' Connect to Windows Installer object
On Error Resume Next
Dim installer : Set installer = Nothing
Set installer = Wscript.CreateObject("WindowsInstaller.Installer") : CheckError

' Open database
Dim databasePath : databasePath = Wscript.Arguments(0)
Dim database : Set database = installer.OpenDatabase(databasePath, msiOpenDatabaseModeReadOnly) : CheckError

' Create preview object
Dim preview : Set preview = Database.EnableUIpreview : CheckError

' Get properties from Property table and put into preview object
Dim record, view : Set view = database.OpenView("SELECT `Property`,`Value` FROM `Property`") : CheckError
view.Execute : CheckError
Do
	Set record = view.Fetch : CheckError
	If record Is Nothing Then Exit Do
	preview.Property(record.StringData(1)) = record.StringData(2) : CheckError
Loop

' Loop through list of dialog names and display each one
If argCount = 1 Then ' No dialog name, loop through all dialogs
	Set view = database.OpenView("SELECT `Dialog` FROM `Dialog`") : CheckError
	view.Execute : CheckError
	Do
		Set record = view.Fetch : CheckError
		If record Is Nothing Then Exit Do
		preview.ViewDialog(record.StringData(1)) : CheckError
		Wait
	Loop
Else ' explicit dialog names supplied
	Set view = database.OpenView("SELECT `Dialog` FROM `Dialog` WHERE `Dialog`=?") : CheckError
	Dim paramRecord, argNum, argArray, dialogName, controlName, billboardName
	Set paramRecord = installer.CreateRecord(1)
	For argNum = 1 To argCount-1
		dialogName = Wscript.Arguments(argNum)
		argArray = Split(dialogName,":",-1,vbTextCompare)
		If UBound(argArray) <> 0 Then  ' billboard to add to dialog
			If UBound(argArray) <> 2 Then Fail "Incorrect billboard syntax, must specify 3 values"
			dialogName    = argArray(0)
			controlName   = argArray(1) ' we could validate that controlName is in the Control table
			billboardName = argArray(2) ' we could validate that billboard is in the Billboard table
		End If
		paramRecord.StringData(1) = dialogName
		view.Execute paramRecord : CheckError
		If view.Fetch Is Nothing Then Fail "Dialog not found: " & dialogName
		preview.ViewDialog(dialogName) : CheckError
		If UBound(argArray) = 2 Then preview.ViewBillboard controlName, billboardName : CheckError
		Wait
	Next
End If
preview.ViewDialog ""  ' clear dialog, must do this to release object deadlock

' Wait until user input to clear dialog. Too bad there's no function to wait for keyboard input
Sub Wait
	Dim shell : Set shell = Wscript.CreateObject("Wscript.Shell")
	MsgBox "Next",0,"Drag me away"
End Sub

Sub CheckError
	Dim message, errRec
	If Err = 0 Then Exit Sub
	message = Err.Source & " " & Hex(Err) & ": " & Err.Description
	If Not installer Is Nothing Then
		Set errRec = installer.LastErrorRecord
		If Not errRec Is Nothing Then message = message & vbLf & errRec.FormatText
	End If
	Fail message
End Sub

Sub Fail(message)
	Wscript.Echo message
	Wscript.Quit 2
End Sub

'' SIG '' Begin signature block
'' SIG '' MIImWQYJKoZIhvcNAQcCoIImSjCCJkYCAQExDzANBglg
'' SIG '' hkgBZQMEAgEFADB3BgorBgEEAYI3AgEEoGkwZzAyBgor
'' SIG '' BgEEAYI3AgEeMCQCAQEEEE7wKRaZJ7VNj+Ws4Q8X66sC
'' SIG '' AQACAQACAQACAQACAQAwMTANBglghkgBZQMEAgEFAAQg
'' SIG '' mOr7DzNLA7B3kQygPHKkFo0lJ4ImjipM2G/ZKh4w1cKg
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
'' SIG '' MYIaOzCCGjcCAQEwgZUwfjELMAkGA1UEBhMCVVMxEzAR
'' SIG '' BgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1v
'' SIG '' bmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlv
'' SIG '' bjEoMCYGA1UEAxMfTWljcm9zb2Z0IENvZGUgU2lnbmlu
'' SIG '' ZyBQQ0EgMjAxMAITMwAABaZYEGdLPWx89gAAAAAFpjAN
'' SIG '' BglghkgBZQMEAgEFAKCBxjAZBgkqhkiG9w0BCQMxDAYK
'' SIG '' KwYBBAGCNwIBBDAcBgorBgEEAYI3AgELMQ4wDAYKKwYB
'' SIG '' BAGCNwIBFTAvBgkqhkiG9w0BCQQxIgQgQEDcxtKduE1D
'' SIG '' dS/GQ0Sp90VjaMMkhCrwmuQvbvZ44LowWgYKKwYBBAGC
'' SIG '' NwIBDDFMMEqgJIAiAE0AaQBjAHIAbwBzAG8AZgB0ACAA
'' SIG '' VwBpAG4AZABvAHcAc6EigCBodHRwOi8vd3d3Lm1pY3Jv
'' SIG '' c29mdC5jb20vd2luZG93czANBgkqhkiG9w0BAQEFAASC
'' SIG '' AQAQIXFcorQkt6WXIXgPl9VY98OIGMwL3yRAvGi/wWmN
'' SIG '' bA+nkDTrEEQYrAAc3LUO9MdJGGGwqr/bLCiS0KjLEx/v
'' SIG '' Lg7lWpqduZ3ndA2W7BBkpqqC4bHfKrEeToM2yBnXHw3i
'' SIG '' sU159I5vBu7J6Gye8kdAQlBnB8GnoTfJYfg49iQpk+K+
'' SIG '' bxAJrB92KZ214BITu5GkTHuRFCzqgg4UGERng47eVY5o
'' SIG '' TkgYdN0T8mhpXSyXZ/r4ZEBXwHLDq/4bHLU8ns9fiQne
'' SIG '' Nx5YUg1gsxv3yqeLUlNDmOksTwL9IE5H3+W8PVQkoSak
'' SIG '' znNWBgvRKt3bNOHkI2+oOj585o/Gmc02xh71oYIXrTCC
'' SIG '' F6kGCisGAQQBgjcDAwExgheZMIIXlQYJKoZIhvcNAQcC
'' SIG '' oIIXhjCCF4ICAQMxDzANBglghkgBZQMEAgEFADCCAVoG
'' SIG '' CyqGSIb3DQEJEAEEoIIBSQSCAUUwggFBAgEBBgorBgEE
'' SIG '' AYRZCgMBMDEwDQYJYIZIAWUDBAIBBQAEIAFNchZxPGCM
'' SIG '' c8FD5cxnT9VOPIJsw6gkjt/2fwQUXvn3AgZm6yNVpbQY
'' SIG '' EzIwMjQxMTE2MDkxNjQ1Ljk0MlowBIACAfSggdmkgdYw
'' SIG '' gdMxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5n
'' SIG '' dG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVN
'' SIG '' aWNyb3NvZnQgQ29ycG9yYXRpb24xLTArBgNVBAsTJE1p
'' SIG '' Y3Jvc29mdCBJcmVsYW5kIE9wZXJhdGlvbnMgTGltaXRl
'' SIG '' ZDEnMCUGA1UECxMeblNoaWVsZCBUU1MgRVNOOjM2MDUt
'' SIG '' MDVFMC1EOTQ3MSUwIwYDVQQDExxNaWNyb3NvZnQgVGlt
'' SIG '' ZS1TdGFtcCBTZXJ2aWNloIIR+zCCBygwggUQoAMCAQIC
'' SIG '' EzMAAAH3WCB1BMr7wvQAAQAAAfcwDQYJKoZIhvcNAQEL
'' SIG '' BQAwfDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hp
'' SIG '' bmd0b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoT
'' SIG '' FU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEmMCQGA1UEAxMd
'' SIG '' TWljcm9zb2Z0IFRpbWUtU3RhbXAgUENBIDIwMTAwHhcN
'' SIG '' MjQwNzI1MTgzMTA2WhcNMjUxMDIyMTgzMTA2WjCB0zEL
'' SIG '' MAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24x
'' SIG '' EDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jv
'' SIG '' c29mdCBDb3Jwb3JhdGlvbjEtMCsGA1UECxMkTWljcm9z
'' SIG '' b2Z0IElyZWxhbmQgT3BlcmF0aW9ucyBMaW1pdGVkMScw
'' SIG '' JQYDVQQLEx5uU2hpZWxkIFRTUyBFU046MzYwNS0wNUUw
'' SIG '' LUQ5NDcxJTAjBgNVBAMTHE1pY3Jvc29mdCBUaW1lLVN0
'' SIG '' YW1wIFNlcnZpY2UwggIiMA0GCSqGSIb3DQEBAQUAA4IC
'' SIG '' DwAwggIKAoICAQDQ50dME2ibr+5cpoQo/2s8hORPpDEX
'' SIG '' XW2PMHQ2TVvIOk+sVMeFreHHBJ1NyvxjRreToRCXCrLp
'' SIG '' E7PjZ7RHl4Nb50KhBqmKkMgVQ5ineg26rBf/F6kBMSRj
'' SIG '' XszJcXHqtBbY1xZQlbdCjYC4nQc61uVKki1Bk8aYecaq
'' SIG '' S38MHjkXDGTpWhK/E1xAqEoROS7Ou3xToNFxxCbUV2GY
'' SIG '' 8qAPOBx8M8zmj4afNuIy7rLTr0DgQeYsyaR5xKRW8GZx
'' SIG '' nxWfMUdMOQYt2mcNXkVeNU5sCBtIzRyephIZ9GntUYcF
'' SIG '' GrKixy9HhtxD4JX2kONsnpLmtmfW4DyFGGPT0ezfcdF6
'' SIG '' +3ihYBVgYi2ASwb4GsJhumBYwMQhWcCA9kSI8BojzAEZ
'' SIG '' 6YTh94SS7PtMDCCREFxTMuBDi68+pEPUD4mS3br6kOpZ
'' SIG '' hKfQwDyPTNpxCT2r8C9yI9cP0i3Z7P6aoTOAVFGwkYu1
'' SIG '' x/0eSy8rwmx3ojnMVKGWqLlunN/Vjg06I06HlDBbWki8
'' SIG '' DmKuVqXuoWGQB555mqainz643FlfEUJAbdHezmldbz0W
'' SIG '' IKH2uZetWo4LCBxcUglABCSWUqwj5Qmoar2uZEAEnPmU
'' SIG '' cpMViYXBwznYpZaM3HfPqh3DPaH6zFrF7BOh70aq0PHf
'' SIG '' 9pT7Ko1FwHzDS1JdR/7KU3i6TnEcSkunH5k02wIDAQAB
'' SIG '' o4IBSTCCAUUwHQYDVR0OBBYEFN9GpDM/eb09la4t/Wnz
'' SIG '' +Z4V+SaYMB8GA1UdIwQYMBaAFJ+nFV0AXmJdg/Tl0mWn
'' SIG '' G1M1GelyMF8GA1UdHwRYMFYwVKBSoFCGTmh0dHA6Ly93
'' SIG '' d3cubWljcm9zb2Z0LmNvbS9wa2lvcHMvY3JsL01pY3Jv
'' SIG '' c29mdCUyMFRpbWUtU3RhbXAlMjBQQ0ElMjAyMDEwKDEp
'' SIG '' LmNybDBsBggrBgEFBQcBAQRgMF4wXAYIKwYBBQUHMAKG
'' SIG '' UGh0dHA6Ly93d3cubWljcm9zb2Z0LmNvbS9wa2lvcHMv
'' SIG '' Y2VydHMvTWljcm9zb2Z0JTIwVGltZS1TdGFtcCUyMFBD
'' SIG '' QSUyMDIwMTAoMSkuY3J0MAwGA1UdEwEB/wQCMAAwFgYD
'' SIG '' VR0lAQH/BAwwCgYIKwYBBQUHAwgwDgYDVR0PAQH/BAQD
'' SIG '' AgeAMA0GCSqGSIb3DQEBCwUAA4ICAQA3RqNp8gt4vpJA
'' SIG '' gwgwBczVB3rFqhyLaY6ulHy8pbLJOwvdvzcDtcYuIBtD
'' SIG '' FOuqde9VZZ42y3lhAPyxo75ROA4sl1N19QAOEtegr5GX
'' SIG '' CN+d2KYglP0wf21RhcvMlcqFkzT2i4/A2yufxg4sil0C
'' SIG '' LlM/I3wKXXU4ZlKU/2vwme+iZbTQCgng+X2uWDQbmVxC
'' SIG '' ScBeodr2dB1anVnFeo137QmwqaVHy1wA1ffcKUz02doK
'' SIG '' UkTEtAeIp4dRRa2rIsyXrlNbrBEzteUXtj49OcLx241a
'' SIG '' fi4ueD4439nf0Y7qoGPsgRnGirijdq8SH1trjdRTpODN
'' SIG '' VloGbxVoDTBLBR7+mqlM5gVY3rZcveCX8kLanN8g/E/r
'' SIG '' pd9EsjFp+MFVebwpUOfZwwv0i9ErTaz3jVjn5FHiBIA6
'' SIG '' EuJBDoDTdU1G6n6ykxrST5dM8CL7ZowfnFrVmNv8ry71
'' SIG '' /0zTlTT9tQwlckM/77KxakltVEOIcbuzNpxr6vceJQ+N
'' SIG '' AnJCXY2I5xhMZX8NwussIErbMbnTcUZvTg3kp/XReADA
'' SIG '' VpeWh3kH14qH3k+dcrHYs0GAvAbzlqeWGEbHEFDmYWwk
'' SIG '' aQGfQ9k+0DNnJ+v3qrHOmnakf0MklyMoIOsyZnOJdrOl
'' SIG '' rlVU3foI7WQNTgAGRJhNc4zxGYle5CbuZQXdtaaP6GMA
'' SIG '' lvinPqFPlTCCB3EwggVZoAMCAQICEzMAAAAVxedrngKb
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
'' SIG '' ZvKhggNWMIICPgIBATCCAQGhgdmkgdYwgdMxCzAJBgNV
'' SIG '' BAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYD
'' SIG '' VQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQg
'' SIG '' Q29ycG9yYXRpb24xLTArBgNVBAsTJE1pY3Jvc29mdCBJ
'' SIG '' cmVsYW5kIE9wZXJhdGlvbnMgTGltaXRlZDEnMCUGA1UE
'' SIG '' CxMeblNoaWVsZCBUU1MgRVNOOjM2MDUtMDVFMC1EOTQ3
'' SIG '' MSUwIwYDVQQDExxNaWNyb3NvZnQgVGltZS1TdGFtcCBT
'' SIG '' ZXJ2aWNloiMKAQEwBwYFKw4DAhoDFQBvbwoMb/Fds0GO
'' SIG '' Yzv+erDduCsQ5qCBgzCBgKR+MHwxCzAJBgNVBAYTAlVT
'' SIG '' MRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdS
'' SIG '' ZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9y
'' SIG '' YXRpb24xJjAkBgNVBAMTHU1pY3Jvc29mdCBUaW1lLVN0
'' SIG '' YW1wIFBDQSAyMDEwMA0GCSqGSIb3DQEBCwUAAgUA6uK9
'' SIG '' pjAiGA8yMDI0MTExNjA2NDM1MFoYDzIwMjQxMTE3MDY0
'' SIG '' MzUwWjB0MDoGCisGAQQBhFkKBAExLDAqMAoCBQDq4r2m
'' SIG '' AgEAMAcCAQACAiOuMAcCAQACAhJUMAoCBQDq5A8mAgEA
'' SIG '' MDYGCisGAQQBhFkKBAIxKDAmMAwGCisGAQQBhFkKAwKg
'' SIG '' CjAIAgEAAgMHoSChCjAIAgEAAgMBhqAwDQYJKoZIhvcN
'' SIG '' AQELBQADggEBAABd94N4HJzxTczvt4JjDUJDWg52qeAK
'' SIG '' jFUrkv/8g7P8SnJYHDHzSONz26F9uCu+1+boa8x6vYtT
'' SIG '' S6NJkh/jQrjfRemSZU5zq+8SbwlvlcAyh8L1Z2Y2iMf5
'' SIG '' FabTQVP7P0FtwQGFJ9V4TWRjbt0ifn8Vzk6+583afxqw
'' SIG '' OunLKmxKVvPPqAkjBM4KTPG9+XzbAvunxk7ba7SHUezp
'' SIG '' QjBcDkH8FIUa9KG8BY8Vv/6W/3wMb6jK2pEo5g2zAqKA
'' SIG '' NczBThB6kIxlbz3p24wtcXecjtAyVN+UvY/0vrSHHFUp
'' SIG '' GDYuwx+U5PtIvvHqhzFEzy+a6dMKik4DThqU8Z5PHyu2
'' SIG '' hHIxggQNMIIECQIBATCBkzB8MQswCQYDVQQGEwJVUzET
'' SIG '' MBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMHUmVk
'' SIG '' bW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBvcmF0
'' SIG '' aW9uMSYwJAYDVQQDEx1NaWNyb3NvZnQgVGltZS1TdGFt
'' SIG '' cCBQQ0EgMjAxMAITMwAAAfdYIHUEyvvC9AABAAAB9zAN
'' SIG '' BglghkgBZQMEAgEFAKCCAUowGgYJKoZIhvcNAQkDMQ0G
'' SIG '' CyqGSIb3DQEJEAEEMC8GCSqGSIb3DQEJBDEiBCCMr2qW
'' SIG '' jIdYIB393KC6TPB17zRyCHwNCuF1fs3ARj+N6DCB+gYL
'' SIG '' KoZIhvcNAQkQAi8xgeowgecwgeQwgb0EICHamNprdxrR
'' SIG '' 5xi6G7rS5gc/8gqc9t51tVAnlKggflniMIGYMIGApH4w
'' SIG '' fDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0
'' SIG '' b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1p
'' SIG '' Y3Jvc29mdCBDb3Jwb3JhdGlvbjEmMCQGA1UEAxMdTWlj
'' SIG '' cm9zb2Z0IFRpbWUtU3RhbXAgUENBIDIwMTACEzMAAAH3
'' SIG '' WCB1BMr7wvQAAQAAAfcwIgQgcfFgQeoOTvzmDaIHhwUh
'' SIG '' 7yklelKPZD6y3DUG49HTbYYwDQYJKoZIhvcNAQELBQAE
'' SIG '' ggIATD3xX7NPZFD2iDfbdcB1dF8rrtNnAvzKH7uWSSmf
'' SIG '' FdmxA4tXEyU+84JRwdyaauPa/Zq6p00T16xTavWtuSOB
'' SIG '' n7RFokIAMOdgtHTlo01aMrRksI1OqrgQhgP3KYklumk0
'' SIG '' pXxGsIrqWIZFDwGjARlCma/RaLQ/SQ/GMIhSQl3u//kH
'' SIG '' VzR+QutYiCI5kv1M9Ci0D0WdhO91PGH4sQwWMjVOw6A5
'' SIG '' JpVcF5xaW4wcRp9UpfAlikImLWo/VL+2xPmfpd42FzMA
'' SIG '' csYSxt2txgYg+JBrQhq57REj6UIgs6OouODik1ulKzzD
'' SIG '' UoDP4lCwF1P26yQ2uY8CWWIf0YEmdt1dGEExXPsri57R
'' SIG '' djENYhx09siGdn5EUqVXQKSSm0Nj4P2GSPjRhz8sullJ
'' SIG '' 7QqowGp0zwuIMvPMURaRI1qCshH5NnkCC8KtVI9fJUMs
'' SIG '' dMK7SAaIvQTMxJ2LXTVojmBSc0dg9BQA7JRtdx8T0j/n
'' SIG '' FPscqGE/KmOUlxJMBpPhJCSVzfJUSZHGwrIIJM5LxFRk
'' SIG '' BxBcadshAnyZFHrZtq274qX5aMWKXqKyz7YrjvkI6Kxw
'' SIG '' yqlEQa6oCYBdH7T8z7A8Wn7O+d/Y3CpTVRXkpFUzX4vm
'' SIG '' qMOWftA5Z46tIXdjEpeMvntWoPLUHDnkPXCL3lchXXY+
'' SIG '' /SAjh/XsEG143wu0JTLgzH3HWsM=
'' SIG '' End signature block
