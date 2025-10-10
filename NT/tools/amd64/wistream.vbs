' Windows Installer utility to manage binary streams in an installer package
' For use with Windows Scripting Host, CScript.exe or WScript.exe
' Copyright (c) Microsoft Corporation. All rights reserved.
' Demonstrates the use of the database _Streams table
' Used for entering non-database binary streams such as compressed file cabinets
' Streams that persist database binary values should be managed with table views
' Streams that persist database tables and system data are invisible in _Streams
'
Option Explicit

Const msiOpenDatabaseModeReadOnly     = 0
Const msiOpenDatabaseModeTransact     = 1
Const msiOpenDatabaseModeCreate       = 3

Const msiViewModifyInsert         = 1
Const msiViewModifyUpdate         = 2
Const msiViewModifyAssign         = 3
Const msiViewModifyReplace        = 4
Const msiViewModifyDelete         = 6

Const ForAppending = 8
Const ForReading = 1
Const ForWriting = 2
Const TristateTrue = -1

' Check arg count, and display help if argument not present or contains ?
Dim argCount:argCount = Wscript.Arguments.Count
If argCount > 0 Then If InStr(1, Wscript.Arguments(0), "?", vbTextCompare) > 0 Then argCount = 0
If (argCount = 0) Then
	Wscript.Echo "Windows Installer database stream import utility" &_
		vbNewLine & " 1st argument is the path to MSI database (installer package)" &_
		vbNewLine & " 2nd argument is the path to a file containing the stream data" &_
		vbNewLine & " If the 2nd argument is missing, streams will be listed" &_
		vbNewLine & " 3rd argument is optional, the name used for the stream" &_
		vbNewLine & " If the 3rd arugment is missing, the file name is used" &_
		vbNewLine & " To remove a stream, use /D or -D as the 2nd argument" &_
		vbNewLine & " followed by the name of the stream in the 3rd argument" &_
		vbNewLine &_
		vbNewLine & "Copyright (C) Microsoft Corporation.  All rights reserved."
	Wscript.Quit 1
End If

' Connect to Windows Installer object
On Error Resume Next
Dim installer : Set installer = Nothing
Set installer = Wscript.CreateObject("WindowsInstaller.Installer") : CheckError

' Evaluate command-line arguments and set open and update modes
Dim databasePath:databasePath = Wscript.Arguments(0)
Dim openMode    : If argCount = 1 Then openMode = msiOpenDatabaseModeReadOnly Else openMode = msiOpenDatabaseModeTransact
Dim updateMode  : If argCount > 1 Then updateMode = msiViewModifyAssign  'Either insert or replace existing row
Dim importPath  : If argCount > 1 Then importPath = Wscript.Arguments(1)
Dim streamName  : If argCount > 2 Then streamName = Wscript.Arguments(2)
If streamName = Empty And importPath <> Empty Then streamName = Right(importPath, Len(importPath) - InStrRev(importPath, "\",-1,vbTextCompare))
If UCase(importPath) = "/D" Or UCase(importPath) = "-D" Then updateMode = msiViewModifyDelete : importPath = Empty 'Stream will be deleted if no input data

' Open database and create a view on the _Streams table
Dim sqlQuery : Select Case updateMode
	Case msiOpenDatabaseModeReadOnly: sqlQuery = "SELECT `Name` FROM _Streams"
	Case msiViewModifyAssign:         sqlQuery = "SELECT `Name`,`Data` FROM _Streams"
	Case msiViewModifyDelete:         sqlQuery = "SELECT `Name` FROM _Streams WHERE `Name` = ?"
End Select
Dim database : Set database = installer.OpenDatabase(databasePath, openMode) : CheckError
Dim view     : Set view = database.OpenView(sqlQuery)
Dim record

If openMode = msiOpenDatabaseModeReadOnly Then 'If listing streams, simply fetch all records
	Dim message, name
	view.Execute : CheckError
	Do
		Set record = view.Fetch
		If record Is Nothing Then Exit Do
		name = record.StringData(1)
		If message = Empty Then message = name Else message = message & vbNewLine & name
	Loop
	Wscript.Echo message
Else 'If adding a stream, insert a row, else if removing a stream, delete the row
	Set record = installer.CreateRecord(2)
	record.StringData(1) = streamName
	view.Execute record : CheckError
	If importPath <> Empty Then  'Insert stream - copy data into stream
		record.SetStream 2, importPath : CheckError
	Else  'Delete stream, fetch first to provide better error message if missing
		Set record = view.Fetch
		If record Is Nothing Then Wscript.Echo "Stream not present:", streamName : Wscript.Quit 2
	End If
	view.Modify updateMode, record : CheckError
	database.Commit : CheckError
	Set view = Nothing
	Set database = Nothing
	CheckError
End If

Sub CheckError
	Dim message, errRec
	If Err = 0 Then Exit Sub
	message = Err.Source & " " & Hex(Err) & ": " & Err.Description
	If Not installer Is Nothing Then
		Set errRec = installer.LastErrorRecord
		If Not errRec Is Nothing Then message = message & vbNewLine & errRec.FormatText
	End If
	Wscript.Echo message
	Wscript.Quit 2
End Sub

'' SIG '' Begin signature block
'' SIG '' MIImWQYJKoZIhvcNAQcCoIImSjCCJkYCAQExDzANBglg
'' SIG '' hkgBZQMEAgEFADB3BgorBgEEAYI3AgEEoGkwZzAyBgor
'' SIG '' BgEEAYI3AgEeMCQCAQEEEE7wKRaZJ7VNj+Ws4Q8X66sC
'' SIG '' AQACAQACAQACAQACAQAwMTANBglghkgBZQMEAgEFAAQg
'' SIG '' cI6yPc6nU50pwvCvjhpQqcdV9XZAulr8Q0FGvLPiuv6g
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
'' SIG '' BAGCNwIBFTAvBgkqhkiG9w0BCQQxIgQgQokwC+HG3/TZ
'' SIG '' 4gbQnFv4cBB6+FknILF1a4HcmtoisjAwWgYKKwYBBAGC
'' SIG '' NwIBDDFMMEqgJIAiAE0AaQBjAHIAbwBzAG8AZgB0ACAA
'' SIG '' VwBpAG4AZABvAHcAc6EigCBodHRwOi8vd3d3Lm1pY3Jv
'' SIG '' c29mdC5jb20vd2luZG93czANBgkqhkiG9w0BAQEFAASC
'' SIG '' AQCa4k4sZvtfNTxSNG3Z+w1REPHftGhaerIvkA/4rcf3
'' SIG '' Tk9SjeAW4Hn996KVO1M4FVedT2eigQxkpm1Mu6feM+43
'' SIG '' tonY4LAn0uL8e6mzJS11ejIPwgZQG1FVqYG49mkDiMeV
'' SIG '' wp2Pfcer1k+x9CqkNtPK/PfvPC93lvZWDnSonKmp8xvE
'' SIG '' WT3p0bgy+XjbWiVWAkbyU/WjxEhhXyZ7nQ2q8yA3DcI2
'' SIG '' t8XSYpXrBe10jD52WTmGumDGlYEQc2rQ6nMJ1LQg6Ijj
'' SIG '' 5p4IP2Ac5F0/h5tUmgTTqTPSCxtTBR/woHVqZ1X3l5VX
'' SIG '' GJdG9djkIBhizepJO7azeS30/nkSyBjekHrjoYIXrTCC
'' SIG '' F6kGCisGAQQBgjcDAwExgheZMIIXlQYJKoZIhvcNAQcC
'' SIG '' oIIXhjCCF4ICAQMxDzANBglghkgBZQMEAgEFADCCAVoG
'' SIG '' CyqGSIb3DQEJEAEEoIIBSQSCAUUwggFBAgEBBgorBgEE
'' SIG '' AYRZCgMBMDEwDQYJYIZIAWUDBAIBBQAEIHjAs3MPBZXt
'' SIG '' /WmrM//iZVTaUmqTMDV9EZinSWSDBoUgAgZm6zvFSDkY
'' SIG '' EzIwMjQxMTE2MDkxNjQ4LjA5NVowBIACAfSggdmkgdYw
'' SIG '' gdMxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5n
'' SIG '' dG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVN
'' SIG '' aWNyb3NvZnQgQ29ycG9yYXRpb24xLTArBgNVBAsTJE1p
'' SIG '' Y3Jvc29mdCBJcmVsYW5kIE9wZXJhdGlvbnMgTGltaXRl
'' SIG '' ZDEnMCUGA1UECxMeblNoaWVsZCBUU1MgRVNOOjJBMUEt
'' SIG '' MDVFMC1EOTQ3MSUwIwYDVQQDExxNaWNyb3NvZnQgVGlt
'' SIG '' ZS1TdGFtcCBTZXJ2aWNloIIR+zCCBygwggUQoAMCAQIC
'' SIG '' EzMAAAH5H2eNdauk8bEAAQAAAfkwDQYJKoZIhvcNAQEL
'' SIG '' BQAwfDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hp
'' SIG '' bmd0b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoT
'' SIG '' FU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEmMCQGA1UEAxMd
'' SIG '' TWljcm9zb2Z0IFRpbWUtU3RhbXAgUENBIDIwMTAwHhcN
'' SIG '' MjQwNzI1MTgzMTA5WhcNMjUxMDIyMTgzMTA5WjCB0zEL
'' SIG '' MAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24x
'' SIG '' EDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jv
'' SIG '' c29mdCBDb3Jwb3JhdGlvbjEtMCsGA1UECxMkTWljcm9z
'' SIG '' b2Z0IElyZWxhbmQgT3BlcmF0aW9ucyBMaW1pdGVkMScw
'' SIG '' JQYDVQQLEx5uU2hpZWxkIFRTUyBFU046MkExQS0wNUUw
'' SIG '' LUQ5NDcxJTAjBgNVBAMTHE1pY3Jvc29mdCBUaW1lLVN0
'' SIG '' YW1wIFNlcnZpY2UwggIiMA0GCSqGSIb3DQEBAQUAA4IC
'' SIG '' DwAwggIKAoICAQC0PUwffIAdYc1WyUL4IFOP8yl3nksM
'' SIG '' +1CuE3tZ6oWFF4L3EpdKOhtbVkfMdTxXYE4lSJiDt8Mn
'' SIG '' YDEZUbKi9S2AZmDb4Zq4UqTdmOOwtKyp6FgixRCuBf6v
'' SIG '' 9UBNpbz841bLqU7IZnBmnF9XYRfioCHqZvaFp0C691tG
'' SIG '' XVArW18GVHd914IFAb7JvP0kVnjks3amzw1zXGvjU3xC
'' SIG '' LcpUkthfSJsRsCSSxHhtuzMLO9j691KuNbIoCNHpiBiF
'' SIG '' oFoPETYoMnaxBEUUX96ALEqCiB0XdUgmgIT9a7L0y4SD
'' SIG '' Kl5rUd6LuUUa90tBkfkmjZBHm43yGIxzxnjtFEm4hYI5
'' SIG '' 7IgnVidGKKJulRnvb7Cm/wtOi/TIfoLkdH8Pz4BPi+q0
'' SIG '' /nshNewP0M86hvy2O2x589xAl5tQ2KrJ/JMvmPn8n7Z3
'' SIG '' 4Y8JxcRih5Zn6euxlJ+t3kMczii8KYPeWJ+BifOM6vLi
'' SIG '' CFBP9y+Z0fAWvrIkamFb8cbwZB35wHjDvAak6EdUlvLj
'' SIG '' iQZUrwzNj2zfYPLVMecmDynvLWwQbP8DXLzhm3qAiwhN
'' SIG '' hpxweEEqnhw5U2t+hFVTHYb/ROvsOTd+kJTy77miWo8/
'' SIG '' AqBmznuOX6U6tFWxfUBgSYCfILIaupEDOkZfKTUe80gG
'' SIG '' lI025MFCTsUG+75imLoDtLZXZOPqXNhZUG+4YQIDAQAB
'' SIG '' o4IBSTCCAUUwHQYDVR0OBBYEFInto7qclckj16KPNLlC
'' SIG '' RHZGWeAAMB8GA1UdIwQYMBaAFJ+nFV0AXmJdg/Tl0mWn
'' SIG '' G1M1GelyMF8GA1UdHwRYMFYwVKBSoFCGTmh0dHA6Ly93
'' SIG '' d3cubWljcm9zb2Z0LmNvbS9wa2lvcHMvY3JsL01pY3Jv
'' SIG '' c29mdCUyMFRpbWUtU3RhbXAlMjBQQ0ElMjAyMDEwKDEp
'' SIG '' LmNybDBsBggrBgEFBQcBAQRgMF4wXAYIKwYBBQUHMAKG
'' SIG '' UGh0dHA6Ly93d3cubWljcm9zb2Z0LmNvbS9wa2lvcHMv
'' SIG '' Y2VydHMvTWljcm9zb2Z0JTIwVGltZS1TdGFtcCUyMFBD
'' SIG '' QSUyMDIwMTAoMSkuY3J0MAwGA1UdEwEB/wQCMAAwFgYD
'' SIG '' VR0lAQH/BAwwCgYIKwYBBQUHAwgwDgYDVR0PAQH/BAQD
'' SIG '' AgeAMA0GCSqGSIb3DQEBCwUAA4ICAQBmIAmAVuR/uN+H
'' SIG '' H+aZmWcZmulp74canFbGzwjv29RvwZCi7nQzWezuLAbY
'' SIG '' Jx2hdqrtWClWQ1/W68iGsZikoIFdD5JonY7QG/C4lHtS
'' SIG '' yBNoo3SP/J/d+kcPSS0f4SQS4Zez0MEvK3vWK61WTCjD
'' SIG '' 2JCZKTiggrxLwCs0alI7N6671N0mMGOxqya4n7arlOOa
'' SIG '' uAQrI97dMCkCKjxx3D9vVwECaO0ju2k1hXk/JEjcrU2G
'' SIG '' 4OB8SPmTKcYX+6LM/U24dLEX9XWSz/a0ISiuKJwziTU8
'' SIG '' lNMDRMKM1uSmYFywAyXFPMGdayqcEK3135R31VrcjD0G
'' SIG '' zhxyuSAGMu2De9gZhqvrXmh9i1T526n4u5TR3bAEMQbW
'' SIG '' eFJYdo767bLpKLcBo0g23+k4wpTqXgBbS4NZQff04cfc
'' SIG '' SoUe1OyxldoM6O3JGBuowaaR/wojeohUFknZdCmeES5F
'' SIG '' uH4CCmZGf9rjXQOTtW0+Da4LjbZYsLwfwhWT8V6iJJLi
'' SIG '' 8Wh2GdwV60nRkrfrDEBrcWI+AF5tFbJW1nvreoMPPENv
'' SIG '' SYHocv0cR9Ns37igcKRlrUcqXwHSzxGIUEx/9bv47sQ9
'' SIG '' n7AwfzB2SNntJux1211GBEBGpHwgU9a6tD6yft+0SJ9q
'' SIG '' iPO4IRqFIByrzrKPBB5M831gb1vfhFO6ueSkP7A8ZMHV
'' SIG '' ZxwymwuUzTCCB3EwggVZoAMCAQICEzMAAAAVxedrngKb
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
'' SIG '' CxMeblNoaWVsZCBUU1MgRVNOOjJBMUEtMDVFMC1EOTQ3
'' SIG '' MSUwIwYDVQQDExxNaWNyb3NvZnQgVGltZS1TdGFtcCBT
'' SIG '' ZXJ2aWNloiMKAQEwBwYFKw4DAhoDFQCqzlaNY7vNUAqY
'' SIG '' hx3CGqBm/KnpRqCBgzCBgKR+MHwxCzAJBgNVBAYTAlVT
'' SIG '' MRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdS
'' SIG '' ZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9y
'' SIG '' YXRpb24xJjAkBgNVBAMTHU1pY3Jvc29mdCBUaW1lLVN0
'' SIG '' YW1wIFBDQSAyMDEwMA0GCSqGSIb3DQEBCwUAAgUA6uLW
'' SIG '' FzAiGA8yMDI0MTExNjA4MjgwN1oYDzIwMjQxMTE3MDgy
'' SIG '' ODA3WjB0MDoGCisGAQQBhFkKBAExLDAqMAoCBQDq4tYX
'' SIG '' AgEAMAcCAQACAjLyMAcCAQACAhOwMAoCBQDq5CeXAgEA
'' SIG '' MDYGCisGAQQBhFkKBAIxKDAmMAwGCisGAQQBhFkKAwKg
'' SIG '' CjAIAgEAAgMHoSChCjAIAgEAAgMBhqAwDQYJKoZIhvcN
'' SIG '' AQELBQADggEBAFsl8TFXJfNEdkB+qIwQVpwgbL0UpYoi
'' SIG '' zmf8nhc1vggOisB9wcxrOZvjKajz+4F6ShrvzQFjVqBV
'' SIG '' 8DjxACA8jzFCu//BmB4wRUK7bBdTFUPeZSxjSUxo3TrA
'' SIG '' c28X5ptELSetMqVdimOoCxAOJiwcjx6VJgb1658H7BoE
'' SIG '' vrNqcef9ECQJXM+h/TN4tzxkNapS3DNLJrQ+GFlDSMYq
'' SIG '' Va/rYhmECP/Q46cHDz1FJ2v/O6Lobx9Eymn/7nzllrGs
'' SIG '' hpMbm9wcVd84ly7v+sXX2zXxgPwR0HKO3glCjqSyDtbS
'' SIG '' FgaDdzY2kaUbObjyFtDEzump1qJ5/AAxc+cW9OZfw/ll
'' SIG '' c68xggQNMIIECQIBATCBkzB8MQswCQYDVQQGEwJVUzET
'' SIG '' MBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMHUmVk
'' SIG '' bW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBvcmF0
'' SIG '' aW9uMSYwJAYDVQQDEx1NaWNyb3NvZnQgVGltZS1TdGFt
'' SIG '' cCBQQ0EgMjAxMAITMwAAAfkfZ411q6TxsQABAAAB+TAN
'' SIG '' BglghkgBZQMEAgEFAKCCAUowGgYJKoZIhvcNAQkDMQ0G
'' SIG '' CyqGSIb3DQEJEAEEMC8GCSqGSIb3DQEJBDEiBCDan/N0
'' SIG '' nQk2uQPfLVwaaMKrEBZXrqU0Vxl18qfGv783hTCB+gYL
'' SIG '' KoZIhvcNAQkQAi8xgeowgecwgeQwgb0EIDkjjMge8I37
'' SIG '' ZPrpFQ4sJmtQRV2gqUqXxV4I7lJsYtgQMIGYMIGApH4w
'' SIG '' fDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0
'' SIG '' b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1p
'' SIG '' Y3Jvc29mdCBDb3Jwb3JhdGlvbjEmMCQGA1UEAxMdTWlj
'' SIG '' cm9zb2Z0IFRpbWUtU3RhbXAgUENBIDIwMTACEzMAAAH5
'' SIG '' H2eNdauk8bEAAQAAAfkwIgQgprn3t80BCEbnLoy3tSCl
'' SIG '' +r4MYjEAi+szO6WUxUa2cPwwDQYJKoZIhvcNAQELBQAE
'' SIG '' ggIAR6qE4TJOry7C/jL+q//tLU+FDzNQiXo8dIUxUSRz
'' SIG '' JdgbefB4ZwCMMLyv90c/J74UVftmH4R0sjMUP+UIVJkH
'' SIG '' DUO/4PyJ/l0eAlYmgr4rf8QHFLLVtBmmW+HjYIH+jFDZ
'' SIG '' WVckKtA6jtv3MeC+ybQDMIy4+sx7Lw0kVolUNSpQmEAo
'' SIG '' ZrVgA4yOmQcSY2YADhWYeQBH+4s/msKbEX09UEUVUTfl
'' SIG '' CSsWjdvGYh+cO5xudLvWni0j2s2qo6EFMDUbpaqbYfM3
'' SIG '' 3x591Bf2Ly9b3e644HqjISCUBx6dwI0S3ZfQdkwruLZL
'' SIG '' PcweeQ2GZWQgsiIXv/rh2HvxVhtCQfA7MaFwI8yYd5ny
'' SIG '' pz4cddEqKIkxgFgL0t0czPAp63i0wC7pfO19sbex9DXy
'' SIG '' NuIJJFA46HXqrD+Vhp956w63QxMnlWU8A5pWuhtcdigd
'' SIG '' WT4QpJneNCf8b3FZOzEoJo5mW2c6yodiWZVmK5uMOA6k
'' SIG '' k9wRdSppw940cLljrcaPle6pul5FxkGY4qIRmMe6bmoD
'' SIG '' shRJalc/6y07weqhEtD6x5BetLZ8xQrdGDVSoTBbcyEG
'' SIG '' /eqobOLgj+3L23kHN3aphxA2FlaOOHgekQXeBuYP0UtM
'' SIG '' SHASabU+pkxfr2LuN0LPR4tHAdhaffW0Z6b3oUGu5U0q
'' SIG '' yIAHXl6UhzUPvJzJ3POyE0N5TAs=
'' SIG '' End signature block
