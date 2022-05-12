
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : App/app_zigbee.c
  * Description        : Zigbee Application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019-2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  * code partially wirten by Ruben Middelman.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_common.h"
#include "app_entry.h"
#include "dbg_trace.h"
#include "app_zigbee.h"
#include "zigbee_interface.h"
#include "shci.h"
#include "stm_logging.h"
#include "app_conf.h"
#include "stm32wbxx_core_interface_def.h"
#include "zigbee_types.h"
#include "stm32_seq.h"

/* Private includes -----------------------------------------------------------*/
#include <assert.h>
#include "zcl/zcl.h"
#include "zcl/general/zcl.onoff.h"
#include "zcl/general/zcl.level.h"

/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private defines -----------------------------------------------------------*/
#define APP_ZIGBEE_STARTUP_FAIL_DELAY               500U
#define CHANNEL                                     11

#define SW1_ENDPOINT                                17
#define SW2_ENDPOINT                                20
#define SW3_ENDPOINT                                21
#define SW4_ENDPOINT                                22
#define SW5_ENDPOINT                                23
#define SW6_ENDPOINT                                24

/* USER CODE BEGIN PD */
#define SW1_GROUP_ADDR          0x0001
/* USER CODE END PD */

/* Private macros ------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* External definition -------------------------------------------------------*/
enum ZbStatusCodeT ZbStartupWait(struct ZigBeeT *zb, struct ZbStartupT *config);

/* USER CODE BEGIN ED */
extern uint8_t brightness;
extern uint8_t color_R, color_G, color_B;
extern uint8_t time_Level;

extern bool rainbow_Bool;
extern bool LED_bool;
extern bool breathe_Bool;
extern bool blink_Bool;

/* USER CODE END ED */

/* Private function prototypes -----------------------------------------------*/
static void APP_ZIGBEE_StackLayersInit(void);
static void APP_ZIGBEE_ConfigEndpoints(void);
static void APP_ZIGBEE_NwkForm(void);

static void APP_ZIGBEE_TraceError(const char *pMess, uint32_t ErrCode);
static void APP_ZIGBEE_CheckWirelessFirmwareInfo(void);

static void Wait_Getting_Ack_From_M0(void);
static void Receive_Ack_From_M0(void);
static void Receive_Notification_From_M0(void);

/* USER CODE BEGIN PFP */
static void APP_ZIGBEE_ConfigGroupAddr(void);
/* USER CODE END PFP */

/* Private variables ---------------------------------------------------------*/
static TL_CmdPacket_t *p_ZIGBEE_otcmdbuffer;
static TL_EvtPacket_t *p_ZIGBEE_notif_M0_to_M4;
static TL_EvtPacket_t *p_ZIGBEE_request_M0_to_M4;
static __IO uint32_t CptReceiveNotifyFromM0 = 0;
static __IO uint32_t CptReceiveRequestFromM0 = 0;

PLACE_IN_SECTION("MB_MEM1") ALIGN(4) static TL_ZIGBEE_Config_t ZigbeeConfigBuffer;
PLACE_IN_SECTION("MB_MEM2") ALIGN(4) static TL_CmdPacket_t ZigbeeOtCmdBuffer;
PLACE_IN_SECTION("MB_MEM2") ALIGN(4) static uint8_t ZigbeeNotifRspEvtBuffer[sizeof(TL_PacketHeader_t) + TL_EVT_HDR_SIZE + 255U];
PLACE_IN_SECTION("MB_MEM2") ALIGN(4) static uint8_t ZigbeeNotifRequestBuffer[sizeof(TL_PacketHeader_t) + TL_EVT_HDR_SIZE + 255U];

struct zigbee_app_info {
  bool has_init;
  struct ZigBeeT *zb;
  enum ZbStartType startupControl;
  enum ZbStatusCodeT join_status;
  uint32_t join_delay;
  bool init_after_join;

  struct ZbZclClusterT *onOff_server_1;
  struct ZbZclClusterT *levelControl_server_1;
  struct ZbZclClusterT *onOff_server_2;
  struct ZbZclClusterT *levelControl_server_2;
  struct ZbZclClusterT *onOff_server_3;
  struct ZbZclClusterT *levelControl_server_3;
  struct ZbZclClusterT *onOff_server_4;
  struct ZbZclClusterT *levelControl_server_4;
  struct ZbZclClusterT *onOff_server_5;
  struct ZbZclClusterT *levelControl_server_5;
  struct ZbZclClusterT *onOff_server_6;
  struct ZbZclClusterT *levelControl_server_6;
};
static struct zigbee_app_info zigbee_app_info;

/* OnOff server 1 custom callbacks */
static enum ZclStatusCodeT onOff_server_1_off(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT onOff_server_1_on(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT onOff_server_1_toggle(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg);

static struct ZbZclOnOffServerCallbacksT OnOffServerCallbacks_1 = {
  .off = onOff_server_1_off,
  .on = onOff_server_1_on,
  .toggle = onOff_server_1_toggle,
};

/* LevelControl server 1 custom callbacks */
static enum ZclStatusCodeT levelControl_server_1_move_to_level(struct ZbZclClusterT *cluster, struct ZbZclLevelClientMoveToLevelReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT levelControl_server_1_move(struct ZbZclClusterT *cluster, struct ZbZclLevelClientMoveReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT levelControl_server_1_step(struct ZbZclClusterT *cluster, struct ZbZclLevelClientStepReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT levelControl_server_1_stop(struct ZbZclClusterT *cluster, struct ZbZclLevelClientStopReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg);

static struct ZbZclLevelServerCallbacksT LevelServerCallbacks_1 = {
  .move_to_level = levelControl_server_1_move_to_level,
  .move = levelControl_server_1_move,
  .step = levelControl_server_1_step,
  .stop = levelControl_server_1_stop,
};

/* OnOff server 2 custom callbacks */
static enum ZclStatusCodeT onOff_server_2_off(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT onOff_server_2_on(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT onOff_server_2_toggle(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg);

static struct ZbZclOnOffServerCallbacksT OnOffServerCallbacks_2 = {
  .off = onOff_server_2_off,
  .on = onOff_server_2_on,
  .toggle = onOff_server_2_toggle,
};

/* LevelControl server 2 custom callbacks */
static enum ZclStatusCodeT levelControl_server_2_move_to_level(struct ZbZclClusterT *cluster, struct ZbZclLevelClientMoveToLevelReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT levelControl_server_2_move(struct ZbZclClusterT *cluster, struct ZbZclLevelClientMoveReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT levelControl_server_2_step(struct ZbZclClusterT *cluster, struct ZbZclLevelClientStepReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT levelControl_server_2_stop(struct ZbZclClusterT *cluster, struct ZbZclLevelClientStopReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg);

static struct ZbZclLevelServerCallbacksT LevelServerCallbacks_2 = {
  .move_to_level = levelControl_server_2_move_to_level,
  .move = levelControl_server_2_move,
  .step = levelControl_server_2_step,
  .stop = levelControl_server_2_stop,
};

/* OnOff server 3 custom callbacks */
static enum ZclStatusCodeT onOff_server_3_off(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT onOff_server_3_on(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT onOff_server_3_toggle(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg);

static struct ZbZclOnOffServerCallbacksT OnOffServerCallbacks_3 = {
  .off = onOff_server_3_off,
  .on = onOff_server_3_on,
  .toggle = onOff_server_3_toggle,
};

/* LevelControl server 3 custom callbacks */
static enum ZclStatusCodeT levelControl_server_3_move_to_level(struct ZbZclClusterT *cluster, struct ZbZclLevelClientMoveToLevelReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT levelControl_server_3_move(struct ZbZclClusterT *cluster, struct ZbZclLevelClientMoveReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT levelControl_server_3_step(struct ZbZclClusterT *cluster, struct ZbZclLevelClientStepReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT levelControl_server_3_stop(struct ZbZclClusterT *cluster, struct ZbZclLevelClientStopReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg);

static struct ZbZclLevelServerCallbacksT LevelServerCallbacks_3 = {
  .move_to_level = levelControl_server_3_move_to_level,
  .move = levelControl_server_3_move,
  .step = levelControl_server_3_step,
  .stop = levelControl_server_3_stop,
};

/* OnOff server 4 custom callbacks */
static enum ZclStatusCodeT onOff_server_4_off(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT onOff_server_4_on(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT onOff_server_4_toggle(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg);

static struct ZbZclOnOffServerCallbacksT OnOffServerCallbacks_4 = {
  .off = onOff_server_4_off,
  .on = onOff_server_4_on,
  .toggle = onOff_server_4_toggle,
};

/* LevelControl server 4 custom callbacks */
static enum ZclStatusCodeT levelControl_server_4_move_to_level(struct ZbZclClusterT *cluster, struct ZbZclLevelClientMoveToLevelReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT levelControl_server_4_move(struct ZbZclClusterT *cluster, struct ZbZclLevelClientMoveReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT levelControl_server_4_step(struct ZbZclClusterT *cluster, struct ZbZclLevelClientStepReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT levelControl_server_4_stop(struct ZbZclClusterT *cluster, struct ZbZclLevelClientStopReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg);

static struct ZbZclLevelServerCallbacksT LevelServerCallbacks_4 = {
  .move_to_level = levelControl_server_4_move_to_level,
  .move = levelControl_server_4_move,
  .step = levelControl_server_4_step,
  .stop = levelControl_server_4_stop,
};

/* OnOff server 5 custom callbacks */
static enum ZclStatusCodeT onOff_server_5_off(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT onOff_server_5_on(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT onOff_server_5_toggle(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg);

static struct ZbZclOnOffServerCallbacksT OnOffServerCallbacks_5 = {
  .off = onOff_server_5_off,
  .on = onOff_server_5_on,
  .toggle = onOff_server_5_toggle,
};

/* LevelControl server 5 custom callbacks */
static enum ZclStatusCodeT levelControl_server_5_move_to_level(struct ZbZclClusterT *cluster, struct ZbZclLevelClientMoveToLevelReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT levelControl_server_5_move(struct ZbZclClusterT *cluster, struct ZbZclLevelClientMoveReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT levelControl_server_5_step(struct ZbZclClusterT *cluster, struct ZbZclLevelClientStepReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT levelControl_server_5_stop(struct ZbZclClusterT *cluster, struct ZbZclLevelClientStopReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg);

static struct ZbZclLevelServerCallbacksT LevelServerCallbacks_5 = {
  .move_to_level = levelControl_server_5_move_to_level,
  .move = levelControl_server_5_move,
  .step = levelControl_server_5_step,
  .stop = levelControl_server_5_stop,
};

/* OnOff server 6 custom callbacks */
static enum ZclStatusCodeT onOff_server_6_off(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT onOff_server_6_on(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT onOff_server_6_toggle(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg);

static struct ZbZclOnOffServerCallbacksT OnOffServerCallbacks_6 = {
  .off = onOff_server_6_off,
  .on = onOff_server_6_on,
  .toggle = onOff_server_6_toggle,
};

/* LevelControl server 6 custom callbacks */
static enum ZclStatusCodeT levelControl_server_6_move_to_level(struct ZbZclClusterT *cluster, struct ZbZclLevelClientMoveToLevelReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT levelControl_server_6_move(struct ZbZclClusterT *cluster, struct ZbZclLevelClientMoveReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT levelControl_server_6_step(struct ZbZclClusterT *cluster, struct ZbZclLevelClientStepReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg);
static enum ZclStatusCodeT levelControl_server_6_stop(struct ZbZclClusterT *cluster, struct ZbZclLevelClientStopReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg);

static struct ZbZclLevelServerCallbacksT LevelServerCallbacks_6 = {
  .move_to_level = levelControl_server_6_move_to_level,
  .move = levelControl_server_6_move,
  .step = levelControl_server_6_step,
  .stop = levelControl_server_6_stop,
};

/* USER CODE BEGIN PV */
/* USER CODE END PV */
/* Functions Definition ------------------------------------------------------*/

/* OnOff server off 1 command callback */
static enum ZclStatusCodeT onOff_server_1_off(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 0 OnOff server 1 off 1 */
	//rainbow_Bool = 0;
	APP_DBG("off");
	LED_bool = false;
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 0 OnOff server 1 off 1 */
}

/* OnOff server on 1 command callback */
static enum ZclStatusCodeT onOff_server_1_on(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 1 OnOff server 1 on 1 */
	//rainbow_Bool = 1;

	APP_DBG("on");
	LED_bool = true;
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 1 OnOff server 1 on 1 */
}

/* OnOff server toggle 1 command callback */
static enum ZclStatusCodeT onOff_server_1_toggle(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 2 OnOff server 1 toggle 1 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 2 OnOff server 1 toggle 1 */
}

/* LevelControl server move_to_level 1 command callback */
//sets the amount of red in the LED's
static enum ZclStatusCodeT levelControl_server_1_move_to_level(struct ZbZclClusterT *cluster, struct ZbZclLevelClientMoveToLevelReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg)
{
	color_R = req->level;
	APP_DBG("red level changed");
  return ZCL_STATUS_SUCCESS;
}

/* LevelControl server move 1 command callback */
static enum ZclStatusCodeT levelControl_server_1_move(struct ZbZclClusterT *cluster, struct ZbZclLevelClientMoveReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 4 LevelControl server 1 move 1 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 4 LevelControl server 1 move 1 */
}

/* LevelControl server step 1 command callback */
static enum ZclStatusCodeT levelControl_server_1_step(struct ZbZclClusterT *cluster, struct ZbZclLevelClientStepReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 5 LevelControl server 1 step 1 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 5 LevelControl server 1 step 1 */
}

/* LevelControl server stop 1 command callback */
static enum ZclStatusCodeT levelControl_server_1_stop(struct ZbZclClusterT *cluster, struct ZbZclLevelClientStopReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 6 LevelControl server 1 stop 1 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 6 LevelControl server 1 stop 1 */
}

/* OnOff server off 2 command callback */
//set rainbow mode to false.
static enum ZclStatusCodeT onOff_server_2_off(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg){
	rainbow_Bool = false;
	APP_DBG("rainbow off");
  return ZCL_STATUS_SUCCESS;
}

/* OnOff server on 2 command callback */
//sets rainbow mode to true.
static enum ZclStatusCodeT onOff_server_2_on(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg){
	rainbow_Bool = true;
	APP_DBG("rainbow on");
  return ZCL_STATUS_SUCCESS;
}

/* OnOff server toggle 2 command callback */
static enum ZclStatusCodeT onOff_server_2_toggle(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 9 OnOff server 2 toggle 2 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 9 OnOff server 2 toggle 2 */
}

/* LevelControl server move_to_level 2 command callback */
static enum ZclStatusCodeT levelControl_server_2_move_to_level(struct ZbZclClusterT *cluster, struct ZbZclLevelClientMoveToLevelReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg){
	color_G = req->level;
	APP_DBG("green level changed!");
  return ZCL_STATUS_SUCCESS;
}

/* LevelControl server move 2 command callback */
static enum ZclStatusCodeT levelControl_server_2_move(struct ZbZclClusterT *cluster, struct ZbZclLevelClientMoveReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 11 LevelControl server 2 move 2 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 11 LevelControl server 2 move 2 */
}

/* LevelControl server step 2 command callback */
static enum ZclStatusCodeT levelControl_server_2_step(struct ZbZclClusterT *cluster, struct ZbZclLevelClientStepReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 12 LevelControl server 2 step 2 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 12 LevelControl server 2 step 2 */
}

/* LevelControl server stop 2 command callback */
static enum ZclStatusCodeT levelControl_server_2_stop(struct ZbZclClusterT *cluster, struct ZbZclLevelClientStopReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 13 LevelControl server 2 stop 2 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 13 LevelControl server 2 stop 2 */
}

/* OnOff server off 3 command callback */
//sets breathe mode to false
static enum ZclStatusCodeT onOff_server_3_off(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg){
	breathe_Bool = false;
	APP_DBG("breathe off");
  return ZCL_STATUS_SUCCESS;
}

/* OnOff server on 3 command callback */
//sets breathe mode to on
static enum ZclStatusCodeT onOff_server_3_on(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg){
	breathe_Bool = true;
	APP_DBG("breathe on");
  return ZCL_STATUS_SUCCESS;
}

/* OnOff server toggle 3 command callback */
static enum ZclStatusCodeT onOff_server_3_toggle(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 16 OnOff server 3 toggle 3 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 16 OnOff server 3 toggle 3 */
}

/* LevelControl server move_to_level 3 command callback */
//sets the blue level of the LED's
static enum ZclStatusCodeT levelControl_server_3_move_to_level(struct ZbZclClusterT *cluster, struct ZbZclLevelClientMoveToLevelReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg){
	color_B = req->level;
	APP_DBG("blue level changed");
  return ZCL_STATUS_SUCCESS;
}

/* LevelControl server move 3 command callback */
static enum ZclStatusCodeT levelControl_server_3_move(struct ZbZclClusterT *cluster, struct ZbZclLevelClientMoveReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 18 LevelControl server 3 move 3 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 18 LevelControl server 3 move 3 */
}

/* LevelControl server step 3 command callback */
static enum ZclStatusCodeT levelControl_server_3_step(struct ZbZclClusterT *cluster, struct ZbZclLevelClientStepReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 19 LevelControl server 3 step 3 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 19 LevelControl server 3 step 3 */
}

/* LevelControl server stop 3 command callback */
static enum ZclStatusCodeT levelControl_server_3_stop(struct ZbZclClusterT *cluster, struct ZbZclLevelClientStopReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 20 LevelControl server 3 stop 3 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 20 LevelControl server 3 stop 3 */
}

/* OnOff server off 4 command callback */
//sets blink mode to false
static enum ZclStatusCodeT onOff_server_4_off(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg){
	blink_Bool = false;
	APP_DBG("false");
  return ZCL_STATUS_SUCCESS;
}

/* OnOff server on 4 command callback */
//sets blink mode to True
static enum ZclStatusCodeT onOff_server_4_on(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg){
	blink_Bool = true;
	APP_DBG("blink on")
  return ZCL_STATUS_SUCCESS;
}

/* OnOff server toggle 4 command callback */
static enum ZclStatusCodeT onOff_server_4_toggle(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 23 OnOff server 4 toggle 4 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 23 OnOff server 4 toggle 4 */
}

/* LevelControl server move_to_level 4 command callback */
static enum ZclStatusCodeT levelControl_server_4_move_to_level(struct ZbZclClusterT *cluster, struct ZbZclLevelClientMoveToLevelReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 24 LevelControl server 4 move_to_level 4 */
	brightness = req->level;
	if(brightness > 45)
		brightness = 45;
	APP_DBG("brightness changed");
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 24 LevelControl server 4 move_to_level 4 */
}

/* LevelControl server move 4 command callback */
static enum ZclStatusCodeT levelControl_server_4_move(struct ZbZclClusterT *cluster, struct ZbZclLevelClientMoveReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 25 LevelControl server 4 move 4 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 25 LevelControl server 4 move 4 */
}

/* LevelControl server step 4 command callback */
static enum ZclStatusCodeT levelControl_server_4_step(struct ZbZclClusterT *cluster, struct ZbZclLevelClientStepReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 26 LevelControl server 4 step 4 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 26 LevelControl server 4 step 4 */
}

/* LevelControl server stop 4 command callback */
static enum ZclStatusCodeT levelControl_server_4_stop(struct ZbZclClusterT *cluster, struct ZbZclLevelClientStopReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 27 LevelControl server 4 stop 4 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 27 LevelControl server 4 stop 4 */
}

/* OnOff server off 5 command callback */
static enum ZclStatusCodeT onOff_server_5_off(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 28 OnOff server 5 off 5 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 28 OnOff server 5 off 5 */
}

/* OnOff server on 5 command callback */
static enum ZclStatusCodeT onOff_server_5_on(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 29 OnOff server 5 on 5 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 29 OnOff server 5 on 5 */
}

/* OnOff server toggle 5 command callback */
static enum ZclStatusCodeT onOff_server_5_toggle(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 30 OnOff server 5 toggle 5 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 30 OnOff server 5 toggle 5 */
}

/* LevelControl server move_to_level 5 command callback */
//changes the time level
static enum ZclStatusCodeT levelControl_server_5_move_to_level(struct ZbZclClusterT *cluster, struct ZbZclLevelClientMoveToLevelReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg){
	time_Level = req->level;
	APP_DBG("time level changed");
  return ZCL_STATUS_SUCCESS;
}

/* LevelControl server move 5 command callback */
static enum ZclStatusCodeT levelControl_server_5_move(struct ZbZclClusterT *cluster, struct ZbZclLevelClientMoveReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 32 LevelControl server 5 move 5 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 32 LevelControl server 5 move 5 */
}

/* LevelControl server step 5 command callback */
static enum ZclStatusCodeT levelControl_server_5_step(struct ZbZclClusterT *cluster, struct ZbZclLevelClientStepReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 33 LevelControl server 5 step 5 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 33 LevelControl server 5 step 5 */
}

/* LevelControl server stop 5 command callback */
static enum ZclStatusCodeT levelControl_server_5_stop(struct ZbZclClusterT *cluster, struct ZbZclLevelClientStopReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 34 LevelControl server 5 stop 5 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 34 LevelControl server 5 stop 5 */
}

/* OnOff server off 6 command callback */
static enum ZclStatusCodeT onOff_server_6_off(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 35 OnOff server 6 off 6 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 35 OnOff server 6 off 6 */
}

/* OnOff server on 6 command callback */
static enum ZclStatusCodeT onOff_server_6_on(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 36 OnOff server 6 on 6 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 36 OnOff server 6 on 6 */
}

/* OnOff server toggle 6 command callback */
static enum ZclStatusCodeT onOff_server_6_toggle(struct ZbZclClusterT *cluster, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 37 OnOff server 6 toggle 6 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 37 OnOff server 6 toggle 6 */
}

/* LevelControl server move_to_level 6 command callback */
static enum ZclStatusCodeT levelControl_server_6_move_to_level(struct ZbZclClusterT *cluster, struct ZbZclLevelClientMoveToLevelReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 38 LevelControl server 6 move_to_level 6 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 38 LevelControl server 6 move_to_level 6 */
}

/* LevelControl server move 6 command callback */
static enum ZclStatusCodeT levelControl_server_6_move(struct ZbZclClusterT *cluster, struct ZbZclLevelClientMoveReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 39 LevelControl server 6 move 6 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 39 LevelControl server 6 move 6 */
}

/* LevelControl server step 6 command callback */
static enum ZclStatusCodeT levelControl_server_6_step(struct ZbZclClusterT *cluster, struct ZbZclLevelClientStepReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 40 LevelControl server 6 step 6 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 40 LevelControl server 6 step 6 */
}

/* LevelControl server stop 6 command callback */
static enum ZclStatusCodeT levelControl_server_6_stop(struct ZbZclClusterT *cluster, struct ZbZclLevelClientStopReqT *req, struct ZbZclAddrInfoT *srcInfo, void *arg){
  /* USER CODE BEGIN 41 LevelControl server 6 stop 6 */
  return ZCL_STATUS_SUCCESS;
  /* USER CODE END 41 LevelControl server 6 stop 6 */
}

/**
 * @brief  Zigbee application initialization
 * @param  None
 * @retval None
 */
void APP_ZIGBEE_Init(void)
{
  SHCI_CmdStatus_t ZigbeeInitStatus;

  APP_DBG("APP_ZIGBEE_Init");

  /* Check the compatibility with the Coprocessor Wireless Firmware loaded */
  APP_ZIGBEE_CheckWirelessFirmwareInfo();

  /* Register cmdbuffer */
  APP_ZIGBEE_RegisterCmdBuffer(&ZigbeeOtCmdBuffer);

  /* Init config buffer and call TL_ZIGBEE_Init */
  APP_ZIGBEE_TL_INIT();

  /* Register task */
  /* Create the different tasks */

  UTIL_SEQ_RegTask(1U << (uint32_t)CFG_TASK_NOTIFY_FROM_M0_TO_M4, UTIL_SEQ_RFU, APP_ZIGBEE_ProcessNotifyM0ToM4);
  UTIL_SEQ_RegTask(1U << (uint32_t)CFG_TASK_REQUEST_FROM_M0_TO_M4, UTIL_SEQ_RFU, APP_ZIGBEE_ProcessRequestM0ToM4);

  /* Task associated with network creation process */
  UTIL_SEQ_RegTask(1U << CFG_TASK_ZIGBEE_NETWORK_FORM, UTIL_SEQ_RFU, APP_ZIGBEE_NwkForm);

  /* USER CODE BEGIN APP_ZIGBEE_INIT */
  /* USER CODE END APP_ZIGBEE_INIT */

  /* Start the Zigbee on the CPU2 side */
  ZigbeeInitStatus = SHCI_C2_ZIGBEE_Init();
  /* Prevent unused argument(s) compilation warning */
  UNUSED(ZigbeeInitStatus);

  /* Initialize Zigbee stack layers */
  APP_ZIGBEE_StackLayersInit();

} /* APP_ZIGBEE_Init */

/**
 * @brief  Initialize Zigbee stack layers
 * @param  None
 * @retval None
 */
static void APP_ZIGBEE_StackLayersInit(void)
{
  APP_DBG("APP_ZIGBEE_StackLayersInit");

  zigbee_app_info.zb = ZbInit(0U, NULL, NULL);
  assert(zigbee_app_info.zb != NULL);

  /* Create the endpoint and cluster(s) */
  APP_ZIGBEE_ConfigEndpoints();

  /* USER CODE BEGIN APP_ZIGBEE_StackLayersInit */
  /* USER CODE END APP_ZIGBEE_StackLayersInit */

  /* Configure the joining parameters */
  zigbee_app_info.join_status = (enum ZbStatusCodeT) 0x01; /* init to error status */
  zigbee_app_info.join_delay = HAL_GetTick(); /* now */
  zigbee_app_info.startupControl = ZbStartTypeJoin;

  /* Initialization Complete */
  zigbee_app_info.has_init = true;

  /* run the task */
  UTIL_SEQ_SetTask(1U << CFG_TASK_ZIGBEE_NETWORK_FORM, CFG_SCH_PRIO_0);
} /* APP_ZIGBEE_StackLayersInit */

/**
 * @brief  Configure Zigbee application endpoints
 * @param  None
 * @retval None
 */
static void APP_ZIGBEE_ConfigEndpoints(void)
{
  struct ZbApsmeAddEndpointReqT req;
  struct ZbApsmeAddEndpointConfT conf;

  memset(&req, 0, sizeof(req));

  /* Endpoint: SW1_ENDPOINT */
  req.profileId = ZCL_PROFILE_HOME_AUTOMATION;
  req.deviceId = ZCL_DEVICE_LEVEL_SWITCH;
  req.endpoint = SW1_ENDPOINT;
  ZbZclAddEndpoint(zigbee_app_info.zb, &req, &conf);
  assert(conf.status == ZB_STATUS_SUCCESS);

  /* OnOff server */
  zigbee_app_info.onOff_server_1 = ZbZclOnOffServerAlloc(zigbee_app_info.zb, SW1_ENDPOINT, &OnOffServerCallbacks_1, NULL);
  assert(zigbee_app_info.onOff_server_1 != NULL);
  ZbZclClusterEndpointRegister(zigbee_app_info.onOff_server_1);

  /* LevelControl server */
  zigbee_app_info.levelControl_server_1 = ZbZclLevelServerAlloc(zigbee_app_info.zb, SW1_ENDPOINT, zigbee_app_info.onOff_server_1, &LevelServerCallbacks_1, NULL);
  assert(zigbee_app_info.levelControl_server_1 != NULL);
  ZbZclClusterEndpointRegister(zigbee_app_info.levelControl_server_1);

  /* Endpoint: SW2_ENDPOINT */
  req.profileId = ZCL_PROFILE_HOME_AUTOMATION;
  req.deviceId = ZCL_DEVICE_ONOFF_SWITCH;
  req.endpoint = SW2_ENDPOINT;
  ZbZclAddEndpoint(zigbee_app_info.zb, &req, &conf);
  assert(conf.status == ZB_STATUS_SUCCESS);

  /* OnOff server */
  zigbee_app_info.onOff_server_2 = ZbZclOnOffServerAlloc(zigbee_app_info.zb, SW2_ENDPOINT, &OnOffServerCallbacks_2, NULL);
  assert(zigbee_app_info.onOff_server_2 != NULL);
  ZbZclClusterEndpointRegister(zigbee_app_info.onOff_server_2);

  /* LevelControl server */
  zigbee_app_info.levelControl_server_2 = ZbZclLevelServerAlloc(zigbee_app_info.zb, SW2_ENDPOINT, zigbee_app_info.onOff_server_2, &LevelServerCallbacks_2, NULL);
  assert(zigbee_app_info.levelControl_server_2 != NULL);
  ZbZclClusterEndpointRegister(zigbee_app_info.levelControl_server_2);

  /* Endpoint: SW3_ENDPOINT */
  req.profileId = ZCL_PROFILE_HOME_AUTOMATION;
  req.deviceId = ZCL_DEVICE_ONOFF_SWITCH;
  req.endpoint = SW3_ENDPOINT;
  ZbZclAddEndpoint(zigbee_app_info.zb, &req, &conf);
  assert(conf.status == ZB_STATUS_SUCCESS);

  /* OnOff server */
  zigbee_app_info.onOff_server_3 = ZbZclOnOffServerAlloc(zigbee_app_info.zb, SW3_ENDPOINT, &OnOffServerCallbacks_3, NULL);
  assert(zigbee_app_info.onOff_server_3 != NULL);
  ZbZclClusterEndpointRegister(zigbee_app_info.onOff_server_3);

  /* LevelControl server */
  zigbee_app_info.levelControl_server_3 = ZbZclLevelServerAlloc(zigbee_app_info.zb, SW3_ENDPOINT, zigbee_app_info.onOff_server_3, &LevelServerCallbacks_3, NULL);
  assert(zigbee_app_info.levelControl_server_3 != NULL);
  ZbZclClusterEndpointRegister(zigbee_app_info.levelControl_server_3);

  /* Endpoint: SW4_ENDPOINT */
  req.profileId = ZCL_PROFILE_HOME_AUTOMATION;
  req.deviceId = ZCL_DEVICE_LEVEL_SWITCH;
  req.endpoint = SW4_ENDPOINT;
  ZbZclAddEndpoint(zigbee_app_info.zb, &req, &conf);
  assert(conf.status == ZB_STATUS_SUCCESS);

  /* OnOff server */
  zigbee_app_info.onOff_server_4 = ZbZclOnOffServerAlloc(zigbee_app_info.zb, SW4_ENDPOINT, &OnOffServerCallbacks_4, NULL);
  assert(zigbee_app_info.onOff_server_4 != NULL);
  ZbZclClusterEndpointRegister(zigbee_app_info.onOff_server_4);

  /* LevelControl server */
  zigbee_app_info.levelControl_server_4 = ZbZclLevelServerAlloc(zigbee_app_info.zb, SW4_ENDPOINT, zigbee_app_info.onOff_server_4, &LevelServerCallbacks_4, NULL);
  assert(zigbee_app_info.levelControl_server_4 != NULL);
  ZbZclClusterEndpointRegister(zigbee_app_info.levelControl_server_4);

  /* Endpoint: SW5_ENDPOINT */
  req.profileId = ZCL_PROFILE_HOME_AUTOMATION;
  req.deviceId = ZCL_DEVICE_LEVEL_SWITCH;
  req.endpoint = SW5_ENDPOINT;
  ZbZclAddEndpoint(zigbee_app_info.zb, &req, &conf);
  assert(conf.status == ZB_STATUS_SUCCESS);

  /* OnOff server */
  zigbee_app_info.onOff_server_5 = ZbZclOnOffServerAlloc(zigbee_app_info.zb, SW5_ENDPOINT, &OnOffServerCallbacks_5, NULL);
  assert(zigbee_app_info.onOff_server_5 != NULL);
  ZbZclClusterEndpointRegister(zigbee_app_info.onOff_server_5);

  /* LevelControl server */
  zigbee_app_info.levelControl_server_5 = ZbZclLevelServerAlloc(zigbee_app_info.zb, SW5_ENDPOINT, zigbee_app_info.onOff_server_5, &LevelServerCallbacks_5, NULL);
  assert(zigbee_app_info.levelControl_server_5 != NULL);
  ZbZclClusterEndpointRegister(zigbee_app_info.levelControl_server_5);

  /* Endpoint: SW6_ENDPOINT */
  req.profileId = ZCL_PROFILE_HOME_AUTOMATION;
  req.deviceId = ZCL_DEVICE_LEVEL_SWITCH;
  req.endpoint = SW6_ENDPOINT;
  ZbZclAddEndpoint(zigbee_app_info.zb, &req, &conf);
  assert(conf.status == ZB_STATUS_SUCCESS);

  /* OnOff server */
  zigbee_app_info.onOff_server_6 = ZbZclOnOffServerAlloc(zigbee_app_info.zb, SW6_ENDPOINT, &OnOffServerCallbacks_6, NULL);
  assert(zigbee_app_info.onOff_server_6 != NULL);
  ZbZclClusterEndpointRegister(zigbee_app_info.onOff_server_6);

  /* LevelControl server */
  zigbee_app_info.levelControl_server_6 = ZbZclLevelServerAlloc(zigbee_app_info.zb, SW6_ENDPOINT, zigbee_app_info.onOff_server_6, &LevelServerCallbacks_6, NULL);
  assert(zigbee_app_info.levelControl_server_6 != NULL);
  ZbZclClusterEndpointRegister(zigbee_app_info.levelControl_server_6);

  /* USER CODE BEGIN CONFIG_ENDPOINT */
  /* USER CODE END CONFIG_ENDPOINT */
} /* APP_ZIGBEE_ConfigEndpoints */

/**
 * @brief  Handle Zigbee network forming and joining
 * @param  None
 * @retval None
 */
static void APP_ZIGBEE_NwkForm(void)
{
  if ((zigbee_app_info.join_status != ZB_STATUS_SUCCESS) && (HAL_GetTick() >= zigbee_app_info.join_delay))
  {
    struct ZbStartupT config;
    enum ZbStatusCodeT status;

    /* Configure Zigbee Logging */
    ZbSetLogging(zigbee_app_info.zb, ZB_LOG_MASK_LEVEL_5, NULL);

    /* Attempt to join a zigbee network */
    ZbStartupConfigGetProDefaults(&config);

    /* Set the centralized network */
    APP_DBG("Network config : APP_STARTUP_CENTRALIZED_ROUTER");
    config.startupControl = zigbee_app_info.startupControl;

    /* Using the default HA preconfigured Link Key */
    memcpy(config.security.preconfiguredLinkKey, sec_key_ha, ZB_SEC_KEYSIZE);


    //channel mask needs to be changed from how CubeIDE creates it.
    //CubeIDE expects the channel is known for the application but in most cases it isn't.
    //so instead of the normal channel we set the channel mask to the whole Zigbee range.
    config.channelList.count = 1;
    config.channelList.list[0].page = 0;
    config.channelList.list[0].channelMask = WPAN_CHANNELMASK_2400MHZ; /*Channel in use */

    /* Using ZbStartupWait (blocking) */
    status = ZbStartupWait(zigbee_app_info.zb, &config);

    APP_DBG("ZbStartup Callback (status = 0x%02x)", status);
    zigbee_app_info.join_status = status;

    if (status == ZB_STATUS_SUCCESS) {
      /* USER CODE BEGIN 42 */
      zigbee_app_info.join_delay = 0U;
    }
    else
    {
      /* USER CODE END 42 */
      APP_DBG("Startup failed, attempting again after a short delay (%d ms)", APP_ZIGBEE_STARTUP_FAIL_DELAY);
      zigbee_app_info.join_delay = HAL_GetTick() + APP_ZIGBEE_STARTUP_FAIL_DELAY;
    }
  }

  /* If Network forming/joining was not successful reschedule the current task to retry the process */
  if (zigbee_app_info.join_status != ZB_STATUS_SUCCESS)
  {
    UTIL_SEQ_SetTask(1U << CFG_TASK_ZIGBEE_NETWORK_FORM, CFG_SCH_PRIO_0);
  }

  /* USER CODE BEGIN NW_FORM */
  else
  {
    zigbee_app_info.init_after_join = false;

    /* Assign ourselves to the group addresses */
    APP_ZIGBEE_ConfigGroupAddr();

    /* Since we're using group addressing (broadcast), shorten the broadcast timeout */
    uint32_t bcast_timeout = 3;
    ZbNwkSet(zigbee_app_info.zb, ZB_NWK_NIB_ID_NetworkBroadcastDeliveryTime, &bcast_timeout, sizeof(bcast_timeout));
  }
  /* USER CODE END NW_FORM */
} /* APP_ZIGBEE_NwkForm */

/*************************************************************
 * ZbStartupWait Blocking Call
 *************************************************************/
struct ZbStartupWaitInfo {
  bool active;
  enum ZbStatusCodeT status;
};

static void ZbStartupWaitCb(enum ZbStatusCodeT status, void *cb_arg)
{
  struct ZbStartupWaitInfo *info = cb_arg;

  info->status = status;
  info->active = false;
  UTIL_SEQ_SetEvt(EVENT_ZIGBEE_STARTUP_ENDED);
} /* ZbStartupWaitCb */

enum ZbStatusCodeT ZbStartupWait(struct ZigBeeT *zb, struct ZbStartupT *config)
{
  struct ZbStartupWaitInfo *info;
  enum ZbStatusCodeT status;

  info = malloc(sizeof(struct ZbStartupWaitInfo));
  if (info == NULL) {
    return ZB_STATUS_ALLOC_FAIL;
  }
  memset(info, 0, sizeof(struct ZbStartupWaitInfo));

  info->active = true;
  status = ZbStartup(zb, config, ZbStartupWaitCb, info);
  if (status != ZB_STATUS_SUCCESS) {
    info->active = false;
    return status;
  }
  UTIL_SEQ_WaitEvt(EVENT_ZIGBEE_STARTUP_ENDED);
  status = info->status;
  free(info);
  return status;
} /* ZbStartupWait */

/**
 * @brief  Trace the error or the warning reported.
 * @param  ErrId :
 * @param  ErrCode
 * @retval None
 */
void APP_ZIGBEE_Error(uint32_t ErrId, uint32_t ErrCode)
{
  switch (ErrId) {
  default:
    APP_ZIGBEE_TraceError("ERROR Unknown ", 0);
    break;
  }
} /* APP_ZIGBEE_Error */

/*************************************************************
 *
 * LOCAL FUNCTIONS
 *
 *************************************************************/

/**
 * @brief  Warn the user that an error has occurred.
 *
 * @param  pMess  : Message associated to the error.
 * @param  ErrCode: Error code associated to the module (Zigbee or other module if any)
 * @retval None
 */
static void APP_ZIGBEE_TraceError(const char *pMess, uint32_t ErrCode)
{
  APP_DBG("**** Fatal error = %s (Err = %d)", pMess, ErrCode);
  /* USER CODE BEGIN TRACE_ERROR */
  while (1U == 1U) {
  }
  /* USER CODE END TRACE_ERROR */

} /* APP_ZIGBEE_TraceError */

/**
 * @brief Check if the Coprocessor Wireless Firmware loaded supports Zigbee
 *        and display associated information
 * @param  None
 * @retval None
 */
static void APP_ZIGBEE_CheckWirelessFirmwareInfo(void)
{
  WirelessFwInfo_t wireless_info_instance;
  WirelessFwInfo_t *p_wireless_info = &wireless_info_instance;

  if (SHCI_GetWirelessFwInfo(p_wireless_info) != SHCI_Success) {
    APP_ZIGBEE_Error((uint32_t)ERR_ZIGBEE_CHECK_WIRELESS, (uint32_t)ERR_INTERFACE_FATAL);
  }
  else {
    APP_DBG("**********************************************************");
    APP_DBG("WIRELESS COPROCESSOR FW:");
    /* Print version */
    APP_DBG("VERSION ID = %d.%d.%d", p_wireless_info->VersionMajor, p_wireless_info->VersionMinor, p_wireless_info->VersionSub);

    switch (p_wireless_info->StackType) {
    case INFO_STACK_TYPE_ZIGBEE_FFD:
      APP_DBG("FW Type : FFD Zigbee stack");
      break;
   case INFO_STACK_TYPE_ZIGBEE_RFD:
      APP_DBG("FW Type : RFD Zigbee stack");
      break;
    default:
      /* No Zigbee device supported ! */
      APP_ZIGBEE_Error((uint32_t)ERR_ZIGBEE_CHECK_WIRELESS, (uint32_t)ERR_INTERFACE_FATAL);
      break;
    }
    APP_DBG("**********************************************************");
  }
} /* APP_ZIGBEE_CheckWirelessFirmwareInfo */

/*************************************************************
 *
 * WRAP FUNCTIONS
 *
 *************************************************************/

void APP_ZIGBEE_RegisterCmdBuffer(TL_CmdPacket_t *p_buffer)
{
  p_ZIGBEE_otcmdbuffer = p_buffer;
} /* APP_ZIGBEE_RegisterCmdBuffer */

Zigbee_Cmd_Request_t * ZIGBEE_Get_OTCmdPayloadBuffer(void)
{
  return (Zigbee_Cmd_Request_t *)p_ZIGBEE_otcmdbuffer->cmdserial.cmd.payload;
} /* ZIGBEE_Get_OTCmdPayloadBuffer */

Zigbee_Cmd_Request_t * ZIGBEE_Get_OTCmdRspPayloadBuffer(void)
{
  return (Zigbee_Cmd_Request_t *)((TL_EvtPacket_t *)p_ZIGBEE_otcmdbuffer)->evtserial.evt.payload;
} /* ZIGBEE_Get_OTCmdRspPayloadBuffer */

Zigbee_Cmd_Request_t * ZIGBEE_Get_NotificationPayloadBuffer(void)
{
  return (Zigbee_Cmd_Request_t *)(p_ZIGBEE_notif_M0_to_M4)->evtserial.evt.payload;
} /* ZIGBEE_Get_NotificationPayloadBuffer */

Zigbee_Cmd_Request_t * ZIGBEE_Get_M0RequestPayloadBuffer(void)
{
  return (Zigbee_Cmd_Request_t *)(p_ZIGBEE_request_M0_to_M4)->evtserial.evt.payload;
}

/**
 * @brief  This function is used to transfer the commands from the M4 to the M0.
 *
 * @param   None
 * @return  None
 */
void ZIGBEE_CmdTransfer(void)
{
  Zigbee_Cmd_Request_t *cmd_req = (Zigbee_Cmd_Request_t *)p_ZIGBEE_otcmdbuffer->cmdserial.cmd.payload;

  /* Zigbee OT command cmdcode range 0x280 .. 0x3DF = 352 */
  p_ZIGBEE_otcmdbuffer->cmdserial.cmd.cmdcode = 0x280U;
  /* Size = otCmdBuffer->Size (Number of OT cmd arguments : 1 arg = 32bits so multiply by 4 to get size in bytes)
   * + ID (4 bytes) + Size (4 bytes) */
  p_ZIGBEE_otcmdbuffer->cmdserial.cmd.plen = 8U + (cmd_req->Size * 4U);

  TL_ZIGBEE_SendM4RequestToM0();

  /* Wait completion of cmd */
  Wait_Getting_Ack_From_M0();
} /* ZIGBEE_CmdTransfer */

/**
 * @brief  This function is called when the M0+ acknowledge the fact that it has received a Cmd
 *
 *
 * @param   Otbuffer : a pointer to TL_EvtPacket_t
 * @return  None
 */
void TL_ZIGBEE_CmdEvtReceived(TL_EvtPacket_t *Otbuffer)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(Otbuffer);

  Receive_Ack_From_M0();
} /* TL_ZIGBEE_CmdEvtReceived */

/**
 * @brief  This function is called when notification from M0+ is received.
 *
 * @param   Notbuffer : a pointer to TL_EvtPacket_t
 * @return  None
 */
void TL_ZIGBEE_NotReceived(TL_EvtPacket_t *Notbuffer)
{
  p_ZIGBEE_notif_M0_to_M4 = Notbuffer;

  Receive_Notification_From_M0();
} /* TL_ZIGBEE_NotReceived */

/**
 * @brief  This function is called before sending any ot command to the M0
 *         core. The purpose of this function is to be able to check if
 *         there are no notifications coming from the M0 core which are
 *         pending before sending a new ot command.
 * @param  None
 * @retval None
 */
void Pre_ZigbeeCmdProcessing(void)
{
  UTIL_SEQ_WaitEvt(EVENT_SYNCHRO_BYPASS_IDLE);
} /* Pre_ZigbeeCmdProcessing */

/**
 * @brief  This function waits for getting an acknowledgment from the M0.
 *
 * @param  None
 * @retval None
 */
static void Wait_Getting_Ack_From_M0(void)
{
  UTIL_SEQ_WaitEvt(EVENT_ACK_FROM_M0_EVT);
} /* Wait_Getting_Ack_From_M0 */

/**
 * @brief  Receive an acknowledgment from the M0+ core.
 *         Each command send by the M4 to the M0 are acknowledged.
 *         This function is called under interrupt.
 * @param  None
 * @retval None
 */
static void Receive_Ack_From_M0(void)
{
  UTIL_SEQ_SetEvt(EVENT_ACK_FROM_M0_EVT);
} /* Receive_Ack_From_M0 */

/**
 * @brief  Receive a notification from the M0+ through the IPCC.
 *         This function is called under interrupt.
 * @param  None
 * @retval None
 */
static void Receive_Notification_From_M0(void)
{
    CptReceiveNotifyFromM0++;
    UTIL_SEQ_SetTask(1U << (uint32_t)CFG_TASK_NOTIFY_FROM_M0_TO_M4, CFG_SCH_PRIO_0);
}

/**
 * @brief  This function is called when a request from M0+ is received.
 *
 * @param   Notbuffer : a pointer to TL_EvtPacket_t
 * @return  None
 */
void TL_ZIGBEE_M0RequestReceived(TL_EvtPacket_t *Reqbuffer)
{
    p_ZIGBEE_request_M0_to_M4 = Reqbuffer;

    CptReceiveRequestFromM0++;
    UTIL_SEQ_SetTask(1U << (uint32_t)CFG_TASK_REQUEST_FROM_M0_TO_M4, CFG_SCH_PRIO_0);
}

/**
 * @brief Perform initialization of TL for Zigbee.
 * @param  None
 * @retval None
 */
void APP_ZIGBEE_TL_INIT(void)
{
    ZigbeeConfigBuffer.p_ZigbeeOtCmdRspBuffer = (uint8_t *)&ZigbeeOtCmdBuffer;
    ZigbeeConfigBuffer.p_ZigbeeNotAckBuffer = (uint8_t *)ZigbeeNotifRspEvtBuffer;
    ZigbeeConfigBuffer.p_ZigbeeNotifRequestBuffer = (uint8_t *)ZigbeeNotifRequestBuffer;
    TL_ZIGBEE_Init(&ZigbeeConfigBuffer);
}

/**
 * @brief Process the messages coming from the M0.
 * @param  None
 * @retval None
 */
void APP_ZIGBEE_ProcessNotifyM0ToM4(void)
{
    if (CptReceiveNotifyFromM0 != 0) {
        /* If CptReceiveNotifyFromM0 is > 1. it means that we did not serve all the events from the radio */
        if (CptReceiveNotifyFromM0 > 1U) {
            APP_ZIGBEE_Error(ERR_REC_MULTI_MSG_FROM_M0, 0);
        }
        else {
            Zigbee_CallBackProcessing();
        }
        /* Reset counter */
        CptReceiveNotifyFromM0 = 0;
    }
}

/**
 * @brief Process the requests coming from the M0.
 * @param
 * @return
 */
void APP_ZIGBEE_ProcessRequestM0ToM4(void)
{
    if (CptReceiveRequestFromM0 != 0) {
        Zigbee_M0RequestProcessing();
        CptReceiveRequestFromM0 = 0;
    }
}
/* USER CODE BEGIN FD_LOCAL_FUNCTIONS */

/**
 * @brief  Set group addressing mode
 * @param  None
 * @retval None
 */
static void APP_ZIGBEE_ConfigGroupAddr(void)
{
  struct ZbApsmeAddGroupReqT req;
  struct ZbApsmeAddGroupConfT conf;

  memset(&req, 0, sizeof(req));
  req.endpt = SW1_ENDPOINT;
  req.groupAddr = SW1_GROUP_ADDR;
  ZbApsmeAddGroupReq(zigbee_app_info.zb, &req, &conf);

} /* APP_ZIGBEE_ConfigGroupAddr */

/* USER CODE END FD_LOCAL_FUNCTIONS */
