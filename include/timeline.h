#ifndef CHRONOVYAN_TIMELINE_H
#define CHRONOVYAN_TIMELINE_H

namespace chronovyan {

/**
 * @class Timeline
 * @brief Represents a timeline in the Chronovyan system that can be manipulated and compressed
 */
class Timeline {
public:
    /**
     * @brief Default constructor
     */
    Timeline() : m_length(500), m_compressionRatio(1.0) {}
    
    /**
     * @brief Get the current length of the timeline
     * @return The timeline length in chronons
     */
    int getLength() const { return m_length; }
    
    /**
     * @brief Set the length of the timeline
     * @param length New length in chronons
     */
    void setLength(int length) { m_length = length; }
    
    /**
     * @brief Get the current compression ratio
     * @return The compression ratio (1.0 means no compression)
     */
    double getCompressionRatio() const { return m_compressionRatio; }
    
    /**
     * @brief Set the compression ratio
     * @param ratio New compression ratio
     */
    void setCompressionRatio(double ratio) { m_compressionRatio = ratio; }
    
private:
    int m_length;               ///< The length of the timeline in chronons
    double m_compressionRatio;  ///< The compression ratio applied to the timeline
};

} // namespace chronovyan

#endif // CHRONOVYAN_TIMELINE_H 