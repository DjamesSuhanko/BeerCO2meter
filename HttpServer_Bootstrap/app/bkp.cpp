#include <user_config.h>
#include <SmingCore/SmingCore.h>

// If you want, you can define WiFi settings globally in Eclipse Environment Variables
#define WIFI_SSID "SuhankoFamily" // Put you SSID and Password here
#define WIFI_PWD "fsjmr112"

#define LED_PIN 0 // GPIO number

#define ENGLISH   1.8
#define AMERICAN  2.5
#define BELGIAN   2.3
#define GERMANY   3.4
#define STRONG    3.8
#define DANGEROUS 4.0

HttpServer server;
int counter = 0;

//confirma se o led de alarme deve ficar ligado
bool activate_alarm = false;
//variavel que guarda a pressão em PSI
float current_pressure_psi = 0.0;
//temperatura em graus Celsius
float currentTemperature = 0.0;
//estilo monitorado
String beer_style = "Nenhum";
//carbonatacao selecionada
float CO2 = 0.0;
//PSI necessário para atingir o objetivo
float psi_to_target = 0.0;

Vector <String> check_style;
Vector <float>  value_style;

void onIndex(HttpRequest &request, HttpResponse &response)
{
	counter++;
	bool led = request.getQueryParameter("led") == "on";
	//digitalWrite(LED_PIN, led);
        activate_alarm = led;
        
        beer_style = request.getQueryParameter("style");
        
        if (check_style.contains(beer_style)){
            CO2 = value_style.at(check_style.indexOf(beer_style));
            Serial.print("CO2: ");
            Serial.println(CO2);
        }

	TemplateFileStream *tmpl = new TemplateFileStream("index.html");
	auto &vars               = tmpl->variables();
        vars["counter"]          = String(counter);
	vars["IP"]               = WifiStation.getIP().toString();
        vars["PSI_CARBON"]       = String(current_pressure_psi);
        vars["TEMPERATURE"]      = String(currentTemperature); 
        vars["BEER_STYLE"]       = beer_style;
        vars["PSI_TO_TARGET"]    = psi_to_target;
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
	Serial.println("==============================\r\n");
}

Timer downloadTimer;
HttpClient downloadClient;
int dowfid = 0;
void downloadContentFiles()
{
	if (downloadClient.isProcessing()) return; // Please, wait.

	if (downloadClient.isSuccessful())
		dowfid++; // Success. Go to next file!
	downloadClient.reset(); // Reset current download status

	if (dowfid == 0)
	    //downloadClient.downloadFile("http://simple.anakod.ru/templates/index.html");
            downloadClient.downloadFile("http://192.168.1.7/index.html");
	else if (dowfid == 1)
	    //downloadClient.downloadFile("http://simple.anakod.ru/templates/bootstrap.css.gz");
            downloadClient.downloadFile("http://192.168.1.7/bootstrap.css.gz");
	else if (dowfid == 2)
	    //downloadClient.downloadFile("http://simple.anakod.ru/templates/jquery.js.gz");
            downloadClient.downloadFile("http://192.168.1.7/jquery.js.gz");
	else
	{
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

void init()
{
    check_style.add("english");
    check_style.add("american");
    check_style.add("belgian");
    check_style.add("high");
    check_style.add("high");
    check_style.add("dangerous");
    
    value_style.add(ENGLISH);
    value_style.add(AMERICAN);
    value_style.add(BELGIAN);
    value_style.add(GERMANY);
    value_style.add(STRONG);
    value_style.add(DANGEROUS);
  
    pinMode(LED_PIN, OUTPUT);

    Serial.begin(115200); // 115200 by default
    Serial.systemDebugOutput(true); // Enable debug output to serial

    WifiStation.enable(true);
    WifiStation.config(WIFI_SSID, WIFI_PWD);
    WifiAccessPoint.enable(false);

    // Run our method when station was connected to AP
    WifiStation.waitConnection(connectOk);

    //Change CPU freq. to 160MHZ
    System.setCpuFrequency(eCF_160MHz);
    Serial.print("New CPU frequency is:");
    Serial.println((int)System.getCpuFrequency());
}
