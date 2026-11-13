#pragma once
#include <string>
// Blocking transport: call only on a worker thread. Native TLS validates host and chain.
std::string ShopRequest(const std::string& url, const std::string& token,
                        const std::string& body, const std::string& key);
