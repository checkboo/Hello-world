#include <Arduino.h>

#include "AppState.h"

namespace journal { static App g_app; }

void setup() { journal::g_app.begin(); }
void loop()  { journal::g_app.loop();  }
