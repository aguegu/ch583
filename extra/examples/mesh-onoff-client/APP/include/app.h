#ifndef app_H
#define app_H

#ifdef __cplusplus
extern "C" {
#endif

#define APP_RESET_MESH_EVENT (1 << 0)
#define APP_BUTTON_POLL_EVENT (1 << 1)

void App_Init(void);

#ifdef __cplusplus
}
#endif

#endif
