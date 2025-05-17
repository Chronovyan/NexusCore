#include <curl/curl.h>
#include <iostream>
#include <string>

static size_t WriteCallback(char* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append(contents, size * nmemb);
    return size * nmemb;
}

int main() {
    // Initialize curl
    CURL* curl = curl_easy_init();
    CURLcode res;
    std::string readBuffer;
    
    if(curl) {
        std::cout << "CURL initialized successfully!" << std::endl;
        
        // Make a simple GET request to a known endpoint
        curl_easy_setopt(curl, CURLOPT_URL, "https://httpbin.org/get");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        
        // Perform the request
        std::cout << "Performing request..." << std::endl;
        res = curl_easy_perform(curl);
        
        // Check for errors
        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        } else {
            std::cout << "Request successful!" << std::endl;
            std::cout << "Response (" << readBuffer.size() << " bytes):" << std::endl;
            
            // Print first 200 characters of response
            if(readBuffer.size() > 200) {
                std::cout << readBuffer.substr(0, 200) << "..." << std::endl;
            } else {
                std::cout << readBuffer << std::endl;
            }
        }
        
        // Clean up
        curl_easy_cleanup(curl);
    } else {
        std::cerr << "Failed to initialize CURL!" << std::endl;
        return 1;
    }
    
    return 0;
} 