' Windows Installer utility to list feature composition in an MSI database
' For use with Windows Scripting Host, CScript.exe or WScript.exe
' Copyright (c) Microsoft Corporation. All rights reserved.
' Demonstrates the use of adding temporary columns to a read-only database
'
Option Explicit
Public isGUI, installer, database, message, featureParam, nextSequence  'global variables accessed across functions

Const msiOpenDatabaseModeReadOnly = 0
Const msiDbNullInteger            = &h80000000
Const msiViewModifyUpdate         = 2

' Check if run from GUI script host, in order to modify display
If UCase(Mid(Wscript.FullName, Len(Wscript.Path) + 2, 1)) = "W" Then isGUI = True

' Show help if no arguments or if argument contains ?
Dim argCount:argCount = Wscript.Arguments.Count
If argCount > 0 Then If InStr(1, Wscript.Arguments(0), "?", vbTextCompare) > 0 Then argCount = 0
If argCount = 0 Then
	Wscript.Echo "Windows Installer utility to list feature composition in an installer database." &_
		vbLf & " The 1st argument is the path to an install database, relative or complete path" &_
		vbLf & " The 2nd argument is the name of the feature (the primary key of Feature table)" &_
		vbLf & " If the 2nd argument is not present, all feature names will be listed as a tree" &_
		vbLf & " If the 2nd argument is ""*"" then the composition of all features will be listed" &_
		vbLf & " Large databases or features are better displayed by using CScript than WScript" &_
		vbLf & " Note: The name of the feature, if provided,  is case-sensitive" &_
		vbNewLine &_
		vbNewLine & "Copyright (C) Microsoft Corporation.  All rights reserved."
	Wscript.Quit 1
End If

' Connect to Windows Installer object
On Error Resume Next
Set installer = Nothing
Set installer = Wscript.CreateObject("WindowsInstaller.Installer") : CheckError

' Open database
Dim databasePath:databasePath = Wscript.Arguments(0)
Set database = installer.OpenDatabase(databasePath, msiOpenDatabaseModeReadOnly) : CheckError
REM Set database = installer.OpenDatabase(databasePath, 1) : CheckError

If argCount = 1 Then  'If no feature specified, then simply list features
	ListFeatures False
	ShowOutput "Features for " & databasePath, message
ElseIf Left(Wscript.Arguments(1), 1) = "*" Then 'List all features
	ListFeatures True
Else
	QueryFeature Wscript.Arguments(1) 
End If
Wscript.Quit 0

' List all table rows referencing a given feature
Function QueryFeature(feature)
	' Get feature info and format output header
	Dim view, record, header, parent
	Set view = database.OpenView("SELECT `Feature_Parent` FROM `Feature` WHERE `Feature` = ?") : CheckError
	Set featureParam = installer.CreateRecord(1)
	featureParam.StringData(1) = feature
	view.Execute featureParam : CheckError
	Set record = view.Fetch : CheckError
	Set view = Nothing
	If record Is Nothing Then Fail "Feature not in database: " & feature
	parent = record.StringData(1)
	header = "Feature: "& feature & "  Parent: " & parent

	' List of tables with foreign keys to Feature table - with subsets of columns to display
	DoQuery "FeatureComponents","Component_"                         '
	DoQuery "Condition",        "Level,Condition"                    '
	DoQuery "Billboard",        "Billboard,Action"                   'Ordering

	QueryFeature = ShowOutput(header, message)
	message = Empty
End Function

' Query used for sorting and corresponding record field indices
const irecParent   = 1  'put first in order to use as query parameter
const irecChild    = 2  'primary key of Feature table
const irecSequence = 3  'temporary column added for sorting
const sqlSort = "SELECT `Feature_Parent`,`Feature`,`Sequence` FROM `Feature`"

' Recursive function to resolve parent feature chain, return tree level (low order 8 bits of sequence number)
Function LinkParent(childView)
	Dim view, record, level
	On Error Resume Next
	Set record = childView.Fetch
	If record Is Nothing Then Exit Function  'return Empty if no record found
	If Not record.IsNull(irecSequence) Then LinkParent = (record.IntegerData(irecSequence) And 255) + 1 : Exit Function 'Already resolved
	If record.IsNull(irecParent) Or record.StringData(irecParent) = record.StringData(irecChild) Then 'Root node
		level = 0
	Else  'child node, need to get level from parent
		Set view = database.OpenView(sqlSort & " WHERE `Feature` = ?") : CheckError
		view.Execute record : CheckError '1st param is parent feature
		level = LinkParent(view)
		If IsEmpty(level) Then Fail "Feature parent does not exist: " & record.StringData(irecParent)
	End If
	record.IntegerData(irecSequence) = nextSequence + level
	nextSequence = nextSequence + 256
	childView.Modify msiViewModifyUpdate, record : CheckError
	LinkParent = level + 1
End Function

' List all features in database, sorted hierarchically
Sub ListFeatures(queryAll)
	Dim viewSchema, view, record, feature, level
	On Error Resume Next
	Set viewSchema = database.OpenView("ALTER TABLE Feature ADD Sequence LONG TEMPORARY") : CheckError
	viewSchema.Execute : CheckError  'Add ordering column, keep view open to hold temp columns
	Set view = database.OpenView(sqlSort) : CheckError
	view.Execute : CheckError
	nextSequence = 0
	While LinkParent(view) : Wend  'Loop to link rows hierachically
	Set view = database.OpenView("SELECT `Feature`,`Title`, `Sequence` FROM `Feature` ORDER BY Sequence") : CheckError
	view.Execute : CheckError
	Do
		Set record = view.Fetch : CheckError
		If record Is Nothing Then Exit Do
		feature = record.StringData(1)
		level = record.IntegerData(3) And 255
		If queryAll Then
			If QueryFeature(feature) = vbCancel Then Exit Sub
		Else
			If Not IsEmpty(message) Then message = message & vbLf
			message = message & Space(level * 2) & feature & "  (" & record.StringData(2) & ")"
		End If
	Loop
End Sub

' Perform a join to query table rows linked to a given feature, delimiting and qualifying names to prevent conflicts
Sub DoQuery(table, columns)
	Dim view, record, columnCount, column, output, header, delim, columnList, tableList, tableDelim, query, joinTable, primaryKey, foreignKey, columnDelim
	On Error Resume Next
	tableList  = Replace(table,   ",", "`,`")
	tableDelim = InStr(1, table, ",", vbTextCompare)
	If tableDelim Then  ' need a 3-table join
		joinTable = Right(table, Len(table)-tableDelim)
		table = Left(table, tableDelim-1)
		foreignKey = columns
		Set record = database.PrimaryKeys(joinTable)
		primaryKey = record.StringData(1)
		columnDelim = InStr(1, columns, ",", vbTextCompare)
		If columnDelim Then foreignKey = Left(columns, columnDelim - 1)
		query = " AND `" & foreignKey & "` = `" & primaryKey & "`"
	End If
	columnList = table & "`." & Replace(columns, ",", "`,`" & table & "`.`")
	query = "SELECT `" & columnList & "` FROM `" & tableList & "` WHERE `Feature_` = ?" & query
	If database.TablePersistent(table) <> 1 Then Exit Sub
	Set view = database.OpenView(query) : CheckError
	view.Execute featureParam : CheckError
	Do
		Set record = view.Fetch : CheckError
		If record Is Nothing Then Exit Do
		If IsEmpty(output) Then
			If Not IsEmpty(message) Then message = message & vbLf
			message = message & "----" & table & " Table----  (" & columns & ")" & vbLf
		End If
		output = Empty
		columnCount = record.FieldCount
		delim = "  "
		For column = 1 To columnCount
			If column = columnCount Then delim = vbLf
			output = output & record.StringData(column) & delim
		Next
		message = message & output
	Loop
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

Function ShowOutput(header, message)
	ShowOutput = vbOK
	If IsEmpty(message) Then Exit Function
	If isGUI Then
		ShowOutput = MsgBox(message, vbOKCancel, header)
	Else
		Wscript.Echo "> " & header
		Wscript.Echo message
	End If
End Function

Sub Fail(message)
	Wscript.Echo message
	Wscript.Quit 2
End Sub

'' SIG '' Begin signature block
'' SIG '' MIImMQYJKoZIhvcNAQcCoIImIjCCJh4CAQExDzANBglg
'' SIG '' hkgBZQMEAgEFADB3BgorBgEEAYI3AgEEoGkwZzAyBgor
'' SIG '' BgEEAYI3AgEeMCQCAQEEEE7wKRaZJ7VNj+Ws4Q8X66sC
'' SIG '' AQACAQACAQACAQACAQAwMTANBglghkgBZQMEAgEFAAQg
'' SIG '' o40u8w/79QYYEVTk+LHUW40T5s95rhdh/xj2PF/Wgf2g
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
'' SIG '' 9w0BCQQxIgQgZi2jXnHmXUs+wBApyuT8W3+JK0Ql3la+
'' SIG '' 1qnCnmPAE0QwWgYKKwYBBAGCNwIBDDFMMEqgJIAiAE0A
'' SIG '' aQBjAHIAbwBzAG8AZgB0ACAAVwBpAG4AZABvAHcAc6Ei
'' SIG '' gCBodHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vd2luZG93
'' SIG '' czANBgkqhkiG9w0BAQEFAASCAQBJGBcBuey/QwUzvWaa
'' SIG '' PdqRdwWHWK7lsvuE6CxBcgh4SLOsuq8mq5cSLElUFo2I
'' SIG '' 7SPFt9nDPWfzsn13Px17Dt1h9oTJ5blf0d0eZRrfqtTj
'' SIG '' 2u3YxcniZezr7UuEe3rdtk/UZT2JGLkOqdY/gS6pqYk2
'' SIG '' s+CyTQYd7TMLxtYS5mo55AOlm7seixb06guxJViUsc/W
'' SIG '' mSw4Xto/lqc2zSZFN9Jpf8qySOUGrw2Us19gcyWp1lDg
'' SIG '' OKInVO9Az3LlxjxarXTjWoJ4Le8M6ysKejfMmGQwiFd7
'' SIG '' GjfGqFtOm802aZYtdr+ggz0oyOqZ8ZMzN/CN9HsmJ+C0
'' SIG '' BnVg9bL5yifa69yXoYIXlDCCF5AGCisGAQQBgjcDAwEx
'' SIG '' gheAMIIXfAYJKoZIhvcNAQcCoIIXbTCCF2kCAQMxDzAN
'' SIG '' BglghkgBZQMEAgEFADCCAVIGCyqGSIb3DQEJEAEEoIIB
'' SIG '' QQSCAT0wggE5AgEBBgorBgEEAYRZCgMBMDEwDQYJYIZI
'' SIG '' AWUDBAIBBQAEIBR3cEV6anAIbg+Yd1pn6E6rYwFY3rdc
'' SIG '' sFIg1FEO1ahKAgZmvjWYD10YEzIwMjQwOTA1MDkxNzI3
'' SIG '' LjQ5N1owBIACAfSggdGkgc4wgcsxCzAJBgNVBAYTAlVT
'' SIG '' MRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdS
'' SIG '' ZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9y
'' SIG '' YXRpb24xJTAjBgNVBAsTHE1pY3Jvc29mdCBBbWVyaWNh
'' SIG '' IE9wZXJhdGlvbnMxJzAlBgNVBAsTHm5TaGllbGQgVFNT
'' SIG '' IEVTTjpGMDAyLTA1RTAtRDk0NzElMCMGA1UEAxMcTWlj
'' SIG '' cm9zb2Z0IFRpbWUtU3RhbXAgU2VydmljZaCCEeowggcg
'' SIG '' MIIFCKADAgECAhMzAAAB8j4y12SscJGUAAEAAAHyMA0G
'' SIG '' CSqGSIb3DQEBCwUAMHwxCzAJBgNVBAYTAlVTMRMwEQYD
'' SIG '' VQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25k
'' SIG '' MR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24x
'' SIG '' JjAkBgNVBAMTHU1pY3Jvc29mdCBUaW1lLVN0YW1wIFBD
'' SIG '' QSAyMDEwMB4XDTIzMTIwNjE4NDU1OFoXDTI1MDMwNTE4
'' SIG '' NDU1OFowgcsxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpX
'' SIG '' YXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYD
'' SIG '' VQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xJTAjBgNV
'' SIG '' BAsTHE1pY3Jvc29mdCBBbWVyaWNhIE9wZXJhdGlvbnMx
'' SIG '' JzAlBgNVBAsTHm5TaGllbGQgVFNTIEVTTjpGMDAyLTA1
'' SIG '' RTAtRDk0NzElMCMGA1UEAxMcTWljcm9zb2Z0IFRpbWUt
'' SIG '' U3RhbXAgU2VydmljZTCCAiIwDQYJKoZIhvcNAQEBBQAD
'' SIG '' ggIPADCCAgoCggIBALzl88sXCmliDHBjGRIR5i9AG2dg
'' SIG '' lO0oqPYUrHMfHR+BXpeAgiuYJaakqX0g7O858n+TqI/R
'' SIG '' GehGjkXz0B3b153MZ2VZsKPVDLHkdQc1jzK70SUk6Z2B
'' SIG '' 6429MrhFbjC72IHn/PZJ4K5irJf+/zPo+m/b2HW201ax
'' SIG '' Jz8o8566HNIBeqQDbrkFIVPmTKTG/MHQvGjFLqhahdYr
'' SIG '' rDHXvY1ElFhwg19cOFRG9R8PvSOKgT3atb86CNw4rFmR
'' SIG '' 9DEuXBoVKtKcazteEyun1OxSCbCzJxMQ4F0ZWZ/UcIPt
'' SIG '' Y5rPkQRxDIhLYGlFhjCw8xsHre4eInXnyo2HVIle6gvn
'' SIG '' AYO79tlTM34HNwuP3qLELvAkZAwGLFYf1375XxuXXRFh
'' SIG '' 1cNmWWNEC9LqIXA3OtqG7gOthvtvwzu+/CEQvTEI69vt
'' SIG '' YUyyy2xxd+R0TmD41JpymGAV9yh+1Dmo8PY81WasbfwO
'' SIG '' YcOhiGCP26o8s/u+ehd/uPr4tbxWifXnwPRauaTsK6a5
'' SIG '' xBOIdHJ6kRpUOecDYaSImh6H+vd9KEvoIeA+hMHuhhT9
'' SIG '' 3ok6dxGKgNiqpF9XbCWkpU7xv5VgcvyGfXUlEXHqnr2Y
'' SIG '' vwFG1Jnp0b8YURUT59WaDFh8gJSumCHJCURMk8hMQFLX
'' SIG '' kixpS5bQa9eUtKh8Z/a3kMCgOS4oJsL7dV0+aVhVAgMB
'' SIG '' AAGjggFJMIIBRTAdBgNVHQ4EFgQUlVuHACbq0DEEzlwf
'' SIG '' wGDT5jrihnkwHwYDVR0jBBgwFoAUn6cVXQBeYl2D9OXS
'' SIG '' ZacbUzUZ6XIwXwYDVR0fBFgwVjBUoFKgUIZOaHR0cDov
'' SIG '' L3d3dy5taWNyb3NvZnQuY29tL3BraW9wcy9jcmwvTWlj
'' SIG '' cm9zb2Z0JTIwVGltZS1TdGFtcCUyMFBDQSUyMDIwMTAo
'' SIG '' MSkuY3JsMGwGCCsGAQUFBwEBBGAwXjBcBggrBgEFBQcw
'' SIG '' AoZQaHR0cDovL3d3dy5taWNyb3NvZnQuY29tL3BraW9w
'' SIG '' cy9jZXJ0cy9NaWNyb3NvZnQlMjBUaW1lLVN0YW1wJTIw
'' SIG '' UENBJTIwMjAxMCgxKS5jcnQwDAYDVR0TAQH/BAIwADAW
'' SIG '' BgNVHSUBAf8EDDAKBggrBgEFBQcDCDAOBgNVHQ8BAf8E
'' SIG '' BAMCB4AwDQYJKoZIhvcNAQELBQADggIBAD1Lp47gex8H
'' SIG '' TRek6A9ptw3dBl7KKmCKVxBINnyDpUK/0VUfN1Kr1ekC
'' SIG '' yWNlIo1ZIKWEkTPk6jdSb+1o+ehsX7wKQB2RwtCEt2RK
'' SIG '' F+v3WTPL28M+s6aUIDYVD2NWEVpq3ZAzffPWn4YI/m26
'' SIG '' +KsVpRbNRZUMU6mj87nMOnOg9i1OvRwWDe5dpEtPnhRD
'' SIG '' dji49heqfrC6dm1RBEyIkzPGlSW919YZS0K+dbd4MGKQ
'' SIG '' OSLHVcT3xVxgjPb7l91y+sdV5RqsZfLgtG3DObCmwK1S
'' SIG '' Hu1HrCEKtViRvoW50F1YztNW+OLukaB+N6yCcBJoP8KE
'' SIG '' u7Hro8bBohoX7EvOTRs3GwCPS6F3pB1avpNPf2b9I1nX
'' SIG '' 9RdTuTMSh3S8BjeYifxfkDgj7397WcE2lREnpiIMpB3l
'' SIG '' hWDGy5kJa/hDBvSZeEch70K5t9KpmO8NrB/Yjbb03cuy
'' SIG '' 0MlRKvW8YUHyJDlbxkszk/BPy+2woQHAcRibCy5aazGS
'' SIG '' KYgXkFBtLOD3DPU7qN1ZPEYbQ5S3VxdY4wlQnPIQfhZI
'' SIG '' pkc7HnepwC8P2HRTqMQXZ+4GO0n9AOtZtvi6u8B+u+o2
'' SIG '' f2UfuBU+mWo08Mi9DwORneW9tCxiqXPrXt7vqBrtJjTD
'' SIG '' vX5A/XrkI93NRjfp63ZKbim+ykQryGWWrchhzJfS/z3v
'' SIG '' 5f1h55wzU9vWMIIHcTCCBVmgAwIBAgITMwAAABXF52ue
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
'' SIG '' ZWxkIFRTUyBFU046RjAwMi0wNUUwLUQ5NDcxJTAjBgNV
'' SIG '' BAMTHE1pY3Jvc29mdCBUaW1lLVN0YW1wIFNlcnZpY2Wi
'' SIG '' IwoBATAHBgUrDgMCGgMVAGuL3jdwUsfZN9AR8HTlIsgK
'' SIG '' DvgIoIGDMIGApH4wfDELMAkGA1UEBhMCVVMxEzARBgNV
'' SIG '' BAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQx
'' SIG '' HjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEm
'' SIG '' MCQGA1UEAxMdTWljcm9zb2Z0IFRpbWUtU3RhbXAgUENB
'' SIG '' IDIwMTAwDQYJKoZIhvcNAQELBQACBQDqg7j0MCIYDzIw
'' SIG '' MjQwOTA1MDQ1ODI4WhgPMjAyNDA5MDYwNDU4MjhaMHQw
'' SIG '' OgYKKwYBBAGEWQoEATEsMCowCgIFAOqDuPQCAQAwBwIB
'' SIG '' AAICCgQwBwIBAAICE0cwCgIFAOqFCnQCAQAwNgYKKwYB
'' SIG '' BAGEWQoEAjEoMCYwDAYKKwYBBAGEWQoDAqAKMAgCAQAC
'' SIG '' AwehIKEKMAgCAQACAwGGoDANBgkqhkiG9w0BAQsFAAOC
'' SIG '' AQEApwWPvLf1hU7gY25d9B7ohcl41A4eizl87jlBGPrN
'' SIG '' V3C9sxWzl9nuAQQCHsaARNcvc3ctfVePH/sWU2PH/S4H
'' SIG '' EYPOdXKFNntSfeWNv7n+iDVBJgO467NeE+cgCmT43IX2
'' SIG '' /uKCFjBDX3tUTeR8VBG2vkw2bDZRiH76BGZxjAcXXJgd
'' SIG '' Hk+9NDmJcxSm5e8VXQDCpFEBDmDHMKlEEB0p/vf+hi6f
'' SIG '' +hT59uzZZs1gk+J8DfhLjyd8QkyBdWQcObqk7ON+LCE5
'' SIG '' NKwuBRyd2X2Gnjj9AVnu2IdI0ORuqA+Ys/Wa9HRQM29I
'' SIG '' fnVfxRfN0AHfNs16uiexAX/xJvGuoLVQjj4APDGCBA0w
'' SIG '' ggQJAgEBMIGTMHwxCzAJBgNVBAYTAlVTMRMwEQYDVQQI
'' SIG '' EwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4w
'' SIG '' HAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xJjAk
'' SIG '' BgNVBAMTHU1pY3Jvc29mdCBUaW1lLVN0YW1wIFBDQSAy
'' SIG '' MDEwAhMzAAAB8j4y12SscJGUAAEAAAHyMA0GCWCGSAFl
'' SIG '' AwQCAQUAoIIBSjAaBgkqhkiG9w0BCQMxDQYLKoZIhvcN
'' SIG '' AQkQAQQwLwYJKoZIhvcNAQkEMSIEIEGHOo1eZkWJ2Tmx
'' SIG '' qQcmVymIQJoR3jfrlTMfWcNLfISqMIH6BgsqhkiG9w0B
'' SIG '' CRACLzGB6jCB5zCB5DCBvQQg+No+HS4xUlzTj5jhG7kF
'' SIG '' RRscTiy5nqdEdJS7RddKQ0QwgZgwgYCkfjB8MQswCQYD
'' SIG '' VQQGEwJVUzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4G
'' SIG '' A1UEBxMHUmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0
'' SIG '' IENvcnBvcmF0aW9uMSYwJAYDVQQDEx1NaWNyb3NvZnQg
'' SIG '' VGltZS1TdGFtcCBQQ0EgMjAxMAITMwAAAfI+MtdkrHCR
'' SIG '' lAABAAAB8jAiBCC01RK9XT1pYuP121ROJdlTArToCvDY
'' SIG '' GINmapSQvhVNoDANBgkqhkiG9w0BAQsFAASCAgCabsCG
'' SIG '' qvcPJmjeyD2qObJc7Jzrd8ueQilrAwxqCpVOBT1gTt/+
'' SIG '' RZbDU3ISm4gVn1aLX5nZJB3HI9ikWuY+ExG1hOdn0DZr
'' SIG '' 8aqeKTkxw8laQGPBfg/OheuScYYQP170RlDUzH8kfYbQ
'' SIG '' V0JGNElO3iPBoB+z5RkoXmu7xpgonh+VMibTERDoVVmK
'' SIG '' eBUBT03h7sp0rre55ReIkp/ETpYp+1wCGUklLmooYCN4
'' SIG '' G6GtkY7lHqLhhKEUMRYwYyiLznoYbOi5Neqfs66HpptX
'' SIG '' vY4Eda27y3rtZSbBiXJdtph2SM+dSQ+Pe0atBKBQc08V
'' SIG '' f0U00x7iksiMiZHaaVtGWtl6j/q5ZVyNzl/xxD+84Riw
'' SIG '' oqorbo+VGOp8WQ8eyT49Hah/9XknYiqq922AKkKjcJsr
'' SIG '' 0tVr3q+ne9Jeek2QmtzPzJxn99+h1HTE7aMnE4NyFMeN
'' SIG '' Q1NOYbSLjrhEneP/28Bti6qMLZ/4NdfW6uCnRi1kbIzh
'' SIG '' oondArF1O3ewSujNtxquChOk7GDbPA5sZ3pP0yg42kkR
'' SIG '' TaIpjG9AwWLbEC9/cgyPHttKetFrsHs5xg6fvUhp0WLh
'' SIG '' UUiAqamtMKrjJPZmBMiENf5lRmbLZsxn12WQwAY707S7
'' SIG '' f5HMKxCJGqKx/cNXITLatqDxG1mvIrlfMBNE/W7vx1gf
'' SIG '' A1qB2OEiG+bPUCqrCg==
'' SIG '' End signature block
