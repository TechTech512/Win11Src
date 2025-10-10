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
'' SIG '' MIImXAYJKoZIhvcNAQcCoIImTTCCJkkCAQExDzANBglg
'' SIG '' hkgBZQMEAgEFADB3BgorBgEEAYI3AgEEoGkwZzAyBgor
'' SIG '' BgEEAYI3AgEeMCQCAQEEEE7wKRaZJ7VNj+Ws4Q8X66sC
'' SIG '' AQACAQACAQACAQACAQAwMTANBglghkgBZQMEAgEFAAQg
'' SIG '' WoLLQA6rHA8fttRtGpZpVGF985uNg+TIhlmzKb0W2sOg
'' SIG '' ggt2MIIE/jCCA+agAwIBAgITMwAABaZYEGdLPWx89gAA
'' SIG '' AAAFpjANBgkqhkiG9w0BAQsFADB+MQswCQYDVQQGEwJV
'' SIG '' UzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMH
'' SIG '' UmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBv
'' SIG '' cmF0aW9uMSgwJgYDVQQDEx9NaWNyb3NvZnQgQ29kZSBT
'' SIG '' aWduaW5nIFBDQSAyMDEwMB4XDTI0MDgyMjE5MjU1N1oX
'' SIG '' DTI1MDcwNTE5MjU1N1owdDELMAkGA1UEBhMCVVMxEzAR
'' SIG '' BgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1v
'' SIG '' bmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlv
'' SIG '' bjEeMBwGA1UEAxMVTWljcm9zb2Z0IENvcnBvcmF0aW9u
'' SIG '' MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA
'' SIG '' umNGOR6LgZFk1THk2vW3NrXGKPwTj9NWpe4hKbCXtraY
'' SIG '' afofK2h67UZgcaUTBp5DnWodrzw5nhwZPG9glhUmMj/e
'' SIG '' sHENG8/ya59HX9Nv6jQY2MLVPB+Mr/DwlcBtSyXzUbeV
'' SIG '' nmdmIPF+pxkGFEQLl8KY0bnMmJT5S+s6uuJ12SiACfdu
'' SIG '' OwvE0JOP44cvTsNjy8PCHNnWo3ejNQVmGUz5Nzn31Li3
'' SIG '' W8OWY5J7BKMU2c/lf34/VMjJdrPq7qYeDo2IJsYMkSPN
'' SIG '' ysvnyvokbaWA4oy8ANC7j4m+Ou1WL9JlpDFWr5gN7jfF
'' SIG '' pbUyqsrSK9rfiwNsdmEAA8yXpII+qEzHEwIDAQABo4IB
'' SIG '' fTCCAXkwHwYDVR0lBBgwFgYKKwYBBAGCNz0GAQYIKwYB
'' SIG '' BQUHAwMwHQYDVR0OBBYEFE6cI++DFsHzFTkxX8wNFC39
'' SIG '' UpYtMFQGA1UdEQRNMEukSTBHMS0wKwYDVQQLEyRNaWNy
'' SIG '' b3NvZnQgSXJlbGFuZCBPcGVyYXRpb25zIExpbWl0ZWQx
'' SIG '' FjAUBgNVBAUTDTIzMDg2NSs1MDI3MTIwHwYDVR0jBBgw
'' SIG '' FoAU5vxfe7siAFjkck619CF0IzLm76wwVgYDVR0fBE8w
'' SIG '' TTBLoEmgR4ZFaHR0cDovL2NybC5taWNyb3NvZnQuY29t
'' SIG '' L3BraS9jcmwvcHJvZHVjdHMvTWljQ29kU2lnUENBXzIw
'' SIG '' MTAtMDctMDYuY3JsMFoGCCsGAQUFBwEBBE4wTDBKBggr
'' SIG '' BgEFBQcwAoY+aHR0cDovL3d3dy5taWNyb3NvZnQuY29t
'' SIG '' L3BraS9jZXJ0cy9NaWNDb2RTaWdQQ0FfMjAxMC0wNy0w
'' SIG '' Ni5jcnQwDAYDVR0TAQH/BAIwADANBgkqhkiG9w0BAQsF
'' SIG '' AAOCAQEAtf5mqFjLeh0YfVmslPmBu6sVN3x8lLTdH2eY
'' SIG '' 1PWbITjY6IEh5YtAf6y2pmPjRij7ebZeJB1lLjdzDh7/
'' SIG '' 0Y/XG9vQUNyuczRPjMwAk1FNW+w1n8NG2XHMLy+YDqlF
'' SIG '' UjcuvxVSQXDqLO374g51sl75wnuPb+uoQyMC6s7BdfNm
'' SIG '' 376xWV+7cVq0PfJltFIciJPp8bxp6zjZ6od39acS/QWe
'' SIG '' 8710FpYi9ENiV845KUKBFTq4MX6f84Rtz8CDCa5/Yonk
'' SIG '' NVUerDLKo1b0s6m8b8zMaiu/2s3tj97VP5SvAGz6uhwF
'' SIG '' nnLZXUJR47uy7tNI6dzWNz9XBgm0DSVjkf1nvdX0QDCC
'' SIG '' BnAwggRYoAMCAQICCmEMUkwAAAAAAAMwDQYJKoZIhvcN
'' SIG '' AQELBQAwgYgxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpX
'' SIG '' YXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYD
'' SIG '' VQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xMjAwBgNV
'' SIG '' BAMTKU1pY3Jvc29mdCBSb290IENlcnRpZmljYXRlIEF1
'' SIG '' dGhvcml0eSAyMDEwMB4XDTEwMDcwNjIwNDAxN1oXDTI1
'' SIG '' MDcwNjIwNTAxN1owfjELMAkGA1UEBhMCVVMxEzARBgNV
'' SIG '' BAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQx
'' SIG '' HjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEo
'' SIG '' MCYGA1UEAxMfTWljcm9zb2Z0IENvZGUgU2lnbmluZyBQ
'' SIG '' Q0EgMjAxMDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCC
'' SIG '' AQoCggEBAOkOZFB5Z7XE4/0JAEyelKz3VmjqRNjPxVhP
'' SIG '' qaV2fG1FutM5krSkHvn5ZYLkF9KP/UScCOhlk84sVYS/
'' SIG '' fQjjLiuoQSsYt6JLbklMaxUH3tHSwokecZTNtX9LtK8I
'' SIG '' 2MyI1msXlDqTziY/7Ob+NJhX1R1dSfayKi7VhbtZP/iQ
'' SIG '' tCuDdMorsztG4/BGScEXZlTJHL0dxFViV3L4Z7klIDTe
'' SIG '' XaallV6rKIDN1bKe5QO1Y9OyFMjByIomCll/B+z/Du2A
'' SIG '' EjVMEqa+Ulv1ptrgiwtId9aFR9UQucboqu6Lai0FXGDG
'' SIG '' tCpbnCMcX0XjGhQebzfLGTOAaolNo2pmY3iT1TDPlR8C
'' SIG '' AwEAAaOCAeMwggHfMBAGCSsGAQQBgjcVAQQDAgEAMB0G
'' SIG '' A1UdDgQWBBTm/F97uyIAWORyTrX0IXQjMubvrDAZBgkr
'' SIG '' BgEEAYI3FAIEDB4KAFMAdQBiAEMAQTALBgNVHQ8EBAMC
'' SIG '' AYYwDwYDVR0TAQH/BAUwAwEB/zAfBgNVHSMEGDAWgBTV
'' SIG '' 9lbLj+iiXGJo0T2UkFvXzpoYxDBWBgNVHR8ETzBNMEug
'' SIG '' SaBHhkVodHRwOi8vY3JsLm1pY3Jvc29mdC5jb20vcGtp
'' SIG '' L2NybC9wcm9kdWN0cy9NaWNSb29DZXJBdXRfMjAxMC0w
'' SIG '' Ni0yMy5jcmwwWgYIKwYBBQUHAQEETjBMMEoGCCsGAQUF
'' SIG '' BzAChj5odHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vcGtp
'' SIG '' L2NlcnRzL01pY1Jvb0NlckF1dF8yMDEwLTA2LTIzLmNy
'' SIG '' dDCBnQYDVR0gBIGVMIGSMIGPBgkrBgEEAYI3LgMwgYEw
'' SIG '' PQYIKwYBBQUHAgEWMWh0dHA6Ly93d3cubWljcm9zb2Z0
'' SIG '' LmNvbS9QS0kvZG9jcy9DUFMvZGVmYXVsdC5odG0wQAYI
'' SIG '' KwYBBQUHAgIwNB4yIB0ATABlAGcAYQBsAF8AUABvAGwA
'' SIG '' aQBjAHkAXwBTAHQAYQB0AGUAbQBlAG4AdAAuIB0wDQYJ
'' SIG '' KoZIhvcNAQELBQADggIBABp071dPKXvEFoV4uFDTIvwJ
'' SIG '' nayCl/g0/yosl5US5eS/z7+TyOM0qduBuNweAL7SNW+v
'' SIG '' 5X95lXflAtTx69jNTh4bYaLCWiMa8IyoYlFFZwjjPzwe
'' SIG '' k/gwhRfIOUCm1w6zISnlpaFpjCKTzHSY56FHQ/JTrMAP
'' SIG '' MGl//tIlIG1vYdPfB9XZcgAsaYZ2PVHbpjlIyTdhbQfd
'' SIG '' UxnLp9Zhwr/ig6sP4GubldZ9KFGwiUpRpJpsyLcfShoO
'' SIG '' aanX3MF+0Ulwqratu3JHYxf6ptaipobsqBBEm2O2smmJ
'' SIG '' BsdGhnoYP+jFHSHVe/kCIy3FQcu/HUzIFu+xnH/8IktJ
'' SIG '' im4V46Z/dlvRU3mRhZ3V0ts9czXzPK5UslJHasCqE5XS
'' SIG '' jhHamWdeMoz7N4XR3HWFnIfGWleFwr/dDY+Mmy3rtO7P
'' SIG '' J9O1Xmn6pBYEAackZ3PPTU+23gVWl3r36VJN9HcFT4XG
'' SIG '' 2Avxju1CCdENduMjVngiJja+yrGMbqod5IXaRzNij6TJ
'' SIG '' kTNfcR5Ar5hlySLoQiElihwtYNk3iUGJKhYP12E8lGhg
'' SIG '' Uu/WR5mggEDuFYF3PpzgUxgaUB04lZseZjMTJzkXeIc2
'' SIG '' zk7DX7L1PUdTtuDl2wthPSrXkizON1o+QEIxpB8QCMJW
'' SIG '' nL8kXVECnWp50hfT2sGUjgd7JXFEqwZq5tTG3yOalnXF
'' SIG '' MYIaPjCCGjoCAQEwgZUwfjELMAkGA1UEBhMCVVMxEzAR
'' SIG '' BgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1v
'' SIG '' bmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlv
'' SIG '' bjEoMCYGA1UEAxMfTWljcm9zb2Z0IENvZGUgU2lnbmlu
'' SIG '' ZyBQQ0EgMjAxMAITMwAABaZYEGdLPWx89gAAAAAFpjAN
'' SIG '' BglghkgBZQMEAgEFAKCBxjAZBgkqhkiG9w0BCQMxDAYK
'' SIG '' KwYBBAGCNwIBBDAcBgorBgEEAYI3AgELMQ4wDAYKKwYB
'' SIG '' BAGCNwIBFTAvBgkqhkiG9w0BCQQxIgQgU+H2zhtPO/bs
'' SIG '' 1TiBZyzIPq79ccTVGaHmw1wCd9WpjFgwWgYKKwYBBAGC
'' SIG '' NwIBDDFMMEqgJIAiAE0AaQBjAHIAbwBzAG8AZgB0ACAA
'' SIG '' VwBpAG4AZABvAHcAc6EigCBodHRwOi8vd3d3Lm1pY3Jv
'' SIG '' c29mdC5jb20vd2luZG93czANBgkqhkiG9w0BAQEFAASC
'' SIG '' AQBV9ye7WiWHoIYIjeuOn2cPmTHiOyLX1bpFhlnCCgFq
'' SIG '' jFnymLK4G8gvGxz4Rh3+jVun1NOtBM9P+lusu3U5b4GD
'' SIG '' 09sSxNvQ8Gb+8AZFerH6HQIPEaIuOFDj9C5Ag3cb5j9q
'' SIG '' XcGnrLQjMZbM0sVJnlYsL6Ca5Fyey3DGybMQwuGfkPQq
'' SIG '' 07shHgsbb3nfPNjo/DAPyudoKMawA8Ky9SjEjQhSGhN3
'' SIG '' 2JCG2DXI+ODCUmjuKAWN/BmTDMw0o88W9Fv7PNZYiiXM
'' SIG '' gfm72WQZrcfzqAdwV50Qmj7V9MqzfOSuFeU1lMEt7OlG
'' SIG '' Az+4meqhIIL/x2cBJG7/2JuRuby02TENxUgioYIXsDCC
'' SIG '' F6wGCisGAQQBgjcDAwExghecMIIXmAYJKoZIhvcNAQcC
'' SIG '' oIIXiTCCF4UCAQMxDzANBglghkgBZQMEAgEFADCCAVoG
'' SIG '' CyqGSIb3DQEJEAEEoIIBSQSCAUUwggFBAgEBBgorBgEE
'' SIG '' AYRZCgMBMDEwDQYJYIZIAWUDBAIBBQAEIKbwwAUui3c+
'' SIG '' uadJy1mbXOF8FITDxyrYlQTcFnmei7NuAgZm63ITC94Y
'' SIG '' EzIwMjQxMTE2MDkxNjQzLjI3OVowBIACAfSggdmkgdYw
'' SIG '' gdMxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5n
'' SIG '' dG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVN
'' SIG '' aWNyb3NvZnQgQ29ycG9yYXRpb24xLTArBgNVBAsTJE1p
'' SIG '' Y3Jvc29mdCBJcmVsYW5kIE9wZXJhdGlvbnMgTGltaXRl
'' SIG '' ZDEnMCUGA1UECxMeblNoaWVsZCBUU1MgRVNOOjU1MUEt
'' SIG '' MDVFMC1EOTQ3MSUwIwYDVQQDExxNaWNyb3NvZnQgVGlt
'' SIG '' ZS1TdGFtcCBTZXJ2aWNloIIR/jCCBygwggUQoAMCAQIC
'' SIG '' EzMAAAIB0UVZmBDMQk8AAQAAAgEwDQYJKoZIhvcNAQEL
'' SIG '' BQAwfDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hp
'' SIG '' bmd0b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoT
'' SIG '' FU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEmMCQGA1UEAxMd
'' SIG '' TWljcm9zb2Z0IFRpbWUtU3RhbXAgUENBIDIwMTAwHhcN
'' SIG '' MjQwNzI1MTgzMTIyWhcNMjUxMDIyMTgzMTIyWjCB0zEL
'' SIG '' MAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24x
'' SIG '' EDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jv
'' SIG '' c29mdCBDb3Jwb3JhdGlvbjEtMCsGA1UECxMkTWljcm9z
'' SIG '' b2Z0IElyZWxhbmQgT3BlcmF0aW9ucyBMaW1pdGVkMScw
'' SIG '' JQYDVQQLEx5uU2hpZWxkIFRTUyBFU046NTUxQS0wNUUw
'' SIG '' LUQ5NDcxJTAjBgNVBAMTHE1pY3Jvc29mdCBUaW1lLVN0
'' SIG '' YW1wIFNlcnZpY2UwggIiMA0GCSqGSIb3DQEBAQUAA4IC
'' SIG '' DwAwggIKAoICAQC1at/4fMO7uyTnTLlgeF4IgkbS7FFI
'' SIG '' UVwc16T5N31mbImPbQQSNpTwkMm7mlZzik8CwEjfw0QA
'' SIG '' nVv4oGeVK2pTy69cXiqcRKEN2Lyc+xllCxNkMpYCBQza
'' SIG '' JlM6JYi5lwWzlkLz/4aWtUszmHVj2d8yHkHgOdRA5cyt
'' SIG '' 6YBP0yS9SGDe5piCaouWZjI4OZtriVdkG7XThIsAWxc5
'' SIG '' +X9MuGlOhPjrLuUj2xsj26rf8B6uHdo+LaSce8QRrOKV
'' SIG '' d6ihc0sLB274izqjyRAui5SfcrBRCbRvtpS2y/Vf86A+
'' SIG '' aw4mLrI3cthbIchK+s24isniJg2Ad0EG6ZBgrwuNmZBp
'' SIG '' MoVpzGGZcnSraDNoh/EXbIjAz5X2xCqQeSD9D6JIM2ky
'' SIG '' vqav87CSc4QiMjSDpkw7KaK+kKHMM2g/P2GQreBUdkpb
'' SIG '' s1Xz5QFc3gbRoFfr18pRvEEEvKTZwL4+E6hSOSXpQLz9
'' SIG '' zSG6qPnFfyb5hUiTzV7u3oj5X8TjJdF55mCvQWFio2m9
'' SIG '' OMZxo7ZauQ/leaxhLsi8w8h/gMLIglRlqqgExOgAkkcZ
'' SIG '' F74M+oIeDpuYY+b3sys5a/Xr8KjpL1xAORen28xJJFBZ
'' SIG '' fLgq0mFl+a4PPa+vWPDg16LHC4gMbDWa1X9N1Ij6+ksl
'' SIG '' 9SIuX9v3D+0kH3YEAtBPx7Vgfw2mF06jXELCRwIDAQAB
'' SIG '' o4IBSTCCAUUwHQYDVR0OBBYEFLByr1uWoug8+JWvKb2Y
'' SIG '' WYVZvLJSMB8GA1UdIwQYMBaAFJ+nFV0AXmJdg/Tl0mWn
'' SIG '' G1M1GelyMF8GA1UdHwRYMFYwVKBSoFCGTmh0dHA6Ly93
'' SIG '' d3cubWljcm9zb2Z0LmNvbS9wa2lvcHMvY3JsL01pY3Jv
'' SIG '' c29mdCUyMFRpbWUtU3RhbXAlMjBQQ0ElMjAyMDEwKDEp
'' SIG '' LmNybDBsBggrBgEFBQcBAQRgMF4wXAYIKwYBBQUHMAKG
'' SIG '' UGh0dHA6Ly93d3cubWljcm9zb2Z0LmNvbS9wa2lvcHMv
'' SIG '' Y2VydHMvTWljcm9zb2Z0JTIwVGltZS1TdGFtcCUyMFBD
'' SIG '' QSUyMDIwMTAoMSkuY3J0MAwGA1UdEwEB/wQCMAAwFgYD
'' SIG '' VR0lAQH/BAwwCgYIKwYBBQUHAwgwDgYDVR0PAQH/BAQD
'' SIG '' AgeAMA0GCSqGSIb3DQEBCwUAA4ICAQA6NADLPRxXO1MU
'' SIG '' apdfkktHEUr87+gx7nm4OoQLxV3WBtwzbwoFQ+C9Qg9e
'' SIG '' b+90M3YhUGRYebAKngAhzLh1m2S5SZ3R+e7ppP0y+jWd
'' SIG '' 2wunZglwygUsS3dO2uIto76Lgau/RlQu1ZdQ8Bb8yflJ
'' SIG '' yOCfTFl24Y8EP9ezcnv6B6337xm8GKmyD83umiKZg5Ww
'' SIG '' fEtx6btXld0w2zK1Ob+4KiaEz/CBHkqUNhNU0BcHFxIo
'' SIG '' x4lqIPdXX4eE2RWWIyXlU4/6fDnFYXnm72Hp4XYbt4E+
'' SIG '' pP6pIVD6tAJB0is3TIbVA308muiC4r4UlAl1DN18PdFZ
'' SIG '' WxyIHKBthpmVPVwYkjUjJDvgNDRQF1Ol94azKsRD08jx
'' SIG '' DKpUupvarsu0joMkw2mFi76Ov//SymvVRW/IM+25GdsZ
'' SIG '' BE2LUI7AlyP05iaWQWAo14J9sNPtTe4Q69aiZ6RfrRj+
'' SIG '' bm61FxQ9V4A92GQH4PENp6/cnXLAM13K73XWcBU+BGTI
'' SIG '' qAwrdRIsbfsR2Vq0OTwXK4KvHi2IfKoc7fATrE/DfZDx
'' SIG '' 7++a5A+gFm5fepR6gUizJkR6cerZJwy6eFypbfZJRUCL
'' SIG '' mhnhned/t0CA1q7bU0Q/CBb7bCSs2oODsenzIfKg4puA
'' SIG '' QG7pERBu9J9nkqHg9X5LaDF/a6roahgOeWoAE4xjDPfT
'' SIG '' 0hKLRs8yHzCCB3EwggVZoAMCAQICEzMAAAAVxedrngKb
'' SIG '' SZkAAAAAABUwDQYJKoZIhvcNAQELBQAwgYgxCzAJBgNV
'' SIG '' BAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYD
'' SIG '' VQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQg
'' SIG '' Q29ycG9yYXRpb24xMjAwBgNVBAMTKU1pY3Jvc29mdCBS
'' SIG '' b290IENlcnRpZmljYXRlIEF1dGhvcml0eSAyMDEwMB4X
'' SIG '' DTIxMDkzMDE4MjIyNVoXDTMwMDkzMDE4MzIyNVowfDEL
'' SIG '' MAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24x
'' SIG '' EDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jv
'' SIG '' c29mdCBDb3Jwb3JhdGlvbjEmMCQGA1UEAxMdTWljcm9z
'' SIG '' b2Z0IFRpbWUtU3RhbXAgUENBIDIwMTAwggIiMA0GCSqG
'' SIG '' SIb3DQEBAQUAA4ICDwAwggIKAoICAQDk4aZM57RyIQt5
'' SIG '' osvXJHm9DtWC0/3unAcH0qlsTnXIyjVX9gF/bErg4r25
'' SIG '' PhdgM/9cT8dm95VTcVrifkpa/rg2Z4VGIwy1jRPPdzLA
'' SIG '' EBjoYH1qUoNEt6aORmsHFPPFdvWGUNzBRMhxXFExN6AK
'' SIG '' OG6N7dcP2CZTfDlhAnrEqv1yaa8dq6z2Nr41JmTamDu6
'' SIG '' GnszrYBbfowQHJ1S/rboYiXcag/PXfT+jlPP1uyFVk3v
'' SIG '' 3byNpOORj7I5LFGc6XBpDco2LXCOMcg1KL3jtIckw+DJ
'' SIG '' j361VI/c+gVVmG1oO5pGve2krnopN6zL64NF50ZuyjLV
'' SIG '' wIYwXE8s4mKyzbnijYjklqwBSru+cakXW2dg3viSkR4d
'' SIG '' Pf0gz3N9QZpGdc3EXzTdEonW/aUgfX782Z5F37ZyL9t9
'' SIG '' X4C626p+Nuw2TPYrbqgSUei/BQOj0XOmTTd0lBw0gg/w
'' SIG '' EPK3Rxjtp+iZfD9M269ewvPV2HM9Q07BMzlMjgK8Qmgu
'' SIG '' EOqEUUbi0b1qGFphAXPKZ6Je1yh2AuIzGHLXpyDwwvoS
'' SIG '' CtdjbwzJNmSLW6CmgyFdXzB0kZSU2LlQ+QuJYfM2BjUY
'' SIG '' hEfb3BvR/bLUHMVr9lxSUV0S2yW6r1AFemzFER1y7435
'' SIG '' UsSFF5PAPBXbGjfHCBUYP3irRbb1Hode2o+eFnJpxq57
'' SIG '' t7c+auIurQIDAQABo4IB3TCCAdkwEgYJKwYBBAGCNxUB
'' SIG '' BAUCAwEAATAjBgkrBgEEAYI3FQIEFgQUKqdS/mTEmr6C
'' SIG '' kTxGNSnPEP8vBO4wHQYDVR0OBBYEFJ+nFV0AXmJdg/Tl
'' SIG '' 0mWnG1M1GelyMFwGA1UdIARVMFMwUQYMKwYBBAGCN0yD
'' SIG '' fQEBMEEwPwYIKwYBBQUHAgEWM2h0dHA6Ly93d3cubWlj
'' SIG '' cm9zb2Z0LmNvbS9wa2lvcHMvRG9jcy9SZXBvc2l0b3J5
'' SIG '' Lmh0bTATBgNVHSUEDDAKBggrBgEFBQcDCDAZBgkrBgEE
'' SIG '' AYI3FAIEDB4KAFMAdQBiAEMAQTALBgNVHQ8EBAMCAYYw
'' SIG '' DwYDVR0TAQH/BAUwAwEB/zAfBgNVHSMEGDAWgBTV9lbL
'' SIG '' j+iiXGJo0T2UkFvXzpoYxDBWBgNVHR8ETzBNMEugSaBH
'' SIG '' hkVodHRwOi8vY3JsLm1pY3Jvc29mdC5jb20vcGtpL2Ny
'' SIG '' bC9wcm9kdWN0cy9NaWNSb29DZXJBdXRfMjAxMC0wNi0y
'' SIG '' My5jcmwwWgYIKwYBBQUHAQEETjBMMEoGCCsGAQUFBzAC
'' SIG '' hj5odHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vcGtpL2Nl
'' SIG '' cnRzL01pY1Jvb0NlckF1dF8yMDEwLTA2LTIzLmNydDAN
'' SIG '' BgkqhkiG9w0BAQsFAAOCAgEAnVV9/Cqt4SwfZwExJFvh
'' SIG '' nnJL/Klv6lwUtj5OR2R4sQaTlz0xM7U518JxNj/aZGx8
'' SIG '' 0HU5bbsPMeTCj/ts0aGUGCLu6WZnOlNN3Zi6th542DYu
'' SIG '' nKmCVgADsAW+iehp4LoJ7nvfam++Kctu2D9IdQHZGN5t
'' SIG '' ggz1bSNU5HhTdSRXud2f8449xvNo32X2pFaq95W2KFUn
'' SIG '' 0CS9QKC/GbYSEhFdPSfgQJY4rPf5KYnDvBewVIVCs/wM
'' SIG '' nosZiefwC2qBwoEZQhlSdYo2wh3DYXMuLGt7bj8sCXgU
'' SIG '' 6ZGyqVvfSaN0DLzskYDSPeZKPmY7T7uG+jIa2Zb0j/aR
'' SIG '' AfbOxnT99kxybxCrdTDFNLB62FD+CljdQDzHVG2dY3RI
'' SIG '' LLFORy3BFARxv2T5JL5zbcqOCb2zAVdJVGTZc9d/HltE
'' SIG '' AY5aGZFrDZ+kKNxnGSgkujhLmm77IVRrakURR6nxt67I
'' SIG '' 6IleT53S0Ex2tVdUCbFpAUR+fKFhbHP+CrvsQWY9af3L
'' SIG '' wUFJfn6Tvsv4O+S3Fb+0zj6lMVGEvL8CwYKiexcdFYmN
'' SIG '' cP7ntdAoGokLjzbaukz5m/8K6TT4JDVnK+ANuOaMmdbh
'' SIG '' IurwJ0I9JZTmdHRbatGePu1+oDEzfbzL6Xu/OHBE0ZDx
'' SIG '' yKs6ijoIYn/ZcGNTTY3ugm2lBRDBcQZqELQdVTNYs6Fw
'' SIG '' ZvKhggNZMIICQQIBATCCAQGhgdmkgdYwgdMxCzAJBgNV
'' SIG '' BAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYD
'' SIG '' VQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQg
'' SIG '' Q29ycG9yYXRpb24xLTArBgNVBAsTJE1pY3Jvc29mdCBJ
'' SIG '' cmVsYW5kIE9wZXJhdGlvbnMgTGltaXRlZDEnMCUGA1UE
'' SIG '' CxMeblNoaWVsZCBUU1MgRVNOOjU1MUEtMDVFMC1EOTQ3
'' SIG '' MSUwIwYDVQQDExxNaWNyb3NvZnQgVGltZS1TdGFtcCBT
'' SIG '' ZXJ2aWNloiMKAQEwBwYFKw4DAhoDFQDX7bpxH/IfXTQO
'' SIG '' I0UZaG4C/atgGqCBgzCBgKR+MHwxCzAJBgNVBAYTAlVT
'' SIG '' MRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdS
'' SIG '' ZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9y
'' SIG '' YXRpb24xJjAkBgNVBAMTHU1pY3Jvc29mdCBUaW1lLVN0
'' SIG '' YW1wIFBDQSAyMDEwMA0GCSqGSIb3DQEBCwUAAgUA6uJj
'' SIG '' oTAiGA8yMDI0MTExNjAwMTk0NVoYDzIwMjQxMTE3MDAx
'' SIG '' OTQ1WjB3MD0GCisGAQQBhFkKBAExLzAtMAoCBQDq4mOh
'' SIG '' AgEAMAoCAQACAhpJAgH/MAcCAQACAhQuMAoCBQDq47Uh
'' SIG '' AgEAMDYGCisGAQQBhFkKBAIxKDAmMAwGCisGAQQBhFkK
'' SIG '' AwKgCjAIAgEAAgMHoSChCjAIAgEAAgMBhqAwDQYJKoZI
'' SIG '' hvcNAQELBQADggEBAD+jE7HfHa8PLWUyT4nKAg1NB1AX
'' SIG '' STcv0pzODuEsErwZ+jvyb+6M8g1VIdyG2lk2EwvQkc0F
'' SIG '' oOlqDIPMtacA3aoonnwKRGB/A2IWb4ilTGINrW7JaKO5
'' SIG '' KaFjPN0RTWJR8X+VmnAPyKdXUqQjwBuHsDeO4yn4gvM/
'' SIG '' V8ebpu+dBUR1O/Ct1JsxkS/4MoCXSRvStiaO6Im7TMLV
'' SIG '' GCxdTD6dOGnqZM/XOVw9ef2kYckJm+29LnptrnNOc2Ra
'' SIG '' iJat8ZCXWqZyDA6WkKFNLpo6ijPC31kwQS8PoEKp+2Mg
'' SIG '' pP3wKmzWzJaz7Ka08o1zN7lzG4ertiirbSZptMH8XFg+
'' SIG '' aM2EB74xggQNMIIECQIBATCBkzB8MQswCQYDVQQGEwJV
'' SIG '' UzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMH
'' SIG '' UmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBv
'' SIG '' cmF0aW9uMSYwJAYDVQQDEx1NaWNyb3NvZnQgVGltZS1T
'' SIG '' dGFtcCBQQ0EgMjAxMAITMwAAAgHRRVmYEMxCTwABAAAC
'' SIG '' ATANBglghkgBZQMEAgEFAKCCAUowGgYJKoZIhvcNAQkD
'' SIG '' MQ0GCyqGSIb3DQEJEAEEMC8GCSqGSIb3DQEJBDEiBCA9
'' SIG '' eCmdKpXH0kdUWj8c9X6pu9Qj8L8hmVY0qKTShvABmjCB
'' SIG '' +gYLKoZIhvcNAQkQAi8xgeowgecwgeQwgb0EIFhrsjpM
'' SIG '' lBFybHQdpJNZl0mCjB2uX35muvSkh2oe1zgjMIGYMIGA
'' SIG '' pH4wfDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hp
'' SIG '' bmd0b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoT
'' SIG '' FU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEmMCQGA1UEAxMd
'' SIG '' TWljcm9zb2Z0IFRpbWUtU3RhbXAgUENBIDIwMTACEzMA
'' SIG '' AAIB0UVZmBDMQk8AAQAAAgEwIgQgcd+nw1+K4FpZjKyR
'' SIG '' B5aLL9akXxku4aDPCQM2f0gyEyowDQYJKoZIhvcNAQEL
'' SIG '' BQAEggIAKm/GhQGwifikdab1I5KO6BbnVhtGBmsyIiB6
'' SIG '' NmsNUccdWVv9pdgzmIDEWZPCmkIvclo4nl5ei1tPMnJ2
'' SIG '' uWviuBJKI5vBWF+2F4i58hJ+dQGTdL2OGMi03hGqtvD7
'' SIG '' 2Mp3NCZDP0hva+NoRmozw94Su8MgWGOW1SWT9lQ7CFTB
'' SIG '' SEHq52MTPhHr+M0fqCg4qefxQ4Edc4ZbEZFcqegKd7fZ
'' SIG '' vvpScaIdsrxCN7QobF36DtUdWvLyRnhRCfBc8u1ad/Rl
'' SIG '' U0HAOjSzxUTiwSYdTTTbshzsQSCflmAtrVGABcn8gtp2
'' SIG '' LDjf3lNcL+afYf1fz+0XSppCRLr7O3DE0rrOVrS/uxyY
'' SIG '' P5tE4vox0VwnwjmbuMd8KL0WKC8pll5nXh0n/jPbuCyb
'' SIG '' NavvBS/6ld2DeyrExqjlTscx/I/2s/Eg+6UctILsEA7r
'' SIG '' RoR+NOI7g5ATMNVlZwggk5N9DcaJ5cfX/TJMVQptK7Z9
'' SIG '' QcTs5nSIa52Jj1jLYYO/Fk4O5vKk+eGcOhtmsjGcRIgH
'' SIG '' UmvuwGkQQRnKnJ/uO6eqdZRa9Q/c0MVnMlbjhgPOzbEs
'' SIG '' jIEh9Lh/nrP2WCNv0tWObvzxs6ShL08LSFaVJ8Y8oHUt
'' SIG '' O0+8pqoAfQUUSUESE5RKbAH8CYIwjp0swen5GW/hlDin
'' SIG '' n3mQYtt16pF9ibl8IcwnWqWHNmvGY50=
'' SIG '' End signature block
