' Windows Installer utility to applay a transform to an installer database
' For use with Windows Scripting Host, CScript.exe or WScript.exe
' Copyright (c) Microsoft Corporation. All rights reserved.
' Demonstrates use of Database.ApplyTransform and MsiDatabaseApplyTransform
'
Option Explicit

' Error conditions that may be suppressed when applying transforms
Const msiTransformErrorAddExistingRow         = 1 'Adding a row that already exists. 
Const msiTransformErrorDeleteNonExistingRow   = 2 'Deleting a row that doesn't exist. 
Const msiTransformErrorAddExistingTable       = 4 'Adding a table that already exists. 
Const msiTransformErrorDeleteNonExistingTable = 8 'Deleting a table that doesn't exist. 
Const msiTransformErrorUpdateNonExistingRow  = 16 'Updating a row that doesn't exist. 
Const msiTransformErrorChangeCodePage       = 256 'Transform and database code pages do not match 

Const msiOpenDatabaseModeReadOnly     = 0
Const msiOpenDatabaseModeTransact     = 1
Const msiOpenDatabaseModeCreate       = 3

If (Wscript.Arguments.Count < 2) Then
	Wscript.Echo "Windows Installer database tranform application utility" &_
		vbNewLine & " 1st argument is the path to an installer database" &_
		vbNewLine & " 2nd argument is the path to the transform file to apply" &_
		vbNewLine & " 3rd argument is optional set of error conditions to suppress:" &_
		vbNewLine & "     1 = adding a row that already exists" &_
		vbNewLine & "     2 = deleting a row that doesn't exist" &_
		vbNewLine & "     4 = adding a table that already exists" &_
		vbNewLine & "     8 = deleting a table that doesn't exist" &_
		vbNewLine & "    16 = updating a row that doesn't exist" &_
		vbNewLine & "   256 = mismatch of database and transform codepages" &_
		vbNewLine &_
		vbNewLine & "Copyright (C) Microsoft Corporation.  All rights reserved."
	Wscript.Quit 1
End If

' Connect to Windows Installer object
On Error Resume Next
Dim installer : Set installer = Nothing
Set installer = Wscript.CreateObject("WindowsInstaller.Installer") : CheckError

' Open database and apply transform
Dim database : Set database = installer.OpenDatabase(Wscript.Arguments(0), msiOpenDatabaseModeTransact) : CheckError
Dim errorConditions:errorConditions = 0
If Wscript.Arguments.Count >= 3 Then errorConditions = CLng(Wscript.Arguments(2))
Database.ApplyTransform Wscript.Arguments(1), errorConditions : CheckError
Database.Commit : CheckError

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
'' SIG '' MIImMQYJKoZIhvcNAQcCoIImIjCCJh4CAQExDzANBglg
'' SIG '' hkgBZQMEAgEFADB3BgorBgEEAYI3AgEEoGkwZzAyBgor
'' SIG '' BgEEAYI3AgEeMCQCAQEEEE7wKRaZJ7VNj+Ws4Q8X66sC
'' SIG '' AQACAQACAQACAQACAQAwMTANBglghkgBZQMEAgEFAAQg
'' SIG '' ocXRzPIBsTOs40BugTYvo1tESbFrFB3U6AbYVQhStNmg
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
'' SIG '' 9w0BCQQxIgQgLQn6o9kUODATKIS+smjNPFWNt2x3Bw5g
'' SIG '' zkZaBTK4sbgwWgYKKwYBBAGCNwIBDDFMMEqgJIAiAE0A
'' SIG '' aQBjAHIAbwBzAG8AZgB0ACAAVwBpAG4AZABvAHcAc6Ei
'' SIG '' gCBodHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vd2luZG93
'' SIG '' czANBgkqhkiG9w0BAQEFAASCAQAyJv8drk+ZZECKo8Ih
'' SIG '' ehRMOiV4Wwpayq0mpVnBVW+YfmLNCw9Y1kCPNiBWcuBs
'' SIG '' 9+nqh8JEY9Smhiq45yhE9dLBPUtF3RO2MNKkCzlaQCb/
'' SIG '' T3YQhhpPA35gqRzcCdlcqKfxr3p/TlnbrTasZP1IO48/
'' SIG '' 5GXV5tHCJRMkb9qYc/DskZa64QtgmNY8lRvCIWoOYAV+
'' SIG '' XYdaznNvezwLtvVfboGY36y5OwFA1BFTKHyMI9vUaDOo
'' SIG '' ZdYrqkfUXmeFzfEZoeUreXki/82kbdPcF4kOW4+PaCmZ
'' SIG '' FjkO6nT1W1A6QyR9vykqwRq0hOiHSq6bXhF/uxMzmCt3
'' SIG '' BWtjVR75YBdZKs5YoYIXlDCCF5AGCisGAQQBgjcDAwEx
'' SIG '' gheAMIIXfAYJKoZIhvcNAQcCoIIXbTCCF2kCAQMxDzAN
'' SIG '' BglghkgBZQMEAgEFADCCAVIGCyqGSIb3DQEJEAEEoIIB
'' SIG '' QQSCAT0wggE5AgEBBgorBgEEAYRZCgMBMDEwDQYJYIZI
'' SIG '' AWUDBAIBBQAEIMQRUOjZCD8EHo6JpUq2onXimQaNB0AV
'' SIG '' a9WRlP7vlB6HAgZmveopet4YEzIwMjQwOTA1MDkxNzA0
'' SIG '' LjM2OFowBIACAfSggdGkgc4wgcsxCzAJBgNVBAYTAlVT
'' SIG '' MRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdS
'' SIG '' ZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9y
'' SIG '' YXRpb24xJTAjBgNVBAsTHE1pY3Jvc29mdCBBbWVyaWNh
'' SIG '' IE9wZXJhdGlvbnMxJzAlBgNVBAsTHm5TaGllbGQgVFNT
'' SIG '' IEVTTjpBOTM1LTAzRTAtRDk0NzElMCMGA1UEAxMcTWlj
'' SIG '' cm9zb2Z0IFRpbWUtU3RhbXAgU2VydmljZaCCEeowggcg
'' SIG '' MIIFCKADAgECAhMzAAAB6Q9xMH5d8RI2AAEAAAHpMA0G
'' SIG '' CSqGSIb3DQEBCwUAMHwxCzAJBgNVBAYTAlVTMRMwEQYD
'' SIG '' VQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25k
'' SIG '' MR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24x
'' SIG '' JjAkBgNVBAMTHU1pY3Jvc29mdCBUaW1lLVN0YW1wIFBD
'' SIG '' QSAyMDEwMB4XDTIzMTIwNjE4NDUyNloXDTI1MDMwNTE4
'' SIG '' NDUyNlowgcsxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpX
'' SIG '' YXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYD
'' SIG '' VQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xJTAjBgNV
'' SIG '' BAsTHE1pY3Jvc29mdCBBbWVyaWNhIE9wZXJhdGlvbnMx
'' SIG '' JzAlBgNVBAsTHm5TaGllbGQgVFNTIEVTTjpBOTM1LTAz
'' SIG '' RTAtRDk0NzElMCMGA1UEAxMcTWljcm9zb2Z0IFRpbWUt
'' SIG '' U3RhbXAgU2VydmljZTCCAiIwDQYJKoZIhvcNAQEBBQAD
'' SIG '' ggIPADCCAgoCggIBAKyajDFBFWCnhNJzedNrrKsA8mdX
'' SIG '' oDtplidPD/LH3S7UNIfz2e99A3Nv7l+YErymkfvpOYnO
'' SIG '' MdRwiZ3zjkD+m9ljk7w8IG7sar7Hld7qmVC3jHBVRRxA
'' SIG '' hPGSU5nVGb18nmeHyCfE7Fp7MUwzjWwMjssykrAgpAzB
'' SIG '' cNy1gq8LJDLqQ7axUsHraQXz3ZnBximIhXHctPUs90y3
'' SIG '' Uh5LfkpjkzHKVF1NLsTUmhyXfQ2BwGIl+qcxx7Tl4SKk
'' SIG '' ixM7gMif/9O0/VHHntVd+8I7w1IKH13GzK+eDSVRVj66
'' SIG '' ur8bxBEWg6X/ug4jRF/xCD7eHJhrIewj3C28McadPfQ2
'' SIG '' vjXHNOnDYjplZoiE/Ay7kO92QQbNXu9hPe1v21O+Jjem
'' SIG '' y6XVPkP3fz8B80upqdUIm0/jLPRUkFIZX6HrplxpQk7G
'' SIG '' ltIiMiZo4sXXw06OZ/WfANq2wGi5dZcUrsTlLRUtHKhO
'' SIG '' oMLEcbiZbeak1Cikz9TVYmeOyxZCW4rx5v4wMqWT0T+E
'' SIG '' 4FgqzYp95Dgcbt05wr7Aw5qYZ/C+Qh7t2TKXObwF4BRA
'' SIG '' LwvGsBDKSFIfL4VpD3cMCV9BijBgO3MZeoTrA4BN4oUj
'' SIG '' fS71iXENPMC4sMrTvdyd0xXipoPd65cDrFQ0KjODuuKG
'' SIG '' IdRozjcCZv0Qa5GXTbb7I/ByWbKSyyTfRrhGne/1AgMB
'' SIG '' AAGjggFJMIIBRTAdBgNVHQ4EFgQUkX4zicUIdiO4iPRa
'' SIG '' 6/6NyO0H7E4wHwYDVR0jBBgwFoAUn6cVXQBeYl2D9OXS
'' SIG '' ZacbUzUZ6XIwXwYDVR0fBFgwVjBUoFKgUIZOaHR0cDov
'' SIG '' L3d3dy5taWNyb3NvZnQuY29tL3BraW9wcy9jcmwvTWlj
'' SIG '' cm9zb2Z0JTIwVGltZS1TdGFtcCUyMFBDQSUyMDIwMTAo
'' SIG '' MSkuY3JsMGwGCCsGAQUFBwEBBGAwXjBcBggrBgEFBQcw
'' SIG '' AoZQaHR0cDovL3d3dy5taWNyb3NvZnQuY29tL3BraW9w
'' SIG '' cy9jZXJ0cy9NaWNyb3NvZnQlMjBUaW1lLVN0YW1wJTIw
'' SIG '' UENBJTIwMjAxMCgxKS5jcnQwDAYDVR0TAQH/BAIwADAW
'' SIG '' BgNVHSUBAf8EDDAKBggrBgEFBQcDCDAOBgNVHQ8BAf8E
'' SIG '' BAMCB4AwDQYJKoZIhvcNAQELBQADggIBAFaxKn6uazEU
'' SIG '' t7rUAT3Qp6fZc+BAckOJLhJsuG/N9WMM8OY51ETvm5Ci
'' SIG '' FiEUx0bAcptWYsrSUdXUCnP8dyJmijJ6gC+QdBoeYuHA
'' SIG '' EaSjIABXFxppScc0hRL0u94vTQ/CZxIMuA3RX8XKTbRC
'' SIG '' kcMS6TApHyR9oERfzcDK9DOV/9ugM2hYoSCl0CwvxLML
'' SIG '' NcUucOjPMIkarRHPBCB4QGvwTgrbBDZZcj9knFlL/53c
'' SIG '' V3AbgSsEXPNSJJtXabfGww/dyoJEUO0nULf8meNcwKGe
'' SIG '' b1ssMPXBontM+nnBh2/Q6X35o3S3UGY7MKPwOaoq5TDO
'' SIG '' AIr1OO3DkpSNo7pCN6AfOd1f+1mtjv3Z19EBevl0asqS
'' SIG '' mywgerqutY7g+Uvc5L7hyIv+Xymb6g0ldYZdgkvkfos2
'' SIG '' crJclUTD/UVs7j4bP5Th8UXGzZLxTC+sFthxxVD074WW
'' SIG '' PvFMB4hMmwem0C9ESoJz79jHOEgqQDzxDxCEkpQO1rNq
'' SIG '' 0kftk52LQsIrCCpA7gfzUpkYNIuS0W81GGHxkEB6efWl
'' SIG '' b7lQEZjPYamBzFVcpPUK5Rh2UdH0Po2tWEap2EZODs6D
'' SIG '' 93/ygyU8bdiO6oXGJ2IiygDDb4yEjXNesiLnq3omQnvk
'' SIG '' nr0X6WSH2bIkmk2THjWxIHVcraMlaCrtWUG4/UG5eNne
'' SIG '' qDKb2vXC/Qy1MIIHcTCCBVmgAwIBAgITMwAAABXF52ue
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
'' SIG '' ZWxkIFRTUyBFU046QTkzNS0wM0UwLUQ5NDcxJTAjBgNV
'' SIG '' BAMTHE1pY3Jvc29mdCBUaW1lLVN0YW1wIFNlcnZpY2Wi
'' SIG '' IwoBATAHBgUrDgMCGgMVAKtph/XEOTasydT9UmjYYYrW
'' SIG '' fGjxoIGDMIGApH4wfDELMAkGA1UEBhMCVVMxEzARBgNV
'' SIG '' BAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQx
'' SIG '' HjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEm
'' SIG '' MCQGA1UEAxMdTWljcm9zb2Z0IFRpbWUtU3RhbXAgUENB
'' SIG '' IDIwMTAwDQYJKoZIhvcNAQELBQACBQDqg21+MCIYDzIw
'' SIG '' MjQwOTA0MjMzNjMwWhgPMjAyNDA5MDUyMzM2MzBaMHQw
'' SIG '' OgYKKwYBBAGEWQoEATEsMCowCgIFAOqDbX4CAQAwBwIB
'' SIG '' AAICBM8wBwIBAAICFWgwCgIFAOqEvv4CAQAwNgYKKwYB
'' SIG '' BAGEWQoEAjEoMCYwDAYKKwYBBAGEWQoDAqAKMAgCAQAC
'' SIG '' AwehIKEKMAgCAQACAwGGoDANBgkqhkiG9w0BAQsFAAOC
'' SIG '' AQEAcsnJWBXzlNv2qi99cYTYvo9ciA0mbc31BqRuTo8u
'' SIG '' t6aCNE/fw/q855KcwHHpKvRwTv41BeVKBe2EUUZTXz0b
'' SIG '' +PD/nhqPc8uH2wlx+aDfP9HfNWPfqzpb78Q699tK/a0W
'' SIG '' YORYMlW9AoWHueqBp5N9WAWd+We/9JOj1kcsYyHxBsYm
'' SIG '' gUF16PujEtofhA9xawL03pHPnF3UB5BIuvSRRWLgF5GE
'' SIG '' ezrL6f/Y7FTN00Pf/uMF8mbAiqLovivJzBhMH+hmh4aJ
'' SIG '' APk+RSMYAYYdJyZaE2REed9/3qda5y7dqWksYj53ubIp
'' SIG '' qL0sV37PHYt6L0EF6l60sA3HZl2mjkYFJCepMjGCBA0w
'' SIG '' ggQJAgEBMIGTMHwxCzAJBgNVBAYTAlVTMRMwEQYDVQQI
'' SIG '' EwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4w
'' SIG '' HAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xJjAk
'' SIG '' BgNVBAMTHU1pY3Jvc29mdCBUaW1lLVN0YW1wIFBDQSAy
'' SIG '' MDEwAhMzAAAB6Q9xMH5d8RI2AAEAAAHpMA0GCWCGSAFl
'' SIG '' AwQCAQUAoIIBSjAaBgkqhkiG9w0BCQMxDQYLKoZIhvcN
'' SIG '' AQkQAQQwLwYJKoZIhvcNAQkEMSIEIFeXWdYRKCIuiz3G
'' SIG '' VlS8xYv/jmaAIBtOcaiYBw3HibS2MIH6BgsqhkiG9w0B
'' SIG '' CRACLzGB6jCB5zCB5DCBvQQgpJCSeJdpNyaPVMpBYX7H
'' SIG '' ZTiuJWisYPxPCaBVs32qxCUwgZgwgYCkfjB8MQswCQYD
'' SIG '' VQQGEwJVUzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4G
'' SIG '' A1UEBxMHUmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0
'' SIG '' IENvcnBvcmF0aW9uMSYwJAYDVQQDEx1NaWNyb3NvZnQg
'' SIG '' VGltZS1TdGFtcCBQQ0EgMjAxMAITMwAAAekPcTB+XfES
'' SIG '' NgABAAAB6TAiBCA19TUtoKHsuUtoWHALvD7sVmKww8in
'' SIG '' yO5pfAZKRxUrHzANBgkqhkiG9w0BAQsFAASCAgA/ThVI
'' SIG '' JnwUTC6aAuduezJvWI55RttNumo4YtIfzt6JGfA+IN0O
'' SIG '' iMRnGvKbd6nycFZa4sD7qJbzbdbj46ehPlglrlczYjYN
'' SIG '' crNKS5uk+nw6FiRA6Svrac37FwrwBs7aFUpNisoV+mi1
'' SIG '' QLfIZfoTdk9mGSD6Ef8yglVz0aSJSraLopwabC20sTP+
'' SIG '' bsZyziSKnYcXkLADKZE/sqFUV2+w0QO5le8BaCZSQtVl
'' SIG '' V46j+vk65mg69VW/XKnHxhWNGDx6A4JEj+rrrrtu57DL
'' SIG '' LBg1yyDrKWUmmQmkXaAb9Sq4Fkz5H74aKme8QBgkHa3a
'' SIG '' 1/0ehf8SYs3+Gy9e0V311q2QjpNr1z6V/sypLdOH3Z7G
'' SIG '' u1kQIwetpz4DktYI8RhMGRqLopFSKzSdgVNI7tn6hhhJ
'' SIG '' ozakji0Mx4i7GIfQz2mgcOP9q5FOdlzzeHargLJpuzM7
'' SIG '' Gx3yjcA10NIBA+XXhrBaBPhMzph65NlG4UX7tGr+DzaE
'' SIG '' 0s1Icmzt4WSAHv2BPVYrnyfW3NnAe3l178nCdiNJUSzB
'' SIG '' M78b64ePzvdJkiBEJmLAQxlDbuhz0XIVxjxyaz10ueRa
'' SIG '' BHEvrjD31pe7CQ8foEz5V8PHlq/gckmZLQCpTQsg7kub
'' SIG '' +Fs+tLykMFOgqJsWsQWxyy8pVfNjBtBhzlEwvmIqkf63
'' SIG '' 9zs49VMfRlJyvESH7g==
'' SIG '' End signature block
