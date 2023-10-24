void setup()
{
    int a = 2;
}
//424664
void loop()
{
    typedef struct Capteur
{
    int type;
    int min;
    int max;
    //int timeout; question a poser au prof
    int actif;
    int nombre_erreur;
    float derniere_valeur[10];
    int tab_moy_index;
    float moyenne;
} Capteur;

Capteur temperature;

float moyenne(Capteur *capteuri){
    float moyenne = 0;
    for(int i=0; i<10; i++){
        moyenne = moyenne + capteuri->derniere_valeur[i];
    }
    moyenne = moyenne/10;  
    return moyenne;
}

}