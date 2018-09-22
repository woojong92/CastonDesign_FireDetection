#include "wifi.h"
#include <tinyara/gpio.h>
#include <apps/netutils/mqtt_api.h>
#include <apps/netutils/dhcpc.h>

//for i2c
#include <sys/types.h>
#include <tinyara/i2c.h>

#include <tinyara/analog/adc.h>
#include <tinyara/analog/ioctl.h>

#include <apps/shell/tash.h>

// for NTP
#include <apps/netutils/ntpclient.h>

#define AMG8833_ADDRESS          (0x69)
#define AMG8833_NORMAL_MODE		(0X00)
#define AMG8833_SLEEP_MODE		(0x01)
#define AMG8833_STAND_BY_60		(0x20)
#define AMG8833_STAND_BY_10		(0x21)
#define AMG8833_PIXEL_ARRAY_SIZE	64

static struct i2c_dev_s *i2c_dev;
static struct i2c_config_s configs;

#define DEFAULT_CLIENT_ID "123456789"
#define SERVER_ADDR "api.artik.cloud"
//#define SERVER_ADDR "52.86.204.150"
#define SERVER_PORT 8883
//#define SERVER_PORT 1883 // non-secure mode, Not supported in ARTIK Cloud
#define RED "RED"
#define BLUE "BLUE"
#define GREEN "GREEN"
#define RED_LED 45 // on-board LED
#define GREEN_LED 60
#define BLUE_LED 49 // on-board LED
#define RED_ON_BOARD_LED 45
#define NET_DEVNAME "wl1"

char device_id[] = 		"device id";
char device_token[] = 	"device_token";

char *strTopicMsg;
char *strTopicAct;

static const char mqtt_ca_cert_str[] = \
		"-----BEGIN CERTIFICATE-----\r\n"
		"MIIGrTCCBZWgAwIBAgIQASAP9e8Tbenonqd/EQFJaDANBgkqhkiG9w0BAQsFADBN\r\n"
		"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMScwJQYDVQQDEx5E\r\n"
		"aWdpQ2VydCBTSEEyIFNlY3VyZSBTZXJ2ZXIgQ0EwHhcNMTgwMzA4MDAwMDAwWhcN\r\n"
		"MjAwNDA1MTIwMDAwWjBzMQswCQYDVQQGEwJVUzETMBEGA1UECBMKQ2FsaWZvcm5p\r\n"
		"YTERMA8GA1UEBxMIU2FuIEpvc2UxJDAiBgNVBAoTG1NhbXN1bmcgU2VtaWNvbmR1\r\n"
		"Y3RvciwgSW5jLjEWMBQGA1UEAwwNKi5hcnRpay5jbG91ZDCCASIwDQYJKoZIhvcN\r\n"
		"AQEBBQADggEPADCCAQoCggEBANghNaTXWDfYV/JWgBnX4hmhcClPSO0onx5B2url\r\n"
		"YzpvTc3MBaQ+08YBpAKvTqZvPqrJUIM45Q91M301I5e2kz0DMq2zQZOGB0B83V/O\r\n"
		"O4vwETq4PCjAPhMinF4dN6HeJCuqo1CLh8evhfkFiJvpEfQWTxdjzPJ0Zdj/2U8E\r\n"
		"8Ht7zV5pWiDtuejtIDHB5H6fCx4xeQy/E+5l4V6R3BnRKpZsJtlhTh0RFqWhw5DJ\r\n"
		"/WWpGP//1VTZSHyW9SABsPd+jP1YgDraRD4b4lZBU6c8nC5qT3dhdiYoG6xUgTb3\r\n"
		"kfgUhhlOFpe3sBtR32OS8RuFrFeQDGaa3r6pfSy06Kph/eECAwEAAaOCA2EwggNd\r\n"
		"MB8GA1UdIwQYMBaAFA+AYRyCMWHVLyjnjUY4tCzhxtniMB0GA1UdDgQWBBSNBf6r\r\n"
		"7S/j0oV3A0XmEflXErutQDAlBgNVHREEHjAcgg0qLmFydGlrLmNsb3VkggthcnRp\r\n"
		"ay5jbG91ZDAOBgNVHQ8BAf8EBAMCBaAwHQYDVR0lBBYwFAYIKwYBBQUHAwEGCCsG\r\n"
		"AQUFBwMCMGsGA1UdHwRkMGIwL6AtoCuGKWh0dHA6Ly9jcmwzLmRpZ2ljZXJ0LmNv\r\n"
		"bS9zc2NhLXNoYTItZzYuY3JsMC+gLaArhilodHRwOi8vY3JsNC5kaWdpY2VydC5j\r\n"
		"b20vc3NjYS1zaGEyLWc2LmNybDBMBgNVHSAERTBDMDcGCWCGSAGG/WwBATAqMCgG\r\n"
		"CCsGAQUFBwIBFhxodHRwczovL3d3dy5kaWdpY2VydC5jb20vQ1BTMAgGBmeBDAEC\r\n"
		"AjB8BggrBgEFBQcBAQRwMG4wJAYIKwYBBQUHMAGGGGh0dHA6Ly9vY3NwLmRpZ2lj\r\n"
		"ZXJ0LmNvbTBGBggrBgEFBQcwAoY6aHR0cDovL2NhY2VydHMuZGlnaWNlcnQuY29t\r\n"
		"L0RpZ2lDZXJ0U0hBMlNlY3VyZVNlcnZlckNBLmNydDAJBgNVHRMEAjAAMIIBfwYK\r\n"
		"KwYBBAHWeQIEAgSCAW8EggFrAWkAdgCkuQmQtBhYFIe7E6LMZ3AKPDWYBPkb37jj\r\n"
		"d80OyA3cEAAAAWIHFb1dAAAEAwBHMEUCIQCQ0UjVVJSQDRB3oxzI5aD1Hs5GhbXj\r\n"
		"I6Cqt3/tkXT1WQIgNVWRgbJ72Ik9gp5QoNxhCZ+h//or0uL7PHnv3cP5L9UAdgBv\r\n"
		"U3asMfAxGdiZAKRRFf93FRwR2QLBACkGjbIImjfZEwAAAWIHFb73AAAEAwBHMEUC\r\n"
		"IQDxCxJCsZjuqbQvuwipgdUf1l6qXdiekM5zn33i1+KYxgIgKDMJEuKHzhkweT2S\r\n"
		"Y4dWBuzSdOAzZfoDrIGdsFvkxi0AdwC72d+8H4pxtZOUI5eqkntHOFeVCqtS6BqQ\r\n"
		"lmQ2jh7RhQAAAWIHFb1YAAAEAwBIMEYCIQCNDYdxWmqUGGwNzXlJ1/NXxzwqPYIB\r\n"
		"eSJDuR1xfWtSsQIhAJsygf2rqPS+O7qQAzggCQ2V/3JDRUhuxNDPqwooo47uMA0G\r\n"
		"CSqGSIb3DQEBCwUAA4IBAQBvRGWibvHFrRUWsArJ9lmS5MMZFbXXQPXbflgv3nSG\r\n"
		"ShmhBC3o+k97J0Wgp/wH7uDf01RrRMAVNm458g1Mr4AMAXq3zzxNNTwjGYw/USuG\r\n"
		"UprrKqc9onugtAUX8DGvlZr8SWO3FhPlyamWQ69jutx/X4nfHyZr41bX9WQ/ay0F\r\n"
		"GQJ1tRTrX1eUPO+ucXeG8vTbt09bRNnoY+i97dzrwHakXySfHohNsIbwmrsS4SQv\r\n"
		"7eG9g5+5vsc2B9ugGcELIYKrzDWNPshir37KSpcwLUCmDJkTQp8+KhJUKgbTALTa\r\n"
		"nxuDyNwZIwW66vv1t0Zi4vKU8hfUsAN2N3wcsb6pY/RA\r\n"
		"-----END CERTIFICATE-----\r\n"
		"-----BEGIN CERTIFICATE-----\r\n"
		"MIIElDCCA3ygAwIBAgIQAf2j627KdciIQ4tyS8+8kTANBgkqhkiG9w0BAQsFADBh\r\n"
		"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\r\n"
		"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\r\n"
		"QTAeFw0xMzAzMDgxMjAwMDBaFw0yMzAzMDgxMjAwMDBaME0xCzAJBgNVBAYTAlVT\r\n"
		"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxJzAlBgNVBAMTHkRpZ2lDZXJ0IFNIQTIg\r\n"
		"U2VjdXJlIFNlcnZlciBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\r\n"
		"ANyuWJBNwcQwFZA1W248ghX1LFy949v/cUP6ZCWA1O4Yok3wZtAKc24RmDYXZK83\r\n"
		"nf36QYSvx6+M/hpzTc8zl5CilodTgyu5pnVILR1WN3vaMTIa16yrBvSqXUu3R0bd\r\n"
		"KpPDkC55gIDvEwRqFDu1m5K+wgdlTvza/P96rtxcflUxDOg5B6TXvi/TC2rSsd9f\r\n"
		"/ld0Uzs1gN2ujkSYs58O09rg1/RrKatEp0tYhG2SS4HD2nOLEpdIkARFdRrdNzGX\r\n"
		"kujNVA075ME/OV4uuPNcfhCOhkEAjUVmR7ChZc6gqikJTvOX6+guqw9ypzAO+sf0\r\n"
		"/RR3w6RbKFfCs/mC/bdFWJsCAwEAAaOCAVowggFWMBIGA1UdEwEB/wQIMAYBAf8C\r\n"
		"AQAwDgYDVR0PAQH/BAQDAgGGMDQGCCsGAQUFBwEBBCgwJjAkBggrBgEFBQcwAYYY\r\n"
		"aHR0cDovL29jc3AuZGlnaWNlcnQuY29tMHsGA1UdHwR0MHIwN6A1oDOGMWh0dHA6\r\n"
		"Ly9jcmwzLmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbFJvb3RDQS5jcmwwN6A1\r\n"
		"oDOGMWh0dHA6Ly9jcmw0LmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbFJvb3RD\r\n"
		"QS5jcmwwPQYDVR0gBDYwNDAyBgRVHSAAMCowKAYIKwYBBQUHAgEWHGh0dHBzOi8v\r\n"
		"d3d3LmRpZ2ljZXJ0LmNvbS9DUFMwHQYDVR0OBBYEFA+AYRyCMWHVLyjnjUY4tCzh\r\n"
		"xtniMB8GA1UdIwQYMBaAFAPeUDVW0Uy7ZvCj4hsbw5eyPdFVMA0GCSqGSIb3DQEB\r\n"
		"CwUAA4IBAQAjPt9L0jFCpbZ+QlwaRMxp0Wi0XUvgBCFsS+JtzLHgl4+mUwnNqipl\r\n"
		"5TlPHoOlblyYoiQm5vuh7ZPHLgLGTUq/sELfeNqzqPlt/yGFUzZgTHbO7Djc1lGA\r\n"
		"8MXW5dRNJ2Srm8c+cftIl7gzbckTB+6WohsYFfZcTEDts8Ls/3HB40f/1LkAtDdC\r\n"
		"2iDJ6m6K7hQGrn2iWZiIqBtvLfTyyRRfJs8sjX7tN8Cp1Tm5gr8ZDOo0rwAhaPit\r\n"
		"c+LJMto4JQtV05od8GiG7S5BNO98pVAdvzr508EIDObtHopYJeS4d60tbvVS3bR0\r\n"
		"j6tJLp07kzQoH3jOlOrHvdPJbRzeXDLz\r\n"
		"-----END CERTIFICATE-----\r\n";

// mqtt client handle
mqtt_client_t* pClientHandle = NULL;

// mqtt client parameters
mqtt_client_config_t clientConfig;

//typedef struct _mqtt_tls_param_t {
//	const unsigned char *ca_cert;	/* CA certificate, common between client and MQTT Broker */
//	const unsigned char *cert;	/* Client certificate */
//	const unsigned char *key;	/* Client private key */
//	int ca_cert_len;			/* the length of CA certificate  */
//	int cert_len;				/* the length of Client certificate */
//	int key_len;				/* the length of key */
//} mqtt_tls_param_t;

mqtt_tls_param_t clientTls;

//int blinkerValue = 0;
int currentLED = 0;

struct ntpc_server_conn_s g_server_conn[2];

const unsigned char *get_ca_cert(void) {
	return (const unsigned char*)mqtt_ca_cert_str;
}

// mqtt client on connect callback
void onConnect(void* client, int result) {
    printf("mqtt client connected to the server\n");
}

// mqtt client on disconnect callback
void onDisconnect(void* client, int result) {
    printf("mqtt client disconnected from the server\n");
}

// mqtt client on publish callback
void onPublish(void* client, int result) {
   printf("mqtt client Published message\n");
}

// Write the value of given gpio port.
void gpio_write(int port, int value) {
    char str[4];
    static char devpath[16];
    snprintf(devpath, 16, "/dev/gpio%d", port);
    int fd = open(devpath, O_RDWR);

    ioctl(fd, GPIOIOC_SET_DIRECTION, GPIO_DIRECTION_OUT);
    write(fd, str, snprintf(str, 4, "%d", value != 0) + 1);

    close(fd);
}

//�ּҰ���ȯ �Լ�
uint8_t min(uint8_t a, uint8_t b){
     if (a < b )
          return a;
     else return b;
}

void AMG8833_initialize(void)
{
	uint8_t data[2];
	int ret;

	//Power Control Register
	data[0]=0x00;
	data[1]=0x00;

	i2c_write(i2c_dev, &configs, data, 2);

	//Reset Register
	data[0]=0x01;
	data[1]=0x3f;

	i2c_write(i2c_dev, &configs, data, 2);

	//Frame Rate
	data[0]=0x02;
	data[1]=0x00;

	i2c_write(i2c_dev, &configs, data, 2);

	//Interrupt Setup
	data[0]=0x03;
	data[1]=0x00;

	i2c_write(i2c_dev, &configs, data, 2);

	up_mdelay(1000);
}


// Utility function to configure mqtt client
void initializeConfigUtil(void) {
    uint8_t macId[IFHWADDRLEN];
    int result = netlib_getmacaddr("wl1", macId);
    if (result < 0) {
        printf("Get MAC Address failed. Assigning \
                Client ID as 123456789");
        clientConfig.client_id =
                DEFAULT_CLIENT_ID; // MAC id Artik 053
    } else {
    printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
            ((uint8_t *) macId)[0],
            ((uint8_t *) macId)[1], ((uint8_t *) macId)[2],
            ((uint8_t *) macId)[3], ((uint8_t *) macId)[4],
            ((uint8_t *) macId)[5]);
    char buf[12];
    sprintf(buf, "%02x%02x%02x%02x%02x%02x",
            ((uint8_t *) macId)[0],
            ((uint8_t *) macId)[1], ((uint8_t *) macId)[2],
            ((uint8_t *) macId)[3], ((uint8_t *) macId)[4],
            ((uint8_t *) macId)[5]);
    clientConfig.client_id = buf; // MAC id Artik 053
    printf("Registering mqtt client with id = %s\n", buf);
    }

    clientConfig.user_name = device_id;
    clientConfig.password = device_token;
    clientConfig.debug = true;
    clientConfig.on_connect = (void*) onConnect;
    clientConfig.on_disconnect = (void*) onDisconnect;
    //clientConfig.on_message = (void*) onMessage;
    clientConfig.on_publish = (void*) onPublish;

    clientConfig.protocol_version = MQTT_PROTOCOL_VERSION_311;
    clientConfig.clean_session = true;

    clientTls.ca_cert = get_ca_cert();
    clientTls.ca_cert_len = sizeof(mqtt_ca_cert_str);
    clientTls.cert = NULL;
    clientTls.cert_len = 0;
    clientTls.key = NULL;
    clientTls.key_len = 0;

    clientConfig.tls = &clientTls;
}

static void ntp_link_error(void)
{
	printf("ntp_link_error() callback is called.\n");
}



int main(int argc, FAR char *argv[])
{

	//���� �︲
	gpio_write(52, 1);
	//led ���
	//gpio_write(57, 0);
    bool wifiConnected = false;
    gpio_write(RED_ON_BOARD_LED, 1); // Turn on on board Red LED to indicate no WiFi connection is established

    strTopicMsg = (char*)malloc(sizeof(char)*256);
    strTopicAct = (char*)malloc(sizeof(char)*256);
    sprintf(strTopicMsg, "/v1.1/messages/%s", device_id);
    sprintf(strTopicAct, "/v1.1/actions/%s", device_token);

    memset(&clientConfig, 0, sizeof(clientConfig));
    memset(&clientTls, 0, sizeof(clientTls));

    // for NTP Client
    memset(&g_server_conn, 0, sizeof(g_server_conn));
    g_server_conn[0].hostname = "0.asia.pool.ntp.org";
    g_server_conn[0].port = 123;
    g_server_conn[1].hostname = "1.asia.pool.ntp.org";
    g_server_conn[1].port = 123;

#ifdef CONFIG_CTRL_IFACE_FIFO
    int ret;

    while(!wifiConnected) {
        ret = mkfifo(CONFIG_WPA_CTRL_FIFO_DEV_REQ, CONFIG_WPA_CTRL_FIFO_MK_MODE);
        if (ret != 0 && ret != -EEXIST) {
            printf("mkfifo error for %s: %s", CONFIG_WPA_CTRL_FIFO_DEV_REQ, strerror(errno));
        }
        ret = mkfifo(CONFIG_WPA_CTRL_FIFO_DEV_CFM, CONFIG_WPA_CTRL_FIFO_MK_MODE);
        if (ret != 0 && ret != -EEXIST) {
            printf("mkfifo error for %s: %s", CONFIG_WPA_CTRL_FIFO_DEV_CFM, strerror(errno));
        }

        ret = mkfifo(CONFIG_WPA_MONITOR_FIFO_DEV, CONFIG_WPA_CTRL_FIFO_MK_MODE);
        if (ret != 0 && ret != -EEXIST) {
            printf("mkfifo error for %s: %s", CONFIG_WPA_MONITOR_FIFO_DEV, strerror(errno));
        }
    #endif

        if (start_wifi_interface() == SLSI_STATUS_ERROR) {
            printf("Connect Wi-Fi failed. Try Again.\n");
        }
        else {
            wifiConnected = true;
            gpio_write(RED_ON_BOARD_LED, 0); // Turn off Red LED to indicate WiFi connection is established
        }
    }

    printf("Connect to Wi-Fi success\n");

    bool mqttConnected = false;
    bool ipObtained = false;
    printf("Get IP address\n");

    struct dhcpc_state state;
    void *dhcp_handle;

    while(!ipObtained) {
        dhcp_handle = dhcpc_open(NET_DEVNAME);
        ret = dhcpc_request(dhcp_handle, &state);
        dhcpc_close(dhcp_handle);

        if (ret != OK) {
            printf("Failed to get IP address\n");
            printf("Try again\n");
            sleep(1);
        }
        else {
            ipObtained = true;
        }
    }
    netlib_set_ipv4addr(NET_DEVNAME, &state.ipaddr);
    netlib_set_ipv4netmask(NET_DEVNAME, &state.netmask);
    netlib_set_dripv4addr(NET_DEVNAME, &state.default_router);

    printf("IP address  %s\n", inet_ntoa(state.ipaddr));

    up_mdelay(2000);

    int ret_ntp = ntpc_start(g_server_conn, 2, 1000, ntp_link_error);
    printf("ret: %d\n", ret_ntp);

    // Connect to the WiFi network for Internet connectivity
    printf("mqtt client tutorial\n");

    // Initialize mqtt client
    initializeConfigUtil();

    pClientHandle = mqtt_init_client(&clientConfig);
    if (pClientHandle == NULL) {
        printf("mqtt client handle initialization fail\n");
        return 0;
    }

    while (mqttConnected == false ) {
        sleep(2);
        // Connect mqtt client to server
        int result = mqtt_connect(pClientHandle, SERVER_ADDR, SERVER_PORT, 60);

        if (result == 0) {
            mqttConnected = true;
            printf("mqtt client connected to server\n");
            break;
        } else {
            continue;
        }
    }

    bool mqttSubscribe = false;

    //AMG3388

	int i=0, j=0, cnt=0, port =0;
	float before_Buf[64];
	float current_Buf[64];
	//float temper;
	float temp;
	float before_Temp=0, current_Temp=0;
	float converted;
	char buf[40];
	uint8_t bytesToRead=128;
	uint8_t rawArray[128];
	uint8_t reg;
	uint8_t data[2];
	uint8_t size;
	uint16_t recast;
	uint16_t absVal;
	uint8_t pos;

	float signedMag12ToFloat;

	i2c_dev = up_i2cinitialize(port);
	if (i2c_dev == NULL) {
		printf("up_i2cinitialize failed(i2c:%d)\n", port);
		fflush(stdout);
	}

	configs.frequency = 100000;
	configs.address = AMG8833_ADDRESS;
	configs.addrlen = 7;

	AMG8833_initialize();

	//��տµ� data read
	reg=0x0E;
	i2c_write(i2c_dev, &configs, &reg, 1);
	i2c_read(i2c_dev, &configs, data, 2);
	recast=((uint16_t)data[1]<<8) | ((uint16_t)data[0]);

	//signedMag12ToFloat
	absVal=(recast&0x7FF);

	if(recast & 0x8000){
		signedMag12ToFloat=(0-(float)absVal);
	}else{
		signedMag12ToFloat=(float)absVal;
	}

	printf("%f \n", signedMag12ToFloat*0.0625);
	before_Temp=signedMag12ToFloat*0.0625;
	up_mdelay(1000);

	//pixel data read
	reg=0x80;
	i2c_write(i2c_dev, &configs, &reg, 1);
	i2c_read(i2c_dev, &configs, rawArray, 128);

	for(i=0; i<64; i++){
		pos=i<<1;
		recast=((uint16_t)rawArray[pos+1] <<8) | ((uint16_t)rawArray[pos]);

		//signedMag12ToFloat
		absVal=(recast&0x7FF);

		if(recast & 0x8000){
			signedMag12ToFloat=(0-(float)absVal);
		}else{
			signedMag12ToFloat=(float)absVal;
		}

		converted=signedMag12ToFloat*0.25;
		before_Buf[i]=converted;
	}

	while(1){
		gpio_write(51, 1);
		sleep(1);
		gpio_write(51, 0);
		sleep(30);

		//��տµ� data read
		reg=0x0E;
		i2c_write(i2c_dev, &configs, &reg, 1);
		i2c_read(i2c_dev, &configs, data, 2);
		recast=((uint16_t)data[1]<<8) | ((uint16_t)data[0]);

		//signedMag12ToFloat
		absVal=(recast&0x7FF);

		if(recast & 0x8000){
			signedMag12ToFloat=(0-(float)absVal);
		}else{
			signedMag12ToFloat=(float)absVal;
		}

		//printf("%f \n", signedMag12ToFloat*0.0625);
		current_Temp=signedMag12ToFloat*0.0625;
		up_mdelay(1000);

		//pixel data read
		reg=0x80;
		i2c_write(i2c_dev, &configs, &reg, 1);
		i2c_read(i2c_dev, &configs, rawArray, 128);

		for(i=0; i<64; i++){
			pos=i<<1;
			recast=((uint16_t)rawArray[pos+1] <<8) | ((uint16_t)rawArray[pos]);

			//signedMag12ToFloat
			absVal=(recast&0x7FF);

			if(recast & 0x8000){
				signedMag12ToFloat=(0-(float)absVal);
			}else{
				signedMag12ToFloat=(float)absVal;
			}

			converted=signedMag12ToFloat*0.25;
			current_Buf[i]=converted;
		}

		//�� �迭 ��
		for(i=0;i<64;i++){
			if( (current_Buf[i]-before_Buf[i]) >= 10) cnt++;
		}

		printf("*************************\n");
		printf("before_Temp: %f \n", before_Temp);

		printf("before pixel: \n");
		for(i=1; i<=64; i++){
			printf("%f", before_Buf[i-1]);
			printf(",");
			if(i%8==0) printf("\n");
		}
		printf("\n");

		printf("current_Temp: %f \n", current_Temp);

		printf("current pixel: \n");
		for(i=1; i<=64; i++){
			printf("%f", current_Buf[i-1]);
			printf(",");
			if(i%8==0) printf("\n");
		}
		printf("\n");

		printf("symptom: %d\n", cnt);
		printf("*************************\n\n");

	   before_Temp=current_Temp;
		//current_Buf�� ���� before_Buf�� ����

	   memset(before_Buf, 0, 64);
	   fflush(stdin);

	   for(i=0; i<64; i++){
		   before_Buf[i]=current_Buf[i];
	   }


		/************************�̻�¡�� ����********************************/

			//�̻�¡�� ���� �ķκ��� �µ���·� �Ǵ�

				//�̻�¡�� 7�̻�-> fire_state: Level-3
		if(cnt>6){

			printf("symptom: %d\n", cnt);
			printf("fire_state: Level-3\n\n");

			sprintf(buf, "{\"fire_state\" :\"Level-3\" ,\"temp\" : %f, \"symptom\" : %d }", current_Temp ,cnt);

			// construct the mqtt message to be published
			mqtt_msg_t message;
			message.payload = (char*)buf;
			message.payload_len = strlen(buf);
			message.topic = strTopicMsg;
			message.qos = 0;
			message.retain = 0;

			ret = mqtt_publish(pClientHandle, message.topic, (char*)message.payload, message.payload_len, message.qos, message.retain);


			gpio_write(51, 1);
			//ȭ��߻��� ���� �� LED setOn
			gpio_write(52, 0);
		}
		//�̻�¡�� 4�̻� 7�̸�-> fire_state: Level-2
		else if(cnt>3){


			//sleep(1);
			//gpio_write(52, 1);

			//while(1){
				//gpio_write(52, 0);
				//sleep(1);
				//gpio_write(52, 1);
				//sleep(1);

			printf("symptom: %d\n", cnt);
			printf("fire_state: Level-2\n\n");

			//mqtt publish �̻�¡��cnt, ��տµ�
			sprintf(buf, "{\"fire_state\" :\"Level-2\" ,\"temp\" : %f, \"symptom\" : %d }", current_Temp ,cnt);

			// construct the mqtt message to be published
			mqtt_msg_t message;
			message.payload = (char*)buf;
			message.payload_len = strlen(buf);
			message.topic = strTopicMsg;
			message.qos = 0;
			message.retain = 0;

			ret = mqtt_publish(pClientHandle, message.topic, (char*)message.payload, message.payload_len, message.qos, message.retain);

			gpio_write(51, 1);
			//ȭ��߻��� ���� �� LED setOn
			gpio_write(52, 0);

			//}
			//���� �︲
			//gpio_write(51, 1);
			//led ���
			//gpio_write(57, 1);
			//printf("fire Alarm!!\n");
		}
		else if(cnt>0)
		{
			printf("symptom: %d\n", cnt);
			printf("fire_state: Level-1\n\n");

			//mqtt publish �̻�¡��cnt, ��տµ�
			sprintf(buf, "{\"fire_state\" :\"Level-1\", \"temp\" : %f, \"symptom\" : %d }", current_Temp ,cnt);
			//printf("symptom: %d\n",);
			//char* msg = buf;
			// construct the mqtt message to be published
			mqtt_msg_t message;
			message.payload = (char*)buf;
			message.payload_len = strlen(buf);
			message.topic = strTopicMsg;
			message.qos = 0;
			message.retain = 0;

			ret = mqtt_publish(pClientHandle, message.topic, (char*)message.payload, message.payload_len, message.qos, message.retain);
		}else{

			//mqtt publish �̻�¡��cnt, ��տµ�
	        sprintf(buf, "{\"fire_state\" :\"Level-0\" ,\"temp\" : %f, \"symptom\" : %d }", current_Temp ,cnt);
	        //printf("symptom: %d\n",);
	        //char* msg = buf;
	        // construct the mqtt message to be published
	        mqtt_msg_t message;
	        message.payload = (char*)buf;
	        message.payload_len = strlen(buf);
	        message.topic = strTopicMsg;
	        message.qos = 0;
	        message.retain = 0;

	        ret = mqtt_publish(pClientHandle, message.topic, (char*)message.payload, message.payload_len, message.qos, message.retain);

		}

	}


}
