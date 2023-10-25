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

Capteur *capteurs[9];

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

//int temps_global = millis();

void setup()
{
    mode = MODE_STANDARD;
    pinMode(button1, INPUT);  // bouton
    pinMode(button2, INPUT);  // bouton
    pinMode(9, OUTPUT);
    pinMode(10, OUTPUT);
    pinMode(11, OUTPUT); // led
    attachInterrupt(digitalPinToInterrupt(2), timer, CHANGE);
    attachInterrupt(digitalPinToInterrupt(3), timer, CHANGE);
    TCCR1A = 0;
    TCCR1B = 0;
    //config_OCR1A = (16000000/(prescaler*1000))-1;
    Serial.begin(9600);
    Serial.println("Mode de lancement : " + String(mode));
    digitalWrite(9, HIGH);

    for (int i = 0; i < 9; i++)
    {
      capteurs[i] = calloc(1, sizeof(Capteur));
    }

    capteurs[0]->type = 0; //'GPS';
    capteurs[1]->type = 1; //'hygrometrie';
    capteurs[2]->type = 2; //'temperature';
    capteurs[3]->type = 3; //'pression';
    capteurs[4]->type = 4; //'luminosite';
    capteurs[5]->type = 5; //'particule';
    capteurs[6]->type = 6; //'vent';
    capteurs[7]->type = 7; //'temp_eau';
    capteurs[8]->type = 8; //'courant';
    /*
    for (int i = 0; i < 9; i++)
    {
      Serial.println(capteurs[i]->type);
    }*/
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


ISR(TIMER1_COMPA_vect)
{
  
  if (digitalRead(2) == HIGH)
  {
    bouton_test = 2;
    //Serial.println(bouton_test);
  }

  if (digitalRead(3) == HIGH)
  {
    bouton_test = 3;
    //Serial.println(bouton_test);
  }


    switch (mode)
    {
      case MODE_STANDARD:
        if (bouton_test == 2) 
        {
          gestionnaire_modes(MODE_MAINTENANCE);           
          break;
        }   
        else
        {
          gestionnaire_modes(MODE_ECO);           
          break;
        }                
      
      case MODE_ECO:
        if (bouton_test == 3) 
        {
          gestionnaire_modes(MODE_STANDARD);        
          break;
        }  

      case MODE_MAINTENANCE:                              
        if (bouton_test == 2) 
        {
          gestionnaire_modes(previous_mode);        
          break;
        }  
        
    }
}


void loop()
{
  get_commande();
    //if (mode == MODE_STANDARD || mode == MODE_ECO)
    //{
      //tab_recu = Acquisition();     // appel de acquisition
      //Moyenne();     // Moyenne capteur
      //Changement_fichier();   // appel changement fichier
      //Enregistrement(tab_recu);   //appel enregistrement
    //}

    if (mode == MODE_MAINTENANCE)  // A REVOIR
    {
      int temps = millis();
      //get_commande();
      if (temps > 10000) 
      {
        gestionnaire_modes(previous_mode);
      }
    }

    //if (mode == MODE_MAINTENANCE)
    //{
      //tab_recu = Acquisition();     // appel de acquisition
      //Moyenne();     // Moyenne capteur
      //Serialprintln("Mode maintenance acitvée, Voici les données des capteurs", tab_recu);    }
}

void gestionnaire_modes(int nvmode)
{

    previous_mode = mode;

    switch (nvmode)
    {
    case 0:
        digitalWrite(9, HIGH);
        digitalWrite(10, LOW);
        digitalWrite(11, LOW); // en vert
        for (int i = 0; i < sizeof(capteurs) / 2; i++)
        {
            capteurs[i]->actif = 1;
        }
        //previous_mode = MODE_STANDARD;
        break;

    case 1:
  
        digitalWrite(9, HIGH);
        digitalWrite(11, HIGH);
        digitalWrite(10, LOW); // en jaune
         // en jaune
        for (int i = 0; i < sizeof(capteurs) / 2; i++)
        {
            capteurs[i]->actif = 0;
        }
        break;

    case 2:
        digitalWrite(10, HIGH);
        digitalWrite(11, HIGH);
        digitalWrite(9, LOW); // en orange
        for (int i = 0; i < sizeof(capteurs) / 2; i++)
        {
            capteurs[i]->actif = 1;
            
        }
        
        
        //previous_mode = 3;
        break;

    case 3:

        digitalWrite(10, LOW);
        digitalWrite(9, LOW);
        digitalWrite(11, HIGH); // en bleu
        for (int i = 0; i < sizeof(capteurs) / 2; i++)
        {
            // capteurs[i]->actif = !(particule || temp_eau || vent || courant);
            if (capteurs[i]->type == particule || capteurs[i]->type == temp_eau || capteurs[i]->type == vent || capteurs[i]->type == courant)
            {
                capteurs[i]->actif = 0;
            }
            else
                capteurs[i]->actif = MODE_ECO;
        }
        //previous_mode = 3;
        break;
    }
    mode = nvmode;
    Serial.println("Changement de mode" + String(mode));
    for (int i = 0; i < sizeof(capteurs) / 2; i++)
        {
          Serial.println(capteurs[i]->actif);
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

    if(command.indexOf("LUMIN") > 0)
    {
      i = 4;
      MIN = 0;
      MAX = 1023;
    }

    if(command.indexOf("TEMP") > 0)
    {
      i = 1;
      MIN = -40;
      MAX = 85;
    }

    if(command.indexOf("HYGR") > 0)
    {
      i = 2;
      MIN = -40;
      MAX = 85;
    }

    if(command.indexOf("PRESSURE") > 0)
    {
      i = 3;
      MIN = 300;
      MAX = 1100;
    }

    if(command == "LUMIN" || command == "TEMP_AIR" || command == "HYGR" || command == "PRESSURE")
    {
      if (value == 0 || value == 1)
      {
        capteurs[i]->actif = value;
        Serial.println("succès de l'operation !!");
      }
      else
      {
        Serial.println("veuillez entrer une valeur entre 0 et 1.\n");
      }
      command = "None";
    }
    

    if(command.indexOf("MIN") > 0)
    {
      if (value > MIN && value < MAX)
      {
        capteurs[i]->min = value;
        Serial.println("succès de l'operation !!");
      }
      else
      {
        Serial.println("veuillez entrer une valeur entre "+ String(MIN) + " et " + String(MAX) + ".\n");
      }
      command = "None";
    }


    if(command.indexOf("MAX") > 0)
    {
      if (value > MIN && value < MAX)
      {
        capteurs[i]->max = value;
        Serial.println("succès de l'operation !!");
      }
      else
      {
        Serial.println("veuillez entrer une valeur entre "+ String(MIN) + " et " + String(MAX) + ".\n");
      }
      command = "None";
    }


    /*
    for (int i = 0; i < 9; i++)
    {
      Serial.println("Nouvelles valeurs actives des capteurs : " + String(capteurs[i]->actif));
      Serial.println("Nouvelles valeurs des seuil MIN : " + String(capteurs[i]->min));
      Serial.println("Nouvelles valeurs des seuil MAX : " + String(capteurs[i]->max));
    }*/

  }
}


