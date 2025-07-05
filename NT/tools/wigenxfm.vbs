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
'' SIG '' MIImMQYJKoZIhvcNAQcCoIImIjCCJh4CAQExDzANBglg
'' SIG '' hkgBZQMEAgEFADB3BgorBgEEAYI3AgEEoGkwZzAyBgor
'' SIG '' BgEEAYI3AgEeMCQCAQEEEE7wKRaZJ7VNj+Ws4Q8X66sC
'' SIG '' AQACAQACAQACAQACAQAwMTANBglghkgBZQMEAgEFAAQg
'' SIG '' LEOvI1OL4Ms+i8VD+9Lv0JnljZ8+Z42whrFijf9Ua/Cg
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
'' SIG '' 9w0BCQQxIgQggN866NpS7HixjtjP92kpjmQFYaHpBAuj
'' SIG '' o7+KZVwNlocwWgYKKwYBBAGCNwIBDDFMMEqgJIAiAE0A
'' SIG '' aQBjAHIAbwBzAG8AZgB0ACAAVwBpAG4AZABvAHcAc6Ei
'' SIG '' gCBodHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vd2luZG93
'' SIG '' czANBgkqhkiG9w0BAQEFAASCAQB+1v6AVtae2cXy9HEw
'' SIG '' KaamPYK/qlkm5Z7czf2TM2cGVJothA+9HJimQsNQtx/6
'' SIG '' gDKgvZpDzoAZdf+wh4Rs9Fd06NBC0ZhQyi1JF1bERUvg
'' SIG '' t5EIhoTmvNxZqxfuh+tP1QroZskspvwNejE2bipwod8a
'' SIG '' niCQcaZ/fd1f733F3Q7gPQtLidSyN5N6Q+R+o8MPyyyh
'' SIG '' skjnEqBJJ1zUCHYNEmmiBHnmQAnW4LnaMsAd4QTCeyxB
'' SIG '' 7o8TIKGkAlQoApFjDAMI7HbAE4j7Ul35eTWmkudKMpXE
'' SIG '' nQyoBzBXS22v7/pY8ZADG5TOMvRiRSOsn+Vyb+vZ7GUC
'' SIG '' 7NNukvUtGVXI6xLRoYIXlDCCF5AGCisGAQQBgjcDAwEx
'' SIG '' gheAMIIXfAYJKoZIhvcNAQcCoIIXbTCCF2kCAQMxDzAN
'' SIG '' BglghkgBZQMEAgEFADCCAVIGCyqGSIb3DQEJEAEEoIIB
'' SIG '' QQSCAT0wggE5AgEBBgorBgEEAYRZCgMBMDEwDQYJYIZI
'' SIG '' AWUDBAIBBQAEILH29Z9GhqvufG8Rpz29Uef2nDxDKbQl
'' SIG '' EGBKE9GQOl86AgZmvkRiaJIYEzIwMjQwOTA1MDkxNzEy
'' SIG '' LjM4NlowBIACAfSggdGkgc4wgcsxCzAJBgNVBAYTAlVT
'' SIG '' MRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdS
'' SIG '' ZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9y
'' SIG '' YXRpb24xJTAjBgNVBAsTHE1pY3Jvc29mdCBBbWVyaWNh
'' SIG '' IE9wZXJhdGlvbnMxJzAlBgNVBAsTHm5TaGllbGQgVFNT
'' SIG '' IEVTTjozMzAzLTA1RTAtRDk0NzElMCMGA1UEAxMcTWlj
'' SIG '' cm9zb2Z0IFRpbWUtU3RhbXAgU2VydmljZaCCEeowggcg
'' SIG '' MIIFCKADAgECAhMzAAAB5tlCnuoA+H3hAAEAAAHmMA0G
'' SIG '' CSqGSIb3DQEBCwUAMHwxCzAJBgNVBAYTAlVTMRMwEQYD
'' SIG '' VQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25k
'' SIG '' MR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24x
'' SIG '' JjAkBgNVBAMTHU1pY3Jvc29mdCBUaW1lLVN0YW1wIFBD
'' SIG '' QSAyMDEwMB4XDTIzMTIwNjE4NDUxNVoXDTI1MDMwNTE4
'' SIG '' NDUxNVowgcsxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpX
'' SIG '' YXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYD
'' SIG '' VQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xJTAjBgNV
'' SIG '' BAsTHE1pY3Jvc29mdCBBbWVyaWNhIE9wZXJhdGlvbnMx
'' SIG '' JzAlBgNVBAsTHm5TaGllbGQgVFNTIEVTTjozMzAzLTA1
'' SIG '' RTAtRDk0NzElMCMGA1UEAxMcTWljcm9zb2Z0IFRpbWUt
'' SIG '' U3RhbXAgU2VydmljZTCCAiIwDQYJKoZIhvcNAQEBBQAD
'' SIG '' ggIPADCCAgoCggIBAL2+mHzi2CW4TOb/Ck0qUCNwSUbN
'' SIG '' +W8oANnUP7Z3+J5hgS0XYcoysoYUM4uktZYbkMTKIpuV
'' SIG '' gqsTae3njQ4a7flnHSBckETTNZqdkQCMKO3h4YGL65qR
'' SIG '' myTvTMdNAcfJ8/4HebYFJI0U+GCxUg+nq+j/23o5417M
'' SIG '' jBfkTn5XAQbfudmAR7FAXZ9BlhvFDUBq6oO9F1exKkrV
'' SIG '' 2HVQG30RoyzO65xpHmczBA3qwOMb30XN0r0C3NufhKaW
'' SIG '' ygtS1ECH/vrywp3RjWEyYpUfAhfz/gm5RFQFFnQla7Q1
'' SIG '' hAGnySGS7XxDwIBDnTS0UHtUfekPzOgDiVwDsmTFMag8
'' SIG '' qu5+b6VFkADiIyBtwtnY//FJ2coXFTy8vfVGg2VkmIYv
'' SIG '' kypNe+/IEvP4xE/gSf03J7U3zH+UkPWy102jnAkb6aBe
'' SIG '' wT/N/ODYZpWpBzMUeDQ2Xxukiqc0VRF5BGrcLWNVgwJJ
'' SIG '' x6A3Md5i3Dk6Zn/t5WdGaNeUKwu92zE7NzVhWfqdkuRA
'' SIG '' PnLfUdisH2Ige6zCFoy/aEk02NWd2SlbL3fg8hm5ZMyT
'' SIG '' frSSNc8XCXZa/VPOb206sKrz6XjTwogvon55+gY2RHxg
'' SIG '' Hcz67W1h5UM79Nw5sYfFoYUHpBnEBSmd8Hk38yYE3Ew6
'' SIG '' rMbU3xCLBbyC2OMwmIUF/qJhisKO1HAXsg91AsW1AgMB
'' SIG '' AAGjggFJMIIBRTAdBgNVHQ4EFgQU5QQxee03nj7XVkz5
'' SIG '' C7tDmuDcVz0wHwYDVR0jBBgwFoAUn6cVXQBeYl2D9OXS
'' SIG '' ZacbUzUZ6XIwXwYDVR0fBFgwVjBUoFKgUIZOaHR0cDov
'' SIG '' L3d3dy5taWNyb3NvZnQuY29tL3BraW9wcy9jcmwvTWlj
'' SIG '' cm9zb2Z0JTIwVGltZS1TdGFtcCUyMFBDQSUyMDIwMTAo
'' SIG '' MSkuY3JsMGwGCCsGAQUFBwEBBGAwXjBcBggrBgEFBQcw
'' SIG '' AoZQaHR0cDovL3d3dy5taWNyb3NvZnQuY29tL3BraW9w
'' SIG '' cy9jZXJ0cy9NaWNyb3NvZnQlMjBUaW1lLVN0YW1wJTIw
'' SIG '' UENBJTIwMjAxMCgxKS5jcnQwDAYDVR0TAQH/BAIwADAW
'' SIG '' BgNVHSUBAf8EDDAKBggrBgEFBQcDCDAOBgNVHQ8BAf8E
'' SIG '' BAMCB4AwDQYJKoZIhvcNAQELBQADggIBAGFu6iBNqlGy
'' SIG '' 7BKRoUxDp3K7xkJhSlZDyIituLjS1TaErqkeC7SGPTP/
'' SIG '' 3MVFHHkN+G6SO9uMD91LlVh/HPUQhs+W3z3swnawEY7Z
'' SIG '' gtjBh6V8mkPBsHRdL1mSuqnOrpf+WYNAOfcbm9xilhAI
'' SIG '' nnksu/IWUnX3kBWjhbLxRfmnuD1bcyA0dAykz4RXrj5y
'' SIG '' zOPgejlpCZ4oa0rLvDvZ5Fj+9YO6m2u/Ou4U2YoIi3XZ
'' SIG '' RwDkE6xenU+2SPHbJGwKPvsNKaXTNViOpb8hJaSsaPJ5
'' SIG '' Un6SHNy3FouSSVXALGKCiQPp+RZvLSEIQpM5M8zOG6A8
'' SIG '' gBzFwexHazHTVhFr2kfbO912y4ER9IUboKPRBK8Rn8z2
'' SIG '' Yn6HiaJpBJHsARtUYNvJEqRifzRL7cCZGWHdk574EWon
'' SIG '' ns5d14gNIdu8fMnuhOobz3qXd5SE+xmDr182DFPGW9E2
'' SIG '' ZET/7rViPtnW4HRdhA/rSuwwt1OVVgTJlSXkwtMvku+o
'' SIG '' WjNmVLZeiOLgEQ/p11VPOYcnih05kxZNN5DQjCdYb3y9
'' SIG '' a/+ug96AKvUbrUVWt1csTcBch+3hk3hmQNOegCE/DsNk
'' SIG '' 09GVJbhNtWP8vDRe+ctg3AxQD2i5j/DH215Nony9ORuB
'' SIG '' jJo5goXPqs1Fdnhp/p7chfAwJ98JqykpRcLvZgy7lbwv
'' SIG '' /PJPGw1QSAFtMIIHcTCCBVmgAwIBAgITMwAAABXF52ue
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
'' SIG '' ZWxkIFRTUyBFU046MzMwMy0wNUUwLUQ5NDcxJTAjBgNV
'' SIG '' BAMTHE1pY3Jvc29mdCBUaW1lLVN0YW1wIFNlcnZpY2Wi
'' SIG '' IwoBATAHBgUrDgMCGgMVAOJY0F4Un2O9oSs3rgPUbzp4
'' SIG '' vSa7oIGDMIGApH4wfDELMAkGA1UEBhMCVVMxEzARBgNV
'' SIG '' BAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQx
'' SIG '' HjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEm
'' SIG '' MCQGA1UEAxMdTWljcm9zb2Z0IFRpbWUtU3RhbXAgUENB
'' SIG '' IDIwMTAwDQYJKoZIhvcNAQELBQACBQDqg8fBMCIYDzIw
'' SIG '' MjQwOTA1MDYwMTM3WhgPMjAyNDA5MDYwNjAxMzdaMHQw
'' SIG '' OgYKKwYBBAGEWQoEATEsMCowCgIFAOqDx8ECAQAwBwIB
'' SIG '' AAICHeswBwIBAAICE1swCgIFAOqFGUECAQAwNgYKKwYB
'' SIG '' BAGEWQoEAjEoMCYwDAYKKwYBBAGEWQoDAqAKMAgCAQAC
'' SIG '' AwehIKEKMAgCAQACAwGGoDANBgkqhkiG9w0BAQsFAAOC
'' SIG '' AQEAmVMhjSWX5aOBXzza488yps5Jr1WQC1HdAOQ+FVS/
'' SIG '' LioF3R1u2U/Y/Ri+Ho1XqcNiPsfPqCG47WpdTe8n3SAp
'' SIG '' rk0TlDFtoV9xlrBiZL1u4jsz7tNpqEZrSi2gTU8qeyli
'' SIG '' mg4oulx+uW9BTMxCxEkZ3ksr8YecPxhDnJnAZEV8VRnR
'' SIG '' 3Z5EILWvnUyBR3Kcu1q9LZIugsZ3BmEE+HZOVVN14vwM
'' SIG '' 4IlHp7iodOg/C6Tn9BIJ+AGQ/8bJoArjvGpNwnwgA2Fs
'' SIG '' 17UwsQFuMpC+HhyBWVoDJAbtb0vZxe1xL84efEGSFJUQ
'' SIG '' UzFqMepPtdEbdcUa4QtOmb4fCBivQGEqm56nPjGCBA0w
'' SIG '' ggQJAgEBMIGTMHwxCzAJBgNVBAYTAlVTMRMwEQYDVQQI
'' SIG '' EwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4w
'' SIG '' HAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xJjAk
'' SIG '' BgNVBAMTHU1pY3Jvc29mdCBUaW1lLVN0YW1wIFBDQSAy
'' SIG '' MDEwAhMzAAAB5tlCnuoA+H3hAAEAAAHmMA0GCWCGSAFl
'' SIG '' AwQCAQUAoIIBSjAaBgkqhkiG9w0BCQMxDQYLKoZIhvcN
'' SIG '' AQkQAQQwLwYJKoZIhvcNAQkEMSIEIAfq0Y/ujFvAWcYC
'' SIG '' 7wKk6zynapkakzuIKCzjdyibdxNnMIH6BgsqhkiG9w0B
'' SIG '' CRACLzGB6jCB5zCB5DCBvQQgz7ujhqgeswlobyCcs7Wr
'' SIG '' XqEhhxGejLoWc4JudIPSxlkwgZgwgYCkfjB8MQswCQYD
'' SIG '' VQQGEwJVUzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4G
'' SIG '' A1UEBxMHUmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0
'' SIG '' IENvcnBvcmF0aW9uMSYwJAYDVQQDEx1NaWNyb3NvZnQg
'' SIG '' VGltZS1TdGFtcCBQQ0EgMjAxMAITMwAAAebZQp7qAPh9
'' SIG '' 4QABAAAB5jAiBCDz/GDGxEECfSfmx2yxd5ovkUs4k6Uj
'' SIG '' v/+3Tg/tADUnhTANBgkqhkiG9w0BAQsFAASCAgC3m0kY
'' SIG '' YlVzPRaT2Jj+w37rP4pZ0Sf8J4Uy9+Wwq6PXiurfwg4C
'' SIG '' 4zUCzE9ThuJWhm7OMGed75+q174xKv3OHek3gCJeo+gy
'' SIG '' N0wRTrIhoxA2jomBgYMebzxP1oby8dv7rwdaiwO6yjQf
'' SIG '' HKa5EtUD7+vyfMPC+NaMjcCbK7LUK526b5mDBhCswstZ
'' SIG '' XG2dIgwOQaTdiGSOETZ+ERvLdzCRL7dVCpY301rPEcRI
'' SIG '' SA3A4IocPWlSYIvzevlS9c6i7SxiXHBsabwuKC+GuuW2
'' SIG '' Qxlm6IYYo+7Xie4PFcq3JPj0eMwu1JQk34+HDDNQ62L4
'' SIG '' UEWi4Nr+LVYGSfdojCUaZzSEBYEgQ5Ldb6OByyqmxQdx
'' SIG '' ooB0Ui9BvpfYRaVPxRFMS3Y1AivCG9kV0L/hKsm1D8Sn
'' SIG '' DTbJQnrpjWo3CeXnaScHttwIr4Gi/QVEGJzviQlmQjCe
'' SIG '' upug68Gq8461ySo4ZCJMctvtU+CrBZ6C8zCyAJHs6aeW
'' SIG '' SKfyfo89qdAKviXMjKoKTaENNBSJjdhCiGGeehkesGp1
'' SIG '' QeQKZX65EJBxdTwr9OgTdmTwf2QJcoiZkCaNYS1a0BJU
'' SIG '' U4U4x//wciZPa3axk8dalqC4Aor6UOrzA25MnLK+hjc9
'' SIG '' kWNHuInpxWS08pMR5GZJdWXzTQhXuUfxarZlCNo3iBD0
'' SIG '' ieJB91fXPfbPqty4qw==
'' SIG '' End signature block
