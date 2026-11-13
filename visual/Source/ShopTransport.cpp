#include "ShopTransport.h"
#include "axmol.h"
#if defined(_WIN32)
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
namespace {
struct Handle { HINTERNET value; ~Handle() { if(value) WinHttpCloseHandle(value); } operator HINTERNET() const { return value; } };
std::wstring wide(const std::string& s) { return std::wstring(s.begin(),s.end()); }
}
#elif AX_TARGET_PLATFORM == AX_PLATFORM_ANDROID
#include "platform/android/jni/JniHelper.h"
#endif
std::string ShopRequest(const std::string& url,const std::string& token,const std::string& body,const std::string& key) {
    if(url.rfind("https://",0)!=0) return {};
#if defined(_WIN32)
    auto address=wide(url);
    URL_COMPONENTS parts{}; parts.dwStructSize=sizeof(parts);
    parts.dwHostNameLength=parts.dwUrlPathLength=parts.dwExtraInfoLength=parts.dwUserNameLength=parts.dwPasswordLength=static_cast<DWORD>(-1);
    if(!WinHttpCrackUrl(address.c_str(),0,0,&parts) || parts.nScheme!=INTERNET_SCHEME_HTTPS || parts.dwUserNameLength || parts.dwPasswordLength) return {};
    const std::wstring host(parts.lpszHostName,parts.dwHostNameLength);
    std::wstring path(parts.lpszUrlPath,parts.dwUrlPathLength);
    if(parts.dwExtraInfoLength) path.append(parts.lpszExtraInfo,parts.dwExtraInfoLength);
    Handle session{WinHttpOpen(L"JacaRun Shop/1",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0)};
    if(!session) return {};
    WinHttpSetTimeouts(session,10000,10000,15000,15000);
    Handle connection{WinHttpConnect(session,host.c_str(),parts.nPort,0)};
    if(!connection) return {};
    Handle request{WinHttpOpenRequest(connection,body.empty()?L"GET":L"POST",path.c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE)};
    if(!request) return {};
    DWORD policy=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    if(!WinHttpSetOption(request,WINHTTP_OPTION_REDIRECT_POLICY,&policy,sizeof(policy))) return {};
    auto headers=wide("Content-Type: application/json\r\nAuthorization: Bearer "+token+"\r\nIdempotency-Key: "+key+"\r\n");
    if(!WinHttpSendRequest(request,headers.c_str(),static_cast<DWORD>(headers.size()),body.empty()?nullptr:const_cast<char*>(body.data()),static_cast<DWORD>(body.size()),static_cast<DWORD>(body.size()),0) || !WinHttpReceiveResponse(request,nullptr)) return {};
    DWORD status=0,size=sizeof(status);
    if(!WinHttpQueryHeaders(request,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&status,&size,WINHTTP_NO_HEADER_INDEX) || status<200 || status>=300) return {};
    std::string result; char buffer[4096]; DWORD count=0;
    do { if(!WinHttpReadData(request,buffer,sizeof(buffer),&count)) return {}; result.append(buffer,count); if(result.size()>65536) return {}; } while(count);
    return result;
#elif AX_TARGET_PLATFORM == AX_PLATFORM_ANDROID
    return ax::JniHelper::callStaticStringMethod("dev/axmol/app/AppActivity","shopRequest",url,token,body,key);
#else
    return {}; // Platform must supply a verified native TLS implementation before enabling purchases.
#endif
}
