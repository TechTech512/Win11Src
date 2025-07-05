' Windows Installer utility to report or update file versions, sizes, languages
' For use with Windows Scripting Host, CScript.exe or WScript.exe
' Copyright (c) Microsoft Corporation. All rights reserved.
' Demonstrates the access to install engine and actions
'
Option Explicit

' FileSystemObject.CreateTextFile and FileSystemObject.OpenTextFile
Const OpenAsASCII   = 0 
Const OpenAsUnicode = -1

' FileSystemObject.CreateTextFile
Const OverwriteIfExist = -1
Const FailIfExist      = 0

' FileSystemObject.OpenTextFile
Const OpenAsDefault    = -2
Const CreateIfNotExist = -1
Const FailIfNotExist   = 0
Const ForReading = 1
Const ForWriting = 2
Const ForAppending = 8

Const msiOpenDatabaseModeReadOnly = 0
Const msiOpenDatabaseModeTransact = 1

Const msiViewModifyInsert         = 1
Const msiViewModifyUpdate         = 2
Const msiViewModifyAssign         = 3
Const msiViewModifyReplace        = 4
Const msiViewModifyDelete         = 6

Const msiUILevelNone = 2

Const msiRunModeSourceShortNames = 9

Const msidbFileAttributesNoncompressed = &h00002000

Dim argCount:argCount = Wscript.Arguments.Count
Dim iArg:iArg = 0
If argCount > 0 Then If InStr(1, Wscript.Arguments(0), "?", vbTextCompare) > 0 Then argCount = 0
If (argCount < 1) Then
	Wscript.Echo "Windows Installer utility to updata File table sizes and versions" &_
		vbNewLine & " The 1st argument is the path to MSI database, at the source file root" &_
		vbNewLine & " The 2nd argument can optionally specify separate source location from the MSI" &_
		vbNewLine & " The following options may be specified at any point on the command line" &_
		vbNewLine & "  /U to update the MSI database with the file sizes, versions, and languages" &_
		vbNewLine & "  /H to populate the MsiFileHash table (and create if it doesn't exist)" &_
		vbNewLine & " Notes:" &_
		vbNewLine & "  If source type set to compressed, all files will be opened at the root" &_
		vbNewLine & "  Using CSCRIPT.EXE without the /U option, the file info will be displayed" &_
		vbNewLine & "  Using the /H option requires Windows Installer version 2.0 or greater" &_
		vbNewLine & "  Using the /H option also requires the /U option" &_
		vbNewLine &_
		vbNewLine & "Copyright (C) Microsoft Corporation.  All rights reserved."
	Wscript.Quit 1
End If

' Get argument values, processing any option flags
Dim updateMsi    : updateMsi    = False
Dim populateHash : populateHash = False
Dim sequenceFile : sequenceFile = False
Dim databasePath : databasePath = NextArgument
Dim sourceFolder : sourceFolder = NextArgument
If Not IsEmpty(NextArgument) Then Fail "More than 2 arguments supplied" ' process any trailing options
If Not IsEmpty(sourceFolder) And Right(sourceFolder, 1) <> "\" Then sourceFolder = sourceFolder & "\"
Dim console : If UCase(Mid(Wscript.FullName, Len(Wscript.Path) + 2, 1)) = "C" Then console = True

' Connect to Windows Installer object
On Error Resume Next
Dim installer : Set installer = Nothing
Set installer = Wscript.CreateObject("WindowsInstaller.Installer") : CheckError

Dim errMsg

' Check Installer version to see if MsiFileHash table population is supported
Dim supportHash : supportHash = False
Dim verInstaller : verInstaller = installer.Version
If CInt(Left(verInstaller, 1)) >= 2 Then supportHash = True
If populateHash And NOT supportHash Then
	errMsg = "The version of Windows Installer on the machine does not support populating the MsiFileHash table."
	errMsg = errMsg & " Windows Installer version 2.0 is the mininum required version. The version on the machine is " & verInstaller & vbNewLine
	Fail errMsg
End If

' Check if multiple language package, and force use of primary language
REM	Set sumInfo = database.SummaryInformation(3) : CheckError

' Open database
Dim database, openMode, view, record, updateMode, sumInfo
If updateMsi Then openMode = msiOpenDatabaseModeTransact Else openMode = msiOpenDatabaseModeReadOnly
Set database = installer.OpenDatabase(databasePath, openMode) : CheckError

' Create MsiFileHash table if we will be populating it and it is not already present
Dim hashView, iTableStat, fileHash, hashUpdateRec
iTableStat = Database.TablePersistent("MsiFileHash")
If populateHash Then
	If NOT updateMsi Then
		errMsg = "Populating the MsiFileHash table requires that the database be open for writing. Please include the /U option"
		Fail errMsg		
	End If

	If iTableStat <> 1 Then
		Set hashView = database.OpenView("CREATE TABLE `MsiFileHash` ( `File_` CHAR(72) NOT NULL, `Options` INTEGER NOT NULL, `HashPart1` LONG NOT NULL, `HashPart2` LONG NOT NULL, `HashPart3` LONG NOT NULL, `HashPart4` LONG NOT NULL PRIMARY KEY `File_` )") : CheckError
		hashView.Execute : CheckError
	End If

	Set hashView = database.OpenView("SELECT `File_`, `Options`, `HashPart1`, `HashPart2`, `HashPart3`, `HashPart4` FROM `MsiFileHash`") : CheckError
	hashView.Execute : CheckError

	Set hashUpdateRec = installer.CreateRecord(6)
End If

' Create an install session and execute actions in order to perform directory resolution
installer.UILevel = msiUILevelNone
Dim session : Set session = installer.OpenPackage(database,1) : If Err <> 0 Then Fail "Database: " & databasePath & ". Invalid installer package format"
Dim shortNames : shortNames = session.Mode(msiRunModeSourceShortNames) : CheckError
If Not IsEmpty(sourceFolder) Then session.Property("OriginalDatabase") = sourceFolder : CheckError
Dim stat : stat = session.DoAction("CostInitialize") : CheckError
If stat <> 1 Then Fail "CostInitialize failed, returned " & stat

' Join File table to Component table in order to find directories
Dim orderBy : If sequenceFile Then orderBy = "Directory_" Else orderBy = "Sequence"
Set view = database.OpenView("SELECT File,FileName,Directory_,FileSize,Version,Language FROM File,Component WHERE Component_=Component ORDER BY " & orderBy) : CheckError
view.Execute : CheckError

' Create view on File table to check for companion file version syntax so that we don't overwrite them
Dim companionView
set companionView = database.OpenView("SELECT File FROM File WHERE File=?") : CheckError

' Fetch each file and request the source path, then verify the source path, and get the file info if present
Dim fileKey, fileName, folder, sourcePath, fileSize, version, language, delim, message, info
Do
	Set record = view.Fetch : CheckError
	If record Is Nothing Then Exit Do
	fileKey    = record.StringData(1)
	fileName   = record.StringData(2)
	folder     = record.StringData(3)
REM	fileSize   = record.IntegerData(4)
REM	companion  = record.StringData(5)
	version    = record.StringData(5)
REM	language   = record.StringData(6)

	' Check to see if this is a companion file
	Dim companionRec
	Set companionRec = installer.CreateRecord(1) : CheckError
	companionRec.StringData(1) = version
	companionView.Close : CheckError
	companionView.Execute companionRec : CheckError
	Dim companionFetch
	Set companionFetch = companionView.Fetch : CheckError
	Dim companionFile : companionFile = True
	If companionFetch Is Nothing Then
		companionFile = False
	End If

	delim = InStr(1, fileName, "|", vbTextCompare)
	If delim <> 0 Then
		If shortNames Then fileName = Left(fileName, delim-1) Else fileName = Right(fileName, Len(fileName) - delim)
	End If
	sourcePath = session.SourcePath(folder) & fileName
	If installer.FileAttributes(sourcePath) = -1 Then
		message = message & vbNewLine & sourcePath
	Else
		fileSize = installer.FileSize(sourcePath) : CheckError
		version  = Empty : version  = installer.FileVersion(sourcePath, False) : Err.Clear ' early MSI implementation fails if no version
		language = Empty : language = installer.FileVersion(sourcePath, True)  : Err.Clear ' early MSI implementation doesn't support language
		If language = version Then language = Empty ' Temp check for MSI.DLL version without language support
		If Err <> 0 Then version = Empty : Err.Clear
		If updateMsi Then
			' update File table info
			record.IntegerData(4) = fileSize
			If Len(version)  > 0 Then record.StringData(5) = version
			If Len(language) > 0 Then record.StringData(6) = language
			view.Modify msiViewModifyUpdate, record : CheckError

			' update MsiFileHash table info if this is an unversioned file
			If populateHash And Len(version) = 0 Then
				Set fileHash = installer.FileHash(sourcePath, 0) : CheckError
				hashUpdateRec.StringData(1) = fileKey
				hashUpdateRec.IntegerData(2) = 0
				hashUpdateRec.IntegerData(3) = fileHash.IntegerData(1)
				hashUpdateRec.IntegerData(4) = fileHash.IntegerData(2)
				hashUpdateRec.IntegerData(5) = fileHash.IntegerData(3)
				hashUpdateRec.IntegerData(6) = fileHash.IntegerData(4)
				hashView.Modify msiViewModifyAssign, hashUpdateRec : CheckError
			End If
		ElseIf console Then
			If companionFile Then
				info = "* "
				info = info & fileName : If Len(info) < 12 Then info = info & Space(12 - Len(info))
				info = info & "  skipped (version is a reference to a companion file)"
			Else
				info = fileName : If Len(info) < 12 Then info = info & Space(12 - Len(info))
				info = info & "  size=" & fileSize : If Len(info) < 26 Then info = info & Space(26 - Len(info))
				If Len(version)  > 0 Then info = info & "  vers=" & version : If Len(info) < 45 Then info = info & Space(45 - Len(info))
				If Len(language) > 0 Then info = info & "  lang=" & language
			End If
			Wscript.Echo info
		End If
	End If
Loop
REM Wscript.Echo "SourceDir = " & session.Property("SourceDir")
If Not IsEmpty(message) Then Fail "Error, the following files were not available:" & message

' Update SummaryInformation
If updateMsi Then
	Set sumInfo = database.SummaryInformation(3) : CheckError
	sumInfo.Property(11) = Now
	sumInfo.Property(13) = Now
	sumInfo.Persist
End If

' Commit database in case updates performed
database.Commit : CheckError
Wscript.Quit 0

' Extract argument value from command line, processing any option flags
Function NextArgument
	Dim arg
	Do  ' loop to pull in option flags until an argument value is found
		If iArg >= argCount Then Exit Function
		arg = Wscript.Arguments(iArg)
		iArg = iArg + 1
		If (AscW(arg) <> AscW("/")) And (AscW(arg) <> AscW("-")) Then Exit Do
		Select Case UCase(Right(arg, Len(arg)-1))
			Case "U" : updateMsi    = True
			Case "H" : populateHash = True
			Case Else: Wscript.Echo "Invalid option flag:", arg : Wscript.Quit 1
		End Select
	Loop
	NextArgument = arg
End Function

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
'' SIG '' MIImQwYJKoZIhvcNAQcCoIImNDCCJjACAQExDzANBglg
'' SIG '' hkgBZQMEAgEFADB3BgorBgEEAYI3AgEEoGkwZzAyBgor
'' SIG '' BgEEAYI3AgEeMCQCAQEEEE7wKRaZJ7VNj+Ws4Q8X66sC
'' SIG '' AQACAQACAQACAQACAQAwMTANBglghkgBZQMEAgEFAAQg
'' SIG '' 90R1z4uuv6FSmeekmrnJ1Xqp08A0D4fjgi9+4dO31L2g
'' SIG '' ggt2MIIE/jCCA+agAwIBAgITMwAABVbJICsfdDJdLQAA
'' SIG '' AAAFVjANBgkqhkiG9w0BAQsFADB+MQswCQYDVQQGEwJV
'' SIG '' UzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMH
'' SIG '' UmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBv
'' SIG '' cmF0aW9uMSgwJgYDVQQDEx9NaWNyb3NvZnQgQ29kZSBT
'' SIG '' aWduaW5nIFBDQSAyMDEwMB4XDTIzMTAxOTE5NTExMVoX
'' SIG '' DTI0MTAxNjE5NTExMVowdDELMAkGA1UEBhMCVVMxEzAR
'' SIG '' BgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1v
'' SIG '' bmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlv
'' SIG '' bjEeMBwGA1UEAxMVTWljcm9zb2Z0IENvcnBvcmF0aW9u
'' SIG '' MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA
'' SIG '' ltpIPPc1p7LIgxvQBav7MapD0+N1eDGer8LuwuPrJcuO
'' SIG '' kCQOFDcUkZxg8/bvH9fDkdfwK/YLkA6kbYazjpLS2qJe
'' SIG '' PR2X7/JdQxHgf7oLlktKhSCXvnCum+4K1X5dEme1PMjl
'' SIG '' 7uu5+ds/kCTfolMXCJNClnLv7CWfCn3sCsZzQzAyBx4V
'' SIG '' B7yI0FobysTiwv08C9IuME8pF7kMG8CGbrhou02APNkN
'' SIG '' i5GDi5cDkzzm9HqMIXFCOwml5VN9CIKBuH62PprWTGZ0
'' SIG '' 8dIGv2t+hlTXaujXgSs5RmywdNv1iD/nOQAwwl7IXlqZ
'' SIG '' IsybfWj4c2LqJ7fjcdDoSB9OJSRbwqo5YwIDAQABo4IB
'' SIG '' fTCCAXkwHwYDVR0lBBgwFgYKKwYBBAGCNz0GAQYIKwYB
'' SIG '' BQUHAwMwHQYDVR0OBBYEFCbfBYUBcF+4OQP9HpQ8ZI8M
'' SIG '' PNnaMFQGA1UdEQRNMEukSTBHMS0wKwYDVQQLEyRNaWNy
'' SIG '' b3NvZnQgSXJlbGFuZCBPcGVyYXRpb25zIExpbWl0ZWQx
'' SIG '' FjAUBgNVBAUTDTIzMDg2NSs1MDE2NTUwHwYDVR0jBBgw
'' SIG '' FoAU5vxfe7siAFjkck619CF0IzLm76wwVgYDVR0fBE8w
'' SIG '' TTBLoEmgR4ZFaHR0cDovL2NybC5taWNyb3NvZnQuY29t
'' SIG '' L3BraS9jcmwvcHJvZHVjdHMvTWljQ29kU2lnUENBXzIw
'' SIG '' MTAtMDctMDYuY3JsMFoGCCsGAQUFBwEBBE4wTDBKBggr
'' SIG '' BgEFBQcwAoY+aHR0cDovL3d3dy5taWNyb3NvZnQuY29t
'' SIG '' L3BraS9jZXJ0cy9NaWNDb2RTaWdQQ0FfMjAxMC0wNy0w
'' SIG '' Ni5jcnQwDAYDVR0TAQH/BAIwADANBgkqhkiG9w0BAQsF
'' SIG '' AAOCAQEAQp2ZaDMYxwVRyRD+nftLexAyXzQdIe4/Yjl+
'' SIG '' i0IjzHUAFdcagOiYG/1RD0hFbNO+ggCZ9yj+Saa+Azrq
'' SIG '' NdgRNgqArrGQx5/u2j9ssZ4DBhkHCSs+FHzswzEvWK9r
'' SIG '' Jd0enzD9fE+AnubeyGBSt+jyPx37xzvAMwd09CoVSIn6
'' SIG '' rEsGfJhLpMP8IuHbiWLpWMVdpWNpDB8L/zirygLK03d9
'' SIG '' /B5Z7kfs/TWb0rTVItWvLE8HBDKxD/JYLaMWmXtGKbvz
'' SIG '' oZ+D6k3nxFVikCS1Nihciw5KGpg3XtMnQM8x2BKnQUDF
'' SIG '' tIMVsryfX44BfwtjykFbv9EjAYXMKNOHhc3/8O6WfzCC
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
'' SIG '' MYIaJTCCGiECAQEwgZUwfjELMAkGA1UEBhMCVVMxEzAR
'' SIG '' BgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1v
'' SIG '' bmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlv
'' SIG '' bjEoMCYGA1UEAxMfTWljcm9zb2Z0IENvZGUgU2lnbmlu
'' SIG '' ZyBQQ0EgMjAxMAITMwAABVbJICsfdDJdLQAAAAAFVjAN
'' SIG '' BglghkgBZQMEAgEFAKCBxjAZBgkqhkiG9w0BCQMxDAYK
'' SIG '' KwYBBAGCNwIBBDAcBgorBgEEAYI3AgELMQ4wDAYKKwYB
'' SIG '' BAGCNwIBFTAvBgkqhkiG9w0BCQQxIgQgaZ81X6BTsfXj
'' SIG '' ZaDeKIpyi4UteXGGVt5deRIRLgwg7WkwWgYKKwYBBAGC
'' SIG '' NwIBDDFMMEqgJIAiAE0AaQBjAHIAbwBzAG8AZgB0ACAA
'' SIG '' VwBpAG4AZABvAHcAc6EigCBodHRwOi8vd3d3Lm1pY3Jv
'' SIG '' c29mdC5jb20vd2luZG93czANBgkqhkiG9w0BAQEFAASC
'' SIG '' AQABhRB0XyvN71RF/23lE9aNfwSJnURZMsNser6L2cHL
'' SIG '' 4aMgaiQ41hk0DEs+wK2ZOCWjYEyaZXf/KBwuHoXwLSrm
'' SIG '' aIIng4K2u3UCifdoOCS2kkih2aHx41eQ12CC8XDmYA16
'' SIG '' jEus9Qy0eo5E+uqbegIWVTCEF1AmaulMIhjrfcZaXTAq
'' SIG '' FVZoUj3n1K6nEBkG6i33TO91ObVKFer4G8/aTr0v/JY2
'' SIG '' eUBq5mbR3qxQ5A8GgeQjG+9I8mNnyj12z8e5S0ApKtXZ
'' SIG '' iH25VedM06VSqu1lnTYNjOw0k62IuAidfzugW+7XR1kx
'' SIG '' c6nDUQoXLHpgmMzW1m2EVKaGsr1BLBM4tzryoYIXlzCC
'' SIG '' F5MGCisGAQQBgjcDAwExgheDMIIXfwYJKoZIhvcNAQcC
'' SIG '' oIIXcDCCF2wCAQMxDzANBglghkgBZQMEAgEFADCCAVIG
'' SIG '' CyqGSIb3DQEJEAEEoIIBQQSCAT0wggE5AgEBBgorBgEE
'' SIG '' AYRZCgMBMDEwDQYJYIZIAWUDBAIBBQAEIBiM7AG2xQl2
'' SIG '' vVlBx/NvsYBOeWgwWzny2JqH5m1+1yOrAgZmviYGuTQY
'' SIG '' EzIwMjQwOTA1MDkxNzA4LjI5NFowBIACAfSggdGkgc4w
'' SIG '' gcsxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5n
'' SIG '' dG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVN
'' SIG '' aWNyb3NvZnQgQ29ycG9yYXRpb24xJTAjBgNVBAsTHE1p
'' SIG '' Y3Jvc29mdCBBbWVyaWNhIE9wZXJhdGlvbnMxJzAlBgNV
'' SIG '' BAsTHm5TaGllbGQgVFNTIEVTTjozNzAzLTA1RTAtRDk0
'' SIG '' NzElMCMGA1UEAxMcTWljcm9zb2Z0IFRpbWUtU3RhbXAg
'' SIG '' U2VydmljZaCCEe0wggcgMIIFCKADAgECAhMzAAAB6pok
'' SIG '' ctVZP2FjAAEAAAHqMA0GCSqGSIb3DQEBCwUAMHwxCzAJ
'' SIG '' BgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAw
'' SIG '' DgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3Nv
'' SIG '' ZnQgQ29ycG9yYXRpb24xJjAkBgNVBAMTHU1pY3Jvc29m
'' SIG '' dCBUaW1lLVN0YW1wIFBDQSAyMDEwMB4XDTIzMTIwNjE4
'' SIG '' NDUzMFoXDTI1MDMwNTE4NDUzMFowgcsxCzAJBgNVBAYT
'' SIG '' AlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQH
'' SIG '' EwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29y
'' SIG '' cG9yYXRpb24xJTAjBgNVBAsTHE1pY3Jvc29mdCBBbWVy
'' SIG '' aWNhIE9wZXJhdGlvbnMxJzAlBgNVBAsTHm5TaGllbGQg
'' SIG '' VFNTIEVTTjozNzAzLTA1RTAtRDk0NzElMCMGA1UEAxMc
'' SIG '' TWljcm9zb2Z0IFRpbWUtU3RhbXAgU2VydmljZTCCAiIw
'' SIG '' DQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBALULX/FI
'' SIG '' PyAH1fsu52ijatZvaSypoXrlC0mRtCmaxzobhuDkw6/p
'' SIG '' Y/+4nhc4m8pf9zW3R6PihYGp0YPpVuNdfhPQp/KVO6Wv
'' SIG '' Mq2DGfFmHurW4PQPL/DkbQMkM9vqjFCvPq8xXZnfL1nG
'' SIG '' N9moGcN+oaif/hUMedmF1qzbay9ILkYfLCxDYn3Qwzsv
'' SIG '' h5xjxOcsjzmRddNURJvT23Eva0cxisH4ocLLTx2zfpqf
'' SIG '' shw4Z9GaEdsWg9rmib1galUpLzF5PsQDBbtZtcv+Wjmn
'' SIG '' 0pFEiMCWwEEcPVN0YG5ysYLdNBdJOn2zsOOS+80W5RrQ
'' SIG '' EqzPpSIIvEkZBJmF3aI4lMR8nV/FiTadjpIIqxX5Wa1X
'' SIG '' lqI/Nj+xagVjnjb7POsA+vh6Wu+v24HpyL8pyL/8Q4RF
'' SIG '' kRRME9cwT+Jr63yOtPbLe6DXkxIJW6E6w2ua5kXBpEKt
'' SIG '' EQPTLPhX3CUxMYcglbnmI0zcc9UknX285K+sI/2WwRwT
'' SIG '' BZkhDUULI86eQzV+zvzzR1qEBrlSY+oyTlYQrHMM9WnT
'' SIG '' zVflFDocZVTPpl2BDSNxPn0Qb4IoM9EPqbHyi/MilL+v
'' SIG '' /AQc8q3mQ6FiuPJAddz0ocpNZ9ekBWPVLKq3lfiev4yl
'' SIG '' 65u/438+NAQ+vSJgkONLMmuoguEGzmnK1vq/JHwdRUyn
'' SIG '' 6YADiteM7Dja+Qd9AgMBAAGjggFJMIIBRTAdBgNVHQ4E
'' SIG '' FgQUK4FFJaJR5ukXQFTUxMhyiwVuWV4wHwYDVR0jBBgw
'' SIG '' FoAUn6cVXQBeYl2D9OXSZacbUzUZ6XIwXwYDVR0fBFgw
'' SIG '' VjBUoFKgUIZOaHR0cDovL3d3dy5taWNyb3NvZnQuY29t
'' SIG '' L3BraW9wcy9jcmwvTWljcm9zb2Z0JTIwVGltZS1TdGFt
'' SIG '' cCUyMFBDQSUyMDIwMTAoMSkuY3JsMGwGCCsGAQUFBwEB
'' SIG '' BGAwXjBcBggrBgEFBQcwAoZQaHR0cDovL3d3dy5taWNy
'' SIG '' b3NvZnQuY29tL3BraW9wcy9jZXJ0cy9NaWNyb3NvZnQl
'' SIG '' MjBUaW1lLVN0YW1wJTIwUENBJTIwMjAxMCgxKS5jcnQw
'' SIG '' DAYDVR0TAQH/BAIwADAWBgNVHSUBAf8EDDAKBggrBgEF
'' SIG '' BQcDCDAOBgNVHQ8BAf8EBAMCB4AwDQYJKoZIhvcNAQEL
'' SIG '' BQADggIBACiDrVZeP37+fFVtfcbfsqC/Kg0Ce67bDceh
'' SIG '' ZmPcfRgJ5Ddv0pJlOFVOFbiIVwesqeEUwFtclfi5Ajne
'' SIG '' Q5ZJpYJpXfELOelG3dzj+BKfd287/UY/cwmSkl+CjnoK
'' SIG '' BL3Ms6I/fWR+alR0+p6RlviK8xHoug9vkc2WrRZsGnMV
'' SIG '' u2xOM2tPJ+qpyoDBzqv30N/ZRBOoNrS/PCkDwLGICDYq
'' SIG '' Vs/IzAE49yv2ElPywalf9mEsOHXV1lxtQDNcejVEmitJ
'' SIG '' J+1Vr2EtafPEbMQZp89TAuagROKE4YuohCUKm+v3geJq
'' SIG '' TQarTBjqV25RCOT+XFngTMDD9wYx6TwndB2I1Ly726Ni
'' SIG '' HUHs0uvq3ciCV9JwNXdt1VZ63WK1NSgpVEsiK9EPABPt
'' SIG '' 1EfXcKrfaPYkbkFi79eK1ETxx3NomYNUHNiGU+X1Be8L
'' SIG '' 7qpHwjo0g3/33XhtOr9LiDoUXh/V2LFTETiqV9Q8yLEa
'' SIG '' vQW3j9LQ/h/CaGz5YdGfrY8HiPfMIeLEokKxGf0hHcTE
'' SIG '' FApB0yLlq6KoHrFAEANR/4XuFIpl9sDywVIWt4tKqG+P
'' SIG '' 6pRAXzg1zG5rGlslZWmw7XwgvhBu3jkLP9AxrsSYwY2f
'' SIG '' trwwze5NA6VDLS7pz+OrXXWLUmoyNrJNx5Bk0wEwzkQx
'' SIG '' zkOvmbdPhsOP1ZM0uA/xIV7cSpNpZUw5MIIHcTCCBVmg
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
'' SIG '' baUFEMFxBmoQtB1VM1izoXBm8qGCA1AwggI4AgEBMIH5
'' SIG '' oYHRpIHOMIHLMQswCQYDVQQGEwJVUzETMBEGA1UECBMK
'' SIG '' V2FzaGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwG
'' SIG '' A1UEChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMSUwIwYD
'' SIG '' VQQLExxNaWNyb3NvZnQgQW1lcmljYSBPcGVyYXRpb25z
'' SIG '' MScwJQYDVQQLEx5uU2hpZWxkIFRTUyBFU046MzcwMy0w
'' SIG '' NUUwLUQ5NDcxJTAjBgNVBAMTHE1pY3Jvc29mdCBUaW1l
'' SIG '' LVN0YW1wIFNlcnZpY2WiIwoBATAHBgUrDgMCGgMVAInb
'' SIG '' HtxB+OlGyQnxQYhy04KSYSSPoIGDMIGApH4wfDELMAkG
'' SIG '' A1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAO
'' SIG '' BgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29m
'' SIG '' dCBDb3Jwb3JhdGlvbjEmMCQGA1UEAxMdTWljcm9zb2Z0
'' SIG '' IFRpbWUtU3RhbXAgUENBIDIwMTAwDQYJKoZIhvcNAQEL
'' SIG '' BQACBQDqg6liMCIYDzIwMjQwOTA1MDM1MjAyWhgPMjAy
'' SIG '' NDA5MDYwMzUyMDJaMHcwPQYKKwYBBAGEWQoEATEvMC0w
'' SIG '' CgIFAOqDqWICAQAwCgIBAAICIFsCAf8wBwIBAAICEq8w
'' SIG '' CgIFAOqE+uICAQAwNgYKKwYBBAGEWQoEAjEoMCYwDAYK
'' SIG '' KwYBBAGEWQoDAqAKMAgCAQACAwehIKEKMAgCAQACAwGG
'' SIG '' oDANBgkqhkiG9w0BAQsFAAOCAQEAWLBB4Rm+tF1SHi5h
'' SIG '' Itw50WDiTcIo/QmveN0low0uy3tjwSlDSFZRi7MMwnMM
'' SIG '' xsvmm49u4Mek2kc2ax0B7HRc9b7ckSnKS9oK+J+TJ3gH
'' SIG '' LfW2TQM+DMkttMybfbWQyyQSI+OM7PGDFWyHn/EfGfVL
'' SIG '' s+C7UC8bNsVuaKSuknV9o1nuIA3ItpxVkg7+6n2l81Eg
'' SIG '' YoF76wCbtyblT/ypIQRZwETqnkbyvZazlrkjZ7PDdrdu
'' SIG '' 8l3+45BAiNua56xqB4nhkR5JoufF2dcvqCQ7pE5c/RZw
'' SIG '' Vwp0zcnpV3KrnKiRKYooY7KbIB0xD17Tq0zyhTQ3TSrd
'' SIG '' StvMbNfSIzKb8jzRRTGCBA0wggQJAgEBMIGTMHwxCzAJ
'' SIG '' BgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAw
'' SIG '' DgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3Nv
'' SIG '' ZnQgQ29ycG9yYXRpb24xJjAkBgNVBAMTHU1pY3Jvc29m
'' SIG '' dCBUaW1lLVN0YW1wIFBDQSAyMDEwAhMzAAAB6pokctVZ
'' SIG '' P2FjAAEAAAHqMA0GCWCGSAFlAwQCAQUAoIIBSjAaBgkq
'' SIG '' hkiG9w0BCQMxDQYLKoZIhvcNAQkQAQQwLwYJKoZIhvcN
'' SIG '' AQkEMSIEIACfv08et/rRqyKeXxodzO3dc4pa+Aa+Uq9w
'' SIG '' 7s8EhM+cMIH6BgsqhkiG9w0BCRACLzGB6jCB5zCB5DCB
'' SIG '' vQQgKY+h1eNkNHiLCDSW0sA1cGHkbW4qooi+ryyMp6S4
'' SIG '' ZngwgZgwgYCkfjB8MQswCQYDVQQGEwJVUzETMBEGA1UE
'' SIG '' CBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEe
'' SIG '' MBwGA1UEChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMSYw
'' SIG '' JAYDVQQDEx1NaWNyb3NvZnQgVGltZS1TdGFtcCBQQ0Eg
'' SIG '' MjAxMAITMwAAAeqaJHLVWT9hYwABAAAB6jAiBCCM6u3E
'' SIG '' /Uk4WfFVrcgT+2yr8WlE98+Rhmj2m2wivWP7yDANBgkq
'' SIG '' hkiG9w0BAQsFAASCAgB+sTaPiv+qnG/tkWOf9QoyGxVd
'' SIG '' FWUQUoUBQn08rOecsYUSE49GoLSIz+3pjbSyTyWsiJD5
'' SIG '' zU7br4lnX+32tkZW1Y38+yxMk0zKV1QxlTd7EiIupdMq
'' SIG '' HbQhQfKZxBeLorPRYTUVdWmWdbTZmiguieir4lLjL4hq
'' SIG '' SJMw42JZVAXAqE26y7lN4+amiOx9ZWL6kBw3cQmZfNy/
'' SIG '' cS3C1mVgFWa95TiRlqZ/2+Gdw6j1IYbXO0tw5nVdl5ek
'' SIG '' zBXe1x/X+YeDD0VFOquCGRTrUI+v0kb4ByD3B3k4E6Qv
'' SIG '' hxEzvnNVOYL/eCzzEL2ZbZr4zvjs2Rk5lQMi0gKtzdxT
'' SIG '' SJXoPfy7crg7X1d+97mEas8s8Ysm9wf3/ks4gWiP1iWU
'' SIG '' DprPppS5eVjRP501Jl+BYNOCAlRwsEFpiJRIIfouSz4W
'' SIG '' egw/bGE7oXQSRbDrPsdgOc8No1XplcBS+Xg3CdL+hrKk
'' SIG '' y5FlNN10cPjJIluaZo8DmRAHTQ9u02nz8a8PZ+ZstTyE
'' SIG '' 96rtm9IPKos7Z7bNVcTkT7uWo/9H1NxknJdtzhBvSfQ2
'' SIG '' fH2c78gpvWoGRzOqOjaNdHa1A0tam2ndcDz6aWaSdz+J
'' SIG '' 9ypKLjkthNpLi86ys4A9mQGVpA44TKZruGXh5RwkWJls
'' SIG '' iuOo5LZMyZeb14tiEhbv0FQKUFwcUvztJqJQr3gr8w==
'' SIG '' End signature block
