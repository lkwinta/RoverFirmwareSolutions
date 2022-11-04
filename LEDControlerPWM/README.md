# LEDControlerPWM
W tym rozwiązaniu założyłem że do dyspozycji mam zwykły LED RGB ze wspólną anodą i trzema katodami dla każdego koloru.
Cały kod do obsługi tego LEDa znajduje się w pliku main.c

# Podłączenie:
   - PA6 - katoda czerwonego LEDa (przez opornik 330Ω)
   - PA7 - katoda zielonego LEDa (przez opornik 330Ω)
   - PB0 - katoda niebieskiego LEDa (przez opornik 1kΩ)

   - 5V - wspólna anoda
 
   - Debugger - PA14 (SWCLK) and PA13 (SWDIO)
