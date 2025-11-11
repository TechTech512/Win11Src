MessageIdTypedef=DWORD

MessageId=0x1
SymbolicName=MSG_TXN_COMMIT_FAILED
Language=English
The Transaction (UOW=%1, Description='%3') was unable to be committed, and instead rolled back; this was due to an error message returned by CLFS while attempting to write a Prepare or Commit record for the Transaction.  The CLFS error returned was: %4.
.

MessageId=0x2
SymbolicName=MSG_TXN_BLOCKED_FREEZE
Language=English
The Transaction (UOW=%1, Description='%3') blocked a Freeze from completing.  Freeze is necessary to ensure that a VSS snapshot is transactionally consistent.  The ResourceManager (RmId=%4, Description='%6') may not be functioning correctly, since it did not allow the transaction to drain in the allotted time.  Contact the vendor for that ResourceManager for assistance.
.

MessageId=0x3
SymbolicName=MSG_TXN_HEURISTIC_ABORT
Language=English
The transaction (UOW=%1, Description='%3') was heuristically aborted and forgotten from the TransactionManager (TmId=%4, LogPath=%6) so that the TransactionManager can continue to make forward progress.  This may cause data corruption in any subordinate ResourceManagers or Transactionmanager.
.

MessageId=0x4
SymbolicName=MSG_TXN_LOG_TAIL_BLOCKED
Language=English
The TransactionManager (TmId=%1, LogPath=%3) has failed to advance its log tail, due to the transaction (UOW=%4, Description='%6') being unresolved for some time.  The transaction must be forced to resolve in order for the TransactionManager to continue to provide transactional services.  Forcing the incorrect outcome may cause data corruption in any subordinate ResourceManagers or Transactionmanagers.
.
