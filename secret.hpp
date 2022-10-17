// WiFi（2.4G帯のみ対応）のSSID, PASSWORDを入力

#define WIFI_SSID ""
#define WIFI_PASSWORD ""

// CO2測定用
// #define LOOP_TIME_INTERVAL_MS 1000
// #define BASELINE_IS_STORED_FLAG (0X55)
//#define ARRAY_TO_U32(a)  (a[0]<<24|a[1]<<16|a[2]<<8|a[3])    //MSB first  //Not suitable for 8-bit platform

// AWS IoT Coreのエンドポイントをコンソール左の設定から確認し入力
#define CLOUD_ENDPOINT ""
#define DEVICE_NAME ""  // デバイス証明書作成時に登録したデバイス名（モノ）
#define CLOUD_PORT 8883 //この番号が定番らしい
#define CLOUD_TOPIC1 "" // AWS IoTで送信したデータを確認・ソートする際に用いる
#define CLOUD_TOPIC2 "" // lambdaに送るために別途作成するルール（ルール2）用

//証明書作成時は
// 1. ポリシー
// 2. モノ
// 3. 証明書の順に作成する
// 詳しくは https://aws.amazon.com/jp/builders-flash/202101/iot-patlite/?awsf.filter-name=*all#

// ダウンロードした AWS IoT のルート CA 証明書をダウンロードしテキストで開いてコピペ
// 以下3つは作成時しかダウンロードできないので注意
// CA証明書はおそらく同じ内容 https://www.amazontrust.com/repository/AmazonRootCA1.pem を参照。
const char *ROOT_CA = R"(-----BEGIN CERTIFICATE-----
-----END CERTIFICATE-----
)";
// AWS IoT からダウンロードした XXXXXXXXXX-certificate.pem.crt の情報を同様にコピペ
const char *CERTIFICATE = R"(-----BEGIN CERTIFICATE-----
-----END CERTIFICATE-----
)";
// AWS IoT からダウンロードした XXXXXXXXXX-private.pem.key の情報を⽤いて更新
const char *PRIVATE_KEY = R"(-----BEGIN RSA PRIVATE KEY-----
-----END RSA PRIVATE KEY-----
)";
