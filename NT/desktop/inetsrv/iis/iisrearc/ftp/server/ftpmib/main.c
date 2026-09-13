/*++ BUILD Version: 0001

Copyright (c) 1991  Microsoft Corporation

Module Name:

    main.c

Abstract:

    SNMP Extension Agent for FTP Service on Windows NT.

Created:

  MuraliK   22-Feb-1995

Revision History:
  Murali R. Krishnan (MuraliK)  16-Nov-1995 Removed undoc apis

--*/

/************************************************************
 *   Include Headers
 ************************************************************/

#include <windows.h>
#include <snmp.h>
#include <lm.h>
#include <iisinfo.h>

#include "ftpd.h"
#include "mib.h"


/************************************************************
 *   Forward Declarations
 ************************************************************/

extern int __cdecl FtpQueryCounters(LPFTP_STATISTICS_0 pStats);

extern UINT
MIB_Stat(
   IN OUT RFC1157VarBind  * pRfcVarBinding,
   IN UINT                  pduAction,
   IN struct _MIB_ENTRY   * pMibeCurrent,
   IN struct _MIB_ENTRIES * pMibEntries,
   IN LPVOID                pStatistics
   );

extern UINT
ResolveVarBind(
   IN OUT RFC1157VarBind   * pRfcVarBinding,
   IN BYTE                   pduAction,
   IN LPVOID                 pStatistics,
   IN LPMIB_ENTRIES          pMibEntries
  );

/************************************************************
 *   Variable Definitions
 ************************************************************/

//
// Definition of the MIB objects
//

//
//  The InternetServer section of the OID tree is organized as follows:
//
//      iso(1)
//          org(3)
//              dod(6)
//                  internet(1)
//                      private(4)
//                          enterprises(1)
//                              microsoft(311)
//                                  software(1)
//                                      InternetServer(7)
//                                          InetSrvCommon(1)
//                                              InetSrvStatistics(1)
//                                          FtpServer(2)
//                                              FtpStatistics(1)
//                                          HttpServer(3)
//                                              HttpStatistics(1)
//                                          GopherServer(4)
//                                              GopherStatistics(1)
//

static UINT   sg_rguiPrefix[] =  { 1, 3, 6, 1, 4, 1, 311, 1, 7, 2 };
static AsnObjectIdentifier MIB_OidPrefix =
                      OID_FROM_UINT_ARRAY( sg_rguiPrefix);

# define FTP_PREFIX_OID_LENGTH    ( GET_OID_LENGTH( MIB_OidPrefix))

# define FTP_STATISTICS_OID_SUFFIX            ( 1)


//
// Following is the global description of all MIB Entries ( Mibe s) for
//   Ftp Service.
// Definition appears as:
//  Mibe( MIB Entry Name, Index in MIB Block, FtpStatisticsField)
//
//   Incidentally, MIB Entry suffix coincides with the entry name in OID Tree
//
//  Any New MIB should be added here. and dont change use of after this defn.
//

# define ALL_MIB_ENTRIES()    \
Mibe( TotalBytesSent_HighWord,     1,    TotalBytesSent.HighPart)       \
Mibe( TotalBytesSent_LowWord,      2,    TotalBytesSent.LowPart)        \
Mibe( TotalBytesReceived_HighWord, 3,    TotalBytesReceived.HighPart)   \
Mibe( TotalBytesReceived_LowWord,  4,    TotalBytesReceived.LowPart)    \
Mibe( TotalFilesSent,              5,    TotalFilesSent)                \
Mibe( TotalFilesReceived,          6,    TotalFilesReceived)            \
Mibe( CurrentAnonymousUsers,       7,    CurrentAnonymousUsers)         \
Mibe( CurrentNonAnonymousUsers,    8,    CurrentNonAnonymousUsers)      \
Mibe( TotalAnonymousUsers,         9,    TotalAnonymousUsers)           \
Mibe( TotalNonAnonymousUsers,      10,   TotalNonAnonymousUsers)        \
Mibe( MaxAnonymousUsers,           11,   MaxAnonymousUsers)             \
Mibe( MaxNonAnonymousUsers,        12,   MaxNonAnonymousUsers)          \
Mibe( CurrentConnections,          13,   CurrentConnections)            \
Mibe( MaxConnections,              14,   MaxConnections)                \
Mibe( ConnectionAttempts,          15,   ConnectionAttempts)            \
Mibe( LogonAttempts,               16,   LogonAttempts)                 \
Mibe( ServiceUptime,               17,   ServiceUptime)


//
// Individual OID Definitions.
//   All Leaf variables should have a zero appended to their OID to indicate
//   that it is the only instance of this variable and that it exists.
//  Declare just the id's starting from next to the prefix given above.
//


//
// Few Convenience Macros for MIB entries addition.
//

# define MIB_VAR_NAME( NameSuffix)       MIB_ ## NameSuffix

# define DEFINE_MIBOID( NameSuffix, uiArray)   \
           UINT MIB_VAR_NAME( NameSuffix)[] = uiArray

# define DEFINE_MIBOID_LEAF( NameSuffix, NodeNumber) \
           UINT MIB_VAR_NAME( NameSuffix)[] = \
                          { FTP_STATISTICS_OID_SUFFIX, ( NodeNumber), 0 }

//
// Define all the OIDs. First define the higher level node and then leaves.
//
DEFINE_MIBOID( Statistics,     { FTP_STATISTICS_OID_SUFFIX} );

//
//  Define the Leaf OIDs.
//
# define Mibe( NameSuffix, Index, FieldName)  \
     DEFINE_MIBOID_LEAF( NameSuffix, Index);

//
// Expand the macro ALL_MIB_ENTRIES to obtain definitions of MIB Leafs.
//
ALL_MIB_ENTRIES()

# undef Mibe


//
//  MIB Variable definition
//

//
// Define Mibe()  to be for variable definitions of counters.
//  Note that the comma is appearing before a new counter name. It is used
//   for structure initialization.
//

# define OFFSET_IN_FTP_STATISTICS( Field)    \
     FIELD_OFFSET( FTP_STATISTICS_0,   Field)

# define Mibe( NameSuffix, Index, Field)        \
     , MIB_COUNTER( OID_FROM_UINT_ARRAY( MIB_VAR_NAME( NameSuffix)), \
                    OFFSET_IN_FTP_STATISTICS(Field),              \
                    MIB_Stat)

static MIB_ENTRY  sg_rgFtpMib[] = {

    //
    // Statistics
    //

    MIB_ENTRY_HEADER( OID_FROM_UINT_ARRAY( MIB_VAR_NAME( Statistics)))
    ALL_MIB_ENTRIES()
};

# undef Mibe




static MIB_ENTRIES  sg_FtpMibs =
  {
    &MIB_OidPrefix,
    ( sizeof( sg_rgFtpMib) / sizeof( MIB_ENTRY)),
    sg_rgFtpMib
  };


/************************************************************
 *  Entry Points of SNMP Extension DLL For Ftp Service
 ************************************************************/

//
//  Extension Agent DLLs need access to elapsed time agent has been active.
//  This is implemented by initializing the Extension Agent with a time zero
//  reference, and allowing the agent to compute elapsed time by subtracting
//  the time zero reference from the current system time.  This example
//  Extension Agent implements this reference with dwTimeZero.
//

DWORD dwTimeZero = 0;


BOOL
SnmpExtensionInit(
    IN  DWORD                 dwTimeZeroReference,
    OUT HANDLE              * phPollForTrapEvent,
    OUT AsnObjectIdentifier * pAsnOidSupportedView
    )
/*++
  Description:
     The Extension Agent DLLs provide this entry point SnmpExtensionInit()
     to co-ordinate the initializations of the extension agent and the
     extendible  agent.
     The Extendible agent provides extension agent with a time zero reference.
     The Extension Agent provides Extendible agent with an Event Handle
         for communicating occurences of traps.
     The Extension Agent also provides Extendible agent with an ObjectId
         representing the root of the MIB structure
         that it (extension) supports.

  Arguments:
     dwTimeZeroReference    DWORD containing the Time Zero Reference for sync.
     phPollForTrapEvent     pointer to handle which on successful return
                             may contain an event handle to be polled for
                             traps.
     pAsnOidSupportedView   pointer to ASN ( Abstract Syntax Notation OID)
                             that contains the oid representing root of the
                             MIB structure.

  Returns:
    TRUE on success and FALSE if there is any failure.
--*/
{

    //
    //  Record the time reference provided by the Extendible Agent.
    //

    dwTimeZero = dwTimeZeroReference;

    //
    //  Indicate the MIB view supported by this Extension Agent, an object
    //  identifier representing the sub root of the MIB that is supported.
    //

    *pAsnOidSupportedView = MIB_OidPrefix; // NOTE!  structure copy

    //
    // Though the following is a handle, dont use INVALID_HANDLE_VALUE ( -1)
    //  because that constant is only for few people ( Win32). But all through
    //  NT invalid handle value is NULL ( 0).
    //

    *phPollForTrapEvent = NULL;

    //
    //  Indicate that Extension Agent initialization was sucessfull.
    //

    return ( TRUE);

}   // SnmpExtensionInit()


BOOL
SnmpExtensionTrap(
    OUT AsnObjectIdentifier * pAsnOidEnterprise,
    OUT AsnInteger          * pAsniGenericTrap,
    OUT AsnInteger          * pAsniSpecificTrap,
    OUT AsnTimeticks        * pAsnTimeStamp,
    OUT RFC1157VarBindList  * pRfcVariableBindings
    )
/*++
  Description:
     This function is used to communicate traps to the Extendible Agent.
     The Extendible Agent will invoke this entry point when the trap event
      ( supplied at the initialization time) is asserted, which indicates
      that zero or more traps had occured.
     The Extendible agent will repeatedly query this function till this
      function returns FALSE.

  Arguments:
    pAsnOidEnterprise      pointer to ASN OID for Enterprise, indicating
                             original enterprise generating trap.
    pAsniGenericTrap       pointer to ASN Integer which on return will
                             contain the indication of the generic trap.
    pAsniSpecificTrap      pointer to ASN Integer which on return will
                             contain the specific trap generated.
    pAsnTimeStamp          pointer to ASN containing the received Time-Stamp.
    pRfcVariableBindings   pointer to RFC 1157 compliant variable bindings.


  Returns:
    TRUE if success and there are more traps to be queried.
    FALSE if all traps are answered and work done.

--*/
{

    //
    //  We don't support traps (yet).
    //

    return ( FALSE);

}   // SnmpExtensionTrap()


BOOL
SnmpExtensionQuery(
    IN BYTE                     bRequestType,
    IN OUT RFC1157VarBindList * pRfcVariableBindings,
    OUT AsnInteger         *    pAsniErrorStatus,
    OUT AsnInteger         *    pAsniErrorIndex
    )
/*++
  Description:
    This function is called by Extendible Agent to resolve the SNMP requests
    for queries on MIB Variables in the Extension Agent's supported MIB view.
    ( which was supplied at initialization time).
    The Request Type is GET, GETNEXT, and SET.

  Arguments:
    bRequestType    byte containing the type of request.
                    It can be one of
                     ASN_RFC1157_GETREQUEST
                     ASN_RFC1157_GETNEXTREQUEST
                     ASN_RFC1157_SETREQUEST

    pRfcVariableBindings
                   pointer to RFC 1157 compliant variable bindings.

    pAsniErrorStatus
                   pointer to ASN Integer for Error Status

    pAsniErrorIndex
                  pointer to ASN INteger giving the index for error.

  Returns:
    TRUE on success and FALSE on failure.
--*/

{
    FTP_STATISTICS_0    FtpStatistics;
    int                 Status;


    //
    //  Try to query the statistics now so we'll have a consitent
    //  view across all variable bindings.
    //
    Status = FtpQueryCounters(
                             &FtpStatistics );

    if ( Status < 0 )
    {

        //
        //  Couldn't collect statistics; report no-such-name for every
        //  binding we were asked about.
        //

        *pAsniErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
        *pAsniErrorIndex  = 0;
    }
    else
    {

        _try
        {
            //
            //  Iterate through the variable bindings list to resolve individual
            //  variable bindings.
            //

            RFC1157VarBind * pVarBinding;

            for( pVarBinding = pRfcVariableBindings->list;
                pVarBinding < ( pRfcVariableBindings->list +
                                pRfcVariableBindings->len);
                pVarBinding++ )
           {

                *pAsniErrorStatus = ResolveVarBind( pVarBinding,
                                                    bRequestType,
                                                    &FtpStatistics,
                                                    &sg_FtpMibs);

                //
                //  Test and handle case where Get Next past end of MIB view
                //  supported by this Extension Agent occurs.  Special
                //  processing is required to communicate this situation to
                //  the Extendible Agent so it can take appropriate action,
                //  possibly querying other Extension Agents.
                //

                if(( *pAsniErrorStatus == SNMP_ERRORSTATUS_NOSUCHNAME ) &&
                   ( bRequestType == MIB_GETNEXT ) )
                {


                    *pAsniErrorStatus = SNMP_ERRORSTATUS_NOERROR;

                    //
                    //  Modify variable binding of such variables so the OID
                    //  points just outside the MIB view supported by this
                    //  Extension Agent.  The Extendible Agent tests for this,
                    //  and takes appropriate action.
                    //

                    SNMP_oidfree( &pVarBinding->name );
                    SNMP_oidcpy( &pVarBinding->name, &MIB_OidPrefix);
                    pVarBinding->name.ids[ FTP_PREFIX_OID_LENGTH - 1]++;
                }

                //
                //  If an error was indicated, communicate error status and error
                //  index to the Extendible Agent.  The Extendible Agent will
                //  ensure that the origional variable bindings are returned in
                //  the response packet.

                *pAsniErrorIndex =
                  (( *pAsniErrorStatus != SNMP_ERRORSTATUS_NOERROR ) ?
                   (( (int)(pVarBinding - pRfcVariableBindings->list) ) + 1) : 0);

            } // for

        } // try
        _except ( EXCEPTION_EXECUTE_HANDLER   )
        {

            //
            //  For now do nothing.
            //

        }
    }

    return ( SNMPAPI_NOERROR);

}   // SnmpExtensionQuery()

