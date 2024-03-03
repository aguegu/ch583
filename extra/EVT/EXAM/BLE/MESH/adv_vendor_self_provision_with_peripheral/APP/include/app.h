/********************************** (C) COPYRIGHT *******************************
 * File Name          : app.h
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2022/03/31
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef app_H
#define app_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/

#define APP_NODE_EVT                       (1 << 0)
#define APP_NODE_PROVISION_EVT             (1 << 1)
#define APP_DELETE_NODE_TIMEOUT_EVT        (1 << 2)
#define APP_DELETE_LOCAL_NODE_EVT          (1 << 3)
#define APP_DELETE_NODE_INFO_EVT           (1 << 4)
#define APP_ASK_STATUS_NODE_TIMEOUT_EVT    (1 << 5)
#define APP_OTA_UPDATE_TIMEOUT_EVT         (1 << 6)
#define APP_SET_SUB_TIMEOUT_EVT            (1 << 7)

#define CMD_PROVISION_INFO                 0xA0
#define CMD_PROVISION_INFO_ACK             0x80
#define CMD_PROVISION                      0xA1
#define CMD_PROVISION_ACK                  0x81
#define CMD_DELETE_NODE                    0xA2
#define CMD_DELETE_NODE_ACK                0x82
#define CMD_DELETE_NODE_INFO               0xA3
#define CMD_DELETE_NODE_INFO_ACK           0x83
#define CMD_ASK_STATUS                     0xA4
#define CMD_ASK_STATUS_ACK                 0x84
#define CMD_TRANSFER                       0xA5
#define CMD_TRANSFER_RECEIVE               0x85
#define CMD_IMAGE_INFO                     0xA6
#define CMD_IMAGE_INFO_ACK                 0x86
#define CMD_UPDATE                         0xA7
#define CMD_UPDATE_ACK                     0x87
#define CMD_VERIFY                         0xA8
#define CMD_VERIFY_ACK                     0x88
#define CMD_END                            0xA9
#define CMD_SET_SUB                        0xAA
#define CMD_SET_SUB_ACK                    0x8A
#define CMD_LOCAL_RESET                    0xAF
#define CMD_LOCAL_RESET_ACK                0x8F

#define PERIPHERAL_CMD_LEN                 1
#define PROVISION_NET_KEY_LEN              16
#define ADDRESS_LEN                        2
#define UPDATE_ADDRESS_LEN                 2

// 设置配网信息命令，包含 1字节命令码+1字节控制字+4字节iv index+1字节更新标志flag
#define PROVISION_INFO_DATA_LEN            (PERIPHERAL_CMD_LEN + 1 + 4 + 1)
// 设置配网信息命令应答，包含 1字节命令码+1字节状态码+4字节iv index+1字节更新标志flag
#define PROVISION_INFO_ACK_DATA_LEN        (PERIPHERAL_CMD_LEN + 1 + 4 + 1)
// 设置配网命令，包含 1字节命令码+16字节网络密钥+2字节网络地址
#define PROVISION_DATA_LEN                 (PERIPHERAL_CMD_LEN + PROVISION_NET_KEY_LEN + ADDRESS_LEN)
// 设置配网命令应答，包含 1字节命令码+2字节网络地址+1字节状态码
#define PROVISION_ACK_DATA_LEN             (PERIPHERAL_CMD_LEN + ADDRESS_LEN + 1)
// 删除节点命令，包含 1字节命令码+2字节需要删除的节点地址
#define DELETE_NODE_DATA_LEN               (PERIPHERAL_CMD_LEN + ADDRESS_LEN)
// 删除节点命令应答，包含 1字节命令码+2字节删除的节点地址+1字节状态码
#define DELETE_NODE_ACK_DATA_LEN           (PERIPHERAL_CMD_LEN + ADDRESS_LEN + 1)
// 删除存储的节点信息命令，包含 1字节命令码
#define DELETE_NODE_INFO_DATA_LEN          (PERIPHERAL_CMD_LEN)
// 删除存储的节点信息命令应答，包含 1字节命令码+2字节删除的节点地址
#define DELETE_NODE_INFO_ACK_DATA_LEN      (PERIPHERAL_CMD_LEN + ADDRESS_LEN)
// 查询节点状态命令，包含 1字节命令码+2字节网络地址
#define ASK_STATUS_DATA_LEN                (PERIPHERAL_CMD_LEN + ADDRESS_LEN)
// 查询节点状态命令应答，包含 1字节命令码+2字节网络地址+1字节状态码
#define ASK_STATUS_ACK_DATA_LEN            (PERIPHERAL_CMD_LEN + ADDRESS_LEN + 1)
// 数据传输命令，包含 1字节命令码+2字节网络地址+N字节内容
#define TRANSFER_DATA_LEN                  (PERIPHERAL_CMD_LEN + ADDRESS_LEN)
// 数据传输命令应答，包含 1字节命令码+2字节网络地址+N字节内容
#define TRANSFER_RECEIVE_DATA_LEN          (PERIPHERAL_CMD_LEN + ADDRESS_LEN)
// OTA查询命令，包含 1字节命令码+2字节网络地址
#define IMAGE_INFO_DATA_LEN                (PERIPHERAL_CMD_LEN + ADDRESS_LEN)
// OTA查询命令应答，包含 1字节命令码+2字节网络地址+4字节image大小+2字节块大小+2字节芯片型号+1字节状态码
#define IMAGE_INFO_ACK_DATA_LEN            (PERIPHERAL_CMD_LEN + ADDRESS_LEN + 4 + 2 + 2 + 1)
// OTA升级命令，包含 1字节命令码+2字节网络地址+2字节flash地址+N字节内容
#define UPDATE_DATA_LEN                    (PERIPHERAL_CMD_LEN + ADDRESS_LEN + UPDATE_ADDRESS_LEN)
// OTA升级命令应答，包含 1字节命令码+2字节网络地址+2字节flash地址+1字节状态码
#define UPDATE_ACK_DATA_LEN                (PERIPHERAL_CMD_LEN + ADDRESS_LEN + UPDATE_ADDRESS_LEN + 1)
// OTA校验命令，包含 1字节命令码+2字节网络地址+2字节flash地址+N字节内容
#define VERIFY_DATA_LEN                    (PERIPHERAL_CMD_LEN + ADDRESS_LEN + UPDATE_ADDRESS_LEN)
// OTA校验命令应答，包含 1字节命令码+2字节网络地址+2字节flash地址+1字节状态码
#define VERIFY_ACK_DATA_LEN                (PERIPHERAL_CMD_LEN + ADDRESS_LEN + UPDATE_ADDRESS_LEN + 1)
// OTA结束命令，包含 1字节命令码+2字节网络地址
#define END_DATA_LEN                       (PERIPHERAL_CMD_LEN + ADDRESS_LEN)
// 设置订阅命令，包含 1字节命令码+2字节网络地址+1字节控制字+2字节网络地址
#define SET_SUB_DATA_LEN                   (PERIPHERAL_CMD_LEN + ADDRESS_LEN + 1 + ADDRESS_LEN)
// 设置订阅命令应答，包含 1字节命令码+2字节网络地址+1字节状态码
#define SET_SUB_ACK_DATA_LEN               (PERIPHERAL_CMD_LEN + ADDRESS_LEN + 1)
// 本地复位命令，包含 1字节命令码
#define LOCAL_RESET_DATA_LEN               (PERIPHERAL_CMD_LEN)
// 本地复位命令，包含 1字节命令码+1字节状态码
#define LOCAL_RESET_ACK_DATA_LEN           (PERIPHERAL_CMD_LEN + 1)

// 状态码定义
#define STATUS_SUCCESS                     0x00
#define STATUS_TIMEOUT                     0x01
#define STATUS_NOMEM                       0x02
#define STATUS_INVALID                     0x03

#define APP_MAX_TX_SIZE                    MAX(CONFIG_MESH_UNSEG_LENGTH_DEF, CONFIG_MESH_TX_SEG_DEF *BLE_MESH_APP_SEG_SDU_MAX - 8)

/* 整个用户code区分成五块，4K，152K，152K，4K，136K，后四块下面分别叫做imageA（APP），imageB（OTA），imageIAP和LIB */

/* FLASH定义 */
#define FLASH_BLOCK_SIZE                   EEPROM_BLOCK_SIZE
#define IMAGE_SIZE                         152 * 1024

/* imageA定义 */
#define IMAGE_A_FLAG                       0x01
#define IMAGE_A_START_ADD                  0x1000
#define IMAGE_A_SIZE                       IMAGE_SIZE

/* imageB定义 */
#define IMAGE_B_FLAG                       0x02
#define IMAGE_B_START_ADD                  (IMAGE_A_START_ADD + IMAGE_SIZE)
#define IMAGE_B_SIZE                       IMAGE_SIZE

/* imageIAP定义 */
#define IMAGE_IAP_FLAG                     0x03
#define IMAGE_IAP_START_ADD                (IMAGE_B_START_ADD + IMAGE_SIZE)
#define IMAGE_IAP_SIZE                     4 * 1024

/* 存放在DataFlash地址，不能占用蓝牙的位置 */
#define OTA_DATAFLASH_ADD                  0x00077000 - FLASH_ROM_MAX_SIZE

/* 存放在DataFlash里的OTA信息 */
typedef struct
{
    unsigned char ImageFlag; //记录的当前的image标志
    unsigned char Revd[3];
} OTADataFlashInfo_t;

/******************************************************************************/

typedef struct
{
    uint16_t node_addr;
    uint16_t elem_count;
    uint16_t net_idx;
    uint16_t retry_cnt : 12,
        fixed : 1,
        blocked : 1;

} node_t;

typedef union
{
    struct
    {
        uint8_t cmd;         /* 命令码 CMD_PROVISION_INFO */
        uint8_t set_flag;    /* 控制字 为1表示设置，为0表示查询*/
        uint8_t iv_index[4]; /* iv index */
        uint8_t flag;        /* Net key refresh flag */
    } provision_info;        /* 配网信息命令 */
    struct
    {
        uint8_t cmd;         /* 命令码 CMD_PROVISION_INFO_ACK */
        uint8_t status;      /* 状态码*/
        uint8_t iv_index[4]; /* iv index */
        uint8_t flag;        /* Net key refresh flag */
    } provision_info_ack;    /* 配网信息命令应答 */
    struct
    {
        uint8_t cmd;                            /* 命令码 CMD_PROVISION */
        uint8_t net_key[PROVISION_NET_KEY_LEN]; /* 后续数据长度 */
        uint8_t addr[ADDRESS_LEN];              /* 配网地址 */
    } provision;                                /* 配网命令 */
    struct
    {
        uint8_t cmd;               /* 命令码 CMD_PROVISION_ACK */
        uint8_t addr[ADDRESS_LEN]; /* 配网地址 */
        uint8_t status;            /* 状态码备用 */
    } provision_ack;               /* 配网命令应答 */
    struct
    {
        uint8_t cmd;               /* 命令码 CMD_DELETE_NODE */
        uint8_t addr[ADDRESS_LEN]; /* 删除地址 */
    } delete_node;                 /* 删除节点命令 */
    struct
    {
        uint8_t cmd;               /* 命令码 CMD_DELETE_NODE_ACK */
        uint8_t addr[ADDRESS_LEN]; /* 发送地址 */
        uint8_t status;            /* 状态码 */
    } delete_node_ack;             /* 删除节点命令应答 */
    struct
    {
        uint8_t cmd;    /* 命令码 CMD_DELETE_NODE_INFO */
    } delete_node_info; /* 删除存储的节点信息命令 */
    struct
    {
        uint8_t cmd;               /* 命令码 CMD_DELETE_NODE_INFO_ACK */
        uint8_t addr[ADDRESS_LEN]; /* 发送地址 */
    } delete_node_info_ack;        /* 删除存储的节点信息命令应答 */
    struct
    {
        uint8_t cmd;               /* 命令码 CMD_ASK_STATUS */
        uint8_t addr[ADDRESS_LEN]; /* 发送地址 */
    } ask_status;                  /* 查询节点状态命令 */
    struct
    {
        uint8_t cmd;               /* 命令码 CMD_ASK_STATUS_ACK */
        uint8_t addr[ADDRESS_LEN]; /* 发送地址 */
        uint8_t status;            /* 状态码备用 */
    } ask_status_ack;              /* 查询节点状态命令应答 */
    struct
    {
        uint8_t cmd;                       /* 命令码 CMD_TRANSFER */
        uint8_t addr[ADDRESS_LEN];         /* 发送地址 */
        uint8_t data[APP_MAX_TX_SIZE - 3]; /* 数据内容*/
    } transfer;                            /* 发送数据命令 */
    struct
    {
        uint8_t cmd;                       /* 命令码 CMD_TRANSFER_ACK */
        uint8_t addr[ADDRESS_LEN];         /* 发送地址 */
        uint8_t data[APP_MAX_TX_SIZE - 3]; /* 数据内容*/
    } transfer_receive;                    /* 发送数据命令应答 */
    struct
    {
        uint8_t cmd;               /* 命令码 CMD_IMAGE_INFO */
        uint8_t addr[ADDRESS_LEN]; /* 发送地址 */
    } image_info;                  /* OTA查询命令 */
    struct
    {
        uint8_t cmd;               /* 命令码 CMD_IMAGE_INFO_ACK */
        uint8_t addr[ADDRESS_LEN]; /* 发送地址 */
        uint8_t image_size[4];     /* image大小 */
        uint8_t block_size[2];     /* falsh块大小 */
        uint8_t chip_id[2];        /* 芯片型号 */
        uint8_t status;            /* 状态码备用 */
    } image_info_ack;              /* OTA查询命令应答 */
    struct
    {
        uint8_t cmd;                             /* 命令码 CMD_UPDATE */
        uint8_t addr[ADDRESS_LEN];               /* 发送地址 */
        uint8_t update_addr[UPDATE_ADDRESS_LEN]; /* 升级地址 */
        uint8_t data[APP_MAX_TX_SIZE - 5];       /* 升级数据内容*/
    } update;                                    /* OTA升级数据命令 */
    struct
    {
        uint8_t cmd;                             /* 命令码 CMD_UPDATE_ACK */
        uint8_t addr[ADDRESS_LEN];               /* 发送地址 */
        uint8_t update_addr[UPDATE_ADDRESS_LEN]; /* 升级地址 */
        uint8_t status;                          /* 状态码备用 */
    } update_ack;                                /* OTA升级数据命令应答 */
    struct
    {
        uint8_t cmd;                             /* 命令码 CMD_VERIFY */
        uint8_t addr[ADDRESS_LEN];               /* 发送地址 */
        uint8_t update_addr[UPDATE_ADDRESS_LEN]; /* 升级地址 */
        uint8_t data[APP_MAX_TX_SIZE - 5];       /* 升级数据内容*/
    } verify;                                    /* OTA验证数据命令 */
    struct
    {
        uint8_t cmd;                             /* 命令码 CMD_VERIFY_ACK */
        uint8_t addr[ADDRESS_LEN];               /* 发送地址 */
        uint8_t update_addr[UPDATE_ADDRESS_LEN]; /* 升级地址 */
        uint8_t status;                          /* 状态码备用 */
    } verify_ack;                                /* OTA验证数据命令应答 */
    struct
    {
        uint8_t cmd;               /* 命令码 CMD_END */
        uint8_t addr[ADDRESS_LEN]; /* 发送地址 */
    } end;                         /* OTA完成命令 */
    struct
    {
        uint8_t cmd;                   /* 命令码 CMD_SET_SUB */
        uint8_t addr[ADDRESS_LEN];     /* 发送地址 */
        uint8_t add_flag;              /* 为1表示添加，为0表示删除 */
        uint8_t sub_addr[ADDRESS_LEN]; /* 订阅地址 */
    } set_sub;                         /* 设置订阅命令 */
    struct
    {
        uint8_t cmd;               /* 命令码 CMD_SET_SUB_ACK */
        uint8_t addr[ADDRESS_LEN]; /* 发送地址 */
        uint8_t status;            /* 状态码 */
    } set_sub_ack;                 /* 设置订阅命令应答 */
    struct
    {
        uint8_t cmd; /* 命令码 CMD_LOCAL_RESET */
    } local_reset;   /* 本地恢复出厂设置命令 */
    struct
    {
        uint8_t cmd;    /* 命令码 CMD_LOCAL_RESET */
        uint8_t status; /* 状态码备用 */
    } local_reset_ack;  /* 本地恢复出厂设置命令应答 */
    struct
    {
        uint8_t buf[APP_MAX_TX_SIZE]; /* 数据内容*/
    } data;
} app_mesh_manage_t;

void App_Init(void);

void App_peripheral_reveived(uint8_t *pValue, uint16_t len);

/******************************************************************************/

/******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif
