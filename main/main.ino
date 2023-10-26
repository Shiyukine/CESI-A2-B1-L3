#define ERR_SD_PLEIN 0x0
#define ERR_SD_IO 0x1
#define ERR_RTC 0x2
#define ERR_CAPTEUR_ACCES 0x3
#define ERR_CAPTEUR_INCOHERENTE 0x4
#define ERR_GPS 0x5

#define MODE_STANDARD 0x0
#define MODE_CONFIGURATION 0x1
#define MODE_MAINTENANCE 0x2
#define MODE_ECO 0x3

#define DEFAULT_LOG_INTERVAL 10
#define DEFAULT_FILE_MAX_SIZE 2048
#define DEFAULT_TIMEOUT_CAPTEUR 30
#define DEFAULT_ACTIVATION 1
#define DEFAULT_LUMIN_LOW 255
#define DEFAULT_LUMIN_HIGH 768
#define DEFAULT_MIN_TEMP -10
#define DEFAULT_MAX_TEMP 60
#define DEFAULT_HYGR_MINT 0
#define DEFAULT_HYGR_MAXT 50
#define DEFAULT_PRESSURE_MIN 850
#define DEFAULT_PRESSURE_MAX 1080

#define LUMIN_PORT TODO
#define TEMP_PORT TODO
#define HYGR_PORT TODO
#define PRESSURE_PORT TODO

#define CAPTEUR_TYPE_LUMIN 0x0
#define CAPTEUR_TYPE_TEMP 0x1
#define CAPTEUR_TYPE_HYGR 0x2
#define CAPTEUR_TYPE_PRESSURE 0x3
#define CAPTEUR_TYPE_GPS 0x4
#define CAPTEUR_TYPE_PARTICLE 0x5
#define CAPTEUR_TYPE_TEMP_EAU 0x6
#define CAPTEUR_TYPE_VENT 0x7
#define CAPTEUR_TYPE_COURANTS 0x8

#define button1 2
#define button2 3

#define V 9
#define R 10
#define B 11

#include <RTClib.h>
#include <SdFat.h>
#include <Arduino.h>

#include <SPI.h>
#include <SoftwareSerial.h>

#include <Wire.h>
#include <RTClib.h>

#include <DHT.h>
#define DHT_PIN 5
#define DHTTYPE DHT11

#include <Adafruit_BMP280.h>

// to works with VSCode
#ifndef TCCR1B
extern int TCCR1A;
extern int TCCR1B;
extern int TIMSK1;
extern int OCR1A;
extern int TCNT1;
#endif

typedef struct // Structure de nos capteurs.
{
  int type;
  int min;
  int max;
  int timeout;
  bool actif;
  int nombre_erreur;
  int dernieres_valeurs[10];
  int tableau_valeurs_index;
  int moyenne;
} Capteur;

int log_interval;
float version = 1.0;
int num_lot;
int inactivite_config;
float config_OCR1A;
int prescaler;

int hygrometrie;
int temperature;
int pression;
int luminosite;
int particule;
int vent;
int temp_eau;
int courant;

int bouton_test;

int file_max_size = 2048;
int compteur_taille_fichier = 0;
int compteur_revision = 0;
int mode = 0;
Capteur *capteurs[9];
int year;
int month;
int day;
int previous_mode = -1;
int inactivite = 0;

int etatled = 0;
int code_couleur = 2;
int compteur_sec = 0;

SdFat32 *SD;
RTC_DS1307 *rtc;

// initialisation du périph, pin 7 réception, pin 8 envoie
SoftwareSerial GPS(7, 8);
int interval_GPS = 0;

// Buffer de communication et son compteur
char buffer[80];
int count = 0;

void clearBufferArray() // function to clear buffer array
{
  for (int i = 0; i < count; i++)
  {
    buffer[i] = NULL;
  } // clear all index of array with command NULL
}

//------

// Instance du RTC
RTC_DS1307 RTC;

// Converti le numéro de jour en jour /!\ la semaine commence un dimanche
String donne_jour_semaine(uint8_t j)
{
  switch (j)
  {
  case 0:
    return "DIM";
  case 1:
    return "LUN";
  case 2:
    return "MAR";
  case 3:
    return "MER";
  case 4:
    return "JEU";
  case 5:
    return "VEN";
  case 6:
    return "SAM";
  default:
    return "   ";
  }
}

// permet d'afficher les nombres sur deux chiffres
String Vers2Chiffres(byte nombre)
{
  String resultat = "";
  if (nombre < 10)
    resultat = "0";
  return resultat += String(nombre, DEC);
}

// affiche la date et l'heure sur l'écran
void affiche_date_heure(DateTime datetime)
{

  // Date
  String jour = donne_jour_semaine(datetime.dayOfTheWeek()) + " " +
                Vers2Chiffres(datetime.day()) + "/" +
                Vers2Chiffres(datetime.month()) + "/" +
                String(datetime.year(), DEC);

  // heure
  String heure = "";
  heure = Vers2Chiffres(datetime.hour()) + ":" +
          Vers2Chiffres(datetime.minute()) + ":" +
          Vers2Chiffres(datetime.second());

  // affichage sur l'écran
  Serial.println(jour + " | " + heure);
}

// Instance DHT(Température et humidité)
DHT dht(DHT_PIN, DHTTYPE);

// Instance de la lumière
int lum_PIN = A1;
int lum_val;

// Instance du BMP(Pression)
Adafruit_BMP280 bmp;

File *changement_fichier(int mess_size)
{
  DateTime *now = &rtc->now();
  static File actualFile;
  static bool firstcall = true;
  if (firstcall)
  {
    String aa = String(now->year());
    aa = String(aa[2]) + String(aa[3]);
    String mm = String(now->month());
    if (now->month() < 10)
      mm = "0" + mm;
    String jj = String(now->day());
    if (now->day() < 10)
      jj = "0" + jj;
    actualFile = SD->open(aa + mm + jj + "_" + String(compteur_revision) + ".LOG", O_RDWR | O_CREAT | O_TRUNC);
  }
  firstcall = false;
  if (!actualFile)
    gestionnaire_erreur(ERR_SD_IO);
  bool changeFile = false;
  if (actualFile.position() + mess_size > file_max_size)
  {
    compteur_revision++;
    changeFile = true;
    compteur_taille_fichier = 0;
  }
  if (year != now->year() && month != now->month() && day != now->day())
  {
    compteur_revision = 0;
    year = now->year();
    month = now->month();
    day = now->day();
    changeFile = true;
  }
  if (changeFile)
  {
    String aa = String(now->year());
    aa = String(aa[2]) + String(aa[3]);
    String mm = String(now->month());
    if (now->month() < 10)
      mm = "0" + mm;
    String jj = String(now->day());
    if (now->day() < 10)
      jj = "0" + jj;
    Serial.println("Copy to " + aa + mm + jj + "_" + String(compteur_revision) + ".LOG");
    File newFile = SD->open(aa + mm + jj + "_" + String(compteur_revision) + ".LOG", O_RDWR | O_CREAT | O_TRUNC);
    if (!newFile)
    {
      compteur_revision--;
      gestionnaire_erreur(ERR_SD_IO);
    }
    else
    {
      actualFile.seek(0);
      while (actualFile.available())
      {
        int lastPos = actualFile.position();
        newFile.write(actualFile.read());
        if (lastPos == actualFile.position())
        {
          gestionnaire_erreur(ERR_SD_PLEIN);
          break;
        }
      }
      newFile.flush();
      newFile.close();
      actualFile.seek(0);
      Serial.println(F("Changed file"));
    }
  }
  return &actualFile;
}

void enregistrement()
{
  String mess = "";
  File *actualFile = changement_fichier(mess.length());
  for (int i = 0; i < sizeof(capteurs) / sizeof(capteurs[0]); i++)
  {
    mess += "Capteur " + String(i) + " : " + capteurs[i]->dernieres_valeurs[capteurs[i]->tableau_valeurs_index];
  }
  int pos = actualFile->position();
  actualFile->println(mess);
  if (pos == actualFile->position())
    gestionnaire_erreur(ERR_SD_PLEIN);
  else
    actualFile->flush();
}

void setup()
{
  Serial.begin(115200);

  cli();                   // désactive les interruptions.
  pinMode(button1, INPUT); // Etablissement des pins
  pinMode(button2, INPUT); //
  pinMode(9, OUTPUT);      // Vert
  pinMode(10, OUTPUT);     // Rouge
  pinMode(11, OUTPUT);     // Bleu

  mode = MODE_STANDARD;  // Si on ne fait rien au démarrage, la station est en mode standard.
  digitalWrite(9, HIGH); // Allulage LED verte.

  while (digitalRead(3) == HIGH) // Si on appuie sur le bouton 2, le mode configuration se lance.
  {
    mode = MODE_CONFIGURATION;
    digitalWrite(11, HIGH);
    digitalWrite(10, HIGH);
    digitalWrite(9, HIGH);
  }

  TCCR1A = 0;          // Reset TCCR1A à 0 (timer + comparateur)
  TCCR1B = 0;          // Reset TCCR1A à 0 (timer + comparateur)
  TCCR1B |= B00000100; // Met CS12 à 1 pour un prescaler à 256 (limite)
  TIMSK1 |= B00000010; // Met OCIE1A à 1 pour comparer le comparer au match A

  OCR1A = 31250;

  sei(); // Reactivation des interruptions.

  attachInterrupt(digitalPinToInterrupt(2), timer, CHANGE); // interruptions materielles pour les 2 boutons.
  attachInterrupt(digitalPinToInterrupt(3), timer, CHANGE);

  Serial.println("Mode de lancement : " + String(mode));

  for (int i = 0; i < 9; i++) // allocation mémoire des capteurs de la structure.
  {
    capteurs[i] = (Capteur *)calloc(1, sizeof(Capteur));
    capteurs[i]->type = i;
  }

  for (int i = 0; i < 9; i++)
  {
    Serial.println("Listes des capteurs : " + String(capteurs[i]->type));
  }

  for (int i = 0; i < 3; i++) // Mise a défauts des caractéristiques de tous les capteurs.
  {
    capteurs[i]->timeout = DEFAULT_TIMEOUT_CAPTEUR; // reset du timeout des capteurs
    capteurs[i]->actif = DEFAULT_ACTIVATION;        // reset de l'etat des capteurs
  }
  capteurs[0]->min = DEFAULT_LUMIN_LOW; // reset des valeurs de la luminosite
  capteurs[0]->max = DEFAULT_LUMIN_HIGH;

  capteurs[1]->min = DEFAULT_MIN_TEMP; // reset des valeurs de la temperature
  capteurs[1]->max = DEFAULT_MAX_TEMP;

  capteurs[2]->min = DEFAULT_HYGR_MINT; // reset des valeurs de l'hygrometrie
  capteurs[2]->max = DEFAULT_HYGR_MAXT;

  capteurs[3]->min = DEFAULT_PRESSURE_MIN; // reset des valeurs de la pression
  capteurs[3]->max = DEFAULT_PRESSURE_MAX;

  // GPS
  GPS.begin(9600);

  // DHT
  dht.begin();

  // BMP
  if (!bmp.begin(0x76))
    gestionnaire_erreur(ERR_CAPTEUR_ACCES);
  //---

  // RTC
  Wire.begin();
  if (!RTC.begin())
    gestionnaire_erreur(ERR_CAPTEUR_ACCES);

  // RTC
  Wire.begin();
  RTC.begin();

  Serial.print("\n");
  DateTime now = RTC.now();

  affiche_date_heure(now);

} // fin setup

void timer()
{                      // fonction d'interruption logiciel appelé par l'attachInterrupt.
  cli();               // Stop interrupts
  TCNT1 = 0;           // compteur du timer a 0.
  TCCR1B |= B00000101; // configure le registre TCCR1B pour activer le Timer 1 en mode CTC avec un préscaleur de 1024.
  TIMSK1 |= B00000010; // Cela active l'interruption de correspondance avec le registre OCR1A pour le Timer 1.
  OCR1A = 31250;       // configure la valeur de comparaison du Timer 1 à 39063. (2.5 secondes.)
  sei();               // Reactivation des interruptions.
}

ISR(TIMER1_COMPA_vect) // (Interruption Service Routine)
{
  if (mode != previous_mode)
  {
    switch (mode) // Différents cas.
    {
    case MODE_STANDARD:           // mode = 0
      if (digitalRead(2) == HIGH) // si on est en mode standard, et qu'on appui sur le bouton 2.
      {
        gestionnaire_modes(MODE_MAINTENANCE); // appel le gestionnaire de mode avec le nouveau mode.
        break;                                // stop le switch case
      }
      if (digitalRead(3) == HIGH) // si on est en mode standard, et qu'on appui sur le bouton 3.
      {
        gestionnaire_modes(MODE_ECO);
        break;
      }

    case MODE_ECO:                // mode = 3
      if (digitalRead(3) == HIGH) // si on est en mode eco, et qu'on appui sur le bouton 3.
      {
        gestionnaire_modes(MODE_STANDARD);
        break;
      }
      if (digitalRead(2) == HIGH) // si on est en mode eco, et qu'on appui sur le bouton 2.
      {
        gestionnaire_modes(MODE_MAINTENANCE);
        break;
      }

    case MODE_MAINTENANCE: // mode = 2
      if (digitalRead(2) == HIGH)
      {
        gestionnaire_modes(previous_mode); // appel le gestionnaire de mode avec l'ancien mode.
        break;
      }
    }
  }
  // gestionnaire erreur
  if (code_couleur == 2)
  { // Erreur Horloge RTC
    if (etatled == 0)
    {
      if (compteur_sec == 1)
      {
        compteur_sec = 0;
        analogWrite(R, 255);
        analogWrite(B, 0);
        etatled = 1;
      }
      else
      {
        compteur_sec++;
      }
    }
    else
    {
      if (compteur_sec == 1)
      {
        compteur_sec = 0;
        analogWrite(R, 0);
        analogWrite(B, 255);
        etatled = 0;
      }
      else
      {
        compteur_sec++;
      }
    }
    Serial.println("Erreur accès horloge RTC");
  }

  if (code_couleur == 5)
  { // Erreur GPS
    if (etatled == 0)
    {
      if (compteur_sec == 1)
      {
        compteur_sec = 0;
        analogWrite(R, 255);
        analogWrite(V, 0);
        etatled = 1;
      }
      else
      {
        compteur_sec++;
      }
    }
    else
    {
      if (compteur_sec == 1)
      {
        compteur_sec = 0;
        analogWrite(R, 255);
        analogWrite(V, 255);
        etatled = 0;
      }
      else
      {
        compteur_sec++;
      }
    }
    Serial.println("Erreur accès GPS");
  }

  if (code_couleur == 3)
  { // Erreur accès capteurs
    if (etatled == 0)
    {
      if (compteur_sec == 1)
      {
        compteur_sec = 0;
        analogWrite(R, 255);
        analogWrite(V, 0);
        etatled = 1;
      }
      else
      {
        compteur_sec++;
      }
    }
    else
    {
      if (compteur_sec == 1)
      {
        compteur_sec = 0;
        analogWrite(R, 0);
        analogWrite(V, 255);
        etatled = 0;
      }
      else
      {
        compteur_sec++;
      }
    }
    Serial.println("Erreur accès capteur");
  }

  if (code_couleur == 4)
  { // Erreur incohérence
    if (etatled == 0)
    {
      if (compteur_sec == 1)
      {
        compteur_sec = 0;
        analogWrite(R, 0);
        analogWrite(V, 255);
        etatled = 1;
      }
      else
      {
        compteur_sec++;
      }
    }
    else
    {
      if (compteur_sec == 3)
      {
        compteur_sec = 0;
        analogWrite(R, 255);
        analogWrite(V, 0);
        etatled = 0;
      }
      else
      {
        compteur_sec++;
      }
    }
    Serial.println("Erreur données incohérentes // Vérification matérielle requise");
  }

  if (code_couleur == 0)
  { // Erreur carte SD pleine
    if (etatled == 0)
    {
      if (compteur_sec == 1)
      {
        compteur_sec = 0;
        analogWrite(R, 255);
        analogWrite(V, 0);
        analogWrite(B, 0);
        etatled = 1;
      }
      else
      {
        compteur_sec++;
      }
    }
    else
    {
      if (compteur_sec == 1)
      {
        compteur_sec = 0;
        analogWrite(R, 255);
        analogWrite(V, 255);
        analogWrite(B, 255);
        etatled = 0;
      }
      else
      {
        compteur_sec++;
      }
    }
    Serial.println("Erreur : Carte SD pleine");
  }

  if (code_couleur == 1)
  { // Erreur accès/écriture carte SD
    if (etatled == 0)
    {
      if (compteur_sec == 1)
      {
        compteur_sec = 0;
        analogWrite(R, 255);
        analogWrite(V, 255);
        analogWrite(V, 255);
        etatled = 1;
      }
      else
      {
        compteur_sec++;
      }
    }
    else
    {
      if (compteur_sec == 3)
      {
        compteur_sec = 0;
        analogWrite(R, 255);
        analogWrite(V, 0);
        analogWrite(V, 0);
        etatled = 0;
      }
      else
      {
        compteur_sec++;
      }
    }
    Serial.println("Erreur accès/écriture sur carte SD");
  }
}

void loop() // test de loop rapide (a ne pas prendre en compte)
{
  if (mode == MODE_STANDARD)
  {

    long watchdog = millis() + 10000;
    byte buffer = 0;
    char values[5];
    char latitude[16];
    char longitude[16];
    // on attend essaie pendant 10 secondes de récupérer la variable de la position
    while (watchdog > millis())
    {
      // attente du début de variable $
      while (GPS.read() != 36)
      {
        yield();
      } // (char)36 <=> $
      for (int i = 0; i < 5; i++)
      {
        while (!GPS.available() && watchdog > millis())
        {
          yield();
        } // wait for char to get
        values[i] = GPS.read();
      }
      if ((String)values == "GPRMC")
      {
        break;
      }
    }
    free(values);
    // préparation du tableau à stocker
    float *coordonnee = (float *)calloc(2, sizeof(float));

    // Une fois la variable trouvée, on vérfie on décale sur la latitude
    byte commaCounter = 0;
    while (!GPS.available() && watchdog > millis())
    {
      yield();
    } // wait for char to get
    char caractere = GPS.read();
    while (commaCounter < 3 && watchdog > millis())
    {
      if (caractere == ',')
      {
        commaCounter++;
      }
      while (!GPS.available() && watchdog > millis())
      {
        yield();
      } // wait for char to get
      caractere = GPS.read();
    }
    // On vérfie que la latitude existe
    latitude[buffer] = caractere;
    if (latitude[0] != ',')
    {
      while (latitude[buffer++] != ',' && watchdog > millis())
      {
        while (!GPS.available() && watchdog > millis())
        {
          yield();
        } // wait for char to get
        latitude[buffer] = GPS.read();
      }
      latitude[buffer - 1] = '\0';
      coordonnee[0] = atof(latitude);
      // free(latitude);

      // je retire l'unité
      for (int i = 0; i < 2; i++)
      {
        while (!GPS.available() && watchdog > millis())
        {
          yield();
        } // wait for char to get
        GPS.read();
      }

      // On récupère la valeur de la longitude
      buffer = 0;
      while (!GPS.available() && watchdog > millis())
      {
        yield();
      } // wait for char to get
      longitude[buffer] = GPS.read();
      while (longitude[buffer++] != ',' && watchdog > millis())
      {
        while (!GPS.available() && watchdog > millis())
        {
          yield();
        } // wait for char to get
        longitude[buffer] = GPS.read();
      }
      longitude[buffer - 1] = '\0';
      coordonnee[1] = atof(longitude);
      free(longitude);
    }
    Serial.print("\nLatitude: ");
    Serial.println(coordonnee[0]);
    Serial.print("Longitude: ");
    Serial.println(coordonnee[1]);

    // DHT
    Serial.print("température:");
    Serial.println(dht.readTemperature());
    Serial.print("humidité:");
    Serial.println(dht.readHumidity());

    // Lumière
    lum_val = analogRead(lum_PIN);
    Serial.print("Valeur luminosité :");
    Serial.println(lum_val);

    // Pression
    Serial.print("Pression:");
    Serial.println(bmp.readPressure() / 100);

    // Temperature eau
    Serial.print("Temperature eau : ");
    float t_eau = rand() % 30;
    Serial.print(t_eau);
    Serial.println(" °C");

    // courant marin
    Serial.print("Courant marin : ");
    float c_marin = rand() % 5;
    Serial.print(c_marin);
    Serial.println(" noeud");

    // Force du vent
    Serial.print("Force du vent : ");
    float f_vent = rand() % 30;
    Serial.print(f_vent);
    Serial.println(" metre/seconde");

    // Taux de particule fine
    Serial.print("Taux de particule fine : ");
    float partic = rand() % 200;
    Serial.print(partic);
    Serial.println(" microgramme/metre^3");

    if (dht.readTemperature() > 40 || dht.readTemperature() < -10 || lum_val > 25000 || lum_val < 0 || bmp.readPressure() / 100 < 900 ||
        bmp.readPressure() / 100 > 1050 || t_eau > 29 || c_marin > 4 || f_vent > 28 || partic > 170)
      gestionnaire_erreur(ERR_CAPTEUR_INCOHERENTE);
    // enregistrement();
    delay(2000);
  }

  if (mode == MODE_CONFIGURATION && millis() - inactivite >= 30000)
    gestionnaire_modes(0);
  else
  {
    get_commande();
  }
  delay(1e3);
}

void gestionnaire_modes(int nvmode) // gestionnaire de mode avec le nouveau mode comme parametre.
{
  previous_mode = mode; // le mode actuel devient l'ancien mode.

  switch (nvmode)
  {
  case 0: // standard
    digitalWrite(9, HIGH);
    digitalWrite(10, LOW);
    digitalWrite(11, LOW); // en vert

    for (int i = 0; i < sizeof(capteurs) / 2; i++)
    {
      capteurs[i]->actif = 1; // active tout les capteurs en standard.
    }
    Serial.print(F("Initializing SD card..."));
    SD = new SdFat32();
    rtc = new RTC_DS1307();
    if (!SD->begin(4))
    {
      Serial.println(F("initialization failed!"));
      gestionnaire_erreur(ERR_SD_IO);
    }
    else
    {
      Serial.println(F("initialization done."));
    }
    if (!rtc->begin())
    {
      Serial.println(F("Horloge introuvable"));
    }
    else
    {
      DateTime *now = &rtc->now();
      year = now->year();
      month = now->month();
      day = now->day();
    }
    break;

  case 1: // config
    inactivite = millis();
    analogWrite(10, 255);
    analogWrite(9, 120);
    digitalWrite(11, LOW); // en jaune
    for (int i = 0; i < sizeof(capteurs) / 2; i++)
    {
      capteurs[i]->actif = 0; // desactive tout les capteurs en config.
    }
    break;

  case 2: // maintenance
    inactivite = millis();
    analogWrite(10, 255);
    analogWrite(11, 165);
    digitalWrite(9, LOW); // en violet (au lieu de orange)
    for (int i = 0; i < sizeof(capteurs) / 2; i++)
    {
      capteurs[i]->actif = 1; // active tout les capteurs en maintenance.
    }
    break;

  case 3: // eco

    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
    digitalWrite(11, HIGH); // en bleu
    for (int i = 0; i < sizeof(capteurs) / 2; i++)
    {
      if (capteurs[i]->type == 5 || capteurs[i]->type == 6 || capteurs[i]->type == 7 || capteurs[i]->type == 8)
      {
        capteurs[i]->actif = 0; // desactive les capteurs non prioritaires dans le mode eco.
      }
      else
        capteurs[i]->actif = 1;
    }
    break;
  }
  mode = nvmode;                                              // on actualise le nouveau mode.
  Serial.println("previous mode : " + String(previous_mode)); // affichage
  Serial.println("Changement de mode : " + String(mode));
  for (int i = 0; i < sizeof(capteurs) / 2; i++)
  {
    Serial.println("Liste des etats des capteurs : " + String(capteurs[i]->actif));
  }
}

void get_commande() // fonction pour les commandes en mode config.
{
  int i;   // index pour les futurs capteurs que nous allons modifier.
  int MIN; // valeur pour modifier les min et max des capteurs.
  int MAX; // valeur pour modifier les min et max des capteurs.

  if (Serial.available() != 0) // si on ecrit dans le port serie.
  {
    inactivite = millis();
    String command = Serial.readStringUntil('=');     // command = la commande ecrite avant le "=".
    int value = Serial.readStringUntil('\n').toInt(); // value = la valeur ecrite apres le "=".

    Serial.println(command);

    if (command.startsWith("LUMIN")) // si la commande comment par LUMIN
    {
      i = 0; // actualisation en fonction des des capteurs.
      MIN = 0;
      MAX = 1023;
    }

    else if (command.startsWith("TEMP_AIR")) // si la commande comment par TEMP_AIR
    {
      i = 1; // actualisation en fonction des des capteurs.
      MIN = -40;
      MAX = 85;
    }

    else if (command.startsWith("HYGR")) // si la commande comment par HYGR
    {
      i = 2; // actualisation en fonction des des capteurs.
      MIN = -40;
      MAX = 85;
    }

    else if (command.startsWith("PRESSURE")) // si la commande comment par PRESSURE
    {
      i = 3; // actualisation en fonction des des capteurs.
      MIN = 300;
      MAX = 1100;
    }

    if (command == "LUMIN" || command == "TEMP_AIR" || command == "HYGR" || command == "PRESSURE")
    {
      if (value == 0 || value == 1) // si la valeur est bonne.
      {
        capteurs[i]->actif = value; // change l etat actif des capteurs
        Serial.println("succès de l'operation !!");
        /*
                for (int i; i < 9; i++)
                {
                  Serial.println("Etat des capteurs apres config : " + String(capteurs[i]->actif));
                }*/
      }
      else // cas d'erreurs
      {
        Serial.println("veuillez entrer une valeur entre 0 et 1.\n");
      }
      command = "None";
    }

    if (command.indexOf("MIN") > 0) // si la commande contient "MIN".
    {
      if (value > MIN && value < MAX) // Si les valeurs sont bonnes dans la commande.
      {
        capteurs[i]->min = value; // actualisation des minimum des capteurs.
        Serial.println("succès de l'operation !!");
        /*
                for (int i; i < 9; i++)
                {
                  Serial.println("Etat des minimum apres config : " + String(capteurs[i]->min));
                }*/
      }
      else // cas d'erreur
      {
        Serial.println("veuillez entrer une valeur entre " + String(MIN) + " et " + String(MAX) + ".\n");
      }
      command = "None";
    }

    if (command.indexOf("MAX") > 0) // si la commande contient "MAX".
    {
      if (value > MIN && value < MAX) // Verifie les valeurs.
      {
        capteurs[i]->max = value; // acutalise les valeurs max des capteurs.
        Serial.println("succès de l'operation !!");

        for (int i; i < 9; i++) // affichage
        {
          Serial.println("Etat des maximum apres config : " + String(capteurs[i]->max));
        }
      }

      else // cas de commande fausse.
      {
        Serial.println("veuillez entrer une valeur entre " + String(MIN) + " et " + String(MAX) + ".\n");
      }
      command = "None";
    }

    if (command == "LOG_INTERVAL") // si la commande est de modifier le LOG_INTERVAL
    {
      if (value > 0)
      {
        log_interval = value; // actualisation de LOG_INTERVAL

        Serial.println("Nouvelle valeur de log_interval : " + String(log_interval));
      }
      else
      {
        Serial.println("Valeur trop basse pour LOG_INTERVAL !\n");
      }
    }

    if (command == "FILE_SIZE") // si la commande est de modifier le FILE_SIZE
    {
      if (value < 16384 && value > 512) // si la valeur est bonne.
      {
        file_max_size = value; // actualisation de la taille

        Serial.println("Nouvelle valeur de file_max_size : " + String(file_max_size));
      }
      else
      {
        Serial.println("Valeur incohérente pour FILE_MAX_SIZE !\n");
      }
    }

    if (command == "RESET") // si la commande est RESET.
    {
      file_max_size = DEFAULT_FILE_MAX_SIZE; // reset TOUTE les valeurs pouvant etre modifiées.
      log_interval = DEFAULT_LOG_INTERVAL;
      for (int i = 0; i < 3; i++)
      {
        capteurs[i]->timeout = DEFAULT_TIMEOUT_CAPTEUR; // reset du timeout des capteurs
        capteurs[i]->actif = DEFAULT_ACTIVATION;        // reset de l'etat des capteurs
      }
      capteurs[0]->min = DEFAULT_LUMIN_LOW; // reset des valeurs de la luminosite
      capteurs[0]->max = DEFAULT_LUMIN_HIGH;

      capteurs[1]->min = DEFAULT_MIN_TEMP; // reset des valeurs de la temperature
      capteurs[1]->max = DEFAULT_MAX_TEMP;

      capteurs[2]->min = DEFAULT_HYGR_MINT; // reset des valeurs de l'hygrometrie
      capteurs[2]->max = DEFAULT_HYGR_MAXT;

      capteurs[3]->min = DEFAULT_PRESSURE_MIN; // reset des valeurs de la pression
      capteurs[3]->max = DEFAULT_PRESSURE_MAX;
    }

    if (command == "VERSION") // commande d affichage de version.
    {
      Serial.println("Version : " + String(version));
    }

    if (command == "TIMEOUT") // si la commande est de modifier le TIMEOUT.
    {
      if (value > 0) // si la valeur est bonne.
      {
        for (int i = 0; i < 9; i++)
        {
          capteurs[i]->timeout = value; // actualisation des timeout des capteurs.
        }
        Serial.println("timeout : " + String(value));
      }
      else
      {
        Serial.println("NTM CA MARCHE PAS TA VALEUR !!!");
      }
    }

    for (int i = 0; i < 9; i++) // affichage
    {
      Serial.println("Nouvelles valeurs actives des capteurs : " + String(capteurs[i]->actif));
      Serial.println("Nouvelles valeurs des seuil MIN : " + String(capteurs[i]->min));
      Serial.println("Nouvelles valeurs des seuil MAX : " + String(capteurs[i]->max));
    }
  }
}

void gestionnaire_erreur(int erreur)
{
  if (erreur == ERR_SD_PLEIN)
  { // Erreur 0 pour carte SD pleine
    code_couleur = erreur;
  }

  if (erreur == ERR_SD_IO)
  { // Erreur 1 pour écriture/accès carte SD
    code_couleur = erreur;
  }

  if (erreur == ERR_RTC)
  { // Erreur 2 pour horloge RTC
    code_couleur = erreur;
  }

  if (erreur == ERR_CAPTEUR_ACCES)
  { // Erreur 3 pour accès capteur
    code_couleur = erreur;
  }

  if (erreur == ERR_CAPTEUR_INCOHERENTE)
  { // Erreur 4 pour incohérence
    code_couleur = erreur;
  }

  if (erreur == ERR_GPS)
  { // Erreur 5 pour GPS
    code_couleur = erreur;
  }

  // ISR(TIMER1_COMPA_vect);
}
