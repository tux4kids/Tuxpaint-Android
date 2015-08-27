//
// OCSetupHlp.iss
// --------------
//
// OpenCandy Helper Include File
//
// This file defines functions and procedures that need to
// be called from your main installer script in order to
// initialize and setup OpenCandy.
//
// Please consult the accompanying SDK documentation for
// integration details and contact partner support for
// assistance with any advanced integration needs.
//
// IMPORTANT:
// ----------
// Publishers should have no need to modify the content
// of this file. If you are modifying this file for any
// reason other than as directed by partner support
// you are probably making a mistake. Please contact
// partner support instead.
//
// Copyright (c) 2008 - 2011 OpenCandy, Inc.
//

[Code]

//--------------------------------
// OpenCandy types
//--------------------------------

#ifdef UNICODE
type OCWString = String;
type OCAString = AnsiString;
type OCTString = OCWString;
#else
type OCAString = String;
type OCTString = OCAString;
#endif



//--------------------------------
// OpenCandy definitions
//--------------------------------

// Size of strings (including terminating character)
#define OC_STR_CHARS 1024

// Values used with OCInit2A(), OCInit2W() APIs
#define OC_INIT_SUCCESS      0
#define OC_INIT_MODE_NORMAL  0
#define OC_INIT_MODE_REMNANT 1

// Values used with OCGetNoCandy() API
#define OC_CANDY_ENABLED  0
#define OC_CANDY_DISABLED 1

// Offer types returned by OCGetOfferType() API
# define OC_OFFER_TYPE_NORMAL   1
# define OC_OFFER_TYPE_EMBEDDED 2

// Values returned by OCGetBannerInfo() API
#define OC_OFFER_BANNER_FOUNDNEITHER     0
#define OC_OFFER_BANNER_FOUNDTITLE       1
#define OC_OFFER_BANNER_FOUNDDESCRIPTION 2
#define OC_OFFER_BANNER_FOUNDBOTH        3

// User choice indicators returned by OCGetOfferState() API
#define OC_OFFER_CHOICE_ACCEPTED  1
#define OC_OFFER_CHOICE_DECLINED  0
#define OC_OFFER_CHOICE_NOCHOICE -1

// Values used with OCCanLeaveOfferPage() API
#define OC_OFFER_LEAVEPAGE_ALLOWED    1
#define OC_OFFER_LEAVEPAGE_DISALLOWED 0

// Values used for OCGetAsyncOfferStatus() API
#define OC_OFFER_STATUS_CANOFFER_READY    0
#define OC_OFFER_STATUS_CANOFFER_NOTREADY 1
#define OC_OFFER_STATUS_QUERYING_NOTREADY 2
#define OC_OFFER_STATUS_NOOFFERSAVAILABLE 3

// Values returned by OCRunDialog() API
#define OC_OFFER_RUNDIALOG_FAILURE -1

// Values returned by OCLoadOpenCandyDLL() API
#define OC_LOADOCDLL_FAILURE 0

// Values used with LogDevModeMessage() API
#define OC_DEVMSG_ERROR_TRUE  1
#define OC_DEVMSG_ERROR_FALSE 0

// Values used in the sample installer script
//
// IMPORTANT:
// Do not modify these definitions or disable the warnings below.
// If you see warnings when you compile your script you must
// modify the values you set where you !insertmacro OpenCandyInit
// (i.e. in your .iss file) before releasing your installer publicly.
#define OC_SAMPLE_PUBLISHERNAME "Open Candy Sample"
#define OC_SAMPLE_KEY "748ad6d80864338c9c03b664839d8161"
#define OC_SAMPLE_SECRET "dfb3a60d6bfdb55c50e1ef53249f1198"

// Compile-time checks and defaults
#if OC_STR_MY_PRODUCT_NAME == OC_SAMPLE_PUBLISHERNAME
	#pragma warning "Do not forget to change the product name from '" + OC_SAMPLE_PUBLISHERNAME + "' to your company's product name before releasing this installer."
#endif
#if OC_STR_KEY == OC_SAMPLE_KEY
	#pragma warning "Do not forget to change the sample key '" + OC_SAMPLE_KEY + "' to your company's product key before releasing this installer."
#endif
#if OC_STR_SECRET == OC_SAMPLE_SECRET
	#pragma warning "Do not forget to change the sample secret '" + OC_SAMPLE_SECRET + "' to your company's product secret before releasing this installer."
#endif
#if Pos(LowerCase("\OCSetupHlp.dll"),LowerCase(OC_OCSETUPHLP_FILE_PATH)) == 0
	#pragma error "The definition OC_OCSETUPHLP_FILE_PATH does not use ""OCSetupHlp.dll"" for the file part."
#endif

// OC_MAX_INIT_TIME is the maximum time in milliseconds that OCInit may block when fetching offers.
// Note that under normal network conditions OCInit may return sooner. Setting this value too low
// may reduce offer rate. Values of 5000 or greater are recommended. If you intend to override this
// default do so by defining it in your .iss file before #include'ing this header. Be certain to
// make OpenCandy partner support aware of any override you apply because this can affect your metrics.
#ifndef OC_MAX_INIT_TIME
	#define OC_MAX_INIT_TIME 8000
#endif

// OC_INSERT_PAGE_AFTER is the PageID after which the OpenCandy offer screen should be inserted.
// If you intend to override this default do so by defining it in your .iss file before #include'ing this header.
#ifndef OC_INSERT_PAGE_AFTER
	#define OC_INSERT_PAGE_AFTER "wpSelectTasks"
#endif


//--------------------------------
// OpenCandy global variables
//--------------------------------

// IMPORTANT:
// Never modify or reference these directly, they are used
// completely internally to this helper script.

var

	gl_OC_bAttached:Boolean;               // Is the OpenCandy offer window attached?
	gl_OC_objOCOfferPage: TWizardPage;     // Handle to the offer page wizard page
	gl_OC_bHasBeenInitialized: Boolean ;   // Has the OpenCandy client been initialized?
	gl_OC_bNoCandy: Boolean;               // Is OpenCandy disabled?
	gl_OC_bUseOfferPage: Boolean;          // Should shown an offer?
	gl_OC_bHasReachedOCPage: Boolean;      // Has the user reached the OpenCandy offer page?
	gl_OC_bProductInstallSuccess: Boolean; // Has the publisher product install completed succesfully?
	gl_OC_bHasAdjustedPage: Boolean;       // The the offer page window been adjusted?



//-----------------------------------------
// OpenCandy external procedure definitions
//-----------------------------------------

procedure _OCDLL_OCStartDLMgr2Download();
external 'OCPRD379StartDLMgr2Download@files:OCSetupHlp.dll cdecl loadwithalteredsearchpath delayload';

function _OCDLL_OCLoadOpenCandyDLL():Integer;
external 'OCPRD379LoadOpenCandyDLL@files:OCSetupHlp.dll cdecl loadwithalteredsearchpath delayload';

function _OCDLL_OCInit(szPubId, szProdId, szSecret, szInstallLang:OCAString; bAsyncMode:Boolean; iMaxWait, iRemnant:Integer):Integer;
external 'OCPRD379Init2A@files:OCSetupHlp.dll cdecl loadwithalteredsearchpath delayload';

#ifdef UNICODE
function _OCDLL_OCInitW(wszPubId, wszProdId, wszSecret, wszInstallLang:OCWString; bAsyncMode:Boolean; iMaxWait, iRemnant:Integer):Integer;
external 'OCPRD379Init2W@files:OCSetupHlp.dll cdecl loadwithalteredsearchpath delayload';
#endif

function _OCDLL_OCGetBannerInfo(szTitle, szDesc:OCAString):Integer;
external 'OCPRD379GetBannerInfo@files:OCSetupHlp.dll cdecl loadwithalteredsearchpath delayload';

#ifdef UNICODE
function _OCDLL_OCGetBannerInfoW(wszTitle, wszDesc:OCWString):Integer;
external 'OCPRD379GetBannerInfoW@files:OCSetupHlp.dll cdecl loadwithalteredsearchpath delayload';
#endif

function _OCDLL_OCRunDialog(iHwnd:Integer): Integer;
external 'OCPRD379RunDialog@files:OCSetupHlp.dll cdecl loadwithalteredsearchpath delayload';

function _OCDLL_OCAdjustPage(iHwnd, iX, iY, iW, iH:Integer):Integer;
external 'OCPRD379InnoAdjust@files:OCSetupHlp.dll cdecl loadwithalteredsearchpath delayload';

function _OCDLL_OCRestorePage(iHwnd:Integer):Integer;
external 'OCPRD379InnoRestore@files:OCSetupHlp.dll cdecl loadwithalteredsearchpath delayload';

function _OCDLL_OCGetOfferState():Integer;
external 'OCPRD379GetOfferState@files:OCSetupHlp.dll cdecl loadwithalteredsearchpath delayload';

function _OCDLL_OCGetOfferType():Integer;
external 'OCPRD379GetOfferType@files:OCSetupHlp.dll cdecl loadwithalteredsearchpath delayload';

function _OCDLL_OCPrepareDownload():Integer;
external 'OCPRD379PrepareDownload@files:OCSetupHlp.dll cdecl loadwithalteredsearchpath delayload';

function _OCDLL_OCShutdown():Integer;
external 'OCPRD379Shutdown@files:OCSetupHlp.dll cdecl loadwithalteredsearchpath delayload';

function _OCDLL_OCDetach():Integer;
external 'OCPRD379Detach@files:OCSetupHlp.dll cdecl loadwithalteredsearchpath delayload';

function _OCDLL_OCSignalProductInstalled():Integer;
external 'OCPRD379SignalProductInstalled@files:OCSetupHlp.dll cdecl loadwithalteredsearchpath delayload';

function _OCDLL_OCSignalProductFailed():Integer;
external 'OCPRD379SignalProductFailed@files:OCSetupHlp.dll cdecl loadwithalteredsearchpath delayload';

function _OCDLL_OCGetAsyncOfferStatus(bWantToShowOffer:Boolean):Integer;
external 'OCPRD379GetAsyncOfferStatus@files:OCSetupHlp.dll cdecl loadwithalteredsearchpath delayload';

function _OCDLL_OCCanLeaveOfferPage():Integer;
external 'OCPRD379CanLeaveOfferPage@files:OCSetupHlp.dll cdecl loadwithalteredsearchpath delayload';

function _OCDLL_OCSetCmdLineValues(szValue:OCAString):Integer;
external 'OCPRD379SetCmdLineValues@files:OCSetupHlp.dll cdecl loadwithalteredsearchpath delayload';

#ifdef UNICODE
function _OCDLL_OCSetCmdLineValuesW(wszValue:OCWString):Integer;
external 'OCPRD379SetCmdLineValuesW@files:OCSetupHlp.dll cdecl loadwithalteredsearchpath delayload';
#endif

function _OCDLL_OCGetNoCandy():Integer;
external 'OCPRD379GetNoCandy@files:OCSetupHlp.dll cdecl loadwithalteredsearchpath delayload';

procedure _OCDLL_SetOCOfferEnabled(bEnabled:Boolean);
external 'OCPRD379SetOCOfferEnabled@files:OCSetupHlp.dll cdecl loadwithalteredsearchpath delayload';

procedure _OCDLL_LogDevModeMessage(szMessage:OCAString; iError, iFaqID:Integer);
external 'OCPRD379LogDevModeMessage@files:OCSetupHlp.dll cdecl loadwithalteredsearchpath delayload';

#ifdef UNICODE
procedure _OCDLL_LogDevModeMessageW(wszMessage:OCWString; iError, iFaqID:Integer);
external 'OCPRD379LogDevModeMessageW@files:OCSetupHlp.dll cdecl loadwithalteredsearchpath delayload';
#endif


//-------------------------------------------
// OpenCandy runtime functions and procedures
//-------------------------------------------

//
// _OpenCandyDevModeMsg
// --------------------
// This function is internal to this helper script. Do not
// call if from your own code.
//
// The function code is intentionally unguarded with respect
// to gl_OC_bHasBeenInitialized and gl_OC_bNoCandy. Calling
// code is responsible for ensuring appropriate conditions.
//
// Parameters:
//
//   tszMessage  : Message to display
//   bIsError    : True if the message represents an error
//   iFaqID      : ID of the FAQ associated with the message, or 0 if there is no FAQ associated
//
// Usage:
//
//   _OpenCandyDevModeMsg('This is an error with associated FAQ #500', true, 500);
//

procedure _OpenCandyDevModeMsg(tszMessage: OCTString; bIsError: Boolean; iFaqID: Integer);
var
	iError:Integer;
begin
	if bIsError then
	begin
		iError := {#OC_DEVMSG_ERROR_TRUE};
		tszMessage := '{\rtf1 {\colortbl;\red0\green0\blue0;\red255\green0\blue0;}\cf2Status ERROR! \cf1' + tszMessage + '\par}';
	end
	else 
		iError := {#OC_DEVMSG_ERROR_FALSE};
		
	 #ifdef UNICODE
	 _OCDLL_LogDevModeMessageW(tszMessage, iError, iFaqID);
	 #else
	 _OCDLL_LogDevModeMessage(tszMessage, iError, iFaqID);
	 #endif
end;



//
// _OCEnabledAndReady
// ------------------
// This function is internal to this helper script. Do not
// call if from your own code.
//

function _OCEnabledAndReady(): Boolean;
begin
	Result := gl_OC_bHasBeenInitialized and not gl_OC_bNoCandy;
end;



//
// _OpenCandyInitInternal
// ----------------------
// This procedure is internal to this helper script. Do not
// call if from your own code. Instead see OpenCandyInit
// and OpenCandyAsyncInit.
//

procedure _OpenCandyInitInternal(tszPublisher, tszKey, tszSecret, tszLanguage:OCTString; bUseAsyncMode: Boolean; iInitMode:Integer);
var
	i:Integer;
	iRes:Integer;
	tszDesc: OCTString;
	tszTitle: OCTString;
begin
	gl_OC_bAttached := false;
	gl_OC_bHasBeenInitialized := false;
	gl_OC_bNoCandy := false;
	gl_OC_bUseOfferPage := false;
	gl_OC_bHasReachedOCPage := false;
	gl_OC_bProductInstallSuccess := false;
	gl_OC_bHasAdjustedPage := false;

	// Load OpenCandy SDK DLL
	try
		iRes := _OCDLL_OCLoadOpenCandyDLL();
	except
		iRes := {#OC_LOADOCDLL_FAILURE};
	end;	
	if {#OC_LOADOCDLL_FAILURE} = iRes then
		gl_OC_bNoCandy := True
	else
	begin

		// Handle command line options and silent installations
		for i := 0 to ParamCount() do
		begin
			#ifdef UNICODE
			_OCDLL_OCSetCmdLineValuesW(ParamStr(i));
			#else
			_OCDLL_OCSetCmdLineValues(ParamStr(i));
			#endif

			// OpenCandy is disabled during a silent installation
			if WizardSilent() then
				#ifdef UNICODE
				_OCDLL_OCSetCmdLineValuesW('/NOCANDY');
				#else
				_OCDLL_OCSetCmdLineValues('/NOCANDY');
				#endif
		end;

		gl_OC_bNoCandy := {#OC_CANDY_DISABLED} = _OCDLL_OCGetNoCandy();

		if not gl_OC_bNoCandy then
		begin
			#ifdef UNICODE
			if {#OC_INIT_SUCCESS} = _OCDLL_OCInitW(tszPublisher, tszKey, tszSecret, tszLanguage, bUseAsyncMode, {#OC_MAX_INIT_TIME}, iInitMode) then
			#else
			if {#OC_INIT_SUCCESS} = _OCDLL_OCInit(tszPublisher, tszKey, tszSecret, tszLanguage, bUseAsyncMode, {#OC_MAX_INIT_TIME}, iInitMode) then
			#endif
			begin
				gl_OC_bHasBeenInitialized := true;
				tszTitle := StringOfChar(' ',{#OC_STR_CHARS});
				tszDesc := StringOfChar(' ',{#OC_STR_CHARS});
				#ifdef UNICODE
				case _OCDLL_OCGetBannerInfoW(tszTitle, tszDesc) of
				#else
				case _OCDLL_OCGetBannerInfo(tszTitle, tszDesc) of
				#endif
					{#OC_OFFER_BANNER_FOUNDTITLE}: tszDesc := '';
					{#OC_OFFER_BANNER_FOUNDDESCRIPTION}: tszTitle := '';
					{#OC_OFFER_BANNER_FOUNDNEITHER}:
					begin
					  tszTitle := '';
					  tszDesc := '';
					end;
				end;
				gl_OC_objOCOfferPage := CreateCustomPage({#OC_INSERT_PAGE_AFTER}, tszTitle, tszDesc);
			end;
		end;
	end;
end;



//
// OpenCandyInit (Deprecated) / OpenCandyAsyncInit
// -----------------------------------------------
// Performs initialization of the OpenCandy DLL
// and checks for offers to present.
//
// Parameters:
//
//   tszPublisher   : Your publisher name (will be provided by OpenCandy)
//   tszKey         : Your product key (will be provided by OpenCandy)
//   tszSecret      : Your product secret (will be provided by OpenCandy)
//   tszLanguage    : The installation language as an ISO 639-1 Alpha-2 Code
//   iInitMode      : The operating mode. Pass OC_INIT_MODE_NORMAL for normal operation
//                    or OC_INIT_MODE_REMNANT if OpenCandy should not show offers. Do not
//                    use iInitMode to handle /NOCANDY, this is done automatically for you.
//
// Usage (Using sample values for internal testing purposes only):
//
//   procedure InitializeWizard;
//   var
//     OCtszInstallerLanguage: OCTString;
//   begin
//     // Translate language from the value of the "Name"
//     // parameter assigned in the "[Languages]" section
//     // into ISO 639-1 Alpha-2 codes for the OpenCandy API
//     OCtszInstallerLanguage := ActiveLanguage();
//     if(OCtszInstallerLanguage = 'default') then
//       OCtszInstallerLanguage := 'en';
//     OpenCandyAsyncInit('{#OC_STR_MY_PRODUCT_NAME}', '{#OC_STR_KEY}', '{#OC_STR_SECRET}', OCtszInstallerLanguage, {#OC_INIT_MODE_NORMAL});
//   end;
//

procedure OpenCandyAsyncInit(tszPublisher, tszKey, tszSecret, tszLanguage:OCTString; iInitMode:Integer);
begin
	if not (gl_OC_bNoCandy or gl_OC_bHasBeenInitialized) then
		_OpenCandyInitInternal(tszPublisher, tszKey, tszSecret, tszLanguage, true, iInitMode);
end;

procedure OpenCandyInit(tszPublisher, tszKey, tszSecret, tszLanguage:OCTString; iInitMode:Integer);
begin
	if not (gl_OC_bNoCandy or gl_OC_bHasBeenInitialized) then
		_OpenCandyInitInternal(tszPublisher, tszKey, tszSecret, tszLanguage, false, iInitMode);
end;



//
// GetOCOfferStatus
// -----------------
// Allows you to determine if an offer is currently available. This is
// done automatically for you before the offer screen is shown. Typically
// you do not need to call this function from your own code directly.
//
// The offer status is placed on the stack and may be one of:
// {#OC_OFFER_STATUS_CANOFFER_READY}    - An OpenCandy offer is available and ready to be shown
// {#OC_OFFER_STATUS_CANOFFER_NOTREADY} - An offer is available but is not yet ready to be shown
// {#OC_OFFER_STATUS_QUERYING_NOTREADY} - The remote API is still being queried for offers
// {#OC_OFFER_STATUS_NOOFFERSAVAILABLE} - No offers are available
//
// When calling this function you must indicate whether the information returned
// will be used to decide whether the OpenCandy offer screen will be shown, e.g.
// if the information may result in a call to SetOCOfferEnabled. This helps
// to optimize future OpenCandy SDKs for better performance with your product.
//
// Usage:
//
//   // Test if OpenCandy is ready to show an offer.
//   // Indicate the result is informative only and does not directly
//   // determine whether offers from OpenCandy are enabled.
//   if {#OC_OFFER_STATUS_CANOFFER_READY} = GetOCOfferStatus(false) then
//

Function GetOCOfferStatus(bDeterminesOfferEnabled: Boolean): Integer;
begin
	if _OCEnabledAndReady() then
		Result := _OCDLL_OCGetAsyncOfferStatus(bDeterminesOfferEnabled)
	else
		Result := {#OC_OFFER_STATUS_NOOFFERSAVAILABLE};
end;



//
// SetOCOfferEnabled
// -----------------
// Allows you to disable the OpenCandy offer screen easily from your
// installer code. Note that this is not the recommended method - you
// ought to determine during initialization whether OpenCandy should be
// disabled and specify an appropriate mode when calling OpenCandyInit
// or OpenCandyAsyncInit in that case. If you must use this method please
// be sure to inform the OpenCandy partner support team. Never directly
// place logical conditions around other OpenCandy functions and macros because
// this can have unforseen consequences. You should call this procedure only
// after calling OpenCandyInit / OpenCandyAsyncInit.
//
// Usage:
//
//  // This turns off offers from the OpenCandy network
//  SetOCOfferEnabled(false);
//

procedure SetOCOfferEnabled(bEnabled: Boolean);
begin
	if _OCEnabledAndReady() then
		_OCDLL_SetOCOfferEnabled(bEnabled);
end;



//
// OpenCandyShouldSkipPage()
// -------------------------
//
// This function needs to be called from the ShouldSkipPage Inno script
// event function so that Inno Setup can determine whether the OpenCandy
// offer page should be displayed. The function returns true if the
// current page is the OpenCandy offer page and no offer is to be
// presented.
//
// Usage:
//
//   function ShouldSkipPage(PageID: Integer): Boolean;
//   begin
//     Result := false; // Don't skip pages by default
//
//     
//     if OpenCandyShouldSkipPage(PageID) then
//       Result := true;
//   end;
//

function OpenCandyShouldSkipPage(CurPageID: Integer) : Boolean;
begin
	Result := false;
	if _OCEnabledAndReady() then
		if CurPageID = gl_OC_objOCOfferPage.ID then
		begin
			if (not gl_OC_bUseOfferPage) and (not gl_OC_bHasReachedOCPage) then
				gl_OC_bUseOfferPage := {#OC_OFFER_STATUS_CANOFFER_READY} = _OCDLL_OCGetAsyncOfferStatus(true);
			gl_OC_bHasReachedOCPage := true;
			Result := not gl_OC_bUseOfferPage;
		end;
end;



//
// OpenCandyCurPageChanged()
// -------------------------
// This function needs to be called from CurPageChanged() Inno script
// event function so that the OpenCandy offer page is displayed correctly.
//
// Usage:
//
//   procedure CurPageChanged(CurPageID: Integer);
//   begin
//     OpenCandyCurPageChanged(CurPageID);
//   end;
//
procedure OpenCandyCurPageChanged(CurPageID: Integer);
begin
	if _OCEnabledAndReady() and gl_OC_bUseOfferPage then
	begin
		if (CurPageID <> gl_OC_objOCOfferPage.ID) and gl_OC_bAttached then
		begin
			_OCDLL_OCDetach();
			gl_OC_bAttached := false;
		end;
			
		if (CurPageID = gl_OC_objOCOfferPage.ID) and not gl_OC_bAttached then
		begin
			_OCDLL_OCAdjustPage(gl_OC_objOCOfferPage.Surface.Handle,8,60,480,250);
			if {#OC_OFFER_RUNDIALOG_FAILURE} <> _OCDLL_OCRunDialog(gl_OC_objOCOfferPage.Surface.Handle) then
				gl_OC_bAttached := true
			else
				gl_OC_bUseOfferPage := false;		
		end;
	end;
end;



//
// OpenCandyNextButtonClick()
// --------------------------
// This function needs to be called be called from the NextButtonClick()
// Inno script event function so that Inno Setup does not allow an end user
// to proceed past the OpenCandy offer screen in the event that the user
// must make a selection and hasn't yet done so. The function returns false
// if the user should not be allowed to proceed.
//
// Usage:
//
//   function NextButtonClick(CurPageID: Integer): Boolean;
//   begin
//     Result := true; // Allow action by default
//     if not OpenCandyNextButtonClick(CurPageID) then
//       Result := false;
//   end;
//

function OpenCandyNextButtonClick(CurPageID: Integer): Boolean;
begin
	Result := true;
	if _OCEnabledAndReady() and gl_OC_bUseOfferPage and (CurPageID = gl_OC_objOCOfferPage.ID) then
	begin
		// user must make a selection
		if {#OC_OFFER_LEAVEPAGE_DISALLOWED} = _OCDLL_OCCanLeaveOfferPage() then
			Result := false
		else
		begin
			_OCDLL_OCRestorePage(gl_OC_objOCOfferPage.Surface.Handle);
			Result := true;
		end;
	end;
end;



//
// OpenCandyBackButtonClick()
// --------------------------
// This function should be called from BackButtonClick() Inno script
// event function. It restores the layout of the installer window after
// an OpenCandy offer page has been displayed.
//
// Usage:
//
//   function BackButtonClick(CurPageID: Integer): Boolean;
//   begin
//     Result := true; // Allow action by default
//     OpenCandyBackButtonClick(CurPageID);
//   end;
//

procedure OpenCandyBackButtonClick(CurPageID: Integer);
begin
	if _OCEnabledAndReady() and gl_OC_bUseOfferPage and (CurPageID = gl_OC_objOCOfferPage.ID) then
		_OCDLL_OCRestorePage(gl_OC_objOCOfferPage.Surface.Handle);
end;



//
// _OpenCandyExecOfferInternal()
// -----------------------------
// This procedure is internal to this helper script. Do not
// call if from your own code.
//

procedure _OpenCandyExecOfferInternal();
begin
	_OCDLL_OCPrepareDownload();
	if _OCDLL_OCGetOfferState() = {#OC_OFFER_CHOICE_ACCEPTED} then
		_OCDLL_OCStartDLMgr2Download();
end;



//
// OpenCandyCurStepChanged()
// -------------------------
// This should be called from CurStepChanged() Inno script event function.
// It handles necesary operations at the various different stages of the setup,
// such as installing any offer the user may have accepted.
//
// Usage:
//
//   procedure CurStepChanged(CurStep: TSetupStep);
//   begin
//     OpenCandyCurStepChanged(CurStep);
//   end;
//

procedure OpenCandyCurStepChanged(CurStep: TSetupStep);
begin
	if _OCEnabledAndReady() then
	begin
		// ssInstall is just before the product installation starts
		if (CurStep = ssInstall) and gl_OC_bUseOfferPage then
			if {#OC_OFFER_TYPE_EMBEDDED} = _OCDLL_OCGetOfferType() then
				_OpenCandyExecOfferInternal();

		// ssDone is just before Setup terminates after a successful install
		if CurStep = ssDone then
		begin
			if gl_OC_bUseOfferPage and ({#OC_OFFER_TYPE_NORMAL} = _OCDLL_OCGetOfferType()) then
				_OpenCandyExecOfferInternal();
			gl_OC_bProductInstallSuccess := true;
			_OCDLL_OCSignalProductInstalled();
		end;
	end;
end;



//
// OpenCandyDeinitializeSetup()
// ----------------------------
// This should be called from DeinitializeSetup() Inno script event function.
// It signals product installation success or failure, and cleans up the
// OpenCandy library.
//
// Usage:
//   procedure DeinitializeSetup();
//   begin
//     OpenCandyDeinitializeSetup();
//   end;
//

procedure OpenCandyDeinitializeSetup();
begin
	if _OCEnabledAndReady() then
	begin
		if not gl_OC_bProductInstallSuccess then
			_OCDLL_OCSignalProductFailed();
		if gl_OC_bUseOfferPage then
		begin
			if gl_OC_bAttached then
			begin
				_OCDLL_OCDetach();
				gl_OC_bAttached := false;
			end;
			_OCDLL_OCShutdown();
		end;
	end;
end;



//---------------------------------------------------------------------------//
//                    END of OpenCandy Helper Include file                   //
//---------------------------------------------------------------------------//