/*
   SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4 (for MKRZero SD: SDCARD_SS_PIN)
*/

#include <SPI.h>
#include <SD.h>

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
char *aa = "01";
char *mm = "01";
char *jj = "01";

void erreur(int erreur, File *actualFile)
{
    Serial.println("Error " + String(erreur));
    Serial.println(String(actualFile->getWriteError()));
}

File *changement_fichier()
{
    static File actualFile = SD.open((*aa) + (*mm) + (*jj) + "_" + String(compteur_revision) + ".log", FILE_WRITE | FILE_READ);
    static bool firstCall = true;
    if (firstCall)
    {
        Serial.println("yo");
        actualFile.seek(0);
        while (actualFile.available())
        {
            Serial.write(actualFile.read());
        }
        Serial.println("\ndayo");
        actualFile.seek(0);
    }
    firstCall = false;
    if (!actualFile)
        erreur(1, &actualFile);
    bool changeFile = false;
    if (compteur_taille_fichier + 256 > 2048)
    {
        compteur_revision++;
        changeFile = true;
    }
    char *naa = "01";
    char *nmm = "01";
    char *njj = "01";
    if ((*naa) != (*aa) && (*nmm) != (*mm) && (*njj) != (*jj))
    {
        compteur_revision = 0;
        aa = naa;
        mm = nmm;
        jj = njj;
        changeFile = true;
    }
    if (changeFile)
    {
        File newFile = SD.open((*aa) + (*mm) + (*jj) + "_" + String(compteur_revision) + ".log", FILE_WRITE);
        actualFile.seek(0);
        while (actualFile.available())
        {
            if (newFile.availableForWrite())
                newFile.print(actualFile.read());
            else
                erreur(2, &actualFile);
        }
        newFile.flush();
        newFile.close();
        actualFile.write("");
    }
    return &actualFile;
}

void enregistrement()
{
    File *actualFile = changement_fichier();
    /*for (int i = 0; i < sizeof(capteurs) / sizeof(capteurs[0]); i++)
    {
        if (actualFile->availableForWrite())
            actualFile->print("25 ");
        else
            erreur(20);
        actualFile->flush();
    }*/
    if (actualFile->availableForWrite())
        actualFile->write("Capteur 1 = 25, Capteur 2 = 25, Capteur 3 = 25, Capteur 4 = 25, Capteur 5 = 25, Capteur 6 = 25, Capteur 7 = 25, Capteur 8 = 25, Capteur 9 = 25 aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    // actualFile->write("25 25 25 25 25 25 25 25 25");
    // actualFile->println("Capteur 1 = 25, Capteur 2 = 25, Capteur 3 = 25, Capteur 4 = 25, Capteur 5 = 25, Capteur 6 = 25, Capteur 7 = 25, Capteur 8 = 25, Capteur 9 = 25");
    else
        erreur(21, actualFile);
    // actualFile->flush();
}

void mode_standard()
{
    Serial.begin(115200);

    Serial.print("Initializing SD card...");

    if (!SD.begin(4))
    {
        Serial.println("initialization failed!");
        erreur(13, NULL);
        while (1)
            ;
    }
    else
        Serial.println("initialization done.");
}

void setup()
{
    mode_standard();
}

void loop()
{
    enregistrement();
    delay(2e3);
}