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
    // int timeout; question a poser au prof
    int actif;
    int nombre_erreur;
    // int derniere_valeurs[];
    int tab_moy_index;
    int moyenne;
} Capteur;

Capteur capteurs[9];

int log_interval;
int file_max_size;
int version;
int num_lot;
int inactivite_config;
int compteur_revision;
int mode;
int compteur_taille_fichier;
int previous_mode;
float config_OCR1A;
int prescaler;

volatile byte *pointeur = &TCCR1B;

int particule;
int vent;
int temp_eau;
int courant;




void setup()
{
    pinMode(button1, INPUT);  // bouton
    pinMode(button2, INPUT);  // bouton
    pinMode(9, OUTPUT);
    pinMode(10, OUTPUT);
    pinMode(11, OUTPUT); // led
    attachInterrupt(digitalPinToInterrupt(3), timer, CHANGE);
    attachInterrupt(digitalPinToInterrupt(2), timer, CHANGE);
    TCCR1A = 0;
    TCCR1B = 0;
    //config_OCR1A = (16000000/(prescaler*1000))-1;
    Serial.begin(9600);
}


void timer() {
  if (!*pointeur) {
    TCNT1 = 0 ;

    TCCR1B |= B00000101;

    TIMSK1 |= B00000010;        //Set OCIE1A to 1 so we enable compare match B 

    //4. Set the value of register B to 31250//
    OCR1A = 39063;             //2.5s Finally we set compare register B to this value  
    
  } 
  else {              
       TCCR1B = 0;
  }
}
 /*
Mode “standard” : Le système est démarré normalement (sans bouton pressé) pour faire 
l’acquisition des données.

 Mode “configuration” : Le système est démarré avec le bouton rouge pressé. Il permet de 
configurer les paramètres du système, l’acquisition des capteurs est désactivée et le système 
bascule en mode standard au bout de 30 minutes sans activité.

 Mode “maintenance” : Accessible depuis le mode standard ou économique, il permet d’avoir 
accès aux données des capteurs directement depuis une interface série et permet de changer en 
toute sécurité la carte SD sans risque de corrompre les données. On y accède en appuyant 
pendant 5 secondes sur le bouton rouge. En appuyant sur le bouton rouge pendant 5 secondes, 
le système rebascule dans le mode précédent.

 Mode “économique” : Accessible uniquement depuis le mode standard, il permet d’économiser 
de la batterie en désactivant certains capteurs et traitements. On y accède en appuyant pendant 
5 secondes sur le bouton vert. En appuyant 5 secondes sur le bouton rouge, le système rebascule 
en mode standard.
*/


ISR(TIMER1_COMPA_vect)
{
  if (mode == MODE_ECO || mode == MODE_STANDARD)
  {
    gestionnaire_modes(MODE_MAINTENANCE);
  }

  if (mode == MODE_MAINTENANCE)
  {
    gestionnaire_modes(previous_mode);
  }

  if (mode == MODE_ECO)
  {
    gestionnaire_modes(MODE_STANDARD);
  }

  //gestionnaire_modes(MODE_CONFIGURATION);
}

void loop()
{
    if (mode == MODE_STANDARD)
    {
    }

    if (mode == MODE_CONFIGURATION)
    {
        //gestionnaire_modes(previous_mode);
    }

    if (mode == MODE_MAINTENANCE)
    {
    }

    if (mode == MODE_ECO)
    {
    }
}

void gestionnaire_modes(int nvmode)
{

    previous_mode = mode;

    switch (nvmode)
    {
    case 0:
        digitalWrite(9, HIGH); // en vert
        for (int i = 0; i < sizeof(capteurs) / sizeof(Capteur); i++)
        {
            capteurs->actif = 1;
        }
        previous_mode = MODE_STANDARD;
        break;

    case 1:
  
        digitalWrite(9, HIGH); // en jaune
        for (int i = 0; i < sizeof(capteurs) / sizeof(Capteur); i++)
        {
            capteurs->actif = 0;
        }
        break;

    case 2:

        digitalWrite(2, HIGH); // en orange
        for (int i = 0; i < sizeof(capteurs) / sizeof(Capteur); i++)
        {
            capteurs->actif = 1;
        }
        previous_mode = 3;
        break;

    case 3:

        digitalWrite(2, HIGH); // en bleu
        for (int i = 0; i < sizeof(capteurs) / sizeof(Capteur); i++)
        {
            // capteurs->actif = !(particule || temp_eau || vent || courant);
            if (capteurs->type == particule || capteurs->type == temp_eau || capteurs->type == vent || capteurs->type == courant)
            {
                capteurs->actif = 0;
            }
            else
                capteurs->actif = MODE_ECO;
        }
        previous_mode = 3;
        break;
    }
    mode = nvmode;
    Serial.println(mode);
}