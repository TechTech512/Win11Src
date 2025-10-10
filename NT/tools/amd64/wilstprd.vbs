' Windows Installer utility to list registered products and product info
' For use with Windows Scripting Host, CScript.exe or WScript.exe
' Copyright (c) Microsoft Corporation. All rights reserved.
' Demonstrates the use of the product enumeration and ProductInfo methods and underlying APIs
'
Option Explicit

Const msiInstallStateNotUsed      = -7
Const msiInstallStateBadConfig    = -6
Const msiInstallStateIncomplete   = -5
Const msiInstallStateSourceAbsent = -4
Const msiInstallStateInvalidArg   = -2
Const msiInstallStateUnknown      = -1
Const msiInstallStateBroken       =  0
Const msiInstallStateAdvertised   =  1
Const msiInstallStateRemoved      =  1
Const msiInstallStateAbsent       =  2
Const msiInstallStateLocal        =  3
Const msiInstallStateSource       =  4
Const msiInstallStateDefault      =  5

' Connect to Windows Installer object
On Error Resume Next
Dim installer : Set installer = Nothing
Set installer = Wscript.CreateObject("WindowsInstaller.Installer") : CheckError

' If no arguments supplied, then list all installed or advertised products
Dim argCount:argCount = Wscript.Arguments.Count
If (argCount = 0) Then
	Dim product, products, info, productList, version
	On Error Resume Next
	Set products = installer.Products : CheckError
	For Each product In products
		version = DecodeVersion(installer.ProductInfo(product, "Version")) : CheckError
		info = product & " = " & installer.ProductInfo(product, "ProductName") & " " & version : CheckError
		If productList <> Empty Then productList = productList & vbNewLine & info Else productList = info
	Next
	If productList = Empty Then productList = "No products installed or advertised"
	Wscript.Echo productList
	Set products = Nothing
	Wscript.Quit 0
End If

' Check for ?, and show help message if found
Dim productName:productName = Wscript.Arguments(0)
If InStr(1, productName, "?", vbTextCompare) > 0 Then
	Wscript.Echo "Windows Installer utility to list registered products and product information" &_
		vbNewLine & " Lists all installed and advertised products if no arguments are specified" &_
		vbNewLine & " Else 1st argument is a product name (case-insensitive) or product ID (GUID)" &_
		vbNewLine & " If 2nd argument is missing or contains 'p', then product properties are listed" &_
		vbNewLine & " If 2nd argument contains 'f', features, parents, & installed states are listed" &_
		vbNewLine & " If 2nd argument contains 'c', installed components for that product are listed" &_
		vbNewLine & " If 2nd argument contains 'd', HKLM ""SharedDlls"" count for key files are listed" &_
		vbNewLine &_
		vbNewLine & "Copyright (C) Microsoft Corporation.  All rights reserved."
	Wscript.Quit 1
End If

' If Product name supplied, need to search for product code
Dim productCode, property, value, message
If Left(productName, 1) = "{" And Right(productName, 1) = "}" Then
	If installer.ProductState(productName) <> msiInstallStateUnknown Then productCode = UCase(productName)
Else
	For Each productCode In installer.Products : CheckError
		If LCase(installer.ProductInfo(productCode, "ProductName")) = LCase(productName) Then Exit For
	Next
End If
If IsEmpty(productCode) Then Wscript.Echo "Product is not registered: " & productName : Wscript.Quit 2

' Check option argument for type of information to display, default is properties
Dim optionFlag : If argcount > 1 Then optionFlag = LCase(Wscript.Arguments(1)) Else optionFlag = "p"
If InStr(1, optionFlag, "*", vbTextCompare) > 0 Then optionFlag = "pfcd"

If InStr(1, optionFlag, "p", vbTextCompare) > 0 Then
	message = "ProductCode = " & productCode
	For Each property In Array(_
			"Language",_
			"ProductName",_
			"PackageCode",_
			"Transforms",_
			"AssignmentType",_
			"PackageName",_
			"InstalledProductName",_
			"VersionString",_
			"RegCompany",_
			"RegOwner",_
			"ProductID",_
			"ProductIcon",_
			"InstallLocation",_
			"InstallSource",_
			"InstallDate",_
			"Publisher",_
			"LocalPackage",_
			"HelpLink",_
			"HelpTelephone",_
			"URLInfoAbout",_
			"URLUpdateInfo") : CheckError
		value = installer.ProductInfo(productCode, property) ': CheckError
		If Err <> 0 Then Err.Clear : value = Empty
		If (property = "Version") Then value = DecodeVersion(value)
		If value <> Empty Then message = message & vbNewLine & property & " = " & value
	Next
	Wscript.Echo message
End If

If InStr(1, optionFlag, "f", vbTextCompare) > 0 Then
	Dim feature, features, parent, state, featureInfo
	Set features = installer.Features(productCode)
	message = "---Features in product " & productCode & "---"
	For Each feature In features
		parent = installer.FeatureParent(productCode, feature) : CheckError
		If Len(parent) Then parent = " {" & parent & "}"
		state = installer.FeatureState(productCode, feature)
		Select Case(state)
			Case msiInstallStateBadConfig:    state = "Corrupt"
			Case msiInstallStateIncomplete:   state = "InProgress"
			Case msiInstallStateSourceAbsent: state = "SourceAbsent"
			Case msiInstallStateBroken:       state = "Broken"
			Case msiInstallStateAdvertised:   state = "Advertised"
			Case msiInstallStateAbsent:       state = "Uninstalled"
			Case msiInstallStateLocal:        state = "Local"
			Case msiInstallStateSource:       state = "Source"
			Case msiInstallStateDefault:      state = "Default"
			Case Else:                        state = "Unknown"
		End Select
		message = message & vbNewLine & feature & parent & " = " & state
	Next
	Set features = Nothing
	Wscript.Echo message
End If 

If InStr(1, optionFlag, "c", vbTextCompare) > 0 Then
	Dim component, components, client, clients, path
	Set components = installer.Components : CheckError
	message = "---Components in product " & productCode & "---"
	For Each component In components
		Set clients = installer.ComponentClients(component) : CheckError
		For Each client In Clients
			If client = productCode Then
				path = installer.ComponentPath(productCode, component) : CheckError
				message = message & vbNewLine & component & " = " & path
				Exit For
			End If
		Next
		Set clients = Nothing
	Next
	Set components = Nothing
	Wscript.Echo message
End If

If InStr(1, optionFlag, "d", vbTextCompare) > 0 Then
	Set components = installer.Components : CheckError
	message = "---Shared DLL counts for key files of " & productCode & "---"
	For Each component In components
		Set clients = installer.ComponentClients(component) : CheckError
		For Each client In Clients
			If client = productCode Then
				path = installer.ComponentPath(productCode, component) : CheckError
				If Len(path) = 0 Then path = "0"
				If AscW(path) >= 65 Then  ' ignore registry key paths
					value = installer.RegistryValue(2, "SOFTWARE\Microsoft\Windows\CurrentVersion\SharedDlls", path)
					If Err <> 0 Then value = 0 : Err.Clear
					message = message & vbNewLine & value & " = " & path
				End If
				Exit For
			End If
		Next
		Set clients = Nothing
	Next
	Set components = Nothing
	Wscript.Echo message
End If

Function DecodeVersion(version)
	version = CLng(version)
	DecodeVersion = version\65536\256 & "." & (version\65535 MOD 256) & "." & (version Mod 65536)
End Function

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
'' SIG '' MIImTQYJKoZIhvcNAQcCoIImPjCCJjoCAQExDzANBglg
'' SIG '' hkgBZQMEAgEFADB3BgorBgEEAYI3AgEEoGkwZzAyBgor
'' SIG '' BgEEAYI3AgEeMCQCAQEEEE7wKRaZJ7VNj+Ws4Q8X66sC
'' SIG '' AQACAQACAQACAQACAQAwMTANBglghkgBZQMEAgEFAAQg
'' SIG '' +TYCWFk7lUqBMQntWKZoHVk2tbD50YMJse1NdDP1q+Gg
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
'' SIG '' 9w0BCQQxIgQgV38KrmksrNZmRe2PIRhB+jOdW9FWVWoy
'' SIG '' VJi61CV/fDAwWgYKKwYBBAGCNwIBDDFMMEqgJIAiAE0A
'' SIG '' aQBjAHIAbwBzAG8AZgB0ACAAVwBpAG4AZABvAHcAc6Ei
'' SIG '' gCBodHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vd2luZG93
'' SIG '' czANBgkqhkiG9w0BAQEFAASCAQArobxRwmgQT28DX7cN
'' SIG '' kfBeFXHZwHG1b4rFUpDnKl+QbjgZ38CcOgSykoNbA7X/
'' SIG '' 6GBdb5tL9Mv++hO8Jk6orKHeGQa6BNPKxiiC0CJC/Zia
'' SIG '' 2YL4aDvhb9Z3tSqMFSbYPPG+zoZLDxF+ifn/ftv1S88L
'' SIG '' 15m0+TAVrurCpmIAAkDJKs+eYdDzVGI/jboaXbx7qboN
'' SIG '' zqYG9YSM8D2xEHZBqpPoT7uX+B/jpCrx3uDqw7OBLsN6
'' SIG '' fAUIqxx9zM2TJI2zgQHL2C7/gpXmJcKC1W9sYekC+tAL
'' SIG '' og0qEvluGS0uYMtQtU/Ju+oEHNW3D/HHrvZTX/m7Bxc8
'' SIG '' /LLjSl/kfGKO7fuJoYIXsDCCF6wGCisGAQQBgjcDAwEx
'' SIG '' ghecMIIXmAYJKoZIhvcNAQcCoIIXiTCCF4UCAQMxDzAN
'' SIG '' BglghkgBZQMEAgEFADCCAVoGCyqGSIb3DQEJEAEEoIIB
'' SIG '' SQSCAUUwggFBAgEBBgorBgEEAYRZCgMBMDEwDQYJYIZI
'' SIG '' AWUDBAIBBQAEIA2Plwmt5dqza5a0ZWG7oenEN5LO1Tnq
'' SIG '' xl/RLXzUEavsAgZm63lQoDYYEzIwMjQxMTE2MDkxNjQy
'' SIG '' LjY4M1owBIACAfSggdmkgdYwgdMxCzAJBgNVBAYTAlVT
'' SIG '' MRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdS
'' SIG '' ZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9y
'' SIG '' YXRpb24xLTArBgNVBAsTJE1pY3Jvc29mdCBJcmVsYW5k
'' SIG '' IE9wZXJhdGlvbnMgTGltaXRlZDEnMCUGA1UECxMeblNo
'' SIG '' aWVsZCBUU1MgRVNOOjY1MUEtMDVFMC1EOTQ3MSUwIwYD
'' SIG '' VQQDExxNaWNyb3NvZnQgVGltZS1TdGFtcCBTZXJ2aWNl
'' SIG '' oIIR/jCCBygwggUQoAMCAQICEzMAAAH1mQmUvPHGUIwA
'' SIG '' AQAAAfUwDQYJKoZIhvcNAQELBQAwfDELMAkGA1UEBhMC
'' SIG '' VVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcT
'' SIG '' B1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jw
'' SIG '' b3JhdGlvbjEmMCQGA1UEAxMdTWljcm9zb2Z0IFRpbWUt
'' SIG '' U3RhbXAgUENBIDIwMTAwHhcNMjQwNzI1MTgzMTAxWhcN
'' SIG '' MjUxMDIyMTgzMTAxWjCB0zELMAkGA1UEBhMCVVMxEzAR
'' SIG '' BgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1v
'' SIG '' bmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlv
'' SIG '' bjEtMCsGA1UECxMkTWljcm9zb2Z0IElyZWxhbmQgT3Bl
'' SIG '' cmF0aW9ucyBMaW1pdGVkMScwJQYDVQQLEx5uU2hpZWxk
'' SIG '' IFRTUyBFU046NjUxQS0wNUUwLUQ5NDcxJTAjBgNVBAMT
'' SIG '' HE1pY3Jvc29mdCBUaW1lLVN0YW1wIFNlcnZpY2UwggIi
'' SIG '' MA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoICAQDM73Rw
'' SIG '' VBNZ39Y/zghPskwhbV9AvrWx1+CaGV9PSe9gRvaS+Q0X
'' SIG '' TvdnCO965Jai3fzsuMTRMKIb3d7ojQfMgVAGdvEY/9Y8
'' SIG '' FSKsWrtYTlECy6E19hQv48hv2MmrcLBbEgJ/Dm3+lPIg
'' SIG '' 4eMq+jWVA5NZnKmKv+mxnAQTvLa5YA9tklMWsp6flHvf
'' SIG '' HYdvHLh5bUNyZePKbbAVa/XSwEfjRqMl9746TBxN2hit
'' SIG '' jcqSk39FBKN7JwrRuGOjQIZghhr5kwBqjRI1H9HUnVjq
'' SIG '' wuSIIk7dpCttLVLPuX7+omDLx/IRkw0PkyzsLSwRo6+g
'' SIG '' EeJZKlMzi9zTEMsKZzo8a/TcK1a7YqLKqsvwEAHURjI5
'' SIG '' KEjchPv1qsMgOsv5173UV+OZJsFjmP4e9LSXd1eSM/ce
'' SIG '' ifxvviVbCKQXSvMSsXSfeSFUC6zHtKbWgYb7TqHP1cDL
'' SIG '' dai75PpJ7qhrksOJCA9N9ZH+P0U2Twm7TqhJ9OHpzTdX
'' SIG '' S6WVrQjDL4fNSX5aZjEUtTQ+JpeyaC503BWqfnXOv4GL
'' SIG '' dc8nznBa7LoYZPucEOZc3NM2TMr3wMFCNM5ptBdRnzzh
'' SIG '' hv0MU1yKCZ5VNiTJRdnqGxx3w3KrjkDcPduT6deeyiAr
'' SIG '' VnvNmPpdsZ+3vGA5i+95TqnT5+u2FsXsxe/6LmpwP0d8
'' SIG '' WY6rhVgd69V6xhvo/QIDAQABo4IBSTCCAUUwHQYDVR0O
'' SIG '' BBYEFFXgfFv1SjSgcPAlkl7baLF7YHUBMB8GA1UdIwQY
'' SIG '' MBaAFJ+nFV0AXmJdg/Tl0mWnG1M1GelyMF8GA1UdHwRY
'' SIG '' MFYwVKBSoFCGTmh0dHA6Ly93d3cubWljcm9zb2Z0LmNv
'' SIG '' bS9wa2lvcHMvY3JsL01pY3Jvc29mdCUyMFRpbWUtU3Rh
'' SIG '' bXAlMjBQQ0ElMjAyMDEwKDEpLmNybDBsBggrBgEFBQcB
'' SIG '' AQRgMF4wXAYIKwYBBQUHMAKGUGh0dHA6Ly93d3cubWlj
'' SIG '' cm9zb2Z0LmNvbS9wa2lvcHMvY2VydHMvTWljcm9zb2Z0
'' SIG '' JTIwVGltZS1TdGFtcCUyMFBDQSUyMDIwMTAoMSkuY3J0
'' SIG '' MAwGA1UdEwEB/wQCMAAwFgYDVR0lAQH/BAwwCgYIKwYB
'' SIG '' BQUHAwgwDgYDVR0PAQH/BAQDAgeAMA0GCSqGSIb3DQEB
'' SIG '' CwUAA4ICAQBzDGZU9oD3Ed9+6ibRM5KnaGym5/UbRwdb
'' SIG '' 8pccC6Gelbz9K+WrmP1ooj/z8bp8YhAyvTOWlq7yPLzc
'' SIG '' jjNyUZ0mOXlLTZLEOVQprC1a+B/uJ1rTo+CN5AzV5fgu
'' SIG '' 63hts99PQUSnvsbvqGHKxfFMk0e/nL5/BOFR6KJyWKFn
'' SIG '' Cpxkylrjqb6hXqKBNTojQSed6i0yoWzRDfMeBVWvhZeO
'' SIG '' cbYFyeSKnjZ53KD/2JdzOpMGsSS9PPRSWW2kUZpCcvOr
'' SIG '' 42jxUSCrRQbtbUQtkaGabEWYcAHBNPqw8kVXrwN8ugLS
'' SIG '' IH1Btv1Vnya9tNXkm0hIGSVO5UCUSTNeL0siM2bH6Sd0
'' SIG '' F8o/x3Eb/FtFem1ANANoKxLqiAuTAuAfrNKz66X1abMj
'' SIG '' QXzMiZuGdmFTOIgeF4Wjgf5miiM9hyBMrr/duRJs5puZ
'' SIG '' AV/3kHwHp7lapdtLmz050x1SVbWBMjWvm75YDAfYobt3
'' SIG '' Gd6hNt/+NiXdNS0/sAenJyTZzSe6f9DQLJylr1BQf8PL
'' SIG '' TWTq1CiY1caOK+Db8EZyBknQfDwLopV6UQnfXEugTbWb
'' SIG '' 340SBIoJGgUTUuSZrfVLIhrKdt1gRvyw6VnKcx2bzI+V
'' SIG '' 0PC4Xni8mIQCuOtwM1d7oGhtlSJNZIq+/UMlp1HVJQI7
'' SIG '' 853bUaBT6Fmq750qCMoBh15Mi+L1Hau0tjCCB3EwggVZ
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
'' SIG '' RVNOOjY1MUEtMDVFMC1EOTQ3MSUwIwYDVQQDExxNaWNy
'' SIG '' b3NvZnQgVGltZS1TdGFtcCBTZXJ2aWNloiMKAQEwBwYF
'' SIG '' Kw4DAhoDFQAmwAq7jw1tHlhGDdZIFALKPN2S9qCBgzCB
'' SIG '' gKR+MHwxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNo
'' SIG '' aW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQK
'' SIG '' ExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xJjAkBgNVBAMT
'' SIG '' HU1pY3Jvc29mdCBUaW1lLVN0YW1wIFBDQSAyMDEwMA0G
'' SIG '' CSqGSIb3DQEBCwUAAgUA6uJq5DAiGA8yMDI0MTExNjAw
'' SIG '' NTA0NFoYDzIwMjQxMTE3MDA1MDQ0WjB3MD0GCisGAQQB
'' SIG '' hFkKBAExLzAtMAoCBQDq4mrkAgEAMAoCAQACAgjNAgH/
'' SIG '' MAcCAQACAhQxMAoCBQDq47xkAgEAMDYGCisGAQQBhFkK
'' SIG '' BAIxKDAmMAwGCisGAQQBhFkKAwKgCjAIAgEAAgMHoSCh
'' SIG '' CjAIAgEAAgMBhqAwDQYJKoZIhvcNAQELBQADggEBAAJ4
'' SIG '' +acot0dvStLzdet/49aEGY4ByAUaYGWCLi+yd0kdXCM3
'' SIG '' 7CBMhnibU2N4y+SIKs3k5vsyuK8MLnwfj1jrLNYBx1sf
'' SIG '' aF1sFRqo7/2naAZeCnIwK5ImrABfnYzL2oExY5f8fmOb
'' SIG '' cOLH7WpDoVW/waz48dZZZ+LIzh3KWz2KueSdQLomKUWi
'' SIG '' dD3JUhgTa1RJNBPx1VDl8keCBNs6Q2xX231FX5eQdaZt
'' SIG '' iKgD9qimKKnxeYrhjrCfq0tueLSwyA7WodkesVf0e6q7
'' SIG '' Gqg6KYhIDb3SYf71S4HFt8s9t2aAbSvAkgwx+ac14Moz
'' SIG '' Go1icviSl9LqWeQNfo5WS7Dl6/amlWIxggQNMIIECQIB
'' SIG '' ATCBkzB8MQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2Fz
'' SIG '' aGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UE
'' SIG '' ChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMSYwJAYDVQQD
'' SIG '' Ex1NaWNyb3NvZnQgVGltZS1TdGFtcCBQQ0EgMjAxMAIT
'' SIG '' MwAAAfWZCZS88cZQjAABAAAB9TANBglghkgBZQMEAgEF
'' SIG '' AKCCAUowGgYJKoZIhvcNAQkDMQ0GCyqGSIb3DQEJEAEE
'' SIG '' MC8GCSqGSIb3DQEJBDEiBCB/oP5phpzEuZmyweFNofT9
'' SIG '' e49HPs6lOX4B5fX3E0OTwjCB+gYLKoZIhvcNAQkQAi8x
'' SIG '' geowgecwgeQwgb0EIMHW8tIXCHT0hK7iR0S/j+2D15HV
'' SIG '' iTzDnHuPkZOGpo81MIGYMIGApH4wfDELMAkGA1UEBhMC
'' SIG '' VVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcT
'' SIG '' B1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jw
'' SIG '' b3JhdGlvbjEmMCQGA1UEAxMdTWljcm9zb2Z0IFRpbWUt
'' SIG '' U3RhbXAgUENBIDIwMTACEzMAAAH1mQmUvPHGUIwAAQAA
'' SIG '' AfUwIgQg2Z/iN5BINt2zf5s2jC8lJC7gdsb3sMOWP8qU
'' SIG '' YZn0EhAwDQYJKoZIhvcNAQELBQAEggIAgDiX0OW/HWxg
'' SIG '' JieZHHieh/i8FfHjQSzMXcNty9zIg0kvTtdSsAaYK4X0
'' SIG '' pn1FxkcQbdG5gP+VGV/3BCms2lU8OWUvdG6UoHiO3Oki
'' SIG '' UGF8e+Sb2EdBDrMgmbeE8lWmX13U+qyHMFh8K6RDU2ip
'' SIG '' Wbj1wnz7i9WpmMh1hV4NUMigWVadYCQqjfWSFo9liBhV
'' SIG '' PwmEi9sYQHExV40lhuUFAw1MLQtkPV5tYvlGGT/IgRVF
'' SIG '' /4LH1Y8ehKyl6uVm9X070l72XzHlIl87r+hieUJk+FxY
'' SIG '' qdhELkSIPkWLNuezvB5E0EK8B6HVEC3x3EaJPOs7h5xv
'' SIG '' FdMAUf4RzM6ACLQbuWnr2UNGJvcqks5yHVQnmTYOlMPo
'' SIG '' mEc6X9xBVjTjhHXx06S1ziOnQTyxqvZ0PdkK+TXJ8d15
'' SIG '' MAZLS+T6FAxsvjfHC9L9pXEW8BcGlZiglqFsv9zyZ7kl
'' SIG '' TCd58AEhOcE4Cntxrsykjzt2wC8ZV42pBjliesgDyotA
'' SIG '' k2sXWhOl4ps50CBdLMMC3K8A74Nmzoi0cxQ+nzFiLuBE
'' SIG '' ZapuUJKwshqfKYPrVYZ7Ea1MOQlZ54eHzzqIRiiEE1P7
'' SIG '' lM8FjWPHSxFRynL6w52/zAyWXswWK3Wmhqinujd52hKP
'' SIG '' Z2nlxVycaXUqmPTHemYHAbAEnczJFHXGbNUDtVPceSJm
'' SIG '' Dj6dq8mZAps=
'' SIG '' End signature block
