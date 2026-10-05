#include "net.hpp"
#include <windows.h>
#include <winhttp.h>
#include <thread>
#include <fstream>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "winhttp.lib")

namespace sense {
namespace net {

static std::wstring toWide(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring wstr(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size);
    return wstr;
}

static std::string toUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string str(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], size, nullptr, nullptr);
    return str;
}

static HttpResponse executeHttpRequest(
    const std::string& method,
    const std::string& url,
    const std::string& body,
    const std::string& contentType,
    const std::map<std::string, std::string>& headers
) {
    HttpResponse response;
    std::wstring wUrl = toWide(url);

    URL_COMPONENTS urlComp = {};
    urlComp.dwStructSize = sizeof(urlComp);
    wchar_t hostName[256] = {};
    wchar_t urlPath[2048] = {};
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = ARRAYSIZE(hostName);
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = ARRAYSIZE(urlPath);

    if (!WinHttpCrackUrl(wUrl.c_str(), static_cast<DWORD>(wUrl.length()), 0, &urlComp)) {
        response.success = false;
        response.errorMessage = "Failed to parse URL";
        return response;
    }

    HINTERNET hSession = WinHttpOpen(
        L"SenseModAPI/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );
    if (!hSession) {
        response.errorMessage = "WinHttpOpen failed";
        return response;
    }

    HINTERNET hConnect = WinHttpConnect(hSession, hostName, urlComp.nPort, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        response.errorMessage = "WinHttpConnect failed";
        return response;
    }

    DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    std::wstring wMethod = toWide(method);
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        wMethod.c_str(),
        urlPath,
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags
    );
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        response.errorMessage = "WinHttpOpenRequest failed";
        return response;
    }

    // Add Headers
    std::wstring headerStr;
    if (!contentType.empty()) {
        headerStr += L"Content-Type: " + toWide(contentType) + L"\r\n";
    }
    for (const auto& kv : headers) {
        headerStr += toWide(kv.first) + L": " + toWide(kv.second) + L"\r\n";
    }

    if (!headerStr.empty()) {
        WinHttpAddRequestHeaders(
            hRequest,
            headerStr.c_str(),
            static_cast<DWORD>(headerStr.length()),
            WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE
        );
    }

    // Send Request
    BOOL bResults = WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        body.empty() ? WINHTTP_NO_REQUEST_DATA : const_cast<char*>(body.data()),
        static_cast<DWORD>(body.length()),
        static_cast<DWORD>(body.length()),
        0
    );

    if (bResults) {
        bResults = WinHttpReceiveResponse(hRequest, nullptr);
    }

    if (bResults) {
        DWORD statusCode = 0;
        DWORD size = sizeof(statusCode);
        WinHttpQueryHeaders(
            hRequest,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &statusCode,
            &size,
            WINHTTP_NO_HEADER_INDEX
        );
        response.statusCode = static_cast<int>(statusCode);

        // Read Response Body
        std::string responseBody;
        DWORD bytesAvailable = 0;
        while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
            std::vector<char> buffer(bytesAvailable + 1);
            DWORD bytesRead = 0;
            if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead)) {
                responseBody.append(buffer.data(), bytesRead);
            }
        }
        response.body = responseBody;
        response.success = (response.statusCode >= 200 && response.statusCode < 400);
    } else {
        response.errorMessage = "Request failed with error: " + std::to_string(GetLastError());
        response.success = false;
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return response;
}

HttpResponse get(const std::string& url, const std::map<std::string, std::string>& headers) {
    return executeHttpRequest("GET", url, "", "", headers);
}

HttpResponse post(
    const std::string& url,
    const std::string& body,
    const std::string& contentType,
    const std::map<std::string, std::string>& headers
) {
    return executeHttpRequest("POST", url, body, contentType, headers);
}

void getAsync(
    const std::string& url,
    std::function<void(const HttpResponse&)> callback,
    const std::map<std::string, std::string>& headers
) {
    std::thread([url, callback, headers]() {
        HttpResponse res = get(url, headers);
        if (callback) callback(res);
    }).detach();
}

void postAsync(
    const std::string& url,
    const std::string& body,
    std::function<void(const HttpResponse&)> callback,
    const std::string& contentType,
    const std::map<std::string, std::string>& headers
) {
    std::thread([url, body, callback, contentType, headers]() {
        HttpResponse res = post(url, body, contentType, headers);
        if (callback) callback(res);
    }).detach();
}

bool downloadFile(
    const std::string& url,
    const std::string& destinationPath,
    std::function<void(float progress)> progressCallback
) {
    std::ofstream outFile(destinationPath, std::ios::binary);
    if (!outFile.is_open()) return false;

    std::wstring wUrl = toWide(url);
    URL_COMPONENTS urlComp = {};
    urlComp.dwStructSize = sizeof(urlComp);
    wchar_t hostName[256] = {};
    wchar_t urlPath[2048] = {};
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = ARRAYSIZE(hostName);
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = ARRAYSIZE(urlPath);

    if (!WinHttpCrackUrl(wUrl.c_str(), static_cast<DWORD>(wUrl.length()), 0, &urlComp)) {
        return false;
    }

    HINTERNET hSession = WinHttpOpen(L"SenseModAPI/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    HINTERNET hConnect = WinHttpConnect(hSession, hostName, urlComp.nPort, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return false;
    }

    DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", urlPath, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    BOOL res = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (res) res = WinHttpReceiveResponse(hRequest, nullptr);

    if (res) {
        DWORD contentLength = 0;
        DWORD size = sizeof(contentLength);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &contentLength, &size, WINHTTP_NO_HEADER_INDEX);

        DWORD totalRead = 0;
        DWORD bytesAvailable = 0;
        while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
            std::vector<char> buffer(bytesAvailable);
            DWORD bytesRead = 0;
            if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead)) {
                outFile.write(buffer.data(), bytesRead);
                totalRead += bytesRead;
                if (progressCallback && contentLength > 0) {
                    progressCallback(static_cast<float>(totalRead) / contentLength);
                }
            }
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return res != FALSE;
}

static std::string escapeJson(const std::string& s) {
    std::ostringstream o;
    for (char c : s) {
        if (c == '"') o << "\\\"";
        else if (c == '\\') o << "\\\\";
        else if (c == '\b') o << "\\b";
        else if (c == '\f') o << "\\f";
        else if (c == '\n') o << "\\n";
        else if (c == '\r') o << "\\r";
        else if (c == '\t') o << "\\t";
        else o << c;
    }
    return o.str();
}

bool sendDiscordWebhook(const std::string& webhookUrl, const std::string& content, const std::string& username) {
    std::string jsonBody = "{\"username\": \"" + escapeJson(username) + "\", \"content\": \"" + escapeJson(content) + "\"}";
    HttpResponse res = post(webhookUrl, jsonBody, "application/json");
    return res.statusCode >= 200 && res.statusCode < 300;
}

std::string urlEncode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : value) {
        if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
        }
    }
    return escaped.str();
}

std::string urlDecode(const std::string& value) {
    std::string result;
    for (size_t i = 0; i < value.length(); ++i) {
        if (value[i] == '%' && i + 2 < value.length()) {
            int h1 = value[i + 1];
            int h2 = value[i + 2];
            auto fromHex = [](int h) -> int {
                if (h >= '0' && h <= '9') return h - '0';
                if (h >= 'a' && h <= 'f') return h - 'a' + 10;
                if (h >= 'A' && h <= 'F') return h - 'A' + 10;
                return 0;
            };
            result += static_cast<char>((fromHex(h1) << 4) | fromHex(h2));
            i += 2;
        } else if (value[i] == '+') {
            result += ' ';
        } else {
            result += value[i];
        }
    }
    return result;
}

} // namespace net
} // namespace sense

