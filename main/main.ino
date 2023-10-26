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
  int actif;
  int nombre_erreur;
  // int derniere_valeurs[];
  int tab_moy_index;
  int moyenne;
} Capteur;

Capteur *capteurs[9];

int log_interval;
int file_max_size;
float version = 1.0;
int num_lot;
int inactivite_config;
int compteur_revision;
int mode; // le goat
int compteur_taille_fichier;
int previous_mode;
float config_OCR1A;
int prescaler;

int GPS;
int hygrometrie;
int temperature;
int pression;
int luminosite;
int particule;
int vent;
int temp_eau;
int courant;

int bouton_test;

void setup()

{
  Serial.begin(9600);

  cli();
  pinMode(button1, INPUT);
  pinMode(button2, INPUT);
  pinMode(9, OUTPUT);  // Vert
  pinMode(10, OUTPUT); // Rouge
  pinMode(11, OUTPUT); // Bleu

  mode = MODE_STANDARD;
  digitalWrite(9, HIGH);

  while (digitalRead(3) == HIGH)
  {
    mode = MODE_CONFIGURATION;
    digitalWrite(11, HIGH);
    digitalWrite(10, HIGH);
    digitalWrite(9, HIGH);
  }

  sei();

  attachInterrupt(digitalPinToInterrupt(2), timer, CHANGE);
  attachInterrupt(digitalPinToInterrupt(3), timer, CHANGE);
  TCCR1A = 0;
  TCCR1B = 0;

  Serial.println("Mode de lancement : " + String(mode));

  for (int i = 0; i < 9; i++)
  {
    capteurs[i] = calloc(1, sizeof(Capteur));
    capteurs[i]->type = i;
  }

  for (int i = 0; i < 9; i++)
  {
    Serial.println("Listes des capteurs : " + String(capteurs[i]->type));
  }

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

} // fin setup

void timer()
{

  TCNT1 = 0;
  TCCR1B |= B00000101;
  TIMSK1 |= B00000010;
  OCR1A = 39063; // 2.5s Finally we set compare register B to this value

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

ISR(TIMER1_COMPA_vect)
{
  switch (mode) // 2 rouge // 3 jaune
  {
  case MODE_STANDARD:
    if (digitalRead(2) == HIGH)
    {
      gestionnaire_modes(MODE_MAINTENANCE);
      break;
    }
    if (digitalRead(3) == HIGH)
    {
      gestionnaire_modes(MODE_ECO);
      break;
    }

  case MODE_ECO:
    if (digitalRead(3) == HIGH)
    {
      gestionnaire_modes(MODE_STANDARD);
      break;
    }
    if (digitalRead(2) == HIGH)
    {
      gestionnaire_modes(MODE_MAINTENANCE);
      break;
    }

  case MODE_MAINTENANCE:
    if (digitalRead(2) == HIGH)
    {
      gestionnaire_modes(previous_mode);
      break;
    }
  }
}

void loop()
{
  if (mode == MODE_CONFIGURATION) // A REVOIR
  {
    get_commande();
  }
}

void gestionnaire_modes(int nvmode)
{
  previous_mode = mode;

  switch (nvmode)
  {
  case 0: // standard
    digitalWrite(9, HIGH);
    digitalWrite(10, LOW);
    digitalWrite(11, LOW); // en vert
    for (int i = 0; i < sizeof(capteurs) / 2; i++)
    {
      capteurs[i]->actif = 1;
    }
    break;

  case 1: // config

    analogWrite(10, 255);
    analogWrite(9, 120);
    digitalWrite(11, LOW); // en jaune
    for (int i = 0; i < sizeof(capteurs) / 2; i++)
    {
      capteurs[i]->actif = 0;
    }
    break;

  case 2: // maintenance
    analogWrite(10, 255);
    analogWrite(11, 165);
    digitalWrite(9, LOW); // en violet (au lieu de orange)
    for (int i = 0; i < sizeof(capteurs) / 2; i++)
    {
      capteurs[i]->actif = 1;
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
        capteurs[i]->actif = 0;
      }
      else
        capteurs[i]->actif = 1;
    }
    break;
  }
  mode = nvmode;
  Serial.println("previous mode : " + String(previous_mode));
  Serial.println("Changement de mode : " + String(mode));
  for (int i = 0; i < sizeof(capteurs) / 2; i++)
  {
    Serial.println("Liste des etats des capteurs : " + String(capteurs[i]->actif));
  }
}

void get_commande()
{
  int i;
  int MIN;
  int MAX;

  if (Serial.available() != 0)
  {
    String command = Serial.readStringUntil('=');
    int value = Serial.readStringUntil('\n').toInt();

    Serial.println(command);

    if (command.startsWith("LUMIN"))
    {
      i = 0;
      MIN = 0;
      MAX = 1023;
    }

    else if (command.startsWith("TEMP_AIR"))
    {
      i = 1;
      MIN = -40;
      MAX = 85;
    }

    else if (command.startsWith("HYGR"))
    {
      i = 2;
      MIN = -40;
      MAX = 85;
    }

    else if (command.startsWith("PRESSURE"))
    {
      i = 3;
      MIN = 300;
      MAX = 1100;
    }

    if (command == "LUMIN" || command == "TEMP_AIR" || command == "HYGR" || command == "PRESSURE")
    {
      if (value == 0 || value == 1)
      {
        capteurs[i]->actif = value;
        Serial.println("succès de l'operation !!");
        /*
                for (int i; i < 9; i++)
                {
                  Serial.println("Etat des capteurs apres config : " + String(capteurs[i]->actif));
                }*/
      }
      else
      {
        Serial.println("veuillez entrer une valeur entre 0 et 1.\n");
      }
      command = "None";
    }

    if (command.indexOf("MIN") > 0)
    {
      if (value > MIN && value < MAX)
      {
        capteurs[i]->min = value;
        Serial.println("succès de l'operation !!");
        /*
                for (int i; i < 9; i++)
                {
                  Serial.println("Etat des minimum apres config : " + String(capteurs[i]->min));
                }*/
      }
      else
      {
        Serial.println("veuillez entrer une valeur entre " + String(MIN) + " et " + String(MAX) + ".\n");
      }
      command = "None";
    }

    if (command.indexOf("MAX") > 0)
    {
      if (value > MIN && value < MAX)
      {
        capteurs[i]->max = value;
        Serial.println("succès de l'operation !!");

        for (int i; i < 9; i++)
        {
          Serial.println("Etat des maximum apres config : " + String(capteurs[i]->max));
        }
      }

      else
      {
        Serial.println("veuillez entrer une valeur entre " + String(MIN) + " et " + String(MAX) + ".\n");
      }
      command = "None";
    }

    if (command == "LOG_INTERVALL")
    {
      if (value > 0)
      {
        log_interval = value;

        Serial.println("Nouvelle valeur de log_interval : " + String(log_interval));
      }
      else
      {
        Serial.println("Valeur trop basse pour LOG_INTERVAL !\n");
      }
    }

    if (command == "FILE_SIZE")
    {
      if (value < 16384 && value > 512)
      {
        file_max_size = value;

        Serial.println("Nouvelle valeur de file_max_size : " + String(file_max_size));
      }
      else
      {
        Serial.println("Valeur incohérente pour FILE_MAX_SIZE !\n");
      }
    }

    if (command == "RESET")
    {
      file_max_size = DEFAULT_FILE_MAX_SIZE;
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

    if (command == "VERSION")
    {
      Serial.println("Version : " + String(version));
    }

    if (command == "TIMEOUT")
    {
      if (value > 0)
      {
        for (int i = 0; i < 9; i++)
        {
          capteurs[i]->timeout = value;
        }
        Serial.println("timeout : " + String(value));
      }
      else
      {
        Serial.println("NTM CA MARCHE PAS TA VALEUR !!!");
      }
    }

    for (int i = 0; i < 9; i++)
    {
      Serial.println("Nouvelles valeurs actives des capteurs : " + String(capteurs[i]->actif));
      Serial.println("Nouvelles valeurs des seuil MIN : " + String(capteurs[i]->min));
      Serial.println("Nouvelles valeurs des seuil MAX : " + String(capteurs[i]->max));
    }
  }
}