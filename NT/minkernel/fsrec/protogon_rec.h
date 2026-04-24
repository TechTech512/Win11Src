typedef struct _REFS_FS_VERSION {
	unsigned char Major;
	unsigned char Minor;
} REFS_FS_VERSION;

typedef struct _REFS_BOOT_SECTOR {
	unsigned char Jump[3];
	unsigned char Oem[8];
	unsigned char MustBeZero[5];
	unsigned long Identifier;
	unsigned short Length;
	unsigned short Checksum;
	unsigned long long NumberSectors;
	unsigned long BytesPerSector;
	unsigned long SectorsPerCluster;
	REFS_FS_VERSION Version;
	unsigned char UnusedUchars[2];
	unsigned long Flags1;
	unsigned long Flags2;
	unsigned long UnusedUlong1;
	long long SerialNumber;
	long long UnusedLarge1;
	long long UnusedLarge2;
	long long UnusedLarge3;
	unsigned char UnusedAkinBootstrap[424];
} REFS_BOOT_SECTOR;

typedef REFS_BOOT_SECTOR *PREFS_BOOT_SECTOR;



