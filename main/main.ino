#include <RTClib.h>
#include <SdFat.h>

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
int mode = 0;
Capteur *capteurs[9];
int year;
int month;
int day;
SdFat32 *SD;
RTC_DS1307 *rtc;
int previous_mode = -1;
int inactivite = 0;

void get_commande()
{
    int i;
    int MIN;
    int MAX;

    if (Serial.available() != 0)
    {
        inactivite = millis();
        String command = Serial.readStringUntil('=');
        int value = Serial.readStringUntil('\n').toInt();

        if (command.indexOf("LUMIN") > 0)
        {
            i = 4;
            MIN = 0;
            MAX = 1023;
        }

        if (command.indexOf("TEMP") > 0)
        {
            i = 1;
            MIN = -40;
            MAX = 85;
        }

        if (command.indexOf("HYGR") > 0)
        {
            i = 2;
            MIN = -40;
            MAX = 85;
        }

        if (command.indexOf("PRESSURE") > 0)
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
            }
            else
            {
                Serial.println("veuillez entrer une valeur entre " + String(MIN) + " et " + String(MAX) + ".\n");
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

void erreur(int erreur)
{
    Serial.println("Error " + String(erreur));
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
    change_mode(1);
}

void loop()
{
    if (mode == 1 && millis() - inactivite >= 2000)
        change_mode(0);
    else
    {
        get_commande();
    }
}