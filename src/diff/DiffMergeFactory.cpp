#include "DiffMergeFactory.h"
#include "MyersDiff.h"
#include "MergeEngine.h"
#include "AppDebugLog.h"

IDiffEnginePtr DiffMergeFactory::createDiffEngine() {
    LOG_DEBUG("Creating new diff engine");
    return std::make_shared<MyersDiff>();
}

IMergeEnginePtr DiffMergeFactory::createMergeEngine(IDiffEnginePtr diffEngine) {
    if (!diffEngine) {
        diffEngine = createDiffEngine();
    }
    
    LOG_DEBUG("Creating new merge engine");
    return std::make_shared<MergeEngine>(diffEngine);
} 