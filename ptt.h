#pragma once

//PTT is active low
bool IsPttPressed();

//Configures the pin only
void initPtt();

//Starts posting a PttChanged event on every edge of the PTT pin, from interrupt context
void enablePttEvents();
