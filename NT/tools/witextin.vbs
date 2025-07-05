' Windows Installer utility to copy a file into a database text field
' For use with Windows Scripting Host, CScript.exe or WScript.exe
' Copyright (c) Microsoft Corporation. All rights reserved.
' Demonstrates processing of primary key data
'
Option Explicit

Const msiOpenDatabaseModeReadOnly     = 0
Const msiOpenDatabaseModeTransact     = 1

Const msiViewModifyUpdate  = 2
Const msiReadStreamAnsi    = 2

Dim argCount:argCount = Wscript.Arguments.Count
If argCount > 0 Then If InStr(1, Wscript.Arguments(0), "?", vbTextCompare) > 0 Then argCount = 0
If (argCount < 4) Then
	Wscript.Echo "Windows Installer utility to copy a file into a database text field." &_
		vbNewLine & "The 1st argument is the path to the installation database" &_
		vbNewLine & "The 2nd argument is the database table name" &_
		vbNewLine & "The 3rd argument is the set of primary key values, concatenated with colons" &_
		vbNewLine & "The 4th argument is non-key column name to receive the text data" &_
		vbNewLine & "The 5th argument is the path to the text file to copy" &_
		vbNewLine & "If the 5th argument is omitted, the existing data will be listed" &_
		vbNewLine & "All primary keys values must be specified in order, separated by colons" &_
		vbNewLine &_
		vbNewLine & "Copyright (C) Microsoft Corporation.  All rights reserved."
	Wscript.Quit 1
End If

' Connect to Windows Installer object
On Error Resume Next
Dim installer : Set installer = Nothing
Set installer = Wscript.CreateObject("WindowsInstaller.Installer") : CheckError


' Process input arguments and open database
Dim databasePath: databasePath = Wscript.Arguments(0)
Dim tableName   : tableName    = Wscript.Arguments(1)
Dim rowKeyValues: rowKeyValues = Split(Wscript.Arguments(2),":",-1,vbTextCompare)
Dim dataColumn  : dataColumn   = Wscript.Arguments(3)
Dim openMode : If argCount >= 5 Then openMode = msiOpenDatabaseModeTransact Else openMode = msiOpenDatabaseModeReadOnly
Dim database : Set database = installer.OpenDatabase(databasePath, openMode) : CheckError
Dim keyRecord : Set keyRecord = database.PrimaryKeys(tableName) : CheckError
Dim keyCount : keyCount = keyRecord.FieldCount
If UBound(rowKeyValues) + 1 <> keyCount Then Fail "Incorrect number of primary key values"

' Generate and execute query
Dim predicate, keyIndex
For keyIndex = 1 To keyCount
	If Not IsEmpty(predicate) Then predicate = predicate & " AND "
	predicate = predicate & "`" & keyRecord.StringData(keyIndex) & "`='" & rowKeyValues(keyIndex-1) & "'"
Next
Dim query : query = "SELECT `" & dataColumn & "` FROM `" & tableName & "` WHERE " & predicate
REM Wscript.Echo query 
Dim view : Set view = database.OpenView(query) : CheckError
view.Execute : CheckError
Dim resultRecord : Set resultRecord = view.Fetch : CheckError
If resultRecord Is Nothing Then Fail "Requested table row not present"

' Update value if supplied. Cannot store stream object in string column, must convert stream to string
If openMode = msiOpenDatabaseModeTransact Then
	resultRecord.SetStream 1, Wscript.Arguments(4) : CheckError
	Dim sizeStream : sizeStream = resultRecord.DataSize(1)
	resultRecord.StringData(1) = resultRecord.ReadStream(1, sizeStream, msiReadStreamAnsi) : CheckError
	view.Modify msiViewModifyUpdate, resultRecord : CheckError
	database.Commit : CheckError
Else
	Wscript.Echo resultRecord.StringData(1)
End If

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
'' SIG '' MIImSgYJKoZIhvcNAQcCoIImOzCCJjcCAQExDzANBglg
'' SIG '' hkgBZQMEAgEFADB3BgorBgEEAYI3AgEEoGkwZzAyBgor
'' SIG '' BgEEAYI3AgEeMCQCAQEEEE7wKRaZJ7VNj+Ws4Q8X66sC
'' SIG '' AQACAQACAQACAQACAQAwMTANBglghkgBZQMEAgEFAAQg
'' SIG '' +n92KCymBpb0NXQZvmHi9Nqe+zPewtCgF8ON8kB0lL+g
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
'' SIG '' jgd7JXFEqwZq5tTG3yOalnXFMYIaOzCCGjcCAQEwgZUw
'' SIG '' fjELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0
'' SIG '' b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1p
'' SIG '' Y3Jvc29mdCBDb3Jwb3JhdGlvbjEoMCYGA1UEAxMfTWlj
'' SIG '' cm9zb2Z0IENvZGUgU2lnbmluZyBQQ0EgMjAxMAITMwAA
'' SIG '' BVfPkN3H0cCIjAAAAAAFVzANBglghkgBZQMEAgEFAKCB
'' SIG '' xjAZBgkqhkiG9w0BCQMxDAYKKwYBBAGCNwIBBDAcBgor
'' SIG '' BgEEAYI3AgELMQ4wDAYKKwYBBAGCNwIBFTAvBgkqhkiG
'' SIG '' 9w0BCQQxIgQgVoGH4+dmBshTi+at5GlIupRaD+jboiJ/
'' SIG '' aGuZbreGi74wWgYKKwYBBAGCNwIBDDFMMEqgJIAiAE0A
'' SIG '' aQBjAHIAbwBzAG8AZgB0ACAAVwBpAG4AZABvAHcAc6Ei
'' SIG '' gCBodHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vd2luZG93
'' SIG '' czANBgkqhkiG9w0BAQEFAASCAQBb6SItK/iwHbDVFr90
'' SIG '' lzkk6AjEoiTcSvnLmxa+nEfzefiwJk8oYrtEF/mz2yT5
'' SIG '' FaDMvTiXJWdLW7Ah+KwS7sJsoSGlIyW87ReZNnAkUsN1
'' SIG '' WwPMKElXZNe4GUBUsOz0aEkaGj3mPRB7vw+5E433X+Kv
'' SIG '' I5gQgbT4c7PeVdatttysiZquWxqaJWYgX496sB8ye9pt
'' SIG '' y9j8dqeT1r0Vo21SeMKDz6DMUrOHmYYSQVEyXTF9Mxuq
'' SIG '' 4fQxTVW7oXVIVjqnLW3JQ2V02Oq7xmNQgNGrJYuPLQOs
'' SIG '' bOGSO3xzWkkryCrmNuONgPYl7BNc4uO0uQ1safy1oCHU
'' SIG '' 8TAvXcidNdPhIpxmoYIXrTCCF6kGCisGAQQBgjcDAwEx
'' SIG '' gheZMIIXlQYJKoZIhvcNAQcCoIIXhjCCF4ICAQMxDzAN
'' SIG '' BglghkgBZQMEAgEFADCCAVoGCyqGSIb3DQEJEAEEoIIB
'' SIG '' SQSCAUUwggFBAgEBBgorBgEEAYRZCgMBMDEwDQYJYIZI
'' SIG '' AWUDBAIBBQAEIFBmFCcQ+w/EH/YQyL2dPGCTCVZs9okw
'' SIG '' rxw/j/vCYrYKAgZmxnFTZFwYEzIwMjQwOTA1MDkxNzA4
'' SIG '' LjI1NFowBIACAfSggdmkgdYwgdMxCzAJBgNVBAYTAlVT
'' SIG '' MRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdS
'' SIG '' ZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9y
'' SIG '' YXRpb24xLTArBgNVBAsTJE1pY3Jvc29mdCBJcmVsYW5k
'' SIG '' IE9wZXJhdGlvbnMgTGltaXRlZDEnMCUGA1UECxMeblNo
'' SIG '' aWVsZCBUU1MgRVNOOjUyMUEtMDVFMC1EOTQ3MSUwIwYD
'' SIG '' VQQDExxNaWNyb3NvZnQgVGltZS1TdGFtcCBTZXJ2aWNl
'' SIG '' oIIR+zCCBygwggUQoAMCAQICEzMAAAIAC9eqfxsqF1YA
'' SIG '' AQAAAgAwDQYJKoZIhvcNAQELBQAwfDELMAkGA1UEBhMC
'' SIG '' VVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcT
'' SIG '' B1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jw
'' SIG '' b3JhdGlvbjEmMCQGA1UEAxMdTWljcm9zb2Z0IFRpbWUt
'' SIG '' U3RhbXAgUENBIDIwMTAwHhcNMjQwNzI1MTgzMTIxWhcN
'' SIG '' MjUxMDIyMTgzMTIxWjCB0zELMAkGA1UEBhMCVVMxEzAR
'' SIG '' BgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1v
'' SIG '' bmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlv
'' SIG '' bjEtMCsGA1UECxMkTWljcm9zb2Z0IElyZWxhbmQgT3Bl
'' SIG '' cmF0aW9ucyBMaW1pdGVkMScwJQYDVQQLEx5uU2hpZWxk
'' SIG '' IFRTUyBFU046NTIxQS0wNUUwLUQ5NDcxJTAjBgNVBAMT
'' SIG '' HE1pY3Jvc29mdCBUaW1lLVN0YW1wIFNlcnZpY2UwggIi
'' SIG '' MA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoICAQCvVdpp
'' SIG '' 0qQ/ZOS6ehMXnvf+0Xsokz0OiK/dxy/bqaqdSGa/YMzn
'' SIG '' 2KQRPhadF+jluIUgdzouqsh6oKBP19P8aSzlUo73RlBb
'' SIG '' Zq+H88weeXSRl9f8jnN1Wihcdt1RSQ+Jl81ezNazCYv1
'' SIG '' SVajybPK/0un1MC3D+hc5hMDF1hKo4HlTPPVDcDy4Kk2
'' SIG '' W6ZA0MkYNjpxKfQyVi6ReSUCsKGrqX4piuXqv9ac6pdK
'' SIG '' ScAGmCBKwnfegveieYOI31hQClnCOc2H0zqQNqd5LPvz
'' SIG '' 9i0P/akanH38tcQuhrMRQXGHKDgp2ahYY1jB1Hv+J3zW
'' SIG '' B44RHr2Xl0m/vVL+Yf4vFvovr3afy4SYBXDp9W8T5zzC
'' SIG '' OBhluVkI08DKcKcN25Et2TWOzAKqOo1zdf9YjMDsYazg
'' SIG '' dRLhgisBTHwfYD3i8M2IDwBZrtn8dLBMLIiB5VuV1dgz
'' SIG '' YG3EwqreSd5GhPbs1DtjufxlNoCN7sGV+O7zeykY/9BZ
'' SIG '' g1nXBjNhUZHI6l0DxabGrlXx/mvgdob3M9zKQ7ImlFnL
'' SIG '' 5XdEaKCEWawIlcBwzOI7voeKfAIiMiacIUoYn8hsuMfo
'' SIG '' nt8lepE9uD4fqtgtnxcGmZcEUfg9NqRlVjEH8/4RBsvk
'' SIG '' s3DRkgz565/EKWNXg76ceQz7OBLHz7TsFVk8EqyWih5u
'' SIG '' yaEhwE7Tvv2R6FyJgwIDAQABo4IBSTCCAUUwHQYDVR0O
'' SIG '' BBYEFIAlJNq+CSMqqB7nFA5effl9d3c6MB8GA1UdIwQY
'' SIG '' MBaAFJ+nFV0AXmJdg/Tl0mWnG1M1GelyMF8GA1UdHwRY
'' SIG '' MFYwVKBSoFCGTmh0dHA6Ly93d3cubWljcm9zb2Z0LmNv
'' SIG '' bS9wa2lvcHMvY3JsL01pY3Jvc29mdCUyMFRpbWUtU3Rh
'' SIG '' bXAlMjBQQ0ElMjAyMDEwKDEpLmNybDBsBggrBgEFBQcB
'' SIG '' AQRgMF4wXAYIKwYBBQUHMAKGUGh0dHA6Ly93d3cubWlj
'' SIG '' cm9zb2Z0LmNvbS9wa2lvcHMvY2VydHMvTWljcm9zb2Z0
'' SIG '' JTIwVGltZS1TdGFtcCUyMFBDQSUyMDIwMTAoMSkuY3J0
'' SIG '' MAwGA1UdEwEB/wQCMAAwFgYDVR0lAQH/BAwwCgYIKwYB
'' SIG '' BQUHAwgwDgYDVR0PAQH/BAQDAgeAMA0GCSqGSIb3DQEB
'' SIG '' CwUAA4ICAQAo8Ib2eNG0ipD5+4QKDHNYyxA4jce9buxX
'' SIG '' 0+YRYII5aVO4YIjM2LeJt0tlLRvYgMeDTIuu11W4GLcX
'' SIG '' FV16whe7NjKh8h79qVMF1XgOGKMtNe0Hs2A6ejsbXaI7
'' SIG '' e3qPLWE9Kq7MYuvL/aYRkHAixKhLYP4f7ccInE8PKHMw
'' SIG '' Wo/6mWW08AIH9A3Bnur4cbJm/e/x636tBiDywXc9O5Z4
'' SIG '' ODd1H/OTU1rAn918UINiVY14IEIu809AFx4xhcVEUqFx
'' SIG '' JTCzuYV0gMOFmnGrIgoPZYPAXI0gYR7Of6d3iRdG6l40
'' SIG '' TH55KklfKVEP7V3jmFvo/M4gXsGRw+1G0VbbBeCSMuq1
'' SIG '' NZaUGS/OXa419gncI9lVoPIwNppeA74foOKuwnggb2KQ
'' SIG '' h33jX6ZYN6OSPlpif1A3pE5+j8c0eDW2KbCkWhSK+oAW
'' SIG '' 7qKtZkXDlX7IuvwUtzudsxraUVKLHO73rN2cOw8ibPRz
'' SIG '' pK1tjKEpKUze4NGL1RbJ1IqqcRu0noyT5i7G/OmuS5ZA
'' SIG '' lhZ+k++6D7BOeKjKRXBzTJFVyx3jEzOeedG1kMYxJQSX
'' SIG '' 20xWd5thGyHBkjIlOwGAtmYczurZMUr9f33jhKQirJjb
'' SIG '' YBy4t7Qaqg18BIIhxm3Ntn/M/iVPb83SkNufZ98DONmS
'' SIG '' Ej5Cuqv3zeZBlbBl2vdOTUxgSUNOHPYPQzCCB3EwggVZ
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
'' SIG '' RVNOOjUyMUEtMDVFMC1EOTQ3MSUwIwYDVQQDExxNaWNy
'' SIG '' b3NvZnQgVGltZS1TdGFtcCBTZXJ2aWNloiMKAQEwBwYF
'' SIG '' Kw4DAhoDFQCMk58tlveK+KkvexIuVYVsutaOZKCBgzCB
'' SIG '' gKR+MHwxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNo
'' SIG '' aW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQK
'' SIG '' ExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xJjAkBgNVBAMT
'' SIG '' HU1pY3Jvc29mdCBUaW1lLVN0YW1wIFBDQSAyMDEwMA0G
'' SIG '' CSqGSIb3DQEBCwUAAgUA6oNkNDAiGA8yMDI0MDkwNDIy
'' SIG '' NTY1MloYDzIwMjQwOTA1MjI1NjUyWjB0MDoGCisGAQQB
'' SIG '' hFkKBAExLDAqMAoCBQDqg2Q0AgEAMAcCAQACAgk1MAcC
'' SIG '' AQACAhQyMAoCBQDqhLW0AgEAMDYGCisGAQQBhFkKBAIx
'' SIG '' KDAmMAwGCisGAQQBhFkKAwKgCjAIAgEAAgMHoSChCjAI
'' SIG '' AgEAAgMBhqAwDQYJKoZIhvcNAQELBQADggEBACQlIpMl
'' SIG '' xDRJ3YQrCAJUdhBjt3yGbV7Q82AO+QG8yOLuiFX5ti4c
'' SIG '' 9cSwPbXmmiUTfIzEFlQSen5TXE1wpR+eZaZXHN6lVda5
'' SIG '' ThMcobBj+fAY/mpJvgn4/RkHuMaVm+zY35a5BVOaW/mb
'' SIG '' qmDNSHJF/B2lNVJy14zC1miqGF0Km06DETNGrF8ng+eO
'' SIG '' AdTLQqiH/OZKzL4wmS/8/U9chAfVVKOZMo64aN96unMK
'' SIG '' /ZBYvEOGNQi1McqtbaITnm8PRC5gjHm/N/Pm4AKV89Z4
'' SIG '' 7bImoiH2XV+W6Y5Eo9QwNyC/KzqrnJ+DoTuu2nYavb8H
'' SIG '' U/1zXODGlayXRp/xfYzupEa4LZMxggQNMIIECQIBATCB
'' SIG '' kzB8MQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2FzaGlu
'' SIG '' Z3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UEChMV
'' SIG '' TWljcm9zb2Z0IENvcnBvcmF0aW9uMSYwJAYDVQQDEx1N
'' SIG '' aWNyb3NvZnQgVGltZS1TdGFtcCBQQ0EgMjAxMAITMwAA
'' SIG '' AgAL16p/GyoXVgABAAACADANBglghkgBZQMEAgEFAKCC
'' SIG '' AUowGgYJKoZIhvcNAQkDMQ0GCyqGSIb3DQEJEAEEMC8G
'' SIG '' CSqGSIb3DQEJBDEiBCAPCEuoVcXkp8hbioha3+9ou7Ej
'' SIG '' QfTkXcRTDOXe99CfmDCB+gYLKoZIhvcNAQkQAi8xgeow
'' SIG '' gecwgeQwgb0EINTI7ew1ndu6sE0MZQJXg18zaAfhpa5G
'' SIG '' 50iT/0oCT9knMIGYMIGApH4wfDELMAkGA1UEBhMCVVMx
'' SIG '' EzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1Jl
'' SIG '' ZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3Jh
'' SIG '' dGlvbjEmMCQGA1UEAxMdTWljcm9zb2Z0IFRpbWUtU3Rh
'' SIG '' bXAgUENBIDIwMTACEzMAAAIAC9eqfxsqF1YAAQAAAgAw
'' SIG '' IgQg8sdFrgSvuQcZt9eJLGFORNgBtqb+E4sTHsFhoeZm
'' SIG '' OYwwDQYJKoZIhvcNAQELBQAEggIAjlc0/wIMb6EB928O
'' SIG '' XIRr4ab0ybjYkLvPMO8osHeJnJaBDJZIEZsxP0t5BkKp
'' SIG '' jhKAg3Nb8TOQsLxnc+Gu3yRMSY8XCB0+my6QA5zFkysv
'' SIG '' WmTLRB/cgy3DNSjdQn5B3ByZD8p+ltM00N3pZgvvinXW
'' SIG '' sZZNJf89Fbx/3sT8cnG4zgjlLr5E1ydKa2o0ClBcA4tj
'' SIG '' Jj+U/89tb0BJtuutRL6KDXygBR/gHAaxL4Xgb7jJoroN
'' SIG '' v7vUBvKyjiboDC/XaYgo0SBeDSKfj5Pr68PWYwVNuiTD
'' SIG '' rUQgJK4CNe53qBXX3gA4ZEpp98L0bu93g3MmkJKCnCVb
'' SIG '' u87j7HBXskT/tcPDmXIzcRm2RMUBh7NJCzMS8IVqC4sw
'' SIG '' k06se4AsVfB7r8p1luir2r899lh/snhC7fZUFl8AAu+Z
'' SIG '' URK5uNSEAFTBkGPHATh1HeZzvZ9LGnEj6ARCp8BTmnED
'' SIG '' 2pg14OiIh1Tiac+//a6T0JSvXl9jgjSo/gXqkhavjllj
'' SIG '' ldZi+EOZxyAmYjvVyCLs4eMHYfRT3ymrl1eSfDOsnPBm
'' SIG '' we0nsDCEQWXFHdLdsKV1XK25tyRQnuLUvHZDQlzeTzsI
'' SIG '' dwN6v1ph5yyvxq8dk2r9TKl2Zi24bLySEd3lp/fJAXaW
'' SIG '' YECyWfnExjovDeMJ1u5pRfho5fj+px6wkHRKkUTGKy2Q
'' SIG '' IFP9LDI=
'' SIG '' End signature block
