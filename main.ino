#include <Arduino.h>

#include <SPI.h>
#include <SoftwareSerial.h>

#include <Wire.h>
#include <RTClib.h>

#include <DHT.h>
#define DHT_PIN 5
#define DHTTYPE DHT11

#include <Adafruit_BMP280.h>


// initialisation du périph, pin 7 réception, pin 8 envoie
SoftwareSerial GPS(7, 8);
int interval_GPS = 0;

// Buffer de communication et son compteur
char buffer[80];
int count=0; 

void clearBufferArray()                     // function to clear buffer array
{
    for (int i=0; i<count;i++)
    {
        buffer[i]=NULL;
    }                      // clear all index of array with command NULL
}

//------

// Instance du RTC
RTC_DS1307 RTC;


//Converti le numéro de jour en jour /!\ la semaine commence un dimanche
String donne_jour_semaine(uint8_t j){ 
  switch(j){
   case 0: return "DIM";
   case 1: return "LUN";
   case 2: return "MAR";
   case 3: return "MER";
   case 4: return "JEU";
   case 5: return "VEN";
   case 6: return "SAM";
   default: return "   ";
  }
}

//permet d'afficher les nombres sur deux chiffres
String Vers2Chiffres(byte nombre) {
  String resultat = "";
  if(nombre < 10)
    resultat = "0";
  return resultat += String(nombre,DEC);  
}

// affiche la date et l'heure sur l'écran
void affiche_date_heure(DateTime datetime){

  // Date 
  String jour = donne_jour_semaine(datetime.dayOfTheWeek()) + " " + 
                Vers2Chiffres(datetime.day())+ "/" + 
                Vers2Chiffres(datetime.month())+ "/" + 
                String(datetime.year(),DEC);
  
  // heure
  String heure = "";
  heure  = Vers2Chiffres(datetime.hour())+ ":" + 
           Vers2Chiffres(datetime.minute())+ ":" + 
           Vers2Chiffres(datetime.second());

  //affichage sur l'écran
  Serial.println(jour + " | " + heure);
}

//Instance DHT(Température et humidité)
DHT dht(DHT_PIN, DHTTYPE);

//Instance de la lumière
int lum_PIN = A1;
int lum_val;

//Instance du BMP(Pression)
Adafruit_BMP280 bmp;

// -----

void setup() {
  
  Serial.begin(9600);

 // GPS
  if (!GPS.begin(9600)) gestionnaire_erreur(ERR_GPS);

  //DHT
  if (!DHT.begin()) gestionnaire_erreur(ERR_CAPTEUR_ACCES);

  //BMP
  if (!BMP.begin(0x76)) gestionnaire_erreur(ERR_CAPTEUR_ACCES);
  //---

  // RTC
  Wire.begin();
  if (!RTC.begin()) gestionnaire_erreur(ERR_CAPTEUR_ACCES);

  // RTC
  Wire.begin();
  RTC.begin(); 
  /*Initialise la date et le jour au moment de la compilation 
  // /!\ /!\ Les lignes qui suivent servent à définir la date et l'heure afin de régler le module, 
  // pour les montages suivant il ne faut surtout PAS la mettre, sans à chaque démarrage 
  // le module se réinitialisera à la date et heure de compilation
  
  DateTime dt = DateTime(__DATE__, __TIME__);
  RTC.adjust(dt);
  
  // /!\

*/
Serial.print("\n");
  DateTime now=RTC.now();

  affiche_date_heure(now);
  //---

}

void loop() {
  long watchdog = millis() + 10000;
    byte buffer = 0;
    char values[5];
    char latitude[16];
    char longitude[16];
    // on attend essaie pendant 10 secondes de récupérer la variable de la position
    while (watchdog > millis()) {
        // attente du début de variable $
        while (GPS.read() != 36) {yield();} // (char)36 <=> $
        for (int i = 0; i < 5; i++) {
            while (!GPS.available() && watchdog > millis()) {yield();} // wait for char to get
            values[i] = GPS.read();
        }
        if ((String)values == "GPRMC") {
            break;
        }
    }
    free(values);
    // préparation du tableau à stocker
    float* coordonnee = (float*)calloc(2, sizeof(float));

    // Une fois la variable trouvée, on vérfie on décale sur la latitude
    byte commaCounter = 0;
    while (!GPS.available() && watchdog > millis()) {yield();} // wait for char to get
    char caractere = GPS.read();
    while (commaCounter < 3 && watchdog > millis()) {
        if (caractere == ',') {commaCounter++;}
        while (!GPS.available() && watchdog > millis()) {yield();} // wait for char to get
        caractere = GPS.read();
    }
    // On vérfie que la latitude existe
    latitude[buffer] = caractere;
    if (latitude[0] != ',') {
        while (latitude[buffer++] != ',' && watchdog > millis()) {
            while (!GPS.available() && watchdog > millis()) {yield();} // wait for char to get
            latitude[buffer] = GPS.read();
        }
        latitude[buffer-1] = '\0';
        coordonnee[0] = atof(latitude);
        //free(latitude);

        // je retire l'unité
        for (int i = 0; i <2; i++) {
            while (!GPS.available() && watchdog > millis()) {yield();} // wait for char to get
            GPS.read();
        }

        // On récupère la valeur de la longitude
        buffer = 0;
        while (!GPS.available() && watchdog > millis()) {yield();} // wait for char to get
        longitude[buffer] = GPS.read();
        while (longitude[buffer++] != ',' && watchdog > millis()) {
            while (!GPS.available() && watchdog > millis()) {yield();} // wait for char to get
            longitude[buffer] = GPS.read();
        }
        longitude[buffer-1] = '\0';
        coordonnee[1] = atof(longitude);
        free(longitude);
    }
  Serial.print("\nLatitude: ");
  Serial.println(coordonnee[0]);
  Serial.print("Longitude: ");
  Serial.println(coordonnee[1]);


  //DHT
  Serial.print("température:");
  Serial.println(dht.readTemperature());
  Serial.print("humidité:");
  Serial.println(dht.readHumidity());

  //Lumière
  lum_val = analogRead(lum_PIN);
  Serial.print("Valeur luminosité :");
  Serial.println(lum_val);

  //Pression
  Serial.print("Pression:");
  Serial.println(bmp.readPressure() / 100);

   //Temperature eau
  Serial.print("Temperature eau : ");
  float t_eau = rand()%30;
  Serial.print(t_eau);
  Serial.println(" °C");

  //courant marin
  Serial.print("Courant marin : ");
  float c_marin = rand()%5;
  Serial.print(c_marin);
  Serial.println(" noeud");

  //Force du vent
  Serial.print("Force du vent : ");
  float f_vent = rand()%30;
  Serial.print(f_vent);
  Serial.println(" metre/seconde");

  //Taux de particule fine
  Serial.print("Taux de particule fine : ");
  float partic = rand()%200;
  Serial.print(partic);
  Serial.println(" microgramme/metre^3");

  if(dht.readTemperature() > 40 || dht.readTemperature() < -10 || lum_val > 25000 || lum_val < 0 || bmp.readPressure()/100 < 900||
   bmp.readPressure()/100 > 1050 || t_eau > 29 || c_marin >4 || f_vent >28 || partic > 170 ) 
      gestionnaire_erreur(ERR_CAPTEUR_INCOHERENTE);

  delay(2000);
}
