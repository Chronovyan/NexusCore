#include "ThreadSafeVirtualizedTextBuffer.h"
#include "EditorError.h"
#include "LoggingCompatibility.h"
#include "AppDebugLog.h"

// Constructor with optional buffer
ThreadSafeVirtualizedTextBuffer::ThreadSafeVirtualizedTextBuffer(std::shared_ptr<VirtualizedTextBuffer> buffer)
    : buffer_(buffer ? buffer : std::make_shared<VirtualizedTextBuffer>())
{
    LOG_DEBUG("ThreadSafeVirtualizedTextBuffer created");
}

// Constructor with file
ThreadSafeVirtualizedTextBuffer::ThreadSafeVirtualizedTextBuffer(const std::string& filename, 
                                                             size_t pageSize, 
                                                             size_t cacheSize)
    : buffer_(std::make_shared<VirtualizedTextBuffer>(filename, pageSize, cacheSize))
{
    LOG_DEBUG("ThreadSafeVirtualizedTextBuffer created from file: " + filename);
}

// Get the underlying buffer
std::shared_ptr<VirtualizedTextBuffer> ThreadSafeVirtualizedTextBuffer::getUnderlyingBuffer() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_;
}

// Lock for reading
void ThreadSafeVirtualizedTextBuffer::lockForReading() const
{
    mutex_.lock_shared();
}

// Unlock after reading
void ThreadSafeVirtualizedTextBuffer::unlockReading() const
{
    mutex_.unlock_shared();
}

// Lock for writing
void ThreadSafeVirtualizedTextBuffer::lockForWriting()
{
    mutex_.lock();
}

// Unlock after writing
void ThreadSafeVirtualizedTextBuffer::unlockWriting()
{
    mutex_.unlock();
}

// Set the page size
void ThreadSafeVirtualizedTextBuffer::setPageSize(size_t pageSize)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->setPageSize(pageSize);
}

// Set the cache size
void ThreadSafeVirtualizedTextBuffer::setCacheSize(size_t cacheSize)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->setCacheSize(cacheSize);
}

// Get the current page size
size_t ThreadSafeVirtualizedTextBuffer::getPageSize() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_->getPageSize();
}

// Get the current cache size
size_t ThreadSafeVirtualizedTextBuffer::getCacheSize() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_->getCacheSize();
}

// Get the number of pages in memory
size_t ThreadSafeVirtualizedTextBuffer::getPagesInMemory() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_->getPagesInMemory();
}

// Get the cache hit rate
double ThreadSafeVirtualizedTextBuffer::getCacheHitRate() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_->getCacheHitRate();
}

// Reset the cache statistics
void ThreadSafeVirtualizedTextBuffer::resetCacheStats()
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->resetCacheStats();
}

// Prefetch a range of lines
void ThreadSafeVirtualizedTextBuffer::prefetchLines(size_t startLine, size_t endLine)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->prefetchLines(startLine, endLine);
}

// ITextBuffer interface implementation

void ThreadSafeVirtualizedTextBuffer::addLine(const std::string& line)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->addLine(line);
}

void ThreadSafeVirtualizedTextBuffer::insertLine(size_t index, const std::string& line)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->insertLine(index, line);
}

void ThreadSafeVirtualizedTextBuffer::deleteLine(size_t index)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->deleteLine(index);
}

void ThreadSafeVirtualizedTextBuffer::replaceLine(size_t index, const std::string& newLine)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->replaceLine(index, newLine);
}

void ThreadSafeVirtualizedTextBuffer::setLine(size_t lineIndex, const std::string& text)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->setLine(lineIndex, text);
}

void ThreadSafeVirtualizedTextBuffer::deleteLines(size_t startIndex, size_t endIndex)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->deleteLines(startIndex, endIndex);
}

void ThreadSafeVirtualizedTextBuffer::insertLines(size_t index, const std::vector<std::string>& newLines)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->insertLines(index, newLines);
}

const std::string& ThreadSafeVirtualizedTextBuffer::getLine(size_t index) const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    // We need to handle this carefully, as the reference might become invalid
    // if the buffer is modified after this method returns.
    temporaryLine_ = buffer_->getLine(index);
    return temporaryLine_;
}

std::string& ThreadSafeVirtualizedTextBuffer::getLine(size_t index)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    // We need to handle this carefully, as the reference might become invalid
    // if the buffer is modified after this method returns.
    temporaryLine_ = buffer_->getLine(index);
    return temporaryLine_;
}

size_t ThreadSafeVirtualizedTextBuffer::lineCount() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_->lineCount();
}

bool ThreadSafeVirtualizedTextBuffer::isEmpty() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_->isEmpty();
}

size_t ThreadSafeVirtualizedTextBuffer::lineLength(size_t lineIndex) const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_->lineLength(lineIndex);
}

size_t ThreadSafeVirtualizedTextBuffer::characterCount() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_->characterCount();
}

std::vector<std::string> ThreadSafeVirtualizedTextBuffer::getAllLines() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_->getAllLines();
}

bool ThreadSafeVirtualizedTextBuffer::isValidPosition(size_t lineIndex, size_t colIndex) const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_->isValidPosition(lineIndex, colIndex);
}

std::pair<size_t, size_t> ThreadSafeVirtualizedTextBuffer::clampPosition(size_t lineIndex, size_t colIndex) const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_->clampPosition(lineIndex, colIndex);
}

void ThreadSafeVirtualizedTextBuffer::printToStream(std::ostream& os) const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    buffer_->printToStream(os);
}

bool ThreadSafeVirtualizedTextBuffer::saveToFile(const std::string& filename) const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_->saveToFile(filename);
}

bool ThreadSafeVirtualizedTextBuffer::loadFromFile(const std::string& filename)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    return buffer_->loadFromFile(filename);
}

void ThreadSafeVirtualizedTextBuffer::insertChar(size_t lineIndex, size_t colIndex, char ch)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->insertChar(lineIndex, colIndex, ch);
}

void ThreadSafeVirtualizedTextBuffer::deleteChar(size_t lineIndex, size_t colIndex)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->deleteChar(lineIndex, colIndex);
}

void ThreadSafeVirtualizedTextBuffer::deleteCharForward(size_t lineIndex, size_t colIndex)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->deleteCharForward(lineIndex, colIndex);
}

void ThreadSafeVirtualizedTextBuffer::replaceLineSegment(size_t lineIndex, size_t startCol, size_t endCol, const std::string& newText)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->replaceLineSegment(lineIndex, startCol, endCol, newText);
}

void ThreadSafeVirtualizedTextBuffer::deleteLineSegment(size_t lineIndex, size_t startCol, size_t endCol)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->deleteLineSegment(lineIndex, startCol, endCol);
}

void ThreadSafeVirtualizedTextBuffer::splitLine(size_t lineIndex, size_t colIndex)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->splitLine(lineIndex, colIndex);
}

void ThreadSafeVirtualizedTextBuffer::joinLines(size_t lineIndex)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->joinLines(lineIndex);
}

void ThreadSafeVirtualizedTextBuffer::clear(bool keepEmptyLine)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->clear(keepEmptyLine);
}

void ThreadSafeVirtualizedTextBuffer::insertString(size_t lineIndex, size_t colIndex, const std::string& text)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->insertString(lineIndex, colIndex, text);
}

std::string ThreadSafeVirtualizedTextBuffer::getLineSegment(size_t lineIndex, size_t startCol, size_t endCol) const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_->getLineSegment(lineIndex, startCol, endCol);
}

size_t ThreadSafeVirtualizedTextBuffer::getLineCount() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_->getLineCount();
}

std::vector<std::string> ThreadSafeVirtualizedTextBuffer::getLines() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_->getLines();
}

void ThreadSafeVirtualizedTextBuffer::replaceText(size_t startLine, size_t startCol, size_t endLine, size_t endCol, const std::string& text)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->replaceText(startLine, startCol, endLine, endCol, text);
}

void ThreadSafeVirtualizedTextBuffer::insertText(size_t line, size_t col, const std::string& text)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->insertText(line, col, text);
}

void ThreadSafeVirtualizedTextBuffer::deleteText(size_t startLine, size_t startCol, size_t endLine, size_t endCol)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->deleteText(startLine, startCol, endLine, endCol);
}

bool ThreadSafeVirtualizedTextBuffer::isModified() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_->isModified();
}

void ThreadSafeVirtualizedTextBuffer::setModified(bool modified)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->setModified(modified);
} 