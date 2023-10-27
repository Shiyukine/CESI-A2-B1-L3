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
#define CAPTEUR_TYPE_GPS_LAT 0x4
#define CAPTEUR_TYPE_PARTICLE 0x5
#define CAPTEUR_TYPE_TEMP_EAU 0x6
#define CAPTEUR_TYPE_VENT 0x7
#define CAPTEUR_TYPE_COURANTS 0x8
#define CAPTEUR_TYPE_GPS_LON 0x9

#define button1 2
#define button2 3

#define V 9
#define R 10
#define B 11

#define lum_PIN A1

#include <RTClib.h>
#include <SdFat.h>
#include <Arduino.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>

#define DHT_PIN 5
#define DHTTYPE DHT11

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
int prescaler;

int file_max_size = 2048;
int compteur_taille_fichier = 0;
int compteur_revision = 0;
int mode = -1;
Capteur *capteurs[10];
int year;
int month;
int day;
int previous_mode = -2;
bool mode_change = true;
int inactivite = 0;

int etatled = 0;
int code_couleur = -1;
int compteur_sec = 0;

bool sdmounted = false;

SdFat32 *SD;
RTC_DS1307 *rtc;

// initialisation du périph, pin 7 réception, pin 8 envoie
SoftwareSerial *GPS;

// Instance DHT(Température et humidité)
DHT *dht;

// Instance du BMP(Pression)
Adafruit_BMP280 *bmp;

File *changement_fichier(int mess_size)
{
  DateTime *now = &rtc->now();
  String aa = String(now->year()).substring(1, 3);
  String mm = String(now->month());
  if (now->month() < 10)
    mm = "0" + mm;
  String jj = String(now->day());
  if (now->day() < 10)
    jj = "0" + jj;
  String file_name = aa + mm + jj + "_" + (String)compteur_revision + ".LOG";
  static File actualFile = SD->open(file_name, O_RDWR | O_CREAT | O_TRUNC);
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
    Serial.println("Copy to " + file_name);
    File newFile = SD->open(file_name, O_RDWR | O_CREAT | O_TRUNC);
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
    mess += "Capteur " + (String)i + " : " + (String)capteurs[i]->dernieres_valeurs[capteurs[i]->tableau_valeurs_index - 1];
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

  pinMode(button1, INPUT); // Etablissement des pins
  pinMode(button2, INPUT); //
  pinMode(V, OUTPUT);      // Vert
  pinMode(R, OUTPUT);      // Rouge
  pinMode(B, OUTPUT);      // Bleu

  int init_mode = MODE_STANDARD; // Si on ne fait rien au démarrage, la station est en mode standard.

  while (digitalRead(3) == HIGH) // Si on appuie sur le bouton 2, le mode configuration se lance.
  {
    init_mode = MODE_CONFIGURATION;
    digitalWrite(B, HIGH);
    digitalWrite(R, HIGH);
    digitalWrite(V, HIGH);
  }

  gestionnaire_modes(init_mode);

  cli(); // désactive les interruptions.

  TCCR1A = 0;          // Reset TCCR1A à 0 (timer + comparateur)
  TCCR1B = 0;          // Reset TCCR1A à 0 (timer + comparateur)
  TCCR1B |= B00000100; // Met CS12 à 1 pour un prescaler à 256 (limite)
  TIMSK1 |= B00000010; // Met OCIE1A à 1 pour comparer le comparer au match A

  OCR1A = 31250;

  sei(); // Reactivation des interruptions.

  attachInterrupt(digitalPinToInterrupt(2), timer, CHANGE); // interruptions materielles pour les 2 boutons.
  attachInterrupt(digitalPinToInterrupt(3), timer, CHANGE);

  for (int i = 0; i < 10; i++) // allocation mémoire des capteurs de la structure.
  {
    capteurs[i] = (Capteur *)calloc(1, sizeof(Capteur));
    capteurs[i]->type = i;
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
  GPS = new SoftwareSerial(7, 8);
  GPS->begin(9600);

  // DHT
  dht = new DHT(DHT_PIN, DHTTYPE);
  dht->begin();

  // BMP
  bmp = new Adafruit_BMP280();
  if (!bmp->begin(0x76))
    gestionnaire_erreur(ERR_CAPTEUR_ACCES);
  //---
  rtc = new RTC_DS1307();
  rtc->begin();

  DateTime *now = &rtc->now();
  year = now->year();
  month = now->month();
  day = now->day();

} // fin setup

void timer()
{                      // fonction d'interruption logiciel appelé par l'attachInterrupt.
  cli();               // Stop interrupts
  TCNT1 = 0;           // compteur du timer a 0.
  TCCR1B |= B00000101; // configure le registre TCCR1B pour activer le Timer 1 en mode CTC avec un préscaleur de 1024.
  TIMSK1 |= B00000010; // Cela active l'interruption de correspondance avec le registre OCR1A pour le Timer 1.
  OCR1A = 39063;       // configure la valeur de comparaison du Timer 1 à 39063. (2.5 secondes.)
  sei();               // Reactivation des interruptions.
  mode_change = true;
}

ISR(TIMER1_COMPA_vect) // (Interruption Service Routine)
{
  if (mode_change)
  {
    mode_change = false;
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
    mode_change = false;
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
    Serial.println(F("Erreur accès horloge RTC"));
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
    Serial.println(F("Erreur accès GPS"));
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
    Serial.println(F("Erreur accès capteur"));
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
    Serial.println(F("Erreur données incohérentes // Vérification matérielle requise"));
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
    Serial.println(F("Erreur : Carte SD pleine"));
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
    Serial.println(F("Erreur accès/écriture sur carte SD"));
  }
}

void loop() // test de loop rapide (a ne pas prendre en compte)
{
  if (mode == MODE_STANDARD)
  {
    /*while (GPS->available())
    {
      GPS->read();
      String str = GPS->readStringUntil('\n');
      if (str.startsWith("$GPGGA"))
      {
        char lati[] = "";
        char longi[] = "";
        int delim_index = 0;
        for (int i = 0; i < str.length(); i++)
        {
          if (str[i] == ',')
            delim_index++;
          else
          {
            if (delim_index == 2)
              lati[i] = str[i];
            if (delim_index == 4)
              longi[i] = str[i];
          }
        }
        capteurs[CAPTEUR_TYPE_GPS_LAT]->dernieres_valeurs[capteurs[CAPTEUR_TYPE_GPS_LAT]->tableau_valeurs_index] = String(lati).toInt();
        capteurs[CAPTEUR_TYPE_GPS_LAT]->tableau_valeurs_index++;
        capteurs[CAPTEUR_TYPE_GPS_LON]->dernieres_valeurs[capteurs[CAPTEUR_TYPE_GPS_LON]->tableau_valeurs_index] = String(longi).toInt();
        capteurs[CAPTEUR_TYPE_GPS_LON]->tableau_valeurs_index++;
      }
    }
    for (int i = 0; i < 10; i++)
    {
      int val = -999;
      switch (i)
      {
      case CAPTEUR_TYPE_LUMIN:
        val = analogRead(lum_PIN);
        break;

      case CAPTEUR_TYPE_TEMP:
        // val = dht->readTemperature();
        break;

      case CAPTEUR_TYPE_HYGR:
        // val = dht->readHumidity();
        break;

      case CAPTEUR_TYPE_PRESSURE:
        // val = bmp->readPressure() / 100;
        break;

      case CAPTEUR_TYPE_PARTICLE:
        val = rand() % 200;
        break;

      case CAPTEUR_TYPE_TEMP_EAU:
        val = rand() % 30;
        break;

      case CAPTEUR_TYPE_VENT:
        val = rand() % 30;
        break;

      case CAPTEUR_TYPE_COURANTS:
        val = rand() % 5;
        break;

      default:
        break;
      }
      if (val != -999)
      {
        capteurs[i]->dernieres_valeurs[capteurs[i]->tableau_valeurs_index] = val;
        capteurs[i]->tableau_valeurs_index++;
      }
    }*/

    /*if (dht->readTemperature() > 40 || dht->readTemperature() < -10 || lum_val > 25000 || lum_val < 0 || bmp->readPressure() / 100 < 900 ||
        bmp->readPressure() / 100 > 1050 || t_eau > 29 || c_marin > 4 || f_vent > 28 || partic > 170)
      gestionnaire_erreur(ERR_CAPTEUR_INCOHERENTE);*/
    // enregistrement();
    //   delay(2000);
  }
  else if (mode == MODE_CONFIGURATION)
  {
    if (millis() - inactivite >= 30000)
      gestionnaire_modes(MODE_STANDARD);
    else
    {
      get_commande();
    }
  }
  // delay(1e3);
}

void gestionnaire_modes(int nvmode) // gestionnaire de mode avec le nouveau mode comme parametre.
{
  mode_change = true;
  previous_mode = mode; // le mode actuel devient l'ancien mode.

  switch (nvmode)
  {
  case 0: // standard
    digitalWrite(V, HIGH);
    digitalWrite(R, LOW);
    digitalWrite(B, LOW); // en vert

    for (int i = 0; i < sizeof(capteurs) / 2; i++)
    {
      capteurs[i]->actif = 1; // active tout les capteurs en standard.
    }
    /*if (!sdmounted)
    {
      SD = new SdFat32();
      if (!SD->begin(4))
      {
        Serial.println(F("initialization failed!"));
        gestionnaire_erreur(ERR_SD_IO);
      }
      else
      {
        Serial.println(F("SD card OK."));
      }
      sdmounted = true;
    }*/
    break;

  case 1: // config
    inactivite = millis();
    analogWrite(R, 255);
    analogWrite(V, 120);
    digitalWrite(B, LOW); // en jaune
    for (int i = 0; i < sizeof(capteurs) / 2; i++)
    {
      capteurs[i]->actif = 0; // desactive tout les capteurs en config.
    }
    break;

  case 2: // maintenance
    inactivite = millis();
    analogWrite(R, 255);
    analogWrite(B, 165);
    digitalWrite(V, LOW); // en violet (au lieu de orange)
    for (int i = 0; i < sizeof(capteurs) / 2; i++)
    {
      capteurs[i]->actif = 1; // active tout les capteurs en maintenance.
    }
    break;

  case 3: // eco

    digitalWrite(R, LOW);
    digitalWrite(V, LOW);
    digitalWrite(B, HIGH); // en bleu
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
  mode = nvmode;
  Serial.println(F("Changement de mode : "));
  Serial.println(previous_mode);
  Serial.println(mode);
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
      }
      else // cas d'erreurs
      {
        Serial.println(F("veuillez entrer une valeur entre 0 et 1.\n"));
      }
      command = "None";
    }

    if (command.indexOf("MIN") > 0) // si la commande contient "MIN".
    {
      if (value > MIN && value < MAX) // Si les valeurs sont bonnes dans la commande.
      {
        capteurs[i]->min = value; // actualisation des minimum des capteurs.
      }
      else // cas d'erreur
      {
        Serial.print(F("veuillez entrer une valeur entre "));
        Serial.print(MIN);
        Serial.print(F(" et "));
        Serial.print(MAX);
        Serial.println(".");
      }
      command = "None";
    }

    if (command.indexOf("MAX") > 0) // si la commande contient "MAX".
    {
      if (value > MIN && value < MAX) // Verifie les valeurs.
      {
        capteurs[i]->max = value; // acutalise les valeurs max des capteurs.

        for (int i; i < 9; i++) // affichage
        {
          Serial.print(F("Etat des maximum apres config : "));
          Serial.println(capteurs[i]->max);
        }
      }

      else // cas de commande fausse.
      {
        Serial.print(F("veuillez entrer une valeur entre "));
        Serial.print(MIN);
        Serial.print(" et ");
        Serial.print(MAX);
        Serial.println(".");
      }
      command = "None";
    }

    if (command == "LOG_INTERVAL") // si la commande est de modifier le LOG_INTERVAL
    {
      if (value > 0)
      {
        log_interval = value; // actualisation de LOG_INTERVAL

        Serial.print(F("Nouvelle valeur de log_interval : "));
        Serial.println(log_interval);
      }
      else
      {
        Serial.println(F("Valeur trop basse pour LOG_INTERVAL !\n"));
      }
    }

    if (command == "FILE_SIZE") // si la commande est de modifier le FILE_SIZE
    {
      if (value < 16384 && value > 512) // si la valeur est bonne.
      {
        file_max_size = value; // actualisation de la taille

        Serial.print(F("Nouvelle valeur de file_max_size : "));
        Serial.println(file_max_size);
      }
      else
      {
        Serial.println(F("Valeur incohérente pour FILE_MAX_SIZE !\n"));
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
      Serial.print(F("Version : "));
      Serial.println(version);
    }

    if (command == "TIMEOUT") // si la commande est de modifier le TIMEOUT.
    {
      if (value > 0) // si la valeur est bonne.
      {
        for (int i = 0; i < 9; i++)
        {
          capteurs[i]->timeout = value; // actualisation des timeout des capteurs.
        }
        Serial.print(F("timeout : "));
        Serial.println(value);
      }
      else
      {
        Serial.println(F("RIP !!!"));
      }
    }

    for (int i = 0; i < 9; i++) // affichage
    {
      Serial.print(F("Nouvelles valeurs actives des capteurs : "));
      Serial.println(capteurs[i]->actif);
      Serial.print(F("Nouvelles valeurs des seuil MIN : "));
      Serial.println(capteurs[i]->min);
      Serial.print(F("Nouvelles valeurs des seuil MAX : "));
      Serial.println(capteurs[i]->max);
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
