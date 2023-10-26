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

// 0; //'luminosite';
// 1; //'temperature';

int etatled = 0;
int code_couleur = 4;
int compteur_sec = 0;

void setup()
{
  Serial.begin(9600);
  cli();               // Stop interrupts
  TCCR1A = 0;          // Reset TCCR1A à 0 (timer + comparateur)
  TCCR1B = 0;          // Reset TCCR1A à 0 (timer + comparateur)
  TCCR1B |= B00000100; // Met CS12 à 1 pour un prescaler à 256 (limite)
  TIMSK1 |= B00000010; // Met OCIE1A à 1 pour comparer le comparer au match A

  OCR1A = 31250; // Compteur max à 62500 pour 1 s
  sei();         // Remet en marche les interruptions

  pinMode(R, OUTPUT);
  pinMode(B, OUTPUT);
  pinMode(V, OUTPUT);
}

void loop()
{
}

ISR(TIMER1_COMPA_vect)
{
  TCNT1 = 0;
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
