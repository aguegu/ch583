#ifndef app_H
#define app_H

#ifdef __cplusplus
extern "C" {
#endif

#define APP_RESET_MESH_EVENT       (1 << 0)

typedef union {
  struct {
    uint8_t buf[20]; /* 接收数据包*/
  } data;
} app_mesh_manage_t;

void App_Init(void);


#ifdef __cplusplus
}
#endif

#endif
