#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "wifi_device.h"
#include "lwip/netifapi.h"
#include "lwip/api_shell.h"

void connect_wifi(void)
{

    WifiErrorCode errCode;
    WifiDeviceConfig apConfig = {};
    int netId = -1;

    // setup your AP params
    // strcpy(apConfig.ssid, "TP-LINK_888");        //
    // strcpy(apConfig.preSharedKey, "Cto888.com"); //密码
    strcpy(apConfig.ssid, "Line");       //
    strcpy(apConfig.preSharedKey, "123456silent"); //密码
    apConfig.securityType = WIFI_SEC_TYPE_PSK;

    errCode = EnableWifi();
    errCode = AddDeviceConfig(&apConfig, &netId);

    errCode = ConnectTo(netId);
    printf("ConnectTo(%d): %d\r\n", netId, errCode);
    usleep(1000 * 1000);

    struct netif *iface = netifapi_netif_find("wlan0");
    if (iface)
    {
        err_t ret = netifapi_dhcp_start(iface);
        printf("netifapi_dhcp_start: %d\r\n", ret);

        usleep(2000 * 1000);
        ret = netifapi_netif_common(iface, dhcp_clients_info_show, NULL);
        printf("netifapi_netif_common: %d\r\n", ret);
    }
}
