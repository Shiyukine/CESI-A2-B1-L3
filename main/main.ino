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
        actualFile = SD->open(String(now->year()) + String(mm) + String(jj) + "_" + String(compteur_revision) + ".LOG", O_RDWR | O_CREAT | O_TRUNC);
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
        Serial.println(aa + mm + jj + "_" + String(compteur_revision) + ".LOG");
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