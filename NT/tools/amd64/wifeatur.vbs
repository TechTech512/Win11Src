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
'' SIG '' MIImSAYJKoZIhvcNAQcCoIImOTCCJjUCAQExDzANBglg
'' SIG '' hkgBZQMEAgEFADB3BgorBgEEAYI3AgEEoGkwZzAyBgor
'' SIG '' BgEEAYI3AgEeMCQCAQEEEE7wKRaZJ7VNj+Ws4Q8X66sC
'' SIG '' AQACAQACAQACAQACAQAwMTANBglghkgBZQMEAgEFAAQg
'' SIG '' o40u8w/79QYYEVTk+LHUW40T5s95rhdh/xj2PF/Wgf2g
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
'' SIG '' jgd7JXFEqwZq5tTG3yOalnXFMYIaOTCCGjUCAQEwgZUw
'' SIG '' fjELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0
'' SIG '' b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1p
'' SIG '' Y3Jvc29mdCBDb3Jwb3JhdGlvbjEoMCYGA1UEAxMfTWlj
'' SIG '' cm9zb2Z0IENvZGUgU2lnbmluZyBQQ0EgMjAxMAITMwAA
'' SIG '' Bae4j/uXXTWE7AAAAAAFpzANBglghkgBZQMEAgEFAKCB
'' SIG '' xjAZBgkqhkiG9w0BCQMxDAYKKwYBBAGCNwIBBDAcBgor
'' SIG '' BgEEAYI3AgELMQ4wDAYKKwYBBAGCNwIBFTAvBgkqhkiG
'' SIG '' 9w0BCQQxIgQgZi2jXnHmXUs+wBApyuT8W3+JK0Ql3la+
'' SIG '' 1qnCnmPAE0QwWgYKKwYBBAGCNwIBDDFMMEqgJIAiAE0A
'' SIG '' aQBjAHIAbwBzAG8AZgB0ACAAVwBpAG4AZABvAHcAc6Ei
'' SIG '' gCBodHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vd2luZG93
'' SIG '' czANBgkqhkiG9w0BAQEFAASCAQBh0PA5B8SB/K0XOQTc
'' SIG '' FwFI3Fpgj7BCRPi623n8i5ZoT6ssF+qzcvtrjtqAXGr6
'' SIG '' KMfNESVDiohHMmdxm7RRPTOX7jwzpAcLZjkfn6Ns+rwD
'' SIG '' cyRH9ys8VM5u9g71GNGcka5pVSzLATToFFg4n2g6qwxY
'' SIG '' 3IlaNNQax/QyjDdFUOcQQ9O/8Wz9v4Btc3H+Na7JfW9O
'' SIG '' mKKgHeOtHy9Rqaxtgm4zgveuxDi/sr+Gm5AlBcrTZOTw
'' SIG '' Cj0EfjYq0MT3k+jovxfxmag/jJoK8zedOsHsx4xYh4dv
'' SIG '' IlDjITdZvYxlugBe+u6AqObuluYBh6UMVPjpudnQDYCo
'' SIG '' fmUDLbeHBmiK3vMcoYIXqzCCF6cGCisGAQQBgjcDAwEx
'' SIG '' gheXMIIXkwYJKoZIhvcNAQcCoIIXhDCCF4ACAQMxDzAN
'' SIG '' BglghkgBZQMEAgEFADCCAVgGCyqGSIb3DQEJEAEEoIIB
'' SIG '' RwSCAUMwggE/AgEBBgorBgEEAYRZCgMBMDEwDQYJYIZI
'' SIG '' AWUDBAIBBQAEIHmtXW53bM5eo6iQAz+OPkJO7ReCiiuF
'' SIG '' qrQeSN3MpwtjAgZm6zOOo6oYETIwMjQxMTE2MDkxNjUz
'' SIG '' LjJaMASAAgH0oIHZpIHWMIHTMQswCQYDVQQGEwJVUzET
'' SIG '' MBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMHUmVk
'' SIG '' bW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBvcmF0
'' SIG '' aW9uMS0wKwYDVQQLEyRNaWNyb3NvZnQgSXJlbGFuZCBP
'' SIG '' cGVyYXRpb25zIExpbWl0ZWQxJzAlBgNVBAsTHm5TaGll
'' SIG '' bGQgVFNTIEVTTjoyRDFBLTA1RTAtRDk0NzElMCMGA1UE
'' SIG '' AxMcTWljcm9zb2Z0IFRpbWUtU3RhbXAgU2VydmljZaCC
'' SIG '' EfswggcoMIIFEKADAgECAhMzAAAB/XP5aFrNDGHtAAEA
'' SIG '' AAH9MA0GCSqGSIb3DQEBCwUAMHwxCzAJBgNVBAYTAlVT
'' SIG '' MRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdS
'' SIG '' ZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9y
'' SIG '' YXRpb24xJjAkBgNVBAMTHU1pY3Jvc29mdCBUaW1lLVN0
'' SIG '' YW1wIFBDQSAyMDEwMB4XDTI0MDcyNTE4MzExNloXDTI1
'' SIG '' MTAyMjE4MzExNlowgdMxCzAJBgNVBAYTAlVTMRMwEQYD
'' SIG '' VQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25k
'' SIG '' MR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24x
'' SIG '' LTArBgNVBAsTJE1pY3Jvc29mdCBJcmVsYW5kIE9wZXJh
'' SIG '' dGlvbnMgTGltaXRlZDEnMCUGA1UECxMeblNoaWVsZCBU
'' SIG '' U1MgRVNOOjJEMUEtMDVFMC1EOTQ3MSUwIwYDVQQDExxN
'' SIG '' aWNyb3NvZnQgVGltZS1TdGFtcCBTZXJ2aWNlMIICIjAN
'' SIG '' BgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAoWWs+D+O
'' SIG '' u4JjYnRHRedu0MTFYzNJEVPnILzc02R3qbnujvhZgkhp
'' SIG '' +p/lymYLzkQyG2zpxYceTjIF7HiQWbt6FW3ARkBrthJU
'' SIG '' z05ZnKpcF31lpUEb8gUXiD2xIpo8YM+SD0S+hTP1TCA/
'' SIG '' we38yZ3BEtmZtcVnaLRp/Avsqg+5KI0Kw6TDJpKwTLl0
'' SIG '' VW0/23sKikeWDSnHQeTprO0zIm/btagSYm3V/8zXlfxy
'' SIG '' 7s/EVFdSglHGsUq8EZupUO8XbHzz7tURyiD3kOxNnw5o
'' SIG '' x1eZX/c/XmW4H6b4yNmZF0wTZuw37yA1PJKOySSrXrWE
'' SIG '' h+H6++Wb6+1ltMCPoMJHUtPP3Cn0CNcNvrPyJtDacqjn
'' SIG '' ITrLzrsHdOLqjsH229Zkvndk0IqxBDZgMoY+Ef7ffFRP
'' SIG '' 2pPkrF1F9IcBkYz8hL+QjX+u4y4Uqq4UtT7VRnsqvR/x
'' SIG '' /+QLE0pcSEh/XE1w1fcp6Jmq8RnHEXikycMLN/a/KYxp
'' SIG '' SP3FfFbLZuf+qIryFL0gEDytapGn1ONjVkiKpVP2uqVI
'' SIG '' Yj4ViCjy5pLUceMeqiKgYqhpmUHCE2WssLLhdQBHdpl2
'' SIG '' 8+k+ZY6m4dPFnEoGcJHuMcIZnw4cOwixojROr+Nq71cJ
'' SIG '' j7Q4L0XwPvuTHQt0oH7RKMQgmsy7CVD7v55dOhdHXdYs
'' SIG '' yO69dAdK+nWlyYcCAwEAAaOCAUkwggFFMB0GA1UdDgQW
'' SIG '' BBTpDMXA4ZW8+yL2+3vA6RmU7oEKpDAfBgNVHSMEGDAW
'' SIG '' gBSfpxVdAF5iXYP05dJlpxtTNRnpcjBfBgNVHR8EWDBW
'' SIG '' MFSgUqBQhk5odHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20v
'' SIG '' cGtpb3BzL2NybC9NaWNyb3NvZnQlMjBUaW1lLVN0YW1w
'' SIG '' JTIwUENBJTIwMjAxMCgxKS5jcmwwbAYIKwYBBQUHAQEE
'' SIG '' YDBeMFwGCCsGAQUFBzAChlBodHRwOi8vd3d3Lm1pY3Jv
'' SIG '' c29mdC5jb20vcGtpb3BzL2NlcnRzL01pY3Jvc29mdCUy
'' SIG '' MFRpbWUtU3RhbXAlMjBQQ0ElMjAyMDEwKDEpLmNydDAM
'' SIG '' BgNVHRMBAf8EAjAAMBYGA1UdJQEB/wQMMAoGCCsGAQUF
'' SIG '' BwMIMA4GA1UdDwEB/wQEAwIHgDANBgkqhkiG9w0BAQsF
'' SIG '' AAOCAgEAY9hYX+T5AmCrYGaH96TdR5T52/PNOG7ySYeo
'' SIG '' pv4flnDWQLhBlravAg+pjlNv5XSXZrKGv8e4s5dJ5Wdh
'' SIG '' fC9ywFQq4TmXnUevPXtlubZk+02BXK6/23hM0TSKs2Kl
'' SIG '' hYiqzbRe8QbMfKXEDtvMoHSZT7r+wI2IgjYQwka+3P9V
'' SIG '' XgERwu46/czz8IR/Zq+vO5523Jld6ssVuzs9uwIrJhfc
'' SIG '' YBj50mXWRBcMhzajLjWDgcih0DuykPcBpoTLlOL8LpXo
'' SIG '' oqnr+QLYE4BpUep3JySMYfPz2hfOL3g02WEfsOxp8ANb
'' SIG '' cdiqM31dm3vSheEkmjHA2zuM+Tgn4j5n+Any7IODYQkI
'' SIG '' rNVhLdML09eu1dIPhp24lFtnWTYNaFTOfMqFa3Ab8KDK
'' SIG '' icmp0AthRNZVg0BPAL58+B0UcoBGKzS9jscwOTu1JmNl
'' SIG '' isOKkVUVkSJ5Fo/ctfDSPdCTVaIXXF7l40k1cM/X2O0J
'' SIG '' dAS97T78lYjtw/PybuzX5shxBh/RqTPvCyAhIxBVKfN/
'' SIG '' hfs4CIoFaqWJ0r/8SB1CGsyyIcPfEgMo8ceq1w5Zo0Jf
'' SIG '' nyFi6Guo+z3LPFl/exQaRubErsAUTfyBY5/5liyvjAgy
'' SIG '' DYnEB8vHO7c7Fg2tGd5hGgYs+AOoWx24+XcyxpUkAajD
'' SIG '' hky9Dl+8JZTjts6BcT9sYTmOodk/SgIwggdxMIIFWaAD
'' SIG '' AgECAhMzAAAAFcXna54Cm0mZAAAAAAAVMA0GCSqGSIb3
'' SIG '' DQEBCwUAMIGIMQswCQYDVQQGEwJVUzETMBEGA1UECBMK
'' SIG '' V2FzaGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwG
'' SIG '' A1UEChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMTIwMAYD
'' SIG '' VQQDEylNaWNyb3NvZnQgUm9vdCBDZXJ0aWZpY2F0ZSBB
'' SIG '' dXRob3JpdHkgMjAxMDAeFw0yMTA5MzAxODIyMjVaFw0z
'' SIG '' MDA5MzAxODMyMjVaMHwxCzAJBgNVBAYTAlVTMRMwEQYD
'' SIG '' VQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25k
'' SIG '' MR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24x
'' SIG '' JjAkBgNVBAMTHU1pY3Jvc29mdCBUaW1lLVN0YW1wIFBD
'' SIG '' QSAyMDEwMIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIIC
'' SIG '' CgKCAgEA5OGmTOe0ciELeaLL1yR5vQ7VgtP97pwHB9Kp
'' SIG '' bE51yMo1V/YBf2xK4OK9uT4XYDP/XE/HZveVU3Fa4n5K
'' SIG '' Wv64NmeFRiMMtY0Tz3cywBAY6GB9alKDRLemjkZrBxTz
'' SIG '' xXb1hlDcwUTIcVxRMTegCjhuje3XD9gmU3w5YQJ6xKr9
'' SIG '' cmmvHaus9ja+NSZk2pg7uhp7M62AW36MEBydUv626GIl
'' SIG '' 3GoPz130/o5Tz9bshVZN7928jaTjkY+yOSxRnOlwaQ3K
'' SIG '' Ni1wjjHINSi947SHJMPgyY9+tVSP3PoFVZhtaDuaRr3t
'' SIG '' pK56KTesy+uDRedGbsoy1cCGMFxPLOJiss254o2I5Jas
'' SIG '' AUq7vnGpF1tnYN74kpEeHT39IM9zfUGaRnXNxF803RKJ
'' SIG '' 1v2lIH1+/NmeRd+2ci/bfV+AutuqfjbsNkz2K26oElHo
'' SIG '' vwUDo9Fzpk03dJQcNIIP8BDyt0cY7afomXw/TNuvXsLz
'' SIG '' 1dhzPUNOwTM5TI4CvEJoLhDqhFFG4tG9ahhaYQFzymei
'' SIG '' XtcodgLiMxhy16cg8ML6EgrXY28MyTZki1ugpoMhXV8w
'' SIG '' dJGUlNi5UPkLiWHzNgY1GIRH29wb0f2y1BzFa/ZcUlFd
'' SIG '' Etsluq9QBXpsxREdcu+N+VLEhReTwDwV2xo3xwgVGD94
'' SIG '' q0W29R6HXtqPnhZyacaue7e3PmriLq0CAwEAAaOCAd0w
'' SIG '' ggHZMBIGCSsGAQQBgjcVAQQFAgMBAAEwIwYJKwYBBAGC
'' SIG '' NxUCBBYEFCqnUv5kxJq+gpE8RjUpzxD/LwTuMB0GA1Ud
'' SIG '' DgQWBBSfpxVdAF5iXYP05dJlpxtTNRnpcjBcBgNVHSAE
'' SIG '' VTBTMFEGDCsGAQQBgjdMg30BATBBMD8GCCsGAQUFBwIB
'' SIG '' FjNodHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vcGtpb3Bz
'' SIG '' L0RvY3MvUmVwb3NpdG9yeS5odG0wEwYDVR0lBAwwCgYI
'' SIG '' KwYBBQUHAwgwGQYJKwYBBAGCNxQCBAweCgBTAHUAYgBD
'' SIG '' AEEwCwYDVR0PBAQDAgGGMA8GA1UdEwEB/wQFMAMBAf8w
'' SIG '' HwYDVR0jBBgwFoAU1fZWy4/oolxiaNE9lJBb186aGMQw
'' SIG '' VgYDVR0fBE8wTTBLoEmgR4ZFaHR0cDovL2NybC5taWNy
'' SIG '' b3NvZnQuY29tL3BraS9jcmwvcHJvZHVjdHMvTWljUm9v
'' SIG '' Q2VyQXV0XzIwMTAtMDYtMjMuY3JsMFoGCCsGAQUFBwEB
'' SIG '' BE4wTDBKBggrBgEFBQcwAoY+aHR0cDovL3d3dy5taWNy
'' SIG '' b3NvZnQuY29tL3BraS9jZXJ0cy9NaWNSb29DZXJBdXRf
'' SIG '' MjAxMC0wNi0yMy5jcnQwDQYJKoZIhvcNAQELBQADggIB
'' SIG '' AJ1VffwqreEsH2cBMSRb4Z5yS/ypb+pcFLY+TkdkeLEG
'' SIG '' k5c9MTO1OdfCcTY/2mRsfNB1OW27DzHkwo/7bNGhlBgi
'' SIG '' 7ulmZzpTTd2YurYeeNg2LpypglYAA7AFvonoaeC6Ce57
'' SIG '' 32pvvinLbtg/SHUB2RjebYIM9W0jVOR4U3UkV7ndn/OO
'' SIG '' PcbzaN9l9qRWqveVtihVJ9AkvUCgvxm2EhIRXT0n4ECW
'' SIG '' OKz3+SmJw7wXsFSFQrP8DJ6LGYnn8AtqgcKBGUIZUnWK
'' SIG '' NsIdw2FzLixre24/LAl4FOmRsqlb30mjdAy87JGA0j3m
'' SIG '' Sj5mO0+7hvoyGtmW9I/2kQH2zsZ0/fZMcm8Qq3UwxTSw
'' SIG '' ethQ/gpY3UA8x1RtnWN0SCyxTkctwRQEcb9k+SS+c23K
'' SIG '' jgm9swFXSVRk2XPXfx5bRAGOWhmRaw2fpCjcZxkoJLo4
'' SIG '' S5pu+yFUa2pFEUep8beuyOiJXk+d0tBMdrVXVAmxaQFE
'' SIG '' fnyhYWxz/gq77EFmPWn9y8FBSX5+k77L+DvktxW/tM4+
'' SIG '' pTFRhLy/AsGConsXHRWJjXD+57XQKBqJC4822rpM+Zv/
'' SIG '' Cuk0+CQ1ZyvgDbjmjJnW4SLq8CdCPSWU5nR0W2rRnj7t
'' SIG '' fqAxM328y+l7vzhwRNGQ8cirOoo6CGJ/2XBjU02N7oJt
'' SIG '' pQUQwXEGahC0HVUzWLOhcGbyoYIDVjCCAj4CAQEwggEB
'' SIG '' oYHZpIHWMIHTMQswCQYDVQQGEwJVUzETMBEGA1UECBMK
'' SIG '' V2FzaGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwG
'' SIG '' A1UEChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMS0wKwYD
'' SIG '' VQQLEyRNaWNyb3NvZnQgSXJlbGFuZCBPcGVyYXRpb25z
'' SIG '' IExpbWl0ZWQxJzAlBgNVBAsTHm5TaGllbGQgVFNTIEVT
'' SIG '' TjoyRDFBLTA1RTAtRDk0NzElMCMGA1UEAxMcTWljcm9z
'' SIG '' b2Z0IFRpbWUtU3RhbXAgU2VydmljZaIjCgEBMAcGBSsO
'' SIG '' AwIaAxUAoj0WtVVQUNSKoqtrjinRAsBUdoOggYMwgYCk
'' SIG '' fjB8MQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2FzaGlu
'' SIG '' Z3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UEChMV
'' SIG '' TWljcm9zb2Z0IENvcnBvcmF0aW9uMSYwJAYDVQQDEx1N
'' SIG '' aWNyb3NvZnQgVGltZS1TdGFtcCBQQ0EgMjAxMDANBgkq
'' SIG '' hkiG9w0BAQsFAAIFAOrizd8wIhgPMjAyNDExMTYwNzUz
'' SIG '' MDNaGA8yMDI0MTExNzA3NTMwM1owdDA6BgorBgEEAYRZ
'' SIG '' CgQBMSwwKjAKAgUA6uLN3wIBADAHAgEAAgI2KTAHAgEA
'' SIG '' AgITMjAKAgUA6uQfXwIBADA2BgorBgEEAYRZCgQCMSgw
'' SIG '' JjAMBgorBgEEAYRZCgMCoAowCAIBAAIDB6EgoQowCAIB
'' SIG '' AAIDAYagMA0GCSqGSIb3DQEBCwUAA4IBAQCNTKm3mUq5
'' SIG '' fCtR955W1Ni8qtq3XlZ+xEqlA5QWcvz4X2jElAwMowjU
'' SIG '' YrrnstySR7r27Fm3yh8tM1ATdbCCrxtl8qQBrLnVtIiR
'' SIG '' ZRWvKY/kL0Qjo2f+IBu7ar18kncZS21kriaMJqgxtTxJ
'' SIG '' 9GQ/Aoz+9arEj7mE6pcp+17c6pisZNKQ+WI2ca08AccB
'' SIG '' uSx4VkstwgMJpXj/B1YXdaisCaMxRrFFYZA3JLV/Bcyo
'' SIG '' 6ChsZYjD2TYgAuZkPPqsaBMcqCJsxmCCIjOoLj2rIfu6
'' SIG '' e1WK0OggYZAYOTlz0BF2/dRu+m9gqe8mJD9WUXHCwXVQ
'' SIG '' lc2a87nEniWvLbuoEERL31sSMYIEDTCCBAkCAQEwgZMw
'' SIG '' fDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0
'' SIG '' b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1p
'' SIG '' Y3Jvc29mdCBDb3Jwb3JhdGlvbjEmMCQGA1UEAxMdTWlj
'' SIG '' cm9zb2Z0IFRpbWUtU3RhbXAgUENBIDIwMTACEzMAAAH9
'' SIG '' c/loWs0MYe0AAQAAAf0wDQYJYIZIAWUDBAIBBQCgggFK
'' SIG '' MBoGCSqGSIb3DQEJAzENBgsqhkiG9w0BCRABBDAvBgkq
'' SIG '' hkiG9w0BCQQxIgQgSl+DPPBmlQJpjxD/ZU6iqWOUCG5f
'' SIG '' NTxPBJ7xKR8YlKIwgfoGCyqGSIb3DQEJEAIvMYHqMIHn
'' SIG '' MIHkMIG9BCCAKEgNyUowvIfx/eDfYSupHkeF1p6GFwjK
'' SIG '' Bs8lRB4NRzCBmDCBgKR+MHwxCzAJBgNVBAYTAlVTMRMw
'' SIG '' EQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRt
'' SIG '' b25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRp
'' SIG '' b24xJjAkBgNVBAMTHU1pY3Jvc29mdCBUaW1lLVN0YW1w
'' SIG '' IFBDQSAyMDEwAhMzAAAB/XP5aFrNDGHtAAEAAAH9MCIE
'' SIG '' IMPYrlCagajZG6D/dhIxPw5Acz5o46VX+r1JBKgvwYrw
'' SIG '' MA0GCSqGSIb3DQEBCwUABIICAJBAkAI1hLu2LIm/0Y32
'' SIG '' nsQoG5hts7eP0WIGxTHJ10Twn6l0sxHrAFn1IbPW6HCE
'' SIG '' n31zfUIRvRPtZcrrKw22qP49fLrCINe/0HCoErpaljVb
'' SIG '' U7CyNKtzP4A6iQgrk4HCPH85xon6AEPN/7QG/sbFPqKT
'' SIG '' huz6HVSDzmF5KnHd8sJot2sTuuXOKHZ3bsDh3Kmj77yi
'' SIG '' oAXiND8kTpMmh0eeOu5STox5KaF6hE6o5RNBsY19sWLD
'' SIG '' TQtx3107lzihhiubVxbLwbNrkMfuzHIkM/3M271OlV5W
'' SIG '' tc6wE8URX7tyGEFA1rxpwYzjKb9Bl2qVddsSshyhuzje
'' SIG '' gBfRGYVsOS2BlCwTDOXXiChKHP8akKv8HVHYDDTyDJ6c
'' SIG '' bB1Yijyk1Ls1zIjZrfsYJ2nXRepXqC5GM7zYXFMzwS5q
'' SIG '' WHhlp+w0Qugf2V0ivexNl32iPZLZZ32jpNDFu5yPitTQ
'' SIG '' DEfVLN6H7XWG8HB2RYJ/ozBrn5iIM1HS8g8IW0fgO96q
'' SIG '' qnRkHOFfqidp99gX5KNiCiRj/HSnZqnw+6MytJ0jj77g
'' SIG '' +v8Qi15+Oooj8gtt3AZlckfVAyWuEc8gDMuGmc2wyiJC
'' SIG '' MmtFwBH+toExoRb7wWsLknwARYt4F6na1rGBoso+PFU2
'' SIG '' b9U4YKarkz2bzlIxdDm+WNhK6mDCCwcSPYhZbZ4gN0Va
'' SIG '' 3V+W
'' SIG '' End signature block
