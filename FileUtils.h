/*
 ============================================================================
 Name		: FileUtils.h
 Author	  : artem78
 Version	 : 2.1
 Copyright   : 
 Description : Collection of file utilities
 ============================================================================
 */

#ifndef FILEUTILS_H
#define FILEUTILS_H

// Includes

#include <e32base.h>	// For CActive, link against: euser.lib
//#include <e32std.h>
#include <f32file.h>
#include "hash.h"

// Types

typedef TBuf8<MD5_HASH * 2> TFileNameHashBuff;

// Forward declarations

class MAsyncFileManObserver;

// Classes

class TDirStats
	{
public:
	TInt iFilesCount; // Total amount of files in directory
	//TInt iDirsCount;
	TInt iSize; // Total size of all files in bytes
	};

class FileUtils
	{
public:
	static void FileSizeToReadableString(/*TUint64*/ TInt aBytes, TDes &aDes);
	static TInt DirectoryStats(RFs &aFs, const TDesC &aDir, TDirStats &aDirStats);
	
	// Returns drive letter where program is installed (i.e. drive of EXE)
	static char InstallationDrive();
	
private:
	static TInt DoDirectoryStats(RFs &aFs, const TDesC &aDir, TDirStats &aDirStats);
	};


// Wrapper for provide public access to some protected members of CFileMan class
class CFileManExtended : public CFileMan
	{
// Constructors / destructors
public:
	static CFileManExtended* NewL(RFs& aFs);
	static CFileManExtended* NewL(RFs& aFs, MFileManObserver* anObserver);
private:
	CFileManExtended(RFs& aFs) : CFileMan(aFs) {};
	
// New methods
public:
	inline TInt ProcessedFiles() { return iNumberOfFilesProcessed; };
	inline TInt TotalFiles() { return iDirList != NULL ? iDirList->Count() : 0; };
	
	/* FixMe: When delete files recursively, TotalFiles() returns amount of files
	          ONLY in current processing dir, but not total count */
	};


class CAsyncFileMan : public CActive, public MFileManObserver
// FixMe: Memory leak when cancel delete operation
	{
public:
	// Cancel and destroy
	~CAsyncFileMan();

	// Two-phased constructor.
	static CAsyncFileMan* NewL(RFs &aFs, MAsyncFileManObserver* aObserver);

	// Two-phased constructor.
	static CAsyncFileMan* NewLC(RFs &aFs, MAsyncFileManObserver* aObserver);

public:
	// New functions
	// Function for making the initial request
//	void StartL();

private:
	// C++ constructor
	CAsyncFileMan(MAsyncFileManObserver* aObserver);

	// Second-phase constructor
	void ConstructL(RFs &aFs);

private:
	// From CActive
	// Handle completion
	void RunL();

	// How to cancel me
	void DoCancel();

	// Override to handle leaves from RunL(). Default implementation causes
	// the active scheduler to panic.
	TInt RunError(TInt aError);
	
// from MFileManObserver
public:
	MFileManObserver::TControl NotifyFileManStarted();
	MFileManObserver::TControl NotifyFileManOperation();
	MFileManObserver::TControl NotifyFileManEnded();

private:
	CFileManExtended* iFileMan;
	MAsyncFileManObserver* iObserver;
	TBool iCancelOperation; // Used for cancel current operation in file manager
	
public:
	// ToDo: Add other operations (rename, copy, etc...)
	TInt Delete(const TDesC& aName, TUint aSwitch=0);

	inline TInt ProcessedFiles() { return iFileMan->ProcessedFiles(); };
	inline TInt TotalFiles() { return iFileMan->TotalFiles(); };
	
	};


class MAsyncFileManObserver
	{
public:
	virtual MFileManObserver::TControl OnFileManStarted();
	virtual MFileManObserver::TControl OnFileManOperation();
	virtual MFileManObserver::TControl OnFileManEnded();
	virtual void OnFileManFinished(TInt aStatus);
	};


/*
 * Used for store large number of files in tree structure  
 */
class CFileTreeMapper : public CBase
	{
public:
	~CFileTreeMapper();
	static CFileTreeMapper* NewL(const TDesC &aBaseDir, TInt aLevels = 2,
			TInt aSubdirNameLength = 2, TBool aPreserveOriginalFileName = EFalse);
	static CFileTreeMapper* NewLC(const TDesC &aBaseDir, TInt aLevels = 2,
			TInt aSubdirNameLength = 2, TBool aPreserveOriginalFileName = EFalse);

private:
	CFileTreeMapper(const TDesC &aBaseDir, TInt aLevels, TInt aSubdirNameLength,
			TBool aPreserveOriginalFileName);
	void ConstructL();
	
private:
	TFileName iBaseDir;
	TInt iLevels;           // ToDo: Check range (min=1, max=???)
	TInt iSubdirNameLength; // ToDo: Check range (min=1, max=???)
	TBool iPreserveOriginalFileName;
	CMD5* iMd5;
	
	void CalculateHash(const TDesC/*8*/ &aSrc, TFileNameHashBuff &aHash);
	
public:	
	void GetFilePath(const TDesC &anOriginalFileName, TFileName &aFilePath);
	inline void SetBaseDir(const TDesC &aBaseDir)
		{ iBaseDir.Copy(aBaseDir); };
	};


#endif // FILEUTILS_H
