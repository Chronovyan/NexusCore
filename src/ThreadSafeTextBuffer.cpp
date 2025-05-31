#include "ThreadSafeTextBuffer.h"
#include <stdexcept>

ThreadSafeTextBuffer::ThreadSafeTextBuffer(std::shared_ptr<TextBuffer> buffer)
    : buffer_(buffer ? buffer : std::make_shared<TextBuffer>()) {
}

std::shared_ptr<TextBuffer> ThreadSafeTextBuffer::getUnderlyingBuffer() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_;
}

void ThreadSafeTextBuffer::lockForReading() const {
    mutex_.lock_shared();
}

void ThreadSafeTextBuffer::unlockReading() const {
    mutex_.unlock_shared();
}

void ThreadSafeTextBuffer::lockForWriting() {
    mutex_.lock();
}

void ThreadSafeTextBuffer::unlockWriting() {
    mutex_.unlock();
}

// Read operations with shared locks
const std::string& ThreadSafeTextBuffer::getLine(size_t index) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_->getLine(index);
}

size_t ThreadSafeTextBuffer::lineCount() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_->lineCount();
}

bool ThreadSafeTextBuffer::isEmpty() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_->isEmpty();
}

size_t ThreadSafeTextBuffer::lineLength(size_t lineIndex) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_->lineLength(lineIndex);
}

size_t ThreadSafeTextBuffer::characterCount() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_->characterCount();
}

std::vector<std::string> ThreadSafeTextBuffer::getAllLines() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_->getAllLines();
}

bool ThreadSafeTextBuffer::isValidPosition(size_t lineIndex, size_t colIndex) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_->isValidPosition(lineIndex, colIndex);
}

std::pair<size_t, size_t> ThreadSafeTextBuffer::clampPosition(size_t lineIndex, size_t colIndex) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_->clampPosition(lineIndex, colIndex);
}

void ThreadSafeTextBuffer::printToStream(std::ostream& os) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    buffer_->printToStream(os);
}

std::string ThreadSafeTextBuffer::getLineSegment(size_t lineIndex, size_t startCol, size_t endCol) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_->getLineSegment(lineIndex, startCol, endCol);
}

size_t ThreadSafeTextBuffer::getLineCount() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_->getLineCount();
}

std::vector<std::string> ThreadSafeTextBuffer::getLines() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return buffer_->getLines();
}

bool ThreadSafeTextBuffer::isModified() const {
    return modified_.load(std::memory_order_acquire);
}

// Write operations with exclusive locks
void ThreadSafeTextBuffer::addLine(const std::string& line) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->addLine(line);
    modified_.store(true, std::memory_order_release);
}

void ThreadSafeTextBuffer::insertLine(size_t index, const std::string& line) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->insertLine(index, line);
    modified_.store(true, std::memory_order_release);
}

void ThreadSafeTextBuffer::deleteLine(size_t index) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->deleteLine(index);
    modified_.store(true, std::memory_order_release);
}

void ThreadSafeTextBuffer::replaceLine(size_t index, const std::string& newLine) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->replaceLine(index, newLine);
    modified_.store(true, std::memory_order_release);
}

void ThreadSafeTextBuffer::setLine(size_t lineIndex, const std::string& text) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->setLine(lineIndex, text);
    modified_.store(true, std::memory_order_release);
}

void ThreadSafeTextBuffer::deleteLines(size_t startIndex, size_t endIndex) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->deleteLines(startIndex, endIndex);
    modified_.store(true, std::memory_order_release);
}

void ThreadSafeTextBuffer::insertLines(size_t index, const std::vector<std::string>& newLines) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->insertLines(index, newLines);
    modified_.store(true, std::memory_order_release);
}

bool ThreadSafeTextBuffer::saveToFile(const std::string& filename) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    bool result = buffer_->saveToFile(filename);
    if (result) {
        modified_.store(false, std::memory_order_release);
    }
    return result;
}

bool ThreadSafeTextBuffer::loadFromFile(const std::string& filename) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    bool result = buffer_->loadFromFile(filename);
    if (result) {
        modified_.store(false, std::memory_order_release);
    }
    return result;
}

void ThreadSafeTextBuffer::insertChar(size_t lineIndex, size_t colIndex, char ch) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->insertChar(lineIndex, colIndex, ch);
    modified_.store(true, std::memory_order_release);
}

void ThreadSafeTextBuffer::deleteChar(size_t lineIndex, size_t colIndex) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->deleteChar(lineIndex, colIndex);
    modified_.store(true, std::memory_order_release);
}

void ThreadSafeTextBuffer::deleteCharForward(size_t lineIndex, size_t colIndex) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->deleteCharForward(lineIndex, colIndex);
    modified_.store(true, std::memory_order_release);
}

void ThreadSafeTextBuffer::replaceLineSegment(size_t lineIndex, size_t startCol, size_t endCol, const std::string& newText) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->replaceLineSegment(lineIndex, startCol, endCol, newText);
    modified_.store(true, std::memory_order_release);
}

void ThreadSafeTextBuffer::deleteLineSegment(size_t lineIndex, size_t startCol, size_t endCol) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->deleteLineSegment(lineIndex, startCol, endCol);
    modified_.store(true, std::memory_order_release);
}

void ThreadSafeTextBuffer::splitLine(size_t lineIndex, size_t colIndex) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->splitLine(lineIndex, colIndex);
    modified_.store(true, std::memory_order_release);
}

void ThreadSafeTextBuffer::joinLines(size_t lineIndex) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->joinLines(lineIndex);
    modified_.store(true, std::memory_order_release);
}

void ThreadSafeTextBuffer::clear(bool keepEmptyLine) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->clear(keepEmptyLine);
    modified_.store(true, std::memory_order_release);
}

void ThreadSafeTextBuffer::insertString(size_t lineIndex, size_t colIndex, const std::string& text) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->insertString(lineIndex, colIndex, text);
    modified_.store(true, std::memory_order_release);
}

void ThreadSafeTextBuffer::replaceText(size_t startLine, size_t startCol, size_t endLine, size_t endCol, const std::string& text) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->replaceText(startLine, startCol, endLine, endCol, text);
    modified_.store(true, std::memory_order_release);
}

void ThreadSafeTextBuffer::insertText(size_t line, size_t col, const std::string& text) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->insertText(line, col, text);
    modified_.store(true, std::memory_order_release);
}

void ThreadSafeTextBuffer::deleteText(size_t startLine, size_t startCol, size_t endLine, size_t endCol) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    buffer_->deleteText(startLine, startCol, endLine, endCol);
    modified_.store(true, std::memory_order_release);
}

void ThreadSafeTextBuffer::setModified(bool modified) {
    modified_.store(modified, std::memory_order_release);
}

// This method is more complex because it returns a non-const reference
// We need to return a copy to avoid thread safety issues
std::string& ThreadSafeTextBuffer::getLine(size_t index) {
    // Note: This implementation is problematic for thread safety since we return a reference
    // In a truly thread-safe implementation, we would need to return a proxy object or
    // require explicit locking by the caller.
    // For now, we use a unique lock to ensure exclusivity
    std::unique_lock<std::shared_mutex> lock(mutex_);
    return buffer_->getLine(index);
} 