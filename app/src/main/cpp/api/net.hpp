#pragma once

#include "types.hpp"
#include <string>
#include <map>
#include <functional>

namespace sense {
namespace net {

// Synchronous HTTP GET request
HttpResponse get(
    const std::string& url,
    const std::map<std::string, std::string>& headers = {}
);

// Synchronous HTTP POST request
HttpResponse post(
    const std::string& url,
    const std::string& body,
    const std::string& contentType = "application/json",
    const std::map<std::string, std::string>& headers = {}
);

// Asynchronous HTTP GET request (runs on background thread)
void getAsync(
    const std::string& url,
    std::function<void(const HttpResponse&)> callback,
    const std::map<std::string, std::string>& headers = {}
);

// Asynchronous HTTP POST request (runs on background thread)
void postAsync(
    const std::string& url,
    const std::string& body,
    std::function<void(const HttpResponse&)> callback,
    const std::string& contentType = "application/json",
    const std::map<std::string, std::string>& headers = {}
);

// Downloads file from URL to disk
bool downloadFile(
    const std::string& url,
    const std::string& destinationPath,
    std::function<void(float progress)> progressCallback = nullptr
);

// Helper for sending Discord Webhooks
bool sendDiscordWebhook(
    const std::string& webhookUrl,
    const std::string& content,
    const std::string& username = "SENSE: The Game"
);

// URL String Utilities
std::string urlEncode(const std::string& value);
std::string urlDecode(const std::string& value);

} // namespace net
} // namespace sense

