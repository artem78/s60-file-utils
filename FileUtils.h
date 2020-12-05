/*
 ============================================================================
 Name		: FileUtils.h
 Author	  : artem78
 Version	 : 1.0
 Copyright   : 
 Description : Collection of file utilites
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
	
private:
	static TInt DoDirectoryStats(RFs &aFs, const TDesC &aDir, TDirStats &aDirStats);
	};


// Wrapper for provide public access to some protected members of CFileMan class  
#ifdef WINSCW
	// Note: This class is temporarily disabled due to phone build fails
	// (it works only on emulator). Now CAsyncFileMan::ProcessedFiles() and
	// CAsyncFileMan::TotalFiles() only on phone are stubs and always return 0.
	// Discussion: https://discord.com/channels/431429574975422464/743412813279526914/777944434888146974
	// FixMe: fix and uncomment
class CFileManExtended : public CFileMan
	{
public:
// Constructors / destructors
	static CFileManExtended* NewL(RFs& aFs);
	static CFileManExtended* NewL(RFs& aFs,MFileManObserver* anObserver);
	
// New methods
	inline TInt ProcessedFiles() { return iNumberOfFilesProcessed; };
	inline TInt TotalFiles() { return iDirList != NULL ? iDirList->Count() : 0; };
	};
#endif


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
#ifdef WINSCW
	CFileManExtended* iFileMan;
#else
	CFileMan* iFileMan;
#endif
	MAsyncFileManObserver* iObserver;
	TBool iCancelOperation; // Used for cancel current operation in file manager
	
public:
	// ToDo: Add other operations (rename, copy, etc...)
	TInt Delete(const TDesC& aName, TUint aSwitch=0);

#ifdef WINSCW
	inline TInt ProcessedFiles() { return iFileMan->ProcessedFiles(); };
	inline TInt TotalFiles() { return iFileMan->TotalFiles(); };
#else
	// Stubs   ToDo: remove!
	#warning "CFileManExtended::ProcessedFiles() and CFileManExtended::TotalFiles() work in stub mode and not show real values!"
	inline TInt ProcessedFiles() { return 0; };
	inline TInt TotalFiles() { return 0; };
#endif
	
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
