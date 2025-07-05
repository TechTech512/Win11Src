' Windows Installer utility to report the differences between two databases
' For use with Windows Scripting Host, CScript.exe only, lists to stdout
' Copyright (c) Microsoft Corporation. All rights reserved.
' Simply generates a transform between the databases and then view the transform
'
Option Explicit

Const icdLong       = 0
Const icdShort      = &h400
Const icdObject     = &h800
Const icdString     = &hC00
Const icdNullable   = &h1000
Const icdPrimaryKey = &h2000
Const icdNoNulls    = &h0000
Const icdPersistent = &h0100
Const icdTemporary  = &h0000

Const msiOpenDatabaseModeReadOnly     = 0
Const msiOpenDatabaseModeTransact     = 1
Const msiOpenDatabaseModeCreate       = 3
Const iteViewTransform       = 256

If Wscript.Arguments.Count < 2 Then
	Wscript.Echo "Windows Installer database difference utility" &_
		vbNewLine & " Generates a temporary transform file, then display it" &_
		vbNewLine & " 1st argument is the path to the original installer database" &_
		vbNewLine & " 2nd argument is the path to the updated installer database" &_
		vbNewLine &_
		vbNewLine & "Copyright (C) Microsoft Corporation.  All rights reserved."
	Wscript.Quit 1
End If

' Cannot run with GUI script host, as listing is performed to standard out
If UCase(Mid(Wscript.FullName, Len(Wscript.Path) + 2, 1)) = "W" Then
	WScript.Echo "Cannot use WScript.exe - must use CScript.exe with this program"
	Wscript.Quit 2
End If

' Connect to Windows Installer object
On Error Resume Next
Dim installer : Set installer = Nothing
Set installer = Wscript.CreateObject("WindowsInstaller.Installer") : CheckError

' Create path for temporary transform file
Dim WshShell : Set WshShell = Wscript.CreateObject("Wscript.Shell") : CheckError
Dim tempFilePath:tempFilePath = WshShell.ExpandEnvironmentStrings("%TEMP%") & "\diff.tmp"

' Open databases, generate transform, then list transform
Dim database1 : Set database1 = installer.OpenDatabase(Wscript.Arguments(0), msiOpenDatabaseModeReadOnly) : CheckError
Dim database2 : Set database2 = installer.OpenDatabase(Wscript.Arguments(1), msiOpenDatabaseModeReadOnly) : CheckError
Dim different : different = Database2.GenerateTransform(Database1, tempFilePath) : CheckError
If different Then
	database1.ApplyTransform tempFilePath, iteViewTransform + 0 : CheckError' should not need error suppression flags
	ListTransform database1
End If

' Open summary information streams and compare them
Dim sumInfo1 : Set sumInfo1 = database1.SummaryInformation(0) : CheckError
Dim sumInfo2 : Set sumInfo2 = database2.SummaryInformation(0) : CheckError
Dim iProp, value1, value2
For iProp = 1 to 19              
	value1 = sumInfo1.Property(iProp) : CheckError
	value2 = sumInfo2.Property(iProp) : CheckError
	If value1 <> value2 Then
		Wscript.Echo "\005SummaryInformation   [" & iProp & "] {" & value1 & "}->{" & value2 & "}"
		different = True
	End If
Next
If Not different Then Wscript.Echo "Databases are identical"
Wscript.Quit 0

Function DecodeColDef(colDef)
	Dim def
	Select Case colDef AND (icdShort OR icdObject)
	Case icdLong
		def = "LONG"
	Case icdShort
		def = "SHORT"
	Case icdObject
		def = "OBJECT"
	Case icdString
		def = "CHAR(" & (colDef AND 255) & ")"
	End Select
	If (colDef AND icdNullable)   =  0 Then def = def & " NOT NULL"
	If (colDef AND icdPrimaryKey) <> 0 Then def = def & " PRIMARY KEY"
	DecodeColDef = def
End Function

Sub ListTransform(database)
	Dim view, record, row, column, change
	On Error Resume Next
	Set view = database.OpenView("SELECT * FROM `_TransformView` ORDER BY `Table`, `Row`")
	If Err <> 0 Then Wscript.Echo "Transform viewing supported only in builds 4906 and beyond of MSI.DLL" : Wscript.Quit 2
	view.Execute : CheckError
	Do
		Set record = view.Fetch : CheckError
		If record Is Nothing Then Exit Do
		change = Empty
		If record.IsNull(3) Then
			row = "<DDL>"
			If NOT record.IsNull(4) Then change = "[" & record.StringData(5) & "]: " & DecodeColDef(record.StringData(4))
		Else
			row = "[" & Join(Split(record.StringData(3), vbTab, -1), ",") & "]"
			If record.StringData(2) <> "INSERT" AND record.StringData(2) <> "DELETE" Then change = "{" & record.StringData(5) & "}->{" & record.StringData(4) & "}"
		End If
		column = record.StringData(1) & " " & record.StringData(2)
		if Len(column) < 24 Then column = column & Space(24 - Len(column))
		WScript.Echo column, row, change
	Loop
End Sub

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
'' SIG '' MIImQwYJKoZIhvcNAQcCoIImNDCCJjACAQExDzANBglg
'' SIG '' hkgBZQMEAgEFADB3BgorBgEEAYI3AgEEoGkwZzAyBgor
'' SIG '' BgEEAYI3AgEeMCQCAQEEEE7wKRaZJ7VNj+Ws4Q8X66sC
'' SIG '' AQACAQACAQACAQACAQAwMTANBglghkgBZQMEAgEFAAQg
'' SIG '' cwzPLmwXndAasgrSds09lWoEa+nEByy+weD1dY9VbGig
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
'' SIG '' BAGCNwIBFTAvBgkqhkiG9w0BCQQxIgQgHCTPOFkSQWVL
'' SIG '' rWvDdzO6kh973pAJZgDZdMlwiQfCFnYwWgYKKwYBBAGC
'' SIG '' NwIBDDFMMEqgJIAiAE0AaQBjAHIAbwBzAG8AZgB0ACAA
'' SIG '' VwBpAG4AZABvAHcAc6EigCBodHRwOi8vd3d3Lm1pY3Jv
'' SIG '' c29mdC5jb20vd2luZG93czANBgkqhkiG9w0BAQEFAASC
'' SIG '' AQCCvhTBptb/QWdCG1FT7lgy8/H73glhNZhX4PaMKpHi
'' SIG '' iUx9O2T45waAQoVQP50nnXfPBYZEE/9Q0jTVeyBRNQ04
'' SIG '' oqX4C5dtkQAYRzIWQmGlNxcZFNk+B1bBMFCBYt0b9mRS
'' SIG '' UfYla0hHRv+u4YNrevzUssHAaRr7lFiADigBbfUSxRwn
'' SIG '' BZgKY+YjxP2eOkrnWcpzuadCznVtwnDpF2bfiaA9RKu+
'' SIG '' Sd75kmWFtFiCw0hutJCnyDq+k6S3bn0UYk4fj/PlIkg7
'' SIG '' snJlJnKTmof+VKb3M+AAjAWBo6oJNlZo4ySXtZJADv5g
'' SIG '' L1YR4dIMtI/cb85iC0mdwESosRko5RoT1VzSoYIXlzCC
'' SIG '' F5MGCisGAQQBgjcDAwExgheDMIIXfwYJKoZIhvcNAQcC
'' SIG '' oIIXcDCCF2wCAQMxDzANBglghkgBZQMEAgEFADCCAVIG
'' SIG '' CyqGSIb3DQEJEAEEoIIBQQSCAT0wggE5AgEBBgorBgEE
'' SIG '' AYRZCgMBMDEwDQYJYIZIAWUDBAIBBQAEIAAdpiuviiTG
'' SIG '' Qo9mlE4Qf+Hz42pmTZhC33Z9Yh6kDbmLAgZmvh6lpC0Y
'' SIG '' EzIwMjQwOTA1MDkxNzE4LjgwOVowBIACAfSggdGkgc4w
'' SIG '' gcsxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5n
'' SIG '' dG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVN
'' SIG '' aWNyb3NvZnQgQ29ycG9yYXRpb24xJTAjBgNVBAsTHE1p
'' SIG '' Y3Jvc29mdCBBbWVyaWNhIE9wZXJhdGlvbnMxJzAlBgNV
'' SIG '' BAsTHm5TaGllbGQgVFNTIEVTTjo4OTAwLTA1RTAtRDk0
'' SIG '' NzElMCMGA1UEAxMcTWljcm9zb2Z0IFRpbWUtU3RhbXAg
'' SIG '' U2VydmljZaCCEe0wggcgMIIFCKADAgECAhMzAAAB7eFf
'' SIG '' y9X3pV1zAAEAAAHtMA0GCSqGSIb3DQEBCwUAMHwxCzAJ
'' SIG '' BgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAw
'' SIG '' DgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3Nv
'' SIG '' ZnQgQ29ycG9yYXRpb24xJjAkBgNVBAMTHU1pY3Jvc29m
'' SIG '' dCBUaW1lLVN0YW1wIFBDQSAyMDEwMB4XDTIzMTIwNjE4
'' SIG '' NDU0MVoXDTI1MDMwNTE4NDU0MVowgcsxCzAJBgNVBAYT
'' SIG '' AlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQH
'' SIG '' EwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29y
'' SIG '' cG9yYXRpb24xJTAjBgNVBAsTHE1pY3Jvc29mdCBBbWVy
'' SIG '' aWNhIE9wZXJhdGlvbnMxJzAlBgNVBAsTHm5TaGllbGQg
'' SIG '' VFNTIEVTTjo4OTAwLTA1RTAtRDk0NzElMCMGA1UEAxMc
'' SIG '' TWljcm9zb2Z0IFRpbWUtU3RhbXAgU2VydmljZTCCAiIw
'' SIG '' DQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAKgwwmyS
'' SIG '' uupqnJwvE8LUfvMPuDzw2lsRpDpKNxMhFvMJXJhA2zPx
'' SIG '' NovWmoVMQA8vVfuiMvj8RoRb5SM2pmz9rIzJbhgikU9k
'' SIG '' /bHgUExUJ12x4XaL5owyMMeLQtxNBnEzazeYUysJkBZJ
'' SIG '' 8thdgMiKYUHPyPSgtYbLdWAuYFMozjEuq/sNlTPwHZKg
'' SIG '' CZsS2nraeBKXSE6g3vdIXAT5jbhK8ZAxaHKSkb69cPBy
'' SIG '' la/AN75OCestHsBNEVc3klLbp2bbLLpJgUxFicwTd0wc
'' SIG '' JD9RAhBA0LycuYi90qQChYQxe0mwYSjdCszZLZIG/g+k
'' SIG '' dHNG6TNO0/5QBx4bEz0nKvBRA/k4ISZbphyETJENLA/i
'' SIG '' FT1/sHQDKHXg/D28mjuN7A2N4w8iSad7ItKLSu6/ajH/
'' SIG '' FEa1wn3IE0LkFpGS2PPuy09qiNH48MDZ+4G0KjzEqWS3
'' SIG '' neZRvsBj4JkceqEubvql0wXoEe/ZO/CVUF5BE3bZeNpV
'' SIG '' VHAKCOAmc17C3s96NyulSfSocuAur7UE3UPNi6RaROvv
'' SIG '' BPTOXSJev422pSRZI6dZF97w3bW0Hq6/dWRbycV0KG1t
'' SIG '' tlnPbil4u0kRm42s3xd/09M8zNlcMkEjURyJH/3VBwah
'' SIG '' kWZVsVVvatQgCzTX5mR7C9uGYZUN59f2hkbj8riAZSxO
'' SIG '' 9Nb6vUlkzFRPYzCpAgMBAAGjggFJMIIBRTAdBgNVHQ4E
'' SIG '' FgQUzhvw7PfeECoER8qUBl/Q0qHgIhkwHwYDVR0jBBgw
'' SIG '' FoAUn6cVXQBeYl2D9OXSZacbUzUZ6XIwXwYDVR0fBFgw
'' SIG '' VjBUoFKgUIZOaHR0cDovL3d3dy5taWNyb3NvZnQuY29t
'' SIG '' L3BraW9wcy9jcmwvTWljcm9zb2Z0JTIwVGltZS1TdGFt
'' SIG '' cCUyMFBDQSUyMDIwMTAoMSkuY3JsMGwGCCsGAQUFBwEB
'' SIG '' BGAwXjBcBggrBgEFBQcwAoZQaHR0cDovL3d3dy5taWNy
'' SIG '' b3NvZnQuY29tL3BraW9wcy9jZXJ0cy9NaWNyb3NvZnQl
'' SIG '' MjBUaW1lLVN0YW1wJTIwUENBJTIwMjAxMCgxKS5jcnQw
'' SIG '' DAYDVR0TAQH/BAIwADAWBgNVHSUBAf8EDDAKBggrBgEF
'' SIG '' BQcDCDAOBgNVHQ8BAf8EBAMCB4AwDQYJKoZIhvcNAQEL
'' SIG '' BQADggIBAJ3WArZF354YvR4eL6ITr+oNjyxtuw7h6Zqd
'' SIG '' ynoo837GrlkBq2IFHiOZFGGb71WKTQWjQMtaL83bxsUj
'' SIG '' t1djDT2ne8KKluPLgSiJ+bQ253v/hTfSL37tG9btc5De
'' SIG '' vHfv5Tu+r2WTrJikYI2nSOUnXzz8K5E+Comd+rkR15p8
'' SIG '' fYCgbjqEpZN4HsO5dqwa3qykk56cZ51Kt7fgxZmp5MhD
'' SIG '' Sto4i1mcW4YPLj7GgPWpHPZBb67aAIdobwBCOFhQzi5O
'' SIG '' L23qS22PpztdqavbOta5x4OHPuwou20tMnvCzlisDYjx
'' SIG '' xOVswB/YpbQZWMptgZ34tkZ24Qrv/t+zgZSQypznUWw1
'' SIG '' 0bWf7OBzvMe7agYZ4IGDizxlHRkXLHuOyCb2xIUIpDkK
'' SIG '' xsC+Wv/rQ12TlN4xHwmzaQ1SJy7YKpnTfzfdOy9OCTuI
'' SIG '' PUouB9LXocS+M3qbhUokqCMns4knNpu1LglCBScmshl/
'' SIG '' KiyTgPXytmeL2lTA3TdaBOZ3XRZPCJk67iDxSfqIpw8x
'' SIG '' j+IWpO7ie2TMVTEEGlsUbqTUIg1maiKsRaYK0beXJnYh
'' SIG '' 12aO0h59OQi8ZZvgnHPPuXab8TaQY6LEMkexqFlWbCyg
'' SIG '' 2+HLmS7+KdT751cfPD6GW+pNIVPz2sgVWFyaxY8Mk81F
'' SIG '' JKkyGgnfdXZlr+WQpxuRQzRJtCBL2qx3MIIHcTCCBVmg
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
'' SIG '' MScwJQYDVQQLEx5uU2hpZWxkIFRTUyBFU046ODkwMC0w
'' SIG '' NUUwLUQ5NDcxJTAjBgNVBAMTHE1pY3Jvc29mdCBUaW1l
'' SIG '' LVN0YW1wIFNlcnZpY2WiIwoBATAHBgUrDgMCGgMVAO4d
'' SIG '' rIpMJpixjEmH6hZPHq5U8XD5oIGDMIGApH4wfDELMAkG
'' SIG '' A1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAO
'' SIG '' BgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29m
'' SIG '' dCBDb3Jwb3JhdGlvbjEmMCQGA1UEAxMdTWljcm9zb2Z0
'' SIG '' IFRpbWUtU3RhbXAgUENBIDIwMTAwDQYJKoZIhvcNAQEL
'' SIG '' BQACBQDqg6H9MCIYDzIwMjQwOTA1MDMyMDI5WhgPMjAy
'' SIG '' NDA5MDYwMzIwMjlaMHcwPQYKKwYBBAGEWQoEATEvMC0w
'' SIG '' CgIFAOqDof0CAQAwCgIBAAICEM0CAf8wBwIBAAICFLEw
'' SIG '' CgIFAOqE830CAQAwNgYKKwYBBAGEWQoEAjEoMCYwDAYK
'' SIG '' KwYBBAGEWQoDAqAKMAgCAQACAwehIKEKMAgCAQACAwGG
'' SIG '' oDANBgkqhkiG9w0BAQsFAAOCAQEAotMPhRITxPb6f7nS
'' SIG '' 8L7+VTatrqqGz1AoUZOwk8Hma4husXOAD5N+LgGzHON1
'' SIG '' ygRTjvIEt9k1FGz4Z2GhytAa+Ixvapu5idRYXiKe/h/1
'' SIG '' lzJJaYxmnH/rlfgkVZGnJniEzQR3QhCkdTzeh1I6fuFq
'' SIG '' /qzDVhvZxcxxYSeES4XEaN/OUtCIFIc5u6axR1+192Q7
'' SIG '' jvRBVvROSYZ4lLeu30zJH0JlnTNkba1+s35pbO4c5uOt
'' SIG '' Hnn9l2Py0agaeNiGlRoNdYEnQzNb2Na3mmTPXd4afJj7
'' SIG '' gZy9+9gf2XmjgqoZlfMB5jwTb0LGXwDHsQL+Z+jC87J8
'' SIG '' L0xltZKwZRiIAWCGfDGCBA0wggQJAgEBMIGTMHwxCzAJ
'' SIG '' BgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAw
'' SIG '' DgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3Nv
'' SIG '' ZnQgQ29ycG9yYXRpb24xJjAkBgNVBAMTHU1pY3Jvc29m
'' SIG '' dCBUaW1lLVN0YW1wIFBDQSAyMDEwAhMzAAAB7eFfy9X3
'' SIG '' pV1zAAEAAAHtMA0GCWCGSAFlAwQCAQUAoIIBSjAaBgkq
'' SIG '' hkiG9w0BCQMxDQYLKoZIhvcNAQkQAQQwLwYJKoZIhvcN
'' SIG '' AQkEMSIEIONIyJGZKnLyF0xl6DOLeEjYouHxhr2JTt0D
'' SIG '' 81hK/zwjMIH6BgsqhkiG9w0BCRACLzGB6jCB5zCB5DCB
'' SIG '' vQQgjS4NaDQIWx4rErcDXqXXCsjCuxIOtdnrFJ/QXjFs
'' SIG '' yjkwgZgwgYCkfjB8MQswCQYDVQQGEwJVUzETMBEGA1UE
'' SIG '' CBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEe
'' SIG '' MBwGA1UEChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMSYw
'' SIG '' JAYDVQQDEx1NaWNyb3NvZnQgVGltZS1TdGFtcCBQQ0Eg
'' SIG '' MjAxMAITMwAAAe3hX8vV96VdcwABAAAB7TAiBCBWujyV
'' SIG '' BpJu2VowVFXaHNZgncNXdqC7/Z4bbREJi35jTDANBgkq
'' SIG '' hkiG9w0BAQsFAASCAgABhvP8+89ovPEyvwAMi2gIaCzQ
'' SIG '' ZL4p3pe1OWnqaq/qu17EVBAO7+EP31+E9FzFVb4i1IWz
'' SIG '' 8nnO+6ZJmQQRsZInrFmjik59hQSuJTY3vPBmiqMkkFZr
'' SIG '' 02cZG7wByVZw069ivW05noaNi/7SPPCSs6GUAIEvYuWp
'' SIG '' SokP1pqFcZuBlAQ8kAlw2AQG129VBuEd/VSMFTV45mBq
'' SIG '' hB5Hr5Bf0QhfgC5+yLjkiE+KvwEagYjmgKOPlCPOg/UL
'' SIG '' RQXwo8BAcKvrG5lLj8uSGOncl0dAgi1t1e6x9EGmo4Rg
'' SIG '' KaqUKycFYMGlUoKyr4a0CfxO3ImrAYUiI0tUj7aGTUCi
'' SIG '' DpqOJcLVwOC6vGDU2XkvmNbcBmgf+CX5zj1BxZAzMfaS
'' SIG '' qWMXXHgYvV8f280td2MBLIiHlPL3xFR8dw+a7x0vOcYI
'' SIG '' MWz1hOLt9B9G7CQOBQYaJjyKDxoSkrDrNaZUwziD7m33
'' SIG '' /fMYTBqkQbqcH04rAMX7Uh51r8s9JeZ+rsCZW+6Ak/9b
'' SIG '' owa9n9IhtvGMxnbxBJawIXWc2+/buk/ioEBUTCDIHxME
'' SIG '' ONyMapdOzLpb3sqqYgUtL8rOSM7JXfo3tS4MNwHQYAEh
'' SIG '' 7kqhj0hPM1ttz6IHYBjDOLWuu3Teh+d4XsDGqLFmBBjl
'' SIG '' cfGwuru3D/5TeyASBv0YTfEnc+9xB8K/Q/QChOnAaw==
'' SIG '' End signature block
