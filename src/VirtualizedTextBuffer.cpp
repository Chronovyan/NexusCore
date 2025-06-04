#include "VirtualizedTextBuffer.h"
#include "LoggingCompatibility.h"
#include "EditorError.h"
#include "AppDebugLog.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <filesystem>
#include <cmath>

namespace fs = std::filesystem;

// Constructor
VirtualizedTextBuffer::VirtualizedTextBuffer()
    : isFromFile_(false)
    , pageSize_(1000)
    , cacheSize_(10)
    , totalLines_(0)
{
    // Initialize with one empty page containing one empty line
    auto page = std::make_shared<Page>();
    page->lines.resize(1);
    page->lines[0] = "";
    page->lastAccessed = std::chrono::steady_clock::now();
    
    pageCache_[0] = page;
    lruList_.push_back(0);
    totalLines_ = 1;
}

// Constructor with file
VirtualizedTextBuffer::VirtualizedTextBuffer(const std::string& filename, size_t pageSize, size_t cacheSize)
    : isFromFile_(true)
    , filename_(filename)
    , pageSize_(pageSize)
    , cacheSize_(cacheSize)
    , totalLines_(0)
{
    loadFromFile(filename);
}

// Destructor
VirtualizedTextBuffer::~VirtualizedTextBuffer()
{
    LOG_DEBUG("VirtualizedTextBuffer destroyed");
    
    // Save any dirty pages
    std::unique_lock<std::shared_mutex> lock(mutex_);
    for (const auto& pair : pageCache_) {
        if (pair.second->dirty) {
            savePage(pair.first, *pair.second);
        }
    }
    
    // Close the file stream
    if (fileStream_) {
        fileStream_->close();
    }
}

// Initialize from file
void VirtualizedTextBuffer::initFromFile(const std::string& filename)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    filename_ = filename;
    
    // Try to load the index file first
    if (loadIndexFile()) {
        // Index file loaded successfully, open the main file
        fileStream_ = std::make_shared<std::fstream>(filename_, std::ios::in | std::ios::out | std::ios::binary);
        if (!fileStream_->is_open()) {
            LOG_ERROR("Failed to open file: " + filename_);
            throw TextBufferException("Failed to open file: " + filename_, EditorException::Severity::EDITOR_ERROR);
        }
    } else {
        // No index file or invalid, rebuild the index by scanning the file
        fileStream_ = std::make_shared<std::fstream>(filename_, std::ios::in | std::ios::out | std::ios::binary);
        if (!fileStream_->is_open()) {
            LOG_ERROR("Failed to open file: " + filename_);
            throw TextBufferException("Failed to open file: " + filename_, EditorException::Severity::EDITOR_ERROR);
        }
        
        rebuildLineIndex();
        updateIndexFile();
    }
    
    LOG_DEBUG("Initialized VirtualizedTextBuffer with " + std::to_string(totalLines_) + " lines");
}

// Clear the buffer
void VirtualizedTextBuffer::clear(bool keepEmptyLine)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    pageCache_.clear();
    lruList_.clear();
    lineOffsets_.clear();
    totalLines_ = 0;
    
    if (keepEmptyLine) {
        // Add an empty line
        auto page = std::make_shared<Page>();
        page->lines.emplace_back("");
        page->lastAccessed = std::chrono::steady_clock::now();
        page->dirty = true;
        
        pageCache_[0] = page;
        lruList_.push_back(0);
        totalLines_ = 1;
        
        // Update line offsets if this is a file-backed buffer
        if (isFromFile_) {
            lineOffsets_.push_back(0);
            updateIndexFile();
        }
    }
    
    setModified(true);
}

// Load the index file
bool VirtualizedTextBuffer::loadIndexFile()
{
    // The index file has the same name as the main file with .idx extension
    std::string indexFilename = filename_ + ".idx";
    
    // Check if the index file exists
    if (!fs::exists(indexFilename)) {
        LOG_DEBUG("Index file does not exist: " + indexFilename);
        return false;
    }
    
    // Check if the index file is newer than the main file
    auto mainFileTime = fs::last_write_time(filename_);
    auto indexFileTime = fs::last_write_time(indexFilename);
    
    if (indexFileTime < mainFileTime) {
        LOG_DEBUG("Index file is older than the main file, will rebuild");
        return false;
    }
    
    // Open the index file
    std::ifstream indexFile(indexFilename, std::ios::binary);
    if (!indexFile.is_open()) {
        LOG_ERROR("Failed to open index file: " + indexFilename);
        return false;
    }
    
    // Read the number of lines
    indexFile.read(reinterpret_cast<char*>(&totalLines_), sizeof(totalLines_));
    
    // Read the line offsets
    lineOffsets_.resize(totalLines_);
    for (size_t i = 0; i < totalLines_; ++i) {
        std::streampos offset;
        indexFile.read(reinterpret_cast<char*>(&offset), sizeof(offset));
        lineOffsets_[i] = offset;
    }
    
    indexFile.close();
    
    LOG_DEBUG("Loaded index file with " + std::to_string(totalLines_) + " lines");
    return true;
}

// Update the index file
void VirtualizedTextBuffer::updateIndexFile() const
{
    if (!isFromFile_ || filename_.empty()) {
        return;
    }
    
    // The index file has the same name as the main file with .idx extension
    std::string indexFilename = filename_ + ".idx";
    
    // Open the index file
    std::ofstream indexFile(indexFilename, std::ios::binary);
    if (!indexFile.is_open()) {
        LOG_ERROR("Failed to create index file: " + indexFilename);
        return;
    }
    
    // Write the number of lines
    indexFile.write(reinterpret_cast<const char*>(&totalLines_), sizeof(totalLines_));
    
    // Write the line offsets
    for (const auto& offset : lineOffsets_) {
        indexFile.write(reinterpret_cast<const char*>(&offset), sizeof(offset));
    }
    
    indexFile.close();
    
    LOG_DEBUG("Updated index file with " + std::to_string(totalLines_) + " lines");
}

// Rebuild the line index by scanning the file
void VirtualizedTextBuffer::rebuildLineIndex()
{
    if (!fileStream_ || !fileStream_->is_open()) {
        LOG_ERROR("File stream is not open");
        return;
    }
    
    LOG_DEBUG("Rebuilding line index for file: " + filename_);
    
    // Clear existing index
    lineOffsets_.clear();
    totalLines_ = 0;
    
    // Reset file position
    fileStream_->seekg(0, std::ios::beg);
    
    // Scan the file and build the line index
    std::string line;
    std::streampos pos = 0;
    
    while (std::getline(*fileStream_, line)) {
        lineOffsets_.push_back(pos);
        totalLines_++;
        
        // Calculate the position of the next line
        // +1 for the newline character that was consumed by getline
        pos = fileStream_->tellg();
        
        // Show progress for very large files
        if (totalLines_ % 100000 == 0) {
            LOG_DEBUG("Indexed " + std::to_string(totalLines_) + " lines so far");
        }
    }
    
    // Reset file position
    fileStream_->clear(); // Clear EOF flag
    fileStream_->seekg(0, std::ios::beg);
    
    LOG_DEBUG("Rebuilt line index with " + std::to_string(totalLines_) + " lines");
}

// Get the page number for a line index
size_t VirtualizedTextBuffer::getPageNumber(size_t lineIndex) const
{
    return lineIndex / pageSize_;
}

// Get the line index within a page
size_t VirtualizedTextBuffer::getLineIndexInPage(size_t lineIndex) const
{
    return lineIndex % pageSize_;
}

// Get a file offset for a line
std::streampos VirtualizedTextBuffer::getFileOffsetForLine(size_t lineIndex) const
{
    if (lineIndex >= lineOffsets_.size()) {
        LOG_ERROR("Line index out of range: " + std::to_string(lineIndex));
        throw TextBufferException("Line index out of range", EditorException::Severity::EDITOR_ERROR);
    }
    
    return lineOffsets_[lineIndex];
}

// Load a page from disk
std::shared_ptr<VirtualizedTextBuffer::Page> VirtualizedTextBuffer::loadPage(size_t pageNumber) const
{
    if (!isFromFile_ || !fileStream_ || !fileStream_->is_open()) {
        LOG_ERROR("Cannot load page: file stream is not open");
        throw TextBufferException("Cannot load page: file stream is not open", EditorException::Severity::EDITOR_ERROR);
    }
    
    LOG_DEBUG("Loading page " + std::to_string(pageNumber) + " from disk");
    
    auto page = std::make_shared<Page>();
    page->lastAccessed = std::chrono::steady_clock::now();
    page->dirty = false;
    
    // Calculate the range of lines in this page
    size_t startLine = pageNumber * pageSize_;
    size_t endLine = std::min(startLine + pageSize_, totalLines_);
    
    // Allocate space for the lines
    page->lines.resize(endLine - startLine);
    
    // Load each line in the page
    for (size_t i = startLine; i < endLine; ++i) {
        // Seek to the line offset
        fileStream_->seekg(getFileOffsetForLine(i));
        
        // Read the line
        std::getline(*fileStream_, page->lines[i - startLine]);
    }
    
    cacheMisses_++;
    return page;
}

// Get a page from cache or load it from disk
std::shared_ptr<VirtualizedTextBuffer::Page> VirtualizedTextBuffer::getPage(size_t pageNumber) const
{
    // Check if the page is in cache
    auto it = pageCache_.find(pageNumber);
    if (it != pageCache_.end()) {
        // Update the page's access time and position in cache
        updatePageAccess(pageNumber);
        
        // Track access pattern for prefetching
        updateAccessPattern(pageNumber);
        
        // Count as a cache hit
        cacheHits_++;
        
        LOG_DEBUG("Cache hit for page " + std::to_string(pageNumber));
        
        return it->second;
    }
    
    // Count as a cache miss
    cacheMisses_++;
    
    LOG_DEBUG("Cache miss for page " + std::to_string(pageNumber));
    
    // Page is not in cache, load it from disk
    auto page = loadPage(pageNumber);
    
    // Add the page to cache
    pageCache_[pageNumber] = page;
    
    // Add to appropriate data structure based on eviction policy
    switch (evictionPolicy_) {
        case CacheEvictionPolicy::LRU:
            lruList_.push_back(pageNumber);
            break;
            
        case CacheEvictionPolicy::SLRU:
            probationarySegment_.push_back(pageNumber);
            break;
            
        case CacheEvictionPolicy::ARC:
            // Check if this page was in the ghost caches
            if (ghostRecent_.find(pageNumber) != ghostRecent_.end()) {
                // Increase p
                arcP_ = std::min(arcP_ + 1.0, static_cast<double>(cacheSize_));
                ghostRecent_.erase(pageNumber);
                frequentlyUsed_.insert(pageNumber);
            } else if (ghostFrequent_.find(pageNumber) != ghostFrequent_.end()) {
                // Decrease p
                arcP_ = std::max(arcP_ - 1.0, 0.0);
                ghostFrequent_.erase(pageNumber);
                frequentlyUsed_.insert(pageNumber);
            } else {
                recentlyUsed_.insert(pageNumber);
            }
            break;
            
        case CacheEvictionPolicy::SPATIAL:
            lruList_.push_back(pageNumber);
            // New pages get a medium priority
            spatialScores_[pageNumber] = 0.5;
            break;
    }
    
    // Track access pattern for prefetching
    updateAccessPattern(pageNumber);
    
    // Evict pages if cache is full
    while (pageCache_.size() > cacheSize_) {
        evictPage();
    }
    
    // Initiate prefetching if appropriate
    if (prefetchStrategy_ != PrefetchStrategy::NONE) {
        initiateStrategicPrefetch(pageNumber);
    }
    
    return page;
}

// Ensure a page is loaded and return a reference to it
std::shared_ptr<VirtualizedTextBuffer::Page> VirtualizedTextBuffer::ensurePage(size_t pageNumber, bool forWriting)
{
    auto page = getPage(pageNumber);
    
    if (forWriting) {
        markPageDirty(pageNumber);
    }
    
    return page;
}

// Mark a page as dirty
void VirtualizedTextBuffer::markPageDirty(size_t pageNumber)
{
    auto it = pageCache_.find(pageNumber);
    if (it != pageCache_.end()) {
        it->second->dirty = true;
        setModified(true);
    }
}

// Save a dirty page to disk
void VirtualizedTextBuffer::savePage(size_t pageNumber, const Page& page) const
{
    if (!isFromFile_ || !fileStream_ || !fileStream_->is_open()) {
        LOG_ERROR("Cannot save page: file stream is not open");
        return;
    }
    
    if (!page.dirty) {
        return; // Nothing to save
    }
    
    LOG_DEBUG("Saving page " + std::to_string(pageNumber) + " to disk");
    
    // Calculate the range of lines in this page
    size_t startLine = pageNumber * pageSize_;
    size_t endLine = std::min(startLine + pageSize_, totalLines_);
    
    // Save each line in the page
    for (size_t i = startLine; i < endLine; ++i) {
        // Seek to the line offset
        fileStream_->seekg(getFileOffsetForLine(i));
        
        // Write the line
        *fileStream_ << page.lines[i - startLine];
        
        // Write a newline character if this is not the last line
        if (i < totalLines_ - 1) {
            *fileStream_ << '\n';
        }
    }
    
    // Flush the file
    fileStream_->flush();
}

// Evict the least recently used page from cache
void VirtualizedTextBuffer::evictLRUPage() const
{
    if (lruList_.empty()) {
        return;
    }
    
    // Get the least recently used page
    size_t pageNumber = lruList_.front();
    
    // Remove the page from the LRU list
    lruList_.erase(lruList_.begin());
    
    // Save the page if it's dirty
    auto it = pageCache_.find(pageNumber);
    if (it != pageCache_.end() && it->second->dirty) {
        savePage(pageNumber, *it->second);
    }
    
    // Remove the page from cache
    pageCache_.erase(pageNumber);
    
    LOG_DEBUG("Evicted page " + std::to_string(pageNumber) + " from cache");
}

// Update the access time for a page
void VirtualizedTextBuffer::updatePageAccess(size_t pageNumber) const
{
    // Update the page's access time
    auto it = pageCache_.find(pageNumber);
    if (it != pageCache_.end()) {
        it->second->lastAccessed = std::chrono::steady_clock::now();
        it->second->accessCount++;
    } else {
        return; // Page not in cache
    }
    
    // Update position in appropriate data structure based on eviction policy
    switch (evictionPolicy_) {
        case CacheEvictionPolicy::LRU:
            // Move the page to the end of the LRU list
            {
                auto lruIt = std::find(lruList_.begin(), lruList_.end(), pageNumber);
                if (lruIt != lruList_.end()) {
                    lruList_.erase(lruIt);
                    lruList_.push_back(pageNumber);
                }
            }
            break;
            
        case CacheEvictionPolicy::SLRU:
            // If in probationary segment, move to protected segment
            {
                auto probIt = std::find(probationarySegment_.begin(), probationarySegment_.end(), pageNumber);
                if (probIt != probationarySegment_.end()) {
                    probationarySegment_.erase(probIt);
                    protectedSegment_.push_back(pageNumber);
                } else {
                    // If already in protected segment, move to the end
                    auto protIt = std::find(protectedSegment_.begin(), protectedSegment_.end(), pageNumber);
                    if (protIt != protectedSegment_.end()) {
                        protectedSegment_.erase(protIt);
                        protectedSegment_.push_back(pageNumber);
                    }
                }
            }
            break;
            
        case CacheEvictionPolicy::ARC:
            // If in recently used, move to frequently used
            if (recentlyUsed_.find(pageNumber) != recentlyUsed_.end()) {
                recentlyUsed_.erase(pageNumber);
                frequentlyUsed_.insert(pageNumber);
            } 
            // If already in frequently used, it stays there
            break;
            
        case CacheEvictionPolicy::SPATIAL:
            // Move the page to the end of the LRU list
            {
                auto lruIt = std::find(lruList_.begin(), lruList_.end(), pageNumber);
                if (lruIt != lruList_.end()) {
                    lruList_.erase(lruIt);
                    lruList_.push_back(pageNumber);
                }
            }
            
            // Increase spatial score for this page and neighbors
            {
                double currentScore = spatialScores_[pageNumber];
                spatialScores_[pageNumber] = std::min(currentScore + 0.2, 1.0);
                
                // Boost adjacent pages' scores
                for (int offset = -2; offset <= 2; ++offset) {
                    if (offset == 0) continue; // Skip current page
                    
                    int neighbor = static_cast<int>(pageNumber) + offset;
                    if (neighbor >= 0) {
                        double neighborScore = spatialScores_[neighbor];
                        double boost = 0.1 / (1.0 + std::abs(offset));
                        spatialScores_[neighbor] = std::min(neighborScore + boost, 1.0);
                    }
                }
                
                // Decay other pages' scores
                for (auto& score : spatialScores_) {
                    if (score.first != pageNumber && 
                        std::abs(static_cast<int>(score.first) - static_cast<int>(pageNumber)) > 2) {
                        score.second *= 0.99; // Gentle decay
                    }
                }
            }
            break;
    }
}

// Get the number of pages in memory
size_t VirtualizedTextBuffer::getPagesInMemory() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return pageCache_.size();
}

// Get the cache hit rate
double VirtualizedTextBuffer::getCacheHitRate() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    size_t totalAccesses = cacheHits_ + cacheMisses_;
    if (totalAccesses == 0) {
        return 0.0;
    }
    
    return (static_cast<double>(cacheHits_) / totalAccesses) * 100.0;
}

// Reset the cache statistics
void VirtualizedTextBuffer::resetCacheStats()
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    cacheHits_ = 0;
    cacheMisses_ = 0;
}

// Set the page size
void VirtualizedTextBuffer::setPageSize(size_t pageSize)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (pageSize == 0) {
        LOG_ERROR("Page size cannot be zero");
        throw TextBufferException("Page size cannot be zero", EditorException::Severity::EDITOR_ERROR);
    }
    
    if (pageSize == pageSize_) {
        return; // Nothing to change
    }
    
    LOG_DEBUG("Changing page size from " + std::to_string(pageSize_) + " to " + std::to_string(pageSize));
    
    // Save all dirty pages before changing the page size
    for (const auto& pair : pageCache_) {
        if (pair.second->dirty) {
            savePage(pair.first, *pair.second);
        }
    }
    
    // Clear the cache
    pageCache_.clear();
    lruList_.clear();
    
    // Update the page size
    pageSize_ = pageSize;
}

// Set the cache size
void VirtualizedTextBuffer::setCacheSize(size_t cacheSize)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (cacheSize == 0) {
        LOG_ERROR("Cache size cannot be zero");
        throw TextBufferException("Cache size cannot be zero", EditorException::Severity::EDITOR_ERROR);
    }
    
    if (cacheSize == cacheSize_) {
        return; // Nothing to change
    }
    
    LOG_DEBUG("Changing cache size from " + std::to_string(cacheSize_) + " to " + std::to_string(cacheSize));
    
    // Update the cache size
    cacheSize_ = cacheSize;
    
    // Evict pages if cache is too large
    while (pageCache_.size() > cacheSize_) {
        evictLRUPage();
    }
}

// Get the current page size
size_t VirtualizedTextBuffer::getPageSize() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return pageSize_;
}

// Get the current cache size
size_t VirtualizedTextBuffer::getCacheSize() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return cacheSize_;
}

// Lock for reading
void VirtualizedTextBuffer::lockForReading() const
{
    mutex_.lock_shared();
}

// Unlock after reading
void VirtualizedTextBuffer::unlockReading() const
{
    mutex_.unlock_shared();
}

// Lock for writing
void VirtualizedTextBuffer::lockForWriting()
{
    mutex_.lock();
}

// Unlock after writing
void VirtualizedTextBuffer::unlockWriting()
{
    mutex_.unlock();
}

// Check if memory usage is high
bool VirtualizedTextBuffer::isMemoryUsageHigh() const
{
    // This is a simplified check - in a real implementation,
    // you might want to use platform-specific APIs to check
    // actual memory usage.
    return pageCache_.size() >= cacheSize_;
}

// Prefetch a range of lines
void VirtualizedTextBuffer::prefetchLines(size_t startLine, size_t endLine)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    // Clamp to valid range
    startLine = std::min(startLine, totalLines_ > 0 ? totalLines_ - 1 : 0);
    endLine = std::min(endLine, totalLines_ > 0 ? totalLines_ - 1 : 0);
    
    if (startLine > endLine) {
        return;
    }
    
    LOG_DEBUG("Prefetching lines " + std::to_string(startLine) + " to " + std::to_string(endLine));
    
    // Calculate the range of pages to prefetch
    size_t startPage = getPageNumber(startLine);
    size_t endPage = getPageNumber(endLine);
    
    // Special case: If we're prefetching many pages, temporarily boost prefetch distance
    size_t originalPrefetchDistance = prefetchDistance_;
    if (endPage - startPage > prefetchDistance_ * 2) {
        // Temporarily expand prefetch distance to cover the entire range
        prefetchDistance_ = (endPage - startPage) / 2 + 1;
    }
    
    // Use the strategic prefetching for the first page
    if (prefetchStrategy_ != PrefetchStrategy::NONE) {
        // Mark the middle page as "accessed" to trigger prefetching
        size_t triggerPage = (startPage + endPage) / 2;
        
        // Add to access pattern
        recentAccesses_.push_back(triggerPage);
        if (recentAccesses_.size() > recentAccessesMaxSize_) {
            recentAccesses_.pop_front();
        }
        
        // Initiate strategic prefetching
        initiateStrategicPrefetch(triggerPage);
    } else {
        // Legacy fallback: Prefetch each page in the range
        for (size_t pageNumber = startPage; pageNumber <= endPage; ++pageNumber) {
            // Skip if the page is already in cache
            if (pageCache_.find(pageNumber) != pageCache_.end()) {
                continue;
            }
            
            // Check if we need to evict pages
            if (pageCache_.size() >= cacheSize_) {
                evictPage();
            }
            
            // Load the page
            auto page = loadPage(pageNumber);
            
            // Add the page to cache
            pageCache_[pageNumber] = page;
            lruList_.push_back(pageNumber);
        }
    }
    
    // Restore original prefetch distance
    prefetchDistance_ = originalPrefetchDistance;
}

// Load a line to the temporary buffer
void VirtualizedTextBuffer::loadLineToTemporary(size_t lineIndex) const
{
    if (lineIndex >= totalLines_) {
        LOG_ERROR("Line index out of range: " + std::to_string(lineIndex));
        throw TextBufferException("Line index out of range", EditorException::Severity::EDITOR_ERROR);
    }
    
    size_t pageNumber = getPageNumber(lineIndex);
    size_t lineIndexInPage = getLineIndexInPage(lineIndex);
    
    auto page = getPage(pageNumber);
    temporaryLine_ = page->lines[lineIndexInPage];
}

// ITextBuffer interface implementation

// Get a line (const version)
const std::string& VirtualizedTextBuffer::getLine(size_t index) const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    if (index >= totalLines_) {
        LOG_ERROR("Line index out of range: " + std::to_string(index));
        throw TextBufferException("Line index out of range", EditorException::Severity::EDITOR_ERROR);
    }
    
    size_t pageNumber = getPageNumber(index);
    size_t lineIndexInPage = getLineIndexInPage(index);
    
    auto page = getPage(pageNumber);
    return page->lines[lineIndexInPage];
}

// Get a line (non-const version)
std::string& VirtualizedTextBuffer::getLine(size_t index)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (index >= totalLines_) {
        LOG_ERROR("Line index out of range: " + std::to_string(index));
        throw TextBufferException("Line index out of range", EditorException::Severity::EDITOR_ERROR);
    }
    
    size_t pageNumber = getPageNumber(index);
    size_t lineIndexInPage = getLineIndexInPage(index);
    
    auto page = ensurePage(pageNumber, true);
    return page->lines[lineIndexInPage];
}

// Get the number of lines
size_t VirtualizedTextBuffer::lineCount() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return totalLines_;
}

// Check if the buffer is empty
bool VirtualizedTextBuffer::isEmpty() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return totalLines_ == 0;
}

// Get the length of a line
size_t VirtualizedTextBuffer::lineLength(size_t lineIndex) const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    if (lineIndex >= totalLines_) {
        LOG_ERROR("Line index out of range: " + std::to_string(lineIndex));
        throw TextBufferException("Line index out of range", EditorException::Severity::EDITOR_ERROR);
    }
    
    size_t pageNumber = getPageNumber(lineIndex);
    size_t lineIndexInPage = getLineIndexInPage(lineIndex);
    
    auto page = getPage(pageNumber);
    return page->lines[lineIndexInPage].length();
}

// Add a line to the end of the buffer
void VirtualizedTextBuffer::addLine(const std::string& line)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    size_t pageNumber = getPageNumber(totalLines_);
    size_t lineIndexInPage = getLineIndexInPage(totalLines_);
    
    auto page = ensurePage(pageNumber, true);
    
    // If this is a new page or the page is not full, add the line to the page
    if (lineIndexInPage < pageSize_) {
        if (lineIndexInPage >= page->lines.size()) {
            page->lines.resize(lineIndexInPage + 1);
        }
        page->lines[lineIndexInPage] = line;
    } else {
        // Create a new page for this line
        auto newPage = std::make_shared<Page>();
        newPage->lines.push_back(line);
        newPage->lastAccessed = std::chrono::steady_clock::now();
        newPage->dirty = true;
        
        pageCache_[pageNumber] = newPage;
        lruList_.push_back(pageNumber);
        
        // Evict pages if cache is full
        while (pageCache_.size() > cacheSize_) {
            evictLRUPage();
        }
    }
    
    // Update the total line count
    totalLines_++;
    
    // Update line offsets if this is a file-backed buffer
    if (isFromFile_) {
        // For simplicity, we'll rebuild the line index after adding a line
        // In a real implementation, you might want to update the index incrementally
        rebuildLineIndex();
        updateIndexFile();
    }
    
    setModified(true);
}

// Insert a line at the specified index
void VirtualizedTextBuffer::insertLine(size_t index, const std::string& line)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (index > totalLines_) {
        LOG_ERROR("Line index out of range for insertLine: " + std::to_string(index));
        throw TextBufferException("Line index out of range for insertLine", EditorException::Severity::EDITOR_ERROR);
    }
    
    // Special case: inserting at the end is the same as adding a line
    if (index == totalLines_) {
        lock.unlock();
        addLine(line);
        return;
    }
    
    // We need to shift all lines after the insertion point
    // For simplicity, we'll rebuild the buffer after inserting a line
    // In a real implementation, you might want to update the buffer incrementally
    
    // Load all lines
    std::vector<std::string> allLines;
    allLines.reserve(totalLines_ + 1);
    
    for (size_t i = 0; i < totalLines_; ++i) {
        if (i == index) {
            allLines.push_back(line);
        }
        
        size_t pageNumber = getPageNumber(i);
        size_t lineIndexInPage = getLineIndexInPage(i);
        
        auto page = getPage(pageNumber);
        allLines.push_back(page->lines[lineIndexInPage]);
    }
    
    // Insert the line at the end if needed
    if (index == totalLines_) {
        allLines.push_back(line);
    }
    
    // Update the total line count
    totalLines_++;
    
    // Clear the cache
    pageCache_.clear();
    lruList_.clear();
    
    // Reload the buffer with the new lines
    for (size_t i = 0; i < allLines.size(); ++i) {
        size_t pageNumber = getPageNumber(i);
        size_t lineIndexInPage = getLineIndexInPage(i);
        
        auto it = pageCache_.find(pageNumber);
        if (it == pageCache_.end()) {
            auto page = std::make_shared<Page>();
            page->lines.resize(std::min(pageSize_, allLines.size() - pageNumber * pageSize_));
            page->lastAccessed = std::chrono::steady_clock::now();
            page->dirty = true;
            
            pageCache_[pageNumber] = page;
            lruList_.push_back(pageNumber);
        }
        
        auto& page = pageCache_[pageNumber];
        if (lineIndexInPage >= page->lines.size()) {
            page->lines.resize(lineIndexInPage + 1);
        }
        page->lines[lineIndexInPage] = allLines[i];
    }
    
    // Update line offsets if this is a file-backed buffer
    if (isFromFile_) {
        rebuildLineIndex();
        updateIndexFile();
    }
    
    setModified(true);
}

// Delete a line at the specified index
void VirtualizedTextBuffer::deleteLine(size_t index)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (index >= totalLines_) {
        LOG_ERROR("Line index out of range for deleteLine: " + std::to_string(index));
        throw TextBufferException("Line index out of range for deleteLine", EditorException::Severity::EDITOR_ERROR);
    }
    
    // Special case: if there's only one line, just clear it
    if (totalLines_ == 1) {
        auto page = ensurePage(0, true);
        page->lines[0] = "";
        return;
    }
    
    // We need to shift all lines after the deletion point
    // For simplicity, we'll rebuild the buffer after deleting a line
    // In a real implementation, you might want to update the buffer incrementally
    
    // Load all lines except the one to delete
    std::vector<std::string> allLines;
    allLines.reserve(totalLines_ - 1);
    
    for (size_t i = 0; i < totalLines_; ++i) {
        if (i == index) {
            continue;
        }
        
        size_t pageNumber = getPageNumber(i);
        size_t lineIndexInPage = getLineIndexInPage(i);
        
        auto page = getPage(pageNumber);
        allLines.push_back(page->lines[lineIndexInPage]);
    }
    
    // Update the total line count
    totalLines_--;
    
    // Clear the cache
    pageCache_.clear();
    lruList_.clear();
    
    // Reload the buffer with the new lines
    for (size_t i = 0; i < allLines.size(); ++i) {
        size_t pageNumber = getPageNumber(i);
        size_t lineIndexInPage = getLineIndexInPage(i);
        
        auto it = pageCache_.find(pageNumber);
        if (it == pageCache_.end()) {
            auto page = std::make_shared<Page>();
            page->lines.resize(std::min(pageSize_, allLines.size() - pageNumber * pageSize_));
            page->lastAccessed = std::chrono::steady_clock::now();
            page->dirty = true;
            
            pageCache_[pageNumber] = page;
            lruList_.push_back(pageNumber);
        }
        
        auto& page = pageCache_[pageNumber];
        if (lineIndexInPage >= page->lines.size()) {
            page->lines.resize(lineIndexInPage + 1);
        }
        page->lines[lineIndexInPage] = allLines[i];
    }
    
    // Update line offsets if this is a file-backed buffer
    if (isFromFile_) {
        rebuildLineIndex();
        updateIndexFile();
    }
    
    setModified(true);
}

// Replace a line at the specified index
void VirtualizedTextBuffer::replaceLine(size_t index, const std::string& newLine)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (index >= totalLines_) {
        LOG_ERROR("Line index out of range for replaceLine: " + std::to_string(index));
        throw TextBufferException("Line index out of range for replaceLine", EditorException::Severity::EDITOR_ERROR);
    }
    
    size_t pageNumber = getPageNumber(index);
    size_t lineIndexInPage = getLineIndexInPage(index);
    
    auto page = ensurePage(pageNumber, true);
    page->lines[lineIndexInPage] = newLine;
    
    setModified(true);
}

// Set the content of a line
void VirtualizedTextBuffer::setLine(size_t lineIndex, const std::string& text)
{
    // This is the same as replaceLine
    replaceLine(lineIndex, text);
}

// Delete multiple lines
void VirtualizedTextBuffer::deleteLines(size_t startIndex, size_t endIndex)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (startIndex >= totalLines_ || startIndex >= endIndex) {
        LOG_ERROR("Invalid range for deleteLines: " + std::to_string(startIndex) + " to " + std::to_string(endIndex));
        throw TextBufferException("Invalid range for deleteLines", EditorException::Severity::EDITOR_ERROR);
    }
    
    // Clamp endIndex to the number of lines
    endIndex = std::min(endIndex, totalLines_);
    
    // Special case: if deleting all lines, just clear the buffer
    if (startIndex == 0 && endIndex == totalLines_) {
        lock.unlock();
        clear(true);
        return;
    }
    
    // Load all lines except the ones to delete
    std::vector<std::string> allLines;
    allLines.reserve(totalLines_ - (endIndex - startIndex));
    
    for (size_t i = 0; i < totalLines_; ++i) {
        if (i >= startIndex && i < endIndex) {
            continue;
        }
        
        size_t pageNumber = getPageNumber(i);
        size_t lineIndexInPage = getLineIndexInPage(i);
        
        auto page = getPage(pageNumber);
        allLines.push_back(page->lines[lineIndexInPage]);
    }
    
    // Update the total line count
    totalLines_ = allLines.size();
    
    // Clear the cache
    pageCache_.clear();
    lruList_.clear();
    
    // Reload the buffer with the new lines
    for (size_t i = 0; i < allLines.size(); ++i) {
        size_t pageNumber = getPageNumber(i);
        size_t lineIndexInPage = getLineIndexInPage(i);
        
        auto it = pageCache_.find(pageNumber);
        if (it == pageCache_.end()) {
            auto page = std::make_shared<Page>();
            page->lines.resize(std::min(pageSize_, allLines.size() - pageNumber * pageSize_));
            page->lastAccessed = std::chrono::steady_clock::now();
            page->dirty = true;
            
            pageCache_[pageNumber] = page;
            lruList_.push_back(pageNumber);
        }
        
        auto& page = pageCache_[pageNumber];
        if (lineIndexInPage >= page->lines.size()) {
            page->lines.resize(lineIndexInPage + 1);
        }
        page->lines[lineIndexInPage] = allLines[i];
    }
    
    // Update line offsets if this is a file-backed buffer
    if (isFromFile_) {
        rebuildLineIndex();
        updateIndexFile();
    }
    
    setModified(true);
}

// Insert multiple lines
void VirtualizedTextBuffer::insertLines(size_t index, const std::vector<std::string>& newLines)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (index > totalLines_) {
        LOG_ERROR("Line index out of range for insertLines: " + std::to_string(index));
        throw TextBufferException("Line index out of range for insertLines", EditorException::Severity::EDITOR_ERROR);
    }
    
    if (newLines.empty()) {
        return; // Nothing to do
    }
    
    // Load all lines
    std::vector<std::string> allLines;
    allLines.reserve(totalLines_ + newLines.size());
    
    for (size_t i = 0; i < index; ++i) {
        size_t pageNumber = getPageNumber(i);
        size_t lineIndexInPage = getLineIndexInPage(i);
        
        auto page = getPage(pageNumber);
        allLines.push_back(page->lines[lineIndexInPage]);
    }
    
    // Insert the new lines
    allLines.insert(allLines.end(), newLines.begin(), newLines.end());
    
    // Add the remaining lines
    for (size_t i = index; i < totalLines_; ++i) {
        size_t pageNumber = getPageNumber(i);
        size_t lineIndexInPage = getLineIndexInPage(i);
        
        auto page = getPage(pageNumber);
        allLines.push_back(page->lines[lineIndexInPage]);
    }
    
    // Update the total line count
    totalLines_ = allLines.size();
    
    // Clear the cache
    pageCache_.clear();
    lruList_.clear();
    
    // Reload the buffer with the new lines
    for (size_t i = 0; i < allLines.size(); ++i) {
        size_t pageNumber = getPageNumber(i);
        size_t lineIndexInPage = getLineIndexInPage(i);
        
        auto it = pageCache_.find(pageNumber);
        if (it == pageCache_.end()) {
            auto page = std::make_shared<Page>();
            page->lines.resize(std::min(pageSize_, allLines.size() - pageNumber * pageSize_));
            page->lastAccessed = std::chrono::steady_clock::now();
            page->dirty = true;
            
            pageCache_[pageNumber] = page;
            lruList_.push_back(pageNumber);
        }
        
        auto& page = pageCache_[pageNumber];
        if (lineIndexInPage >= page->lines.size()) {
            page->lines.resize(lineIndexInPage + 1);
        }
        page->lines[lineIndexInPage] = allLines[i];
    }
    
    // Update line offsets if this is a file-backed buffer
    if (isFromFile_) {
        rebuildLineIndex();
        updateIndexFile();
    }
    
    setModified(true);
}

// Get the total number of characters in the buffer
size_t VirtualizedTextBuffer::characterCount() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    size_t count = 0;
    
    // We'll need to load all pages to count characters
    // This could be optimized by keeping track of character counts per page
    for (size_t i = 0; i < totalLines_; ++i) {
        size_t pageNumber = getPageNumber(i);
        size_t lineIndexInPage = getLineIndexInPage(i);
        
        auto page = getPage(pageNumber);
        count += page->lines[lineIndexInPage].length();
    }
    
    return count;
}

// Get all lines in the buffer
std::vector<std::string> VirtualizedTextBuffer::getAllLines() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    // Special case: if there's only one line and it's empty, return an empty vector
    if (totalLines_ == 1) {
        size_t pageNumber = getPageNumber(0);
        size_t lineIndexInPage = getLineIndexInPage(0);
        
        auto page = getPage(pageNumber);
        if (page->lines[lineIndexInPage].empty()) {
            return std::vector<std::string>();
        }
    }
    
    // Otherwise, return all lines
    std::vector<std::string> allLines;
    allLines.reserve(totalLines_);
    
    for (size_t i = 0; i < totalLines_; ++i) {
        size_t pageNumber = getPageNumber(i);
        size_t lineIndexInPage = getLineIndexInPage(i);
        
        auto page = getPage(pageNumber);
        allLines.push_back(page->lines[lineIndexInPage]);
    }
    
    return allLines;
}

// Check if a position is valid
bool VirtualizedTextBuffer::isValidPosition(size_t lineIndex, size_t colIndex) const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    // Empty buffer has no valid positions
    if (totalLines_ == 0) {
        return false;
    }
    
    // Check line index
    if (lineIndex >= totalLines_) {
        return false;
    }
    
    // Check column index (can be at the end of the line, hence <=)
    size_t pageNumber = getPageNumber(lineIndex);
    size_t lineIndexInPage = getLineIndexInPage(lineIndex);
    
    auto page = getPage(pageNumber);
    return colIndex <= page->lines[lineIndexInPage].length();
}

// Clamp a position to the buffer bounds
std::pair<size_t, size_t> VirtualizedTextBuffer::clampPosition(size_t lineIndex, size_t colIndex) const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    // Handle empty buffer case
    if (totalLines_ == 0) {
        return {0, 0};
    }
    
    // Clamp line index
    lineIndex = std::min(lineIndex, totalLines_ - 1);
    
    // Clamp column index
    size_t pageNumber = getPageNumber(lineIndex);
    size_t lineIndexInPage = getLineIndexInPage(lineIndex);
    
    auto page = getPage(pageNumber);
    colIndex = std::min(colIndex, page->lines[lineIndexInPage].length());
    
    return {lineIndex, colIndex};
}

// Print the buffer to an output stream
void VirtualizedTextBuffer::printToStream(std::ostream& os) const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    for (size_t i = 0; i < totalLines_; ++i) {
        size_t pageNumber = getPageNumber(i);
        size_t lineIndexInPage = getLineIndexInPage(i);
        
        auto page = getPage(pageNumber);
        os << page->lines[lineIndexInPage];
        
        // Add newline character unless it's the very last line
        if (i < totalLines_ - 1) {
            os << '\n';
        }
    }
}

// Save the buffer to a file
bool VirtualizedTextBuffer::saveToFile(const std::string& filename) const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    // If we're saving to the same file we loaded from, just flush all dirty pages
    if (isFromFile_ && filename == filename_) {
        for (const auto& pair : pageCache_) {
            if (pair.second->dirty) {
                savePage(pair.first, *pair.second);
            }
        }
        
        return true;
    }
    
    // Otherwise, create a new file
    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        LOG_ERROR("Could not open file for saving: " + filename);
        return false;
    }
    
    for (size_t i = 0; i < totalLines_; ++i) {
        size_t pageNumber = getPageNumber(i);
        size_t lineIndexInPage = getLineIndexInPage(i);
        
        auto page = getPage(pageNumber);
        outfile << page->lines[lineIndexInPage];
        
        // Add newline character unless it's the very last line
        if (i < totalLines_ - 1) {
            outfile << '\n';
        }
    }
    
    if (outfile.fail()) {
        LOG_ERROR("Failed while writing to file: " + filename);
        outfile.close();
        return false;
    }
    
    outfile.close();
    return true;
}

// Load the buffer from a file
bool VirtualizedTextBuffer::loadFromFile(const std::string& filename)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    // Clear the current buffer
    pageCache_.clear();
    lruList_.clear();
    lineOffsets_.clear();
    totalLines_ = 0;
    
    // Set the new filename
    filename_ = filename;
    isFromFile_ = true;
    
    // Initialize from the file
    try {
        initFromFile(filename);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load file: " + std::string(e.what()));
        return false;
    }
    
    setModified(false);
    return true;
}

// Insert a character at the specified position
void VirtualizedTextBuffer::insertChar(size_t lineIndex, size_t colIndex, char ch)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (lineIndex >= totalLines_) {
        LOG_ERROR("Line index out of range for insertChar: " + std::to_string(lineIndex));
        throw TextBufferException("Line index out of range for insertChar", EditorException::Severity::EDITOR_ERROR);
    }
    
    size_t pageNumber = getPageNumber(lineIndex);
    size_t lineIndexInPage = getLineIndexInPage(lineIndex);
    
    auto page = ensurePage(pageNumber, true);
    
    if (colIndex > page->lines[lineIndexInPage].length()) {
        LOG_ERROR("Column index out of range for insertChar: " + std::to_string(colIndex));
        throw TextBufferException("Column index out of range for insertChar", EditorException::Severity::EDITOR_ERROR);
    }
    
    page->lines[lineIndexInPage].insert(colIndex, 1, ch);
    
    setModified(true);
}

// Delete a character before the specified position (backspace)
void VirtualizedTextBuffer::deleteChar(size_t lineIndex, size_t colIndex)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (lineIndex >= totalLines_) {
        LOG_ERROR("Line index out of range for deleteChar: " + std::to_string(lineIndex));
        throw TextBufferException("Line index out of range for deleteChar", EditorException::Severity::EDITOR_ERROR);
    }
    
    size_t pageNumber = getPageNumber(lineIndex);
    size_t lineIndexInPage = getLineIndexInPage(lineIndex);
    
    auto page = ensurePage(pageNumber, true);
    std::string& line = page->lines[lineIndexInPage];
    
    if (colIndex == 0) {
        // Backspace at start of line - join with previous line if possible
        if (lineIndex > 0) {
            // Get the previous line
            size_t prevPageNumber = getPageNumber(lineIndex - 1);
            size_t prevLineIndexInPage = getLineIndexInPage(lineIndex - 1);
            
            auto prevPage = ensurePage(prevPageNumber, true);
            std::string& prevLine = prevPage->lines[prevLineIndexInPage];
            
            // Join the lines
            prevLine += line;
            
            // Delete the current line
            lock.unlock();
            deleteLine(lineIndex);
        }
    } else if (colIndex <= line.length()) {
        // Normal backspace within a line
        if (colIndex > 0 && colIndex <= line.length() && line.length() > 0) {
            line.erase(colIndex - 1, 1);
            setModified(true);
        }
    } else {
        // If colIndex is beyond line length, treat as backspace at the end of the line
        if (line.length() > 0) {
            line.erase(line.length() - 1, 1);
            setModified(true);
        }
    }
}

// Delete a character at the specified position (delete)
void VirtualizedTextBuffer::deleteCharForward(size_t lineIndex, size_t colIndex)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (lineIndex >= totalLines_) {
        LOG_ERROR("Line index out of range for deleteCharForward: " + std::to_string(lineIndex));
        throw TextBufferException("Line index out of range for deleteCharForward", EditorException::Severity::EDITOR_ERROR);
    }
    
    size_t pageNumber = getPageNumber(lineIndex);
    size_t lineIndexInPage = getLineIndexInPage(lineIndex);
    
    auto page = ensurePage(pageNumber, true);
    std::string& line = page->lines[lineIndexInPage];
    
    if (colIndex > line.length() && (lineIndex == totalLines_ - 1 || colIndex > line.length() + 100)) {
        LOG_ERROR("Column index out of range for deleteCharForward: " + std::to_string(colIndex));
        throw TextBufferException("Column index out of range for deleteCharForward", EditorException::Severity::EDITOR_ERROR);
    }
    
    if (colIndex < line.length()) {
        // Normal delete within a line
        line.erase(colIndex, 1);
        setModified(true);
    } else if (lineIndex < totalLines_ - 1) {
        // Delete at end of line - join with next line
        // Get the next line
        size_t nextPageNumber = getPageNumber(lineIndex + 1);
        size_t nextLineIndexInPage = getLineIndexInPage(lineIndex + 1);
        
        auto nextPage = ensurePage(nextPageNumber, true);
        std::string& nextLine = nextPage->lines[nextLineIndexInPage];
        
        // Join the lines
        line += nextLine;
        
        // Delete the next line
        lock.unlock();
        deleteLine(lineIndex + 1);
    }
    // If we're at the end of the last line, do nothing
}

// Replace a segment of a line
void VirtualizedTextBuffer::replaceLineSegment(size_t lineIndex, size_t startCol, size_t endCol, const std::string& newText)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (lineIndex >= totalLines_) {
        LOG_ERROR("Line index out of range for replaceLineSegment: " + std::to_string(lineIndex));
        throw TextBufferException("Line index out of range for replaceLineSegment", EditorException::Severity::EDITOR_ERROR);
    }
    
    size_t pageNumber = getPageNumber(lineIndex);
    size_t lineIndexInPage = getLineIndexInPage(lineIndex);
    
    auto page = ensurePage(pageNumber, true);
    std::string& line = page->lines[lineIndexInPage];
    
    // Ensure startCol <= endCol
    if (startCol > endCol) {
        std::swap(startCol, endCol);
    }
    
    // Clamp endCol to line length if it exceeds it
    endCol = std::min(endCol, line.length());
    
    // If startCol is beyond line length, treat as append
    if (startCol >= line.length()) {
        line.append(newText);
    } else {
        // Replace the segment
        line.replace(startCol, endCol - startCol, newText);
    }
    
    setModified(true);
}

// Delete a segment of a line
void VirtualizedTextBuffer::deleteLineSegment(size_t lineIndex, size_t startCol, size_t endCol)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (lineIndex >= totalLines_) {
        LOG_ERROR("Line index out of range for deleteLineSegment: " + std::to_string(lineIndex));
        throw TextBufferException("Line index out of range for deleteLineSegment", EditorException::Severity::EDITOR_ERROR);
    }
    
    size_t pageNumber = getPageNumber(lineIndex);
    size_t lineIndexInPage = getLineIndexInPage(lineIndex);
    
    auto page = ensurePage(pageNumber, true);
    std::string& line = page->lines[lineIndexInPage];
    
    // Ensure startCol <= endCol
    if (startCol > endCol) {
        std::swap(startCol, endCol);
    }
    
    // Clamp endCol to line length if it exceeds it
    endCol = std::min(endCol, line.length());
    
    // If startCol is beyond line length or startCol equals endCol, do nothing
    if (startCol >= line.length() || startCol == endCol) {
        return;
    }
    
    // Delete the segment
    line.erase(startCol, endCol - startCol);
    
    setModified(true);
}

// Split a line at the specified position
void VirtualizedTextBuffer::splitLine(size_t lineIndex, size_t colIndex)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (lineIndex >= totalLines_) {
        LOG_ERROR("Line index out of range for splitLine: " + std::to_string(lineIndex));
        throw TextBufferException("Line index out of range for splitLine", EditorException::Severity::EDITOR_ERROR);
    }
    
    size_t pageNumber = getPageNumber(lineIndex);
    size_t lineIndexInPage = getLineIndexInPage(lineIndex);
    
    auto page = ensurePage(pageNumber, true);
    std::string& line = page->lines[lineIndexInPage];
    
    if (colIndex > line.length()) {
        LOG_ERROR("Column index out of range for splitLine: " + std::to_string(colIndex));
        throw TextBufferException("Column index out of range for splitLine", EditorException::Severity::EDITOR_ERROR);
    }
    
    // Extract the part of the line after the split point
    std::string newLine = line.substr(colIndex);
    
    // Keep only the part before the split point in the original line
    line.erase(colIndex);
    
    // Insert the new line after the current line
    lock.unlock();
    insertLine(lineIndex + 1, newLine);
}

// Join a line with the next line
void VirtualizedTextBuffer::joinLines(size_t lineIndex)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (lineIndex >= totalLines_ - 1) {
        LOG_ERROR("Cannot join last line with next line");
        throw TextBufferException("Cannot join last line with next line", EditorException::Severity::EDITOR_ERROR);
    }
    
    size_t pageNumber = getPageNumber(lineIndex);
    size_t lineIndexInPage = getLineIndexInPage(lineIndex);
    
    auto page = ensurePage(pageNumber, true);
    std::string& line = page->lines[lineIndexInPage];
    
    // Get the next line
    size_t nextPageNumber = getPageNumber(lineIndex + 1);
    size_t nextLineIndexInPage = getLineIndexInPage(lineIndex + 1);
    
    auto nextPage = ensurePage(nextPageNumber, true);
    std::string& nextLine = nextPage->lines[nextLineIndexInPage];
    
    // Join the lines
    line += nextLine;
    
    // Delete the next line
    lock.unlock();
    deleteLine(lineIndex + 1);
}

// Insert a string at the specified position
void VirtualizedTextBuffer::insertString(size_t lineIndex, size_t colIndex, const std::string& text)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (lineIndex >= totalLines_) {
        LOG_ERROR("Line index out of range for insertString: " + std::to_string(lineIndex));
        throw TextBufferException("Line index out of range for insertString", EditorException::Severity::EDITOR_ERROR);
    }
    
    size_t pageNumber = getPageNumber(lineIndex);
    size_t lineIndexInPage = getLineIndexInPage(lineIndex);
    
    auto page = ensurePage(pageNumber, true);
    std::string& line = page->lines[lineIndexInPage];
    
    if (colIndex > line.length()) {
        LOG_ERROR("Column index out of range for insertString: " + std::to_string(colIndex));
        throw TextBufferException("Column index out of range for insertString", EditorException::Severity::EDITOR_ERROR);
    }
    
    // Check if the text contains newlines
    size_t newlinePos = text.find('\n');
    if (newlinePos == std::string::npos) {
        // Simple case: no newlines, just insert the text
        line.insert(colIndex, text);
        setModified(true);
        return;
    }
    
    // The text contains newlines, we need to split it
    size_t currentPosInInputText = 0;
    size_t lastNewlinePosInInputText = std::string::npos;
    
    for (size_t i = 0; i < text.length(); ++i) {
        if (text[i] == '\n') {
            // Get the text after the insertion point in the current line
            std::string textAfterNewline = line.substr(colIndex + currentPosInInputText);
            
            // Truncate the current line at the insertion point
            line.erase(colIndex + currentPosInInputText);
            
            // Append the text up to the newline
            line += text.substr(lastNewlinePosInInputText == std::string::npos ? 0 : lastNewlinePosInInputText + 1, 
                                i - (lastNewlinePosInInputText == std::string::npos ? 0 : lastNewlinePosInInputText + 1));
            
            // Insert a new line after the current line
            lock.unlock();
            insertLine(lineIndex + 1, textAfterNewline);
            lock.lock();
            
            // Update the current line
            lineIndex++;
            pageNumber = getPageNumber(lineIndex);
            lineIndexInPage = getLineIndexInPage(lineIndex);
            
            page = ensurePage(pageNumber, true);
            line = page->lines[lineIndexInPage];
            
            colIndex = 0;
            currentPosInInputText = 0;
            lastNewlinePosInInputText = i;
        } else {
            currentPosInInputText++;
        }
    }
    
    // Insert any remaining text
    std::string remainingTextToInsert = text.substr(lastNewlinePosInInputText == std::string::npos ? 0 : lastNewlinePosInInputText + 1);
    
    if (!remainingTextToInsert.empty()) {
        line.insert(colIndex, remainingTextToInsert);
    }
    
    setModified(true);
}

// Get a segment of a line
std::string VirtualizedTextBuffer::getLineSegment(size_t lineIndex, size_t startCol, size_t endCol) const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    if (lineIndex >= totalLines_) {
        LOG_ERROR("Line index out of range for getLineSegment: " + std::to_string(lineIndex));
        throw TextBufferException("Line index out of range for getLineSegment", EditorException::Severity::EDITOR_ERROR);
    }
    
    size_t pageNumber = getPageNumber(lineIndex);
    size_t lineIndexInPage = getLineIndexInPage(lineIndex);
    
    auto page = getPage(pageNumber);
    const std::string& line = page->lines[lineIndexInPage];
    
    // Validate column indices
    if (startCol > endCol || startCol > line.length()) {
        LOG_ERROR("Invalid column range for getLineSegment: " + std::to_string(startCol) + " to " + std::to_string(endCol));
        throw TextBufferException("Invalid column range for getLineSegment", EditorException::Severity::EDITOR_ERROR);
    }
    
    // Clamp endCol to line length
    endCol = std::min(endCol, line.length());
    
    // Return the segment
    return line.substr(startCol, endCol - startCol);
}

// Get the number of lines
size_t VirtualizedTextBuffer::getLineCount() const
{
    // This is the same as lineCount()
    return lineCount();
}

// Get all lines
std::vector<std::string> VirtualizedTextBuffer::getLines() const
{
    // This is the same as getAllLines()
    return getAllLines();
}

// Replace a range of text
void VirtualizedTextBuffer::replaceText(size_t startLine, size_t startCol, size_t endLine, size_t endCol, const std::string& text)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (startLine >= totalLines_ || endLine >= totalLines_) {
        LOG_ERROR("Line index out of range for replaceText: " + std::to_string(startLine) + " to " + std::to_string(endLine));
        throw TextBufferException("Line index out of range for replaceText", EditorException::Severity::EDITOR_ERROR);
    }
    
    if (startLine == endLine) {
        // Single line replacement
        lock.unlock();
        replaceLineSegment(startLine, startCol, endCol, text);
        return;
    }
    
    // Multi-line replacement
    
    // Store text after endCol in the last line
    size_t endPageNumber = getPageNumber(endLine);
    size_t endLineIndexInPage = getLineIndexInPage(endLine);
    
    auto endPage = ensurePage(endPageNumber, true);
    std::string& endLineText = endPage->lines[endLineIndexInPage];
    
    std::string endLineRemainder = "";
    if (endCol < endLineText.length()) {
        endLineRemainder = endLineText.substr(endCol);
    }
    
    // Keep text before startCol in the first line
    size_t startPageNumber = getPageNumber(startLine);
    size_t startLineIndexInPage = getLineIndexInPage(startLine);
    
    auto startPage = ensurePage(startPageNumber, true);
    std::string& startLineText = startPage->lines[startLineIndexInPage];
    
    std::string startLinePrefix = "";
    if (startCol > 0) {
        startLinePrefix = startLineText.substr(0, startCol);
    }
    
    // Delete all lines between startLine+1 and endLine (inclusive)
    for (size_t i = endLine; i > startLine; --i) {
        lock.unlock();
        deleteLine(i);
        lock.lock();
    }
    
    // Replace the content of the first line
    startPage = ensurePage(startPageNumber, true);
    startLineText = startLinePrefix + text + endLineRemainder;
    
    setModified(true);
}

// Insert text at a position
void VirtualizedTextBuffer::insertText(size_t line, size_t col, const std::string& text)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (line >= totalLines_) {
        LOG_ERROR("Line index out of range for insertText: " + std::to_string(line));
        throw TextBufferException("Line index out of range for insertText", EditorException::Severity::EDITOR_ERROR);
    }
    
    size_t pageNumber = getPageNumber(line);
    size_t lineIndexInPage = getLineIndexInPage(line);
    
    auto page = ensurePage(pageNumber, true);
    std::string& lineText = page->lines[lineIndexInPage];
    
    if (col > lineText.length()) {
        LOG_ERROR("Column index out of range for insertText: " + std::to_string(col));
        throw TextBufferException("Column index out of range for insertText", EditorException::Severity::EDITOR_ERROR);
    }
    
    // Check if text contains newlines
    size_t newlinePos = text.find('\n');
    if (newlinePos == std::string::npos) {
        // Simple case: no newlines, just insert the text
        lineText.insert(col, text);
        setModified(true);
    } else {
        // Text contains newlines, need to split it
        lock.unlock();
        insertString(line, col, text);
    }
}

// Delete a range of text
void VirtualizedTextBuffer::deleteText(size_t startLine, size_t startCol, size_t endLine, size_t endCol)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (startLine >= totalLines_ || endLine >= totalLines_) {
        LOG_ERROR("Line index out of range for deleteText: " + std::to_string(startLine) + " to " + std::to_string(endLine));
        throw TextBufferException("Line index out of range for deleteText", EditorException::Severity::EDITOR_ERROR);
    }
    
    if (startLine == endLine) {
        // Single line deletion
        lock.unlock();
        deleteLineSegment(startLine, startCol, endCol);
        return;
    }
    
    // Multi-line deletion
    
    // Keep text before startCol in the first line
    size_t startPageNumber = getPageNumber(startLine);
    size_t startLineIndexInPage = getLineIndexInPage(startLine);
    
    auto startPage = ensurePage(startPageNumber, true);
    std::string& startLineText = startPage->lines[startLineIndexInPage];
    
    std::string startLinePrefix = "";
    if (startCol > 0) {
        startLinePrefix = startLineText.substr(0, startCol);
    }
    
    // Keep text after endCol in the last line
    size_t endPageNumber = getPageNumber(endLine);
    size_t endLineIndexInPage = getLineIndexInPage(endLine);
    
    auto endPage = ensurePage(endPageNumber, true);
    std::string& endLineText = endPage->lines[endLineIndexInPage];
    
    std::string endLineSuffix = "";
    if (endCol < endLineText.length()) {
        endLineSuffix = endLineText.substr(endCol);
    }
    
    // Combine the remaining parts
    startPage = ensurePage(startPageNumber, true);
    startLineText = startLinePrefix + endLineSuffix;
    
    // Delete all lines between startLine+1 and endLine (inclusive)
    for (size_t i = endLine; i > startLine; --i) {
        lock.unlock();
        deleteLine(i);
        lock.lock();
    }
    
    setModified(true);
}

// Check if the buffer has been modified
bool VirtualizedTextBuffer::isModified() const
{
    return modified_.load();
}

// Set the modified state of the buffer
void VirtualizedTextBuffer::setModified(bool modified)
{
    modified_.store(modified);
}

// Set the cache eviction policy
void VirtualizedTextBuffer::setCacheEvictionPolicy(CacheEvictionPolicy policy)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (policy == evictionPolicy_) {
        return; // No change needed
    }
    
    LOG_DEBUG("Changing cache eviction policy");
    
    // Initialize data structures for the new policy if needed
    switch (policy) {
        case CacheEvictionPolicy::LRU:
        default:
            // LRU is already the default, no special initialization needed
            break;
    }
    
    evictionPolicy_ = policy;
} // End of setCacheEvictionPolicy

// Get the current cache eviction policy
VirtualizedTextBuffer::CacheEvictionPolicy VirtualizedTextBuffer::getCacheEvictionPolicy() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return evictionPolicy_;
}

// Set the prefetch strategy
void VirtualizedTextBuffer::setPrefetchStrategy(PrefetchStrategy strategy)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (strategy == prefetchStrategy_) {
        return; // No change needed
    }
    
    LOG_DEBUG("Changing prefetch strategy");
    
    // Initialize data structures for the new strategy if needed
    switch (strategy) {
        case PrefetchStrategy::NONE:
            // When disabling prefetching, clear the queue
            while (!prefetchQueue_.empty()) {
                prefetchQueue_.pop();
            }
            break;
        case PrefetchStrategy::ADJACENT:
        case PrefetchStrategy::PREDICTIVE:
        case PrefetchStrategy::ADAPTIVE:
        default:
            // These strategies will be handled during page requests
            break;
    }

    // Clear any existing prefetch queue
    while (!prefetchQueue_.empty()) {
        prefetchQueue_.pop();
    }
    
    prefetchStrategy_ = strategy;
} // End of setPrefetchStrategy

// Get the current prefetch strategy
VirtualizedTextBuffer::PrefetchStrategy VirtualizedTextBuffer::getPrefetchStrategy() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return prefetchStrategy_;
}

// Set the prefetch distance
void VirtualizedTextBuffer::setPrefetchDistance(size_t distance)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    prefetchDistance_ = distance;
}

// Get the current prefetch distance
size_t VirtualizedTextBuffer::getPrefetchDistance() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return prefetchDistance_;
}

// Set the maximum prefetch queue size
void VirtualizedTextBuffer::setMaxPrefetchQueueSize(size_t size)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    maxPrefetchQueueSize_ = size;
}

// Update the access pattern tracking
void VirtualizedTextBuffer::updateAccessPattern(size_t pageNumber) const
{
    // Add this page to recent accesses
    recentAccesses_.push_back(pageNumber);
    
    // Maintain maximum size of recent accesses queue
    if (recentAccesses_.size() > recentAccessesMaxSize_) {
        recentAccesses_.pop_front();
    }
    
    // Update transition counts if we have at least 2 accesses
    if (recentAccesses_.size() >= 2) {
        size_t prevPage = recentAccesses_[recentAccesses_.size() - 2];
        
        // Don't count self-transitions
        if (prevPage != pageNumber) {
            // Increment the count for this transition
            transitionCounts_[prevPage][pageNumber]++;
        }
    }
    
    // If this page was in the prefetch queue and was just accessed,
    // it's a prefetch hit
    std::priority_queue<PrefetchRequest> tempQueue;
    
    while (!prefetchQueue_.empty()) {
        PrefetchRequest req = prefetchQueue_.top();
        prefetchQueue_.pop();
        
        if (req.pageNumber == pageNumber) {
            prefetchHits_++;
        } else {
            tempQueue.push(req);
        }
    }
    
    // Restore the queue without the hit page
    prefetchQueue_ = std::move(tempQueue);
    
    // Initiate prefetching based on this access
    if (prefetchStrategy_ != PrefetchStrategy::NONE) {
        initiateStrategicPrefetch(pageNumber);
    }
}

// Initiate prefetching based on current strategy and recent accesses
void VirtualizedTextBuffer::initiateStrategicPrefetch(size_t triggerPageNumber) const
{
    switch (prefetchStrategy_) {
        case PrefetchStrategy::ADJACENT:
        {
            prefetchAdjacentPages(triggerPageNumber);
            break;
        }
            
        case PrefetchStrategy::PREDICTIVE:
        {
            prefetchPredictivePages(triggerPageNumber);
            break;
        }
            
        case PrefetchStrategy::ADAPTIVE:
        {
            prefetchAdaptivePages(triggerPageNumber);
            break;
        }
            
        case PrefetchStrategy::NONE:
        default:
        {
            // No prefetching
            break;
        }
    }
    
    // Process the prefetch queue
    processPrefetchQueue(maxPrefetchQueueSize_);
}

// Prefetch pages adjacent to the specified page
void VirtualizedTextBuffer::prefetchAdjacentPages(size_t pageNumber) const
{
    // Calculate the range of pages to prefetch
    size_t startPage = (pageNumber > prefetchDistance_) ? 
                        pageNumber - prefetchDistance_ : 0;
    
    size_t endPage = std::min(pageNumber + prefetchDistance_, 
                             (totalLines_ / pageSize_) + (totalLines_ % pageSize_ > 0 ? 1 : 0) - 1);
    
    // Queue pages for prefetching, prioritizing closer pages
    for (size_t page = startPage; page <= endPage; ++page) {
        // Skip the trigger page and pages already in cache
        if (page == pageNumber || pageCache_.find(page) != pageCache_.end()) {
            continue;
        }
        
        // Calculate priority based on distance
        double priority = 1.0 / (1.0 + std::abs(static_cast<int>(page) - static_cast<int>(pageNumber)));
        
        queueForPrefetch(page, priority);
    }
}

// Prefetch pages based on predicted access patterns
void VirtualizedTextBuffer::prefetchPredictivePages(size_t pageNumber) const
{
    // First, get the most likely pages to be accessed next based on transition counts
    auto it = transitionCounts_.find(pageNumber);
    if (it != transitionCounts_.end()) {
        // Build a vector of (nextPage, count) pairs
        std::vector<std::pair<size_t, size_t>> transitions;
        
        for (const auto& next : it->second) {
            transitions.push_back(next);
        }
        
        // Sort by count in descending order
        std::sort(transitions.begin(), transitions.end(), 
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Queue the top N pages for prefetching
        size_t maxTransitions = std::min(transitions.size(), maxPrefetchQueueSize_);
        
        for (size_t i = 0; i < maxTransitions; ++i) {
            size_t nextPage = transitions[i].first;
            size_t count = transitions[i].second;
            
            // Skip pages already in cache
            if (pageCache_.find(nextPage) != pageCache_.end()) {
                continue;
            }
            
            // Calculate priority based on transition count
            double priority = static_cast<double>(count) / transitions[0].second;
            
            queueForPrefetch(nextPage, priority);
        }
    }
    
    // If we don't have enough history, fall back to adjacent prefetching
    if (prefetchQueue_.empty()) {
        prefetchAdjacentPages(pageNumber);
    }
}

// Adaptively prefetch pages based on current access patterns and cache stats
void VirtualizedTextBuffer::prefetchAdaptivePages(size_t pageNumber) const
{
    // Calculate prefetch effectiveness
    double effectiveness = 0.0;
    
    if (prefetchHits_ + prefetchMisses_ > 0) {
        effectiveness = static_cast<double>(prefetchHits_) / (prefetchHits_ + prefetchMisses_);
    }
    
    // If predictive prefetching is effective, use it
    if (effectiveness > 0.5 && !transitionCounts_.empty()) {
        prefetchPredictivePages(pageNumber);
    } else {
        // Otherwise, fall back to adjacent prefetching
        prefetchAdjacentPages(pageNumber);
    }
}

// Calculate the priority of a page for prefetching
double VirtualizedTextBuffer::calculatePrefetchPriority(size_t pageNumber, size_t triggerPage) const
{
    double priority = 1.0;
    
    // Base priority on distance
    double distanceFactor = 1.0 / (1.0 + std::abs(static_cast<int>(pageNumber) - static_cast<int>(triggerPage)));
    priority *= distanceFactor;
    
    // If we have transition data, incorporate it
    auto it = transitionCounts_.find(triggerPage);
    if (it != transitionCounts_.end()) {
        auto nextIt = it->second.find(pageNumber);
        if (nextIt != it->second.end()) {
            // Adjust priority based on transition frequency
            priority *= (1.0 + log10(1.0 + nextIt->second));
        }
    }
    
    // Reduce priority for pages that are likely to be evicted soon
    if (evictionPolicy_ == CacheEvictionPolicy::LRU) {
        auto lruPos = std::find(lruList_.begin(), lruList_.end(), pageNumber);
        if (lruPos != lruList_.end()) {
            double lruFactor = static_cast<double>(std::distance(lruList_.begin(), lruPos)) / lruList_.size();
            priority *= (1.0 - lruFactor);
        }
    }
    
    return priority;
}

// Queue a page for prefetching with a given priority
void VirtualizedTextBuffer::queueForPrefetch(size_t pageNumber, double priority) const
{
    // Skip pages already in cache or queue
    if (pageCache_.find(pageNumber) != pageCache_.end()) {
        return;
    }
    
    // Check if this page is already in the queue
    bool alreadyQueued = false;
    std::priority_queue<PrefetchRequest> tempQueue;
    
    while (!prefetchQueue_.empty()) {
        PrefetchRequest req = prefetchQueue_.top();
        prefetchQueue_.pop();
        
        if (req.pageNumber == pageNumber) {
            // Update priority if higher
            if (priority > req.priority) {
                req.priority = priority;
            }
            alreadyQueued = true;
        }
        
        tempQueue.push(req);
    }
    
    // Restore the queue
    prefetchQueue_ = std::move(tempQueue);
    
    // Add to queue if not already there
    if (!alreadyQueued) {
        // Ensure we don't exceed the maximum queue size
        if (prefetchQueue_.size() >= maxPrefetchQueueSize_) {
            // Remove the lowest priority item
            std::priority_queue<PrefetchRequest> newQueue;
            
            size_t itemsToKeep = maxPrefetchQueueSize_ - 1;
            
            while (itemsToKeep > 0 && !prefetchQueue_.empty()) {
                newQueue.push(prefetchQueue_.top());
                prefetchQueue_.pop();
                itemsToKeep--;
            }
            
            prefetchQueue_ = std::move(newQueue);
        }
        
        // Add the new request
        PrefetchRequest req{pageNumber, priority};
        prefetchQueue_.push(req);
    }
}

// Process the prefetch queue
void VirtualizedTextBuffer::processPrefetchQueue(size_t maxPages) const
{
    size_t processedPages = 0;
    
    // Limit the number of pages to prefetch at once
    while (processedPages < maxPages && !prefetchQueue_.empty()) {
        // Get the highest priority request
        PrefetchRequest req = prefetchQueue_.top();
        prefetchQueue_.pop();
        
        // Skip if already in cache
        if (pageCache_.find(req.pageNumber) != pageCache_.end()) {
            continue;
        }
        
        // Ensure we have space in cache
        while (pageCache_.size() >= cacheSize_) {
            evictPage();
        }
        
        // Load the page
        try {
            LOG_DEBUG("Prefetching page " + std::to_string(req.pageNumber) + " with priority " + std::to_string(req.priority));
            
            auto page = loadPage(req.pageNumber);
            
            // Add the page to cache
            pageCache_[req.pageNumber] = page;
            
            // Add to appropriate structure based on eviction policy
            switch (evictionPolicy_) {
                case CacheEvictionPolicy::LRU:
                    lruList_.push_back(req.pageNumber);
                    break;
                    
                case CacheEvictionPolicy::SLRU:
                    probationarySegment_.push_back(req.pageNumber);
                    break;
                    
                case CacheEvictionPolicy::ARC:
                    recentlyUsed_.insert(req.pageNumber);
                    break;
                    
                case CacheEvictionPolicy::SPATIAL:
                    lruList_.push_back(req.pageNumber);
                    spatialScores_[req.pageNumber] = 0.5; // Middle priority for prefetched pages
                    break;
            }
            
            processedPages++;
            
            // Count as a miss for now (will be counted as a hit if accessed)
            prefetchMisses_++;
            
        } catch (const std::exception& e) {
            LOG_ERROR("Error prefetching page " + std::to_string(req.pageNumber) + ": " + e.what());
        }
    }
}

// Evict a page from cache according to current policy
void VirtualizedTextBuffer::evictPage() const
{
    switch (evictionPolicy_) {
        case CacheEvictionPolicy::SLRU:
            evictSLRUPage();
            break;
            
        case CacheEvictionPolicy::ARC:
            evictARCPage();
            break;
            
        case CacheEvictionPolicy::SPATIAL:
            evictSpatialPage();
            break;
            
        case CacheEvictionPolicy::LRU:
        default:
            evictLRUPage();
            break;
    }
}

// Evict a page using the Segmented LRU policy
void VirtualizedTextBuffer::evictSLRUPage() const
{
    // If both segments are empty, there's nothing to evict
    if (probationarySegment_.empty() && protectedSegment_.empty()) {
        return;
    }
    
    // Always evict from probationary segment if possible
    if (!probationarySegment_.empty()) {
        size_t pageNumber = probationarySegment_.front();
        probationarySegment_.pop_front();
        
        // Save the page if it's dirty
        auto it = pageCache_.find(pageNumber);
        if (it != pageCache_.end() && it->second->dirty) {
            savePage(pageNumber, *it->second);
        }
        
        // Remove the page from cache
        pageCache_.erase(pageNumber);
        
        LOG_DEBUG("Evicted page " + std::to_string(pageNumber) + " from probationary segment");
        
    } else {
        // If probationary segment is empty, evict from protected segment
        size_t pageNumber = protectedSegment_.front();
        protectedSegment_.pop_front();
        
        // Save the page if it's dirty
        auto it = pageCache_.find(pageNumber);
        if (it != pageCache_.end() && it->second->dirty) {
            savePage(pageNumber, *it->second);
        }
        
        // Remove the page from cache
        pageCache_.erase(pageNumber);
        
        LOG_DEBUG("Evicted page " + std::to_string(pageNumber) + " from protected segment");
    }
}

// Evict a page using the Adaptive Replacement Cache policy
void VirtualizedTextBuffer::evictARCPage() const
{
    // If both segments are empty, there's nothing to evict
    if (recentlyUsed_.empty() && frequentlyUsed_.empty()) {
        return;
    }
    
    // Decide which list to evict from based on the adaptive parameter p
    size_t pageNumber;
    bool fromRecent = true;
    
    if (recentlyUsed_.size() > static_cast<size_t>(arcP_) && !recentlyUsed_.empty()) {
        // Evict from recently used
        pageNumber = *recentlyUsed_.begin();
        recentlyUsed_.erase(recentlyUsed_.begin());
        
        // Add to ghost cache
        ghostRecent_.insert(pageNumber);
        
        // Limit ghost cache size
        if (ghostRecent_.size() > cacheSize_) {
            ghostRecent_.erase(ghostRecent_.begin());
        }
        
    } else if (!frequentlyUsed_.empty()) {
        // Evict from frequently used
        pageNumber = *frequentlyUsed_.begin();
        frequentlyUsed_.erase(frequentlyUsed_.begin());
        
        // Add to ghost cache
        ghostFrequent_.insert(pageNumber);
        
        // Limit ghost cache size
        if (ghostFrequent_.size() > cacheSize_) {
            ghostFrequent_.erase(ghostFrequent_.begin());
        }
        
        fromRecent = false;
    } else {
        // Fall back to evicting from recently used
        pageNumber = *recentlyUsed_.begin();
        recentlyUsed_.erase(recentlyUsed_.begin());
        
        // Add to ghost cache
        ghostRecent_.insert(pageNumber);
        
        // Limit ghost cache size
        if (ghostRecent_.size() > cacheSize_) {
            ghostRecent_.erase(ghostRecent_.begin());
        }
    }
    
    // Save the page if it's dirty
    auto it = pageCache_.find(pageNumber);
    if (it != pageCache_.end() && it->second->dirty) {
        savePage(pageNumber, *it->second);
    }
    
    // Remove the page from cache
    pageCache_.erase(pageNumber);
    
    LOG_DEBUG("Evicted page " + std::to_string(pageNumber) + " from " + 
             (fromRecent ? "recently used" : "frequently used") + " segment");
}

// Evict a page using the Spatial locality aware policy
void VirtualizedTextBuffer::evictSpatialPage() const
{
    // If LRU list is empty, there's nothing to evict
    if (lruList_.empty()) {
        return;
    }
    
    // Find the page with the lowest spatial score
    size_t lowestScorePage = lruList_.front();
    double lowestScore = spatialScores_.count(lowestScorePage) ? 
                         spatialScores_.at(lowestScorePage) : 0.0;
    
    for (const auto& pageNumber : lruList_) {
        double score = spatialScores_.count(pageNumber) ? 
                      spatialScores_.at(pageNumber) : 0.0;
        
        // Skip pinned pages
        auto it = pageCache_.find(pageNumber);
        if (it != pageCache_.end() && it->second->isPinned) {
            continue;
        }
        
        if (score < lowestScore) {
            lowestScore = score;
            lowestScorePage = pageNumber;
        }
    }
    
    // Remove the page from the LRU list
    auto lruIt = std::find(lruList_.begin(), lruList_.end(), lowestScorePage);
    if (lruIt != lruList_.end()) {
        lruList_.erase(lruIt);
    }
    
    // Save the page if it's dirty
    auto it = pageCache_.find(lowestScorePage);
    if (it != pageCache_.end() && it->second->dirty) {
        savePage(lowestScorePage, *it->second);
    }
    
    // Remove the page from cache
    pageCache_.erase(lowestScorePage);
    spatialScores_.erase(lowestScorePage);
    
    LOG_DEBUG("Evicted page " + std::to_string(lowestScorePage) + " with spatial score " + 
             std::to_string(lowestScore));
} 
