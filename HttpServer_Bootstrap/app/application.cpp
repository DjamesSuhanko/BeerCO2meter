#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <Libraries/BMP180/BMP180.h>

#define WIFI_SSID "SeuSSID" // Put you SSID and Password here
#define WIFI_PWD "SuaSenhaWiFi"

//1 Pascal = 0,000145038 PSI
#define PSI 0.000145038
#define BAR 0.0689476
#define VOL 2.5 //serve para a maioria dos estilos

#define LED_PIN 4 // D4 no Wemos D1 Mini

#define ENGLISH   1.8
#define AMERICAN  2.5
#define BELGIAN   2.3
#define GERMANY   3.4
#define STRONG    3.8
#define DANGEROUS 4.0

HttpServer server;
int counter = 0;

String carbonation_status  = "Em andamento";
bool activate_alarm        = false;//confirma se o led de alarme deve ficar ligado
float pressure_in_psi      = 0.0;//variavel que guarda a pressão em PSI
float currentTemperature   = 0.0;//temperatura em graus Celsius
String beer_style          = "Nenhum";//estilo monitorado
float CO2                  = 0.0;//carbonatacao selecionada
float pressure_target      = 0.0;//PSI necessário para atingir o objetivo
float farenheit            = 0.0;//guarda a conversão de Celsius para Fahrenheit

BMP180 barometer;//instância do barômetro
Timer getValues;//timer de leitura do BMP180

void ap_if();

float calculateCO2(float volume){
    float pressure_needed = -16.6999 - 0.0101059 * farenheit + 0.00116512 * pow(farenheit,2) + 0.173354 * farenheit * volume + 4.24267 * volume - 0.0684226 * pow(volume,2);
    return pressure_needed;
}

void bmp180CB(){
    //pressao em hPa
    float currentPressure = (float) barometer.GetPressure()/100.0;

    //converte para PSI
    pressure_in_psi = (float) currentPressure*PSI*100.0;
    float pressure_in_bar = (float) currentPressure/1000.0;

    //Temperatura em graus Celsius
    currentTemperature = barometer.GetTemperature();

    Serial.print("\tTemperaturas: ");
    Serial.print(currentTemperature);
    Serial.print(" C (");

    //converte Celsius para Fahrenheit
    farenheit = 1.8*currentTemperature+32;
    
    pressure_target = calculateCO2(CO2);
    
    Serial.print(farenheit);
    Serial.println(" F)");

    Serial.print("Pressao atual em hPa: ");
    Serial.println(currentPressure);
    Serial.print("Pressao atual em PSI: ");
    Serial.println(pressure_in_psi);
    Serial.print("Pressao atual em BAR: ");
    Serial.println(pressure_in_bar);
    Serial.print("Pressao ideal para o estilo: ");
    Serial.print(pressure_target);
    Serial.println(" PSI");
    Serial.println("-------------------");
    
    if (pressure_target <= pressure_in_psi && pressure_target > 0){
        if (activate_alarm){
            digitalWrite(LED_PIN,HIGH);
        }
        carbonation_status = "Concluido";
    }
    else if (pressure_target < 0){
        carbonation_status = "Não iniciado";
    }
    else{
        carbonation_status = "Em andamento";
    }
}

void onIndex(HttpRequest &request, HttpResponse &response)
{
    counter++;
    bool led = request.getQueryParameter("led") == "on";
    //digitalWrite(LED_PIN, led);
    activate_alarm = led;
    
    beer_style = request.getQueryParameter("style");

    if (beer_style == "english"){
        CO2 = ENGLISH;
    }
    else if (beer_style == "american"){
        CO2 = AMERICAN;
    }
    else if (beer_style == "germany"){
        CO2 = GERMANY;
    }
    else if (beer_style == "belgian"){
        CO2 = BELGIAN;
    }
    else if (beer_style ==  "high"){
        CO2 = STRONG;
    }
    else if (beer_style == "dangerous"){
        CO2 = DANGEROUS;
    }
    bmp180CB();

    TemplateFileStream *tmpl = new TemplateFileStream("index.html");
    auto &vars               = tmpl->variables();
    vars["counter"]          = String(counter);
    vars["IP"]               = WifiStation.getIP().toString();
    vars["PSI_CARBON"]       = String(pressure_in_psi);
    vars["TEMPERATURE"]      = String(currentTemperature); 
    vars["BEER_STYLE"]       = beer_style;
    vars["PSI_TO_TARGET"]    = pressure_target;
    vars["CARB_STATUS"]      = carbonation_status;
    if (activate_alarm){
        vars["LED_STATUS"]   = "ON";
    }
    else{
        vars["LED_STATUS"]   = "OFF";
    }
    response.sendTemplate(tmpl); // this template object will be deleted automatically
}

void onHello(HttpRequest &request, HttpResponse &response)
{
	response.setContentType(ContentType::HTML);
	// Use direct strings output only for small amount of data (huge memory allocation)
	response.sendString("Sming. Let's do smart things.");
}

void onFile(HttpRequest &request, HttpResponse &response)
{
	String file = request.getPath();
	if (file[0] == '/')
		file = file.substring(1);

	if (file[0] == '.')
		response.forbidden();
	else
	{
		response.setCache(86400, true); // It's important to use cache for better performance.
		response.sendFile(file);
	}
}

void startWebServer()
{
	server.listen(80);
	server.addPath("/", onIndex);
	server.addPath("/hello", onHello);
	server.setDefaultHandler(onFile);

	Serial.println("\r\n=== WEB SERVER STARTED ===");
	Serial.println(WifiStation.getIP());
        //Serial.println(WifiAccessPoint.getIP());
	Serial.println("==============================\r\n");
}

Timer downloadTimer;
HttpClient downloadClient;
int dowfid = 0;
void downloadContentFiles(){
    if (downloadClient.isProcessing()){
        return; // Please, wait.  
    }  

    if (downloadClient.isSuccessful()){
        dowfid++; // Success. Go to next file!
    }

    downloadClient.reset(); // Reset current download status

    if (dowfid == 0){
        downloadClient.downloadFile("http://192.168.1.7/index.html");
    }
    else if (dowfid == 1){
        downloadClient.downloadFile("http://192.168.1.7/bootstrap.css.gz");
    }
    else if (dowfid == 2){
        downloadClient.downloadFile("http://192.168.1.7/jquery.js.gz");
    }
    else{
            // Content download was completed
            downloadTimer.stop();   
            startWebServer();
    }
}

// Will be called when WiFi station was connected to AP
void connectOk()
{
	Serial.println("I'm CONNECTED");

	if (!fileExist("index.html") || !fileExist("bootstrap.css.gz") || !fileExist("jquery.js.gz"))
	{
		// Download server content at first
		downloadTimer.initializeMs(3000, downloadContentFiles).start();
	}
	else
	{
	    startWebServer();
	}
}

void sta_if(){
    WifiStation.enable(true);
    WifiStation.config(WIFI_SSID, WIFI_PWD);
    WifiAccessPoint.enable(false);

    // Run our method when station was connected to AP
    WifiStation.waitConnection(connectOk);
}

void ap_if(){
    WifiStation.disconnect();
    WifiStation.enable(false);
    WifiAccessPoint.enable(true);
    WifiAccessPoint.config("BeerCO2","dobitaobyte",AUTH_WPA2_PSK,false,11,2);
}
void init(){
    pinMode(LED_PIN, OUTPUT);

    Serial.begin(115200); // 115200 by default
    Serial.systemDebugOutput(true); // Enable debug output to serial
    
    //loadVectors();
    
    sta_if();

    Wire.begin();
    
    if(!barometer.EnsureConnected()){
        Serial.println("Nao pude me conectar ao BMP180.");
    }
    
    barometer.Initialize();
    barometer.PrintCalibrationData();
    
    //Change CPU freq. to 160MHZ
    System.setCpuFrequency(eCF_160MHz);
    Serial.print("New CPU frequency is:");
    Serial.println((int)System.getCpuFrequency());
    
    Serial.print("Iniciando leitura");
    getValues.initializeMs(60000,bmp180CB).start();
}
