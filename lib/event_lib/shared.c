#include "shared.h"

// Khai báo Event Group
EventGroupHandle_t shared_event_group = NULL;
// Khai báo Event Group
EventGroupHandle_t event_group = NULL;
// Hàm khởi tạo Event Group
void shared_event_group_init(void) {
    if (shared_event_group == NULL) {
        shared_event_group = xEventGroupCreate();
    }
    // Khởi tạo Event Group
    if (event_group == NULL) {
        event_group = xEventGroupCreate();
    }
}
