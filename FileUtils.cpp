/*
 ============================================================================
 Name		: FileUtils.cpp
 Author	  : artem78
 Version	 : 1.0
 Copyright   : 
 Description :
 ============================================================================
 */

#include "FileUtils.h"
#include "Logger.h"
#include <eikprogi.h> // For CEikProgressInfo
////////////
//#include <eikenv.h>
////////////
#include "FileUtils.rsg"


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
	TRealFormat realFmt;
	realFmt.iType = KRealFormatFixed | KDoNotUseTriads;
	//realFmt.iPoint = '.';
	realFmt.iPlaces = /*1*/ 2;
	realFmt.iTriLen = 0;
	realFmt.iWidth = KDefaultRealWidth;
	
	if (aBytes < KKilo)
		{ // Bytes
		size = aBytes;
		unit.Set(KBytesUnit);
		realFmt.iPlaces = 0;
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
	
	aDes.Zero();
	aDes.Num(size, realFmt);
	aDes.Append(' ');
	aDes.Append(unit);	
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
	
	// Destroy progress window if shown
	if (iProgressDlg != NULL)
		delete iProgressDlg;
	
	// When cancelling, RunL won`t be called later,
	// therefore call observer`s method here 
	iObserver->OnFileManFinished(KErrCancel);
	}

void CAsyncFileMan::RunL()
	{
	DEBUG(_L("RunL status=%d"), iStatus.Int());
	
	// Destroy progress window if shown
	if (iProgressDlg != NULL)
		delete iProgressDlg;
	
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

TInt CAsyncFileMan::Delete(const TDesC& aName, TUint aSwitch, TBool aShowProgressDlg)
	{
	//Cancel();
	if (IsActive())
		return KErrInUse;
	iCancelOperation = EFalse;
	if (aShowProgressDlg)
		{
		iProgressDlg = new (ELeave) CFileOperationProgressDialog;
		iProgressDlg->ExecuteDlgL();
		}
		
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



// CFileOperationProgressDialog

//CFileOperationProgressDialog::CFileOperationProgressDialog()
//	{
//	
//	}

CFileOperationProgressDialog::~CFileOperationProgressDialog()
	{
	TRAP_IGNORE(RemoveDlgL());
	}

void CFileOperationProgressDialog::ExecuteDlgL()
	{
	//////////////
	///CEikonEnv::Static()->InfoMsg(_L("ExecuteDlgL"));
	//////////////
	
	if (iDlg == NULL)
		{
		//DEBUG(_L("Execute tracks deletion dialog"));
		
		// Initialization of progress dialog
		iDlg = new (ELeave) CAknProgressDialog(REINTERPRET_CAST(CEikDialog**, &iDlg));
		iDlg->PrepareLC(R_FILE_OPERATION_PROGRESS_DIALOG);
		//CEikProgressInfo* progressInfo = iDlg->GetProgressInfoL();
		//CGPSTrackerAppUi* appUi = static_cast<CGPSTrackerAppUi *>(AppUi());
		/*TInt totalCount = appUi->iAsyncFileMan->TotalFiles(); // equals to 0
		progressInfo->SetFinalValue(totalCount);*/
		iDlg->RunLD();
		}
	
//	if  (iCallback == NULL)
//		{
//		// Dialog callback for cancel
//		iCallback = new (ELeave) CProgressDialogCallback(this,
//				iDlg, &HandleDialogCanceledL);
//		iDlg->SetCallback(iCallback);
//		}
		
	if (iRefreshTimer == NULL)
		{
		// Starts periodic progress bar update
		const TInt KSecond = 1000000;
		TTimeIntervalMicroSeconds32 updateInterval = KSecond / 5;
		TCallBack callback(UpdateProgress, this);
		iRefreshTimer = CPeriodic::NewL(EPriorityNormal);
		iRefreshTimer->Start(0 /*updateInterval*/, updateInterval, callback);
		}
	}

void CFileOperationProgressDialog::RemoveDlgL(TBool anExceptDialog)
	{
	//////////////
	//CEikonEnv::Static()->InfoMsg(_L("RemoveDlgL"));
	//////////////
	
	// Dialog
	if (!anExceptDialog)
		{
		if (iDlg != NULL)
			{
			//DEBUG(_L("Remove tracks deletion dialog"));
			
			iDlg->SetCallback(NULL);
			iDlg->ProcessFinishedL();
			iDlg = NULL;
			}
		}
		
//	// Dialog cancel callback
//	delete iCallback;
//	iCallback = NULL;
	
	// Progress refresh timer
	if (iRefreshTimer != NULL)
		{
		iRefreshTimer->Cancel();
		delete iRefreshTimer;
		iRefreshTimer = NULL;
		}
	}

TInt CFileOperationProgressDialog::UpdateProgress(TAny* anObject)
	{	
//	if (iDlg == NULL)
//		return (TInt) ETrue;
//	
//	CEikProgressInfo* progressInfo = NULL;
//	TRAPD(r, progressInfo = iDlg->GetProgressInfoL());
//	if (r == KErrNone && progressInfo != NULL)
//		{
//		TInt totalCount = /*iAsyncFileMan->TotalFiles()*/ 100;
//		progressInfo->SetFinalValue(totalCount); // ToDo: May be done one time after deletion started
//		
//		TInt processedCount = /*iAsyncFileMan->ProcessedFiles()*/ 50;
//		progressInfo->SetAndDraw(processedCount);
//		}
//	
//	return (TInt) ETrue;
	}
