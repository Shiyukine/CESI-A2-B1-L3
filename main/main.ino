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
Capteur *capteurs[9];
bool sdMounted = false;
char *aa = "01";
char *mm = "01";
char *jj = "01";
SdFat32 SD;

void erreur(int erreur)
{
    Serial.println("Error " + String(erreur));
}

File *changement_fichier(int mess_size)
{
    static File actualFile = SD.open(String(aa) + String(mm) + String(jj) + "_" + String(compteur_revision) + ".log", O_RDWR | O_CREAT | O_TRUNC);
    if (!actualFile)
        erreur(1);
    bool changeFile = false;
    if (actualFile.position() + mess_size > file_max_size)
    {
        compteur_revision++;
        changeFile = true;
        compteur_taille_fichier = 0;
    }
    char *naa = "01";
    char *nmm = "01";
    char *njj = "01";
    /*if ((*naa) != (*aa) && (*nmm) != (*mm) && (*njj) != (*jj))
    {
        compteur_revision = 0;
        aa = naa;
        mm = nmm;
        jj = njj;
        changeFile = true;
    }*/
    if (changeFile)
    {
        Serial.println(String(aa) + String(mm) + String(jj) + "_" + String(compteur_revision) + ".log");
        File newFile = SD.open(String(aa) + String(mm) + String(jj) + "_" + String(compteur_revision) + ".log", O_RDWR | O_CREAT | O_TRUNC);
        if (!newFile)
        {
            compteur_revision--;
            erreur(5);
        }
        else
        {
            int lastPos = actualFile.position();
            actualFile.seek(0);
            int charstot = 0;
            while (actualFile.available())
            {
                if (!actualFile.getWriteError())
                {
                    newFile.write(actualFile.read());
                }
                else
                {
                    erreur(2);
                }
                charstot++;
            }
            newFile.flush();
            newFile.close();
            actualFile.seek(0);
            Serial.println("Changed file " + String(charstot));
        }
    }
    return &actualFile;
}

void enregistrement()
{
    if (sdMounted)
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
        actualFile->println(mess);
        if (actualFile->getWriteError())
            erreur(21);
        actualFile->flush();
    }
}

void mode_standard()
{
    Serial.begin(115200);
    Serial.print(F("Initializing SD card..."));
    if (!SD.begin(4))
    {
        Serial.println(F("initialization failed!"));
        erreur(13);
    }
    else
    {
        sdMounted = true;
        Serial.println(F("initialization done."));
    }
}

void setup()
{
    mode_standard();
}

void loop()
{
    enregistrement();
    delay(1e3);
}