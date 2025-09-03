#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "wifi_device.h"
#include "lwip/netifapi.h"
#include "lwip/err.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"



// 声明缺失的netifapi函数
extern struct netif* netifapi_netif_find(const char *name);
extern err_t netifapi_dhcp_start(struct netif *netif);
extern err_t netifapi_netif_common(struct netif *netif, void (*voidfunc)(struct netif *netif_p), void *init_arg);

// 在包含api_shell.h之前定义必要的LWIP宏
#ifndef LWIP_RPL
#define LWIP_RPL 0
#endif
#ifndef LWIP_RIPPLE
#define LWIP_RIPPLE 0
#endif
#ifndef LWIP_SNTP
#define LWIP_SNTP 0
#endif
#ifndef LWIP_DHCPS
#define LWIP_DHCPS 0
#endif

#include "lwip/api_shell.h"

void connect_wifi(void){
    WifiErrorCode errCode;
    WifiDeviceConfig apConfig = {};
    int netId = -1;

    strcpy(apConfig.ssid, "qpt");
    strcpy(apConfig.preSharedKey, "qwertyuiop");
    apConfig.securityType = WIFI_SEC_TYPE_PSK;

    errCode = EnableWifi();
    errCode = AddDeviceConfig(&apConfig, &netId);

    errCode = ConnectTo(netId);
    printf("ConnectTo(%d)：%d\r\n", netId, errCode);
    usleep(1000 * 1000);
    // 联网业务开始
    struct netif* netIf = netifapi_netif_find("wlan0");
    if(netIf){
        err_t ret = netifapi_dhcp_start(netIf);
        printf("DHCP Start：%d\r\n", ret);

        // 等待DHCP分配IP
        usleep(2000 * 1000);
        ret = netifapi_netif_common(netIf, dhcp_clients_info_show, NULL);
        printf("netifapi_netif_common：%d\r\n", ret);
    }
}