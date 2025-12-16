// Minimal helper: Registers app capabilities for .mrml/.mrb and launches
// Windows Default Apps UI for user to confirm associations.
// This is not silent: Windows requires explicit user consent on Win10/11.

#include <windows.h>
#include <winreg.h>
#include <shobjidl.h>
#include <ShlObj.h>
#include <strsafe.h>

static void WriteRegString(HKEY root, const wchar_t* subkey, const wchar_t* valueName, const wchar_t* data)
{
  HKEY hKey = nullptr;
  if (RegCreateKeyExW(root, subkey, 0, nullptr, 0, KEY_SET_VALUE | KEY_WOW64_64KEY, nullptr, &hKey, nullptr) == ERROR_SUCCESS)
  {
    const DWORD cb = (DWORD)((wcslen(data) + 1) * sizeof(wchar_t));
    RegSetValueExW(hKey, valueName, 0, REG_SZ, reinterpret_cast<const BYTE*>(data), cb);
    RegCloseKey(hKey);
  }
}

static void WriteRegEmptyString(HKEY root, const wchar_t* subkey, const wchar_t* valueName)
{
  HKEY hKey = nullptr;
  if (RegCreateKeyExW(root, subkey, 0, nullptr, 0, KEY_SET_VALUE | KEY_WOW64_64KEY, nullptr, &hKey, nullptr) == ERROR_SUCCESS)
  {
    const wchar_t* empty = L"";
    const DWORD cb = (DWORD)((wcslen(empty) + 1) * sizeof(wchar_t));
    RegSetValueExW(hKey, valueName, 0, REG_SZ, reinterpret_cast<const BYTE*>(empty), cb);
    RegCloseKey(hKey);
  }
}

static void WriteRegDWORD(HKEY root, const wchar_t* subkey, const wchar_t* valueName, DWORD data)
{
  HKEY hKey = nullptr;
  if (RegCreateKeyExW(root, subkey, 0, nullptr, 0, KEY_SET_VALUE | KEY_WOW64_64KEY, nullptr, &hKey, nullptr) == ERROR_SUCCESS)
  {
    RegSetValueExW(hKey, valueName, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&data), sizeof(DWORD));
    RegCloseKey(hKey);
  }
}

static bool IsProcessElevated()
{
  HANDLE token = nullptr;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
    return false;
  TOKEN_ELEVATION elevation = {};
  DWORD outLen = 0;
  BOOL ok = GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &outLen);
  CloseHandle(token);
  return ok && elevation.TokenIsElevated;
}

#ifndef RRF_SUBKEY_WOW6464KEY
#define RRF_SUBKEY_WOW6464KEY 0x00010000
#endif

static bool HasRegisteredAppUser()
{
  DWORD cb = 0;
  LONG r = RegGetValueW(HKEY_CURRENT_USER,
    L"Software\\RegisteredApplications",
    L"Alice",
    RRF_RT_REG_SZ | RRF_SUBKEY_WOW6464KEY,
    nullptr, nullptr, &cb);
  return r == ERROR_SUCCESS && cb > sizeof(wchar_t);
}

static bool HasRegisteredAppMachine()
{
  DWORD cb = 0;
  LONG r = RegGetValueW(HKEY_LOCAL_MACHINE,
    L"Software\\RegisteredApplications",
    L"Alice",
    RRF_RT_REG_SZ | RRF_SUBKEY_WOW6464KEY,
    nullptr, nullptr, &cb);
  return r == ERROR_SUCCESS && cb > sizeof(wchar_t);
}

static int AssocMain()
{
  // Application identity used for RegisteredApplications; must match below
  const wchar_t* appName = L"Alice"; // display name and RegisteredApplications key
  const wchar_t* company = L"Alice Labs"; // vendor for clarity

  // Resolve install dir from current executable path
  wchar_t exePath[MAX_PATH] = {0};
  GetModuleFileNameW(nullptr, exePath, MAX_PATH);
  // Trim to directory
  wchar_t* lastSlash = wcsrchr(exePath, L'\\');
  if (!lastSlash) return 0;
  *lastSlash = 0; // now exePath is bin dir

  // Expected main app path (launcher) one level up if needed
  // But we register command to current exe dir parent\Alice.exe by convention
  wchar_t appExe[MAX_PATH] = {0};
  StringCchPrintfW(appExe, MAX_PATH, L"%s\\Alice.exe", exePath);
  // Icon spec variants
  wchar_t capIcon[600] = {0};
  StringCchPrintfW(capIcon, 600, L"%s,0", appExe);
  wchar_t iconSpec[600] = {0};
  StringCchPrintfW(iconSpec, 600, L"\"%s\",0", appExe);

  // 1) Register capabilities under HKCU (per-user)
  // HKCU\Software\RegisteredApplications
  WriteRegString(HKEY_CURRENT_USER, L"Software\\RegisteredApplications", appName, L"Software\\Alice\\Capabilities");
  // HKCU\Software\Alice\Capabilities
  WriteRegString(HKEY_CURRENT_USER, L"Software\\Alice\\Capabilities", L"ApplicationName", appName);
  WriteRegString(HKEY_CURRENT_USER, L"Software\\Alice\\Capabilities", L"ApplicationDescription", L"Alice - MRML/MRB scene viewer");
  WriteRegString(HKEY_CURRENT_USER, L"Software\\Alice\\Capabilities", L"ApplicationCompany", company);
  WriteRegString(HKEY_CURRENT_USER, L"Software\\Alice\\Capabilities", L"ApplicationIcon", capIcon);
  // File associations
  WriteRegString(HKEY_CURRENT_USER, L"Software\\Alice\\Capabilities\\FileAssociations", L".mrml", L"Alice.MRML");
  WriteRegString(HKEY_CURRENT_USER, L"Software\\Alice\\Capabilities\\FileAssociations", L".mrb",  L"Alice.MRB");

  // 2) Register ProgIDs with open command
  WriteRegString(HKEY_CURRENT_USER, L"Software\\Classes\\Alice.MRML", nullptr, L"Alice MRML Scene");
  WriteRegString(HKEY_CURRENT_USER, L"Software\\Classes\\Alice.MRML\\DefaultIcon", nullptr, iconSpec);
  WriteRegString(HKEY_CURRENT_USER, L"Software\\Classes\\Alice.MRML\\shell", nullptr, L"open");
  wchar_t openCmd[512] = {0};
  StringCchPrintfW(openCmd, 512, L"\"%s\" \"%%1\"", appExe);
  WriteRegString(HKEY_CURRENT_USER, L"Software\\Classes\\Alice.MRML\\shell\\open\\command", nullptr, openCmd);

  WriteRegString(HKEY_CURRENT_USER, L"Software\\Classes\\Alice.MRB", nullptr, L"Alice MRB Scene Bundle");
  WriteRegString(HKEY_CURRENT_USER, L"Software\\Classes\\Alice.MRB\\DefaultIcon", nullptr, iconSpec);
  WriteRegString(HKEY_CURRENT_USER, L"Software\\Classes\\Alice.MRB\\shell", nullptr, L"open");
  WriteRegString(HKEY_CURRENT_USER, L"Software\\Classes\\Alice.MRB\\shell\\open\\command", nullptr, openCmd);

  // Add to OpenWithProgids under Classes (must be REG_SZ empty data)
  WriteRegEmptyString(HKEY_CURRENT_USER, L"Software\\Classes\\.mrml\\OpenWithProgids", L"Alice.MRML");
  WriteRegEmptyString(HKEY_CURRENT_USER, L"Software\\Classes\\.mrb\\OpenWithProgids",  L"Alice.MRB");

  // Also register under Applications to show up in Open With and Recommended lists
  WriteRegString(HKEY_CURRENT_USER, L"Software\\Classes\\Applications\\Alice.exe\\shell\\open\\command", nullptr, openCmd);
  WriteRegEmptyString(HKEY_CURRENT_USER, L"Software\\Classes\\Applications\\Alice.exe\\SupportedTypes", L".mrml");
  WriteRegEmptyString(HKEY_CURRENT_USER, L"Software\\Classes\\Applications\\Alice.exe\\SupportedTypes", L".mrb");

  // App Paths to help Windows locate the executable without browsing
  WriteRegString(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\Alice.exe", nullptr, appExe);
  WriteRegString(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\Alice.exe", L"Path", exePath);

  // Optional machine-wide registration when elevated (all users can see Alice)
  if (IsProcessElevated())
  {
    // RegisteredApplications (HKLM) -> Capabilities
    WriteRegString(HKEY_LOCAL_MACHINE, L"Software\\RegisteredApplications", appName, L"Software\\Alice\\Capabilities");
    WriteRegString(HKEY_LOCAL_MACHINE, L"Software\\Alice\\Capabilities", L"ApplicationName", appName);
    WriteRegString(HKEY_LOCAL_MACHINE, L"Software\\Alice\\Capabilities", L"ApplicationDescription", L"Alice - MRML/MRB scene viewer");
    WriteRegString(HKEY_LOCAL_MACHINE, L"Software\\Alice\\Capabilities", L"ApplicationCompany", company);
    WriteRegString(HKEY_LOCAL_MACHINE, L"Software\\Alice\\Capabilities", L"ApplicationIcon", capIcon);
    WriteRegString(HKEY_LOCAL_MACHINE, L"Software\\Alice\\Capabilities\\FileAssociations", L".mrml", L"Alice.MRML");
    WriteRegString(HKEY_LOCAL_MACHINE, L"Software\\Alice\\Capabilities\\FileAssociations", L".mrb",  L"Alice.MRB");

    // Applications entry (HKLM) for Recommended Programs
    WriteRegString(HKEY_LOCAL_MACHINE, L"Software\\Classes\\Applications\\Alice.exe\\shell\\open\\command", nullptr, openCmd);
    WriteRegEmptyString(HKEY_LOCAL_MACHINE, L"Software\\Classes\\Applications\\Alice.exe\\SupportedTypes", L".mrml");
    WriteRegEmptyString(HKEY_LOCAL_MACHINE, L"Software\\Classes\\Applications\\Alice.exe\\SupportedTypes", L".mrb");

    // ProgIDs (HKLM) - optional but helps global visibility
    WriteRegString(HKEY_LOCAL_MACHINE, L"Software\\Classes\\Alice.MRML", nullptr, L"Alice MRML Scene");
    WriteRegString(HKEY_LOCAL_MACHINE, L"Software\\Classes\\Alice.MRML\\DefaultIcon", nullptr, iconSpec);
    WriteRegString(HKEY_LOCAL_MACHINE, L"Software\\Classes\\Alice.MRML\\shell", nullptr, L"open");
    WriteRegString(HKEY_LOCAL_MACHINE, L"Software\\Classes\\Alice.MRML\\shell\\open\\command", nullptr, openCmd);
    WriteRegString(HKEY_LOCAL_MACHINE, L"Software\\Classes\\Alice.MRB", nullptr, L"Alice MRB Scene Bundle");
    WriteRegString(HKEY_LOCAL_MACHINE, L"Software\\Classes\\Alice.MRB\\DefaultIcon", nullptr, iconSpec);
    WriteRegString(HKEY_LOCAL_MACHINE, L"Software\\Classes\\Alice.MRB\\shell", nullptr, L"open");
    WriteRegString(HKEY_LOCAL_MACHINE, L"Software\\Classes\\Alice.MRB\\shell\\open\\command", nullptr, openCmd);

    // OpenWithProgids (HKLM Classes)
    WriteRegEmptyString(HKEY_LOCAL_MACHINE, L"Software\\Classes\\.mrml\\OpenWithProgids", L"Alice.MRML");
    WriteRegEmptyString(HKEY_LOCAL_MACHINE, L"Software\\Classes\\.mrb\\OpenWithProgids",  L"Alice.MRB");

    // App Paths (HKLM) - optional convenience
    WriteRegString(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\Alice.exe", nullptr, appExe);
    WriteRegString(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\Alice.exe", L"Path", exePath);
  }

  // (Optional) Explorer OpenWithList is a user MRU. Not required; omit for cleanliness.

  // 3) Notify shell
  SHChangeNotify(0x8000000 /*SHCNE_ASSOCCHANGED*/, 0, 0, 0);
  Sleep(500);

  // 4) Launch default apps UI for user confirmation (Win10/11)
  // Try modern Settings deep link (user by default, or machine if '/machine' present),
  // then fallback to defaults root, then legacy control panel, then COM UI.
  // Return early on first success to avoid multiple windows or double navigation.
  // Choose one URI based on actual registration presence; do not navigate twice.
  const wchar_t* target = nullptr;
  if (HasRegisteredAppUser()) {
    target = L"ms-settings:defaultapps?registeredAppUser=Alice";
  } else if (HasRegisteredAppMachine()) {
    target = L"ms-settings:defaultapps?registeredAppMachine=Alice";
  } else {
    target = L"ms-settings:defaultapps"; // extreme fallback
  }
  HINSTANCE shex = ShellExecuteW(nullptr, L"open", target, nullptr, nullptr, SW_SHOWNORMAL);
  if ((INT_PTR)shex > 32) return 0;

  shex = ShellExecuteW(nullptr, L"open", L"ms-settings:defaultapps", nullptr, nullptr, SW_SHOWNORMAL);
  if ((INT_PTR)shex > 32) return 0;

  shex = ShellExecuteW(nullptr, L"open", L"control.exe", L"/name Microsoft.DefaultPrograms /page pageDefaultProgram", nullptr, SW_SHOWNORMAL);
  if ((INT_PTR)shex > 32) return 0;

  HRESULT hr = CoInitialize(nullptr);
  if (SUCCEEDED(hr))
  {
    IApplicationAssociationRegistrationUI* ui = nullptr;
    const CLSID clsid = {0x3AA7AF7E, 0x9B36, 0x420C, {0xA8,0xE3,0xF7,0x7D,0x46,0x74,0xA4,0x88}};
    const IID   iid   = {0x1F76A169, 0xF994, 0x40AC, {0x8F,0xC8,0x09,0x59,0xE8,0x87,0x47,0x10}};
    hr = CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER, iid, reinterpret_cast<void**>(&ui));
    if (SUCCEEDED(hr) && ui)
    {
      ui->LaunchAdvancedAssociationUI(appName);
      ui->Release();
    }
    CoUninitialize();
  }
  return 0;
}

#ifdef _WIN32
int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
  return AssocMain();
}
#endif

#ifndef _WIN32
int wmain()
{
  return AssocMain();
}
#endif
