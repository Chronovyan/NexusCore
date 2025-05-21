import os

# Read the original file
with open('src/OpenAI_API_Client.cpp.original', 'r', encoding='utf-8') as file:
    content = file.read()

# Remove the RetryStatistics method, keeping the RetryStatistics::Stats method
to_remove = """RetryStatistics OpenAI_API_Client::getRetryStatistics() const {
    return pImpl->getRetryStatistics();
}"""

fixed_content = content.replace(to_remove, "")

# Remove the duplicate getRetryStatistics in the implementation class
to_remove_impl = """    RetryStatistics getRetryStatistics() const {
        return retryStats_;
    }"""

fixed_content = fixed_content.replace(to_remove_impl, "")

# Write the fixed file with UTF-8 encoding
with open('src/OpenAI_API_Client.cpp', 'w', encoding='utf-8') as file:
    file.write(fixed_content)

print("File fixed successfully!") 