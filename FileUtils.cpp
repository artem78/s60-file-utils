/*
 ============================================================================
 Name		: FileUtils.cpp
 Author	  : artem78
 Version	 : 2.0.1
 Copyright   : 
 Description :
 ============================================================================
 */

#include "FileUtils.h"
#include "Logger.h"
//#include <baflutils.h>


// FileUtils

void FileUtils::FileSizeToReadableString(/*TUint64*/ TInt aBytes, TDes &aDes)
	{
	_LIT(KBytesUnit, "B");
	_LIT(KKiloBytesUnit, "KB");
	_LIT(KMegaBytesUnit, "MB");
	_LIT(KGigaBytesUnit, "GB");
	const TInt KGiga = 1024 * KMega;
	
	TReal size;
	TPtrC unit;
//	/* Note: For unknown reason method with real format stopped work in another project.
//	   Therefore use Format() instead. */
//	TRealFormat realFmt;
//	realFmt.iType = KRealFormatFixed | KDoNotUseTriads;
//	//realFmt.iPoint = '.';
//	realFmt.iPlaces = /*1*/ 2;
//	realFmt.iTriLen = 0;
//	realFmt.iWidth = KDefaultRealWidth;
	TBool hasFractionalPart = ETrue;
	
	if (aBytes < KKilo)
		{ // Bytes
		size = aBytes;
		unit.Set(KBytesUnit);
//		realFmt.iPlaces = 0;
		hasFractionalPart = EFalse;
		}
	else if (aBytes < KMega)
		{ // Kilobytes
		size = (TReal)aBytes / KKilo;
		unit.Set(KKiloBytesUnit);
		}
	else if (aBytes < KGiga)
		{ // Megabytes
		size = (TReal)aBytes / KMega;
		unit.Set(KMegaBytesUnit);
		}
	else
		{ // Gigabytes
		size = (TReal)aBytes / KGiga;
		unit.Set(KGigaBytesUnit);
		}
	
//	aDes.Zero();
//	aDes.Num(size, realFmt);
//	aDes.Append(' ');
//	aDes.Append(unit);	
	
	_LIT(KFmtInt, "%.0f %S");
	_LIT(KFmtReal, "%.2f %S");
	TPtrC fmt(hasFractionalPart ? KFmtReal : KFmtInt);
	aDes.Format(fmt, size, &unit);
	}

TInt FileUtils::DirectoryStats(RFs &aFs, const TDesC &aDir, TDirStats &aDirStats)
	{
	// ToDo: Make asynchronous or find quicker way without loop over all files 
	
	// Set zeros initial values before recursion start
	aDirStats.iFilesCount = 0;
	aDirStats.iSize = 0;
	
	// Start recursive call
	/*TInt r =*/ DoDirectoryStats(aFs, aDir, aDirStats);
	
	return KErrNone;
	}

TInt FileUtils::DoDirectoryStats(RFs &aFs, const TDesC &aDir, TDirStats &aDirStats)
	{
	CDir* dirItems = NULL;
	TInt r = aFs.GetDir(aDir, KEntryAttDir | KEntryAttNormal | KEntryAttHidden
			| KEntryAttSystem, ESortNone, dirItems);
	if (r == KErrNone && dirItems != NULL)
		{
		for (TInt i = 0; i < dirItems->Count(); i++)
			{
			const TEntry &entry = (*dirItems)[i];
			
			if (entry.IsDir())
				{ // Directory
				RBuf dir;
				dir.Create(KMaxFileName);
				dir.Copy(aDir);
				TParsePtr parser(dir);
				parser.AddDir(entry.iName);
				/*TInt r =*/ DoDirectoryStats(aFs, parser.FullName(), aDirStats);
				dir.Close();
				}
			else
				{ // File
				aDirStats.iFilesCount++;
				aDirStats.iSize += entry.iSize;
				}
			}
		
		delete dirItems;
		
		return KErrNone;
		}
	else
		return r or KErrUnknown;
	}

char FileUtils::InstallationDrive()
	{
	// Get drive from current process (path to exe)
	RProcess proc;
	TFileName procPath = proc.FileName();
	TParse parser;
	parser.Set(procPath, NULL, NULL);
	return parser.Drive()[0]; // Drop semicolon
	}


// 	CFileManExtended

CFileManExtended* CFileManExtended::NewL(RFs& aFs)
	{
	// Just change class of returned pointer from parent
	return static_cast<CFileManExtended*>(CFileMan::NewL(aFs));
	}

CFileManExtended* CFileManExtended::NewL(RFs& aFs,MFileManObserver* anObserver)
	{
	// Just change class of returned pointer from parent
	return static_cast<CFileManExtended*>(CFileMan::NewL(aFs, anObserver));
	}


// CAsyncFileMan

CAsyncFileMan::CAsyncFileMan(MAsyncFileManObserver* aObserver) :
	CActive(EPriorityStandard), // Standard priority
	iObserver(aObserver)
	{
	}

CAsyncFileMan* CAsyncFileMan::NewLC(RFs &aFs, MAsyncFileManObserver* aObserver)
	{
	CAsyncFileMan* self = new (ELeave) CAsyncFileMan(aObserver);
	CleanupStack::PushL(self);
	self->ConstructL(aFs);
	return self;
	}

CAsyncFileMan* CAsyncFileMan::NewL(RFs &aFs, MAsyncFileManObserver* aObserver)
	{
	CAsyncFileMan* self = CAsyncFileMan::NewLC(aFs, aObserver);
	CleanupStack::Pop(); // self;
	return self;
	}

void CAsyncFileMan::ConstructL(RFs &aFs)
	{
	iFileMan = CFileManExtended::NewL(aFs, this);
	
	CActiveScheduler::Add(this); // Add to scheduler
	}

CAsyncFileMan::~CAsyncFileMan()
	{
	Cancel(); // Cancel any request, if outstanding
	
	delete iFileMan;
	}

void CAsyncFileMan::DoCancel()
	{
	DEBUG(_L("Operation goes to cancell"));
	iCancelOperation = ETrue;
	
	// When cancelling, RunL won`t be called later,
	// therefore call observer`s method here 
	iObserver->OnFileManFinished(KErrCancel);
	}

void CAsyncFileMan::RunL()
	{
	DEBUG(_L("RunL status=%d"), iStatus.Int());
	
	iObserver->OnFileManFinished(iStatus.Int());
	}

TInt CAsyncFileMan::RunError(TInt aError)
	{
	if (aError != KErrNone)
		ERROR(_L("Error, code=%d"), aError);
	
	return /*aError*/ KErrNone;
	}

MFileManObserver::TControl CAsyncFileMan::NotifyFileManStarted()
	{
	if (iCancelOperation)
		{
		DEBUG(_L("Operation cancelled"));
		return MFileManObserver::EAbort;
		}
	
	DEBUG(_L("NotifyFileManStarted"));
	return iObserver->OnFileManStarted();
	}

MFileManObserver::TControl CAsyncFileMan::NotifyFileManOperation()
	{
	if (iCancelOperation)
		{
		DEBUG(_L("Operation cancelled"));
		return MFileManObserver::EAbort;
		}
	
	DEBUG(_L("NotifyFileManOperation"));
	return iObserver->OnFileManOperation(); 
	}

MFileManObserver::TControl CAsyncFileMan::NotifyFileManEnded()
	{
	if (iCancelOperation)
		{
		DEBUG(_L("Operation cancelled"));
		return MFileManObserver::EAbort;
		}
	
	DEBUG(_L("NotifyFileManEnded"));
	return iObserver->OnFileManEnded();
	}

TInt CAsyncFileMan::Delete(const TDesC& aName, TUint aSwitch)
	{
	//Cancel();
	if (IsActive())
		return KErrInUse;
	iCancelOperation = EFalse;
	TInt r = iFileMan->Delete(aName, aSwitch, iStatus); // ToDo: Check r
	SetActive();
	INFO(_L("Delete operation started"));
	return r;
	}


// MAsyncFileManObserver

MFileManObserver::TControl MAsyncFileManObserver::OnFileManStarted()
	{
	return MFileManObserver::EContinue;
	}

MFileManObserver::TControl MAsyncFileManObserver::OnFileManOperation()
	{
	return MFileManObserver::EContinue;
	}

MFileManObserver::TControl MAsyncFileManObserver::OnFileManEnded()
	{
	return MFileManObserver::EContinue;
	}

void MAsyncFileManObserver::OnFileManFinished(TInt /*aStatus*/)
	{
	
	}


// CFileTreeMapper

typedef TBuf8<MD5_HASH> TFileNameHash;


CFileTreeMapper::CFileTreeMapper(const TDesC &aBaseDir, TInt aLevels,
		TInt aSubdirNameLength, TBool aPreserveOriginalFileName) :
		
		iLevels(aLevels),
		iSubdirNameLength(aSubdirNameLength),
		iPreserveOriginalFileName(aPreserveOriginalFileName)
	{
	iBaseDir.Copy(aBaseDir);
	}

CFileTreeMapper::~CFileTreeMapper()
	{
	delete iMd5;
	}

CFileTreeMapper* CFileTreeMapper::NewLC(const TDesC &aBaseDir, TInt aLevels,
		TInt aSubdirNameLength, TBool aPreserveOriginalFileName)
	{
	CFileTreeMapper* self = new (ELeave) CFileTreeMapper(aBaseDir, aLevels,
			aSubdirNameLength, aPreserveOriginalFileName);
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
	}

CFileTreeMapper* CFileTreeMapper::NewL(const TDesC &aBaseDir, TInt aLevels,
		TInt aSubdirNameLength, TBool aPreserveOriginalFileName)
	{
	CFileTreeMapper* self = CFileTreeMapper::NewLC(aBaseDir, aLevels,
			aSubdirNameLength, aPreserveOriginalFileName);
	CleanupStack::Pop(); // self;
	return self;
	}

void CFileTreeMapper::ConstructL()
	{
	iMd5 = CMD5::NewL();
	}

void CFileTreeMapper::CalculateHash(const TDesC/*8*/ &aSrc, TFileNameHashBuff &aHash)
	{
	TPtrC8 srcPtr8((const TUint8*)aSrc.Ptr(),aSrc.Size());
	iMd5->Update(srcPtr8);
	TFileNameHash hash;
	hash.Copy(iMd5->Final());
	
	aHash.Zero();
	// Convert bytes to HEX string
	for (TInt i = 0; i < hash.Length(); i++)
		aHash.AppendNum(hash[i], EHex);
	}

void CFileTreeMapper::GetFilePath(const TDesC &anOriginalFileName, TFileName &aFilePath)
	{
	TFileNameHashBuff hash;
	CalculateHash(anOriginalFileName, hash);
	
	aFilePath.Zero();
	aFilePath.Append(iBaseDir);
	
	// Subdirs
	//TPtrC8 subdirName;
	TBuf<10> subdirName;
	for (TInt level = 1; level <= iLevels; level++)
		{
		TInt pos = (level - 1) * iSubdirNameLength;
		//subdirName.Set(hash.Mid(pos, iSubdirNameLength));
		subdirName.Copy(hash.Mid(pos, iSubdirNameLength));
		aFilePath.Append(subdirName);
		aFilePath.Append(KPathDelimiter);
		}
	//aFilePath.Append(KPathDelimiter);
	//BaflUtils::EnsurePathExistsL(aFilePath);

	// Filename
	if (iPreserveOriginalFileName)
		aFilePath.Append(anOriginalFileName);
	else
		{
		TFileName newFileName;
		TInt pos = iLevels * iSubdirNameLength;
		TInt len = hash.Length() - pos;
		newFileName.Copy(hash.Mid(pos, len));
		aFilePath.Append(newFileName);
		// ToDo: What about file extension?
		}
	}
