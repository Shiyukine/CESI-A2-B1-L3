#define V 9
#define R 10
#define B 11

int etatled = 0;
int code_couleur = 2;
int compteur_sec = 0;

void setup() {
  Serial.begin(9600);
  cli();                    // Stop interrupts
  TCCR1A = 0;               // Reset TCCR1A à 0 (timer + comparateur)
  TCCR1B = 0;               // Reset TCCR1A à 0 (timer + comparateur)
  TCCR1B |= B00000100;      // Met CS12 à 1 pour un prescaler à 256 (limite)
  TIMSK1 |= B00000010;      // Met OCIE1A à 1 pour comparer le comparer au match A

  OCR1A = 62500;            // Compteur max à 62500 pour 1 s
  sei();                    // Remet en marche les interruptions

  pinMode(R, OUTPUT);
  pinMode(B, OUTPUT);
  pinMode(V, OUTPUT);

}

void loop()
{
}

ISR(TIMER1_COMPA_vect) {
  TCNT1 = 0;
  if(code_couleur == 2) {   // Erreur Horloge RTC
    if(etatled == 0) {
      analogWrite(R, 255);   
      analogWrite(B, 0);    
      etatled = 1;
    }
    else {
      analogWrite(R, 0);
      analogWrite(B, 255);
      etatled = 0;
    }
    Serial.println("Erreur accès horloge RTC");
  }

  if(code_couleur == 5) {   // Erreur GPS
    if(etatled == 0) {
      analogWrite(R, 255);
      analogWrite(V, 0);
      etatled = 1;
    }
    else {
      analogWrite(R, 255);
      analogWrite(V, 255);
      etatled = 0;
    }
    Serial.println("Erreur accès GPS");
  }

  if(code_couleur == 3) {   // Erreur capteurs accès
    if(etatled == 0) {
      analogWrite(R, 255);
      analogWrite(V, 0);
      etatled = 1;
    }
    else {
      analogWrite(R, 0);
      analogWrite(V, 255);
      etatled = 0;
    }
    Serial.println("Erreur accès capteur");
  }

  if (code_couleur == 4) { // Erreur capteur incohérente
    if(etatled == 0) {
           analogWrite(R, 0);
           analogWrite(V, 255);
          etatled = 1; 
    }
    else {
      if(compteur_sec == 1) {
          compteur_sec = 0;
          analogWrite(R, 255);
          analogWrite(V, 0);
          etatled = 0;
      }
      else {
        compteur_sec++;
      }
    }
    Serial.println("Erreur données incohérentes // Vérification matérielle requise");
  }

  if(code_couleur == 0) {   // Erreur carte SD pleine
    if(etatled == 0) {
      analogWrite(R, 255);
      analogWrite(V, 0);
      analogWrite(B, 0);
      etatled = 1;
    }
    else {
      analogWrite(R, 255);
      analogWrite(V, 255);
      analogWrite(B, 255);
      etatled = 0;
    }
    Serial.println("Erreur : Carte SD pleine");
  }

  if(code_couleur == 1) {   // Erreur écriture carte SD
    if(etatled == 0) {
      analogWrite(R, 255);
      analogWrite(V, 255);
      analogWrite(B, 255);
      etatled = 1;
    }
    else {
      if(compteur_sec == 1) {
        compteur_sec = 0;
          analogWrite(R, 255);
          analogWrite(V, 0);
          analogWrite(B, 0);
          etatled = 0;
      }
      else {
        compteur_sec++;
      }
    }
    Serial.println("Erreur accès/écriture sur carte SD");
  }
}

void gestionnaire_erreur(int erreur) {
  if(erreur == ERR_SD_PLEIN) {    // Erreur 0 pour carte SD pleine
    code_couleur = erreur;
  }

  if(erreur == ERR_SD_IO) {    // Erreur 1 pour écriture/accès carte SD
    code_couleur = erreur;
  }

  if(erreur == ERR_RTC) {    // Erreur 2 pour horloge RTC
    code_couleur = erreur;
  }

  if(erreur == ERR_CAPTEUR_ACCES) {    // Erreur 3 pour accès capteur
    code_couleur = erreur;
  }

  if(erreur == ERR_CAPTEUR_INCOHERENTE) {    // Erreur 4 pour incohérence
    code_couleur = erreur;
  }

  if(erreur == ERR_GPS) {       // Erreur 5 pour GPS
    code_couleur = erreur;
  }

  ISR(TIMER1_COMPA_vect);
}
