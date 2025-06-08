#pragma once

#include "DIFramework.hpp"
#include <memory>
#include <unordered_map>
#include <string>
#include <mutex>
#include <chrono>
#include <thread>
#include <atomic>
#include <iostream>
#include "AppDebugLog.h"

namespace di {

/**
 * @brief Manages request-scoped services in the DI framework.
 * 
 * This class allows the creation and management of request-scoped services,
 * ensuring that services are created once per request and disposed of at the
 * end of the request or after a timeout period.
 */
class RequestScopeManager {
private:
    struct ScopeInfo {
        std::shared_ptr<DIFramework> scope;
        std::chrono::steady_clock::time_point lastAccessTime;
    };

    std::shared_ptr<DIFramework> rootProvider;
    std::unordered_map<std::string, ScopeInfo> requestScopes;
    std::mutex scopesMutex;
    std::chrono::seconds inactiveScopeTimeout;
    std::thread cleanupThread;
    std::atomic<bool> shouldStopCleanup{false};

    /**
     * @brief Cleanup thread function that removes expired scopes.
     */
    void cleanupExpiredScopes() {
        while (!shouldStopCleanup) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            std::lock_guard<std::mutex> lock(scopesMutex);
            auto now = std::chrono::steady_clock::now();
            
            // Identify expired scopes
            std::vector<std::string> expiredScopeIds;
            for (const auto& [scopeId, scopeInfo] : requestScopes) {
                auto elapsed = now - scopeInfo.lastAccessTime;
                if (elapsed > inactiveScopeTimeout) {
                    expiredScopeIds.push_back(scopeId);
                }
            }
            
            // Remove expired scopes
            for (const auto& scopeId : expiredScopeIds) {
                LOG_DEBUG("Removing expired scope: " + scopeId);
                requestScopes.erase(scopeId);
            }
        }
    }

public:
    /**
     * @brief Constructs a RequestScopeManager.
     * @param rootProvider The root service provider to use for creating scoped providers.
     * @param inactiveScopeTimeout The timeout after which inactive scopes are removed (default: 60 seconds).
     */
    RequestScopeManager(std::shared_ptr<DIFramework> rootProvider,
                       std::chrono::seconds inactiveScopeTimeout = std::chrono::seconds(60))
        : rootProvider(rootProvider),
          inactiveScopeTimeout(inactiveScopeTimeout) {
        
        // Start the cleanup thread
        cleanupThread = std::thread(&RequestScopeManager::cleanupExpiredScopes, this);
    }
    
    /**
     * @brief Destructor. Joins the cleanup thread.
     */
    ~RequestScopeManager() {
        shouldStopCleanup = true;
        if (cleanupThread.joinable()) {
            cleanupThread.join();
        }
    }
    
    /**
     * @brief Gets or creates a scoped service provider for a request.
     * @param requestId The unique identifier for the request.
     * @return A shared pointer to the service provider for this request.
     */
    std::shared_ptr<DIFramework> getOrCreateScope(const std::string& requestId) {
        std::lock_guard<std::mutex> lock(scopesMutex);
        
        auto it = requestScopes.find(requestId);
        if (it != requestScopes.end()) {
            // Update last access time
            it->second.lastAccessTime = std::chrono::steady_clock::now();
            return it->second.scope;
        } else {
            // Create a new scope
            LOG_DEBUG("Creating new scope for request: " + requestId);
            auto scopeProvider = rootProvider->createScope();
            
            ScopeInfo scopeInfo{
                scopeProvider,
                std::chrono::steady_clock::now()
            };
            
            requestScopes.emplace(requestId, scopeInfo);
            return scopeProvider;
        }
    }
    
    /**
     * @brief Gets an existing scoped service provider for a request.
     * @param requestId The unique identifier for the request.
     * @return A shared pointer to the service provider for this request, or nullptr if not found.
     */
    std::shared_ptr<DIFramework> getScope(const std::string& requestId) {
        std::lock_guard<std::mutex> lock(scopesMutex);
        
        auto it = requestScopes.find(requestId);
        if (it != requestScopes.end()) {
            // Update last access time
            it->second.lastAccessTime = std::chrono::steady_clock::now();
            return it->second.scope;
        }
        
        return nullptr;
    }
    
    /**
     * @brief Removes a scoped service provider for a request.
     * @param requestId The unique identifier for the request.
     * @return True if the scope was removed, false if it didn't exist.
     */
    bool removeScope(const std::string& requestId) {
        std::lock_guard<std::mutex> lock(scopesMutex);
        
        auto it = requestScopes.find(requestId);
        if (it != requestScopes.end()) {
            LOG_DEBUG("Manually removing scope: " + requestId);
            requestScopes.erase(it);
            return true;
        }
        
        return false;
    }
};

/**
 * @brief Provides a convenient way to work with request-scoped services.
 * 
 * This class automatically manages the lifecycle of a request scope,
 * providing access to the services registered in that scope.
 */
class RequestContext {
private:
    RequestScopeManager& scopeManager;
    std::string requestId;
    std::shared_ptr<DIFramework> scopeProvider;
    
public:
    /**
     * @brief Constructs a RequestContext.
     * @param scopeManager The RequestScopeManager to use.
     * @param requestId The unique identifier for the request.
     */
    RequestContext(RequestScopeManager& scopeManager, const std::string& requestId)
        : scopeManager(scopeManager), requestId(requestId) {
        scopeProvider = scopeManager.getOrCreateScope(requestId);
    }
    
    /**
     * @brief Destructor. The scope is not automatically removed, but will be
     * cleaned up by the RequestScopeManager after the timeout period.
     */
    ~RequestContext() {
        // Scope will be cleaned up by the RequestScopeManager after timeout
    }
    
    /**
     * @brief Gets a service from the request scope.
     * @tparam T The type of service to get.
     * @param name Optional name of the service.
     * @return A shared pointer to the service.
     */
    template<typename T>
    std::shared_ptr<T> get(const std::string& name = "") {
        return scopeProvider->get<T>(name);
    }
    
    /**
     * @brief Gets the request ID.
     * @return The request ID.
     */
    const std::string& getRequestId() const {
        return requestId;
    }
};

} // namespace di 