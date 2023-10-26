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

typedef struct // Structure de nos capteurs.
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

Capteur *capteurs[9]; // tableau contenant nos 9 capteurs

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
  Serial.begin(9600); // lancement du port serie.

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

  sei(); // Reactivation des interruptions.

  attachInterrupt(digitalPinToInterrupt(2), timer, CHANGE); // interruptions materielles pour les 2 boutons.
  attachInterrupt(digitalPinToInterrupt(3), timer, CHANGE);
  TCCR1A = 0; // désactivez tous les modes de fonctionnement spéciaux et configurez le timer en mode normal
  TCCR1B = 0; // désactivez le Timer 1 en configurant le préscaleur à 0

  Serial.println("Mode de lancement : " + String(mode));

  for (int i = 0; i < 9; i++) // allocation mémoire des capteurs de la structure.
  {
    capteurs[i] = calloc(1, sizeof(Capteur));
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

} // fin setup

void timer()
{ // fonction d'interruption logiciel appelé par l'attachInterrupt.

  TCNT1 = 0;           // compteur du timer a 0.
  TCCR1B |= B00000101; // configure le registre TCCR1B pour activer le Timer 1 en mode CTC avec un préscaleur de 1024.
  TIMSK1 |= B00000010; // Cela active l'interruption de correspondance avec le registre OCR1A pour le Timer 1.
  OCR1A = 39063;       // configure la valeur de comparaison du Timer 1 à 39063. (2.5 secondes.)
}

ISR(TIMER1_COMPA_vect) // (Interruption Service Routine)
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

void loop() // test de loop rapide (a ne pas prendre en compte)
{
  if (mode == MODE_CONFIGURATION)
  {
    get_commande(); // appel de get commande en monde configuration.
  }
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
    break;

  case 1: // config

    analogWrite(10, 255);
    analogWrite(9, 120);
    digitalWrite(11, LOW); // en jaune
    for (int i = 0; i < sizeof(capteurs) / 2; i++)
    {
      capteurs[i]->actif = 0; // desactive tout les capteurs en config.
    }
    break;

  case 2: // maintenance
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
