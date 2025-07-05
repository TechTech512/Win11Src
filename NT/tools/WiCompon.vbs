' Windows Installer utility to list component composition of an MSI database
' For use with Windows Scripting Host, CScript.exe or WScript.exe
' Copyright (c) Microsoft Corporation. All rights reserved.
' Demonstrates the various tables having foreign keys to the Component table
'
Option Explicit
Public isGUI, installer, database, message, compParam  'global variables access across functions

Const msiOpenDatabaseModeReadOnly     = 0

' Check if run from GUI script host, in order to modify display
If UCase(Mid(Wscript.FullName, Len(Wscript.Path) + 2, 1)) = "W" Then isGUI = True

' Show help if no arguments or if argument contains ?
Dim argCount:argCount = Wscript.Arguments.Count
If argCount > 0 Then If InStr(1, Wscript.Arguments(0), "?", vbTextCompare) > 0 Then argCount = 0
If argCount = 0 Then
	Wscript.Echo "Windows Installer utility to list component composition in an install database." &_
		vbLf & " The 1st argument is the path to an install database, relative or complete path" &_
		vbLf & " The 2nd argument is the name of the component (primary key of Component table)" &_
		vbLf & " If the 2nd argument is not present, the names of all components will be listed" &_
		vbLf & " If the 2nd argument is a ""*"", the composition of all components will be listed" &_
		vbLf & " Large databases or components are better displayed using CScript than WScript." &_
		vbLf & " Note: The name of the component, if provided,  is case-sensitive" &_
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

If argCount = 1 Then  'If no component specified, then simply list components
	ListComponents False
	ShowOutput "Components for " & databasePath, message
ElseIf Left(Wscript.Arguments(1), 1) = "*" Then 'List all components
	ListComponents True
Else
	QueryComponent Wscript.Arguments(1) 
End If
Wscript.Quit 0

' List all table rows referencing a given component
Function QueryComponent(component)
	' Get component info and format output header
	Dim view, record, header, componentId
	Set view = database.OpenView("SELECT `ComponentId` FROM `Component` WHERE `Component` = ?") : CheckError
	Set compParam = installer.CreateRecord(1)
	compParam.StringData(1) = component
	view.Execute compParam : CheckError
	Set record = view.Fetch : CheckError
	Set view = Nothing
	If record Is Nothing Then Fail "Component not in database: " & component
	componentId = record.StringData(1)
	header = "Component: "& component & "  ComponentId = " & componentId

	' List of tables with foreign keys to Component table - with subsets of columns to display
	DoQuery "FeatureComponents","Feature_"                           '
	DoQuery "PublishComponent", "ComponentId,Qualifier"              'AppData,Feature
	DoQuery "File",             "File,Sequence,FileName,Version"     'FileSize,Language,Attributes
	DoQuery "SelfReg,File",     "File_"                              'Cost
	DoQuery "BindImage,File",   "File_"                              'Path
	DoQuery "Font,File",        "File_,FontTitle"                    '
	DoQuery "Patch,File",       "File_"                              'Sequence,PatchSize,Attributes,Header
	DoQuery "DuplicateFile",    "FileKey,File_,DestName"             'DestFolder
	DoQuery "MoveFile",         "FileKey,SourceName,DestName"        'SourceFolder,DestFolder,Options
	DoQuery "RemoveFile",       "FileKey,FileName,DirProperty"       'InstallMode
	DoQuery "IniFile",          "IniFile,FileName,Section,Key"       'Value,Action
	DoQuery "RemoveIniFile",    "RemoveIniFile,FileName,Section,Key" 'Value,Action
	DoQuery "Registry",         "Registry,Root,Key,Name"             'Value
	DoQuery "RemoveRegistry",   "RemoveRegistry,Root,Key,Name"       '
	DoQuery "Shortcut",         "Shortcut,Directory_,Name,Target"    'Arguments,Description,Hotkey,Icon_,IconIndex,ShowCmd,WkDir
	DoQuery "Class",            "CLSID,Description"                  'Context,ProgId_Default,AppId_,FileType,Mask,Icon_,IconIndex,DefInprocHandler,Argument,Feature_
	DoQuery "ProgId,Class",     "Class_,ProgId,Description"          'ProgId_Parent,Icon_IconIndex,Insertable
	DoQuery "Extension",        "Extension,ProgId_"                  'MIME_,Feature_
	DoQuery "Verb,Extension",   "Extension_,Verb"                    'Sequence,Command.Argument
	DoQuery "MIME,Extension",   "Extension_,ContentType"             'CLSID
	DoQuery "TypeLib",          "LibID,Language,Version,Description" 'Directory_,Feature_,Cost
	DoQuery "CreateFolder",     "Directory_"                         ' 
	DoQuery "Environment",      "Environment,Name"                   'Value
	DoQuery "ODBCDriver",       "Driver,Description"                 'File_,File_Setup
	DoQuery "ODBCAttribute,ODBCDriver", "Driver_,Attribute,Value" '
	DoQuery "ODBCTranslator",   "Translator,Description"             'File_,File_Setup
	DoQuery "ODBCDataSource",   "DataSource,Description,DriverDescription" 'Registration
	DoQuery "ODBCSourceAttribute,ODBCDataSource", "DataSource_,Attribute,Value" '
	DoQuery "ServiceControl",   "ServiceControl,Name,Event"          'Arguments,Wait
	DoQuery "ServiceInstall",   "ServiceInstall,Name,DisplayName"    'ServiceType,StartType,ErrorControl,LoadOrderGroup,Dependencies,StartName,Password
	DoQuery "ReserveCost",      "ReserveKey,ReserveFolder"           'ReserveLocal,ReserveSource

	QueryComponent = ShowOutput(header, message)
	message = Empty
End Function

' List all components in database
Sub ListComponents(queryAll)
	Dim view, record, component
	Set view = database.OpenView("SELECT `Component`,`ComponentId` FROM `Component`") : CheckError
	view.Execute : CheckError
	Do
		Set record = view.Fetch : CheckError
		If record Is Nothing Then Exit Do
		component = record.StringData(1)
		If queryAll Then
			If QueryComponent(component) = vbCancel Then Exit Sub
		Else
			If Not IsEmpty(message) Then message = message & vbLf
			message = message & component
		End If
	Loop
End Sub

' Perform a join to query table rows linked to a given component, delimiting and qualifying names to prevent conflicts
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
	query = "SELECT `" & columnList & "` FROM `" & tableList & "` WHERE `Component_` = ?" & query
	If database.TablePersistent(table) <> 1 Then Exit Sub
	Set view = database.OpenView(query) : CheckError
	view.Execute compParam : CheckError
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
'' SIG '' MIIlxgYJKoZIhvcNAQcCoIIltzCCJbMCAQExDzANBglg
'' SIG '' hkgBZQMEAgEFADB3BgorBgEEAYI3AgEEoGkwZzAyBgor
'' SIG '' BgEEAYI3AgEeMCQCAQEEEE7wKRaZJ7VNj+Ws4Q8X66sC
'' SIG '' AQACAQACAQACAQACAQAwMTANBglghkgBZQMEAgEFAAQg
'' SIG '' WoLLQA6rHA8fttRtGpZpVGF985uNg+TIhlmzKb0W2sOg
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
'' SIG '' jgd7JXFEqwZq5tTG3yOalnXFMYIZtzCCGbMCAQEwgZUw
'' SIG '' fjELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0
'' SIG '' b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1p
'' SIG '' Y3Jvc29mdCBDb3Jwb3JhdGlvbjEoMCYGA1UEAxMfTWlj
'' SIG '' cm9zb2Z0IENvZGUgU2lnbmluZyBQQ0EgMjAxMAITMwAA
'' SIG '' BVfPkN3H0cCIjAAAAAAFVzANBglghkgBZQMEAgEFAKCB
'' SIG '' xjAZBgkqhkiG9w0BCQMxDAYKKwYBBAGCNwIBBDAcBgor
'' SIG '' BgEEAYI3AgELMQ4wDAYKKwYBBAGCNwIBFTAvBgkqhkiG
'' SIG '' 9w0BCQQxIgQgU+H2zhtPO/bs1TiBZyzIPq79ccTVGaHm
'' SIG '' w1wCd9WpjFgwWgYKKwYBBAGCNwIBDDFMMEqgJIAiAE0A
'' SIG '' aQBjAHIAbwBzAG8AZgB0ACAAVwBpAG4AZABvAHcAc6Ei
'' SIG '' gCBodHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vd2luZG93
'' SIG '' czANBgkqhkiG9w0BAQEFAASCAQCFusOXyJuMLqX/kg30
'' SIG '' Kz56fmatLAPRznvQhMho2b9CMzCVLipyM97yw10JGDOc
'' SIG '' iFd/hAsHWWd/KwsTOxKtxZIVfVZmSocDjvkUG9ShOziQ
'' SIG '' 9cPrh+wzTVA7lTyQkfLmTbdOqC7zoelnhd6wZ0fi703F
'' SIG '' 5FbSRZgBGdtmlJUWMA9LUhuSyBVORy7cLiSN6gp7m6oC
'' SIG '' xxays9+yVHEfdc8TAqWggcdYOvwYw5fWbJDjvljQkMmu
'' SIG '' JYbNKumVvJ3XzWliHjzoAM6tp+8UnLe2tRGLGr6r48Ve
'' SIG '' RmqxHFRGUxuFDSRVOaw4BPymtahch4+r3mEoOulPLY93
'' SIG '' dFzI3BnhUgIN5roZoYIXKTCCFyUGCisGAQQBgjcDAwEx
'' SIG '' ghcVMIIXEQYJKoZIhvcNAQcCoIIXAjCCFv4CAQMxDzAN
'' SIG '' BglghkgBZQMEAgEFADCCAVkGCyqGSIb3DQEJEAEEoIIB
'' SIG '' SASCAUQwggFAAgEBBgorBgEEAYRZCgMBMDEwDQYJYIZI
'' SIG '' AWUDBAIBBQAEINx30dVL0aTDiiq5prGn05EAYufZ+Vr6
'' SIG '' f471VlIv/rKKAgZmxh65kE8YEzIwMjQwOTA1MDkxNzEy
'' SIG '' LjI0OVowBIACAfSggdikgdUwgdIxCzAJBgNVBAYTAlVT
'' SIG '' MRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdS
'' SIG '' ZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9y
'' SIG '' YXRpb24xLTArBgNVBAsTJE1pY3Jvc29mdCBJcmVsYW5k
'' SIG '' IE9wZXJhdGlvbnMgTGltaXRlZDEmMCQGA1UECxMdVGhh
'' SIG '' bGVzIFRTUyBFU046RkM0MS00QkQ0LUQyMjAxJTAjBgNV
'' SIG '' BAMTHE1pY3Jvc29mdCBUaW1lLVN0YW1wIFNlcnZpY2Wg
'' SIG '' ghF4MIIHJzCCBQ+gAwIBAgITMwAAAeKZmZXx3OMg6wAB
'' SIG '' AAAB4jANBgkqhkiG9w0BAQsFADB8MQswCQYDVQQGEwJV
'' SIG '' UzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMH
'' SIG '' UmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBv
'' SIG '' cmF0aW9uMSYwJAYDVQQDEx1NaWNyb3NvZnQgVGltZS1T
'' SIG '' dGFtcCBQQ0EgMjAxMDAeFw0yMzEwMTIxOTA3MjVaFw0y
'' SIG '' NTAxMTAxOTA3MjVaMIHSMQswCQYDVQQGEwJVUzETMBEG
'' SIG '' A1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9u
'' SIG '' ZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBvcmF0aW9u
'' SIG '' MS0wKwYDVQQLEyRNaWNyb3NvZnQgSXJlbGFuZCBPcGVy
'' SIG '' YXRpb25zIExpbWl0ZWQxJjAkBgNVBAsTHVRoYWxlcyBU
'' SIG '' U1MgRVNOOkZDNDEtNEJENC1EMjIwMSUwIwYDVQQDExxN
'' SIG '' aWNyb3NvZnQgVGltZS1TdGFtcCBTZXJ2aWNlMIICIjAN
'' SIG '' BgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAtWO1mFX6
'' SIG '' QWZvxwpCmDabOKwOVEj3vwZvZqYa9sCYJ3TglUZ5N79A
'' SIG '' bMzwptCswOiXsMLuNLTcmRys+xaL1alXCwhyRFDwCRfW
'' SIG '' J0Eb0eHIKykBq9+6/PnmSGXtus9DHsf31QluwTfAyamY
'' SIG '' lqw9amAXTnNmW+lZANQsNwhjKXmVcjgdVnk3oxLFY7zP
'' SIG '' Baviv3GQyZRezsgLEMmvlrf1JJ48AlEjLOdohzRbNnow
'' SIG '' VxNHMss3I8ETgqtW/UsV33oU3EDPCd61J4+DzwSZF7Ov
'' SIG '' ZPcdMUSWd4lfJBh3phDt4IhzvKWVahjTcISD2CGiun2p
'' SIG '' QpwFR8VxLhcSV/cZIRGeXMmwruz9kY9Th1odPaNYahiF
'' SIG '' rZAI6aSCM6YEUKpAUXAWaw+tmPh5CzNjGrhzgeo+dS7i
'' SIG '' FPhqqm9Rneog5dt3JTjak0v3dyfSs9NOV45Sw5BuC+VF
'' SIG '' 22EUIF6nF9vqduynd9xlo8F9Nu1dVryctC4wIGrJ+x5u
'' SIG '' 6qdvCP6UdB+oqmK+nJ3soJYAKiPvxdTBirLUfJidK1OZ
'' SIG '' 7hP28rq7Y78pOF9E54keJKDjjKYWP7fghwUSE+iBoq80
'' SIG '' 2xNWbhBuqmELKSevAHKqisEIsfpuWVG0kwnCa7sZF1NC
'' SIG '' wjHYcwqqmES2lKbXPe58BJ0+uA+GxAhEWQdka6KEvUmO
'' SIG '' Pgu7cJsCaFrSU6sCAwEAAaOCAUkwggFFMB0GA1UdDgQW
'' SIG '' BBREhA4R2r7tB2yWm0mIJE2leAnaBTAfBgNVHSMEGDAW
'' SIG '' gBSfpxVdAF5iXYP05dJlpxtTNRnpcjBfBgNVHR8EWDBW
'' SIG '' MFSgUqBQhk5odHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20v
'' SIG '' cGtpb3BzL2NybC9NaWNyb3NvZnQlMjBUaW1lLVN0YW1w
'' SIG '' JTIwUENBJTIwMjAxMCgxKS5jcmwwbAYIKwYBBQUHAQEE
'' SIG '' YDBeMFwGCCsGAQUFBzAChlBodHRwOi8vd3d3Lm1pY3Jv
'' SIG '' c29mdC5jb20vcGtpb3BzL2NlcnRzL01pY3Jvc29mdCUy
'' SIG '' MFRpbWUtU3RhbXAlMjBQQ0ElMjAyMDEwKDEpLmNydDAM
'' SIG '' BgNVHRMBAf8EAjAAMBYGA1UdJQEB/wQMMAoGCCsGAQUF
'' SIG '' BwMIMA4GA1UdDwEB/wQEAwIHgDANBgkqhkiG9w0BAQsF
'' SIG '' AAOCAgEA5FREMatVFNue6V+yDZxOzLKHthe+FVTs1kyQ
'' SIG '' hMBBiwUQ9WC9K+ILKWvlqneRrvpjPS3/qXG5zMjrDu1e
'' SIG '' ryfhbFRSByPnACGc2iuGcPyWNiptyTft+CBgrf7ATAuE
'' SIG '' /U8YLm29crTFiiZTWdT6Vc7L1lGdKEj8dl0WvDayuC2x
'' SIG '' tajD04y4ANLmWDuiStdrZ1oI4afG5oPUg77rkTuq/Y7R
'' SIG '' bSwaPsBZ06M12l7E+uykvYoRw4x4lWaST87SBqeEXPMc
'' SIG '' CdaO01ad5TXVZDoHG/w6k3V9j3DNCiLJyC844kz3eh3n
'' SIG '' kQZ5fF8Xxuh8tWVQTfMiKShJ537yzrU0M/7H1EzJrabA
'' SIG '' r9izXF28OVlMed0gqyx+a7e+79r4EV/a4ijJxVO8FCm/
'' SIG '' 92tEkPrx6jjTWaQJEWSbL/4GZCVGvHatqmoC7mTQ16/6
'' SIG '' JR0FQqZf+I5opnvm+5CDuEKIEDnEiblkhcNKVfjvDAVq
'' SIG '' vf8GBPCe0yr2trpBEB5L+j+5haSa+q8TwCrfxCYqBOIG
'' SIG '' dZJL+5U9xocTICufIWHkb6p4IaYvjgx8ScUSHFzexo+Z
'' SIG '' eF7oyFKAIgYlRkMDvffqdAPx+fjLrnfgt6X4u5PkXlsW
'' SIG '' 3SYvB34fkbEbM5tmab9zekRa0e/W6Dt1L8N+tx3WyfYT
'' SIG '' iCThbUvWN1EFsr3HCQybBj4Idl4xK8EwggdxMIIFWaAD
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
'' SIG '' pQUQwXEGahC0HVUzWLOhcGbyoYIC1DCCAj0CAQEwggEA
'' SIG '' oYHYpIHVMIHSMQswCQYDVQQGEwJVUzETMBEGA1UECBMK
'' SIG '' V2FzaGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwG
'' SIG '' A1UEChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMS0wKwYD
'' SIG '' VQQLEyRNaWNyb3NvZnQgSXJlbGFuZCBPcGVyYXRpb25z
'' SIG '' IExpbWl0ZWQxJjAkBgNVBAsTHVRoYWxlcyBUU1MgRVNO
'' SIG '' OkZDNDEtNEJENC1EMjIwMSUwIwYDVQQDExxNaWNyb3Nv
'' SIG '' ZnQgVGltZS1TdGFtcCBTZXJ2aWNloiMKAQEwBwYFKw4D
'' SIG '' AhoDFQAWm5lp+nRuekl0iF+IHV3ylOiGb6CBgzCBgKR+
'' SIG '' MHwxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5n
'' SIG '' dG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVN
'' SIG '' aWNyb3NvZnQgQ29ycG9yYXRpb24xJjAkBgNVBAMTHU1p
'' SIG '' Y3Jvc29mdCBUaW1lLVN0YW1wIFBDQSAyMDEwMA0GCSqG
'' SIG '' SIb3DQEBBQUAAgUA6oO6PTAiGA8yMDI0MDkwNTEzMDM1
'' SIG '' N1oYDzIwMjQwOTA2MTMwMzU3WjB0MDoGCisGAQQBhFkK
'' SIG '' BAExLDAqMAoCBQDqg7o9AgEAMAcCAQACAgGEMAcCAQAC
'' SIG '' AhFUMAoCBQDqhQu9AgEAMDYGCisGAQQBhFkKBAIxKDAm
'' SIG '' MAwGCisGAQQBhFkKAwKgCjAIAgEAAgMHoSChCjAIAgEA
'' SIG '' AgMBhqAwDQYJKoZIhvcNAQEFBQADgYEALhidevvoTU97
'' SIG '' BYDTNQNroQA45xs1VUm10OX0CWm5caLq2MNqN6+2pynI
'' SIG '' 2fYaFmStgbQT4nEe2lgtXUcrj0sGZqQkFWrGHHB63FHi
'' SIG '' pcF/7Ymq4rYk7h+J6wuq7yvMNtSxVaWWC6wghhAOVgdC
'' SIG '' wkkNas9TPJkz0t/fY9ZJa2U8K6oxggQNMIIECQIBATCB
'' SIG '' kzB8MQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2FzaGlu
'' SIG '' Z3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UEChMV
'' SIG '' TWljcm9zb2Z0IENvcnBvcmF0aW9uMSYwJAYDVQQDEx1N
'' SIG '' aWNyb3NvZnQgVGltZS1TdGFtcCBQQ0EgMjAxMAITMwAA
'' SIG '' AeKZmZXx3OMg6wABAAAB4jANBglghkgBZQMEAgEFAKCC
'' SIG '' AUowGgYJKoZIhvcNAQkDMQ0GCyqGSIb3DQEJEAEEMC8G
'' SIG '' CSqGSIb3DQEJBDEiBCCcA4wCoB9FXImd84bO7XlFSr2Q
'' SIG '' CqCN08fshhcraSQEpzCB+gYLKoZIhvcNAQkQAi8xgeow
'' SIG '' gecwgeQwgb0EICuJKkoQ/Sa4xsFQRM4Ogvh3ktToj9uO
'' SIG '' 5whmQ4kIj3//MIGYMIGApH4wfDELMAkGA1UEBhMCVVMx
'' SIG '' EzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1Jl
'' SIG '' ZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3Jh
'' SIG '' dGlvbjEmMCQGA1UEAxMdTWljcm9zb2Z0IFRpbWUtU3Rh
'' SIG '' bXAgUENBIDIwMTACEzMAAAHimZmV8dzjIOsAAQAAAeIw
'' SIG '' IgQgTTQuXRya4zlUII52IYE/BeogiPbCvsPXb0v5cx71
'' SIG '' GaYwDQYJKoZIhvcNAQELBQAEggIAo9Px202FBGpmnV1n
'' SIG '' 4jvLTdVl8jyIggf88hPv0waIHp67BsIUeTxYidlwtMlS
'' SIG '' vYkhC+CWQZGZ0Fu8keJ5xstj4yP/0flDpnjLhtbIYice
'' SIG '' i9fkvxiFR+TO6Zsv294bVbEWZWVruEVcB2olF+zcMkHC
'' SIG '' ia3UcCzImPHA20mqCvcEk6YW1U6CZ1fw8ADNMaEbnkKE
'' SIG '' eZS0BGMESan0Fy3tx+QGzBDPgY1tOCkcV/kV8hTlRXbl
'' SIG '' 9tjn9B4W1rszQHDN6Mkh+RjhmdI8d/twkPLI8LrSQMO7
'' SIG '' 5g2C1jOHXx07wJ+o4CcLz2BEdSoBLWFwIXMzUC5mAXG0
'' SIG '' TWCCAH+dZCClQbWdkkTc5b2hAdguSxRpC9oA3qzpu/cw
'' SIG '' LswYLHckxlsKbpS9VDpl0dSM6S7TjlwHRsGqaT3sKIPN
'' SIG '' YtJqiZz/yelvwWylq3OZ3DeVxWvNPZ3GCxatq9xIBFZG
'' SIG '' NxG6z3Chn0Bwxd9R8iplxteeFk0XbXRl7BaebXVQY1Vr
'' SIG '' SNpTvV0OH/gCjOAipVm548lfL/CCQO8A3OQ/oIO7Z+Kr
'' SIG '' HVbe3fJ3sBlAaYA8AL1w30kIAylnFJpB++SGLa2riu5j
'' SIG '' JvfVme8/S6BNeHDz18xrP/ih9V53gDOW5tYSiFfuXDHS
'' SIG '' EKB5BtHWx37WmuSIkG/2zYdx1X6qKXiyySTx353ynttz
'' SIG '' 4boSRM8=
'' SIG '' End signature block
