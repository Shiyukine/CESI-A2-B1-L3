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

// to works with VSCode
#ifndef TCCR1B
extern int TCCR1A;
extern int TCCR1B;
extern int TIMSK1;
extern int OCR1A;
extern int TCNT1;
#endif

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
int previous_mode = -1;
int inactivite = 0;

int etatled = 0;
int code_couleur = 2;
int compteur_sec = 0;

SdFat32 *SD;
RTC_DS1307 *rtc;

void erreur(int erreur)
{
  Serial.println("Error " + String(erreur));
}

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
    erreur(1);
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
      erreur(5);
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
          erreur(2);
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
  char mess[] = "Capteur 1 = 25, Capteur 2 = 25, Capteur 3 = 25, Capteur 4 = 25, Capteur 5 = 25, Capteur 6 = 25, Capteur 7 = 25, Capteur 8 = 25, Capteur 9 = 25 aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
  File *actualFile = changement_fichier(sizeof(mess) / sizeof(mess[0]));
  /*for (int i = 0; i < sizeof(capteurs) / sizeof(capteurs[0]); i++)
  {
      if (actualFile->availableForWrite())
          actualFile->print("25 ");
      else
          erreur(20);
      actualFile->flush();
  }*/
  int pos = actualFile->position();
  actualFile->println(mess);
  if (pos == actualFile->position())
    erreur(21);
  else
    actualFile->flush();
}

void mode_standard()
{
  mode = 1;
  Serial.begin(115200);
  Serial.print(F("Initializing SD card..."));
  SD = new SdFat32();
  rtc = new RTC_DS1307();
  if (!SD->begin(4))
  {
    Serial.println(F("initialization failed!"));
    erreur(13);
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
}

void setup()
{
  mode_standard();
}

void loop()
{
  if (mode == 1)
    enregistrement();
  delay(1e3);
}