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
        String command = Serial.readStringUntil('=');
        Serial.println(command.c_str());
        Serial.println(command.indexOf("l"));
        int value = Serial.readStringUntil('\n').toInt();

        if (command.indexOf("LUMIN") > -1)
        {
            Serial.println("bla1");
            i = 0;
            MIN = 0;
            MAX = 1023;
        }

        if (command.indexOf("TEMP_AIR") > -1)
        {
            Serial.println("bla2");
            i = 1;
            MIN = -40;
            MAX = 85;
        }

        if (command.indexOf("HYGR") > -1)
        {
            Serial.println("bla3");
            i = 2;
            MIN = -40;
            MAX = 85;
        }

        if (command.indexOf("PRESSURE") > -1)
        {
            Serial.println("bla4");
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

        if (command.indexOf("MIN") > -1)
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

        if (command.indexOf("MAX") > -1)
        {
            if (value > MIN && value < MAX)
            {
                capteurs[i]->max = value;
                Serial.println("succès de l'operation !!");
                /*
                        for (int i; i < 9; i++)
                        {
                          Serial.println("Etat des maximum apres config : " + String(capteurs[i]->max));
                        }*/
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

                // Serial.println("Nouvelle valeur de log_interval" + String(log_interval));
            }
            else
            {
                Serial.println("Valeur trop basse !\n");
            }
        }

        if (command == "FILE_MAX_SIZE")
        {
            if (value < 16384 && value > 512)
            {
                file_max_size = value;

                // Serial.println("Nouvelle valeur de file_max_size" + String(file_max_size));
            }
            else
            {
                Serial.println("Valeur incohérente !\n");
            }
        }

        if (command == "RESET=0")
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
            // Serial.println("OKKKKKKKKKK");
        }

        if (command == "VERSION")
        {
            Serial.println(version);
        }

        if (command == "TIMEOUT")
        {
            if (value < 0)
            {
                for (int i = 0; i < 9; i++)
                {
                    capteurs[i]->timeout = value;
                }
            }
            else
            {
                Serial.println("NTM CA MARCHE PAS TA VALEUR !!!");
            }
        }

        for (int i = 0; i < 9; i++)
        {
            Serial.println("Capteur : " + String(i));
            Serial.println("Nouvelles valeurs actives des capteurs : " + String(capteurs[i]->actif));
            Serial.println("Nouvelles valeurs des seuil MIN : " + String(capteurs[i]->min));
            Serial.println("Nouvelles valeurs des seuil MAX : " + String(capteurs[i]->max));
        }
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
    Serial.println(String("nulnuuuuuuuwaittttdjgfsgfsdd").indexOf("aa") > 0);
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