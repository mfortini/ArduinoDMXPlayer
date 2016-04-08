#include <SPI.h>
#include <SD.h>

#include <DmxSimple.h>

//#define DEBUG

#define DEpin 5
#define N_CHAN 24
#define LRpin 7
#define L2pin 8
#define STARTpin 6

#define CONFIGFILE "DMXSEQ.CSV"
#define MAX_TOKEN_LEN (10)
#define MAX_SEQ_LEN (100)

File configFile;

// change this to match your SD shield or module;
// Arduino Ethernet shield: pin 4
// Adafruit SD shields and modules: pin 10
// Sparkfun SD shield: pin 8
const int SDchipSelect = 10;


byte curBrightValues[N_CHAN];
byte nextBrightValues[N_CHAN];
unsigned long sequenceTime;

unsigned long nextDeadline = 0;
unsigned long curSeqTime;
unsigned long nextSeqTime;

bool panic = true;
bool sequenceOff = true;
bool startSequence = false;

void seekToEOL(File f)
{
  int c;

#ifdef DEBUG
  Serial.println("seekToEOL");
#endif

  while ( ((c = f.read()) >= 0) && (c != '\n')) {
#ifdef DEBUG
    Serial.println((char) c);
#endif
  }
}

/*Reads a config line */
int readConfigLine(File f, unsigned long *time_ms, byte *brightValues, int maxChan)
{
  int c = 0;
  int n = -1;
  char token[MAX_TOKEN_LEN];
  int cNum;

  if ( ((c = f.peek()) >= 0) && (c == '#')) {
    seekToEOL(f);
  }

  cNum = 0;
  while ((n < maxChan) && ( (c = f.read()) >= 0)) {
#ifdef DEBUG
    Serial.println((char) c);
#endif
    if ((c == ',') || (c == '\n')) {
      char *ptr;
      unsigned long val;

      token[cNum] = '\0';

      val = strtoul(token, &ptr, 10);

#ifdef DEBUG
      Serial.print(F("Token ")); Serial.println(token);
#endif

      if (ptr == token) {
        Serial.print(F("Cannot parse ")); Serial.println(token);
      } else {
        if (n < 0) {
          *time_ms = val;
          n++;
        } else {
          brightValues[n++] = val;
        }
      }

      cNum = 0;
    } else {
      if (!isSpace(c)) {
        token[cNum++] = c;

        if (cNum >= MAX_TOKEN_LEN) {
          Serial.println(F("Token overrun"));
          cNum = 0;
        }
      }
    }
  }

  if ((c >= 0) && (c != '\n')) {
    Serial.println(c);
    seekToEOL(f);
  }

  if (n < 0) {
    n = 0;
  }

  return n;
}


void setup() {
  int nRead;


  Serial.begin(115200);

  while (!Serial) {
    ;
  }

  if (!SD.begin(SDchipSelect)) {
    Serial.println(F("SD initialization failed!"));
    return;
  }
  Serial.println(F("SD initialization done."));

  /* The most common pin for DMX output is pin 3, which DmxSimple
   ** uses by default. If you need to change that, do it here. */
  DmxSimple.usePin(3);

  /* DMX devices typically need to receive a complete set of channels
   ** even if you only need to adjust the first channel. You can
   ** easily change the number of channels sent here. If you don't
   ** do this, DmxSimple will set the maximum channel number to the
   ** highest channel you DmxSimple.write() to. */
  DmxSimple.maxChannel(N_CHAN + 1);

  pinMode(DEpin, INPUT_PULLUP);
  pinMode(LRpin, OUTPUT);
  pinMode(L2pin, OUTPUT);
  pinMode(STARTpin, INPUT);

  digitalWrite(L2pin, OUTPUT);

  configFile = SD.open(CONFIGFILE, FILE_READ);

  if (!configFile) {
    Serial.println("Cannot open" CONFIGFILE);

    panic = true;

    return;
  }

  while (readConfigLine (configFile, &curSeqTime, curBrightValues, N_CHAN) == N_CHAN) {
    Serial.print (F("time ")); Serial.println (curSeqTime, 10);
    for (int i = 0; i < N_CHAN; i++) {
      Serial.print(curBrightValues[i]); Serial.print(", ");
    }
    Serial.println();
  }
  configFile.seek(0);

  panic = false;
}


void loop() {
  int c;
  int nRead;

  if (panic) {
    int brightness;
    for (brightness = 0; brightness <= 255; brightness++) {
      for (c = 1; c <= N_CHAN; c++) {
        /* Update DMX channel c to new brightness */
        DmxSimple.write(c, brightness);
      }
    }

    delay(10);

    Serial.println("panic");
  }

  if (sequenceOff) {
    if (digitalRead(STARTpin) == HIGH) {
      Serial.println (F("StartSequence"));
      startSequence = true;
      sequenceOff = false;
    }
    digitalWrite(LRpin, LOW);
    digitalWrite(L2pin, HIGH);
  } else {


    if (startSequence) {
      digitalWrite(LRpin, HIGH);
      digitalWrite(L2pin, LOW);
      startSequence = false;

      if (readConfigLine(configFile, &curSeqTime, nextBrightValues, N_CHAN) < N_CHAN) {
        panic = true;
        return;
      }
      nextDeadline = millis() + curSeqTime;
    } else {
      if (((long) (nextDeadline - millis())) < 0) {

        memcpy(curBrightValues, nextBrightValues, sizeof(byte) * N_CHAN);

        nRead = readConfigLine(configFile, &nextSeqTime, nextBrightValues, N_CHAN);
        if (nRead == 0) { // EOF
          Serial.println(F("End of sequence"));
          sequenceOff = true;
          configFile.seek(0);
        } else {
          if (nRead < N_CHAN) {
            panic = true;
            return;
          } else {
            nextDeadline += nextSeqTime - curSeqTime;
            curSeqTime = nextSeqTime;
          }
        }

      }
    }
  }
  for (c = 0; c < N_CHAN; c++) {
    DmxSimple.write(c + 1, curBrightValues[c]);
  }

}
