# horrorVacui
Sistema de inteligencia artificial para detectar el rellenado de silencios que suelen llevar a cabo los oradores. El dispositivo muestrea fragmentos de audio y los envia al modelo entrenado para su inferencia bajo un algoritmo de clasificación. En el caso de superar cierto límite de detección, suma un contador que es mostrado en pantalla.

Fue desarrollado para la charla de Inteligencia Artificial de TICMAS y UCA Rosario "La transformación digital... sin limites"

Este repositorio solo es de utilidad en el caso que quieran replicar el dispositivo con Arduino. Si quieren entrenar un modelo de Machine Learning desde 0 y probarlo desde la computadora, estos archivos e instrucciones no son necesario.

# Partes
Arduino Nano BLE 33 Sense
Pantalla Oled DFRobot 0.91 (la pantalla se conecta con los pins SDA A4 y SCL A5, Vin a VCC y GND a Ground)

# Librerías
Es necesario instalar la librería U8g2lib.h para la pantalla Oled y la librería del modelo entrenado en Edge Impulse (contenida en este repositorio)

# Contacto
Roni Bandini @ronibandini
Mayo de 2023

