/*
 ============================================================================
 Name		: FileUtils.cpp
 Author	  : artem78
 Version	 : 2.1
 Copyright   : 
 Description :
 ============================================================================
 */

#include "FileUtils.h"
#include "Logger.h"
//#include <baflutils.h>


// FileUtils

void FileUtils::FileSizeToReadableString(TUint64 aBytes, TDes &aDes)
	{
	_LIT(KBytesUnit, "B");
	_LIT(KKiloBytesUnit, "KB");
	_LIT(KMegaBytesUnit, "MB");
	_LIT(KGigaBytesUnit, "GB");
	_LIT(KTeraBytesUnit, "TB");
	
	typedef TBuf<2> TUnitName;
	TFixedArray<TUnitName, 5> units;
	units[0] = KBytesUnit;
	units[1] = KKiloBytesUnit;
	units[2] = KMegaBytesUnit;
	units[3] = KGigaBytesUnit;
	units[4] = KTeraBytesUnit;
	
	TUint64 factor(1);
	factor <<= 10 * units.Count();
	
	TPtrC unit;
	for (TInt i = units.Count() - 1; i >= 0; i--)
		{
		unit.Set(units[i]);
		factor >>= 10;
		
		if (aBytes >= factor)
			break;
		}
	
	TReal size = (TReal) aBytes / factor;
	
	_LIT(KFmtInt, "%.0f %S");
	_LIT(KFmtReal, "%.2f %S");
	TPtrC fmt(/*unit == KBytesUnit*/ factor == 1 ? KFmtInt : KFmtReal);
	
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

TBool FileUtils::IsDriveWritable(RFs &aFs, TDriveNumber aDrive)
	{
	//RFs fs = CCoeEnv::Static()->FsSession();
	
	TDriveList drvList;
	if (aFs.DriveList(drvList) != KErrNone)
		return EFalse;
	
	if (!drvList[aDrive])
		return EFalse;
	
	TVolumeInfo volInfo;
	if (aFs.Volume(volInfo, aDrive) != KErrNone)
		return EFalse;
	
#ifdef __WINSCW__
	TChar drvChar;
	if (aFs.DriveToChar(aDrive, drvChar) != KErrNone) drvChar = '?';
	TBuf<32> type(KNullDesC);
	switch (volInfo.iDrive.iType)
		{
		case EMediaNotPresent: type = _L("EMediaNotPresent");break;
		case EMediaUnknown: type = _L("EMediaUnknown");break;
		case EMediaFloppy: type = _L("EMediaFloppy");break;
		case EMediaHardDisk: type = _L("EMediaHardDisk");break;
		case EMediaCdRom: type = _L("EMediaCdRom");break;
		case EMediaRam: type = _L("EMediaRam");break;
		case EMediaFlash: type = _L("EMediaFlash");break;
		case EMediaRom: type = _L("EMediaRom");break;
		case EMediaRemote: type = _L("EMediaRemote");break;
		case EMediaNANDFlash: type = _L("EMediaNANDFlash");break;
		
		
		default:
		break;
		}
	DEBUG(_L("drive=%c (#%d) type=%S mediaAtt=%b write protected=%d local=%d internal=%d removable=%d locaked=%d"),
			(TUint)drvChar, aDrive,
			&type, volInfo.iDrive.iMediaAtt,
			(TInt)(volInfo.iDrive.iMediaAtt & KMediaAttWriteProtected),
			(TInt)(volInfo.iDrive.iMediaAtt & KDriveAttLocal),
			(TInt)(volInfo.iDrive.iMediaAtt & KDriveAttInternal),
			(TInt)(volInfo.iDrive.iMediaAtt & KDriveAttRemovable),
			(TInt)(volInfo.iDrive.iMediaAtt & KMediaAttLocked)
	);
#endif
	
	switch (volInfo.iDrive.iType)
		{
		case EMediaHardDisk:
		case EMediaFlash:
		case EMediaNANDFlash:
		case EMediaRam: // ???
			break;
		
		default:
			return EFalse;
		};
	
	if (volInfo.iDrive.iMediaAtt & KMediaAttWriteProtected)
		return EFalse;
	
	return ETrue;
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
