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
'' SIG '' MIImMAYJKoZIhvcNAQcCoIImITCCJh0CAQExDzANBglg
'' SIG '' hkgBZQMEAgEFADB3BgorBgEEAYI3AgEEoGkwZzAyBgor
'' SIG '' BgEEAYI3AgEeMCQCAQEEEE7wKRaZJ7VNj+Ws4Q8X66sC
'' SIG '' AQACAQACAQACAQACAQAwMTANBglghkgBZQMEAgEFAAQg
'' SIG '' +TYCWFk7lUqBMQntWKZoHVk2tbD50YMJse1NdDP1q+Gg
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
'' SIG '' jgd7JXFEqwZq5tTG3yOalnXFMYIaITCCGh0CAQEwgZUw
'' SIG '' fjELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0
'' SIG '' b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1p
'' SIG '' Y3Jvc29mdCBDb3Jwb3JhdGlvbjEoMCYGA1UEAxMfTWlj
'' SIG '' cm9zb2Z0IENvZGUgU2lnbmluZyBQQ0EgMjAxMAITMwAA
'' SIG '' BVfPkN3H0cCIjAAAAAAFVzANBglghkgBZQMEAgEFAKCB
'' SIG '' xjAZBgkqhkiG9w0BCQMxDAYKKwYBBAGCNwIBBDAcBgor
'' SIG '' BgEEAYI3AgELMQ4wDAYKKwYBBAGCNwIBFTAvBgkqhkiG
'' SIG '' 9w0BCQQxIgQgV38KrmksrNZmRe2PIRhB+jOdW9FWVWoy
'' SIG '' VJi61CV/fDAwWgYKKwYBBAGCNwIBDDFMMEqgJIAiAE0A
'' SIG '' aQBjAHIAbwBzAG8AZgB0ACAAVwBpAG4AZABvAHcAc6Ei
'' SIG '' gCBodHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vd2luZG93
'' SIG '' czANBgkqhkiG9w0BAQEFAASCAQB23lhV9jSzA54vTTlI
'' SIG '' Md/Cqdvj3N/wP0SCfGj3DDYkSHm3bF+QobL4bsZN5deJ
'' SIG '' tIxwbflpl4Lm6wOMABJxwg9oKn2TWp0KtuX78JYOn9th
'' SIG '' MrPaAptYKde6qop2l5c1u+6gTYBLroKGTA8cAB/rUWzH
'' SIG '' QkpRhjQNjLA5M5GImsFqfP6MTFumw4GnIqG6lk5AwI0/
'' SIG '' /4fiATs1Ciza86PUJ9qNq1V1G0GyOpmoFCm8X6qOAolz
'' SIG '' CxmW97xnDfVT8StVpaQkKtWH0SJvtyudtHux/Y/5WXkv
'' SIG '' qP+6ecHNEIggkiQ1X0pWHrbwtmeGGQdidMkC5tpP6DVt
'' SIG '' r/uZ2FqgAR5p6OIroYIXkzCCF48GCisGAQQBgjcDAwEx
'' SIG '' ghd/MIIXewYJKoZIhvcNAQcCoIIXbDCCF2gCAQMxDzAN
'' SIG '' BglghkgBZQMEAgEFADCCAVEGCyqGSIb3DQEJEAEEoIIB
'' SIG '' QASCATwwggE4AgEBBgorBgEEAYRZCgMBMDEwDQYJYIZI
'' SIG '' AWUDBAIBBQAEIOK+LuSGKjNWiGIbuGAUK4SDdR2yH50L
'' SIG '' SLwXvkAcgTSCAgZmvkRiaKoYEjIwMjQwOTA1MDkxNzEy
'' SIG '' LjkyWjAEgAIB9KCB0aSBzjCByzELMAkGA1UEBhMCVVMx
'' SIG '' EzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1Jl
'' SIG '' ZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3Jh
'' SIG '' dGlvbjElMCMGA1UECxMcTWljcm9zb2Z0IEFtZXJpY2Eg
'' SIG '' T3BlcmF0aW9uczEnMCUGA1UECxMeblNoaWVsZCBUU1Mg
'' SIG '' RVNOOjMzMDMtMDVFMC1EOTQ3MSUwIwYDVQQDExxNaWNy
'' SIG '' b3NvZnQgVGltZS1TdGFtcCBTZXJ2aWNloIIR6jCCByAw
'' SIG '' ggUIoAMCAQICEzMAAAHm2UKe6gD4feEAAQAAAeYwDQYJ
'' SIG '' KoZIhvcNAQELBQAwfDELMAkGA1UEBhMCVVMxEzARBgNV
'' SIG '' BAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQx
'' SIG '' HjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEm
'' SIG '' MCQGA1UEAxMdTWljcm9zb2Z0IFRpbWUtU3RhbXAgUENB
'' SIG '' IDIwMTAwHhcNMjMxMjA2MTg0NTE1WhcNMjUwMzA1MTg0
'' SIG '' NTE1WjCByzELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldh
'' SIG '' c2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNV
'' SIG '' BAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjElMCMGA1UE
'' SIG '' CxMcTWljcm9zb2Z0IEFtZXJpY2EgT3BlcmF0aW9uczEn
'' SIG '' MCUGA1UECxMeblNoaWVsZCBUU1MgRVNOOjMzMDMtMDVF
'' SIG '' MC1EOTQ3MSUwIwYDVQQDExxNaWNyb3NvZnQgVGltZS1T
'' SIG '' dGFtcCBTZXJ2aWNlMIICIjANBgkqhkiG9w0BAQEFAAOC
'' SIG '' Ag8AMIICCgKCAgEAvb6YfOLYJbhM5v8KTSpQI3BJRs35
'' SIG '' bygA2dQ/tnf4nmGBLRdhyjKyhhQzi6S1lhuQxMoim5WC
'' SIG '' qxNp7eeNDhrt+WcdIFyQRNM1mp2RAIwo7eHhgYvrmpGb
'' SIG '' JO9Mx00Bx8nz/gd5tgUkjRT4YLFSD6er6P/bejnjXsyM
'' SIG '' F+ROflcBBt+52YBHsUBdn0GWG8UNQGrqg70XV7EqStXY
'' SIG '' dVAbfRGjLM7rnGkeZzMEDerA4xvfRc3SvQLc25+EppbK
'' SIG '' C1LUQIf++vLCndGNYTJilR8CF/P+CblEVAUWdCVrtDWE
'' SIG '' AafJIZLtfEPAgEOdNLRQe1R96Q/M6AOJXAOyZMUxqDyq
'' SIG '' 7n5vpUWQAOIjIG3C2dj/8UnZyhcVPLy99UaDZWSYhi+T
'' SIG '' Kk1778gS8/jET+BJ/TcntTfMf5SQ9bLXTaOcCRvpoF7B
'' SIG '' P8384NhmlakHMxR4NDZfG6SKpzRVEXkEatwtY1WDAknH
'' SIG '' oDcx3mLcOTpmf+3lZ0Zo15QrC73bMTs3NWFZ+p2S5EA+
'' SIG '' ct9R2KwfYiB7rMIWjL9oSTTY1Z3ZKVsvd+DyGblkzJN+
'' SIG '' tJI1zxcJdlr9U85vbTqwqvPpeNPCiC+ifnn6BjZEfGAd
'' SIG '' zPrtbWHlQzv03Dmxh8WhhQekGcQFKZ3weTfzJgTcTDqs
'' SIG '' xtTfEIsFvILY4zCYhQX+omGKwo7UcBeyD3UCxbUCAwEA
'' SIG '' AaOCAUkwggFFMB0GA1UdDgQWBBTlBDF57TeePtdWTPkL
'' SIG '' u0Oa4NxXPTAfBgNVHSMEGDAWgBSfpxVdAF5iXYP05dJl
'' SIG '' pxtTNRnpcjBfBgNVHR8EWDBWMFSgUqBQhk5odHRwOi8v
'' SIG '' d3d3Lm1pY3Jvc29mdC5jb20vcGtpb3BzL2NybC9NaWNy
'' SIG '' b3NvZnQlMjBUaW1lLVN0YW1wJTIwUENBJTIwMjAxMCgx
'' SIG '' KS5jcmwwbAYIKwYBBQUHAQEEYDBeMFwGCCsGAQUFBzAC
'' SIG '' hlBodHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vcGtpb3Bz
'' SIG '' L2NlcnRzL01pY3Jvc29mdCUyMFRpbWUtU3RhbXAlMjBQ
'' SIG '' Q0ElMjAyMDEwKDEpLmNydDAMBgNVHRMBAf8EAjAAMBYG
'' SIG '' A1UdJQEB/wQMMAoGCCsGAQUFBwMIMA4GA1UdDwEB/wQE
'' SIG '' AwIHgDANBgkqhkiG9w0BAQsFAAOCAgEAYW7qIE2qUbLs
'' SIG '' EpGhTEOncrvGQmFKVkPIiK24uNLVNoSuqR4LtIY9M//c
'' SIG '' xUUceQ34bpI724wP3UuVWH8c9RCGz5bfPezCdrARjtmC
'' SIG '' 2MGHpXyaQ8GwdF0vWZK6qc6ul/5Zg0A59xub3GKWEAie
'' SIG '' eSy78hZSdfeQFaOFsvFF+ae4PVtzIDR0DKTPhFeuPnLM
'' SIG '' 4+B6OWkJnihrSsu8O9nkWP71g7qba7867hTZigiLddlH
'' SIG '' AOQTrF6dT7ZI8dskbAo++w0ppdM1WI6lvyElpKxo8nlS
'' SIG '' fpIc3LcWi5JJVcAsYoKJA+n5Fm8tIQhCkzkzzM4boDyA
'' SIG '' HMXB7EdrMdNWEWvaR9s73XbLgRH0hRugo9EErxGfzPZi
'' SIG '' foeJomkEkewBG1Rg28kSpGJ/NEvtwJkZYd2TnvgRaiee
'' SIG '' zl3XiA0h27x8ye6E6hvPepd3lIT7GYOvXzYMU8Zb0TZk
'' SIG '' RP/utWI+2dbgdF2ED+tK7DC3U5VWBMmVJeTC0y+S76ha
'' SIG '' M2ZUtl6I4uARD+nXVU85hyeKHTmTFk03kNCMJ1hvfL1r
'' SIG '' /66D3oAq9RutRVa3VyxNwFyH7eGTeGZA056AIT8Ow2TT
'' SIG '' 0ZUluE21Y/y8NF75y2DcDFAPaLmP8MfbXk2ifL05G4GM
'' SIG '' mjmChc+qzUV2eGn+ntyF8DAn3wmrKSlFwu9mDLuVvC/8
'' SIG '' 8k8bDVBIAW0wggdxMIIFWaADAgECAhMzAAAAFcXna54C
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
'' SIG '' cGbyoYIDTTCCAjUCAQEwgfmhgdGkgc4wgcsxCzAJBgNV
'' SIG '' BAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYD
'' SIG '' VQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQg
'' SIG '' Q29ycG9yYXRpb24xJTAjBgNVBAsTHE1pY3Jvc29mdCBB
'' SIG '' bWVyaWNhIE9wZXJhdGlvbnMxJzAlBgNVBAsTHm5TaGll
'' SIG '' bGQgVFNTIEVTTjozMzAzLTA1RTAtRDk0NzElMCMGA1UE
'' SIG '' AxMcTWljcm9zb2Z0IFRpbWUtU3RhbXAgU2VydmljZaIj
'' SIG '' CgEBMAcGBSsOAwIaAxUA4ljQXhSfY72hKzeuA9RvOni9
'' SIG '' JruggYMwgYCkfjB8MQswCQYDVQQGEwJVUzETMBEGA1UE
'' SIG '' CBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEe
'' SIG '' MBwGA1UEChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMSYw
'' SIG '' JAYDVQQDEx1NaWNyb3NvZnQgVGltZS1TdGFtcCBQQ0Eg
'' SIG '' MjAxMDANBgkqhkiG9w0BAQsFAAIFAOqDx8EwIhgPMjAy
'' SIG '' NDA5MDUwNjAxMzdaGA8yMDI0MDkwNjA2MDEzN1owdDA6
'' SIG '' BgorBgEEAYRZCgQBMSwwKjAKAgUA6oPHwQIBADAHAgEA
'' SIG '' AgId6zAHAgEAAgITWzAKAgUA6oUZQQIBADA2BgorBgEE
'' SIG '' AYRZCgQCMSgwJjAMBgorBgEEAYRZCgMCoAowCAIBAAID
'' SIG '' B6EgoQowCAIBAAIDAYagMA0GCSqGSIb3DQEBCwUAA4IB
'' SIG '' AQCZUyGNJZflo4FfPNrjzzKmzkmvVZALUd0A5D4VVL8u
'' SIG '' KgXdHW7ZT9j9GL4ejVepw2I+x8+oIbjtal1N7yfdICmu
'' SIG '' TROUMW2hX3GWsGJkvW7iOzPu02moRmtKLaBNTyp7KWKa
'' SIG '' Dii6XH65b0FMzELESRneSyvxh5w/GEOcmcBkRXxVGdHd
'' SIG '' nkQgta+dTIFHcpy7Wr0tki6CxncGYQT4dk5VU3Xi/Azg
'' SIG '' iUenuKh06D8LpOf0Egn4AZD/xsmgCuO8ak3CfCADYWzX
'' SIG '' tTCxAW4ykL4eHIFZWgMkBu1vS9nF7XEvzh58QZIUlRBT
'' SIG '' MWox6k+10Rt1xRrhC06Zvh8IGK9AYSqbnqc+MYIEDTCC
'' SIG '' BAkCAQEwgZMwfDELMAkGA1UEBhMCVVMxEzARBgNVBAgT
'' SIG '' Cldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAc
'' SIG '' BgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEmMCQG
'' SIG '' A1UEAxMdTWljcm9zb2Z0IFRpbWUtU3RhbXAgUENBIDIw
'' SIG '' MTACEzMAAAHm2UKe6gD4feEAAQAAAeYwDQYJYIZIAWUD
'' SIG '' BAIBBQCgggFKMBoGCSqGSIb3DQEJAzENBgsqhkiG9w0B
'' SIG '' CRABBDAvBgkqhkiG9w0BCQQxIgQgR4/1ddv3yFmg0YZD
'' SIG '' 7zM59MJcGHtJyU3AFc0Wpyd/B/QwgfoGCyqGSIb3DQEJ
'' SIG '' EAIvMYHqMIHnMIHkMIG9BCDPu6OGqB6zCWhvIJyztate
'' SIG '' oSGHEZ6MuhZzgm50g9LGWTCBmDCBgKR+MHwxCzAJBgNV
'' SIG '' BAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYD
'' SIG '' VQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQg
'' SIG '' Q29ycG9yYXRpb24xJjAkBgNVBAMTHU1pY3Jvc29mdCBU
'' SIG '' aW1lLVN0YW1wIFBDQSAyMDEwAhMzAAAB5tlCnuoA+H3h
'' SIG '' AAEAAAHmMCIEIPP8YMbEQQJ9J+bHbLF3mi+RSziTpSO/
'' SIG '' /7dOD+0ANSeFMA0GCSqGSIb3DQEBCwUABIICAJsBwyua
'' SIG '' bgtg59cL7Wui3KdtoLpfnEDIqCHaSnhRyk05RZ2ZGkDZ
'' SIG '' O3cpoBrABMTg8eQ/o4qr5tAGxAXpOIzalO39fFolLNB0
'' SIG '' QL3jaBL7yJyJetBA098yVHvnDySUgKoYZ2L5UFuc2cUG
'' SIG '' J9aycIpqDiWPBT+OwHDndZikCPfTNMJOJ5RhksEXX+na
'' SIG '' +6zd5NeAR1YKccftT9j0ruVg75Ye5FYhk2uoIwT18KV2
'' SIG '' PIeKG+qVum1qnzw6WoE00UnaMfJK41GpgDQ3L0HsnTu6
'' SIG '' zCUOrdkvUMkruhZ9hkebbhFDFjoRqN1mV/VzyIXBFG0W
'' SIG '' p0lzjwOSipam4vpjv+o2Iv/Kx5a0RnEzrLfoL4m19c4y
'' SIG '' 6H0NNIqDLI0z5QzeYo+QFr9xcMxO0r6UPDblKzN8c+ia
'' SIG '' Ir0fc1dFUZNGDwnw4nrP+S0BEhTZBo41p/QS4TcRAnFU
'' SIG '' ED0oXujEETbqosMGGpsbVgLVD6L0m8OFnoA2Q0mer7Er
'' SIG '' kFg1DR9b3LHvrbmcV7dPHLNAv2mujZakp40y97JxSgif
'' SIG '' ftICJqtOrSvCIlDrtmwFoivUaVST5PLNilnSoXhDTM4A
'' SIG '' qZDlWiOiuEF4nB9mGXJ/IIbYeNVM4UIeJdYuvf9ttB4h
'' SIG '' TEUOMM8HvhfKDEj1xAazGLd7ATynUIG7phcG7WfCd1V4
'' SIG '' 1MKgZaO5mysjC1d0
'' SIG '' End signature block
