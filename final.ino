#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <stdio.h>
#include <Fonts/FreeMono9pt7b.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define TEMP 16                     // пин на датчике температуры
 
OneWire oneWire(TEMP);              // инициализация объекта датчика
DallasTemperature ds(&oneWire);    

#define TFT_CS        10
#define TFT_RST        9
#define TFT_DC         8

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Пины и переменные
int xpin = A0;
int ypin = A1;
volatile int flag = 1;
volatile int a = 0;
volatile int b = 0;
volatile int direction = 0;
volatile bool inputReady = false;
unsigned long previousDisplayTime = 0;
unsigned long previousSensorTime = 0;
unsigned long sensorRequestTime = 0;
const unsigned long displayInterval = 50;    // 20 FPS
const unsigned long sensorInterval = 1000;   // Чтение каждую секунду
const int encoderPin = 3;  // Пин с поддержкой прерываний
volatile long pulseCount = 0;
long oldTime = 0;
int flagg = 0;  // Добавлена инициализация flag

float currentTemperature = 0.0;
bool sensorReading = false;
bool sensorDataReady = false;

bool timerRunning = false;
bool timerPaused = false;
unsigned long previousMillis = 0;
unsigned long remainingTime = 0;

char temp[12] = "Temperature";
char rpm[4] = "RPM";
char t[5] = "Time";
char *strings[] = {
    "Temperat.",
    "RPM",
    "Time",
    "Enter"
};
char *buttonText = "Enter";

int timenums[4] = {0,0,0,0};
int rpmnums[3] = {0,0,0};
int tempnums[3] = {0,0,0};
int numbers[3][4] = {{0,0,0,0},{0,0,0,0},{0,0,0,0}};

// Флаги для обработки в основном цикле
volatile bool updateDisplay = false;
volatile bool changeNumber = false;
volatile int changeRow = 0;
volatile int changePosition = 0;
volatile int changeDirection = 0;

float targetTemperature = 0.0;      // Желаемая температура
int targetRPM = 0;                  // Желаемые RPM  
unsigned long targetTime = 0;       // Желаемое время в секундах
bool valuesSaved = false;           // Флаг что значения сохранены
float previousDisplayedTemp = -100; // Для отслеживания изменений температуры
bool forceTemperatureRedraw = false;

bool alarmActive = false;
unsigned long alarmStartTime = 0;
const unsigned long alarmDuration = 3000; // 3 секунды

// Прототипы функций
void updateTimeDisplay();
int calculateTotalTime();
void updateRemainingTimeFromNumbers();
void HowManyNumbers(int a);
void updateButtonDisplay();
void Selectstr(int a);
void Selectnum1(int b);
void Selectnum2(int b);
void Selectnum3(int b);
void ChangeNumber(int row, int position, int direction);
void handleTimer();
void handleMenuInput();

// Функция обработки прерывания по таймеру
void timerISR() {
    // Чтение аналоговых значений
    int x = analogRead(xpin);
    int y = analogRead(ypin);
    
    // Определение направления
    int newDirection = 0;
    
    if ((x < 750) && (x > 250) && (y < 750) && (y > 250)) {
        flag = 1;
        newDirection = 0;
    }
    
    if ((flag)) {
        if ((x > 950) && (y > 200) && (y < 800)) {
            flag = 0;
            newDirection = 1;
        } else if ((x < 50) && (y > 200) && (y < 800)) {
            flag = 0;
            newDirection = 2;
        } else if ((y > 950) && (x > 200) && (x < 800)) {
            flag = 0;
            newDirection = 3;
        } else if ((y < 50) && (x > 200) && (x < 800)) {
            flag = 0;
            newDirection = 4;
        }
    }
    
    if (newDirection != direction) {
        direction = newDirection;
        inputReady = true;
    }
}

void updateTimeDisplay() {
    int totalSeconds = remainingTime;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    
    numbers[2][3] = minutes / 10;
    numbers[2][2] = minutes % 10;
    numbers[2][1] = seconds / 10;
    numbers[2][0] = seconds % 10;
    
    if (a == 2 && b > 0) {
        Selectnum3(b);
    } else {
        tft.fillRect(74,22,36,11,ST77XX_BLACK);
        tft.setTextColor(ST77XX_WHITE);
        tft.setCursor(76,24);
        tft.print(numbers[2][3]);
        tft.setCursor(83,24);
        tft.print(numbers[2][2]);
        tft.setCursor(88,24);
        tft.print(":");
        tft.setCursor(93,24);
        tft.print(numbers[2][1]);
        tft.setCursor(100,24);
        tft.print(numbers[2][0]);
    }
}

int calculateTotalTime() {
    int minutes = numbers[2][3] * 10 + numbers[2][2];
    int seconds = numbers[2][1] * 10 + numbers[2][0];
    return minutes * 60 + seconds;
}

void updateRemainingTimeFromNumbers() {
    remainingTime = calculateTotalTime();
}

void updateButtonDisplay() {
    tft.fillRect(24,33,83,11,ST77XX_BLACK);
    tft.setCursor(26,35);
    tft.setTextColor(ST77XX_WHITE);
    
    if (a == 3) {
        tft.fillRect(24,33,83,11,ST77XX_WHITE);
        tft.setTextColor(ST77XX_BLACK);
    }
    
    tft.print(buttonText);
    
    // Отображаем "-> Yes" только для четвёртой строки
    if (a == 3) {
        tft.setCursor(70,35);
        tft.print("-> Yes");
    } else {
        tft.fillRect(70,33,40,11,ST77XX_BLACK); // очищаем область "-> Yes"
    }
}

void Selectstr(int a){
  int c=a;
  if(c<3){
  tft.fillRect(0,11*c,72,11,ST77XX_WHITE);
  tft.fillRect(0,11*((c+2)%3),72,11,ST77XX_BLACK);
  tft.fillRect(0,11*((c+4)%3),72,11,ST77XX_BLACK);
  tft.setCursor(2, 2+11*c);
  tft.setTextColor(ST77XX_BLACK);
  tft.print(strings[c]);
  tft.setCursor(2, 2+11*((c+2)%3));
  tft.setTextColor(ST77XX_WHITE);
  tft.print(strings[((c+2)%3)]);
  tft.setCursor(2, 2+11*((c+4)%3));
  tft.print(strings[((c+4)%3)]);
  updateButtonDisplay();
  }
  else{
    tft.fillRect(0,0,72,11,ST77XX_BLACK);
    tft.fillRect(0,11,72,11,ST77XX_BLACK);
    tft.fillRect(0,22,72,11,ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(2,2);
    tft.print(strings[0]);
    tft.setCursor(2,13);
    tft.print(strings[1]);
    tft.setCursor(2,24);
    tft.print(strings[2]);
    updateButtonDisplay();
  }
}

// Три функции листания столбов (по одной на строку)
void Selectnum1(int b){
  int c=b;
  if(c==0){
    tft.fillRect(0,0,72,11,ST77XX_WHITE);
    tft.setCursor(2, 2);
    tft.setTextColor(ST77XX_BLACK);
    tft.print(strings[0]);
    tft.fillRect(74,0,36,11,ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(76,2);
    tft.print(numbers[0][3]);
    tft.setCursor(83,2);
    tft.print(numbers[0][2]);
    tft.setCursor(88,2);
    tft.print(".");
    tft.setCursor(94,2);
    tft.print(numbers[0][1]);
    tft.setCursor(101,2);
    tft.print("C");
  }
  else if(c==1){
    tft.fillRect(0,0,72,11,ST77XX_BLACK);
    tft.setCursor(2, 2);
    tft.setTextColor(ST77XX_WHITE);
    tft.print(strings[0]);
    tft.fillRect(74,0,36,11,ST77XX_BLACK);
    tft.fillRect(75,0,7,11,ST77XX_WHITE);
    tft.setTextColor(ST77XX_BLACK);
    tft.setCursor(76,2);
    tft.print(numbers[0][3]);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(83,2);
    tft.print(numbers[0][2]);
    tft.setCursor(88,2);
    tft.print(".");
    tft.setCursor(94,2);
    tft.print(numbers[0][1]);
    tft.setCursor(101,2);
    tft.print("C");
  }
  else if(c==2){
    tft.fillRect(74,0,36,11,ST77XX_BLACK);
    tft.fillRect(82,0,7,11,ST77XX_WHITE);
    tft.setTextColor(ST77XX_BLACK);
    tft.setCursor(83,2);
    tft.print(numbers[0][2]);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(76,2);
    tft.print(numbers[0][3]);
    tft.setCursor(88,2);
    tft.print(".");
    tft.setCursor(94,2);
    tft.print(numbers[0][1]);
    tft.setCursor(101,2);
    tft.print("C");
  }
  else if(c==3){
    tft.fillRect(0,0,72,11,ST77XX_BLACK);
    tft.setCursor(2, 2);
    tft.setTextColor(ST77XX_WHITE);
    tft.print(strings[0]);
    tft.fillRect(74,0,36,11,ST77XX_BLACK);
    tft.fillRect(93,0,7,11,ST77XX_WHITE);
    tft.setTextColor(ST77XX_BLACK);
    tft.setCursor(94,2);
    tft.print(numbers[0][1]);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(76,2);
    tft.print(numbers[0][3]);
    tft.setCursor(83,2);
    tft.print(numbers[0][2]);
    tft.setCursor(88,2);
    tft.print(".");
    tft.setCursor(101,2);
    tft.print("C");
  }

  
  // Если вышли из режима редактирования (b=0), обновляем отображение температуры
  if (c == 0) {
    forceTemperatureRedraw = true;
    updateTemperatureDisplay();
  }
}

void Selectnum2(int b){
  int c=b;
  if(c==0){
    tft.fillRect(0,11,72,11,ST77XX_WHITE);
    tft.setCursor(2, 13);
    tft.setTextColor(ST77XX_BLACK);
    tft.print(strings[1]);
    tft.fillRect(74,11,36,11,ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(76,13);
    tft.print(numbers[1][3]);
    tft.setCursor(83,13);
    tft.print(numbers[1][2]);
    tft.setCursor(90,13);
    tft.print(numbers[1][1]);
  }
  else if(c==1){
    tft.fillRect(0,11,72,11,ST77XX_BLACK);
    tft.setCursor(2, 13);
    tft.setTextColor(ST77XX_WHITE);
    tft.print(strings[1]);
    tft.fillRect(74,11,36,11,ST77XX_BLACK);
    tft.fillRect(75,11,7,11,ST77XX_WHITE);
    tft.setTextColor(ST77XX_BLACK);
    tft.setCursor(76,13);
    tft.print(numbers[1][3]);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(83,13);
    tft.print(numbers[1][2]);
    tft.setCursor(90,13);
    tft.print(numbers[1][1]);
  }
  else if(c==2){
    tft.fillRect(74,11,36,11,ST77XX_BLACK);
    tft.fillRect(82,11,7,11,ST77XX_WHITE);
    tft.setTextColor(ST77XX_BLACK);
    tft.setCursor(83,13);
    tft.print(numbers[1][2]);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(76,13);
    tft.print(numbers[1][3]);
    tft.setCursor(90,13);
    tft.print(numbers[1][1]);
  }
  else if(c==3){
    tft.fillRect(0,11,72,11,ST77XX_BLACK);
    tft.setCursor(2, 13);
    tft.setTextColor(ST77XX_WHITE);
    tft.print(strings[1]);
    tft.fillRect(74,11,36,11,ST77XX_BLACK);
    tft.fillRect(89,11,7,11,ST77XX_WHITE);
    tft.setTextColor(ST77XX_BLACK);
    tft.setCursor(90,13);
    tft.print(numbers[1][1]);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(76,13);
    tft.print(numbers[1][3]);
    tft.setCursor(83,13);
    tft.print(numbers[1][2]);
  }
}

void Selectnum3(int b){
  int c=b;
  if(c==0){
    tft.fillRect(0,22,72,11,ST77XX_WHITE);
    tft.setCursor(2, 24);
    tft.setTextColor(ST77XX_BLACK);
    tft.print(strings[2]);
    tft.fillRect(74,22,36,11,ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(76,24);
    tft.print(numbers[2][3]);
    tft.setCursor(83,24);
    tft.print(numbers[2][2]);
    tft.setCursor(88,24);
    tft.print(":");
    tft.setCursor(93,24);
    tft.print(numbers[2][1]);
    tft.setCursor(100,24);
    tft.print(numbers[2][0]);
  }
  else if(c==1){
    tft.fillRect(0,22,72,11,ST77XX_BLACK);
    tft.setCursor(2, 24);
    tft.setTextColor(ST77XX_WHITE);
    tft.print(strings[2]);
    tft.fillRect(74,22,36,11,ST77XX_BLACK);
    tft.fillRect(75,22,7,11,ST77XX_WHITE);
    tft.setTextColor(ST77XX_BLACK);
    tft.setCursor(76,24);
    tft.print(numbers[2][3]);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(83,24);
    tft.print(numbers[2][2]);
    tft.setCursor(88,24);
    tft.print(":");
    tft.setCursor(93,24);
    tft.print(numbers[2][1]);
    tft.setCursor(100,24);
    tft.print(numbers[2][0]);
  }
  else if(c==2){
    tft.fillRect(0,22,72,11,ST77XX_BLACK);
    tft.setCursor(2, 24);
    tft.setTextColor(ST77XX_WHITE);
    tft.print(strings[2]);
    tft.fillRect(74,22,36,11,ST77XX_BLACK);
    tft.fillRect(82,22,7,11,ST77XX_WHITE);
    tft.setTextColor(ST77XX_BLACK);
    tft.setCursor(83,24);
    tft.print(numbers[2][2]);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(76,24);
    tft.print(numbers[2][3]);
    tft.setCursor(88,24);
    tft.print(":");
    tft.setCursor(93,24);
    tft.print(numbers[2][1]);
    tft.setCursor(100,24);
    tft.print(numbers[2][0]);
  }
  else if(c==3){
    tft.fillRect(0,22,72,11,ST77XX_BLACK);
    tft.setCursor(2, 24);
    tft.setTextColor(ST77XX_WHITE);
    tft.print(strings[2]);
    tft.fillRect(74,22,36,11,ST77XX_BLACK);
    tft.fillRect(92,22,7,11,ST77XX_WHITE);
    tft.setTextColor(ST77XX_BLACK);
    tft.setCursor(93,24);
    tft.print(numbers[2][1]);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(76,24);
    tft.print(numbers[2][3]);
    tft.setCursor(83,24);
    tft.print(numbers[2][2]);
    tft.setCursor(88,24);
    tft.print(":");
    tft.setCursor(100,24);
    tft.print(numbers[2][0]);
  }
  else if(c==4){
    tft.fillRect(0,22,72,11,ST77XX_BLACK);
    tft.setCursor(2, 24);
    tft.setTextColor(ST77XX_WHITE);
    tft.print(strings[2]);
    tft.fillRect(74,22,36,11,ST77XX_BLACK);
    tft.fillRect(99,22,7,11,ST77XX_WHITE);
    tft.setTextColor(ST77XX_BLACK);
    tft.setCursor(100,24);
    tft.print(numbers[2][0]);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(76,24);
    tft.print(numbers[2][3]);
    tft.setCursor(83,24);
    tft.print(numbers[2][2]);
    tft.setCursor(88,24);
    tft.print(":");
    tft.setCursor(93,24);
    tft.print(numbers[2][1]);
  }
}


void ChangeNumber(int row, int position, int direction) {
    int max_value = 9;
    
    if (row == 2) {
        if ((position == 3)||(position == 1)) {
            max_value = 5;
        }
    }
    if (row == 0) {
        if (position == 3) {
            max_value = 4;
        }
    }
    
    if (direction == 3) {
        numbers[row][position] = (numbers[row][position] + 1) % (max_value + 1);
    } else if (direction == 4) {
        numbers[row][position] = (numbers[row][position] - 1 + (max_value + 1)) % (max_value + 1);
    }
    
    if (row == 2 && timerPaused) {
        updateRemainingTimeFromNumbers();
    }

    if (row == 0) {
        Selectnum1(b);
    } else if (row == 1) {
        if((numbers[1][3] * 100 + numbers[1][2] * 10 + numbers[1][1])>120){
            numbers[1][3]=1;
            numbers[1][2]=2;
            numbers[1][1]=0;
        }
        Selectnum2(b);
    } else if (row == 2) {
        Selectnum3(b);
    }
}

void handleAlarm() {
    if (alarmActive) {
        unsigned long currentTime = millis();
        if (currentTime - alarmStartTime >= alarmDuration) {
            // Сигнал длился 3 секунды - выключаем
            noTone(4);
            alarmActive = false;
        }
        // Сигнал продолжается, ничего не делаем
    }
}

void handleTimer() {
    if (timerRunning && !timerPaused) {
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= 1000) {
            previousMillis = currentMillis;
            if (remainingTime > 0) {
                remainingTime--;
                updateTimeDisplay();
            } else {
                // ТАЙМЕР ЗАВЕРШИЛСЯ - ОСТАНАВЛИВАЕМ МОТОР И ЗАПУСКАЕМ СИГНАЛ
                timerRunning = false;
                timerPaused = false;
                buttonText = "Enter";
                updateButtonDisplay();
                
                // ОСТАНОВКА МОТОРА
                digitalWrite(7,LOW);
                motor(flagg, 0);
                flagg=0;
                
                // ЗАПУСК СИГНАЛА
                alarmActive = true;
                alarmStartTime = millis();
                tone(4, 1000); // Запускаем непрерывный сигнал
            }
        }
    }
}

// Основная функция обработки меню (вызывается из loop)
void handleMenuInput() {
    if (!inputReady) return;
    
    if (a == 0 && b == 0 && (direction == 3 || direction == 4)) {
        saveTargetTemperature();
    }
    
    // Обработка навигации по строкам
    if (b == 0) {
        if (direction == 4) {
            a = (a + 5) % 4;
            Selectstr(a);
        } else if (direction == 3) {
            a = (a + 3) % 4;
            Selectstr(a);
        }
    }

    // Обработка кнопки Enter/Stop/Resume
    // Обработка кнопки Enter/Stop/Resume
if (a == 3 && direction == 1) {
    if (strcmp(buttonText, "Enter") == 0) {
        // СОХРАНЕНИЕ ВСЕХ ЗНАЧЕНИЙ ПРИ ЗАПУСКЕ ТАЙМЕРА
        saveTargetValues();
        remainingTime = calculateTotalTime();
        targetTime = remainingTime;  // Сохраняем целевое время
        timerRunning = true;
        timerPaused = false;
        previousMillis = millis();
        buttonText = "Stop";
        updateButtonDisplay();
        valuesSaved = true;
        
        // ЗАПУСК МОТОРА - флаг = 1 (работа)
        digitalWrite(7,HIGH);
        motor(flagg, targetRPM);
        digitalWrite(6,HIGH);
        
    } else if (strcmp(buttonText, "Stop") == 0) {
        timerPaused = true;
        buttonText = "Resume";
        updateButtonDisplay();
        
        // ОСТАНОВКА МОТОРА - флаг = 0 (стоп)
        flagg = 0;
        digitalWrite(7,LOW);
        digitalWrite(6,LOW);
        
    } else if (strcmp(buttonText, "Resume") == 0) {
        timerPaused = false;
        previousMillis = millis();
        buttonText = "Stop";
        updateButtonDisplay();
        
        // ВОЗОБНОВЛЕНИЕ МОТОРА - флаг = 1 (работа)
        digitalWrite(7,HIGH);
        digitalWrite(6,HIGH);
        motor(flagg, targetRPM);
        
    }
}

    // Обработка изменения значений
    if (b > 0 && (!timerRunning || timerPaused)) {
    if (direction == 3 || direction == 4) {
        ChangeNumber(a, 4-b, direction);
        // Автосохранение температуры при изменении
        if (a == 0) {
            saveTargetTemperature();
            forceTemperatureRedraw = true;  // Принудительно перерисуем температуру после выхода из редактирования
        }
    }
}
    
    // Навигация по цифрам
    if (a == 0) {
        if (direction == 1) {
            b = (b + 5) % 4;
            Selectnum1(b);
        }
        if (direction == 2) {
            b = (b + 3) % 4;
            Selectnum1(b);
        }
    } else if (a == 1) {
        if (direction == 1) {
            b = (b + 5) % 4;
            Selectnum2(b);
        }
        if (direction == 2) {
            b = (b + 3) % 4;
            Selectnum2(b);
        }
    } else if (a == 2) {
        if (direction == 1) {
            b = (b + 6) % 5;
            Selectnum3(b);
        }
        if (direction == 2) {
            b = (b + 4) % 5;
            Selectnum3(b);
        }
    }
    
    inputReady = false;
}

void setup() {
    Serial.begin(9600);
    pinMode(5, OUTPUT);
    pinMode(2, OUTPUT);
    pinMode(encoderPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(encoderPin), countPulse, RISING);
    saveTargetValues();

    // Инициализация дисплея
    tft.initR(INITR_BLACKTAB);
    tft.fillScreen(ST77XX_BLACK);
    
    // Инициализация отображения
     tft.fillRect(0,0,72,11,ST77XX_WHITE);
  tft.setCursor(2, 2);
  tft.setTextColor(ST77XX_BLACK);
  tft.setTextWrap(true);
  tft.print(strings[0]);
  tft.setCursor(2, 13);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextWrap(true);
  tft.print(strings[1]);
  tft.setCursor(2, 24);
  tft.print(strings[2]);
  tft.setCursor(26,35);
  tft.print(strings[3]);

  // Это температура
  tft.setCursor(76,2);
  tft.print(numbers[0][3]);
  tft.setCursor(83,2);
  tft.print(numbers[0][2]);
  tft.setCursor(88,2);
  tft.print(".");
  tft.setCursor(94,2);
  tft.print(numbers[0][1]);
  tft.setCursor(101,2);
  tft.print("C");

  // Это обороты
  tft.setCursor(76,13);
  tft.print(numbers[1][3]);
  tft.setCursor(83,13);
  tft.print(numbers[1][2]);
  tft.setCursor(90,13);
  tft.print(numbers[1][1]);

  // Это время
  tft.setCursor(76,24);
  tft.print(numbers[2][3]);
  tft.setCursor(83,24);
  tft.print(numbers[2][2]);
  tft.setCursor(88,24);
  tft.print(":");
  tft.setCursor(93,24);
  tft.print(numbers[2][1]);
  tft.setCursor(100,24);
  tft.print(numbers[2][0]);
    
    ds.begin();                       // начало считывания информации с датчика
    ds.setWaitForConversion(false);
    pinMode(4, OUTPUT); 
    pinMode(6, OUTPUT);           // управление реле
    pinMode(7, OUTPUT);           // второе реле

    // Настройка прерывания по таймеру
    noInterrupts();
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;
    
    OCR1A = 780;
    TCCR1B |= (1 << WGM12);
    TCCR1B |= (1 << CS12) | (1 << CS10);
    TIMSK1 |= (1 << OCIE1A);
    interrupts();
}

void countPulse() {
  pulseCount++;
}

void motor(int fl, int rpms){
    float znach=255/200*rpms;
    if(fl==0){
        analogWrite(5, 255);
    }
    else{
        analogWrite(5,(int)znach);
    }
    // Флаг увеличивается после первого запуска функции
    if (fl == 0) {
        flagg = 1;  // Первый запуск - устанавливаем флаг в 1
    }
    delay(50);  // Задержка после вызова функции
}

void saveTargetTemperature() {
    targetTemperature = numbers[0][3] * 10.0 + numbers[0][2] + numbers[0][1] * 0.1;
}

// Функция сохранения целевых RPM
void saveTargetRPM() {
    targetRPM = numbers[1][3] * 100 + numbers[1][2] * 10 + numbers[1][1];
}

// Функция сохранения всех значений
void saveTargetValues() {
    saveTargetTemperature();
    saveTargetRPM();
    targetTime = calculateTotalTime();
}

// Функция обновления отображения температуры (текущая или целевая)
void updateTemperatureDisplay() {
    // Если находимся на строке температуры и редактируем цифры - НЕ обновляем температуру
    if (a == 0 && b > 0) {
        return;  // Не обновляем температуру во время редактирования цифр
    }
    
    float tempToDisplay;
    
    // Если находимся на строке температуры - показываем целевое значение
    if (a == 0) {
        tempToDisplay = targetTemperature;
    } else {
        // Иначе показываем текущую измеренную температуру
        tempToDisplay = currentTemperature;
    }
    
    // Обновляем только если значение изменилось
    if (tempToDisplay != previousDisplayedTemp || forceTemperatureRedraw) {
        tft.fillRect(74,0,36,11,ST77XX_BLACK);
        tft.setTextColor(ST77XX_WHITE);
        tft.setCursor(76,2);
        
        // Форматируем вывод температуры
        int wholePart = (int)tempToDisplay;
        int decimalPart = (int)((tempToDisplay - wholePart) * 10 + 0.1);
        
        tft.print(wholePart / 10);      // Десятки
        tft.print(wholePart % 10);      // Единицы
        tft.print(".");
        tft.print(decimalPart);         // Десятые
        tft.print("C");
        
        previousDisplayedTemp = tempToDisplay;
        forceTemperatureRedraw = false;
    }
}

// Обработчик прерывания таймера
ISR(TIMER1_COMPA_vect) {
    timerISR();
}

void loop() {
    unsigned long currentTime = millis();
    
    // Обновление дисплея
    if (currentTime - previousDisplayTime >= displayInterval) {
        handleTimer();
        handleMenuInput();
        handleAlarm(); // Добавляем обработку сигнала
        
        // ВСЕГДА обновляем отображение температуры, кроме случаев редактирования цифр
        if (!(a == 0 && b > 0)) {
            updateTemperatureDisplay();
        }
        
        previousDisplayTime = currentTime;
    }
    
    // Управление датчиком температуры
    if (!sensorReading) {
        // Запрашиваем новое измерение
        if (currentTime - previousSensorTime >= sensorInterval) {
            ds.requestTemperatures(); 
            sensorRequestTime = currentTime;
            sensorReading = true;
            previousSensorTime = currentTime;
        }
    } else {
        // Проверяем, готовы ли данные (750ms для DS18B20)
        if (currentTime - sensorRequestTime >= 750) {
            currentTemperature = ds.getTempCByIndex(0);
            sensorReading = false;
            sensorDataReady = true;
        }
    }
    if (currentTemperature > targetTemperature + 0.1)         // проверка на соответсвие температуры требуемой (с погрешностью)
  {
    digitalWrite(6, HIGH);      // отключение нагревателя
  }
  if (currentTemperature < targetTemperature - 0.1)
  {
    digitalWrite(6, LOW);       // включение нагревателя
  }
}
