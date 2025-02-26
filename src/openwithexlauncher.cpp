#include "openwithexlauncher.h"
#include <stdio.h>
#include <shlwapi.h>
#include <stdarg.h>
#include "wil/com.h"
#include "wil/resource.h"
#include "iobjectwithopenwithflags.h"

// Disable debug calls if we are in release mode:
#ifdef NDEBUG
	#define Log(...)
	#define DebugSetMethodName(METHOD_NAME)
	#define LogReturn(RETURNVALUE) { return RETURNVALUE; }
#endif

#ifndef NDEBUG
	#define DebugSetMethodName(METHOD_NAME) LPCWSTR method = METHOD_NAME;

	#define LogReturn(RETURNVALUE, METHODNAME, FORMAT, ...)       \
	{                                                             \
		Log(METHODNAME, FORMAT, __VA_ARGS__);                     \
        Log(METHODNAME, L"<return value = %s>", L#RETURNVALUE);   \
        return RETURNVALUE;                                       \
	}
#endif

#pragma region "Debugging"
#ifndef NDEBUG
DWORD COpenWithExLauncher::s_instCounter = 0;

void COpenWithExLauncher::Log(const wchar_t *szMethodName, const wchar_t *format, ...)
{
	va_list args;
	va_start(args, format);

	WCHAR buffer[2048];

	vswprintf_s(buffer, format, args);
	wprintf_s(L"[%s:%d] %s", szMethodName, m_instId, buffer);

	va_end(args);
}
#endif
#pragma endregion

#pragma region "IUnknown"
#define QI_PUT_OUT(INTERFACE)                              \
	if (riid == IID_ ## INTERFACE)                         \
	{                                                      \
		*ppvObj = static_cast<INTERFACE *>(this);          \
		Log(method, L"Found interface %s\n", L#INTERFACE); \
        fFailedToFind = false;                             \
	}

STDMETHODIMP COpenWithExLauncher::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	DebugSetMethodName(L"COpenWithExLauncher::QueryInterface");
	HRESULT hr = E_NOINTERFACE;

	Log(method, L"Entered method\n");

	LPWSTR lpString = nullptr;
	(void)StringFromCLSID(riid, &lpString);
	if (lpString && lpString[0])
	{
		Log(method, L"Querying %s\n", lpString);
		CoTaskMemFree(lpString);
	}

	if (!ppvObj)
	{
		Log(method, L"ppvObj is null\n");
		Log(method, L"Exiting method\n");
		return E_INVALIDARG;
	}

	*ppvObj = nullptr;

	bool fFailedToFind = true;

	// For whatever reason, static_cast<IUnknown *> raises a comple error for me
	// (aubymori), but not for the other person (kawapure). Masterful gambit,
	// Microsoft!
	if (riid == IID_IUnknown)
	{
		*ppvObj = this;
		Log(method, L"Found interface IUnknown\n");
		fFailedToFind = false;
	}
	QI_PUT_OUT(IExecuteCommandApplicationHostEnvironment)
	QI_PUT_OUT(IServiceProvider)
	QI_PUT_OUT(IObjectWithSite)
	QI_PUT_OUT(IObjectWithAssociationElement)
	QI_PUT_OUT(IObjectWithSelection)
	QI_PUT_OUT(IInitializeCommand)
	QI_PUT_OUT(IExecuteCommand)
	QI_PUT_OUT(IOpenWithLauncher)
	QI_PUT_OUT(IClassFactory);

	if (!fFailedToFind)
	{
		hr = S_OK;
		AddRef();
	}
	else
	{
		Log(method, L"No such interface supported\n");
		hr = E_NOINTERFACE;
	}

	Log(method, L"Exiting method\n");
	return hr;
}


STDMETHODIMP_(ULONG) COpenWithExLauncher::AddRef()
{
	DebugSetMethodName(L"COpenWithExLauncher::AddRef");

	Log(method, L"Entered method\n");
	auto rv = InterlockedIncrement(&m_cRef);
	Log(method, L"Exiting method\n");

	return rv;
}

STDMETHODIMP_(ULONG) COpenWithExLauncher::Release()
{
	DebugSetMethodName(L"COpenWithExLauncher::Release");

	Log(method, L"Entered method\n");

	ULONG ulRefCount = InterlockedDecrement(&m_cRef);
	if (0 == ulRefCount)
	{
		delete this;
		debuglog(L"[COpenWithExLauncher::Release] Exiting method for now-deleted object\n");
	}
	else
	{
		Log(method, L"Exiting method\n");
	}

	return ulRefCount;
}
#pragma endregion // "IUnknown"

#pragma region "IExecuteCommandApplicationHostEnvironment"
STDMETHODIMP COpenWithExLauncher::GetValue(AHE_TYPE *pahe)
{
	DebugSetMethodName(L"COpenWithExLauncher::GetValue");

	Log(method, L"Entered method\n");

	// Windows will pass in null for this and complain if it fails.
	if (pahe)
	{
		// Matching modern Open With, despite being a desktop app. I don't think it
		// matters much, due to the above comment.
		*pahe = AHE_IMMERSIVE;
	}
	else
	{
		Log(method, L"pahe is a null pointer, so the operating system wants us to fail\n");
	}

	Log(method, L"Exiting method\n");
	return pahe ? S_OK : E_FAIL;
}
#pragma endregion // "IExecuteCommandApplicationHostEnvironment"

#pragma region "IServiceProvider"
STDMETHODIMP COpenWithExLauncher::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
	DebugSetMethodName(L"COpenWithExLauncher::QueryService");

	Log(method, L"Entered method\n");

	LPWSTR lpString = nullptr;
	LPWSTR lpString2 = nullptr;
	(void)StringFromCLSID(guidService, &lpString);
	(void)StringFromCLSID(riid, &lpString2);
	if (lpString && lpString[0] && lpString2 && lpString2[0])
	{
		Log(method, L"Querying service %s with SID %s\n", lpString, lpString2);
		CoTaskMemFree(lpString);
		CoTaskMemFree(lpString2);
	}

	Log(method, L"Exiting method\n");
	return E_NOTIMPL;// IUnknown_QueryService(this, guidService, riid, ppvObject);
}
#pragma endregion // "IServiceProvider"

#pragma region "IObjectWithSite"
STDMETHODIMP COpenWithExLauncher::GetSite(REFIID riid, void **ppvObject)
{
	DebugSetMethodName(L"COpenWithExLauncher::GetSite");

	Log(method, L"Entered method\n");

	if (m_pSite)
	{
		LogReturn(m_pSite->QueryInterface(riid, ppvObject), method, L"Exited method\n");
	}
	else
	{
		LogReturn(E_FAIL, method, L"Exited method\n");
	}
}

STDMETHODIMP COpenWithExLauncher::SetSite(IUnknown *pUnkSite)
{
	DebugSetMethodName(L"COpenWithExLauncher::SetSite");

	Log(method, L"Entered method\n");
	IUnknown_Set(&m_pSite, pUnkSite);
	Log(method, L"Exiting method\n");
	return S_OK;
}
#pragma endregion // "IObjectWithSite"

#pragma region "IObjectWithAssociationElement"
STDMETHODIMP COpenWithExLauncher::SetAssocElement(IAssociationElement *pae)
{
	DebugSetMethodName(L"COpenWithExLauncher::SetAssocElement");

	Log(method, L"Entered method\n");
	IUnknown_Set((IUnknown **)&m_pAssocElm, pae);
	Log(method, L"Exiting method\n");
	return S_OK;
}

STDMETHODIMP COpenWithExLauncher::GetAssocElement(REFIID riid, void **ppv)
{
	DebugSetMethodName(L"COpenWithExLauncher::GetAssocElement");

	Log(method, L"Entered method\n");
	if (m_pAssocElm)
	{
		LogReturn(m_pAssocElm->QueryInterface(riid, ppv), method, L"Exited method\n");
	}
	else
	{
		LogReturn(E_FAIL, method, L"Exited method\n");
	}
}
#pragma endregion // "IObjectWithAssociationElement"

#pragma region "IObjectWithSelection"
STDMETHODIMP COpenWithExLauncher::GetSelection(REFIID riid, void **ppv)
{
	DebugSetMethodName(L"COpenWithExLauncher::GetSelection");

	Log(method, L"Entered method\n");
	if (m_pSelection)
	{
		LogReturn(m_pSelection->QueryInterface(riid, ppv), method, L"Exited method\n");
	}
	else
	{
		LogReturn(E_FAIL, method, L"Exited method\n");
	}
}

STDMETHODIMP COpenWithExLauncher::SetSelection(IShellItemArray *psia)
{
	DebugSetMethodName(L"COpenWithExLauncher::SetSelection");

	Log(method, L"Entered method\n");
	IUnknown_Set((IUnknown **)&m_pSelection, psia);
	Log(method, L"Exiting method\n");
	return S_OK;
}
#pragma endregion // "IObjectWithSelection"

#pragma region "IInitializeCommand"
STDMETHODIMP COpenWithExLauncher::Initialize(LPCWSTR pszCommandName, IPropertyBag *ppb)
{
	DebugSetMethodName(L"COpenWithExLauncher::Initialize");

	Log(method, L"Entered method\n");
	Log(method, L"Exiting method\n");
	return S_OK;
}
#pragma endregion // "IInitializeCommand"

#pragma region "IExecuteCommand"
STDMETHODIMP COpenWithExLauncher::SetKeyState(DWORD grfKeyState)
{
	DebugSetMethodName(L"COpenWithExLauncher::SetKeyState");

	Log(method, L"Entering method\n");
	Log(method, L"Setting key state to 0x%X\n", grfKeyState);
	m_dwKeyState = grfKeyState;
	Log(method, L"Exiting method\n");
	return S_OK;
}

STDMETHODIMP COpenWithExLauncher::SetParameters(__RPC__in_string LPCWSTR pszParameters)
{
	DebugSetMethodName(L"COpenWithExLauncher::SetParameters");

	Log(method, L"Entering method\n");
	Log(method, L"Setting parameters to %s\n", pszParameters);
	Str_SetPtrW(&m_pszParameters, pszParameters);
	Log(method, L"Exiting method\n");
	return S_OK;
}

STDMETHODIMP COpenWithExLauncher::SetPosition(POINT pt)
{
	DebugSetMethodName(L"COpenWithExLauncher::SetPosition");

	Log(method, L"Entering method\n");
	Log(method, L"Setting position to (%i, %i)\n", pt.x, pt.y);
	m_position = pt;
	Log(method, L"Exiting method\n");
	return S_OK;
}

STDMETHODIMP COpenWithExLauncher::SetShowWindow(int nShow)
{
	DebugSetMethodName(L"COpenWithExLauncher::SetShowWindow");

	Log(method, L"Entering method\n");
	Log(method, L"Setting show window to %i\n", nShow);
	m_nShow = nShow;
	Log(method, L"Exiting method\n");
	return S_OK;
}

STDMETHODIMP COpenWithExLauncher::SetNoShowUI(BOOL fNoShowUI)
{
	DebugSetMethodName(L"COpenWithExLauncher::SetNoShowUI");

	Log(method, L"Entering method\n");
	Log(method, L"Setting no show UI to %s\n", fNoShowUI ? L"TRUE" : L"FALSE");
	m_fNoShowUI = fNoShowUI;
	Log(method, L"Exiting method\n");
	return S_OK;
}

STDMETHODIMP COpenWithExLauncher::SetDirectory(__RPC__in_string LPCWSTR pszDirectory)
{
	DebugSetMethodName(L"COpenWithExLauncher::SetDirectory");

	Log(method, L"Entering method\n");
	Log(method, L"Setting directory to %s\n", pszDirectory);
	Str_SetPtrW(&m_pszDirectory, pszDirectory);
	Log(method, L"Exiting method\n");
	return S_OK;
}

STDMETHODIMP COpenWithExLauncher::Execute()
{
	DebugSetMethodName(L"COpenWithExLauncher::Execute");

	Log(method, L"Entering method\n");

	/* Get flags */
	IMMERSIVE_OPENWITH_FLAGS flags = IMMERSIVE_OPENWITH_NONE;
	if (m_pSite)
	{
		Log(method, L"Site exists, trying to get flags\n");
		wil::com_ptr<IObjectWithOpenWithFlags> powf;
		if (SUCCEEDED(IUnknown_QueryService(
			m_pSite,
			IID_IObjectWithOpenWithFlags,
			IID_IObjectWithOpenWithFlags,
			(void **)powf.put()
		)))
		{
			Log(method, L"IObjectWithOpenWithFlags object obtained\n");
			powf->get_Flags(&flags);
		}
	}
	// This get_Flags function is a bit finnicky, just always allow registration.
	*(DWORD *)&flags |= IMMERSIVE_OPENWITH_OVERRIDE;
	Log(method, L"Flags: 0x%X\n", flags);

	WCHAR szPath[MAX_PATH] = { 0 };
	if (m_pSelection)
	{
		wil::com_ptr<IShellItem> psi;
		if (SUCCEEDED(m_pSelection->GetItemAt(0, psi.put())))
		{
			Log(method, L"Got selected shell item\n");
			LPWSTR pszPath = nullptr;
			HRESULT hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
			if (FAILED(hr))
			{
				hr = psi->GetDisplayName(SIGDN_URL, &pszPath);
			}
			if (SUCCEEDED(hr))
			{
				wcscpy_s(szPath, pszPath);
				CoTaskMemFree(pszPath);
			}
		}
	}
	if (*szPath)
	{
		ShowOpenWithDialog(NULL, szPath, flags);
	}
	else
	{
		LocalizedMessageBox(
			NULL,
			IDS_ERR_NOPATH,
			MB_ICONERROR
		);
	}
	PostThreadMessageW(GetCurrentThreadId(), 0x8001, 0, 0);
	Log(method, L"Exiting method\n");
	return S_OK;
}
#pragma endregion

#pragma region "IOpenWithLauncher"
STDMETHODIMP COpenWithExLauncher::Launch(HWND hWndParent, LPCWSTR lpszPath, IMMERSIVE_OPENWITH_FLAGS flags)
{
	debuglog(
		L"[COpenWithExLauncher] IOpenWithLauncher::Launch info:\n"
		L"\nhWndParent: 0x%X\nlpszPath: %s\nflags : 0x%X\n",
		hWndParent, lpszPath, flags
	);
	ShowOpenWithDialog(hWndParent, lpszPath, flags);
	PostThreadMessageW(GetCurrentThreadId(), 0x8001, 0, 0);
	return S_OK;
}
#pragma endregion // "IOpenWithLauncher"

#pragma region "IClassFactory"
STDMETHODIMP COpenWithExLauncher::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject)
{
	DebugSetMethodName(L"COpenWithExLauncher::CreateInstance");

	Log(method, L"Entered method\n");

	if (!ppvObject)
	{
		Log(method, L"ppvObject is null\n");
		return E_INVALIDARG;
	}

	LPWSTR lpString = nullptr;
	(void)StringFromCLSID(riid, &lpString);
	Log(method, L"riid is %s\n", lpString);

	if (riid == IID_IUnknown)
	{
		Log(method, L"riid is IUnknown, putting that out...\n");
		*ppvObject = this;
	}

	Log(method, L"Exiting method\n");
	return S_OK;
}

STDMETHODIMP COpenWithExLauncher::LockServer(BOOL fLock)
{
	DebugSetMethodName(L"COpenWithExLauncher::LockServer");

	Log(method, L"Entered method\n");
	Log(method, L"Exiting method\n");
	return S_OK;
}
#pragma endregion // "IClassFactory"

#pragma region "COpenWithExLauncher"
COpenWithExLauncher::COpenWithExLauncher()
	: m_pSite(nullptr)
	, m_pSelection(nullptr)
{
	DebugSetMethodName(L"COpenWithExLauncher::COpenWithExLauncher");

	// Intentionally not using Log because instance ID isn't set up
	// yet.
	debuglog(L"[COpenWithExLauncher::COpenWithExLauncher] Entered method\n");

#ifndef NDEBUG
	m_instId = s_instCounter++;
	Log(method, L"Set instance ID to %d\n", m_instId);
#endif

	Log(method, L"Exiting method\n");
}

COpenWithExLauncher::~COpenWithExLauncher()
{
	DebugSetMethodName(L"COpenWithExLauncher::~COpenWithExLauncher");

	Log(method, L"Entered method\n");

	if (m_pSite)
		m_pSite->Release();

	if (m_pAssocElm)
		m_pAssocElm->Release();

	if (m_pSelection)
		m_pSelection->Release();

	Log(method, L"Exiting method\n");
}

HRESULT COpenWithExLauncher::RunMessageLoop()
{
	DebugSetMethodName(L"COpenWithExLauncher::RunMessageLoop");

	Log(method, L"Entered method\n");

	DWORD dwRegister = 0;

	Log(method, L"Entering CoRegisterClassObject\n");

	wil::com_ptr<IExecuteCommand> pExec;
	HRESULT hr = QueryInterface(IID_IExecuteCommand, (void **)&pExec);

	if (FAILED(hr))
	{
		Log(method, L"Failed to get IExecuteCommand\n");
		return hr;
	}

	hr = CoRegisterClassObject(CLSID_ExecuteUnknown, pExec.get(), CLSCTX_LOCAL_SERVER, NULL, &dwRegister);
	Log(method, L"Exiting CoRegisterClassObject\n");
	if (FAILED(hr))
	{
		return hr;
	}

	MSG msg;
	while (true)
	{
		if (GetMessageW(&msg, NULL, 0, 0) <= 0)
		{
			Log(method, L"Exiting because message loop is over\n");
			CoRevokeClassObject(dwRegister);
			return hr;
		}

		if (msg.message)
		{
			Log(method, L"Received message 0x%X 0x%X 0x%X 0x%X\n", msg.message, msg.hwnd, msg.wParam, msg.lParam);
			if (msg.message == 0x8001)
			{
				break;
			}
		}

		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	Log(method, L"Exiting method\n");
	return S_OK;
}
#pragma endregion // "COpenWithExLauncher"