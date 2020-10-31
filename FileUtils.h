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
#include <aknprogressdialog.h> // For operation progress dialog

// Forward declarations

class MAsyncFileManObserver;
class CFileOperationProgressDialog;

// Classes

class FileUtils
	{
public:
	static void FileSizeToReadableString(/*TUint64*/ TInt aBytes, TDes &aDes);

	};


// Wrapper for provide public access to some protected members of CFileMan class  
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
	CFileOperationProgressDialog* iProgressDlg;
	
public:
	// ToDo: Add other operations (rename, copy, etc...)
	TInt Delete(const TDesC& aName, TUint aSwitch=0, TBool aShowProgressDlg=EFalse);
	
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


// File operation progress dialog
class CFileOperationProgressDialog : public CBase
	{
private:
//	typedef void ( /*CTrackListBoxView::*/*ProgressDialogEventHandler )( 
//			CAknProgressDialog* aProgressDialog );
//	
//	/**
//	 * This is a helper class for progress/wait dialog callbacks. It routes the dialog's
//	 * cancel notification to the handler function for the cancel event.
//	 */
//	class CProgressDialogCallback : public CBase, public MProgressDialogCallback
//		{ 
//		public:
//			CProgressDialogCallback( 
//					CTrackListBoxView* aHandlerObj, 
//					CAknProgressDialog* aDialog, 
//					ProgressDialogEventHandler aHandler ) :
//				handlerObj( aHandlerObj ), dialog( aDialog ), handler( aHandler )
//				{}
//				
//			void DialogDismissedL( TInt aButtonId ) 
//				{
//				( handlerObj->*handler )( dialog );
//				}
//		private:
//			CTrackListBoxView* handlerObj;
//			CAknProgressDialog* dialog;
//			ProgressDialogEventHandler handler;
//		};
//		
	
	CAknProgressDialog* iDlg; // Operation progress dialog
//	CProgressDialogCallback* iCallback; // Callback for cancel dialog 
	CPeriodic* iRefreshTimer; // For periodically refresh progress bar position
public:
//	CFileOperationProgressDialog();
	~CFileOperationProgressDialog();
	
	void ExecuteDlgL();
		
	// If called from Cancel callback, anExceptDialog must be set to ETrue, because
	// the dialog already has been destroyed by ui framework
	void RemoveDlgL(TBool anExceptDialog=EFalse);
	static TInt UpdateProgress(TAny* anObject);
	};




#endif // FILEUTILS_H
