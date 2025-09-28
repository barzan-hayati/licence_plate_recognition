#include "../include/my_libraries/queue_manager.hpp"

QueueManager::QueueManager() {}

QueueManager::QueueManager(char* queue_name) {
    queue = gst_element_factory_make("queue", queue_name);
}

QueueManager::~QueueManager() {
    // Cleanup code (if needed)
}