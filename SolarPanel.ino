//Библиотеки
#include <IRremote.h>   //Для ир приемника
#include <GyverTimer.h> //Для таймеров
#include <GyverPower.h> //Для энергопотребления, режима сна

//ПИНЫ
#define SOLARPANEL_PIN A0 //Отсюда будет измерение напряжения с солнечной панели
#define IR_RECIVER_PIN 2  //Сигнальный пин приемника
#define LAMP_PIN 4        //Лампа, которой будем управлять

//IR

//#define IR_AVAILABLE 1    //Убрать эту строчку, чтобы отключить поддержку IR приемника

//СИГНАЛЫ IR
//Определяются отдельно для каждого приемника и/или пульта
#define ON_OFF_COMMAND 3810010651
#define AUTOMODE_COMMAND 5316027

//Уровни напряжения
//Значение напряжения, при котором контроллер посчитает напряжение с солнечной панели низким
#define SOLARPANEL_VOLTAGE_MIN 1024/2 // 2.5 Volt

//Настройки таймера(В мс)
#define SPL_TIMER_PERIOD 3000    //Защищает от ложных переключений лампы из-за коротковременного изменения уровня напряжения на солнечной панели.
//Переключение состояние лампы произойдет только если уровень напряжения останется одинаковым на протяжении этого времени
#define SLEEP_AFTER_MEASURE 600  //Задает время, на которое микроконтроллер засыпаем между промежуточными измерениями уровня напряжения

//Состояние контроллера
enum ControllerState : byte {
  MANUALMODE,
  MEASURE,
  WAIT
};

//Глобальные переменные
GTimer SPVTimer(MS); //Таймер

#ifdef IR_AVAILABLE
IRrecv irrecv(IR_RECIVER_PIN); //Обработчик IR приемника
decode_results results; //Результат декодирования ir сигнала
#endif

volatile ControllerState ControllerState = MEASURE; //Переменная содержит текущее состояние контролллера
bool SPVoltageBuff = false; //Здесь запасаем значение с солнечной панели, чтобы после срабатывания таймера сравнить с текущим состоянием
bool LampState = false; //Текущее состояние лампы


void setup() {
  // Настройка пинов
  pinMode(SOLARPANEL_PIN, INPUT);
  pinMode(LAMP_PIN, OUTPUT);

#ifdef IR_AVAILABLE
  pinMode(IR_RECIVER_PIN, INPUT);
  //IRrecive
  irrecv.enableIRIn();//Запускаем прием
  //Настройка внешнего прерывания
  attachInterrupt(0, ListenIR, RISING);
#endif

  //Энергопотребление
  power.autoCalibrate(); // автоматическая калибровка длительности сна ~ 2 секунды , средняя но достаточная точность
  /*
    POWERDOWN_SLEEP – Наиболее глубокий сон, отключается всё кроме WDT и внешних прерываний, просыпается от аппаратных (обычных + PCINT) или WDT, пробуждение за 1000 тактов (62 мкс)
    STANDBY_SLEEP – Глубокий сон, идентичен POWERDOWN_SLEEP + system clock активен, пробуждение за 6 тактов (0.4 мкс)
  */
  power.setSleepMode(STANDBY_SLEEP); //Установка режима сна.
}

void loop() {
  switch (ControllerState) {
    //Ручной режим.
    //Переключение состояния лампы происходит нажатием нужной кнопки. До следующей команды контроллер неактивен


    case MANUALMODE:
#ifdef IR_AVAILABLE
      SPVTimer.stop();
      digitalWrite(LAMP_PIN, LampState);
      power.sleep(SLEEP_FOREVER);//Засыпаем вплоть до нового сигнала от IR
#else
      ControllerState = MEASURE;
#endif
      break;


    //Режим измерения напряжения солнечной панели
    case MEASURE:
      SPVoltageBuff = GetSolarPanelVoltageLevel();
      SPVTimer.setInterval(SPL_TIMER_PERIOD);
      ControllerState = WAIT;
      power.sleepDelay(SLEEP_AFTER_MEASURE);
      break;

    //Режим ожидающий что сигнал станет низким
    case WAIT:
      //Защита от скачков и пограничных состояний между включением и выключением
      if (SPVoltageBuff != GetSolarPanelVoltageLevel()) { //Если до таймера значение опять изменилось, значит переключение состояния еще не нужно
        ControllerState = MEASURE;
      } else if (SPVTimer.isReady()) {
        if (GetSolarPanelVoltageLevel() == SPVoltageBuff) {
          LampState = !SPVoltageBuff; //Если за время таймера значение оставалось стабильным, значит можно переключать состояние лампы на это самое значение
          digitalWrite(LAMP_PIN, LampState);
          ControllerState = MEASURE;
        }
      }
      else
      {
        power.sleepDelay(SLEEP_AFTER_MEASURE);//Если таймер не готов и
      }
      break;

    default:
      ControllerState = MEASURE;
      break;
  }
}


//Функция определяет уровень напряжения на солнечной панели(Высокий или низкий) учитывая заданный порог SOLARPANEL_VOLTAGE_MIN
bool GetSolarPanelVoltageLevel() {
  if (analogRead(SOLARPANEL_PIN) > SOLARPANEL_VOLTAGE_MIN)
    return true;
  else
    return false;
}

#ifdef IR_AVAILABLE
//Обработчик IR команд
void ListenIR() {
  //Если расшифрован новый сигнал
  power.wakeUp();
  if (irrecv.decode(&results)) {
    //В зависимости от сигнала
    Serial.println(results.value, HEX);
    switch (results.value) {

      //Ручное переключение
      case ON_OFF_COMMAND:
        ControllerState = MANUALMODE; //переключение режима
        LampState = !LampState; //переключение режима лампы
        break;

      //Режим автоматической регулировки
      case AUTOMODE_COMMAND:
        ControllerState = MEASURE;//переключение режима
        break;
    }
  }
}
#endif
