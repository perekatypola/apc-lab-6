#include <stdio.h>
#include <dos.h>
#include <string.h>
#include <io.h>
#include <time.h>
#include <stdlib.h>

#define STDOUT 1

typedef void interrupt far (*Interrupt)(void);

void interrupt far keyboardHandler(void);

int running = 1;
int blinking = 0;
int commandSuccess = 0;
int commandProcessed = 0;

void blink(void);
int sendMask(int mask);

int main() {
        Interrupt oldKeyboard;

        /*
        * Инициализация генератора случайных чисел часами реального времени.
        */
        srand(time(NULL));
        puts("Press F to blink and ESC to exit");

        /*
        * Отключить все прерывания.
        */
        _disable();

        /*
        * Переопределение системного вектора.
        */
        oldKeyboard = getvect(0x09);
        setvect(0x09, keyboardHandler);


        /*
        * Включить все прерывания.
        */
        _enable();

        while (running) {
                if (blinking) {
                        blink();
                }
        }

        _disable();

        /*
        * Возврат системного вектора прерывания.
        */
        setvect(0x09, oldKeyboard);
        _enable();

        return 0;
}

void interrupt far keyboardHandler(void) {

        /*
        * 0x60 - Порт данных контроллера клавиатуры.
        * Хранит скан-код последней нажатой клавиши.
        */
        int code = inp(0x60);
        char str[5];

        /*
        * Запись в строку-буфер скан-кода.
        */
        sprintf(str, "%02X  ", code);
        /*
        * Вывод строки в консоль.
        */
        write(STDOUT, str, 4);

        switch (code) {

                /*
                * ESC
                */
                case 0x01: running = 0; break;

                /*
                * 'F'
                */
                case 0x21: blinking = !blinking; break;

                /*
                * 0xFA - произошла ошибка.
                */
                case 0xFE:
                        commandSuccess = 0;
                        commandProcessed = 1;
                        break;

                /*
                * 0xFA - код обработан успешно.
                */
                case 0xFA:
                        commandSuccess = 1;
                        commandProcessed = 1;
                        break;
        }

                /*
                * 'F'
                */
        outp(0x20, 0x20);// сообщает контроллеру прерываний , что прерывание обработано
}

void blink() {
        int mask = rand() % 0x08;
        if (!sendMask(mask)) {
                running = 0;
                return;
        }
        delay(100);
}

int sendMask(int mask) {
        return sendByte(0xED) ? sendByte(mask) : 0; //команда для моргания/мигания
}

int sendByte(int byte) {
        int errorCount;
        for (
                errorCount = commandSuccess = 0;
                errorCount < 3 && !commandSuccess;
                ++errorCount
        ) {
                commandProcessed = 0;
                while (inp(0x64) & 0x02);// подождать пока ...
                outp(0x60, byte);
                while(!commandProcessed);
        }
        return errorCount < 3;
}

