' Windows Installer transform viewer for use with Windows Scripting Host
' Copyright (c) Microsoft Corporation. All rights reserved.
' Demonstrates the use of the database APIs for viewing transform files
'
Option Explicit

Const iteAddExistingRow      = 1
Const iteDelNonExistingRow   = 2
Const iteAddExistingTable    = 4
Const iteDelNonExistingTable = 8
Const iteUpdNonExistingRow   = 16
Const iteChangeCodePage      = 32
Const iteViewTransform       = 256

Const icdLong       = 0
Const icdShort      = &h400
Const icdObject     = &h800
Const icdString     = &hC00
Const icdNullable   = &h1000
Const icdPrimaryKey = &h2000
Const icdNoNulls    = &h0000
Const icdPersistent = &h0100
Const icdTemporary  = &h0000

Const idoReadOnly = 0

Dim gErrors, installer, base, database, argCount, arg, argValue
gErrors = iteAddExistingRow + iteDelNonExistingRow + iteAddExistingTable + iteDelNonExistingTable + iteUpdNonExistingRow + iteChangeCodePage
Set database = Nothing

' Check arg count, and display help if no all arguments present
argCount = WScript.Arguments.Count
If (argCount < 2) Then
	WScript.Echo "Windows Installer Transform Viewer for Windows Scripting Host (CScript.exe)" &_
		vbNewLine & " 1st non-numeric argument is path to base database which transforms reference" &_
		vbNewLine & " Subsequent non-numeric arguments are paths to the transforms to be viewed" &_
		vbNewLine & " Numeric argument is optional error suppression flags (default is ignore all)" &_
		vbNewLine & " Arguments are executed left-to-right, as encountered" &_
		vbNewLine &_
		vbNewLine & "Copyright (C) Microsoft Corporation.  All rights reserved."
	Wscript.Quit 1
End If

' Cannot run with GUI script host, as listing is performed to standard out
If UCase(Mid(Wscript.FullName, Len(Wscript.Path) + 2, 1)) = "W" Then
	WScript.Echo "Cannot use WScript.exe - must use CScript.exe with this program"
	Wscript.Quit 2
End If

' Create installer object
On Error Resume Next
Set installer = CreateObject("WindowsInstaller.Installer") : CheckError

' Process arguments, opening database and applying transforms
For arg = 0 To argCount - 1
	argValue = WScript.Arguments(arg)
	If IsNumeric(argValue) Then
		gErrors = argValue
	ElseIf database Is Nothing Then
		Set database = installer.OpenDatabase(argValue, idoReadOnly)
	Else
		database.ApplyTransform argValue, iteViewTransform + gErrors
	End If
	CheckError
Next
ListTransform(database)

Function DecodeColDef(colDef)
	Dim def
	Select Case colDef AND (icdShort OR icdObject)
	Case icdLong
		def = "LONG"
	Case icdShort
		def = "SHORT"
	Case icdObject
		def = "OBJECT"
	Case icdString
		def = "CHAR(" & (colDef AND 255) & ")"
	End Select
	If (colDef AND icdNullable)   =  0 Then def = def & " NOT NULL"
	If (colDef AND icdPrimaryKey) <> 0 Then def = def & " PRIMARY KEY"
	DecodeColDef = def
End Function

Sub ListTransform(database)
	Dim view, record, row, column, change
	On Error Resume Next
	Set view = database.OpenView("SELECT * FROM `_TransformView` ORDER BY `Table`, `Row`") : CheckError
	view.Execute : CheckError
	Do
		Set record = view.Fetch : CheckError
		If record Is Nothing Then Exit Do
		change = Empty
		If record.IsNull(3) Then
			row = "<DDL>"
			If NOT record.IsNull(4) Then change = "[" & record.StringData(5) & "]: " & DecodeColDef(record.StringData(4))
		Else
			row = "[" & Join(Split(record.StringData(3), vbTab, -1), ",") & "]"
			If record.StringData(2) <> "INSERT" AND record.StringData(2) <> "DELETE" Then change = "{" & record.StringData(5) & "}->{" & record.StringData(4) & "}"
		End If
		column = record.StringData(1) & " " & record.StringData(2)
		if Len(column) < 24 Then column = column & Space(24 - Len(column))
		WScript.Echo column, row, change
	Loop
End Sub

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
'' SIG '' MIImTAYJKoZIhvcNAQcCoIImPTCCJjkCAQExDzANBglg
'' SIG '' hkgBZQMEAgEFADB3BgorBgEEAYI3AgEEoGkwZzAyBgor
'' SIG '' BgEEAYI3AgEeMCQCAQEEEE7wKRaZJ7VNj+Ws4Q8X66sC
'' SIG '' AQACAQACAQACAQACAQAwMTANBglghkgBZQMEAgEFAAQg
'' SIG '' bSE4J2+6sdFrt3HJs8WbQ0/DQ6WkcvTESWq4fOF4kUmg
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
'' SIG '' jgd7JXFEqwZq5tTG3yOalnXFMYIaPTCCGjkCAQEwgZUw
'' SIG '' fjELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0
'' SIG '' b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1p
'' SIG '' Y3Jvc29mdCBDb3Jwb3JhdGlvbjEoMCYGA1UEAxMfTWlj
'' SIG '' cm9zb2Z0IENvZGUgU2lnbmluZyBQQ0EgMjAxMAITMwAA
'' SIG '' Bae4j/uXXTWE7AAAAAAFpzANBglghkgBZQMEAgEFAKCB
'' SIG '' xjAZBgkqhkiG9w0BCQMxDAYKKwYBBAGCNwIBBDAcBgor
'' SIG '' BgEEAYI3AgELMQ4wDAYKKwYBBAGCNwIBFTAvBgkqhkiG
'' SIG '' 9w0BCQQxIgQg/M4LDrkBMu8VzYzK+dUbfaQ78LzdU1Mw
'' SIG '' XYrmpbLq3S8wWgYKKwYBBAGCNwIBDDFMMEqgJIAiAE0A
'' SIG '' aQBjAHIAbwBzAG8AZgB0ACAAVwBpAG4AZABvAHcAc6Ei
'' SIG '' gCBodHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vd2luZG93
'' SIG '' czANBgkqhkiG9w0BAQEFAASCAQBAJ4IoLZXV4Oh/hIue
'' SIG '' BJCNCkfziznVS2lYuzzZxgnG7mbPISjKJATgs7KOkUQd
'' SIG '' KIvhi9YgQ5Xy8Xb+c5ns6S6Y6cyIaxg1L9UBVcCkgdpG
'' SIG '' IWYXmA3QlHZfMS8jZa7n2MLtkkkQL5QKkyWJWE9PIJTD
'' SIG '' MNFRKIHS4dv9CUUAYC8g+AzVE2jTh3ofm0Ebm6O5VTDE
'' SIG '' lXTDQKJBHwzR1TKq2YRxm8r/wOkCjrjbcDan5qmFGZG+
'' SIG '' WNGvngyfLz3Hu4/3IpvdqWEEktGkudjv1C6KjP63VJue
'' SIG '' njUIauC+vD0j2f7dqKlNEEmpKqQp/Qtu9T66iA2j0Tge
'' SIG '' OtJ9aHYwPxYJPZsSoYIXrzCCF6sGCisGAQQBgjcDAwEx
'' SIG '' ghebMIIXlwYJKoZIhvcNAQcCoIIXiDCCF4QCAQMxDzAN
'' SIG '' BglghkgBZQMEAgEFADCCAVkGCyqGSIb3DQEJEAEEoIIB
'' SIG '' SASCAUQwggFAAgEBBgorBgEEAYRZCgMBMDEwDQYJYIZI
'' SIG '' AWUDBAIBBQAEIPbQUPAgrUrCbou6ao8759RWQRIfpktX
'' SIG '' Ywx7QuCZ3MEKAgZm60ySWsgYEjIwMjQxMTE2MDkxNjU1
'' SIG '' Ljc0WjAEgAIB9KCB2aSB1jCB0zELMAkGA1UEBhMCVVMx
'' SIG '' EzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1Jl
'' SIG '' ZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3Jh
'' SIG '' dGlvbjEtMCsGA1UECxMkTWljcm9zb2Z0IElyZWxhbmQg
'' SIG '' T3BlcmF0aW9ucyBMaW1pdGVkMScwJQYDVQQLEx5uU2hp
'' SIG '' ZWxkIFRTUyBFU046NEMxQS0wNUUwLUQ5NDcxJTAjBgNV
'' SIG '' BAMTHE1pY3Jvc29mdCBUaW1lLVN0YW1wIFNlcnZpY2Wg
'' SIG '' ghH+MIIHKDCCBRCgAwIBAgITMwAAAf8SOHz3wWXWoQAB
'' SIG '' AAAB/zANBgkqhkiG9w0BAQsFADB8MQswCQYDVQQGEwJV
'' SIG '' UzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMH
'' SIG '' UmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBv
'' SIG '' cmF0aW9uMSYwJAYDVQQDEx1NaWNyb3NvZnQgVGltZS1T
'' SIG '' dGFtcCBQQ0EgMjAxMDAeFw0yNDA3MjUxODMxMTlaFw0y
'' SIG '' NTEwMjIxODMxMTlaMIHTMQswCQYDVQQGEwJVUzETMBEG
'' SIG '' A1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9u
'' SIG '' ZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBvcmF0aW9u
'' SIG '' MS0wKwYDVQQLEyRNaWNyb3NvZnQgSXJlbGFuZCBPcGVy
'' SIG '' YXRpb25zIExpbWl0ZWQxJzAlBgNVBAsTHm5TaGllbGQg
'' SIG '' VFNTIEVTTjo0QzFBLTA1RTAtRDk0NzElMCMGA1UEAxMc
'' SIG '' TWljcm9zb2Z0IFRpbWUtU3RhbXAgU2VydmljZTCCAiIw
'' SIG '' DQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAMnoldKQ
'' SIG '' e24PP6nP5pIg3SV58yVj2IJPZkxniN6c0KbMq0SURFnC
'' SIG '' mB3f/XW/oN8+HVOFQpAGRF6r5MT+UDU7QRuSKXsaaYeD
'' SIG '' 4W4iSsL1/lEuCpEhYX9cH5QwGNbbvQkKoYcXxxVe74bZ
'' SIG '' qhywgpg8YWT5ggYff13xSUCFMFWUfEbVJIM5jfW5lomI
'' SIG '' H19EfmwwJ53FHbadcYxpgqXQTMoJPytId21E1M0B2+JD
'' SIG '' 39spZCj6FhWJ9hjWIFsPDxgVDtL0zCo2A+qS3gT9IWQ4
'' SIG '' eT93+MYRi5usffMbiEKf0RZ8wW4LYcklxpfjU9XGQKhs
'' SIG '' hIU+y9EnUe6kJb+acAzXq2yt2EhAypN7A4fUutISyTaj
'' SIG '' +9YhypBte+RwMoOs5hOad3zja/f3yBKTwJQvGIrMV2hl
'' SIG '' +EaQwWFSqRo9BQmcIrImbMZtF/cOmUpPDjl3/CcU2FiK
'' SIG '' n0bls3VIq9Gd44jjrWg6u13cqQeIGa4a/dCnD0w0cL8u
'' SIG '' tM60HGv9Q9Sez0CQCTm24mm6ItdrrFfGsbZU/3QnjwuJ
'' SIG '' 3XBXGq9b/n5wpYbPbtxZ+i5Bw0WXzc4V4CwxMG+nQOMt
'' SIG '' 7OhvoEN+aPdI9oumpmmvCbFf3Ahfog0hswMWWNbENZq3
'' SIG '' TJs8X1s1zerDyTMuPbXbFkyIGVlTkkvblB4UmJG4DMZy
'' SIG '' 3oil3geTAfUDHDknAgMBAAGjggFJMIIBRTAdBgNVHQ4E
'' SIG '' FgQUw/qV5P60/3exP9EBO4R9MM/ulGEwHwYDVR0jBBgw
'' SIG '' FoAUn6cVXQBeYl2D9OXSZacbUzUZ6XIwXwYDVR0fBFgw
'' SIG '' VjBUoFKgUIZOaHR0cDovL3d3dy5taWNyb3NvZnQuY29t
'' SIG '' L3BraW9wcy9jcmwvTWljcm9zb2Z0JTIwVGltZS1TdGFt
'' SIG '' cCUyMFBDQSUyMDIwMTAoMSkuY3JsMGwGCCsGAQUFBwEB
'' SIG '' BGAwXjBcBggrBgEFBQcwAoZQaHR0cDovL3d3dy5taWNy
'' SIG '' b3NvZnQuY29tL3BraW9wcy9jZXJ0cy9NaWNyb3NvZnQl
'' SIG '' MjBUaW1lLVN0YW1wJTIwUENBJTIwMjAxMCgxKS5jcnQw
'' SIG '' DAYDVR0TAQH/BAIwADAWBgNVHSUBAf8EDDAKBggrBgEF
'' SIG '' BQcDCDAOBgNVHQ8BAf8EBAMCB4AwDQYJKoZIhvcNAQEL
'' SIG '' BQADggIBADkjTeTKS4srp5vOun61iItIXWsyjS4Ead1m
'' SIG '' T34WIDwtyvwzTMy5YmEFAKelrYKJSK2rYr14zhtZSI2s
'' SIG '' hva+nsOB9Z+V2XQ3yddgy46KWqeXtYlP2JNHrrT8nzon
'' SIG '' r327CM05PxudfrolCZO+9p1c2ruoSNihshgSTrwGwFRU
'' SIG '' dIPKaWcC4IU+M95pBmY6vzuGfz3JlRrYxqbNkwrSOK2Y
'' SIG '' zzVvDuHP+GiUZmEPzXVvdSUazl0acl60ylD3t5DfDeeo
'' SIG '' 6ZfZKLS4Xb3fPUWzrCTX9l86mwFe141eHGgoJQNm7cw8
'' SIG '' XMn38F4S7vRzFN3S2EwCPdYEzVBewQPatRL0pQiipTfD
'' SIG '' ddGOIlNJ8iJH6UcWMgG0cquUD2DyRxgNE8tDw/N2gre/
'' SIG '' UWtCHQyDErsF5aVJ8iMscKw8pYHzhssrFgcEP47NuPW6
'' SIG '' kDmD3acjnYEXvLV3Rq4A6AXrlTivnEQpV6YpjWMK+taG
'' SIG '' dv5DzM1a80VGDJAV3vVqnUns4fLcrbrpWGHESveaooRd
'' SIG '' Iq0LOv1jkCZbUF+/ZcxVxPRRZZ/TIsdGrPguBz83fktG
'' SIG '' wTdwN10UTsAL9NeiArk/IWNSJ8lu48FZjfjpENc3ouui
'' SIG '' 61OUbQM9J08ceTnj8o502iLU0mODhrhlNUl2h+PSUj97
'' SIG '' fMhmAP76K21uFZ3ng+9tRYMGiU6BxZDiMIIHcTCCBVmg
'' SIG '' AwIBAgITMwAAABXF52ueAptJmQAAAAAAFTANBgkqhkiG
'' SIG '' 9w0BAQsFADCBiDELMAkGA1UEBhMCVVMxEzARBgNVBAgT
'' SIG '' Cldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAc
'' SIG '' BgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEyMDAG
'' SIG '' A1UEAxMpTWljcm9zb2Z0IFJvb3QgQ2VydGlmaWNhdGUg
'' SIG '' QXV0aG9yaXR5IDIwMTAwHhcNMjEwOTMwMTgyMjI1WhcN
'' SIG '' MzAwOTMwMTgzMjI1WjB8MQswCQYDVQQGEwJVUzETMBEG
'' SIG '' A1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9u
'' SIG '' ZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBvcmF0aW9u
'' SIG '' MSYwJAYDVQQDEx1NaWNyb3NvZnQgVGltZS1TdGFtcCBQ
'' SIG '' Q0EgMjAxMDCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCC
'' SIG '' AgoCggIBAOThpkzntHIhC3miy9ckeb0O1YLT/e6cBwfS
'' SIG '' qWxOdcjKNVf2AX9sSuDivbk+F2Az/1xPx2b3lVNxWuJ+
'' SIG '' Slr+uDZnhUYjDLWNE893MsAQGOhgfWpSg0S3po5GawcU
'' SIG '' 88V29YZQ3MFEyHFcUTE3oAo4bo3t1w/YJlN8OWECesSq
'' SIG '' /XJprx2rrPY2vjUmZNqYO7oaezOtgFt+jBAcnVL+tuhi
'' SIG '' JdxqD89d9P6OU8/W7IVWTe/dvI2k45GPsjksUZzpcGkN
'' SIG '' yjYtcI4xyDUoveO0hyTD4MmPfrVUj9z6BVWYbWg7mka9
'' SIG '' 7aSueik3rMvrg0XnRm7KMtXAhjBcTyziYrLNueKNiOSW
'' SIG '' rAFKu75xqRdbZ2De+JKRHh09/SDPc31BmkZ1zcRfNN0S
'' SIG '' idb9pSB9fvzZnkXftnIv231fgLrbqn427DZM9ituqBJR
'' SIG '' 6L8FA6PRc6ZNN3SUHDSCD/AQ8rdHGO2n6Jl8P0zbr17C
'' SIG '' 89XYcz1DTsEzOUyOArxCaC4Q6oRRRuLRvWoYWmEBc8pn
'' SIG '' ol7XKHYC4jMYctenIPDC+hIK12NvDMk2ZItboKaDIV1f
'' SIG '' MHSRlJTYuVD5C4lh8zYGNRiER9vcG9H9stQcxWv2XFJR
'' SIG '' XRLbJbqvUAV6bMURHXLvjflSxIUXk8A8FdsaN8cIFRg/
'' SIG '' eKtFtvUeh17aj54WcmnGrnu3tz5q4i6tAgMBAAGjggHd
'' SIG '' MIIB2TASBgkrBgEEAYI3FQEEBQIDAQABMCMGCSsGAQQB
'' SIG '' gjcVAgQWBBQqp1L+ZMSavoKRPEY1Kc8Q/y8E7jAdBgNV
'' SIG '' HQ4EFgQUn6cVXQBeYl2D9OXSZacbUzUZ6XIwXAYDVR0g
'' SIG '' BFUwUzBRBgwrBgEEAYI3TIN9AQEwQTA/BggrBgEFBQcC
'' SIG '' ARYzaHR0cDovL3d3dy5taWNyb3NvZnQuY29tL3BraW9w
'' SIG '' cy9Eb2NzL1JlcG9zaXRvcnkuaHRtMBMGA1UdJQQMMAoG
'' SIG '' CCsGAQUFBwMIMBkGCSsGAQQBgjcUAgQMHgoAUwB1AGIA
'' SIG '' QwBBMAsGA1UdDwQEAwIBhjAPBgNVHRMBAf8EBTADAQH/
'' SIG '' MB8GA1UdIwQYMBaAFNX2VsuP6KJcYmjRPZSQW9fOmhjE
'' SIG '' MFYGA1UdHwRPME0wS6BJoEeGRWh0dHA6Ly9jcmwubWlj
'' SIG '' cm9zb2Z0LmNvbS9wa2kvY3JsL3Byb2R1Y3RzL01pY1Jv
'' SIG '' b0NlckF1dF8yMDEwLTA2LTIzLmNybDBaBggrBgEFBQcB
'' SIG '' AQROMEwwSgYIKwYBBQUHMAKGPmh0dHA6Ly93d3cubWlj
'' SIG '' cm9zb2Z0LmNvbS9wa2kvY2VydHMvTWljUm9vQ2VyQXV0
'' SIG '' XzIwMTAtMDYtMjMuY3J0MA0GCSqGSIb3DQEBCwUAA4IC
'' SIG '' AQCdVX38Kq3hLB9nATEkW+Geckv8qW/qXBS2Pk5HZHix
'' SIG '' BpOXPTEztTnXwnE2P9pkbHzQdTltuw8x5MKP+2zRoZQY
'' SIG '' Iu7pZmc6U03dmLq2HnjYNi6cqYJWAAOwBb6J6Gngugnu
'' SIG '' e99qb74py27YP0h1AdkY3m2CDPVtI1TkeFN1JFe53Z/z
'' SIG '' jj3G82jfZfakVqr3lbYoVSfQJL1AoL8ZthISEV09J+BA
'' SIG '' ljis9/kpicO8F7BUhUKz/AyeixmJ5/ALaoHCgRlCGVJ1
'' SIG '' ijbCHcNhcy4sa3tuPywJeBTpkbKpW99Jo3QMvOyRgNI9
'' SIG '' 5ko+ZjtPu4b6MhrZlvSP9pEB9s7GdP32THJvEKt1MMU0
'' SIG '' sHrYUP4KWN1APMdUbZ1jdEgssU5HLcEUBHG/ZPkkvnNt
'' SIG '' yo4JvbMBV0lUZNlz138eW0QBjloZkWsNn6Qo3GcZKCS6
'' SIG '' OEuabvshVGtqRRFHqfG3rsjoiV5PndLQTHa1V1QJsWkB
'' SIG '' RH58oWFsc/4Ku+xBZj1p/cvBQUl+fpO+y/g75LcVv7TO
'' SIG '' PqUxUYS8vwLBgqJ7Fx0ViY1w/ue10CgaiQuPNtq6TPmb
'' SIG '' /wrpNPgkNWcr4A245oyZ1uEi6vAnQj0llOZ0dFtq0Z4+
'' SIG '' 7X6gMTN9vMvpe784cETRkPHIqzqKOghif9lwY1NNje6C
'' SIG '' baUFEMFxBmoQtB1VM1izoXBm8qGCA1kwggJBAgEBMIIB
'' SIG '' AaGB2aSB1jCB0zELMAkGA1UEBhMCVVMxEzARBgNVBAgT
'' SIG '' Cldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAc
'' SIG '' BgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEtMCsG
'' SIG '' A1UECxMkTWljcm9zb2Z0IElyZWxhbmQgT3BlcmF0aW9u
'' SIG '' cyBMaW1pdGVkMScwJQYDVQQLEx5uU2hpZWxkIFRTUyBF
'' SIG '' U046NEMxQS0wNUUwLUQ5NDcxJTAjBgNVBAMTHE1pY3Jv
'' SIG '' c29mdCBUaW1lLVN0YW1wIFNlcnZpY2WiIwoBATAHBgUr
'' SIG '' DgMCGgMVAKkTjGzEvCXFJXJz5MESxUT1xbKZoIGDMIGA
'' SIG '' pH4wfDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hp
'' SIG '' bmd0b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoT
'' SIG '' FU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEmMCQGA1UEAxMd
'' SIG '' TWljcm9zb2Z0IFRpbWUtU3RhbXAgUENBIDIwMTAwDQYJ
'' SIG '' KoZIhvcNAQELBQACBQDq4j4mMCIYDzIwMjQxMTE1MjEz
'' SIG '' OTUwWhgPMjAyNDExMTYyMTM5NTBaMHcwPQYKKwYBBAGE
'' SIG '' WQoEATEvMC0wCgIFAOriPiYCAQAwCgIBAAICF2MCAf8w
'' SIG '' BwIBAAICEzEwCgIFAOrjj6YCAQAwNgYKKwYBBAGEWQoE
'' SIG '' AjEoMCYwDAYKKwYBBAGEWQoDAqAKMAgCAQACAwehIKEK
'' SIG '' MAgCAQACAwGGoDANBgkqhkiG9w0BAQsFAAOCAQEARAPy
'' SIG '' u9McWU6Pz0wtTxrEexnECVpN1+3/v+vfUGdC7eBbe9VH
'' SIG '' 2P+fEFA90xjB+iDWh8F9b783awOuzHP3DPH77g2yhEPL
'' SIG '' FHrt2IkpjeL0Cd9RfEWlRIakuaHipCixBupJ3O+0ECcx
'' SIG '' ATCDtGx77HHB77JIZkEOHRcem/Aehlh/g9hnnB41gIu8
'' SIG '' +YiHLDjAuuz6poXCNg2oZExE+4JcUMl23PbhdkTayFic
'' SIG '' INrrz2iNrIyZuLjJgeTgzhP6KtgIZ+ixa5PzMthGjeSf
'' SIG '' AbscAAXG9TVX7uWnyJXAnW7Qrz1pwbW+GFostaIufKTc
'' SIG '' mdEiGueMBMekIY+OnHCBxnSIAF/1ETGCBA0wggQJAgEB
'' SIG '' MIGTMHwxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNo
'' SIG '' aW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQK
'' SIG '' ExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xJjAkBgNVBAMT
'' SIG '' HU1pY3Jvc29mdCBUaW1lLVN0YW1wIFBDQSAyMDEwAhMz
'' SIG '' AAAB/xI4fPfBZdahAAEAAAH/MA0GCWCGSAFlAwQCAQUA
'' SIG '' oIIBSjAaBgkqhkiG9w0BCQMxDQYLKoZIhvcNAQkQAQQw
'' SIG '' LwYJKoZIhvcNAQkEMSIEIGdQ0iO4aRvi5sHjOWSR2UaJ
'' SIG '' O+j7zPZa+B0fjh5EFMObMIH6BgsqhkiG9w0BCRACLzGB
'' SIG '' 6jCB5zCB5DCBvQQg5DLvvskCd2msnCLjE+rwOyTbjGlT
'' SIG '' iN6g40hFfLqcp/0wgZgwgYCkfjB8MQswCQYDVQQGEwJV
'' SIG '' UzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMH
'' SIG '' UmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBv
'' SIG '' cmF0aW9uMSYwJAYDVQQDEx1NaWNyb3NvZnQgVGltZS1T
'' SIG '' dGFtcCBQQ0EgMjAxMAITMwAAAf8SOHz3wWXWoQABAAAB
'' SIG '' /zAiBCCTyw4w84ptMqGv0dN5B3RbitPIsEHvqBhFM1mm
'' SIG '' Sf5K3zANBgkqhkiG9w0BAQsFAASCAgAR8Gikyxw2V70z
'' SIG '' Wlm+Sf4FDBD2GkGRMPOJZWTl3hpNXdv1g/vCMrtKdYaL
'' SIG '' i9DVj3DcEZaN4K9LS7PSNOAt0n3BvqYRnwBxrVP3SOkh
'' SIG '' XnwKCRkrvTid0wnTJ8+O5LyQeo5txN1u3whSpzSZXZI4
'' SIG '' WEBGnJzSlFaMoPbyHhC2mX+MbPveh97gYc/NTQaTwQpA
'' SIG '' cWCkiu5KEIfBiNNwV0LTbrjKBGMUzkZ8bYa2HfPr9DxQ
'' SIG '' FKHgalImN1Z2F2z1r6Kmp087o2KmMAsTfKQBi22lQ/rC
'' SIG '' IOzYdA8kntord2egyuhSF9k/6Fvyut1Sj2fMfk3hqal4
'' SIG '' ynh1p2uYGQlElG4rkosQYIPS60LNZDdneycIIugdIw11
'' SIG '' C+7550AdA82GmUIGy78hwIY53FQ/ETOw39p7B09Xp8+y
'' SIG '' kedOvXT2FVTrjgrnBvsw5BwOvJ/EIrKt8BluTUw/ZSuc
'' SIG '' OmPyaIgHAR3LOElbi6mx1zkhiQHse5HgjD2CHUC4K3c8
'' SIG '' s4VJko/lBalIEZSQWeKU2vPRXSw76kKyPaAn0RM9vlhZ
'' SIG '' 3spKLZGbJ589WEAmqkIwX7jhr/A+9eWwOG3i99qestCI
'' SIG '' fvZabgE+FZYkI7+qXQnJkD1Drs28WvP12WYTZgeB9Phu
'' SIG '' NYZba569B82zm7vWmvC4ouKQfGjBaBZO/JsLp+oH0u3f
'' SIG '' Kq+SjCXUPA==
'' SIG '' End signature block
