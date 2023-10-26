#include <RTClib.h>
#include <SdFat.h>

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

typedef struct
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

int file_max_size = 2048;
int compteur_taille_fichier = 0;
int compteur_revision = 0;
int version;
int log_interval;
int mode = 0;
Capteur *capteurs[9];
int year;
int month;
int day;
SdFat32 *SD;
RTC_DS1307 *rtc;
int previous_mode = -1;
int inactivite = 0;

// 0; //'luminosite';
// 1; //'temperature';
// 2; //'hygrometrie';
// 3; //'pression';
// 4; //'GPS';
// 5; //'particule';
// 6; //'temp_eau';
// 7; //'vent';
// 8; //'courant';

void get_commande()
{
  int i;
  int MIN;
  int MAX;

  if (Serial.available() != 0)
  {
    inactivite = millis();
  }
}

void change_mode(int nvmode)
{
  previous_mode = mode;
  mode = nvmode;
  Serial.println(nvmode);
  if (mode == 1)
  {
    inactivite = millis();
  }
}

void setup()
{
  Serial.begin(115200);
  for (int i = 0; i < 9; i++)
  {
    capteurs[i] = (Capteur *)calloc(1, sizeof(Capteur));
    capteurs[i]->type = i;
  }
  // Serial.println(String("blabla").indexOf("c") > 0);
  change_mode(1);
}

void loop()
{
  if (mode == 1 && millis() - inactivite >= 30000)
    change_mode(0);
  else
  {
    get_commande();
  }
}