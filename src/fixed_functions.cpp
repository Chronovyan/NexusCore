// Fixed implementation of the problematic functions

CacheEvictionPolicy VirtualizedTextBuffer::getCacheEvictionPolicy() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return evictionPolicy_;
}

PrefetchStrategy VirtualizedTextBuffer::getPrefetchStrategy() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return prefetchStrategy_;
} 