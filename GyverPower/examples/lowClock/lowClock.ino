// пример управления системной частотой
// мы можем только уменьшить (разделить) системную частоту (на платах ардуино 16 МГц)
// Пониженная частота позволяет чуть снизить потребление или питать МК от пониженного напряжения!

#include <GyverPower.h>

void setup() {
  power.setSystemPrescaler(PRESCALER_16); 	// замедляем в 16 раз
  
  // с понижением системной частоты "уйдут" все завязанные на частоте блоки периферии!
  // чтобы сериал завёлся (если нужен), умножаем скорость на замедление
  // иначе не заведётся на указанной скорости
  Serial.begin(9600 * 16L);  

  Serial.println("serial test");
}

void loop() {
}
