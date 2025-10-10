' Windows Installer utility to generate a transform from two databases
' For use with Windows Scripting Host, CScript.exe or WScript.exe
' Copyright (c) Microsoft Corporation. All rights reserved.
' Demonstrates use of Database.GenerateTransform and MsiDatabaseGenerateTransform
'
Option Explicit

Const msiOpenDatabaseModeReadOnly     = 0
Const msiOpenDatabaseModeTransact     = 1
Const msiOpenDatabaseModeCreate       = 3

If Wscript.Arguments.Count < 2 Then
	Wscript.Echo "Windows Installer database tranform generation utility" &_
		vbNewLine & " 1st argument is the path to the original installer database" &_
		vbNewLine & " 2nd argument is the path to the updated installer database" &_
		vbNewLine & " 3rd argument is the path to the transform file to generate" &_
		vbNewLine & " If the 3rd argument is omitted, the databases are only compared" &_
		vbNewLine &_
		vbNewLine & "Copyright (C) Microsoft Corporation.  All rights reserved."
	Wscript.Quit 1
End If

' Connect to Windows Installer object
On Error Resume Next
Dim installer : Set installer = Nothing
Set installer = Wscript.CreateObject("WindowsInstaller.Installer") : CheckError

' Open databases and generate transform
Dim database1 : Set database1 = installer.OpenDatabase(Wscript.Arguments(0), msiOpenDatabaseModeReadOnly) : CheckError
Dim database2 : Set database2 = installer.OpenDatabase(Wscript.Arguments(1), msiOpenDatabaseModeReadOnly) : CheckError
Dim transform:transform = ""  'Simply compare if no output transform file supplied
If Wscript.Arguments.Count >= 3 Then transform = Wscript.Arguments(2)
Dim different:different = Database2.GenerateTransform(Database1, transform) : CheckError
If Not different Then Wscript.Echo "Databases are identical" Else If transform = Empty Then Wscript.Echo "Databases are different"

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
'' SIG '' MIImSgYJKoZIhvcNAQcCoIImOzCCJjcCAQExDzANBglg
'' SIG '' hkgBZQMEAgEFADB3BgorBgEEAYI3AgEEoGkwZzAyBgor
'' SIG '' BgEEAYI3AgEeMCQCAQEEEE7wKRaZJ7VNj+Ws4Q8X66sC
'' SIG '' AQACAQACAQACAQACAQAwMTANBglghkgBZQMEAgEFAAQg
'' SIG '' LEOvI1OL4Ms+i8VD+9Lv0JnljZ8+Z42whrFijf9Ua/Cg
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
'' SIG '' jgd7JXFEqwZq5tTG3yOalnXFMYIaOzCCGjcCAQEwgZUw
'' SIG '' fjELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0
'' SIG '' b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1p
'' SIG '' Y3Jvc29mdCBDb3Jwb3JhdGlvbjEoMCYGA1UEAxMfTWlj
'' SIG '' cm9zb2Z0IENvZGUgU2lnbmluZyBQQ0EgMjAxMAITMwAA
'' SIG '' Bae4j/uXXTWE7AAAAAAFpzANBglghkgBZQMEAgEFAKCB
'' SIG '' xjAZBgkqhkiG9w0BCQMxDAYKKwYBBAGCNwIBBDAcBgor
'' SIG '' BgEEAYI3AgELMQ4wDAYKKwYBBAGCNwIBFTAvBgkqhkiG
'' SIG '' 9w0BCQQxIgQggN866NpS7HixjtjP92kpjmQFYaHpBAuj
'' SIG '' o7+KZVwNlocwWgYKKwYBBAGCNwIBDDFMMEqgJIAiAE0A
'' SIG '' aQBjAHIAbwBzAG8AZgB0ACAAVwBpAG4AZABvAHcAc6Ei
'' SIG '' gCBodHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vd2luZG93
'' SIG '' czANBgkqhkiG9w0BAQEFAASCAQAkZP8wIY/TDonS9lA+
'' SIG '' 1vgHDdpfhFymGSx0SU86ao6UuI3Buz9bpAg7a6Mj3gZk
'' SIG '' AIIPSMQkQatnUmNVson5aoOtVtOBStXQJIE25B3795Jz
'' SIG '' 2dQ/bBYbDUh7F85jhvv9VDc3kcbR5rqQTK9/P0IClS0e
'' SIG '' +S3hGmWeDbd7j6Cea+awlAOJ6r69n2VEfRHOAyHpnaOw
'' SIG '' CV2saJHFz/J3GUOUpvrLVLgPQOfrQbcq+lF6yo6lWOEM
'' SIG '' rp088gTJ/FLF+jTlsEE44zOcjV9IH/geJ/S1In5fPQL1
'' SIG '' evFQRFbf45xdRFQ/30mJnbBd41S+CxYNIkJo0YRVxFyE
'' SIG '' Z52DwQwntxlljORqoYIXrTCCF6kGCisGAQQBgjcDAwEx
'' SIG '' gheZMIIXlQYJKoZIhvcNAQcCoIIXhjCCF4ICAQMxDzAN
'' SIG '' BglghkgBZQMEAgEFADCCAVoGCyqGSIb3DQEJEAEEoIIB
'' SIG '' SQSCAUUwggFBAgEBBgorBgEEAYRZCgMBMDEwDQYJYIZI
'' SIG '' AWUDBAIBBQAEIAhiYOGAXL3nBPPW+pUjxwUXEjXBKQYm
'' SIG '' gQ7lpgUmCjB3AgZm6zvFSKEYEzIwMjQxMTE2MDkxNjUx
'' SIG '' Ljg0NlowBIACAfSggdmkgdYwgdMxCzAJBgNVBAYTAlVT
'' SIG '' MRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdS
'' SIG '' ZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9y
'' SIG '' YXRpb24xLTArBgNVBAsTJE1pY3Jvc29mdCBJcmVsYW5k
'' SIG '' IE9wZXJhdGlvbnMgTGltaXRlZDEnMCUGA1UECxMeblNo
'' SIG '' aWVsZCBUU1MgRVNOOjJBMUEtMDVFMC1EOTQ3MSUwIwYD
'' SIG '' VQQDExxNaWNyb3NvZnQgVGltZS1TdGFtcCBTZXJ2aWNl
'' SIG '' oIIR+zCCBygwggUQoAMCAQICEzMAAAH5H2eNdauk8bEA
'' SIG '' AQAAAfkwDQYJKoZIhvcNAQELBQAwfDELMAkGA1UEBhMC
'' SIG '' VVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcT
'' SIG '' B1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jw
'' SIG '' b3JhdGlvbjEmMCQGA1UEAxMdTWljcm9zb2Z0IFRpbWUt
'' SIG '' U3RhbXAgUENBIDIwMTAwHhcNMjQwNzI1MTgzMTA5WhcN
'' SIG '' MjUxMDIyMTgzMTA5WjCB0zELMAkGA1UEBhMCVVMxEzAR
'' SIG '' BgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1v
'' SIG '' bmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlv
'' SIG '' bjEtMCsGA1UECxMkTWljcm9zb2Z0IElyZWxhbmQgT3Bl
'' SIG '' cmF0aW9ucyBMaW1pdGVkMScwJQYDVQQLEx5uU2hpZWxk
'' SIG '' IFRTUyBFU046MkExQS0wNUUwLUQ5NDcxJTAjBgNVBAMT
'' SIG '' HE1pY3Jvc29mdCBUaW1lLVN0YW1wIFNlcnZpY2UwggIi
'' SIG '' MA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoICAQC0PUwf
'' SIG '' fIAdYc1WyUL4IFOP8yl3nksM+1CuE3tZ6oWFF4L3EpdK
'' SIG '' OhtbVkfMdTxXYE4lSJiDt8MnYDEZUbKi9S2AZmDb4Zq4
'' SIG '' UqTdmOOwtKyp6FgixRCuBf6v9UBNpbz841bLqU7IZnBm
'' SIG '' nF9XYRfioCHqZvaFp0C691tGXVArW18GVHd914IFAb7J
'' SIG '' vP0kVnjks3amzw1zXGvjU3xCLcpUkthfSJsRsCSSxHht
'' SIG '' uzMLO9j691KuNbIoCNHpiBiFoFoPETYoMnaxBEUUX96A
'' SIG '' LEqCiB0XdUgmgIT9a7L0y4SDKl5rUd6LuUUa90tBkfkm
'' SIG '' jZBHm43yGIxzxnjtFEm4hYI57IgnVidGKKJulRnvb7Cm
'' SIG '' /wtOi/TIfoLkdH8Pz4BPi+q0/nshNewP0M86hvy2O2x5
'' SIG '' 89xAl5tQ2KrJ/JMvmPn8n7Z34Y8JxcRih5Zn6euxlJ+t
'' SIG '' 3kMczii8KYPeWJ+BifOM6vLiCFBP9y+Z0fAWvrIkamFb
'' SIG '' 8cbwZB35wHjDvAak6EdUlvLjiQZUrwzNj2zfYPLVMecm
'' SIG '' DynvLWwQbP8DXLzhm3qAiwhNhpxweEEqnhw5U2t+hFVT
'' SIG '' HYb/ROvsOTd+kJTy77miWo8/AqBmznuOX6U6tFWxfUBg
'' SIG '' SYCfILIaupEDOkZfKTUe80gGlI025MFCTsUG+75imLoD
'' SIG '' tLZXZOPqXNhZUG+4YQIDAQABo4IBSTCCAUUwHQYDVR0O
'' SIG '' BBYEFInto7qclckj16KPNLlCRHZGWeAAMB8GA1UdIwQY
'' SIG '' MBaAFJ+nFV0AXmJdg/Tl0mWnG1M1GelyMF8GA1UdHwRY
'' SIG '' MFYwVKBSoFCGTmh0dHA6Ly93d3cubWljcm9zb2Z0LmNv
'' SIG '' bS9wa2lvcHMvY3JsL01pY3Jvc29mdCUyMFRpbWUtU3Rh
'' SIG '' bXAlMjBQQ0ElMjAyMDEwKDEpLmNybDBsBggrBgEFBQcB
'' SIG '' AQRgMF4wXAYIKwYBBQUHMAKGUGh0dHA6Ly93d3cubWlj
'' SIG '' cm9zb2Z0LmNvbS9wa2lvcHMvY2VydHMvTWljcm9zb2Z0
'' SIG '' JTIwVGltZS1TdGFtcCUyMFBDQSUyMDIwMTAoMSkuY3J0
'' SIG '' MAwGA1UdEwEB/wQCMAAwFgYDVR0lAQH/BAwwCgYIKwYB
'' SIG '' BQUHAwgwDgYDVR0PAQH/BAQDAgeAMA0GCSqGSIb3DQEB
'' SIG '' CwUAA4ICAQBmIAmAVuR/uN+HH+aZmWcZmulp74canFbG
'' SIG '' zwjv29RvwZCi7nQzWezuLAbYJx2hdqrtWClWQ1/W68iG
'' SIG '' sZikoIFdD5JonY7QG/C4lHtSyBNoo3SP/J/d+kcPSS0f
'' SIG '' 4SQS4Zez0MEvK3vWK61WTCjD2JCZKTiggrxLwCs0alI7
'' SIG '' N6671N0mMGOxqya4n7arlOOauAQrI97dMCkCKjxx3D9v
'' SIG '' VwECaO0ju2k1hXk/JEjcrU2G4OB8SPmTKcYX+6LM/U24
'' SIG '' dLEX9XWSz/a0ISiuKJwziTU8lNMDRMKM1uSmYFywAyXF
'' SIG '' PMGdayqcEK3135R31VrcjD0GzhxyuSAGMu2De9gZhqvr
'' SIG '' Xmh9i1T526n4u5TR3bAEMQbWeFJYdo767bLpKLcBo0g2
'' SIG '' 3+k4wpTqXgBbS4NZQff04cfcSoUe1OyxldoM6O3JGBuo
'' SIG '' waaR/wojeohUFknZdCmeES5FuH4CCmZGf9rjXQOTtW0+
'' SIG '' Da4LjbZYsLwfwhWT8V6iJJLi8Wh2GdwV60nRkrfrDEBr
'' SIG '' cWI+AF5tFbJW1nvreoMPPENvSYHocv0cR9Ns37igcKRl
'' SIG '' rUcqXwHSzxGIUEx/9bv47sQ9n7AwfzB2SNntJux1211G
'' SIG '' BEBGpHwgU9a6tD6yft+0SJ9qiPO4IRqFIByrzrKPBB5M
'' SIG '' 831gb1vfhFO6ueSkP7A8ZMHVZxwymwuUzTCCB3EwggVZ
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
'' SIG '' gm2lBRDBcQZqELQdVTNYs6FwZvKhggNWMIICPgIBATCC
'' SIG '' AQGhgdmkgdYwgdMxCzAJBgNVBAYTAlVTMRMwEQYDVQQI
'' SIG '' EwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4w
'' SIG '' HAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xLTAr
'' SIG '' BgNVBAsTJE1pY3Jvc29mdCBJcmVsYW5kIE9wZXJhdGlv
'' SIG '' bnMgTGltaXRlZDEnMCUGA1UECxMeblNoaWVsZCBUU1Mg
'' SIG '' RVNOOjJBMUEtMDVFMC1EOTQ3MSUwIwYDVQQDExxNaWNy
'' SIG '' b3NvZnQgVGltZS1TdGFtcCBTZXJ2aWNloiMKAQEwBwYF
'' SIG '' Kw4DAhoDFQCqzlaNY7vNUAqYhx3CGqBm/KnpRqCBgzCB
'' SIG '' gKR+MHwxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNo
'' SIG '' aW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQK
'' SIG '' ExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xJjAkBgNVBAMT
'' SIG '' HU1pY3Jvc29mdCBUaW1lLVN0YW1wIFBDQSAyMDEwMA0G
'' SIG '' CSqGSIb3DQEBCwUAAgUA6uLWFzAiGA8yMDI0MTExNjA4
'' SIG '' MjgwN1oYDzIwMjQxMTE3MDgyODA3WjB0MDoGCisGAQQB
'' SIG '' hFkKBAExLDAqMAoCBQDq4tYXAgEAMAcCAQACAjLyMAcC
'' SIG '' AQACAhOwMAoCBQDq5CeXAgEAMDYGCisGAQQBhFkKBAIx
'' SIG '' KDAmMAwGCisGAQQBhFkKAwKgCjAIAgEAAgMHoSChCjAI
'' SIG '' AgEAAgMBhqAwDQYJKoZIhvcNAQELBQADggEBAFsl8TFX
'' SIG '' JfNEdkB+qIwQVpwgbL0UpYoizmf8nhc1vggOisB9wcxr
'' SIG '' OZvjKajz+4F6ShrvzQFjVqBV8DjxACA8jzFCu//BmB4w
'' SIG '' RUK7bBdTFUPeZSxjSUxo3TrAc28X5ptELSetMqVdimOo
'' SIG '' CxAOJiwcjx6VJgb1658H7BoEvrNqcef9ECQJXM+h/TN4
'' SIG '' tzxkNapS3DNLJrQ+GFlDSMYqVa/rYhmECP/Q46cHDz1F
'' SIG '' J2v/O6Lobx9Eymn/7nzllrGshpMbm9wcVd84ly7v+sXX
'' SIG '' 2zXxgPwR0HKO3glCjqSyDtbSFgaDdzY2kaUbObjyFtDE
'' SIG '' zump1qJ5/AAxc+cW9OZfw/llc68xggQNMIIECQIBATCB
'' SIG '' kzB8MQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2FzaGlu
'' SIG '' Z3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UEChMV
'' SIG '' TWljcm9zb2Z0IENvcnBvcmF0aW9uMSYwJAYDVQQDEx1N
'' SIG '' aWNyb3NvZnQgVGltZS1TdGFtcCBQQ0EgMjAxMAITMwAA
'' SIG '' AfkfZ411q6TxsQABAAAB+TANBglghkgBZQMEAgEFAKCC
'' SIG '' AUowGgYJKoZIhvcNAQkDMQ0GCyqGSIb3DQEJEAEEMC8G
'' SIG '' CSqGSIb3DQEJBDEiBCDOD5YAkBrSAWa9NSocqBNo3w06
'' SIG '' uz/k9Aar+S0SayeKwjCB+gYLKoZIhvcNAQkQAi8xgeow
'' SIG '' gecwgeQwgb0EIDkjjMge8I37ZPrpFQ4sJmtQRV2gqUqX
'' SIG '' xV4I7lJsYtgQMIGYMIGApH4wfDELMAkGA1UEBhMCVVMx
'' SIG '' EzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1Jl
'' SIG '' ZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3Jh
'' SIG '' dGlvbjEmMCQGA1UEAxMdTWljcm9zb2Z0IFRpbWUtU3Rh
'' SIG '' bXAgUENBIDIwMTACEzMAAAH5H2eNdauk8bEAAQAAAfkw
'' SIG '' IgQgprn3t80BCEbnLoy3tSCl+r4MYjEAi+szO6WUxUa2
'' SIG '' cPwwDQYJKoZIhvcNAQELBQAEggIAeDwxHbhksqU2XsRq
'' SIG '' uLrkE4PkDohDc4cSzFxCJAFfWpHjkUivOj8hNakUhtsR
'' SIG '' J/zBeptCErjpsHzreCOAFgR4ZS0ZYxtseUm/SDL6y/yZ
'' SIG '' BrvVcSjB1UsHAPBlCo3XMpqOYO+Gjua12Ih6DxwGhxF4
'' SIG '' DVqTxYD5leH6ogPkWHG7FGHcytVqrtpovMyj9+4bFHxh
'' SIG '' L2lFg+aKDCsHVOVOc5Y6T7t91AEGvd6YXbZgcTbBcWA/
'' SIG '' bpcdN98udhvRUIJ7H0TASsAJqB1Ha+kN/Mu87hnTsR8X
'' SIG '' WL8uWyfLevISxGWCho9YFEC78i/RCwfO6qb0ZSxKpyy7
'' SIG '' zA6p7wxCYsy8NqHkUAiGFSqPJd8Hgn/60pAf0JMZ32Bo
'' SIG '' SjR/L0vR9qZhQbjeo/CvWvSszQ6fyCiQOn7oKF0+Rj5k
'' SIG '' QYXiIRvXPgrv1DrR1L6JjKBeVUtJ5+N8s0QNtRdGITxA
'' SIG '' denG/i+FJtbuzgvPqw+AV8u9E+rvsX+ThzL/aAiYAfoM
'' SIG '' jPFCM3LLsLeJDUwEeqCrYMLFksh79/inWRHvgTFW/KFL
'' SIG '' tkzinMuzNS2n/ZpQeE5jbrxsCLnG70ndOqqBqR3pBsO1
'' SIG '' T3jIKQ5k4USbkAum1ZIG8b6FGQw5wgcm/TcUZvt38eU0
'' SIG '' JV9AX7bYMhqK0gx7PbnZLhVbungrJY9BIeT83dJ/H9Ul
'' SIG '' yKnwxBg=
'' SIG '' End signature block
