# Balanza Arduino - Documentación

## Descripción

Este proyecto de Arduino implementa una balanza que ofrece diferentes modos de operación y opciones de calibración. Está diseñado para ser fácil de usar. En esta documentación se describen todos los modos de operación y los comandos disponibles para interactuar con la balanza.

## Características

- **Modos de Operación:**
  - Modo de Medición Continua.
  - Modo de Medición Continua con cierto intervalo de tiempo.
  - Modo a Requerimiento.
  - Modo a Requerimiento con cierto intervalo de tiempo.
  - Modo Estable.
  - Modo para seleccionar unidad de medida.
  - Modo de Tara.
  - Modo de Calibración.
    
- **Comandos de Control:**
  - Cambiar entre modos.
  - Realizar mediciones.
  - Calibrar la balanza.

- **Salida de Datos:**
  - Resultados en tiempo real a través de la interfaz serial.



## Modos de Operación

### 1. Modo de Calibración
Utiliza este modo para calibrar la balanza y asegurarte de que las mediciones sean precisas.

- **Comando para activar:** `<STARTCAL>`
- **Instrucciones:**
  1. Coloca un peso conocido en la balanza.
  2. Ingresa el valor del peso en gramos usando el comando `<WEIGHT:peso>`.
  3. Repita este proceso las veces que considere necesario.
  4. Para terminar con la calibración use el comando `<ENDCAL>`.
  5. La balanza ajustará automáticamente sus mediciones basándose en este valor.

### 2. Modo de Medición Continua
Este modo permite realizar mediciones continuas, útil para monitorear cambios de peso en tiempo real.

- **Comando para activar:** `<MODE:CONT>`
- **Funcionamiento:** La balanza envía los datos de peso continuamente a la interfaz serial.

### 3. Modo de Medición Continua con Intervalo
En este modo, al igual que el anterior, envia datos del peso de manera continua, pero lo hace cada cierto intervalo de tiempo establecido por el usuario.

- **Comando para activar:** `<MODE:CONT-tiempo>`
- **Funcionamiento:** La balanza envía los datos de peso continuamente a la interfaz serial cada cierto intervalo de tiempo establecido.

### 4. Modo a Requerimiento 
Este modo permite al usuario decidir cuando desea recibir un dato de la balanza.

- **Comandos para activar:** `<MODE:REQ>` y `<M>`
- **Funcionamiento:** La balanza no hace nada hasta que el usuario solicite un dato a traves del comando `<M>`. Una vez puesto el comando, la balanza mandará el primer dato disponible y te dira si el dato es estable o no.
- **Condiciones:** La balanza determinará si un dato es estable o no si la variación de peso en los últimos segundos se mantiene menor a una tolerancia de 1 gramo.

### 5. Modo a Requerimiento con Intervalo
Este modo permite al usuario decidir cuando desea recibir un dato de la balanza y se calcula el promedio de los datos recibidos en el intervalo establecido.

- **Comandos para activar:** `<MODE:REQ-tiempo>` y `<M>`
- **Funcionamiento:** La balanza no hace nada hasta que el usuario solicite un dato a traves del comando `<M>`. Una vez puesto el comando, se empezaran a recoger datos de la balanza durante el intervalo de tiempo establecido, se promediarán esos datos y los informará.


### 6. Modo Estable
Este modo permite al usuario recibir la primera lectura estable que se detecte. No se vuelve a mandar nada excepto que el peso se estabilice nuevamente en otro valor distinto al anterior.

- **Comandos para activar:** `<MODE:STB>`
- **Funcionamiento:** La balanza mide permanentemente y cuando detecta que la variación de peso es menor a un umbral de 1 gramo durante 3 segundos, manda el dato por el puerto serie una única vez.


## Comandos Disponibles

Aquí tienes una lista de todos los comandos que puedes utilizar:

- `<MODE:CONT>` - Activa el modo de Medición Continua.
- `<MODE:CONT-tiempo>` - Activa el modo de Medición Continua con Intervalo.
- `<MODE:REQ>` - Activa el modo a Requerimiento.
- `<MODE:REQ-tiempo>` - Activa el modo a Requerimiento con Intervalo.
- `<M>` - Solicita el dato a la balanza unicamente cuando algún modo a Requerimiento este activo.
- `<MODE:STB>` -  Activa el modo Estable.
- `<UNITS:REAL>` - Establece la unidad de medida a mostrar en gramos.
- `<UNITS:COUNTS>` - Establece la unidad de medida a mostrar en cuentas.
- `<UNITS:BOTH>` - Establece la unidad de medida a mostrar en gramos y en cuentas.
- `<TARE>` - Tara la balanza mostrando en todo momento el peso real sobre la balanza.
- `<STARTCAL>` - Inicia la calibración de la balanza.
- `<WEIGHT:peso>` - Ingresa el peso que se ha puesto sobre la balanza.
- `<ENDCAL>` - Termina con la calibración de la balanza.


## Uso

1. Conecta la balanza Arduino a tu computadora y abre el monitor serial.
2. Ingresa los comandos según el modo que desees utilizar.
3. Sigue las instrucciones para realizar las mediciones o configuraciones necesarias.

## Instalación

1. Clona este repositorio en tu máquina local.
2. Sube el código al Arduino utilizando el IDE de Arduino.
3. Conecta los sensores y dispositivos periféricos necesarios según el esquema proporcionado.


