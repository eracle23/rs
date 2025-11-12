#ifndef __RadianceApp_BrandingPreferences_h
#define __RadianceApp_BrandingPreferences_h

#include <QtGlobal>
#include <QString>

namespace RadianceBranding
{

inline bool brandStyleEnabled()
{
  if (qEnvironmentVariableIsSet("ALICE_ENABLE_BRAND_STYLE"))
    {
    return qEnvironmentVariableIntValue("ALICE_ENABLE_BRAND_STYLE") != 0;
    }
  if (qEnvironmentVariableIsSet("ALICE_NATIVE_STYLE"))
    {
    return qEnvironmentVariableIntValue("ALICE_NATIVE_STYLE") == 0;
    }
  return false;
}

inline bool nativeStyleEnabled()
{
  return !brandStyleEnabled();
}

}

#endif
