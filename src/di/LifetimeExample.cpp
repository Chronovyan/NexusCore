#include <iostream>
#include <memory>
#include <string>\n#include <unordered_map>
#include "LifetimeManager.hpp"
#include "CoreModule.hpp"

using namespace di;
using namespace di::lifetime;

// Application interfaces
class IConfiguration {
public:
    virtual ~IConfiguration() = default;
    virtual std::string getDatabaseConnectionString() const = 0;
    virtual int getMaxConnections() const = 0;
    virtual bool isDebugMode() const = 0;
};

class IDatabase : public IDisposable {
public:
    virtual ~IDatabase() = default;
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual bool executeQuery(const std::string& query) = 0;
};

class IUserRepository {
public:
    virtual ~IUserRepository() = default;
    virtual bool authenticateUser(const std::string& username, const std::string& password) = 0;
    virtual std::string getUserInfo(const std::string& username) = 0;
};

class IAuthService {
public:
    virtual ~IAuthService() = default;
    virtual bool login(const std::string& username, const std::string& password) = 0;
    virtual void logout(const std::string& username) = 0;
    virtual bool isLoggedIn(const std::string& username) const = 0;
};

class IRequestHandler {
public:
    virtual ~IRequestHandler() = default;
    virtual std::string handleRequest(const std::string& request) = 0;
};

// Implementations
class AppConfiguration : public IConfiguration {
public:
    std::string getDatabaseConnectionString() const override {
        return "Server=localhost;Database=users;User=admin;Password=password123;";
    }
    
    int getMaxConnections() const override {
        return 100;
    }
    
    bool isDebugMode() const override {
        return true;
    }
};

class Database : public IDatabase {
public:
    Database(std::shared_ptr<IConfiguration> config, std::shared_ptr<ISimpleLogger> logger)
        : config_(config), logger_(logger), connected_(false) {
        logger_->log("Database instance created");
    }
    
    ~Database() {
        if (connected_) {
            disconnect();
        }
        logger_->log("Database instance destroyed");
    }
    
    bool connect() override {
        logger_->log("Connecting to database: " + config_->getDatabaseConnectionString());
        connected_ = true;
        return true;
    }
    
    void disconnect() override {
        if (connected_) {
            logger_->log("Disconnecting from database");
            connected_ = false;
        }
    }
    
    bool isConnected() const override {
        return connected_;
    }
    
    bool executeQuery(const std::string& query) override {
        if (!connected_) {
            logger_->logError("Cannot execute query, database not connected");
            return false;
        }
        
        logger_->log("Executing query: " + query);
        return true;
    }
    
    void dispose() override {
        logger_->log("Disposing database connection");
        disconnect();
    }
    
private:
    std::shared_ptr<IConfiguration> config_;
    std::shared_ptr<ISimpleLogger> logger_;
    bool connected_;
};

class UserRepository : public IUserRepository {
public:
    UserRepository(std::shared_ptr<IDatabase> database, std::shared_ptr<ISimpleLogger> logger)
        : database_(database), logger_(logger) {
        logger_->log("UserRepository instance created");
    }
    
    bool authenticateUser(const std::string& username, const std::string& password) override {
        logger_->log("Authenticating user: " + username);
        
        // In a real app, we would execute a query to check credentials
        std::string query = "SELECT * FROM users WHERE username='" + username + "' AND password='" + password + "'";
        return database_->executeQuery(query);
    }
    
    std::string getUserInfo(const std::string& username) override {
        logger_->log("Getting user info for: " + username);
        
        // In a real app, we would execute a query to get user info
        std::string query = "SELECT * FROM users WHERE username='" + username + "'";
        database_->executeQuery(query);
        
        // Return mock data
        return "User: " + username + ", Email: " + username + "@example.com";
    }
    
private:
    std::shared_ptr<IDatabase> database_;
    std::shared_ptr<ISimpleLogger> logger_;
};

class AuthService : public IAuthService {
public:
    AuthService(std::shared_ptr<IUserRepository> userRepository, std::shared_ptr<ISimpleLogger> logger)
        : userRepository_(userRepository), logger_(logger) {
        logger_->log("AuthService instance created");
    }
    
    bool login(const std::string& username, const std::string& password) override {
        logger_->log("Login attempt for user: " + username);
        
        bool authenticated = userRepository_->authenticateUser(username, password);
        if (authenticated) {
            loggedInUsers_[username] = true;
            logger_->log("Login successful for user: " + username);
        } else {
            logger_->logError("Login failed for user: " + username);
        }
        
        return authenticated;
    }
    
    void logout(const std::string& username) override {
        logger_->log("Logout for user: " + username);
        loggedInUsers_.erase(username);
    }
    
    bool isLoggedIn(const std::string& username) const override {
        return loggedInUsers_.find(username) != loggedInUsers_.end();
    }
    
private:
    std::shared_ptr<IUserRepository> userRepository_;
    std::shared_ptr<ISimpleLogger> logger_;
    std::unordered_map<std::string, bool> loggedInUsers_;
};

class RequestHandler : public IRequestHandler {
public:
    RequestHandler(std::shared_ptr<IAuthService> authService, std::shared_ptr<ISimpleLogger> logger)
        : authService_(authService), logger_(logger) {
        logger_->log("RequestHandler instance created");
    }
    
    std::string handleRequest(const std::string& request) override {
        logger_->log("Handling request: " + request);
        
        // Parse the request (simplified)
        if (request.find("login:") == 0) {
            // Format: login:username:password
            size_t pos1 = request.find(':', 6);
            if (pos1 != std::string::npos) {
                std::string username = request.substr(6, pos1 - 6);
                std::string password = request.substr(pos1 + 1);
                
                bool success = authService_->login(username, password);
                return success ? "Login successful" : "Login failed";
            }
        } else if (request.find("logout:") == 0) {
            // Format: logout:username
            std::string username = request.substr(7);
            authService_->logout(username);
            return "Logout successful";
        } else if (request.find("status:") == 0) {
            // Format: status:username
            std::string username = request.substr(7);
            bool loggedIn = authService_->isLoggedIn(username);
            return loggedIn ? "User is logged in" : "User is not logged in";
        }
        
        return "Unknown request";
    }
    
private:
    std::shared_ptr<IAuthService> authService_;
    std::shared_ptr<ISimpleLogger> logger_;
};

// Application context using LifetimeInjector
class ApplicationContext {
public:
    ApplicationContext() {
        // Configure services
        configureServices();
    }
    
    // Create a request scope
    std::shared_ptr<LifetimeInjector> createRequestScope() {
        return injector.createScope();
    }
    
    // Process a request
    std::string processRequest(const std::string& request) {
        // Create a scope for this request
        auto scope = createRequestScope();
        
        // Get the request handler from the scope
        auto requestHandler = scope->get<IRequestHandler>();
        
        // Handle the request
        std::string response = requestHandler->handleRequest(request);
        
        // Clean up the scope when done with the request
        scope->dispose();
        
        return response;
    }
    
    // Shutdown the application
    void shutdown() {
        injector.dispose();
    }
    
private:
    // Configure all services
    void configureServices() {
        // Register singleton services
        injector.registerFactory<ISimpleLogger>([]() {
            return std::make_shared<ConsoleLogger>();
        }, ServiceLifetime::Singleton);
        
        injector.registerFactory<IConfiguration>([]() {
            return std::make_shared<AppConfiguration>();
        }, ServiceLifetime::Singleton);
        
        // Register database with singleton lifetime
        injector.registerFactory<IDatabase>([](Injector& inj) {
            auto config = inj.resolve<IConfiguration>();
            auto logger = inj.resolve<ISimpleLogger>();
            auto db = std::make_shared<Database>(config, logger);
            db->connect(); // Connect at startup
            return db;
        }, ServiceLifetime::Singleton);
        
        // Register user repository with scoped lifetime
        injector.registerFactory<IUserRepository>([](Injector& inj) {
            auto db = inj.resolve<IDatabase>();
            auto logger = inj.resolve<ISimpleLogger>();
            return std::make_shared<UserRepository>(db, logger);
        }, ServiceLifetime::Scoped);
        
        // Register auth service with scoped lifetime
        injector.registerFactory<IAuthService>([](Injector& inj) {
            auto userRepo = inj.resolve<IUserRepository>();
            auto logger = inj.resolve<ISimpleLogger>();
            return std::make_shared<AuthService>(userRepo, logger);
        }, ServiceLifetime::Scoped);
        
        // Register request handler with transient lifetime
        injector.registerFactory<IRequestHandler>([](Injector& inj) {
            auto authService = inj.resolve<IAuthService>();
            auto logger = inj.resolve<ISimpleLogger>();
            return std::make_shared<RequestHandler>(authService, logger);
        }, ServiceLifetime::Transient);
    }
    
    // The lifetime injector for the application
    LifetimeInjector injector;
};

// Main function to demonstrate the example
int main() {
    std::cout << "Starting LifetimeManager example application..." << std::endl;
    
    // Create the application context
    ApplicationContext app;
    
    // Process some requests
    std::cout << "\n--- Processing requests ---" << std::endl;
    std::string response1 = app.processRequest("login:alice:password123");
    std::cout << "Response: " << response1 << std::endl;
    
    std::string response2 = app.processRequest("status:alice");
    std::cout << "Response: " << response2 << std::endl;
    
    std::string response3 = app.processRequest("login:bob:wrong_password");
    std::cout << "Response: " << response3 << std::endl;
    
    std::string response4 = app.processRequest("logout:alice");
    std::cout << "Response: " << response4 << std::endl;
    
    std::string response5 = app.processRequest("status:alice");
    std::cout << "Response: " << response5 << std::endl;
    
    // Shutdown the application
    std::cout << "\nShutting down application..." << std::endl;
    app.shutdown();
    
    std::cout << "Application terminated." << std::endl;
    
    return 0;
} 
